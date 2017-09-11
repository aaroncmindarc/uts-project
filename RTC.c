/*! @file RTC.c
 *
 *  @brief Routines for controlling the Real Time Clock (RTC) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the real time clock (RTC).
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup RTC_Module RTC module documentation
 **  @{
 */
#include "RTC.h"
#include "MK70F12.h"
#include "Cpu.h"

static Callback CallbackFunction; /*<! A global variable to hold the callback function and its arguments pointer */

/*! @brief Initializes the RTC before first use.
 *
 *  Sets up the control register for the RTC and locks it.
 *  Enables the RTC and sets an interrupt every second.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the RTC was successfully initialized.
 */
bool RTC_Init(void (*userFunction)(void*), void* userArguments)
{
  // Assign the arguments of the callback into our specific variable
  CallbackFunction.callbackFunction = userFunction;
  CallbackFunction.callbackArguments = userArguments;

  // Enable clock gate control bit for RTC
  SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;

  // Set a 18pf (16pf + 2pf capacitor) as according to the sheet, not
  // sure what this actually does
  RTC_CR |= (RTC_CR_SC16P_MASK | RTC_CR_SC2P_MASK);

  // Enable the 32KHz oscillator
  RTC_CR |= RTC_CR_OSCE_MASK;

  // Manually delay RTC setup as we wait for the oscillator
  // to power up
  for (int i = 0; i < CPU_XTAL32k_CLK_HZ; i++)
    ;

  // Set TPR so TSR value can be set
  RTC_TPR = RTC_TPR_TPR(0xFFFF);
  //RTC_TSR = RTC_TSR_TSR(0);

  // Enable the Timer Counter
  RTC_SR |= RTC_SR_TCE_MASK;

  // Clear any pending interrupts from RTC interrupt
  NVICICPR2 |= (1 << 3);

  // Interrupt Set Enable RTC in the NVIC
  NVICISER2 |= (1 << 3);

  // Disable the Alaram, Overflow and Invalid interrupts which are
  // set on default
  RTC_IER &= ~RTC_IER_TOIE_MASK;
  RTC_IER &= ~RTC_IER_TAIE_MASK;
  RTC_IER &= ~RTC_IER_TIIE_MASK;

  // Enable RTC interrupts per second (Time Seconds Interrupt Enable)
  RTC_IER |= RTC_IER_TSIE_MASK;

  return true;

}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
  uint32_t totalSeconds = (hours * 3600) + (minutes * 60) + seconds;

  RTC_SR &= ~RTC_SR_TOF_MASK;
  RTC_SR &= ~RTC_SR_TIF_MASK;

  // Disable the time counter so TSR can be written to
  RTC_SR &= ~RTC_SR_TCE_MASK;

  // Set TPR so TSR value can be set
  RTC_TPR = RTC_TPR_TPR(0xFFFF);
  // Set the new TSR value
  RTC_TSR = RTC_TSR_TSR(totalSeconds - 1);

  // Re-enable the time counter so TSR can continue incrementing
  RTC_SR |= RTC_SR_TCE_MASK;

}
/*! @brief Gets the value of the real time clock.
 *
 *  @param hours The address of a variable to store the real time clock hours.
 *  @param minutes The address of a variable to store the real time clock minutes.
 *  @param seconds The address of a variable to store the real time clock seconds.
 *  @note Assumes that the RTC module has been initialized.
 */
void RTC_Get(uint8_t* const hours, uint8_t* const minutes,
    uint8_t* const seconds)
{
  // Read the RTC_TSR and store it into a local variable
  uint32_t totalSeconds = RTC_TSR;

  RTC_SR &= ~RTC_SR_TOF_MASK;
  RTC_SR &= ~RTC_SR_TIF_MASK;

  // Don't need to disable, as we are reading not writing?
  RTC_SR &= ~RTC_SR_TCE_MASK;

  // Work backwards to convert seconds into hours, minutes and seconds
  *hours = totalSeconds / 3600;
  *minutes = (totalSeconds - (*hours * 3600)) / 60;
  *seconds = totalSeconds % 60;

  // Enable time counter, safety first!
  RTC_SR |= RTC_SR_TCE_MASK;
}

/*!
 ** @}
 */

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void __attribute__ ((interrupt))
RTC_ISR(void)
{
  CallbackFunction.callbackFunction(CallbackFunction.callbackArguments);
}
