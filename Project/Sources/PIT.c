/*

 * PIT.c
 *
 *  @brief Implementation of routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This contains the function definition for operating the periodic interrupt timer (PIT).
 *
 *  @author John Thai & Jason Garviel
 *  @date 2017-04-27
 */
/*!
 * @addtogroup PIT_module PIT module documentation
 * @{
 */
/* MODULE PIT */

#include "PIT.h"
#include "MK70F12.h"

static uint32_t PITModuleClk; // Module clock rate in HZ

OS_ECB *PITSemaphore;   /*!< Binary semaphore for PIT thread */



bool PIT_Init(const uint32_t moduleClk)
{
  // Create semaphore
  PITSemaphore = OS_SemaphoreCreate(1);

	// Set receive variable to private global variable
	PITModuleClk = moduleClk;

	// Enable PIT SCGC
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
	
	// Enable PIT timer clock by clearing bit 0
	PIT_MCR &= ~PIT_MCR_MDIS_MASK;
	
	// Enable freeze when debug by setting bit 1
	PIT_MCR |= PIT_MCR_FRZ_MASK;
	
	// Enable Timer0
	PIT0_Enable(true);

	// Enable Timer0 interrupt
	PIT_TCTRL0 = PIT_TCTRL_TIE_MASK;
	
  // Initialize NVIC
  // Vector 0x41=84, IRQ=68
  // NVIC non-IPR=2, IPR=17
	//IRQ mod 32 (p.92 K70)
  // Clear any pending interrupts on PIT
  NVICICPR2 = (1 << 4);

  // Enable interrupts from PIT module
  NVICISER2 = (1 << 4);

	return true;
}


void PIT0_Set(const uint32_t period, const bool restart)
{
	// Clock period (ns) = 1/module clock rate (MHZ)
	uint32_t periodClk = 1e9/PITModuleClk;

	// Clock cycles = interrupt period/clock period
	// LDVAL trigger = clock cycles - 1 (p.1339 K70)
	uint32_t triggerLDVAL = period/periodClk -1;

	// If restart true, disable pit and set up timer as per spec
	if (restart)
	{
		PIT0_Enable(false);
		PIT_LDVAL0 = PIT_LDVAL_TSV(triggerLDVAL); // Set up timer for triggerLDVAL clock cycles
		PIT0_Enable(true);
	}
	else
		PIT_LDVAL0 = PIT_LDVAL_TSV(triggerLDVAL);
}


void PIT0_Enable(const bool enable)
{
	if (enable)
		PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK; // Start timer0
	else
		PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK; // Disable timer0
}


void __attribute__ ((interrupt)) PIT0_ISR(void)
{
	OS_ISREnter();

	// Clear flag by set bit to 1
	PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

	// Signal PIT semaphore
	OS_SemaphoreSignal(PITSemaphore);

	OS_ISRExit();
}


/*!
** @}
*/
