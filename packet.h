/*! @file packet.h
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

#ifndef PACKET_H
#define PACKET_H

// new types
#include "types.h"

//!< extern global variable mask for the command packets ACK bit
extern const uint8_t PACKET_ACK_MASK;

//! A struct for storing the latest recieved packet
extern uint8_t Packet_Command, /*!< The packet's command */
Packet_Parameter1, /*!< The packet's 1st parameter */
Packet_Parameter2, /*!< The packet's 2nd parameter */
Packet_Parameter3, /*!< The packet's 3rd parameter */
Packet_Checksum; /*!< The packet's checksum */

/*! @brief Check if a full packet has been received.
 *
 *  If so then choose which action to take next.
 *
 *  @return void
 */
extern void Packet_Handle(void);

/*! @brief Initializes the packets by calling the initialization routines of the supporting software modules.
 *
 *  @param baudRate The desired baud rate in bits/sec.
 *  @param moduleClk The module clock rate in Hz
 *  @return bool - TRUE if the packet module was successfully initialized.
 */
bool Packet_Init(const uint32_t baudRate, const uint32_t moduleClk);

/*! @brief Attempts to get a packet from the received data.
 *
 *  @return bool - TRUE if a valid packet was received.
 */
bool Packet_Get(void);

/*! @brief Builds a packet and places it in the transmit FIFO buffer.
 *
 *  @return bool - TRUE if a valid packet was sent.
 */
bool Packet_Put(const uint8_t command, const uint8_t parameter1,
    const uint8_t parameter2, const uint8_t parameter3);

#endif

/*!
 ** @}
 */