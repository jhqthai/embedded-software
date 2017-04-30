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
static void (*PITCallback)(void*); // User callback function pointer
static void* PITArguments; // User arguments pointer to use with user callback function

/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
	PITModuleClk = moduleClk;
	PITCallback = userFunction;
	PITArguments = userArguments;

	// Enable PIT SCGC
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;
	
	// Enable PIT timer clock by clearing bit 0
	PIT_MCR &= ~PIT_MCR_MDIS_MASK;
	
	// Enable freeze when debug by setting bit 1
	PIT_MCR |= PIT_MCR_FRZ_MASK;
	
	// Enable Timer0 interrupt
	PIT_TCTRL0 = PIT_TCTRL_TIE_MASK;
	
  // Initialize NVIC
  // see p. 91 of K70P256M150SF3RM.pdf
  // Vector 0x41=65, IRQ=68
  // NVIC non-IPR=2 IPR=12
  // Clear any pending interrupts on LPTMR
  NVICICPR2 = (1 << 4); //IRQ mod 32 (p.92 K70)
  // Enable interrupts from LPTMR module
  NVICISER2 = (1 << 4);

	return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
	// Clock period (ns) = 1/module clock rate (MHZ)
	// Clock cycles = interrupt period/clock period
	// LDVAL trigger = clock cycles - 1
	uint32_t triggerLDVAL = (period/(1/PITModuleClk*1e6))- 1;

	if (restart)
	{
		PIT_Enable(false);
		PIT_LDVAL0 = PIT_LDVAL_TSV(triggerLDVAL); // Set up timer for triggerLDVAL clock cycles
		PIT_Enable(true);
	}
	else
		PIT_LDVAL0 = PIT_LDVAL_TSV(triggerLDVAL);
}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
	if (enable)
		PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK; // Enable PIT
	else
		PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK; // Disable PIT
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT_ISR(void)
{
	//PIT_TFLG0 |= PIT_TFLG_TIF_MASK; // Timeout
	//(PITCallback)(PITArguments); //NOTSURE!!!
	if (PITCallback)
		(*PITCallback)(PITArguments);
}


/*!
** @}
*/

