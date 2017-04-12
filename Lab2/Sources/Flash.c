/*
 * Flash.c
 *@brief Routines for erasing and writing to the Flash.
 *
 *  This contains the implementation needed for accessing the internal Flash.
 *  Created on: 12 Apr 2017
 *      Author: John Thai & Jason Garviel
 */

#include "Flash.h"
#include "MK70F12.h"
#include "math.h"


//memeory map of the first 8 bytes (binary representation: 1-> allocated 0-> unallocated)
uint8_t mMap;

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void){
	//ENABLE NFC clock gate control
	//SIM_SCGC3 |= SIM_SCGC3_NFC_MASK;

	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size){
	uint8_t startByte;
	uint8_t mapMask;

	//different rules for byte, half-word and words
	switch(size)
	{
		case 1: //byte to be placed anywhere that is free
			mapMask = 0x01; //checks single byte in map at a time
			for (startByte=0; startByte<8; startByte++)
			{
				if (!(mMap&mapMask)) //checks mMap to see if the byte at "startByte" is allocated or not
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set mMap to indicated byte is allocated to an address
					mMap |= mapMask;
					return true;
				}
				mapMask <<= 1; //bit-shifts mask ready to check next byte
			}
			return false;

		case 2: //half-word, even addresses only
			mapMask = 0x03; //checks 2 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=2)
			{
				//checks mMap to see if the byte at "startByte" and the next byte are allocated or not
				if (!(mMap&mapMask))
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set mMap to indicated byte is allocated to an address
					mMap |= mapMask;
					return true;
				}
				mapMask <<= 2; //bit-shifts mask ready to check next even address
			}
			return false;

		case 4:	//word, addresses divisible by 4 only
			mapMask = 0x0F; //checks 4 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=4)
			{
				//checks mMap to see if the byte at "startByte" and the next three bytes are allocated or not
				if (!(mMap&mapMask))
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set mMap to indicated byte is allocated to an address
					mMap |= mapMask;
					return true;
				}
				mapMask <<= 4; //bit-shifts mask ready to check next divisible by four address
			}
			return false;
	}
	return false;
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data){

	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data){

	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data){

	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void){

	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
	
	FCCOB0 |= FCMD_ERSSCR; //FCMD Erase sector (p.882 K70RefManual)
	FCC0B1 |= //bits 16-23 (inclusive) of flash address
	FCC0B2 |= //bits 8-15 (inclusive) of flash address
	FCC0B3 |= //first 8 bits of flash address
}







