/*! @file types.h
 *
 *  @brief Declares new types.
 *
 *  This contains types that are especially useful for the Tower to PC Protocol.
 *
 *  @author Aaron Coelho(10858126)
 *  @date 28/04/2017
 */
/*!
 **  @addtogroup types_Module types module documentation
 **  @{
 */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <LEDs.h>

//!< Unions to efficiently access hi and lo parts of integers and words
typedef union
{
  int16_t l;
  struct
  {
    int8_t Lo;
    int8_t Hi;
  } s;
} int16union_t;

//!< Union to efficiently access hi and lo parts of a half-word
typedef union
{
  uint16_t l;
  struct
  {
    uint8_t Lo;
    uint8_t Hi;
  } s;
} uint16union_t;

//!< Union to efficiently access hi and lo parts of a long integer
typedef union
{
  uint32_t l;
  struct
  {
    uint16_t Lo;
    uint16_t Hi;
  } s;
} uint32union_t;

//!< Union to efficiently access hi and lo parts of a 6 byte variable
typedef union
{
  uint8_t Lo;
  uint8_t Mid;
  uint8_t Hi;
} uint48union_t;

//!< Union to efficiently access hi and lo parts of a "phrase" (8 bytes)
typedef union
{
  uint64_t l;
  struct
  {
    uint32_t Lo;
    uint32_t Hi;
  } s;
} uint64union_t;

//!< Union to efficiently access individual bytes of a float
typedef union
{
  float d;
  struct
  {
    uint16union_t dLo;
    uint16union_t dHi;
  } dParts;
} TFloat;

//!< Struct for easily accessing a callback function and its arguments 
typedef struct
{
  void (*callbackFunction)(void*);
  void *callbackArguments;
} Callback;

#endif

/*!
 ** @}
 */
