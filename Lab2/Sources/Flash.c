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


#define FCMD_ERSSCR 0x09 // Flash Command Erase Phrase (p.808 RefManual)
#define FCMD_PPC 0x07 // Flash Command Program Phrase (p.819 RefManual)
//memeory map of the first 8 bytes (binary representation: 1-> allocated 0-> unallocated)
static uint8_t MemoryMap;


//Prepares for a command write sequence
static void Prepare_CWS(void){
	//clear ACCERR and FPVIOL bits "cleared by writing a 1"
	FTFE_FSTAT |= FTFE_FSTAT_ACCERR_MASK;
	FTFE_FSTAT |= FTFE_FSTAT_FPVIOL_MASK;
	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
}

static bool Execute_CWS(void){
	//launches command
	FTFE_FSTAT |= FTFE_FSTAT_CCIF_MASK;
	//waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

	//checks for read collision, protection violation, access or runtime errors
	bool error = FTFE_FSTAT & (FTFE_FSTAT_RDCOLERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_MGSTAT0_MASK);

	//clears register for next command
	FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;
	return !(error);
}

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
				if (!(MemoryMap&mapMask)) //checks MemoryMap to see if the byte at "startByte" is allocated or not
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 1; //bit-shifts mask ready to check next byte
			}
			return false;

		case 2: //half-word, even addresses only
			mapMask = 0x03; //checks 2 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=2)
			{
				//checks MemoryMap to see if the byte at "startByte" and the next byte are allocated or not
				if (!(MemoryMap&mapMask))
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 2; //bit-shifts mask ready to check next even address
			}
			return false;

		case 4:	//word, addresses divisible by 4 only
			mapMask = 0x0F; //checks 4 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=4)
			{
				//checks MemoryMap to see if the byte at "startByte" and the next three bytes are allocated or not
				if (!(MemoryMap&mapMask))
				{
					//if unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 4; //bit-shifts mask ready to check next divisible by four address
			}
			return false;
	}
	return false;
}

//
static bool WritePhrase (const uint64_t* address, const uint64union_t phrase){

	Flash_Erase();

	Prepare_CWS();

/*
	TFloat flashAddressUnion;
	flashAddressUnion.d = *address;

	FTFE_FCCOB0 = FCMD_PPC;
	FTFE_FCCOB1 = flashAddressUnion.dParts.dLo.s.Hi; //bits 16-23 (inclusive) of flash address
	FTFE_FCCOB2 = flashAddressUnion.dParts.dHi.s.Lo; //bits 8-15 (inclusive) of flash address
	FTFE_FCCOB3 = (flashAddressUnion.dParts.dHi.s.Hi & 0xF8); //first 8 bits of flash address
*/

	uint32_8union_t flashStart;
	flashStart.l = (void*) address;

	FTFE_FCCOB0 = FCMD_PPC; // defines the FTFE command to write
	FTFE_FCCOB1 = flashStart.s.b; // sets flash address[23:16] to 128
	FTFE_FCCOB2 = flashStart.s.c; // sets flash address[15:8] to 0
	FTFE_FCCOB3 = (flashStart.s.d & 0xF8);

	uint32_8union_t firstWord; //Most significant
	firstWord.l = phrase.s.Hi;
	uint32_8union_t secondWord; //Least significant
	secondWord.l = phrase.s.Lo;



	FTFE_FCCOB7 = firstWord.s.a;
	FTFE_FCCOB6 = firstWord.s.b;
	FTFE_FCCOB5 = firstWord.s.c;
	FTFE_FCCOB4 = firstWord.s.d;

	FTFE_FCCOBB = secondWord.s.a;
	FTFE_FCCOBA = secondWord.s.b;
	FTFE_FCCOB9 = secondWord.s.c;
	FTFE_FCCOB8 = secondWord.s.d;

	return Execute_CWS();
}


/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data){
	uint64union_t phrase;
	uint64_t *phraseAddress;

	//check 3rd last bit of address
	bool x = (uint32_t) address&0x04;
	if(x)
	{
		//address is second word of a phrase - decrement address by 4
		phraseAddress = (void*) address - 4;
		phrase.s.Hi = _FW(phraseAddress);
		phrase.s.Lo = data;
	}
	else
	{
		//address is first word of a phrase - do nothing
		phraseAddress = (void*) address;
		phrase.s.Hi = data;
		phrase.s.Lo = _FW(address+4);
	}

	return WritePhrase(phraseAddress, phrase);
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data){
	uint32union_t word;
	uint32_t *wordAddress;

	bool x = (uint32_t) address&0x02;
	//check 2nd last bit of address
	if(x)
	{
		//address is second half-word of a word - decrement address by 2
		wordAddress = (void*) address-2;
		word.s.Hi = _FH(wordAddress);
		word.s.Lo = data;
	}
	else
	{
		//address is first half-word of a word - do nothing
		wordAddress = (void*) address;
		word.s.Hi = data;
		word.s.Lo = _FH(address+2);
	}

	return Flash_Write32(wordAddress, word.l);
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data){
	uint16union_t halfWord;
	uint16_t *halfWordAddress;

	bool x = (uint32_t) address&0x01;
	//check last bit of address
	if(x)
	{
		//address is second byte of half-word - decrement address
		halfWordAddress = (void*) address-1;
		halfWord.s.Hi = _FP(halfWordAddress);
		halfWord.s.Lo = data;
	}
	else
	{
		//address is first byte of half-word - do nothing
		halfWordAddress = (void*) address;
		halfWord.s.Hi = data;
		halfWord.s.Lo = _FB(address+1);
	}

	return Flash_Write16(halfWordAddress, halfWord.l);
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void){
	//clears error flags and waits for previous command to finish
	Prepare_CWS();

	//Uses TFloat from types.h which is equivalent of two 16bit units
	//Can be further split up into uint8's
/*
	TFloat flashAddressUnion;
	flashAddressUnion.d = FLASH_DATA_START;

	FTFE_FCCOB0 = FCMD_ERSSCR; //FCMD Erase sector (p.882 K70RefManual)
	FTFE_FCCOB1 = flashAddressUnion.dParts.dLo.s.Hi; //bits 16-23 (inclusive) of flash address
	FTFE_FCCOB2 = flashAddressUnion.dParts.dHi.s.Lo; //bits 8-15 (inclusive) of flash address
	FTFE_FCCOB3 = (flashAddressUnion.dParts.dHi.s.Hi & 0xF0); //first 8 bits of flash address
*/
	uint32_8union_t flashStart;
	flashStart.l = FLASH_DATA_START;

	FTFE_FCCOB0 = FCMD_ERSSCR; // defines the FTFE command to erase
	FTFE_FCCOB1 = flashStart.s.b; // sets flash address[23:16] to 128
	FTFE_FCCOB2 = flashStart.s.c; // sets flash address[15:8] to 0
	FTFE_FCCOB3 = (flashStart.s.d & 0xF0); // sets flash address[7:0] to 0
	//runs command write sequence and returns true/false depending on errors
	return Execute_CWS();

}







