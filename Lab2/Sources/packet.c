/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

// new types
#include "types.h"
#include "stdbool.h"
#include "UART.h"

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */

uint8_t Packet_Command,	/*!< The packet's command */
  Packet_Parameter1, 	/*!< The packet's 1st parameter */
  Packet_Parameter2, 	/*!< The packet's 2nd parameter */
  Packet_Parameter3,	/*!< The packet's 3rd parameter */
  Packet_Checksum;	/*!< The packet's checksum */

uint8_t PacketIndex = 0;

const uint8_t PACKET_ACK_MASK = 0x80;

bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk){
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void){
  //temp variable to store uart input
  uint8_t uartStore;
  for(;;)
  {
    //if uart recieves a character, store in uartStore
    if (UART_InChar(&uartStore))
	{
    //Places uartstore in the variable that corresponds to PacketIndex - also increments PacketIndex
      switch(PacketIndex++)
	  {
		case 0:
		  Packet_Command = uartStore;
		  break;
		case 1:
		  Packet_Parameter1 = uartStore;
		  break;
		case 2:
		  Packet_Parameter2 = uartStore;
		  break;
		case 3:
		  Packet_Parameter3 = uartStore;
		  break;
		case 4:
		  Packet_Checksum = uartStore;
		  //compares incoming byte with expected checksum
		  if((Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3) == Packet_Checksum)
		  {
		  //valid packet, reset PacketIndex and return true
		    return !(PacketIndex = 0);
		  }
		  else
		  {
		    //invalid packet, discard the 1st byte, and shift everything down 1 byte
		    Packet_Command = Packet_Parameter1;
		    Packet_Parameter1 = Packet_Parameter2;
		    Packet_Parameter2 = Packet_Parameter3;
		    Packet_Parameter3 = Packet_Checksum;
		    PacketIndex = 4;
		    return false;
		  }
		  break;
	  }
    }
    else
	{
	  return false;
    }
  }
}



/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
  {
  //calculates checksum from command and parameters and sends all to FIFO
  return (UART_OutChar(command) 
    & UART_OutChar(parameter1)
    & UART_OutChar(parameter2) 
    & UART_OutChar(parameter3) 
    & UART_OutChar(command ^ parameter1 ^ parameter2 ^ parameter3));
  }
}