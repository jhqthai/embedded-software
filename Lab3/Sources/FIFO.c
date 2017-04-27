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


/*! @brief Initialises the FIFO to starting values.
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

/*! @brief gets an 8 bit int and puts it in the correct position of the FIFO buffer if space available.
 *
 *  @param FIFO A pointer to a FIFO struct where data is to be stored.
 *  @param data A byte of data to store in the FIFO buffer.
 *  @return bool - TRUE if data is successfully stored in the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
  //Return false if there is no room in FIFO
  if(FIFO->NbBytes >= FIFO_SIZE)
    return false;

  EnterCritical();
  //If room, continue, increment end, return true
  FIFO->Buffer[FIFO->End++] = data; //The value of End gets incremented AFTER it is accessed
  FIFO->NbBytes++;
  if (FIFO->End>=FIFO_SIZE)
    FIFO->End = 0;
  ExitCritical();

  return false;
}

/*! @brief data in the FIFO, retrieve the "first in" and stores it in the memory address of the pointer argument.
 *
 *  @param FIFO A pointer to a FIFO struct with data to be retrieved.
 *  @param dataPtr A pointer to a memory location to place the retrieved byte.
 *  @return bool - TRUE if data is successfully retrieved from the FIFO.
 *  @note Assumes that FIFO_Init has been called.
 */
bool FIFO_Get(TFIFO * const FIFO, uint8_t volatile * const dataPtr)
{
  //Return false if FIFO is empty
  if(FIFO->NbBytes == 0)
    return false;

  //If not, return true, increment start
  else
  {

  	EnterCritical();
  	*dataPtr = FIFO->Buffer[FIFO->Start++]; // The value of Start is incremented AFTER it is accessed
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
