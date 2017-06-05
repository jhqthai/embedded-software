/*! @file
 *	
 *  @brief Implementation of  packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *	
 *  @author John Thai & Jason Gavriel
 *  @date 2017-04-05
 */
/*!
 * @addtogroup Packet_module Packet module documentation
 * @{
 */
/* MODULE Packet */

#include "UART.h"
#include "packet.h"



TPacket Packet;
const uint8_t PACKET_ACK_MASK = 0x80;
static uint8_t PacketIndex = 0;

/*! @brief Initializes functions of the included modules where necessary.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz.
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
	// PacketSemaphore = OS_SemaphoreCreate(0);
	PacketPutSemaphore = OS_SemaphoreCreate(1);
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Attempts to read bytes from UART and build a valid packet (with checksum).
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void)
{
  //Temp variable to store UART input
  uint8_t uartStore;
  //If UART receives a character, store in uartStore
  while (UART_InChar (&uartStore))
  {
  	//Places uartStore in the variable that corresponds to PacketIndex - also increments PacketIndex
    switch (PacketIndex++)
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

        //Compares incoming byte with expected checksum
        if((Packet_Command ^ Packet_Parameter1 ^ Packet_Parameter2 ^ Packet_Parameter3) == Packet_Checksum)
        {
        	//Valid packet, reset PacketIndex and return true
          PacketIndex = 0;
          return true;
        }
        else
        {
        //Invalid packet, discard the 1st byte, and shift everything down 1 byte
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
  return false;
}



//Given command and three parameter bytes, gets checksum and attempts to send a packet
bool Packet_Put (const uint8_t command, const uint8_t parameter1, const uint8_t parameter2, const uint8_t parameter3)
{
	// Obtain exclusive access to packet put
	OS_SemaphoreWait(PacketPutSemaphore, 0);

  //Calculates checksum from command and parameters and sends all to FIFO
	bool success = (UART_OutChar(command)
	    && UART_OutChar(parameter1)
	    && UART_OutChar(parameter2)
	    && UART_OutChar(parameter3)
	    && UART_OutChar(command ^ parameter1 ^ parameter2 ^ parameter3));

	// Relinquish exclusive access to packet put
	OS_SemaphoreSignal(PacketPutSemaphore);

	return success;
}

/*!
** @}
*/
