/*! @file Flash.c
 *
 *  @brief Routines for erasing and writing to the Flash.
 *
 *  This contains the functions needed for accessing the internal Flash.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup FLASH_module FLASH module documentation
 **  @{
 */
/*
 * FCCOB Endianness and Multi-Byte Access:
 The FCCOB register group uses a big endian addressing convention. For all command parameter fields
 larger than 1 byte, the most significant data resides in the lowest FCCOB register number. The FCCOB
 register group may be read and written as individual bytes, aligned words (2 bytes) or aligned longwords
 (4 bytes).
 */

#include "MK70F12.h"
#include "Flash.h"

// Define the command bytes for FTFE
#define PROGRAM_PHRASE_COMMAND 0x07 /*!< The flash command value for programming the flash data */
#define ERASE_FLASH_SECTOR_COMMAND 0x09 /*!< The flash command value for erasing a flash sector */

const static uint8_t FlashBaseAddress[3] = { 0x08, 0x00, 0x00 }; /*!< The base address of the flash section we are working in */
// TODO - Make this variable name ALL CAPITALSED

//!< Struct to store current command and data of Flash
typedef struct tfccob
{
  uint8_t command; /*<! The command that is to be executed */
  uint8_t data[FLASH_SIZE]; /*!< An array to store the current state of the flash before executing the command */
} TFCCOB;

static TFCCOB CommonCommandObject; /*!< Global static variable that will be used for executing commands */

/*! @brief Set the command of the TFCCOB for later execution.
 *
 *  @param commandCommonObject A pointer to the object that stores the state of the flash.
 *  @param command A byte which represents the current flash command.
 *  @return void
 *  @note Assumes Flash has been initialized.
 */
static void SetupCommand(TFCCOB* commandCommonObject, uint8_t command)
{
  commandCommonObject->command = command;
}

/*! @brief Set the data phrase bytes into the respective FTFE registers.
 *
 *  @return void
 *  @note Assumes Flash has been initialized.
 */
static void ProgramPhrase(void)
{
  FTFE_FCCOB4 = CommonCommandObject.data[3];
  FTFE_FCCOB5 = CommonCommandObject.data[2];
  FTFE_FCCOB6 = CommonCommandObject.data[1];
  FTFE_FCCOB7 = CommonCommandObject.data[0];
  FTFE_FCCOB8 = CommonCommandObject.data[7];
  FTFE_FCCOB9 = CommonCommandObject.data[6];
  FTFE_FCCOBA = CommonCommandObject.data[5];
  FTFE_FCCOBB = CommonCommandObject.data[4];
}

/*! @brief Execute a command.
 *
 *  @param CommonCommandObject A pointer to the object that stores the state of the flash.
 *
 *  @return bool - TRUE if the flash was initialized successfully.
 *  @note Assumes Flash has been initialized.
 */
static bool LaunchCommand(TFCCOB* CommonCommandObject)
{
  // Ensure no previous flash commands are executing
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK))
    ;

  // Clear the status registers error bits
  //FTFE_FSTAT = FTFE_FSTAT_ACCERR_MASK | FTFE_FSTAT_FPVIOL_MASK
  //    | FTFE_FSTAT_RDCOLERR_MASK;

  // Set the command
  FTFE_FCCOB0 = CommonCommandObject->command;

  // The high byte of the flash  base address
  FTFE_FCCOB1 = 0x08;

  // The mid byte of the flash  base address
  FTFE_FCCOB2 = 0x00;

  // The low byte of the flash base address
  FTFE_FCCOB3 = 0x00;

  // Execute the a command based on what command has been set inside the
  // common command object
  switch (CommonCommandObject->command)
  {
  case PROGRAM_PHRASE_COMMAND:
    ProgramPhrase();
    break;
  case ERASE_FLASH_SECTOR_COMMAND:
    break;
  default:
    return false;
  }

  // Set the CCIF flag and execute the command
  FTFE_FSTAT = FTFE_FSTAT_CCIF_MASK;

  // Wait until the command has fully executed
  while (!(FTFE_FSTAT & FTFE_FSTAT_CCIF_MASK))
    ;

  return true;
}

/*! @brief Store the flash state into the common command object.
 *
 *  @return void
 *  @note Assumes Flash has been initialized.
 */
static bool FlashBackup(void)
{
  // Loop through all bytes and store them in the data array of the common command object
  for (int i = 0; i < FLASH_SIZE; i++)
  {
    CommonCommandObject.data[i] = _FB(FLASH_DATA_START+i);
  }
  return true;
}

/*! @brief Allocates space for a non-volatile variable in the Flash memory.
 *
 *  @param variable is the address of a pointer to a variable that is to be allocated space in Flash memory.
 *         The pointer will be allocated to a relevant address:
 *         If the variable is a byte, then any address.
 *         If the variable is a half-word, then an even address.
 *         If the variable is a word, then an address divisible by 4.
 *         This allows the resulting variable to be used with the relevant Flash_Write function which assumes a certain memory address.
 *         e.g. a 16-bit variable will be on an even address
 *  @param size The size, in bytes, of the variable that is to be allocated space in the Flash memory. Valid values are 1, 2 and 4.
 *  @return bool - TRUE if the variable was allocated space in the Flash memory.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_AllocateVar(volatile void** variable, const uint8_t size)
{
  bool addressFound = true;
  uint8_t addressIndex = -1;

  // Ensure that the index is in the correct position
  if (size == 1 || size == 2 || size == 4)
  {

    // Ensure that there are enough consecutive free bytes for the variable size
    for (int i = 0; i < FLASH_SIZE; i += size)
    {

      // If the index i is free
      if (_FB(FLASH_DATA_START+i) == 0xFF)
      {

        // Check that the next (size-1) bytes are also free
        for (int j = 1; j < size; j++)
        {

          //If there are not enough free consecutive free bytes or we have run
          // out of flash space, break out of the loop
          if ((i + j) >= (FLASH_SIZE) || _FB(FLASH_DATA_START+(i+j)) != 0xFF)
          {
            addressFound = false;
            break;
          }
        }

        // If a suitable address has been found, store the initial index
        if (addressFound)
        {
          addressIndex = i;
          break;
        }
      }
    }

    // Put the address value into the pointer variable
    if (addressFound)
    {
      *variable = (void*) (FLASH_DATA_START + addressIndex);
      return true;
    }
  }
  return false;
}

/*! @brief Writes a 64-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 64-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 8-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write64(volatile uint32_t* const address, const uint64_t data)
{
  // Calculate the index of the target address from the starting address
  uint8_t index = (uint32_t) address - FLASH_DATA_START;

  // Check to see if the index is within the flash bounds
  if (index < 0 || index >= FLASH_SIZE || index % 8 != 0)
  {
    return false;
  }

  // Set up a new command object to program a phrase
  SetupCommand(&CommonCommandObject, PROGRAM_PHRASE_COMMAND);

  // Back up the state of the flash so it can be restored after clearing
  bool status = FlashBackup();

  // Set the data into the common command object at the target index
  uint64union_t data64Union = { .l = data };
  uint32union_t data32UnionHi = { .l = data64Union.s.Hi };
  uint32union_t data32UnionLo = { .l = data64Union.s.Lo };
  uint16union_t data16UnionHiHi = { .l = data32UnionHi.s.Hi };
  uint16union_t data16UnionHiLo = { .l = data32UnionHi.s.Lo };
  uint16union_t data16UnionLoHi = { .l = data32UnionLo.s.Hi };
  uint16union_t data16UnionLoLo = { .l = data32UnionLo.s.Lo };
  CommonCommandObject.data[index] = data16UnionLoLo.s.Lo;
  CommonCommandObject.data[index + 1] = data16UnionLoLo.s.Hi;
  CommonCommandObject.data[index + 2] = data16UnionLoHi.s.Lo;
  CommonCommandObject.data[index + 3] = data16UnionLoHi.s.Hi;
  CommonCommandObject.data[index + 4] = data16UnionHiLo.s.Lo;
  CommonCommandObject.data[index + 5] = data16UnionHiLo.s.Hi;
  CommonCommandObject.data[index + 6] = data16UnionHiHi.s.Lo;
  CommonCommandObject.data[index + 7] = data16UnionHiHi.s.Hi;

  // Erase the flash sector
  status &= Flash_Erase();

  // Execute the program phase command and put the data in
  // the common command object back into the flash
  status &= LaunchCommand(&CommonCommandObject);

  return status;
}

/*! @brief Writes a 32-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 32-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 4-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write32(volatile uint32_t* const address, const uint32_t data)
{
  // Calculate the index of the target address from the starting address
  uint8_t index = (uint32_t) address - FLASH_DATA_START;

  // Check to see if the index is within the flash bounds
  if (index < 0 || index >= FLASH_SIZE || index % 4 != 0)
  {
    return false;
  }

  // Set up a new command object to program a phrase
  SetupCommand(&CommonCommandObject, PROGRAM_PHRASE_COMMAND);

  // Back up the state of the flash so it can be restored after clearing
  bool status = FlashBackup();

  // Set the data into the common command object at the target index
  uint32union_t dataUnion = { .l = data };
  uint16union_t dataUnionHi = { .l = dataUnion.s.Hi }, dataUnionLo = { .l =
      dataUnion.s.Lo };
  CommonCommandObject.data[index] = dataUnionLo.s.Lo;
  CommonCommandObject.data[index + 1] = dataUnionLo.s.Hi;
  CommonCommandObject.data[index + 2] = dataUnionHi.s.Lo;
  CommonCommandObject.data[index + 3] = dataUnionHi.s.Hi;

  // Erase the flash sector
  status &= Flash_Erase();

  // Execute the program phase command and put the data in
  // the common command object back into the flash
  status &= LaunchCommand(&CommonCommandObject);

  return status;
}

/*! @brief Writes a 16-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 16-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if address is not aligned to a 2-byte boundary or if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write16(volatile uint16_t* const address, const uint16_t data)
{
  // Calculate the index of the target address from the starting address
  uint8_t index = (uint32_t) address - FLASH_DATA_START;

  // Check to see if the index is within the flash bounds
  if (index < 0 || index >= FLASH_SIZE || index % 2 != 0)
  {
    return false;
  }

  // Set up a new command object to program a phrase
  SetupCommand(&CommonCommandObject, PROGRAM_PHRASE_COMMAND);

  // Back up the state of the flash so it can be restored after clearing
  bool status = FlashBackup();

  // Set the data into the common command object at the target index
  uint16union_t dataUnion = { .l = data };
  CommonCommandObject.data[index] = dataUnion.s.Lo;
  CommonCommandObject.data[index + 1] = dataUnion.s.Hi;

  // Erase the flash sector
  status &= Flash_Erase();

  // Execute the program phase command and put the data in
  // the common command object back into the flash
  status &= LaunchCommand(&CommonCommandObject);

  return status;
}

/*! @brief Writes an 8-bit number to Flash.
 *
 *  @param address The address of the data.
 *  @param data The 8-bit data to write.
 *  @return bool - TRUE if Flash was written successfully, FALSE if there is a programming error.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Write8(volatile uint8_t* const address, const uint8_t data)
{
  // Calculate the index of the target address from the starting address
  uint8_t index = (uint32_t) address - FLASH_DATA_START;

  // Check to see if the index is within the flash bounds
  if (index < 0 || index >= FLASH_SIZE)
  {
    return false;
  }

  // Set up a new command object to program a phrase
  SetupCommand(&CommonCommandObject, PROGRAM_PHRASE_COMMAND);

  // Back up the state of the flash so it can be restored after clearing
  bool status = FlashBackup();

  // Set the data into the common command object at the target index
  CommonCommandObject.data[index] = data;

  // Erase the flash sector
  status &= Flash_Erase();

  // Execute the program phase command and put the data in
  // the common command object back into the flash
  status &= LaunchCommand(&CommonCommandObject);
  return status;
}

/*! @brief Erases the entire Flash sector.
 *
 *  @return bool - TRUE if the Flash "data" sector was erased successfully.
 *  @note Assumes Flash has been initialized.
 */
bool Flash_Erase(void)
{
  // Make a new common command object with command set to erase
  // and launch it
  TFCCOB commonCommandObject;
  SetupCommand(&commonCommandObject, ERASE_FLASH_SECTOR_COMMAND);
  return LaunchCommand(&commonCommandObject);
}

/*!
 ** @}
 */
