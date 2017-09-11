/* ###################################################################
 **     Filename    : main.c
 **     Project     : Lab1
 **     Processor   : MK70FN1M0VMJ12
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*! @file main.c
 *
 *  @brief Main module.
 *
 *  This module contains user's application code.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */
// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "types.h"
#include "UART.h"
#include "packet.h"
#include "Flash.h"
#include "LEDs.h"
#include "RTC.h"
#include "FTM.h"
#include "PIT.h"

#include <stdio.h>

// UART baud rate in Hertz (Hz)
#define BAUD_RATE 115200 /*!< The baud rate the system is running at */

#define CR 0x0D /*<! CR is a shortened name for the Carriage Return byte */

//!<Enum for Tower & PC packet commands with their respective values
enum Commands
{
  TOWER_STARTUP = 0x04, /*!< The command byte for when the tower starts up */
  SPECIAL_GET_STARTUP_VALUES = 0x04, /*!< The command byte for when the tower starts up (special) */
  FLASH_PROGRAM_BYTE = 0x07, /*!< The command byte for programming a specific byte in the flash */
  FLASH_READ_BYTE = 0x08, /*!< The command byte for reading a specific byte in the flash */
  SPECIAL = 0x09, /*!< The command byte for special operations */
  PROTOCOL_MODE = 0x0A, /*!< The command byte for ? */
  TOWER_NUMBER = 0x0B, /*!< The command byte for handling get/set of the tower number */
  TIME = 0x0C, /*!< The command byte for getting the current value of the tower RTC time */
  SET_TIME = 0x0C, /*!< The command byte setting the value of the tower RTC */
  TOWER_MODE = 0x0D, /*!< The command byte for handling get/set of the tower mode */
  PRINT_FLASH = 0x55 /*!< The command byte for printing our specified flash area */
};

//@{
//!< Defining the default tower information
#define TOWER_DEFAULT_NUMBER 0x8126
#define TOWER_DEFAULT_MODE 0x0001
//@}

//@{
//!< Defining the flash addresses for tower number and mode
#define FLASH_TOWER_NUM_ADDR FLASH_DATA_START
#define FLASH_TOWER_MODE_ADDR (FLASH_TOWER_NUM_ADDR+2)
//@}

//@{
/*!< Static const wrapper structs for actions that will be executed
 * after certain interrupts, with obnoxiously long variable names */
static const LEDs_Callback_Args LEDS_CALLBACK_PIT_TOGGLE_GREEN_LED = {
    .command = LEDs_TOGGLE, .LED = LED_GREEN };
static const RTC_Callback_Args RTC_CALLBACK_1S_TOGGLE_YELLOW_LED = { .command =
    RTC_SECOND_ELAPSED };
static const FTM_Callback_Args LED_CALLBACK_FTM_BLUE_LED_OFF = { .command =
    LEDs_OFF, .LED = LED_BLUE };
//@}

void FTM_Callback(void *arguments);

/*!< The Channel settings for Channel0 of the FTM which are
 * used when setting the channel */
static const TFTMChannel Channel0 = { .channelNb = 0, .delayCount =
CPU_MCGFF_CLK_HZ_CONFIG_0, // CPU_MCGFF_CLK_HZ_CONFIG_0
    .timerFunction = TIMER_FUNCTION_OUTPUT_COMPARE, .ioType =
        TIMER_OUTPUT_DISCONNECT, .userFunction = FTM_Callback, .userArguments =
        (void*) &LED_CALLBACK_FTM_BLUE_LED_OFF };

//@{
//!< Tower details stored in respective variables
static uint16union_t TowerVersion = { .s = { 0, 1 } };
static volatile uint16union_t *TowerNumber = FLASH_TOWER_NUM_ADDR;
static volatile uint16union_t *TowerMode = FLASH_TOWER_MODE_ADDR;
//@}

//!< Local global variable which holds the command byte of packet
static enum Commands Command;

/*! @brief Helper function to "print" the flash bytes
 *
 *  @return void
 */
static void PrintFlash(void)
{
  Packet_Put(FLASH_READ_BYTE, 'v', 'v', 'v');
  for (uint32_t i = FLASH_DATA_START; i <= FLASH_DATA_END; i++)
  {
    Packet_Put(FLASH_READ_BYTE, 0, i - FLASH_DATA_START, _FB(i));
  }
  Packet_Put(FLASH_READ_BYTE, '^', '^', '^');
}

/*! @brief Put start up packets in transmit buffer.
 *
 *  @return void
 */
static bool HandleTowerStartup(void)
{
  // Check if the parameters match the packet command
  if (Packet_Parameter1 == 0 && Packet_Parameter2 == 0
      && Packet_Parameter3 == 0)
  {

    bool status = true;

    status &= Packet_Put(TOWER_STARTUP, 0, 0, 0)
        && Packet_Put(SPECIAL, 'v', TowerVersion.s.Hi, TowerVersion.s.Lo)
        && Packet_Put(TOWER_NUMBER, 1, TowerNumber->s.Lo, TowerNumber->s.Hi)
        && Packet_Put(TOWER_MODE, 1, TowerMode->s.Lo, TowerMode->s.Hi);
    return status;
  }
  return false;
}

/*! @brief Put tower version packet into transmit buffer.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleTowerGetVersion(void)
{
  return Packet_Put(Command, 'v', TowerVersion.s.Hi, TowerVersion.s.Lo);
}

/*! @brief Get or set the tower number.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleTowerNumber()
{
  // If the PC has sent a get command, send the tower number
  if (Packet_Parameter1 == 1 && Packet_Parameter2 == 0
      && Packet_Parameter3 == 0)
    return Packet_Put(Command, 1, TowerNumber->s.Lo, TowerNumber->s.Hi);

  // If the PC has sent a set command, set the tower number variable (in flash)
  // to the values passed in through the packet parameters
  else if (Packet_Parameter1 == 2)
  {
    bool status = true;

    // If the flash is initialized (cleared) at the address of the tower number
    if (_FH(FLASH_TOWER_NUM_ADDR) == 0xFFFF)
    {
      // Try to allocate flash memory for the tower number variable
      if (Flash_AllocateVar(&TowerNumber, sizeof(*TowerNumber)))
        // Write the default tower number
        status &= Flash_Write16(TowerNumber, TOWER_DEFAULT_NUMBER);
      else
        status = false;
    }
    else
    {
      // Write the tower number sent through the parameters into the flash
      uint16union_t newTowerNumber = { .s.Lo = Packet_Parameter2, .s.Hi =
          Packet_Parameter3 };
      status &= Flash_Write16(TowerNumber, newTowerNumber.l);
    }
    return status;
  }
  return false;
}

/*! @brief Get or set the tower mode.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleTowerMode(void)
{

  // If the PC has sent a get command, send the tower mode
  if (Packet_Parameter1 == 0x01)
    return Packet_Put(Command, 0x01, TowerMode->s.Lo, TowerMode->s.Hi);

  // If the PC has sent a set command, set the tower mode variable (in flash)
  // to the values passed in through the packet parameters
  else if (Packet_Parameter1 == 0x02)
  {
    bool status = true;

    // If the flash is initialized (cleared) at the address of the tower mode
    if (_FH(FLASH_TOWER_MODE_ADDR) == 0xFFFF)
    {
      // Try to allocate flash memory for the tower mode variable
      if (Flash_AllocateVar(&TowerMode, sizeof(*TowerMode)))
        status &= Flash_Write16(TowerMode, TOWER_DEFAULT_MODE);
      else
        status = false;
    }
    else
    {
      // Write the tower number sent through the parameters into the flash
      uint16union_t newTowerMode = { .s.Lo = Packet_Parameter2, .s.Hi =
          Packet_Parameter3 };
      status &= Flash_Write16(TowerMode, newTowerMode.l);
    }
    return status;
  }
  return false;
}

/*! @brief Set a byte in the flash.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleFlashProgramByte(void)
{
  // If the index is within the flash size then simply write the
  // data byte from the packet parameter
  if (Packet_Parameter1 >= 0x00 && Packet_Parameter1 < 0x08)
    return Flash_Write8(FLASH_DATA_START + Packet_Parameter1, Packet_Parameter3);

  // If the index is equal to the flash size then clear/erase the flash
  else if (Packet_Parameter1 == 0x08)
  {
    bool status = Flash_Erase();
    status &= Flash_Init();
    return status;
  }

  return false;
}

/*! @brief Get a byte in the flash.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleFlashReadByte(void)
{
  // If the byte index is within an acceptable range then simply
  // put that data into the output fifo
  if (Packet_Parameter1 >= 0 && Packet_Parameter1 < FLASH_SIZE)
    return Packet_Put(Command, Packet_Parameter1, 0,
        _FB(FLASH_DATA_START+Packet_Parameter1));
  return false;
}

/*! @brief Handles the special commands.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleSpecial(void)
{
  // If the packet has these certain parameters, send the tower version
  if (Packet_Parameter1
      == 'v'&& Packet_Parameter2 == 'x' && Packet_Parameter3 == CR)
    return HandleTowerGetVersion();
  return false;
}

/*! @brief Handles the time commands. More specifically, only sets time.
 *
 *  @return bool - TRUE if the command executed successfully.
 */
static bool HandleTime(void)
{
  if ((Packet_Parameter1 >= 0 && Packet_Parameter1 <= 23)
      && (Packet_Parameter2 >= 0 && Packet_Parameter2 <= 59)
      && (Packet_Parameter3 >= 0 && Packet_Parameter3 <= 59))
  {
    // 0,0,0 is bad input, don't set to 0 in accordance with manual
    if (Packet_Parameter1 == 0 && Packet_Parameter2 == 0
        && Packet_Parameter3 == 0)
    {
      return false;
    }
    RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
    return true;
  }
  else
  {
    return false;
  }
}

/*! @brief Runs the necessary code when a packet has been
 *  received successfully.
 *
 *  @return void
 */
static void PacketSuccess(void)
{
  // Turn on the blue LED, set Channel0 of FTM and start the timer
  LEDs_On(LED_BLUE);
  FTM_Set(&Channel0);
  FTM_StartTimer(&Channel0);
}

/*! @brief Check if a full packet has been received.
 *
 *  If so then choose which action to take next.
 *
 *  @return void
 */
static void HandlePacket(void)
{
  // check if we have a full packet with correct checksum
  if (Packet_Get())
  {
    // Clear the ACK bit from Packet_Command to get the command
    Command = Packet_Command & (~PACKET_ACK_MASK);
    bool ACK = Packet_Command & PACKET_ACK_MASK;
    bool success = true;

    switch (Command)
    {
    case SPECIAL_GET_STARTUP_VALUES:
      success = HandleTowerStartup();
      break;
    case SPECIAL:
      success = HandleSpecial();
      break;
    case TOWER_NUMBER:
      success = HandleTowerNumber();
      break;
    case TOWER_MODE:
      success = HandleTowerMode();
      break;
    case FLASH_PROGRAM_BYTE:
      success = HandleFlashProgramByte();
      break;
    case FLASH_READ_BYTE:
      success = HandleFlashReadByte();
      break;
    case TIME:
      success = HandleTime();
      break;
    case PRINT_FLASH:
      PrintFlash();
      break;
    default:
      break;
    }

    if (success)
    {
      // If handling this packet was successful, call PacketSuccess
      PacketSuccess();
    }
    if (ACK)
    {
      // If the PC asked for acknowledgment, set the ACK depending
      // on whether the command was handled successfully
      Packet_Put(success << 7 | Command, Packet_Parameter1, Packet_Parameter2,
          Packet_Parameter3);
    }
  }
}

/*! @brief Initialize the flash module.
 *
 *  @return bool - TRUE if the flash was initialized successfully.
 */
extern bool Flash_Init(void)
{
  bool status = true;
  // If the flash is initialized at the tower number block, allocate space
  if (_FH(FLASH_TOWER_NUM_ADDR) == 0xFFFF)
  {
    // If the space is successfully allocated then write the tower number
    // into the newly allocated flash space
    if (Flash_AllocateVar(&TowerNumber, sizeof(*TowerNumber)))
    {
      status &= Flash_Write16(TowerNumber, TOWER_DEFAULT_NUMBER);
    }
    else
    {
      status = false;
    }
  }
  // If the flash is initialized at the tower mode block, allocate space
  if (_FH(FLASH_TOWER_MODE_ADDR) == 0xFFFF)
  {
    // If the space is successfully allocated then write the tower mode
    // into the newly allocated flash space
    if (Flash_AllocateVar(&TowerMode, sizeof(*TowerMode)))
    {
      status &= Flash_Write16(TowerMode, TOWER_DEFAULT_MODE);
    }
    else
    {
      status = false;
    }
  }
  return status;
}

void PIT_Callback(void *arguments){
  LEDs_Toggle(LED_GREEN);
}

void FTM_Callback(void *arguments){
  LEDs_Off(LED_BLUE);
}

/*! @brief A callback function that handles specific
 *  callback information
 *
 *  @param arguments - An address to the argument struct else NULL
 *  @return void
 *  @note Assumes the RTC has been initialized.
 */
extern void RTC_Callback(void *arguments)
{
  LEDs_Toggle(LED_YELLOW);
  return;

  /*
  // Convert the arguments pointer into the specified struct pointer
  // so callback information can be read
  RTC_Callback_Args* callbackInfo = (RTC_Callback_Args*) arguments;
  switch (callbackInfo->command)
  {
  // If the RTC 1 second interrupt has been triggered
  case RTC_SECOND_ELAPSED:
    LEDs_Toggle(LED_YELLOW);
    uint8_t hours, minutes, seconds;
    //EnterCritical()
    // TODO - Verify if critical section needed
    // Get the time and send it to the tower
    RTC_Get(&hours, &minutes, &seconds);
    Packet_Put(TIME, hours, minutes, seconds);
    //ExitCritical();
    break;
  default:
    break;
  }
  */
}

/*! @brief Initialize the Tower for first use.
 *
 *  @return bool - TRUE if the tower initialized without any errors.
 */
static bool TowerInit(void)
{
  // Use a variable to keep the status of initializing all modules
  bool init = true;
  init &= Packet_Init(BAUD_RATE, CPU_BUS_CLK_HZ);
  init &= Flash_Init();
  init &= LEDs_Init();

  init &= FTM_Init(FTM_Callback);

  // TODO - Perhaps initialize the RTC counter might stop the weird stuff

  // Initialize the RTC and set up the specific callback information
  init &= RTC_Init(RTC_Callback, (void*) &RTC_CALLBACK_1S_TOGGLE_YELLOW_LED);

  //RTC_Set(0, 0, 1);

  // Initialize the PIT and set up the specific callback information
  init &= PIT_Init(CPU_BUS_CLK_HZ / 2, PIT_Callback,
      (void*) &LEDS_CALLBACK_PIT_TOGGLE_GREEN_LED);

  // If all modules were initialized successfully then turn on the LED
  // and prepare to handle packets
  if (init)
  {
    LEDs_On(LED_ORANGE);
    HandleTowerStartup();
  }
  return init;
}

/*! @brief The main function.
 *
 *  @return int -
 */
/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  // Disable interrupts at startup
  __DI()
  ;

  // Initialize the tower and ensure that initialization was successful
  if (TowerInit())
  {
    // If the tower has initialized successfully, enable interrupts
    __EI()
    ;

    // Loop infinitely
    for (;;)
    {
      //Poll the UART <- DON'T, WE'RE USING INTERRUPTS NOW
      //UART_Poll();

      // Call handlePacket to check whether a full packet has been received
      // If so, the function executes the desired action
      HandlePacket();
    }
  }
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
#ifdef PEX_RTOS_START
  PEX_RTOS_START(); /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
#endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for (;;)
  {
  }
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
 ** @}
 */
/*
 ** ###################################################################
 **
 **     This file was created by Processor Expert 10.5 [05.21]
 **     for the Freescale Kinetis series of microcontrollers.
 **
 ** ###################################################################
 */
