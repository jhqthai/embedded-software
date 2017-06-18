/* ###################################################################
 **     Filename    : main.c
 **     Project     : Lab6
 **     Processor   : MK70FN1M0VMJ12
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 6.0
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */

// CPU module - contains low level hardware initialization routines
#include "Cpu.h"

// Simple OS
#include "OS.h"

// Analog functions
#include "analog.h"

// Related header files
#include "FIFO.h"
#include "UART.h"
#include "packet.h"
#include "LEDs.h"
#include "Flash.h"
#include "FTM.h"
#include "PIT.h"
#include "math.h"

// Global variables
static uint16union_t volatile *NvTowerNb;
static uint16union_t volatile *NvTowerMd;

// Array to store analog window data
//static TAnalogInput AnalogData[ANALOG_WINDOW_SIZE];
TAnalogInput AnalogInput;

// Defining constants
#define CMD_SGET_STARTUP 0x04
#define CMD_SGET_VERSION 0x09
#define CMD_TOWER_NUMBER 0x0B
#define CMD_TOWER_MODE 0x0D
#define CMD_FPROGRAM_BYTE 0x07
#define CMD_FREAD_BYTE 0x08
#define CMD_CHASTIC 0x01

static const uint32_t BAUD_RATE  = 115200;

// define characteristic types
typedef enum
{
  INVERSE,
  VINVERSE,
  EINVERSE
}
Characteristic_t ;
Characteristic_t Chastic;

//// Characteristic constants
//static const double INVERSE_K = 0.14;
//static const double INVERSE_A = 0.02;
//static const double VINVERSE_K = 13.5;
//static const uint8_t VINVERSE_A = 1;
//static const uint8_t EINVERSE_K = 80;
//static const uint8_t EINVERSE_A = 2;

// Initialises PacketProcessor
bool PacketProcessor(void);

// Initilise RMS Filter
static double RMSFilter (int16_t *values, uint8_t n);


// ----------------------------------------
// Thread set up
// ----------------------------------------
// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_ANALOG_CHANNELS 3

// Thread stacks
OS_THREAD_STACK(InitModulesThreadStack, THREAD_STACK_SIZE); /*!< The stack for the LED Init thread. */
static uint32_t AnalogThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
static uint32_t FTMThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); /*!< The stack for the FTM thread. */
static uint32_t PacketThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); /*!< The stack for the packet thread. */
static uint32_t PITThreadStack[THREAD_STACK_SIZE] __attribute__ ((aligned(0x08))); /*!< The stack for the PIT thread. */


// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------
const uint8_t ANALOG_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] = {1, 2, 3};

/*! @brief Data structure used to pass Analog configuration to a user thread
 *
 */
typedef struct AnalogThreadData
{
  OS_ECB* semaphore;
  uint8_t channelNb;
  int16_t values[ANALOG_WINDOW_SIZE];  /*!< An array of sample values to create a "sliding window". */
  double  irms;
} TAnalogThreadData;

/*! @brief Analog thread configuration data
 *
 */
static TAnalogThreadData AnalogThreadData[NB_ANALOG_CHANNELS] =
{
  {
    .semaphore = NULL,
    .channelNb = 0,
    .values[0] = 0,
    .irms = 0.0
  },
  {
    .semaphore = NULL,
    .channelNb = 1,
    .values[0] = 0,
    .irms = 0.0
  },
  {
    .semaphore = NULL,
    .channelNb = 2,
    .values[0] = 0,
    .irms = 0.0
  }
};

// ----------------------------------------
// Initialise FTM channel0
// ----------------------------------------
static TFTMChannel FTMChannel0 =
{
  0,
  CPU_MCGFF_CLK_HZ_CONFIG_0, // Clock source
  TIMER_FUNCTION_OUTPUT_COMPARE, // FTM Mode
  TIMER_OUTPUT_DISCONNECT, // FTM edge/level select
  NULL, // FTM user function
  NULL // FTM user argument
};


/*! @brief Initialises modules.
 *
 */
static void InitModulesThread(void* pData)
{
  // Analog
  (void)Analog_Init(CPU_BUS_CLK_HZ);

  // Generate the global analog semaphores
  for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
    AnalogThreadData[analogNb].semaphore = OS_SemaphoreCreate(0);


  // Initialise Packet
  Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ);

  // Initialise LEDs
  LEDs_Init();

  // Initialise Flash
  Flash_Init();

  // Initialise FTM
  FTM_Init();

  // Initialise PIT
  PIT_Init(CPU_BUS_CLK_HZ);
  PIT0_Set(1.25e6,false); // Sets PIT0 to interrupts every 1.25ms (1/50HZ/16 samples)
  PIT0_Enable(true);     // Starts  PIT0


  // Allocate flash addresses for tower number and mode
  bool allocTowerNB = Flash_AllocateVar((void*)&NvTowerNb, sizeof(*NvTowerNb));
  bool allocTowerMd = Flash_AllocateVar((void*)&NvTowerMd, sizeof(*NvTowerMd));

  if (allocTowerNB && allocTowerMd)
  {
    if (NvTowerNb->l == 0xFFFF) //Checks if tower number has been set
    {
      Flash_Write16((uint16_t *)NvTowerNb, 0x05F1); //If not, sets it
    }
    if (NvTowerMd->l == 0xFFFF) //Checks if tower mode has been set
    {
      Flash_Write16((uint16_t *)NvTowerMd, 0x0001); //If not, sets it
    }
  }

  // Makes the Packet_Processor function execute "0x04 Special - Get startup values" on startup
  Packet_Command = CMD_SGET_STARTUP;
  PacketProcessor();

  // We only do this once - therefore delete this thread
  OS_ThreadDelete(OS_PRIORITY_SELF);
}

/*! @brief Samples a value on an ADC channel and sends it to the corresponding DAC channel.
 *
 */
void AnalogLoopbackThread(void* pData)
{
  // Make the code easier to read by giving a name to the typecast'ed pointer
  #define analogData ((TAnalogThreadData*)pData)
  static uint8_t counter = 0;

  for (;;)
  {
    int16_t analogInputValue;

    (void)OS_SemaphoreWait(analogData->semaphore, 0);
    Analog_Get(analogData->channelNb, &analogInputValue);

    if (counter < ANALOG_WINDOW_SIZE)
    {
      analogData->values[counter] = analogInputValue;
      counter++;
    }
    else
    {
      counter = 0;
      analogData->values[counter] = analogInputValue;
      analogData->irms = RMSFilter(analogData->values, ANALOG_WINDOW_SIZE);
    }

//    //TODO: Maybe should have an output signal thread
//    // Timing output signal
//    if (analogData->irms < 1.03)
//      // 0V output signal on channel1
//      Analog_Put(0, 0);
//    else
//      // 5V output signal on channel1
//      Analog_Put(0, 5);
//
      //TODO: Check how to know if thread is idle or active
//    // Trip output signal
//    if () //thread idle
//      // 0V output signal on channel2
//      Analog_Put(0, 1);
//    else //thread active
//      // 5V output signal on channel2
//      Analog_Put(0, 5);

    // Put analog sample TODO: Display rms calculation
    Analog_Put(analogData->channelNb, analogInputValue);
  }
}

// TODO: Calculation of RMS
static double RMSFilter (int16_t *values, uint8_t n)
{
  int i;
  uint16_t sum = 0;
  for (i =0; i < n; i++)
    sum += values[i] * values[i];
  double vrms = (double)sum/ n;
  double irms = vrms/0.35;
  return (irms);
}

// TODO: How to pass irms into this function?
static void IDMTCalulate (double irms, Characteristic_t chastic)
{
  double t;
//  if (irms >= 1.03)
    switch (chastic)
    {
      case (INVERSE):
        // Calculation
        t = 0.14 / (pow(irms, 0.02) - 1);
        break;

      case (VINVERSE):
        t = 13.5/(irms - 1);
        break;

      case (EINVERSE):
        t = 80/((irms * irms) - 1);
        break;
    }
//    else
//      t = infinity?;
}

/*! @brief PIT interrupt user thread
 *
 *  Confirm interrupt occurred
 *  Signal the analog channels to take a sample
 *  Toggle green LED
 *  @return void
 */
static void PITThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(PITSemaphore, 0);

    // Signal the analog channels to take a sample
    for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
      (void)OS_SemaphoreSignal(AnalogThreadData[analogNb].semaphore);

    // Toggles green LED
    LEDs_Toggle(LED_GREEN);
  }
}

/*! @brief FTM interrupt user thread
 *
 *  Toggle blue LED
 *  @return void
 */
static void FTMThread(void* pData)
{
  for(;;)
  {
    OS_SemaphoreWait(FTMSemaphore, 0);

    // Turn LED off after FTM interrupt
    LEDs_Off(LED_BLUE);
  }
}

/*! @brief Packet interrupt user thread
 *
 *  Process packet
 *  Toggle blue LED
 *  @return void
 */
static void PacketThread(void* pData)
{
  for (;;)
  {
    OS_SemaphoreWait(PacketSemaphore, 0);

    // Wait for packet
    while (!(Packet_Get()));

    LEDs_On(LED_BLUE);
    FTM_StartTimer(&FTMChannel0); // Start timer for output compare
    PacketProcessor(); //Handle packet according to the command byte
  }
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  OS_ERROR error;

  // Initialise low-level clocks etc using Processor Expert code
  PE_low_level_init();

  // Initialize the RTOS
  OS_Init(CPU_CORE_CLK_HZ, true);

  // Create module initialisation thread
  error = OS_ThreadCreate(InitModulesThread,
                          NULL,
                          &InitModulesThreadStack[THREAD_STACK_SIZE - 1],
                          0); // Highest priority

  // Create threads for 3 analog loopback channels
//  for (uint8_t threadNb = 0; threadNb < NB_ANALOG_CHANNELS; threadNb++)
//  {
//    error = OS_ThreadCreate(AnalogLoopbackThread,
//                            &AnalogThreadData[threadNb],
//                            &AnalogThreadStacks[threadNb][THREAD_STACK_SIZE - 1],
//                            ANALOG_THREAD_PRIORITIES[threadNb]);
//  }
  error = OS_ThreadCreate(AnalogLoopbackThread,
                              &AnalogThreadData[0],
                              &AnalogThreadStacks[0][THREAD_STACK_SIZE - 1],
                              ANALOG_THREAD_PRIORITIES[0]);

  // Create thread for packet handler
  OS_ThreadCreate(PacketThread, NULL, &PacketThreadStack[THREAD_STACK_SIZE-1],8);

  // Create thread for FTM
  OS_ThreadCreate(FTMThread, NULL, &FTMThreadStack[THREAD_STACK_SIZE-1],5);

  // Create thread for PIT
  OS_ThreadCreate(PITThread, NULL, &PITThreadStack[THREAD_STACK_SIZE-1],4);

  // Start multithreading - never returns!
  OS_Start();
}

//--------------------------------------------------------------
// Packet Processor
//--------------------------------------------------------------
/*! Reads the command byte and processes relevant functionality
 *  Also handles ACKing and NAKing
 *
 *  @return bool - TRUE a valid case received & packet ACK
 */
bool PacketProcessor(void)
{
  bool success;

  //Gets the command byte of packet. Sets most significant bit to 0 to get command regardless of ACK enabled/disabled
  switch (Packet_Command & ~PACKET_ACK_MASK)
  {
    //Case Special - Get startup values
    case CMD_SGET_STARTUP:
      if ((Packet_Parameter1 | Packet_Parameter2 | Packet_Parameter3) == 0)
      {
        success = Packet_Put(CMD_SGET_STARTUP, 0x0, 0x0, 0x0);
        success &= Packet_Put(CMD_SGET_VERSION, 0x76, 0x05, 0x00);
        success &= Packet_Put(CMD_TOWER_NUMBER, 0x01, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
        success &= Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
      }
      else
        success = false;
      break;

    //Case Special - Get version
    case CMD_SGET_VERSION:
      if ((Packet_Parameter1 == 0x76) & (Packet_Parameter2 == 0x78) & (Packet_Parameter3 == 0x0D))
      {
        success = Packet_Put(CMD_SGET_VERSION, 0x76, 0x03, 0x00);
      }
      else
        success = false;
      break;

    //Case Tower number (get & set)
    case CMD_TOWER_NUMBER:

      //Gets tower number and sends to PC
      if ((Packet_Parameter1 == 0x01) & (Packet_Parameter2 == 0x00) & (Packet_Parameter3 == 0x00))
        success = Packet_Put(CMD_TOWER_NUMBER, 0x01, NvTowerNb->s.Lo, NvTowerNb->s.Hi);

      //Set incoming bytes to flash address of tower number
      else if ((Packet_Parameter1 == 0x02))
      {
        success = Flash_Write16((uint16_t*)NvTowerNb, Packet_Parameter23);
        success &= Packet_Put(CMD_TOWER_NUMBER, 0x01, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
      }
      else
        success = false;
      break;

    //Case Tower Mode (get & set)
    case CMD_TOWER_MODE:

      //Get tower mode
      if ((Packet_Parameter1 == 0x01) & (Packet_Parameter2 == 0x00) & (Packet_Parameter3 == 0x00))
      {
        success = Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
      }

      //Set tower mode
      else if ((Packet_Parameter1 == 0x02))
      {
        success =  Flash_Write16((uint16_t*)NvTowerMd, Packet_Parameter23);
        success &= Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
      }
      else
      success = false;
      break;

    //Case Flash Program Byte
    case CMD_FPROGRAM_BYTE:

      //Check offset validity to write byte
      if ((0x00 <= Packet_Parameter1 < 0x08) & (Packet_Parameter2 == 0x00))
      {
        uint8_t  *cmdByte;
        cmdByte =  (uint8_t *)(FLASH_DATA_START + Packet_Parameter1);
        success = Flash_Write8(cmdByte, Packet_Parameter3);
      }

      //Erase flash for offset of 8
      else if (Packet_Parameter1 == 0x08)
      {
        success = Flash_Erase();
      }
      else
        success = false;
      break;

    // Case Flash Read Byte
    case CMD_FREAD_BYTE:

      // Check offset validity
      if ((0x00 <= Packet_Parameter1 < 0x07) & (Packet_Parameter2 == 0x00) & (Packet_Parameter3 == 0x00))
      {
        uint8_t rByte = _FB(FLASH_DATA_START + Packet_Parameter1); // Read data from appropriate location
        success = Packet_Put(CMD_FREAD_BYTE, Packet_Parameter1, 0x00, rByte);
      }
      else
        success = false;
        break;

    case CMD_CHASTIC:
      // Get characteristic
      if ((Packet_Parameter1 == 0x00) & (Packet_Parameter2 == 0x01))
        // TODO: DO THIS characteristic get
        //success = Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);


      // Set characteristic for IDMT calculation perform
      if ((Packet_Parameter1 == 0x00) & (Packet_Parameter2 == 0x02))
      {
        // Inverse
        if (Packet_Parameter3 == 0x01)
          Chastic = INVERSE;
        // Very Inverse
        else if (Packet_Parameter3 == 0x02)
          Chastic = VINVERSE;
        // Extremely Inverse
        else if (Packet_Parameter3 == 0x03)
          Chastic = EINVERSE;

        // Call function to calculate timing
        IDMTCalulate(Chastic);

        // TODO: Set characteristic into flash
        //success =  Flash_Write16((uint16_t*)NvTowerMd, Packet_Parameter23);

      }


  }
  // Check if packet acknowledgment enabled
  if ((Packet_Command >= PACKET_ACK_MASK))
  {
    if (success)
    {
      // ACK the packet
      Packet_Put(Packet_Command, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
      return true;
    }
    else
    {
      // NAK the packet
      Packet_Put(Packet_Command & ~PACKET_ACK_MASK, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
      return false;
    }
  }
}


/*!
 ** @}
 */
