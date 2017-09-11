/*! @file PIT.c
 *
 *  @brief Routines for controlling Periodic Interrupt Timer (PIT) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the periodic interrupt timer (PIT).
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup PIT_Module PIT module documentation
 **  @{
 */

#include "PIT.h"
#include "MK70F12.h"

static Callback CallbackFunction; /*!< A variable to hold the callback function and its arguments pointer */

/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*),
    void* userArguments)
{
  //@{
  //!> Assign the arguments of the callback into our specific variable
  CallbackFunction.callbackFunction = userFunction;
  CallbackFunction.callbackArguments = userArguments;
  //@}

  // Enable clock gate control bit for PIT
  SIM_SCGC6 |= SIM_SCGC6_PIT_MASK;

  // Disable the PIT Module so the PIT can be setup
  PIT_MCR |= PIT_MCR_MDIS_MASK;

  // Enable freeze timer for debug mode (?)
  PIT_MCR |= PIT_MCR_FRZ_MASK;

  // Clear any pending interrupts from PIT seconds interrupt
  NVICICPR2 |= (1 << 4);

  // Interrupt Set Enable PIT in the NVIC
  NVICISER2 |= (1 << 4);

  // Enable PIT Module interrupts mask
  PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK;

  // Enable PIT Module
  PIT_MCR &= ~PIT_MCR_MDIS_MASK;

  //Todo - Set first, then enable? Or enable, then set...?

  //Set PIT timer and start PIT module
  PIT_Set(moduleClk, true);

  return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
  if (restart)
  {
    PIT_Enable(false);

    //Set PIT timer with period, passed down from main as CPU_BUS_CLK_HZ/2 for 0.5s and -1 for LDVAL
    PIT_LDVAL0 = period - 1;
    PIT_Enable(true);
  }
  else
  {
    // The PIT is not restarting, so just set the new value
    // and it will be used after a trigger event
    PIT_LDVAL0 = period - 1;
  }
}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
  if (enable)
  {
    //Enables the PIT Timer to start counting from LDVAL value
    PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK;
  }
  else
  {
    //Disables the PIT Timer to stop counting
    PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK;
  }
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt))
PIT_ISR(void)
{
  //Reset the timer interrupt flag by writing a 1 to clear it
  PIT_TFLG0 |= PIT_TFLG_TIF_MASK;

  CallbackFunction.callbackFunction(CallbackFunction.callbackArguments);
}

/*!
 ** @}
 */
