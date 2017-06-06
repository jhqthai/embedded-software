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
static void (*I2CCallback)(void*); // User callback function pointer
static void* I2CArguments; // User arguments pointer to use with user callback function

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
	// Check for pending interrupt flag
	while (!(I2C0_S & I2C_S_IICIF_MASK));

	// Clear interrupt flag
	I2C0_S |= I2C_S_IICIF_MASK;
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

bool I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
	// Set received baud rate to private variable
	uint32_t receivedBaudRate;
	receivedBaudRate = aI2CModule->baudRate; // Check naming convention again!

	// Set received variable to private global variable
	SlaveAddr = aI2CModule->primarySlaveAddress;
	I2CCallback = aI2CModule->readCompleteCallbackFunction;
	I2CArguments = aI2CModule->readCompleteCallbackArguments;

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
	I2C0_C1 &= ~I2C_C1_MST_MASK; //Set for Master Mode Select - Slave mode
	I2C0_C1 &= ~I2C_C1_WUEN_MASK; //Set for Wakeup Enable - Normal operation. No interrupt generated when address matching in low power mode
	I2C0_C1 &= ~I2C_C1_DMAEN_MASK; //Set for DMA Enable - All DMA signaling disabled
	I2C0_C1 &= ~I2C_C1_IICIE_MASK; //Set for IICIE - Disable of I2C interrupt requests

	// Clear I2C glitch filter
	I2C0_FLT = 0;

	// Disable I2C general call address
	//I2C0_C2 &= ~I2C_C2_GCAEN_MASK;

	// Varialbes for baud rate calculation
	uint8_t mulCount = 0;
	uint8_t i;
	uint8_t tempBaudRate;
	uint8_t prescaleBaudRate = 0;

	// Applying exhaustive search to find an achievable baud rate that is closest to the requested baud rate
	// I2C baud rate = I2C module clock speed (Hz)/(mul × SCL divider)
	for (mulCount = 0; mulCount < 3; mulCount++)
	{
		for (i=0; i<sizeof(SCL_DIV_VAL); i++)
		{
			tempBaudRate = moduleClk/(MUL_VAL[mulCount]*SCL_DIV_VAL[i]);

			//Compare calculated divider value with SCL divider for closet value
			if (tempBaudRate < receivedBaudRate && prescaleBaudRate < tempBaudRate)
			{
				prescaleBaudRate = tempBaudRate;
				I2C0_F = I2C_F_MULT(mulCount); 				// Set most achievable multiplier
				I2C0_F = I2C_F_ICR(prescaleBaudRate); // Set most achievable baud rate
			}
		}
	}

	// Initial slave routine selection (NEW)
	//I2C_SelectSlaveDevice(SlaveAddr);

	// Enable I2C interrupt
	//I2C0_C1 |= I2C_C1_IICIE_MASK;

	// Enable I2C module operation
	I2C0_C1 |= I2C_C1_IICEN_MASK;

	//Initilise NVIC
	// Clear any pending interrupt on I2C
	NVICICPR0 = (1<<24);
	// Set enable interrupt for I2C
	NVICISPR0 = (1<<24);

	return true;

}


void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{
	// Set received slave address to access globally
	SlaveAddr = slaveAddress;
}

// MOVE ME (FIX)
static bool I2CBusWait ()
{
	// Wait for bus idle
	if ((I2C0_S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK)
		return true;
	return false;
}

void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{

	// Wait for bus idle
	I2CBusWait();

	// Start signal and transmit mode
	I2C0_C1 |= (I2C_C1_MST_MASK | I2C_C1_TX_MASK);

	//Check for arbitration loss
	//if (I2C0_S & I2C_S_ARBL_MASK)
	//	I2CStop(); // Initiate stop signal

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

}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses polling as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_PollRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	// Set variables to global variables for interrupt
	ReadDestination = data;
	TxNumBytes = nbBytes;

	// Wait for bus idle
	while ((I2C0_S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK) ;

	// Clear interrupt flag (NEW)
	//I2C0_S |= I2C_S_IICIF_MASK;

	// Start signal and transmit mode
	I2C0_C1 |= (I2C_C1_MST_MASK| I2C_C1_TX_MASK);

	// Check for arbitration loss
	//if (I2C0_S & I2C_S_ARBL_MASK)
	//	I2CStop(); // Initiate stop signal

	// 1st byte - slave address (7 bits) + write (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x00;
	I2CInterruptWait();

	// 2nd byte - Register address
	RegAddr = registerAddress;
	I2C0_D = RegAddr;
	I2CInterruptWait();

	// Start repeat
	I2C0_C1 |= (I2C_C1_RSTA_MASK);

	// 3rd byte - slave address (7 bits) + read (1 bit)
	I2C0_D = (SlaveAddr << 1) | 0x01;
	I2CInterruptWait();

	// Select receive mode
	I2C0_C1 &= ~I2C_C1_TX_MASK;
	I2C0_C1 &= ~I2C_C1_TXAK_MASK; // AK

	// Check each byte before last byte and AK it
	for(int count = 0; count < TxNumBytes - 1; count++)
	{
		*ReadDestination = I2C0_D;
		I2CInterruptWait();
		//I2C0_C1 &= ~I2C_C1_TXAK_MASK; // AK

		ReadDestination++;
	}
	// otherwise last byte NAK
	I2C0_C1 |= I2C_C1_TXAK_MASK; // NAK = 1


	I2CStop();

	ReadDestination[TxNumBytes-1] = I2C0_D;
	I2CInterruptWait();


}


void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{
	/*
	// Enable I2C interrupt service
	I2C0_C1 |= I2C_C1_IICIE_MASK;

	// Runs PollRead routine
	I2C_PollRead(registerAddress, data, nbBytes);
	I2CInterruptWait();
	*/

	// Wait for bus idle
	while ((I2C0_S & I2C_S_BUSY_MASK) == I2C_S_BUSY_MASK);

	TxNumBytes = nbBytes;
	ReadDestination = data;
	InterruptCount = 0;

	I2C0_C1 |= (I2C_C1_MST_MASK | I2C_C1_TX_MASK); // Start and set to Transmit
	I2C0_D = (SlaveAddr << 1 | 0x00);			// Slave address transmission with write
	I2CInterruptWait();
	I2C0_D = registerAddress;			// Register address to write to
	I2CInterruptWait();
	I2C0_C1 |= I2C_C1_RSTA_MASK;			// Repeat Start
	I2C0_D = ((SlaveAddr << 1) | 0x01);		// Slave address transmission with read
	I2CInterruptWait();

  I2C0_C1 &= ~I2C_C1_TX_MASK;			// Write: Control Register 1 to enable RX

	I2CStop();

}


void __attribute__ ((interrupt)) I2C_ISR(void)
{
	/*
	// Handle interrupt
	// Check if interrupt is enabled
	if ((I2C0_C1 & I2C_C1_IICIE_MASK))
	{
		// Check interrupt flag is enabled
		if ((I2C0_S & I2C_S_IICIF_MASK))
		{
			// Clear pending interrupt
			I2C0_S |= I2C_S_IICIF_MASK;

			// Call I2C int read to read data
			I2C_IntRead(RegAddr, ReadDestination, TxNumBytes);
		}
	}

	// Handle callback
	if (I2CCallback)
		(*I2CCallback)(I2CArguments);
	*/


  I2C0_S |= I2C_S_IICIF_MASK;
  if (I2C0_S & I2C_S_TCF_MASK)
    {
      if (!(I2C0_C1 & I2C_C1_TX_MASK))	// If receiving is set
      {
	if (TxNumBytes > 1)
	{
			ReadDestination[InterruptCount] = I2C0_D;
	}
	else
	{

	  I2C0_C1 |= I2C_C1_TXAK_MASK;
	  ReadDestination[InterruptCount] = I2C0_D;
	  I2CStop();
	}
	TxNumBytes--;
	InterruptCount++;
      }
    }

  I2CCallback(I2CArguments);
}

/*!
 * @}
*/


