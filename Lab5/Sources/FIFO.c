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
	// Reset FIFO to initial values
  FIFO->Start = 0;
  FIFO->End = 0;
  FIFO->NbBytes = 0;

	// Initialise semaphore
	FIFO->BufferAccess = OS_SemaphoreCreate(1);
	FIFO->SpaceAvailable = OS_SemaphoreCreate(FIFO_SIZE);
	FIFO->ItemsAvailable = OS_SemaphoreCreate(0);

}

bool FIFO_Put(TFIFO * const FIFO, const uint8_t data)
{
	// Wait for space to become available
	OS_SemaphoreWait(FIFO->SpaceAvailable, 0);
	// Obtain exclusive access to the FIFO
	OS_SemaphoreWait(FIFO->BufferAccess, 0);

	// Put data into FIFO
	//The value of End gets incremented AFTER it is accessed
  FIFO->Buffer[FIFO->End] = data;
  // Increment number of bytes in FIFO
  FIFO->NbBytes++;
  FIFO->End++;

  //Wrap FIFO when end of FIFO
  if (FIFO->End>=FIFO_SIZE)
    FIFO->End = 0;

  // Relinquish exclusive access to the FIFO
  OS_SemaphoreSignal(FIFO->BufferAccess);
  // Increment the number of items available
  OS_SemaphoreSignal(FIFO->ItemsAvailable);
  return true;
}


bool FIFO_Get(TFIFO * const FIFO, uint8_t volatile * const dataPtr)
{

	// Wait for items to become available
	OS_SemaphoreWait(FIFO->ItemsAvailable, 0);
	// Obtain exclusive access to the FIFO
	OS_SemaphoreWait(FIFO->BufferAccess, 0);

  // The value of start variable is incremented AFTER it is accessed
  *dataPtr = FIFO->Buffer[FIFO->Start];
  FIFO->NbBytes--;
  FIFO->Start++;
  if (FIFO->Start>=FIFO_SIZE)
    FIFO->Start = 0;

  // Relinquish exclusive access to the FIFO
  OS_SemaphoreSignal(FIFO->BufferAccess);
  // Increment the number of items available
  OS_SemaphoreSignal(FIFO->SpaceAvailable);

  return true;
}

/*!
** @}
*/
