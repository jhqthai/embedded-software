/*!file
 *
 *  @brief The implementation of routines for erasing and writing to the Flash.
 *
 * This contains the implementation for operating the LEDs.
 *
 * @author John Thai & Jason Garviel
 * @date 2017-04-10
 */
/*!
 * @addtogroup LED_module LED module documentation
 * @{
 */
/* MODULE LED */

#include "LEDs.h"
#include "MK70F12.h"

/*! @brief Enables LED Pins and configures the GPIO.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void)
{
	//Enable PORT A pin routing
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;

	//Enables pin for LEDs (Pin MUX ALT 1 GPIO K70RefMan p.317)
	PORTA_PCR11 |= PORT_PCR_MUX(1); //orange
	PORTA_PCR28 |= PORT_PCR_MUX(1); //yellow
	PORTA_PCR29 |= PORT_PCR_MUX(1); //green
	PORTA_PCR10 |= PORT_PCR_MUX(1); //blue

  //Set direction of data register
  GPIOA_PDDR |= LED_ORANGE;
  GPIOA_PDDR |= LED_YELLOW;
  GPIOA_PDDR |= LED_GREEN;
  GPIOA_PDDR |= LED_BLUE;

  //SET PDOR to 1 at the appropriate pin (LED OFF)
  GPIOA_PSOR |= LED_ORANGE;
  GPIOA_PSOR |= LED_YELLOW;
  GPIOA_PSOR |= LED_GREEN;
  GPIOA_PSOR |= LED_BLUE;
  return true;
}

/*! @brief Clears PDOR for the color in arguments (LED ON)
 *
 *  @param color The color of the LED to turn on.
 */
void LEDs_On(const TLED color)
{
  GPIOA_PCOR |= color;
}

/*! @brief Sets PDOR to 1 at the appropriate pin (LED OFF)
 *
 *  @param color The color of the LED to turn off.
 */
void LEDs_Off(const TLED color){
	GPIOA_PSOR |= color;
}


/*! @brief Toggles PDOR to the inverse of it's current state
 *
 *  @param color The color of the LED to turn toggle.
 */
void LEDs_Toggle(const TLED color){
	GPIOA_PTOR |= color;
}

/*!
** @}
*/



