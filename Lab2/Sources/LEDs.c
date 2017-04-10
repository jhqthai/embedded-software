/*
 * LEDs.c
 *
 * This contains the implementation for operating the LEDs.
 *
 *  Created on: 10 Apr 2017
 *      Author: John Thai & Jason Garviel
 */

#include "LEDs.h"
#include "MK70F12.h"
//is this right naming convention?

//Enables LED Pins and configures the GPIO
bool LEDs_Init(void)
{
	//Enable PORT A pin routing
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;

	//enables pin for LEDs (Pin MUX ALT 1 GPIO K70RefMan p.317)
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
}

//clears PDOR for the colour in arguments (LED ON)
void LEDs_On(const TLED color)
{
  GPIOA_PCOR |= color;
}

//sets PDOR to 1 at the appropriate pin (LED OFF)
void LEDs_Off(const TLED color){
	GPIOA_PSOR |= color;
}

//toggles PDOR to the inverse of it's current state
void LEDs_Toggle(const TLED color){
	GPIOA_PTOR |= color;
}




