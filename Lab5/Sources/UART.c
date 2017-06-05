/*! @file
 *
 *  @brief I/O implementation for UART communications on the TWR-K70F120M.
 *
 *  This contains implementation of functions for operating the UART (serial port).
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-04-05
 */
/*!
 * @addtogroup UART_module UART module documentation
 * @{
 */
/* MODULE UART */


#include "FIFO.h"
#include "UART.h"
#include "MK70F12.h"
#include "CPU.h"
#include "OS.h"
#include "Packet.h"


static TFIFO RxFIFO;
static TFIFO TxFIFO;


bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
	// Initialise semaphore to 0 for multiple events occurrence
  TxSemaphore = OS_SemaphoreCreate(0);
  RxSemaphore = OS_SemaphoreCreate(0);

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


  //Enable UART2 C2 RIE
  UART2_C2 |= UART_C2_RIE_MASK;
  //Enable UART2 C2 TIE
  UART2_C2 |= UART_C2_TIE_MASK;


  //Enable UART2 C2 Receiver
  UART2_C2 |= UART_C2_RE_MASK;
  //Enable UART 2 C2 Transmitter
  UART2_C2 |= UART_C2_TE_MASK;

  // Initialize NVIC
  // see p. 91 of K70P256M150SF3RM.pdf
  // Vector 0x41=65, IRQ=49
  // NVIC non-IPR=1 IPR=12
  // Clear any pending interrupts on LPTMR
  NVICICPR1 = (1 << 17); //IRQ mod 32 (p.92 K70)

  // Enable interrupts from LPTMR module
  NVICISER1 = (1 << 17);

  FIFO_Init(&RxFIFO);
  FIFO_Init(&TxFIFO);
  
  return true;
  
}


bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr); // Store incoming byte in variable
}


bool UART_OutChar(const uint8_t data)
{
  return (FIFO_Put(&TxFIFO, data));
}


void RxUARTThread(void* arg)
{
	for(;;)
	{
		// Wait to receive
		OS_SemaphoreWait(RxSemaphore, 0);

		// Write to UART2 data register
		FIFO_Put(&RxFIFO, UART2_D);

		// Signal packet semaphore indicate packet_init ready
		OS_SemaphoreSignal(PacketSemaphore);

		// Enable receiver interrupt mask
		UART2_C2 |= UART_C2_RIE_MASK;
	}
}

void TxUARTThread(void* arg)
{
	for(;;)
	{
		// Wait for transmit signal
		OS_SemaphoreWait(TxSemaphore, 0);

		// Get data from UART2 data register to transmit
		FIFO_Get(&TxFIFO, &UART2_D);

		// Enable transmit interrupt mask
		UART2_C2 |= UART_C2_TIE_MASK;
	}
}



void __attribute__ ((interrupt)) UART_ISR (void)
{
	OS_ISREnter();

	// Clear RDRF flag by reading the status register
	if (UART2_S1 & UART_S1_RDRF_MASK)
	{
		// Signal RxUARTThread ready
		OS_SemaphoreSignal(RxSemaphore);

		// Disable receiver interrupt mask before writing in RxUARTThread
		UART2_C2 &= ~UART_C2_RIE_MASK;
	}
	// Clear TDRE flag by reading the status register
	if (UART2_S1 & UART_S1_TDRE_MASK)
	{
		// Signal TxUARTThread ready
		OS_SemaphoreSignal(TxSemaphore);

		// Disable receiver mask before writing in TxUARTThread
		UART2_C2 &= ~UART_C2_TIE_MASK;
	}
	OS_ISRExit();
}

/*!
** @}
*/


