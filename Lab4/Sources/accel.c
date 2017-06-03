/*! @file
 *
 *  @brief Implementation of HAL for the accelerometer.
 *
 *  This contains the functions definition for interfacing to the MMA8451Q accelerometer.
 *
 *  @author John Thai, Jason Gavriel
 *  @date 2017-05-08
 */

/*!
 *  @addtogroup ACCEL_module ACCEL module documentation
 *  @{
*/
/* MODULE ACCEL */

// Accelerometer functions
#include "accel.h"

// Inter-Integrated Circuit
#include "I2C.h"

// Median filter
#include "median.h"

// K70 module registers
#include "MK70F12.h"

// CPU and PE_types are needed for critical section variables and the defintion of NULL pointer
#include "CPU.h"
#include "PE_types.h"

#include "i2c.h"

// Accelerometer registers
#define ADDRESS_OUT_X_MSB 0x01
#define ADDRESS_INT_SOURCE 0x0C

static union
{
  uint8_t byte;			/*!< The INT_SOURCE bits accessed as a byte. */
  struct
  {
    uint8_t SRC_DRDY   : 1;	/*!< Data ready interrupt status. */
    uint8_t               : 1;
    uint8_t SRC_FF_MT  : 1;	/*!< Freefall/motion interrupt status. */
    uint8_t SRC_PULSE  : 1;	/*!< Pulse detection interrupt status. */
    uint8_t SRC_LNDPRT : 1;	/*!< Orientation interrupt status. */
    uint8_t SRC_TRANS  : 1;	/*!< Transient interrupt status. */
    uint8_t SRC_FIFO   : 1;	/*!< FIFO interrupt status. */
    uint8_t SRC_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt status. */
  } bits;			/*!< The INT_SOURCE bits accessed individually. */
} INT_SOURCE_Union;

#define INT_SOURCE     		INT_SOURCE_Union.byte
#define INT_SOURCE_SRC_DRDY	INT_SOURCE_Union.bits.SRC_DRDY
#define INT_SOURCE_SRC_FF_MT	CTRL_REG4_Union.bits.SRC_FF_MT
#define INT_SOURCE_SRC_PULSE	CTRL_REG4_Union.bits.SRC_PULSE
#define INT_SOURCE_SRC_LNDPRT	CTRL_REG4_Union.bits.SRC_LNDPRT
#define INT_SOURCE_SRC_TRANS	CTRL_REG4_Union.bits.SRC_TRANS
#define INT_SOURCE_SRC_FIFO	CTRL_REG4_Union.bits.SRC_FIFO
#define INT_SOURCE_SRC_ASLP	CTRL_REG4_Union.bits.SRC_ASLP

#define ADDRESS_CTRL_REG1 0x2A

typedef enum
{
  DATE_RATE_800_HZ,
  DATE_RATE_400_HZ,
  DATE_RATE_200_HZ,
  DATE_RATE_100_HZ,
  DATE_RATE_50_HZ,
  DATE_RATE_12_5_HZ,
  DATE_RATE_6_25_HZ,
  DATE_RATE_1_56_HZ
} TOutputDataRate;

typedef enum
{
  SLEEP_MODE_RATE_50_HZ,
  SLEEP_MODE_RATE_12_5_HZ,
  SLEEP_MODE_RATE_6_25_HZ,
  SLEEP_MODE_RATE_1_56_HZ
} TSLEEPModeRate;

static union
{
  uint8_t byte;			/*!< The CTRL_REG1 bits accessed as a byte. */
  struct
  {
    uint8_t ACTIVE    : 1;	/*!< Mode selection. */
    uint8_t F_READ    : 1;	/*!< Fast read mode. */
    uint8_t LNOISE    : 1;	/*!< Reduced noise mode. */
    uint8_t DR        : 3;	/*!< Data rate selection. */
    uint8_t ASLP_RATE : 2;	/*!< Auto-WAKE sample frequency. */
  } bits;			/*!< The CTRL_REG1 bits accessed individually. */
} CTRL_REG1_Union;

#define CTRL_REG1     		    CTRL_REG1_Union.byte
#define CTRL_REG1_ACTIVE	    CTRL_REG1_Union.bits.ACTIVE
#define CTRL_REG1_F_READ  	  CTRL_REG1_Union.bits.F_READ
#define CTRL_REG1_LNOISE  	  CTRL_REG1_Union.bits.LNOISE
#define CTRL_REG1_DR	    	  CTRL_REG1_Union.bits.DR
#define CTRL_REG1_ASLP_RATE	  CTRL_REG1_Union.bits.ASLP_RATE



#define ADDRESS_CTRL_REG2 0x2B

#define ADDRESS_CTL_REG2_RST_MASK 0x40
#define ADDRESS_CTRL_REG3 0x2C

static union
{
  uint8_t byte;			/*!< The CTRL_REG3 bits accessed as a byte. */
  struct
  {
    uint8_t PP_OD       : 1;	/*!< Push-pull/open drain selection. */
    uint8_t IPOL        : 1;	/*!< Interrupt polarity. */
    uint8_t WAKE_FF_MT  : 1;	/*!< Freefall/motion function in SLEEP mode. */
    uint8_t WAKE_PULSE  : 1;	/*!< Pulse function in SLEEP mode. */
    uint8_t WAKE_LNDPRT : 1;	/*!< Orientation function in SLEEP mode. */
    uint8_t WAKE_TRANS  : 1;	/*!< Transient function in SLEEP mode. */
    uint8_t FIFO_GATE   : 1;	/*!< FIFO gate bypass. */
  } bits;			/*!< The CTRL_REG3 bits accessed individually. */
} CTRL_REG3_Union;

#define CTRL_REG3     		    CTRL_REG3_Union.byte
#define CTRL_REG3_PP_OD		    CTRL_REG3_Union.bits.PP_OD
#define CTRL_REG3_IPOL		    CTRL_REG3_Union.bits.IPOL
#define CTRL_REG3_WAKE_FF_MT	CTRL_REG3_Union.bits.WAKE_FF_MT
#define CTRL_REG3_WAKE_PULSE	CTRL_REG3_Union.bits.WAKE_PULSE
#define CTRL_REG3_WAKE_LNDPRT	CTRL_REG3_Union.bits.WAKE_LNDPRT
#define CTRL_REG3_WAKE_TRANS	CTRL_REG3_Union.bits.WAKE_TRANS
#define CTRL_REG3_FIFO_GATE	  CTRL_REG3_Union.bits.FIFO_GATE

#define ADDRESS_CTRL_REG4 0x2D

static union
{
  uint8_t byte;			/*!< The CTRL_REG4 bits accessed as a byte. */
  struct
  {
    uint8_t INT_EN_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t               : 1;
    uint8_t INT_EN_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_EN_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_EN_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_EN_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_EN_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_EN_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG4 bits accessed individually. */
} CTRL_REG4_Union;

#define CTRL_REG4            		CTRL_REG4_Union.byte
#define CTRL_REG4_INT_EN_DRDY	  CTRL_REG4_Union.bits.INT_EN_DRDY
#define CTRL_REG4_INT_EN_FF_MT	CTRL_REG4_Union.bits.INT_EN_FF_MT
#define CTRL_REG4_INT_EN_PULSE	CTRL_REG4_Union.bits.INT_EN_PULSE
#define CTRL_REG4_INT_EN_LNDPRT	CTRL_REG4_Union.bits.INT_EN_LNDPRT
#define CTRL_REG4_INT_EN_TRANS	CTRL_REG4_Union.bits.INT_EN_TRANS
#define CTRL_REG4_INT_EN_FIFO	  CTRL_REG4_Union.bits.INT_EN_FIFO
#define CTRL_REG4_INT_EN_ASLP	  CTRL_REG4_Union.bits.INT_EN_ASLP

#define ADDRESS_CTRL_REG5 0x2E

static union
{
  uint8_t byte;			/*!< The CTRL_REG5 bits accessed as a byte. */
  struct
  {
    uint8_t INT_CFG_DRDY   : 1;	/*!< Data ready interrupt enable. */
    uint8_t                : 1;
    uint8_t INT_CFG_FF_MT  : 1;	/*!< Freefall/motion interrupt enable. */
    uint8_t INT_CFG_PULSE  : 1;	/*!< Pulse detection interrupt enable. */
    uint8_t INT_CFG_LNDPRT : 1;	/*!< Orientation interrupt enable. */
    uint8_t INT_CFG_TRANS  : 1;	/*!< Transient interrupt enable. */
    uint8_t INT_CFG_FIFO   : 1;	/*!< FIFO interrupt enable. */
    uint8_t INT_CFG_ASLP   : 1;	/*!< Auto-SLEEP/WAKE interrupt enable. */
  } bits;			/*!< The CTRL_REG5 bits accessed individually. */
} CTRL_REG5_Union;

#define CTRL_REG5     		      	CTRL_REG5_Union.byte
#define CTRL_REG5_INT_CFG_DRDY		CTRL_REG5_Union.bits.INT_CFG_DRDY
#define CTRL_REG5_INT_CFG_FF_MT		CTRL_REG5_Union.bits.INT_CFG_FF_MT
#define CTRL_REG5_INT_CFG_PULSE		CTRL_REG5_Union.bits.INT_CFG_PULSE
#define CTRL_REG5_INT_CFG_LNDPRT	CTRL_REG5_Union.bits.INT_CFG_LNDPRT
#define CTRL_REG5_INT_CFG_TRANS		CTRL_REG5_Union.bits.INT_CFG_TRANS
#define CTRL_REG5_INT_CFG_FIFO		CTRL_REG5_Union.bits.INT_CFG_FIFO
#define CTRL_REG5_INT_CFG_ASLP		CTRL_REG5_Union.bits.INT_CFG_ASLP

// Global variables
static TAccelMode ProtocolMode;

//static uint64_t AccModuleClk;
static void (*ADRCallback)(void*);	/*!< The user's data ready callback function. */
static void *ADRArgs;		/*!< The user's data ready callback function arguments. */
static void (*ARCCallback)(void*);	/*!< The user's read complete callback function. */
static void *ARCArgs;		/*!< The user's read complete callback function arguments. */

// Address of accelerometer with SA0 high
const static uint8_t MMA_ADDR_SA0H =  0x1D;
const static uint8_t WHO_AM_I_REG_ADDR = 0x0D;



bool Accel_Init(const TAccelSetup* const accelSetup)
{
	// Set receive variable to private global variable
  //AccModuleClk = accelSetup->moduleClk;
  ADRCallback =  accelSetup->dataReadyCallbackFunction;
  ADRArgs =  accelSetup->dataReadyCallbackArguments;
  ARCCallback = accelSetup->readCompleteCallbackFunction;
  ARCArgs = accelSetup->readCompleteCallbackArguments;

	// Pass on the slave address, module clock and relevant callbacks
	TI2CModule I2CModule = {
		MMA_ADDR_SA0H,
		100000,
		ARCCallback,
		ARCArgs
	};

	// Initialise I2C
	I2C_Init(&I2CModule, accelSetup->moduleClk);

	// Enable PORTB module clock gate
	SIM_SCGC5 |= SIM_SCGC5_PORTB_MASK;

	// PORTB PCR4 configuration for GPIO, interrupt status flag, interrupt on falling edge
	PORTB_PCR4 |= PORT_PCR_MUX(1) | PORT_PCR_ISF_MASK | PORT_PCR_IRQC(0x0A);

	// Initialise PORTB NVIC; Vector = 104, IRQ = 88, Non-IPR = 2
	// Clear pending interrupts
	NVICICPR2 = (1 << 24);
	// Set enable interrupts
	NVICISER2 = (1 << 24);

	// Control register 1 - Set relevant masks
	// Disable active mode before setting values
	// Keeps normal mode for low noise
	// Init with a rate of 1.56 Hz
	CTRL_REG1_ACTIVE = 0;
	CTRL_REG1_DR = DATE_RATE_1_56_HZ;
	CTRL_REG1_F_READ = 1;
	CTRL_REG1_LNOISE = 0;
	CTRL_REG1_ASLP_RATE = SLEEP_MODE_RATE_1_56_HZ;
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);

	// Control register 2 - Reset accelerator before use
	I2C_Write(ADDRESS_CTRL_REG2, ADDRESS_CTL_REG2_RST_MASK);

	// Control register 3 - Set relevant masks
	// Set push-pull mode
	CTRL_REG3_PP_OD = 0;
	I2C_Write(ADDRESS_CTRL_REG3, CTRL_REG3);

	// Control register 4 - Set relevant masks
	// Disable data ready interrupts
	CTRL_REG4_INT_EN_DRDY = 0;
	I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);

	// Control register 5
	// Set data ready interrupt to use INT1 pin	(set everything enabled)
	I2C_Write(ADDRESS_CTRL_REG5, CTRL_REG5);

	// Control register 1
	// Activate active mode after setting values
		CTRL_REG1_ACTIVE = 1;
		I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
}


void Accel_ReadXYZ(uint8_t data[3])
{
	// Read XYZ 8 most significant bits (MMA.pdf p.13)
	// Address auto-increment afters each read
	// Skips LSB values due to F_READ being set
	if (ProtocolMode == ACCEL_INT)
		I2C_IntRead(ADDRESS_OUT_X_MSB, data, 3);
	else if (ProtocolMode == ACCEL_POLL)
		I2C_PollRead(ADDRESS_OUT_X_MSB, data, 3);

}


void Accel_SetMode(const TAccelMode mode)
{
	ProtocolMode = mode; // NEW

	// Disable active mode before setting values
	CTRL_REG1_ACTIVE = 0;
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);

	// Interrupt mode = Accelerometer interrupts when data is ready
	if (ProtocolMode == ACCEL_INT)
	{
		// Enable data ready interrupt
		CTRL_REG4_INT_EN_DRDY = 1;
		I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);
		//I2C0_C1 &= ~I2C_C1_IICIE_MASK; // Enable interrupt service
	}

	// Poll mode = I2C Master initiates read of data from accelerometer slave
	else
	{
		// Disable data ready interrupt
		CTRL_REG4_INT_EN_DRDY = 0;
		I2C_Write(ADDRESS_CTRL_REG4, CTRL_REG4);
		//I2C0_C1 |= I2C_C1_IICIE_MASK; // Disable interrupt service
	}
	// Activate active mode after setting values
	CTRL_REG1_ACTIVE = 1;
	I2C_Write(ADDRESS_CTRL_REG1, CTRL_REG1);
}


void __attribute__ ((interrupt)) AccelDataReady_ISR(void)
{
	// Check if interrupt is pending
  if (PORTB_PCR7 & PORT_PCR_ISF_MASK)
  {
  	// Clear interrupt flag
    PORTB_PCR7 |= PORT_PCR_ISF_MASK;
    // Invoke ADRCallback
    ADRCallback(ADRArgs);
  }
}


/*!
 * @}
*/
