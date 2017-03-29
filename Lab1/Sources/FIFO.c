#include "FIFO.h"
//handles bools
#include "stdbool.h"

/*! @brief Initialize the FIFO before first use.
 *
 *  @param FIFO A pointer to the FIFO that needs initializing.
 *  @return void
 */
void FIFO_Init(TFIFO * const FIFO)
{
  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->NbBytes = 0;
}

/*! @brief Put one character into the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  // first need to check if there is space in the fifo
  if(FIFO->NbBytes >= FIFO_SIZE)
  {
  // if not, return false,
  return false;
  }
  else
  {
    // if room, continue, return true, increment end
    FIFO->Buffer[FIFO->End++] = data; //the value of End gets incremented AFTER it is accessed
	FIFO->NbBytes++;
	if (FIFO->End>=FIFO_SIZE)
	{
	  FIFO->End = 0;
	}
    return false;
  } 
}

/*! @brief Get one character from the FIFO.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Get(TFIFO * const FIFO, uint8_t * const dataPtr)
{
  // check if empty
  // if is, return false
  if(FIFO->NbBytes == 0)
  {
    return false;
  }
  // if not, return true, increment start
  else
  {
    *dataPtr = FIFO->Buffer[FIFO->Start++]; //the value of Start is incremented AFTER it is accessed
    FIFO->NbBytes--;
    if (FIFO->Start>=FIFO_SIZE)
    {
      FIFO->Start = 0;
    }	  	
    return true;
  }
}
