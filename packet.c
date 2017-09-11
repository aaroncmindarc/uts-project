/*! @file packet.c
 *
 *  @brief Routines to implement packet encoding and decoding for the serial port.
 *
 *  This contains the functions for implementing the "Tower to PC Protocol" 5-byte packets.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup packet_module packet module documentation
 **  @{
 */
#include "packet.h"
#include "Cpu.h"
#include "UART.h"

//!< The ACK bit is at pos 7 in the command byte of the packet
#define PACKET_ACK_SHIFT 7

//!< This macro takes a bool 'x' and converts it into ACK form
#define PACKET_ACK(x) (((uint8_t)(((uint8_t)(x))<<PACKET_ACK_SHIFT))&PACKET_ACK_MASK)

//!<Mask for the 7th bit in Packet_Command
const uint8_t PACKET_ACK_MASK = 128u;

//@{
//!< Initialize extern global variables
uint8_t Packet_Command = 0;
uint8_t Packet_Parameter1 = 0;
uint8_t Packet_Parameter2 = 0;
uint8_t Packet_Parameter3 = 0;
uint8_t Packet_Checksum = 0;
//@}

//static int variable to keep count of bytes received in packet
static int ByteCount = 0;

/*! @brief Calculates the checksum of the packet bytes.
 *
 *  @param command The command byte of the packet
 *  @param parameter1 The parameter1 byte of the packet
 *  @param parameter2 The parameter2 byte of the packet
 *  @param parameter3 The parameter3 byte of the packet
 *  @return uint_8 - The checksum'd value of all the parameters
 */
static uint8_t calculateChecksum(const uint8_t command,
    const uint8_t parameter1, const uint8_t parameter2,
    const uint8_t parameter3)
{
  return (command ^ parameter1 ^ parameter2 ^ parameter3);
}

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk)
{
  return UART_Init(baudRate, moduleClk);
}

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Get(void)
{
  // local variable to store first byte from transmit FIFO buffer
  uint8_t data;

  // if the transmit FIFO buffer is empty, early exit and return false
  if (!UART_InChar(&data))
    return false;

  // check how many bytes are currently in the packet then
  // take next action (move to next state)
  switch (ByteCount)
  {
  case 0:
    // If there are 0 bytes in packet, set Packet_Command to data
    Packet_Command = data;
    break;
  case 1:
    // If there is 1 byte in packet, set Parameter1 to data
    Packet_Parameter1 = data;
    break;
  case 2:
    // If there are 2 bytes in packet, set Parameter2 to data
    Packet_Parameter2 = data;
    break;
  case 3:
    // If there are 3 bytes in packet, set Parameter3 to data
    Packet_Parameter3 = data;
    break;
  case 4:
    // If we have 4 bytes in the packet, set checksum to data
    Packet_Checksum = data;

    // Check if the checksum of the first 4 bytes is equal to
    // Packet_Checksum
    if (calculateChecksum(Packet_Command, Packet_Parameter1, Packet_Parameter2,
        Packet_Parameter3) == Packet_Checksum)
    {
      // If the checksums are equal then the bytes in the packet
      // are all in the right order. Set the byteCount back to 0
      // and return true.
      ByteCount = 0;
      return true;
    }
    else
    {
      // If the checksums are not equal then the bytes in the
      // packet are out of order. Shift every byte to leftwards
      // (to the previous state)
      Packet_Command = Packet_Parameter1;
      Packet_Parameter1 = Packet_Parameter2;
      Packet_Parameter2 = Packet_Parameter3;
      Packet_Parameter3 = Packet_Checksum;
      return false;
    }
  }

  // Increase the byteCount and return false as not enough bytes
  // have been received into the packet
  ByteCount++;
  return false;
}

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1,
    const uint8_t parameter2, const uint8_t parameter3)
{
  EnterCritical();
  bool success = (UART_OutChar(command) && UART_OutChar(parameter1)
      && UART_OutChar(parameter2) && UART_OutChar(parameter3)
      && UART_OutChar(
          calculateChecksum(command, parameter1, parameter2, parameter3)));
  ExitCritical();
  return success;
}

/*!
 ** @}
 */

