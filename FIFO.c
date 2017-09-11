/*! @file FIFO.c
 *
 *  @brief Routines to implement a FIFO buffer.
 *
 *  This contains the structure and "methods" for accessing a byte-wide FIFO.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup FIFO_module FIFO module documentation
 **  @{
 */

#include "FIFO.h"
#include "Cpu.h"

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
  // Enter critical section to prevent a higher priority interrupt
  // to execute while the FIFO is being changed
  EnterCritical()
  ;

  // Check if the FIFO is not full
  if (FIFO->NbBytes < FIFO_SIZE)
  {
    // Put the data into buffer and increase the byte count (NbBytes)
    FIFO->Buffer[FIFO->End] = data;
    FIFO->NbBytes++;

    // Increment the Start member of the FIFO and modulus it to make
    // sure the index stays within the bounds of 0 and FIFO_SIZE-1
    FIFO->End = (FIFO->End + 1) % FIFO_SIZE;

    // Exit the critical section, an interrupts called while in
    // the critical section are resumed
    ExitCritical();
    return true;

  }
  ExitCritical();
  return false;
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
  // Enter critical section to prevent a higher priority interrupt
  // to execute while the FIFO is being changed
  EnterCritical()
  ;

  // Check if the FIFO is not empty
  if (FIFO->NbBytes > 0)
  {
    // Dereference the data pointer to the first byte from the FIFO buffer
    // and decrement the byte count (NbBytes)
    *dataPtr = FIFO->Buffer[FIFO->Start];
    FIFO->NbBytes--;

    // Increment the Start member of the FIFO and modulus it to make
    // sure the index stays within the bounds of 0 and FIFO_SIZE-1
    FIFO->Start = (FIFO->Start + 1) % FIFO_SIZE;

    // Exit the critical section, an interrupts called while in
    // the critical section are resumed
    ExitCritical();
    return true;
  }
  ExitCritical();
  return false;
}

/*!
 ** @}
 */
