/*
 * FTM.c
 *  @brief Implementation of routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the function definition for operating the FlexTimer module (FTM).
 *
 *  @author John Thai & Jason Garviel
 *  @date 2017-04-27
 */
/*!
 *  @addtogroup FTM_module FTM module documentation
 *  @{
 */
/* MODULE FTM */

#include "FTM.h"
#include "MK70F12.h"

#define MCGFFCLK 0x02 //Fix frequency clock bit
#define NUM_OF_CHANNELS 8 // Number of supported channels

OS_ECB *FTMSemaphore;   /*!< Binary semaphore for FTM thread */


bool FTM_Init()
{
  FTMSemaphore = OS_SemaphoreCreate(1);

	// Enable FTM0 Module
	SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;

	// Enable FTM as free running 16-bit counter (p.1257 K70)
	// Set FTM counter initial value to 0x0000
	FTM0_CNTIN &= ~FTM_CNTIN_INIT_MASK;

	// Set FTM counter before FTM counter mode to avoid first overflow confusion
	// Set FTM counter to any value
	FTM0_CNT = FTM_CNTIN_INIT(0);

	// Set FTM counter mode value to 0xFFFF
	FTM0_MOD |= FTM_MOD_MOD_MASK;

	// Set FTM clock to fixed frequency clock
	FTM0_SC |= FTM_SC_CLKS(MCGFFCLK);

	// Initialize NVIC
	// Vector 0x4E=78, IRQ=62
	// NVIC non-IPR=1, IPR=15
  // IRQ mod 32 (p.92 K70.pdf)
	// Clears pending interrupts on FMT0
	NVICICPR1 = (1<<30);

	// Enables interrupts on FTM0
	NVICISER1 = (1<<30);

  return true;
}


bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
	// Check selected channel
	if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
	{
		// Disable write protection
		FTM0_MODE |= FTM_MODE_WPDIS_MASK;

		// Select mode (p.1212 K70)
		FTM0_CnSC(aFTMChannel->channelNb) |= aFTMChannel->timerFunction << FTM_CnSC_MSA_SHIFT;


		//Select edge or level (p.1212 K70)
		FTM0_CnSC(aFTMChannel->channelNb) |= aFTMChannel->ioType.outputAction << FTM_CnSC_ELSA_SHIFT;

		//Enable write protection
		FTM0_MODE &= ~FTM_MODE_WPDIS_MASK;

		return true;
	}
	//Not output compare - not supported
	return false;

}


bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
	// Produce output compare interrupt
	if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
	{
		// Enable channel output compare with interrupt enabled
		FTM0_CnSC(aFTMChannel->channelNb) = (FTM_CnSC_MSA_MASK | FTM_CnSC_CHIE_MASK);

		// Add delay to counter and store as interrupt time
		FTM0_CnV(aFTMChannel->channelNb) = FTM0_CNT + aFTMChannel->delayCount;

		return true;
	}
	return false;
}


void __attribute__ ((interrupt)) FTM0_ISR(void)
{
	uint8_t i; // Channel counter variable
	// Runs through all channels and checks to see which one requests a callback
	for (i = 0; i < NUM_OF_CHANNELS; i++)
	{
		if (FTM0_CnSC(i) | FTM_CnSC_CHF_MASK | FTM_CnSC_CHIE_MASK)
		{
			// Reset interrupts
			FTM0_CnSC(i) &= ~(FTM_CnSC_CHF_MASK | FTM_CnSC_CHIE_MASK);

			// Signal FTM semaphore
			OS_SemaphoreSignal(FTMSemaphore);
		}
	}
}

/*!
** @}
*/

