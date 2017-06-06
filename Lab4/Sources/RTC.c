/*! @file
 *
 *  @brief Implementation of routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  This contains the functions definition for operating the real time clock (RTC).
 *
 *  @author John Thai, Jason Gavriel
 *  @date 2017-04-30
 */
/*!
 * @addtogroup RTC_module RTC module documentation
 * @{
 */

#include "RTC.h"
#include "MK70F12.h"

// Global Variables
static void (*RTCCallback)(void*); // User callback function pointer
static void* RTCArguments; // User arguments pointer to use with user callback function


bool RTC_Init(void (*userFunction)(void*), void* userArguments)
{
	// Set variable to private global variable
	RTCCallback = userFunction;
	RTCArguments = userArguments;

	// Enable  RTC SCGC
	SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

	// Enable RTC oscillator
	RTC_CR |= RTC_CR_OSCE_MASK;

	// Set RTC oscillator load
	RTC_CR |= RTC_CR_SC16P_MASK | RTC_CR_SC2P_MASK;

	// Write initial value to TSR for hard reset
	RTC_TSR &= ~RTC_TSR_TSR_MASK;

	// Lock control register
	RTC_LR &= ~RTC_LR_CRL_MASK;

	// Enable RTC time counter
	RTC_SR |= RTC_SR_TCE_MASK;

	// Enable time seconds interrupt
	RTC_IER |= RTC_IER_TSIE_MASK;


	// Disable unused interrupt
	RTC_IER &= ~(RTC_IER_TAIE_MASK | RTC_IER_TOIE_MASK | RTC_IER_TIIE_MASK);




  // Initialize NVIC
  // Vector 0x53=83, IRQ=67
  // NVIC non-IPR=2, IPR=16
	// IRQ mod 32 (p.92 K70.pdf)
	// Clear any pending interrupts on RTC
	NVICICPR2 = (1 << 3);

	// Enable interrupts from RTC module
  NVICISER2 = (1 << 3);

	return true;

}


void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
	// Convert hours to seconds
	uint32_t hoursInSeconds = (hours%24)*3600;

	// Convert minutes to seconds
	uint32_t minutesInSeconds = (minutes%60)*60;

	// Add up total seconds since day start
	uint32_t secondsTotal = seconds%60 + minutesInSeconds + hoursInSeconds;

	// Disable RTC time counter so we can write time
	RTC_SR &= ~RTC_SR_TCE_MASK;

	// Write seconds to TSR
	RTC_TSR = secondsTotal;

	// Enable RTC time counter
	RTC_SR |= RTC_SR_TCE_MASK;

}


void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
	// Gets current time in seconds and splits into hours/mins/seconds
	uint32_t secondsTotal = RTC_TSR;
	uint32_t secondsTotal2 = RTC_TSR;
	while(secondsTotal != secondsTotal2) //reads twice and verifies there was no change during first read
	{
		secondsTotal = RTC_TSR;
		secondsTotal2 = RTC_TSR;
	}

	*seconds = (uint8_t)(secondsTotal%60);
	*minutes = (uint8_t)((secondsTotal)/60 % 60);
	*hours = (uint8_t)((secondsTotal)/3600 % 24);
}


void __attribute__ ((interrupt)) RTC_ISR(void)
{
	// Runs callback method if interrupt requested
	if (RTCCallback)
		(*RTCCallback)(RTCArguments);
}

/*!
** @}
*/
