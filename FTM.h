/*! @file FTM.h
 *
 *  @brief Routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the FlexTimer module (FTM).
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup FTM_Module FTM module documentation
 **  @{
 */

#ifndef FTM_H
#define FTM_H

// new types
#include "types.h"
#include "LEDs.h"

//!< An enum for FTM specific callback commands
enum FTM_CALLBACK_COMMANDS
{
  FTM_TIMER
};

//!< A struct that acts as a wrapper for callback arguments
typedef struct
{
  uint8_t command;
  uint32_t LED;
} FTM_Callback_Args;

//!< Enum for Timer functions
typedef enum
{
  TIMER_FUNCTION_INPUT_CAPTURE, TIMER_FUNCTION_OUTPUT_COMPARE
} TTimerFunction;

//!< Enum for Timer output actions
typedef enum
{
  TIMER_OUTPUT_DISCONNECT,
  TIMER_OUTPUT_TOGGLE,
  TIMER_OUTPUT_LOW,
  TIMER_OUTPUT_HIGH
} TTimerOutputAction;

//!< Enum for Timer input detection
typedef enum
{
  TIMER_INPUT_OFF, TIMER_INPUT_RISING, TIMER_INPUT_FALLING, TIMER_INPUT_ANY
} TTimerInputDetection;

//!< Struct for setting up a FTM channel
typedef struct
{
  uint8_t channelNb; //!< The number of the channel to be set (0 - 7)
  uint16_t delayCount; //!< The number of cycles to wait until the interrupt is triggered
  TTimerFunction timerFunction; //!< A variable to store our specified function
  union
  {
    TTimerOutputAction outputAction;
    TTimerInputDetection inputDetection;
  } ioType; //!< The type of trigger
  void (*userFunction)(void*); //!< The callback function to be executed when the FTM interrupt triggers
  void *userArguments; //!< The function arguments which are used by the executing callback function
} TFTMChannel;

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init();

/*! @brief Sets up a timer channel.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *    channelNb is the channel number of the FTM to use.
 *    delayCount is the delay count (in module clock periods) for an output compare event.
 *    timerFunction is used to set the timer up as either an input capture or an output compare.
 *    ioType is a union that depends on the setting of the channel as input capture or output compare:
 *      outputAction is the action to take on a successful output compare.
 *      inputDetection is the type of input capture detection.
 *    userFunction is a pointer to a user callback function.
 *    userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the timer was set up successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_Set(const TFTMChannel* const aFTMChannel);

/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel);

/*! @brief A callback function that handles specific
 *  callback information
 *
 *  @param arguments - An address to the argument struct else NULL
 *  @return void
 *  @note Assumes the FTM has been initialized.
 */
void FTM_Callback(void *arguments);

#endif

/*!
 ** @}
 */

/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt)) FTM0_ISR(void);
