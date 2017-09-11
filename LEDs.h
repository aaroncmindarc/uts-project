/*! @file LEDs.h
 *
 *  @brief Routines to access the LEDs on the TWR-K70F120M.
 *
 *  This contains the functions for operating the LEDs.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup LED_module LED module documentation
 **  @{
 */

#ifndef LEDS_H
#define LEDS_H

#include "types.h"

/*! @brief LED to pin mapping on the TWR-K70F120M
 *
 */
typedef enum
{
  LED_ORANGE = (1 << 11),
  LED_YELLOW = (1 << 28),
  LED_GREEN = (1 << 29),
  LED_BLUE = (1 << 10)
} TLED;

/*! @brief An enum for the LED callback commands */
enum LEDs_CALLBACK_COMMANDS
{
  LEDs_OFF, LEDs_TOGGLE
};

/*! @brief A struct that acts as a wrapper for callback arguments */
typedef struct
{
  uint8_t command;
  uint32_t LED;
} LEDs_Callback_Args;

/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool
LEDs_Init(void);

/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void
LEDs_On(const TLED color);

/*! @brief Turns off an LED.
 *
 *  @param color THe color of the LED to turn off.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void
LEDs_Off(const TLED color);

/*! @brief Toggles an LED.
 *
 *  @param color THe color of the LED to toggle.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void
LEDs_Toggle(const TLED color);

/*! @brief A callback function that handles specific
 *  callback information
 *
 *  @param arguments - An address to the argument struct else NULL
 *  @return void
 *  @note Assumes the FTM has been initialized.
 */
void
LEDs_Callback(void *arguments);

#endif

/*!
 ** @}
 */
