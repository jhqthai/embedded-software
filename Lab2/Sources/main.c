/* ###################################################################
**     Filename    : main.c
**     Project     : Lab1
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
** @version 1.0
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


// Header files
// CPU mpdule - contains low level hardware initialization routines
#include "Cpu.h"
#include "FIFO.h"
#include "UART.h"
#include "Packet.h"

// Global variables

/*! Reads the command byte and processes relevant functionality
 *  Also handles ACKing and NAKing
 *
 *  @return bool - TRUE a valid case received & packet ACK
 */
bool Packet_Processor(void)
{
  bool success;
  // Gets the command byte of packet. Sets most significant bit to 0 to get command regardless of ACK enabled/disabled
  uint8_t commandByte = Packet_Command & ~PACKET_ACK_MASK;
  switch(commandByte)
  {
    case 0x04: // Case Special - Get startup values
	  Packet_Put(0x04, 0x0, 0x0, 0x0);
	  Packet_Put(0x09, 0x76, 0x01, 0x00);
	  Packet_Put(0x0B, 0x01, 0xF1, 0x05);
	  success = true;
	  break;
    case 0x09: // Case Special - Get version
	  Packet_Put(0x09, 0x76, 0x01, 0x00);
	  success = true;
	  break;
    case 0x0B: // Case Tower number (get & set)
	  Packet_Put(0x0B, 0x01, 0xF1, 0x05);
	  success = true;
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
	  Packet_Put(commandByte, Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
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
  //Makes the packer processor execute "0x04 Special - Get startup values" on startup
  Packet_Command = 0x04;
  Packet_Processor();
  for (;;)
  {
    UART_Poll();
    //if a valid packet is received
    if (Packet_Get())
    {
      Packet_Processor();
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
