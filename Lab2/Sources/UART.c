/*! @file
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-xx-xx
 */

#include "FIFO.h"
#include "UART.h"
#include "MK70F12.h"
#include "CPU.h"
#include "stdbool.h"

static TFIFO RxFIFO;
static TFIFO TxFIFO;

/*! @brief Sets up the UART interface before first use.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the UART was successfully initialized.
 */


bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  //Enable UART2 Module
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;
  //Enable PORT E pin routing
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;
  
  //Sets MUX bits in porte16 to refer to alt 3
  PORTE_PCR16 |= PORT_PCR_MUX(3);
  //Sets MUX bits in porte17 to refer to alt 3
  PORTE_PCR17 |= PORT_PCR_MUX(3);

  //DISABLE UART2 C2 Receiver
  UART2_C2 &= ~UART_C2_RE_MASK;
  //DISABLE UART 2 C2 Transmitter
  UART2_C2 &= ~UART_C2_TE_MASK;

  //UART baud rate = UART module clock / (16(SBR[12:0] + BRFD))
  uint16_t sbr = moduleClk/(16*baudRate);
  //Top 5 bits for UART2_BDH
  uint8_t sbrBDH = sbr >> 8;
  //Keep the top 3 bits of bdh and set the last 5 to the top 5 bits of SBR
  UART2_BDH |= sbrBDH & 0x1F;

  //Bottom 8 bits for UART2_BDL
  uint8_t sbrBDL = (uint8_t) sbr;
  //Bottom 8 bits directly into UART2_BDL
  UART2_BDL = sbrBDL;

  //Baud frequency with 5 further bits of accuracy (1/32ths)
  uint8_t brfa = (moduleClk*2/baudRate)%32;
  //Set the bottom 5 bits of c4 to that of baud rate fine adjust (brfa)
  UART2_C4 |=  UART_C4_BRFA(brfa);

  //Enable UART2 C2 Receiver
  UART2_C2 |= UART_C2_RE_MASK;
  //Enable UART 2 C2 Transmitter
  UART2_C2 |= UART_C2_TE_MASK;

  FIFO_Init(&RxFIFO);
  FIFO_Init(&TxFIFO);

  return true;
  
}
/*! @brief Get a character from the receive FIFO if it is not empty.
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return bool - TRUE if the receive FIFO returned a character.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);
}

/*! @brief Put a byte in the transmit FIFO if it is not full.
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return bool - TRUE if the data was placed in the transmit FIFO.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_OutChar(const uint8_t data)
{
  return FIFO_Put(&TxFIFO, data);
}

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.
 */
void UART_Poll(void)
{
  //Checks UART2_S1 "TDRE" bit -> 1 indicates we should read TxFIFO and write to UART_D
  if (UART2_S1 & UART_S1_TDRE_MASK)
    FIFO_Get(&TxFIFO, &UART2_D);

  //Checks UART2_S1 "RDRF" bit -> 1 indicates we should read UART_D and write to RxFIFO
  if (UART2_S1 & UART_S1_RDRF_MASK)
    FIFO_Put(&RxFIFO, UART2_D);
}


