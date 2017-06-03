/*! @file
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the implementation and function definition FIFO buffer.
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-04-24
 */
/*!
**  @addtogroup FIFO_module FIFO module documentation
**  @{
*/  
/* MODULE FIFO */


#include "FIFO.h"
#include "types.h"
#include "CPU.h"
#include "PE_Types.h"

void FIFO_Init(TFIFO * const FIFO)
{
  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->NbBytes = 0;
}

bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
	EnterCritical();
	//Return false if there is no room in FIFO
  if (FIFO->NbBytes >= FIFO_SIZE)
  {
  	ExitCritical();
    return false;
  }

  //If room, continue, increment end, return true
  FIFO->Buffer[FIFO->End++] = data; //The value of End gets incremented AFTER it is accessed
  FIFO->NbBytes++;
  if (FIFO->End>=FIFO_SIZE)
    FIFO->End = 0;
  ExitCritical();

  return true;
}


bool FIFO_Get(TFIFO * const FIFO, uint8_t volatile * const dataPtr)
{
	// Enter critical section
	EnterCritical();
  //Return false if FIFO is empty
  if (FIFO->NbBytes == 0)
  {
    ExitCritical();
  	return false;
  }

  //If not, return true, increment start
  else
  {
  	// The value of start variable is incremented AFTER it is accessed
  	*dataPtr = FIFO->Buffer[FIFO->Start++];
    FIFO->NbBytes--;
    if (FIFO->Start>=FIFO_SIZE)
      FIFO->Start = 0;

    ExitCritical();

    return true;
  }
}

/*!
** @}
*/
