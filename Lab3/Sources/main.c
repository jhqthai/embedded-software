/* ###################################################################
**     Filename    : main.c
**     Project     : Lab3
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
** @version 3.0
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
#include "Events.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"
#include "FIFO.h"
#include "UART.h"
#include "packet.h"
#include "LEDs.h"
#include "Flash.h"
#include "PIT.h"
#include "RTC.h"
#include "FTM.h"

// Defining constants
#define CMD_SGET_STARTUP 0x04
#define CMD_SGET_VERSION 0x09
#define CMD_TOWER_NUMBER 0x0B
#define CMD_TOWER_MODE 0x0D
#define CMD_FPROGRAM_BYTE 0x07
#define CMD_FREAD_BYTE 0x08
#define CMD_SET_TIME 0x0C

static const uint32_t BAUD_RATE  = 115200;

// Global variables
static volatile uint8_t *Write8;
static uint16union_t volatile *NvTowerNb;
static uint16union_t volatile *NvTowerMd;

// Declare function to be use in struct
static void FTMCallback(void* arg);

// Initialise FTM channel0
static TFTMChannel FTMChannel0 =
{
	0,
	CPU_MCGFF_CLK_HZ_CONFIG_0, // Clock source
	TIMER_FUNCTION_OUTPUT_COMPARE, // FTM Mode
	TIMER_OUTPUT_DISCONNECT, // FTM edge/level select
	&FTMCallback, // FTM user function
	NULL // FTM user argument
};


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
        Packet_Put(CMD_SGET_STARTUP, 0x0, 0x0, 0x0);
        Packet_Put(CMD_SGET_VERSION, 0x76, 0x03, 0x00);
        Packet_Put(CMD_TOWER_NUMBER, 0x01, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
        Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
        success = true;
      }
      else
        success = false;
      break;

    //Case Special - Get version
    case CMD_SGET_VERSION:
      if ((Packet_Parameter1 == 0x76) & (Packet_Parameter2 == 0x78) & (Packet_Parameter3 == 0x0D))
      {
        Packet_Put(CMD_SGET_VERSION, 0x76, 0x03, 0x00);
        success = true;
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
				Packet_Put(CMD_TOWER_NUMBER, 0x01, NvTowerNb->s.Lo, NvTowerNb->s.Hi);
      }
			else
        success = false;
      break;

    //Case Tower Mode (get & set)
    case CMD_TOWER_MODE:

    	//Get tower mode
      if ((Packet_Parameter1 == 0x01) & (Packet_Parameter2 == 0x00) & (Packet_Parameter3 == 0x00))
      {
      	Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi); //
      	success = true;
      }

      //Set tower mode
      else if ((Packet_Parameter1 == 0x02))
      {
      	success =  Flash_Write16((uint16_t*)NvTowerMd, Packet_Parameter23);
				Packet_Put(CMD_TOWER_MODE, 0x01, NvTowerMd->s.Lo, NvTowerMd->s.Hi);
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
	
		// Case Set Time
    case CMD_SET_TIME:
    	// Check to see if valid time is received
    	if ((Packet_Parameter1<24) && (Packet_Parameter2<60) && (Packet_Parameter3<60))
    	{
				// Pass parameters to RTC_Set function to set time
				RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
				success = true;
    	}
    	else
    		success = false;
    	break;

    default:
      success = false;
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

/*! @brief PIT interrupt user callback function
 *  
 *	Confirm interrupt occurred
 *	Toggle green LED
 *  @return void
 */
void PITCallback(void* arg)
{
	// Toggles green LED 
	LEDs_Toggle(LED_GREEN);
}

/*! @brief RTC interrupt user callback function
 *
 *  Send current time to PC
 *	Toggle yellow LED
 *  @return void
 */
void RTCCallback(void* arg)
{
	// Variables to store current time
	uint8_t hours, minutes, seconds;
	
	// Gets current time
	RTC_Get(&hours, &minutes, &seconds);
  
	// Sets updated time
	Packet_Put(CMD_SET_TIME, hours, minutes, seconds);
	
	// Toggles yellow LED
	LEDs_Toggle(LED_YELLOW);							
}

/*! @brief FTM interrupt user callback function
 *
 *	Toggle blue LED
 *  @return void
 */
void FTMCallback(void* arg)
{
	// Turn LED off after FTM interrupt
	LEDs_Off(LED_BLUE);
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */

  // Disable global interrupt
  __DI();

  // Initialise Packet
  bool packInit = Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ);

  // Initialise LEDs
  bool LEDsInit = LEDs_Init();

  // Initialise Flash
  bool flashInit = Flash_Init();

  // Turn LED on if tower initialise successful
  if (packInit && LEDsInit && flashInit)
    LEDs_On(LED_ORANGE);

  // Initialise PIT
  PIT_Init(CPU_BUS_CLK_HZ, &PITCallback, NULL);
  PIT_Set(500e6,false);	// Sets PIT0 to interrupts every half second
  PIT_Enable(true);			// Starts  PIT0

  // Initialise RTC
  RTC_Init(&RTCCallback, NULL);

  // Initialise FTM
  FTM_Init();

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

  // Enable global interrupt
  __EI();
  for (;;)
  {
  	// If a valid packet is received
    if (Packet_Get())
    {
    	LEDs_On(LED_BLUE);
    	FTM_StartTimer(&FTMChannel0); // Start timer for output compare
      PacketProcessor(); //Handle packet according to the command byte
    }
  }
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.5 [05.21]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
