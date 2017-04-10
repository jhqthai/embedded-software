/* ###################################################################
**     Filename    : main.c
**     Project     : Lab2
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
** @version 2.0
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

// Defining constants
#define CMD_GET_STARTUP 0x04
#define CMD_SPECIAL_GET_VER 0x09
#define CMD_TOWER_NUMBER 0x0B

/*! Reads the command byte and processes relevant functionality
 *  Also handles ACKing and NAKing
 *
 *  @return bool - TRUE a valid case received & packet ACK
 */
bool Packet_Processor(void)
{
  bool success;
  // Gets the command byte of packet. Sets most significant bit to 0 to get command regardless of ACK enabled/disabled
  switch (Packet_Command & ~PACKET_ACK_MASK)
  {
    case CMD_GET_STARTUP: // Case Special - Get startup values
      if ((Packet_Parameter1 | Packet_Parameter2 | Packet_Parameter3) == 0)
      {
        Packet_Put(CMD_GET_STARTUP, 0x0, 0x0, 0x0);
        Packet_Put(CMD_SPECIAL_GET_VER, 0x76, 0x01, 0x00);
        Packet_Put(CMD_TOWER_NUMBER, 0x01, 0xF1, 0x05);
        success = true;
      }
      else
        success = false;
      break;
    case CMD_SPECIAL_GET_VER: // Case Special - Get version
      if ((Packet_Parameter1 == 0x76) & (Packet_Parameter2 == 0x78) & (Packet_Parameter3 == 0x0D))
      {
        Packet_Put(CMD_SPECIAL_GET_VER, 0x76, 0x01, 0x00);
        success = true;
      }
      else
        success = false;
      break;
    case CMD_TOWER_NUMBER: // Case Tower number (get & set)
      if ((Packet_Parameter1 == 0x01) & (Packet_Parameter2 == 0x00) & (Packet_Parameter3 == 0x00))
      {
        Packet_Put(CMD_TOWER_NUMBER, 0x01, 0xF1, 0x05);
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

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */

  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  // uart init(baud rate, something else);
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  Packet_Init(38400, CPU_BUS_CLK_HZ);

  //init LEDs
  LEDs_Init();

  LEDs_On(LED_BLUE);
  LEDs_Off(LED_BLUE);
  LEDs_Toggle(LED_BLUE);

  //Makes the Packet_Processor function execute "0x04 Special - Get startup values" on startup
  Packet_Command = CMD_GET_STARTUP;
  Packet_Processor();
  for (;;)
  {
    // Check the status of UART hardware
    UART_Poll();
    // If a valid packet is received
    if (Packet_Get())
      Packet_Processor(); // Handle packet according to the command byte
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
