/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-xx-xx
 */


#include "FIFO.h"
#include "types.h"

// Initialises the FIFO to starting values
void FIFO_Init(TFIFO * const FIFO)
{
  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->NbBytes = 0;
}

// gets an 8 bit int and puts it in the correct position of the FIFO buffer if space available
bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  // Return false if there is no room in FIFO
  if(FIFO->NbBytes >= FIFO_SIZE)
    return false;

  //If room, continue, increment end, return true
  FIFO->Buffer[FIFO->End++] = data; // The value of End gets incremented AFTER it is accessed
  FIFO->NbBytes++;
  if (FIFO->End>=FIFO_SIZE)
    FIFO->End = 0;

  return false;
}

//If data in the FIFO, retrieve the "first in" and stores it in the memory address of the pointer argument
bool FIFO_Get(TFIFO * const FIFO, uint8_t volatile * const dataPtr)
{
  // Return false if FIFO is empty
  if(FIFO->NbBytes == 0)
    return false;

  //If not, return true, increment start
  else
  {
    *dataPtr = FIFO->Buffer[FIFO->Start++]; // The value of Start is incremented AFTER it is accessed
    FIFO->NbBytes--;
    if (FIFO->Start>=FIFO_SIZE)
      FIFO->Start = 0;
    return true;
  }
}
