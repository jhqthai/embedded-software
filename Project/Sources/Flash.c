/*!file
 * 
 * Flash.c
 * @brief Implementation of routines for erasing and writing to the Flash.
 *
 * This contains the function definition needed for accessing and implementing the internal Flash.
 *  
 * @author John Thai & Jason Garviel
 * @date 2017-04-12
 */
/*!
 * @addtogroup Flash_module Flash module documentation
 * @{
 */  
/* MODULE Flash */

#include "Flash.h"
#include "MK70F12.h"
#include "math.h"


#define FCMD_ERSSCR 0x09 // Flash Command Erase Sector (p.882 RefManual)
#define FCMD_PPC 0x07 // Flash Command Program Phrase (p.819 RefManual)
//memory map of the first 8 bytes (binary representation: 1-> allocated 0-> unallocated)
static uint8_t MemoryMap;


/*! @brief Prepares for a command write sequence to clears error flags and waits for previous command to finish.
 *
 */
static void PrepareCWS(void)
{
	//Clear ACCERR and FPVIOL bits "cleared by writing a 1"
	FTFE_FSTAT |= FTFE_FSTAT_ACCERR_MASK;
	FTFE_FSTAT |= FTFE_FSTAT_FPVIOL_MASK;

	//Waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
}

/*! @brief Execute for a command write sequence
 *
 *  @return bool - TRUE if no error in the sequence.
 */
static bool ExecuteCWS(void)
{
	//Launches command
	FTFE_FSTAT |= FTFE_FSTAT_CCIF_MASK;

	//Waits for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

	//Checks for read collision, protection violation, access or runtime errors
	bool error = FTFE_FSTAT & (FTFE_FSTAT_RDCOLERR_MASK | FTFE_FSTAT_FPVIOL_MASK | FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_MGSTAT0_MASK);

	//Clears register for next command
	FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;

	//Waits again for command complete interrupt flag
	while(!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));

	return !(error);
}

/*! @brief Enables the Flash module.
 *
 *  @return bool - TRUE if the Flash was setup successfully.
 */
bool Flash_Init(void)
{
	//ENABLE NFC clock gate control
	//SIM_SCGC3 |= SIM_SCGC3_NFC_MASK;

	//Waits for command complete interrupt flag
	while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK));
	return true;
}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
	uint8_t startByte;
	uint8_t mapMask;

	//Different rules for byte, half-word and words
	switch (size)
	{
		case 1: //Byte to be placed anywhere that is free
			mapMask = 0x01; //checks single byte in map at a time
			for (startByte=0; startByte<8; startByte++)
			{
				if (!(MemoryMap&mapMask)) //checks MemoryMap to see if the byte at "startByte" is allocated or not
				{
					//If unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//Set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 1; //Bit-shifts mask ready to check next byte
			}
			return false;

		case 2: //Half-word, even addresses only
			mapMask = 0x03; //checks 2 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=2)
			{
				//Checks MemoryMap to see if the byte at "startByte" and the next byte are allocated or not
				if (!(MemoryMap&mapMask))
				{
					//If unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//Set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 2; //Bit-shifts mask ready to check next even address
			}
			return false;

		case 4:	//Word, addresses divisible by 4 only
			mapMask = 0x0F; //checks 4 bytes in map at a time
			for (startByte=0; startByte<8; startByte+=4)
			{
				//Checks MemoryMap to see if the byte at "startByte" and the next three bytes are allocated or not
				if (!(MemoryMap&mapMask))
				{
					//If unallocated, point pointer at flash address
					*variable = (void*) FLASH_DATA_START + startByte;
					//Set MemoryMap to indicated byte is allocated to an address
					MemoryMap |= mapMask;
					return true;
				}
				mapMask <<= 4; //Bit-shifts mask ready to check next divisible by four address
			}
			return false;
	}
	return false;
}


/*! @brief Writes a 64-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param phrase The uint64union_t phrase to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 */
static bool WritePhrase(const uint64_t* address, const uint64union_t phrase)
{

	Flash_Erase();

	PrepareCWS();

	uint32_8union_t flashAddressUnion;
	flashAddressUnion.l = (uint32_t) address;

	FTFE_FCCOB0 = FCMD_PPC; //Defines the FTFE command to write
	FTFE_FCCOB1 = flashAddressUnion.s.b; //Sets flash address[23:16] to 128
	FTFE_FCCOB2 = flashAddressUnion.s.c; //Sets bits 8-15 (inclusive) of flash address
	FTFE_FCCOB3 = (flashAddressUnion.s.d & 0xF8); //Sets first 8 bits of flash address

	//Most significant phrase
	uint32_8union_t firstWord;
	firstWord.l = phrase.s.Hi;

	//Least significant phrase
	uint32_8union_t secondWord;
	secondWord.l = phrase.s.Lo;


	//Allocate byte according to CCOB Register Endian
	FTFE_FCCOB7 = firstWord.s.a;
	FTFE_FCCOB6 = firstWord.s.b;
	FTFE_FCCOB5 = firstWord.s.c;
	FTFE_FCCOB4 = firstWord.s.d;

	FTFE_FCCOBB = secondWord.s.a;
	FTFE_FCCOBA = secondWord.s.b;
	FTFE_FCCOB9 = secondWord.s.c;
	FTFE_FCCOB8 = secondWord.s.d;

	return ExecuteCWS();
}


/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
	uint64union_t phrase;
	uint64_t *phraseAddress;

  // Checks to make sure word isn't written in an invalid word address
  if ((uint32_t)address&0x03)
    return false;

	//Check 3rd last bit of address
	bool thirdBitAddr = (uint32_t) address&0x04;
	if (thirdBitAddr)
	{
		//Address is second word of a phrase - decrement address by 4
		phraseAddress = (void*) address - 4;

		//Read and set existed data to it location to avoid override
		phrase.s.Hi = _FW(phraseAddress);

		//Set new data to location
		phrase.s.Lo = data;
	}
	else
	{
		//Address is first word of a phrase - do nothing
		phraseAddress = (void*) address;
		phrase.s.Hi = data;
		phrase.s.Lo = _FW(address+1);
	}
	return WritePhrase(phraseAddress, phrase);
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
	uint32union_t word;
	uint32_t *wordAddress;

  //checks to make sure half-word isnt written in an invalid half-word address
  if ((uint32_t)address&0x01)
    return false;

	bool secondBitAddr = (uint32_t) address&0x02;

	//Check 2nd last bit of address
	if (secondBitAddr)
	{
		//Address is second half-word of a word - decrement address by 2
		wordAddress = (void*) address-2;
		word.s.Hi = _FH(wordAddress);
		word.s.Lo = data;
	}
	else
	{
		//Address is first half-word of a word - do nothing
		wordAddress = (void*) address;
		word.s.Hi = data;
		word.s.Lo = _FH(address + 1);
	}

	return Flash_Write32(wordAddress, word.l);
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
	uint16union_t halfWord;
	uint16_t *halfWordAddress;

	//Check last bit of address to decide if in msb or lsb of half byte
	bool firstBitAddr = (uint32_t) address&0x01;
	if (firstBitAddr)
	{
		//Address is second byte of half-word - decrement address
		halfWordAddress = (void*) address-1;

		//Read and set existed data to it location to avoid override
		halfWord.s.Hi = _FP(halfWordAddress);

		//Set new data to location
		halfWord.s.Lo = data;
	}
	else
	{
		//Address is first byte of half-word - do nothing
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
bool Flash_Erase(void)
{
	//Clears error flags and waits for previous command to finish
	PrepareCWS();


	uint32_8union_t flashAddressUnion;
	flashAddressUnion.l = FLASH_DATA_START;

	FTFE_FCCOB0 = FCMD_ERSSCR;
	FTFE_FCCOB1 = flashAddressUnion.s.b; //Sets flash address[23:16] to 128
	FTFE_FCCOB2 = flashAddressUnion.s.c; //Sets flash address[15:8] to 0
	FTFE_FCCOB3 = (flashAddressUnion.s.d & 0xF0); //Sets flash address[7:0] to 0
	
	//Runs command write sequence and returns true/false depending on errors
	return ExecuteCWS();
}

/*!
** @}
*/





