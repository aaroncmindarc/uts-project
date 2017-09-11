/*! @file FTM.c
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

#include "MK70F12.h"
#include "FTM.h"

//!< FIXED_FREQ_CLK is used to set the CLKS to the fixed clock
#define FIXED_FREQ_CLK 2
#define CHANNELS 8

//!< A static array of the callbacks for each channel
static Callback Callbacks[CHANNELS];

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init()
{
  // Enable clock gate control bit for FTM0
  SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK;

  // Set the FTM mode to initialize
  FTM0_MODE |= FTM_MODE_INIT_MASK;

  // Disable Write Protection so FTM registers can be changed
  FTM0_MODE |= FTM_MODE_WPDIS_MASK;

  // Enable the FTM module
  FTM0_MODE |= FTM_MODE_FTMEN_MASK;

  // Set the MOD value for when counter resets to 0
  FTM0_MOD |= FTM_MOD_MOD(0xFFFF);

  // Set the COUNT initial value to 0 (unused)
  FTM0_CNTIN &= FTM_CNTIN_INIT(0);

  // Writing any value to CNT updates the counter with
  // its initial value, CNTIN
  FTM0_CNT &= FTM_CNT_COUNT(0);

  // Set each channels counter value to 0 and sets each
  // channels CnSC register to 0 (pin not used for FTM)
  for (int i = 0; i < CHANNELS; i++)
  {
    FTM0_CnSC(i) = 0;
    FTM0_CnV(i) = FTM_CnV_VAL(0);
  }

  // Clear any pending interrupts from FTM
  NVICICPR1 |= (1 << 30);

  // Interrupt Set Enable FTM in the NVIC
  NVICISER1 |= (1 << 30);

  return true;
}

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
bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
  uint8_t n = aFTMChannel->channelNb;

  // Set the callback function/argument of the given channel
  Callbacks[n].callbackFunction = aFTMChannel->userFunction;
  Callbacks[n].callbackArguments = aFTMChannel->userArguments;

  if (aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
  {
    // Enable channel as output compare
    FTM0_CnSC(n) |= FTM_CnSC_MSA_MASK
        & (aFTMChannel->timerFunction << FTM_CnSC_MSA_SHIFT);
    FTM0_CnSC(n) |= FTM_CnSC_MSB_MASK
        & (aFTMChannel->timerFunction << FTM_CnSC_MSA_SHIFT);

    // Set the channel mode
    FTM0_CnSC(n) |= FTM_CnSC_ELSA_MASK
        & (aFTMChannel->ioType.outputAction << FTM_CnSC_ELSA_SHIFT);
    FTM0_CnSC(n) |= FTM_CnSC_ELSB_MASK
        & (aFTMChannel->ioType.outputAction << FTM_CnSC_ELSA_SHIFT);

    // Set the channel action (configuration)
    FTM0_CnV(n) = FTM0_CNT + aFTMChannel->delayCount;
  }
}

/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
  uint8_t n = aFTMChannel->channelNb;

  // Set the counter to interrupt after specified delay
  FTM0_CnV(n) = FTM0_CNT + aFTMChannel->delayCount;

  // Enable the channel interrupt
  FTM0_CnSC(n) |= FTM_CnSC_CHIE_MASK;

  // Turn on the FTM and use the FIXED FREQUENCY CLOCK
  FTM0_SC |= FTM_SC_CLKS(FIXED_FREQ_CLK);
}

/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt))
FTM0_ISR(void)
{
  // Find which channel the interrupt originated from
  for (int i = 0; i < 1; i++)
  {
    // If the Channel Flag bit is set, an event has occurred
    if (FTM0_CnSC(i) & FTM_CnSC_CHF_MASK)
    {
      // Clear the interrupt flag
      FTM0_CnSC(i) &= ~FTM_CnSC_CHF_MASK;

      //Stop any future interrupts from occurring on this channel
      //FTM0_CnSC(i) &= ~FTM_CnSC_CHIE_MASK;

      // Disable the counter? (Disable every other interrupt channel?)
      // ^ BAD? Still works because using only 1 channel... better to
      // not do then :^)
      //FTM0_SC &= FTM_SC_CLKS(0);

      // Execute the callback function
      Callback* callback = &Callbacks[i];
      callback->callbackFunction(callback->callbackArguments);
      break;
    }
  }
}

/*!
 ** @}
 */

/*! @brief A callback function that handles specific
 *  callback information
 *
 *  @param arguments - An address to the argument struct else NULL
 *  @return void
 *  @note Assumes the FTM has been initialized.
 */
void FTM_Callback2(void *arguments)
{
  // Convert the arguments pointer into the specified struct pointer
  // so callback information can be read
  FTM_Callback_Args *callbackInfo = (FTM_Callback_Args*) arguments;
  switch (callbackInfo->command)
  {
  case FTM_TIMER:
    // If the FTM module has finished delaying
    LEDs_Off(callbackInfo->LED);
    break;
  default:
    break;
  }
}
