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

typedef enum
{
	mul0 = 0x01,
	mul1 = 0x02,
	mul2 = 0x04,
}mulVal; // naming convention

uint16_t sclDividerValues[] = {20,22,24,26,28,30,32,34,36,40,44,48,56,64,68,
		72,80,88,96,104,112,128,144,160,192,224,240,256,288,320,384,448,480,512,
		576,640,768,896,960,1024,1152,1280,1536,1792,1920,2048,2304,2560,3072,3840};


/*! @brief Sets up the I2C before first use.
 *
 *  @param aI2CModule is a structure containing the operating conditions for the module.
 *  @param moduleClk The module clock in Hz.
 *  @return BOOL - TRUE if the I2C module was successfully initialized.
 */
bool I2C_Init(const TI2CModule* const aI2CModule, const uint32_t moduleClk)
{
	(uin32_t) receivedBaudRate = aI2CModule->baudRate; // Check naming convention again!

	// Enable I2C module clock gate
	SIM_SCGC4 |= SIM_SCGC4_IIC0_MASK;

	// Enable PTE clock gate for pin routing of I2C (Schematics.pdf)
	SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

	// Enable PTB clock gate for pin routing of IRQ maybe not needed
	// SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

	// PORTE_PCR18: MUX(4) for I2C0 serial data line (p.279 K70 Ref Manual)
	PORTE_PCR18 |= PORT_PCR_MUX(4);

	// PORTE_PCR19: MUX(4) for I2C0 serial clock line
	PORTE_PCR19 |= PORT_PCR_MUX(4);

	// Clear I2C module operation
	I2C0_C1 = 0;

	// Disable I2C general call address
	//I2C0_C2 &= ~I2C_C2_GCAEN_MASK;

	// Clear I2C glitch filter
	I2C0_FLT = 0;

	//baud rate
	// I2C baud rate = I2C module clock speed (Hz)/(mul × SCL divider)
	//x = receivedbaudrate/moduleClk;
	uint8_t mulCount = 0;
	uint8_t i;
	uint8_t tempBaudRate;
	uint8_t prescaleBaudRate = 0;

	for (mulCount = 0; mulCount < 3; mulCount++)
	{
		for (i=0; i<sizeof(sclDividerValues); i++)
		{
			tempBaudRate = moduleClk/(mulVal->mul(mulCount)*sclDividerValues);

			//Compare calculated divider value with SCL divider for closet value
			if (tempBaudRate < receivedBaudRate && prescaleBaudRate < tempBaudRate)
			{
				prescaleBaudRate = tempBaudRate;
				I2C0_F = I2C_F_MULT(mulCount);
				I2C0_F = I2C_F_ICR(prescaleBaudRate);
			}
		}
	}

	// Enable I2C module operation
	I2C0_C1 |= I2C_C1_IICEN_MASK;

	// Enable I2C interrupt
	I2C0_C1 |= I2C_C1_IICIE_MASK;

	// Enable I2C general call address
	//I2C0_C2 |= I2C_C2_GCAEN_MASK;

	// set slaves module
	//IC20_C2 =  I2C_C2_AD(aI2CModule->primarySlaveAddress);

	//Initilise NVIC
	// Clear any pending interrupt on I2C
	NVICICPR0 = (1<<24);

	// Set enable interrupt for I2C
	NVICISPR0 = (1<<24);

	return true;


}

/*! @brief Selects the current slave device
 *
 * @param slaveAddress The slave device address.
 */
void I2C_SelectSlaveDevice(const uint8_t slaveAddress)
{

}

/*! @brief Write a byte of data to a specified register
 *
 * @param registerAddress The register address.
 * @param data The 8-bit data to write.
 */
void I2C_Write(const uint8_t registerAddress, const uint8_t data)
{

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

}

/*! @brief Reads data of a specified length starting from a specified register
 *
 * Uses interrupts as the method of data reception.
 * @param registerAddress The register address.
 * @param data A pointer to store the bytes that are read.
 * @param nbBytes The number of bytes to read.
 */
void I2C_IntRead(const uint8_t registerAddress, uint8_t* const data, const uint8_t nbBytes)
{

}

/*! @brief Interrupt service routine for the I2C.
 *
 *  Only used for reading data.
 *  At the end of reception, the user callback function will be called.
 *  @note Assumes the I2C module has been initialized.
 */
void __attribute__ ((interrupt)) I2C_ISR(void)
{

}

/*!
 * @}
*/


