/*! @file UART.c
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for operating the UART (serial port).
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup UART_module UART module documentation
 **  @{
 */
#include "UART.h"
#include "FIFO.h"
#include "MK70F12.h"
#include "Cpu.h"

//@{
//!< Global static variables for the Tx and Rx FIFOs
static TFIFO RxFIFO; //!< FIFO for input data
static TFIFO TxFIFO; //!< FIFO for output data
//@}

/*! @brief Sets up the UART interface before first use.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the UART was successfully initialized.
 */
bool UART_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  // Initialize the RxFIFO for input packets
  FIFO_Init(&RxFIFO);

  // Initialize the TxFIFO for output packets
  FIFO_Init(&TxFIFO);

  // Enable clock gate control bit for UART2
  SIM_SCGC4 |= SIM_SCGC4_UART2_MASK;

  // Enable clock gate control bit for PORTE
  SIM_SCGC5 |= SIM_SCGC5_PORTE_MASK;

  // Clear TE bit to disable UART transmitter
  UART2_C2 &= ~UART_C2_TE_MASK;

  // Clear RE to disable UART receiver
  UART2_C2 &= ~UART_C2_RE_MASK;

  // Clear M bit so the tower is working with 8 data bits
  UART2_C1 &= ~UART_C1_M_MASK;

  // Clear PE bit to disable (no) parity
  UART2_C1 &= ~UART_C1_PE_MASK;

  // Assign PTE16 pin to ALT3 functionality (UART2_TX)
  PORTE_PCR16 |= PORT_PCR_MUX(3);

  // Assign PTE17 pin to ALT3 functionality (UART2_RX)
  PORTE_PCR17 |= PORT_PCR_MUX(3);

  // Calculate the baud rate divisor
  uint16union_t baudRateDivisor = { .l = moduleClk / (baudRate * 16) };

  // Clear baud rate fine adjust (BFRA) bit
  UART2_C4 &= ~UART_C4_BRFA_MASK; //TODO - REMOVE THIS!.. or not

  // Set last 5 bits of BDH to the bits to the last 5 bits
  // of the high byte of the baud rate divisor
  UART2_BDH |= UART_BDH_SBR(baudRateDivisor.s.Hi);

  // Set BDL to the low byte of the baud rate divisor
  UART2_BDL = baudRateDivisor.s.Lo;

  // Calculate the baud rate fine adjustment
  uint16_t bfra = (moduleClk * 32 / (baudRate * 16)) - baudRateDivisor.l * 32;

  // Enable the baud rate fine adjust
  UART2_C4 |= UART_C4_BRFA(bfra);

  // Disable the transmit interrupt
  UART2_C2 &= ~UART_C2_TIE_MASK;

  // Enable the receive interrupt
  UART2_C2 |= UART_C2_RIE_MASK;

  // Clear any pending interrupts from UART2 TX_RX
  NVICICPR1 |= (1 << 17);

  // Turn on NVIC for UART2
  NVICISER1 |= (1 << 17);

  // Enable UART transmitter
  UART2_C2 |= UART_C2_TE_MASK;

  // Enable UART reciever
  UART2_C2 |= UART_C2_RE_MASK;

  return true;
}

/*! @brief Get a character from the receive FIFO if it is not empty.
 *
 *  @param dataPtr A pointer to memory to store the retrieved byte.
 *  @return bool - TRUE if the receive FIFO returned a character.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_InChar(uint8_t * const dataPtr)
{
  return FIFO_Get(&RxFIFO, dataPtr);
}

/*! @brief Put a byte in the transmit FIFO if it is not full.
 *
 *  @param data The byte to be placed in the transmit FIFO.
 *  @return bool - TRUE if the data was placed in the transmit FIFO.
 *  @note Assumes that UART_Init has been called.
 */
bool UART_OutChar(const uint8_t data)
{
  bool status = FIFO_Put(&TxFIFO, data);
  if (status == true)
  {
    // Enable the transmit interrupt
    UART2_C2 |= UART_C2_TIE_MASK;
  }
  return status;
}

/*! @brief Poll the UART status register to try and receive and/or transmit one character.
 *
 *  @return void
 *  @note Assumes that UART_Init has been called.
 */
void UART_Poll(void)
{
  // Check the RDRF bit to see if the receive data register is full
  bool readyToRead = UART2_S1 & UART_S1_RDRF_MASK;

  // Check the TDRE bit to see if the transmit data register is empty
  bool readyToTransmit = UART2_S1 & UART_S1_TDRE_MASK;

  // If RDRF is set, read in the data and add it to the RxFIFO
  if (readyToRead)
  {
    // Read and store data from the data register into a local variable
    uint8_t input = UART2_D;

    // Put the data stored in our local variable into the RxFIFO
    FIFO_Put(&RxFIFO, input);
  }

  // If TDRE is set, get a byte from TxFIFO and output it to the data register
  if (readyToTransmit)
  {
    uint8_t data;

    // Check if there is a byte in TxFIFO that ready to be transmitted
    // If there is, get that byte and store it in our local variable (data)
    bool result = FIFO_Get(&TxFIFO, &data);

    // If there is data to be transmitted
    if (result == true)
    {
      // Set the UART data register to the output data
      UART2_D = data;
    }
  }
}

void __attribute__ ((interrupt)) UART_ISR0(void)
{
  if ((UART2_S1 & UART_S1_TDRE_MASK) && (UART2_C2 & UART_C2_TIE_MASK)) // if the TDRE and TIE flag is set
  {
    if (!FIFO_Get(&TxFIFO, (uint8_t * const ) &UART2_D)) // Checks if it can get data from the FIFO
      UART2_C2 &= ~UART_C2_TIE_MASK;               // disable interrupt requests
  }

  if ((UART2_S1 & UART_S1_RDRF_MASK) && (UART2_C2 & UART_C2_RIE_MASK)) // if the RDRF and RIE flag is set
  {
    FIFO_Put(&RxFIFO, UART2_D);                         // Puts data on the FIFO
  }
}

void __attribute__ ((interrupt)) UART_ISR(void)
{
  // If the receive interrupt is enabled
  if (UART2_C2 & UART_C2_RIE_MASK)
  {
    // If RDRF flag is set, read a character
    if (UART2_S1 & UART_S1_RDRF_MASK)
    {
      // Acknowledge interrupt, clear interrupt flag bit
      UART2_S1 &= ~UART_S1_RDRF_MASK;

      // Read and store data from the data register into a local variable
      uint8_t input = UART2_D;

      // Put the data stored in our local variable into the RxFIFO
      FIFO_Put(&RxFIFO, input);
      //return;
    }
  }

  // If the transmit interrupt is enabled
  if (UART2_C2 & UART_C2_TIE_MASK)
  {
    // If TDRE flag is set, transmit a character
    if (UART2_S1 & UART_S1_TDRE_MASK)
    {
      // Acknowledge interrupt, clear interrupt flag bit
      UART2_S1 &= ~UART_S1_TDRE_MASK;

      uint8_t data;

      // Check if there is a byte in TxFIFO that ready to be transmitted
      // If there is, get that byte and store it in our local variable (data)
      bool result = FIFO_Get(&TxFIFO, &data);

      // If there is data to be transmitted
      if (result == true)
      {
        // Set the UART data register to the output data
        UART2_D = data;
      }
      // FIFO empty
      else
      {
        // Disable the transmit interrupt
        UART2_C2 &= ~UART_C2_TIE_MASK;
      }
      //return;
    }
  }
}

/*!
 ** @}
 */
