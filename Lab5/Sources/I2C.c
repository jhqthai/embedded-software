/*! @file
 *
 *  @brief Implementation of I/O routines for the K70 I2C interface.
 *
 *  This contains the functions definition for operating the I2C (inter-integrated circuit) module.
 *
 *  @author John Thai, Jason Gavriel
 *  @date 2017-05-08
 */
/*!
**  @addtogroup I2C_module I2C module documentation
**  @{
*/
/* MODULE I2C */

#include "I2C.h"
#include "MK70F12.h"

// Global variables
static uint8_t SlaveAddr;
static uint8_t *ReadDestination; // Where we store data that is read
static uint8_t RegAddr; // Register address
static uint8_t TxNumBytes; // Number of bytes requested
static uint8_t InterruptCount;

// Arrays for I2C baud rate calculation (K70 p.1863)
const static uint8_t MUL_VAL [] = {0x01, 0x02, 0x04 };
const static uint16_t SCL_DIV_VAL[] = {20,22,24,26,28,30,32,34,36,40,44,48,56,64,68,
		72,80,88,96,104,112,128,144,160,192,224,240,256,288,320,384,448,480,512,
		576,640,768,896,960,1024,1152,1280,1536,1792,1920,2048,2304,2560,3072,3840};


/*! @brief Indicate that an event has occur on the I2C bus
 *
 * @return void
 */
static void I2CInterruptWait()
{
	uint8_t i2csreg;
	// Check for pending interrupt flag
	while (!(I2C0_S & I2C_S_IICIF_MASK))
	{
		i2csreg = I2C0_S;
	}

	// Clear interrupt flag !!! write 1 to clear
	I2C0_S = I2C_S_IICIF_MASK;
}

/*! @brief Wait for the I2C bus mask to idle
 *
 * @return void
 */
static void I2CBusWait ()
{
	// Wait for bus idle
	while ((I2C0_S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK);
}

/*! @brief Start signal and transmit mode
 *
 * @return void
 */
static void I2CStart()
{
	// Start signal and transmit mode
	I2C0_C1 |= (I2C_C1_MST_MASK | I2C_C1_TX_MASK);
}

/*! @brief Generate stop signal to free I2C bus
 *
 * @return void
 */
static void I2CStop()
{
	// Clear master mode bit to generate stop signal
	I2C0_C1 &= ~(I2C_C1_MST_MASK);
	// Clear transmit mode bit to select receive mode
	I2C0_C1 &= ~(I2C_C1_TX_MASK);

}

/*! @brief Set I2C baud rate
 *
 * @return void
 */
static void BaudRate() {
	I2C0_F = I2C_F_MULT(MUL_VAL[0x00]) | I2C_F_ICR(0x23); 				// Set most achievable multiplier

//	I2C0_F = I2C_F_ICR(0x23); // Set most achievable baud rate
//	uint8_t mulCount = 0;
//	uint8_t i;
//	uint8_t tempBaudRate;
//	uint8_t prescaleBaudRate = 0;
//
//	for (mulCount = 0; mulCount < 3; mulCount++)
//	{
//		for (i=0; i<(sizeof(SCL_DIV_VAL)/sizeof(*SCL_DIV_VAL)); i++)
//		{
//			tempBaudRate = moduleClk/(MUL_VAL[mulCount]*SCL_DIV_VAL[i]);
//
//			//Compare calculated divider value with SCL divider for closet value
//			if (tempBaudRate < receivedBaudRate && prescaleBaudRate < tempBaudRate)
//			{
//				prescaleBaudRate = tempBaudRate;
//				I2C0_F = I2C_F_MULT(mulCount); 				// Set most achievable multiplier
//				I2C0_F = I2C_F_ICR(prescaleBaudRate); // Set most achievable baud rate
//				return true;
//			}
//		}
//	}
}

bool I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
	I2CWriteSemaphore = OS_SemaphoreCreate(1);
	// Set received baud rate to private variable
	uint32_t receivedBaudRate;
	receivedBaudRate = aI2CModule->baudRate; // Check naming convention again!

	// Enable I2C module clock gate
	SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK;
	// Enable PTE clock gate for pin routing of I2C (Schematics.pdf)
	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

	// PORTE_PCR18: MUX(4) for I2C0 serial data line (p.279 K70 Ref Manual)
	PORTE_PCR18 |= PORT_PCR_MUX(4);
	// PORTE_PCR19: MUX(4) for I2C0 serial clock line
	PORTE_PCR19 |= PORT_PCR_MUX(4);

	// Open drain enable to configure pin as digital output if necessary
	PORTE_PCR18 |= PORT_PCR_ODE_MASK;
	PORTE_PCR19 |= PORT_PCR_ODE_MASK;

 //Set for Control Register 1
	I2C0_C1 &= ~I2C_C1_IICEN_MASK;  // disable
	I2C0_C1 &= ~I2C_C1_MST_MASK; //Set for Master Mode Select - Slave mode
	I2C0_C1 &= ~I2C_C1_WUEN_MASK; //Set for Wakeup Enable - Normal operation. No interrupt generated when address matching in low power mode
	I2C0_C1 &= ~I2C_C1_DMAEN_MASK; //Set for DMA Enable - All DMA signaling disabled
	I2C0_C1 &= ~I2C_C1_IICIE_MASK; //Set for IICIE - Disable of I2C interrupt requests

	// Clear I2C glitch filter
	I2C0_FLT = 0;

	// Disable I2C general call address
	//I2C0_C2 &= ~I2C_C2_GCAEN_MASK;

	// Varialbes for baud rate calculation


	// Applying exhaustive search to find an achievable baud rate that is closest to the requested baud rate
	// I2C baud rate = I2C module clock speed (Hz)/(mul × SCL divider)
	BaudRate();

	I2C_SelectSlaveDevice(aI2CModule->primarySlaveAddress);

	//Initilise NVIC
	// Clear any pending interrupt on I2C
	NVICICPR0 = (1<<24);
	// Set enable interrupt for I2C
	NVICISER0 = (1<<24);

	// Enable I2C module operation
	I2C0_C1 |= I2C_C1_IICEN_MASK;

	return true;
}


void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
	// Set received slave address to access globally
	SlaveAddr = slaveAddress;
}

void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{

	// Wait for bus idle
	I2CBusWait();

	// Obtain exclusive access to write
	OS_SemaphoreWait(I2CWriteSemaphore,0);

	// Start signal and transmit mode
	I2CStart();

	// 1st byte - slave address (7 bits) + write (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x00;
	I2CInterruptWait();

	// 2nd byte - Register address
	I2C0_D = registerAddress;
	I2CInterruptWait();

	// 3rd byte - send data byte
	I2C0_D = data;
	I2CInterruptWait();

	I2CStop();

	// Relinquish exclusive access to write
	OS_SemaphoreSignal(I2CWriteSemaphore);
}


void I2C_PollRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	uint8_t dataCount;

	// Wait for bus idle and clear interrupt flag
	I2CBusWait();

	// Start signal and transmit mode
	I2CStart();

	// 1st byte - slave address (7 bits) + write (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x00;
	I2CInterruptWait();

	// 2nd byte - Register address
	I2C0_D = registerAddress;
	I2CInterruptWait();

	// Start repeat
	I2C0_C1 |= (I2C_C1_RSTA_MASK);

	// 3rd byte - slave address (7 bits) + read (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x01;
	I2CInterruptWait();

	// Select receive mode
	I2C0_C1 &= ~I2C_C1_TX_MASK;
	// ACK from Master
	I2C0_C1 &= ~I2C_C1_TXAK_MASK;

	// Dummy read
	data[0] = I2C0_D;
	I2CInterruptWait();

	// Check each byte before last byte to ACK it
	for(dataCount = 0; dataCount < nbBytes - 1; dataCount++)
	{
		data[dataCount] = I2C0_D;
		I2CInterruptWait();
	}

	// NACK from Master
	I2C0_C1 |= I2C_C1_TXAK_MASK;

	// Read second last byte
	data[dataCount++] = I2C0_D;
	I2CInterruptWait();

	I2CStop();

	// Read last byte
	data[dataCount++] = I2C0_D;
}


void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	ReadDestination = data;
	TxNumBytes = nbBytes;

	// Enable interrupt
	I2C0_C1 |= I2C_C1_IICIE_MASK;

	// Wait for bus idle and clear interrupt flag
	I2CBusWait();

	// Start signal and transmit mode
	I2CStart();

	// 1st byte - slave address (7 bits) + write (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x00;
	I2CInterruptWait();

	// 2nd byte - Register address
	I2C0_D = registerAddress;
	I2CInterruptWait();

	// Start repeat
	I2C0_C1 |= (I2C_C1_RSTA_MASK);

	// 3rd byte - slave address (7 bits) + read (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x01;
	I2CInterruptWait();

	// Select receive mode
	I2C0_C1 &= ~I2C_C1_TX_MASK;
	// ACK from Master
	I2C0_C1 &= ~I2C_C1_TXAK_MASK;

	// Dummy read
	ReadDestination[0] = I2C0_D;
	I2CInterruptWait();

}


void __attribute__ ((interrupt)) I2C_ISR(void)
{
	OS_ISREnter();

	uint8_t dataCount = 0;

	// Handle interrupt
	// Check if interrupt is enabled
	if ((I2C0_C1 & I2C_C1_IICIE_MASK))
	{
		// Check interrupt flag is enabled
		if ((I2C0_S & I2C_S_IICIF_MASK))
		{
			// Clear pending interrupt
			I2C0_S |= I2C_S_IICIF_MASK;

			// Check if in receive mode
			if (!(I2C0_C1 & I2C_C1_TX_MASK))
			{
				switch (TxNumBytes)
				{
					case (2): // Second last byte
						I2CStop();
						// Read second last byte
						ReadDestination[dataCount++] = I2C0_D;
						TxNumBytes--;
						break;

					case (1): // Last byte
						// NACK from Master
						I2C0_C1 |= I2C_C1_TXAK_MASK;
						// Read last byte
						ReadDestination[dataCount++] = I2C0_D;
						// Handle I2C thread
						OS_SemaphoreSignal(I2CSemaphore);
						break;

					default:
						// Read byte
						ReadDestination[dataCount++] = I2C0_D;
						TxNumBytes--;
						break;
				}
			}

		}
	}
OS_ISRExit();
}

/*!
 * @}
*/


