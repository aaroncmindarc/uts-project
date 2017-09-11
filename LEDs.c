/*! @file LEDs.c
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

#include "LEDs.h"
#include "MK70F12.h"

/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void)
{
  // Enable clock gate control bit for PORTA
  SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK;

  // Assign PCR10 pin to ALT1 functionality
  PORTA_PCR10 |= PORT_PCR_MUX(1);

  // Assign PCR11 pin to ALT1 functionality
  PORTA_PCR11 |= PORT_PCR_MUX(1);

  // Assign PCR28 pin to ALT1 functionality
  PORTA_PCR28 |= PORT_PCR_MUX(1);

  // Assign PCR29 pin to ALT1 functionality
  PORTA_PCR29 |= PORT_PCR_MUX(1);

  // Set the LED pins in the data register to output mode
  GPIOA_PDDR |= (LED_ORANGE | LED_YELLOW | LED_GREEN | LED_BLUE);

  // Set the LED pins the data out register to active high logic so they
  // remain turned off until the bit is manually cleared
  GPIOA_PDOR |= (LED_ORANGE | LED_YELLOW | LED_GREEN | LED_BLUE);

  return true;
}

/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_On(const TLED color)
{
  // Clear the bit at the specified pin, the LED now becomes forward
  // biased (Port Clear Output Register)
  GPIOA_PCOR |= color;
}

/*! @brief Turns off an LED.
 *
 *  @param color The color of the LED to turn off.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Off(const TLED color)
{
  // Set the bit at the specified pin, the LED now becomes an open
  // circuit (Port Set Output Register)
  GPIOA_PSOR |= color;
}

/*! @brief Toggles an LED.
 *
 *  @param color The color of the LED to toggle.
 *  @return void
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Toggle(const TLED color)
{
  // Toggle the bit at the specified pin, the LED now becomes an open
  // circuit (Port Toggle Output Register)
  GPIOA_PTOR |= color;
}

/*! @brief A callback function that handles specific
 *  callback information
 *
 *  @param arguments - An address to the argument struct else NULL
 *  @return void
 *  @note Assumes the FTM has been initialized.
 */
void LEDs_Callback(void *arguments)
{
  // Convert the arguments pointer into the specified struct pointer
  // so callback information can be read
  LEDs_Callback_Args *callbackInfo = (LEDs_Callback_Args*) arguments;
  switch (callbackInfo->command)
  {
  case LEDs_OFF:
    LEDs_Off(callbackInfo->LED);
    break;
  case LEDs_TOGGLE:
    // If the command is to toggle an LED, toggle the LED
    LEDs_Toggle(callbackInfo->LED);
    break;
  default:
    break;
  }
}

/*!
 ** @}
 */
