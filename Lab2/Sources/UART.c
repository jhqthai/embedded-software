/*! @file
 *
 *  @brief I/O routines for UART communications on the TWR-K70F120M.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-xx-xx
 */

#include "FIFO.h"
#include "UART.h"
#include "MK70F12.h"
#include "CPU.h"


static TFIFO RxFIFO;
static TFIFO TxFIFO;

//Initialises the registers needed to enable UART, pin and peripheral function and to setup correct baud rate
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
  UART2_BDH |= UART_BDH_SBR(sbrBDH);

  //Bottom 8 bits for UART2_BDL
  uint8_t sbrBDL = (uint8_t) sbr;
  //Bottom 8 bits directly into UART2_BDL
  UART2_BDL = UART_BDL_SBR(sbrBDL);

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


//Used by tower to receive a character from RxFIFO
bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);
}

//Used by tower to transmit a character to TxFIFO
bool UART_OutChar(const uint8_t data)
{
  return FIFO_Put(&TxFIFO, data);
}

//Checks to see if there is a char to read from or write to the relevant FIFO
void UART_Poll(void)
{
  //Checks UART2_S1 "TDRE" bit -> 1 indicates we should read TxFIFO and write to UART_D
  if (UART2_S1 & UART_S1_TDRE_MASK)
    FIFO_Get(&TxFIFO, &UART2_D);

  //Checks UART2_S1 "RDRF" bit -> 1 indicates we should read UART_D and write to RxFIFO
  if (UART2_S1 & UART_S1_RDRF_MASK)
    FIFO_Put(&RxFIFO, UART2_D);
}


