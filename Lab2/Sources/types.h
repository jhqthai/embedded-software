/*! @file
 *
 *  @brief Declares new types.
 *
 *  This contains types that are especially useful for the Tower to PC Protocol.
 *
 *  @author John Thai & Jason Gavriel
 *  @date 2017-04-05
 */
/*!
 * @addtogroup Types_module Packet module documentation
 * @{
 */

#ifndef TYPES_H
#define TYPES_H
/* MODULE Type */

#include <stdint.h>
#include <stdbool.h>

// Unions to efficiently access hi and lo parts of integers and words
typedef union
{
  int16_t l;
  struct
  {
    int8_t Lo;
    int8_t Hi;
  } s;
} int16union_t;

typedef union
{
  uint16_t l;
  struct
  {
    uint8_t Lo;
    uint8_t Hi;
  } s;
} uint16union_t;

// Union to efficiently access hi and lo parts of a long integer
typedef union
{
  uint32_t l;
  struct
  {
    uint16_t Lo;
    uint16_t Hi;
  } s;
} uint32union_t;

// Union to efficiently access bytes of a long integer
typedef union
{
  uint32_t l;
  struct
  {
    uint8_t d;
    uint8_t c;
    uint8_t b;
		uint8_t a;
  } s;
} uint32_8union_t;

// Union to efficiently access hi and lo parts of a "phrase" (8 bytes)
typedef union
{
  uint64_t l;
  struct
  {
    uint32_t Lo;
    uint32_t Hi;
  } s;
} uint64union_t;

// Union to efficiently access individual bytes of a float
typedef union
{
  float d;
  struct
  {
    uint16union_t dLo;
    uint16union_t dHi;
  } dParts;
} TFloat;

#endif

/*!
** @}
*/
