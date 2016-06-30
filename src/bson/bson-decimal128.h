/*
 * Copyright 2015 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef BSON_DECIMAL128_H
#define BSON_DECIMAL128_H


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
# error "Only <bson.h> can be included directly."
#endif

#include <string.h>

#include "bson-macros.h"
#include "bson-config.h"
#include "bson-types.h"


/**
 * BSON_DEC_128_STRING:
 *
 * The length of a decimal128 string (with null terminator).
 *
 * 1  for the sign
 * 35 for digits and radix
 * 2  for exponent indicator and sign
 * 4  for exponent digits
 */
#define BSON_DECIMAL128_STRING 43


BSON_BEGIN_DECLS

#ifdef BSON_EXPERIMENTAL_FEATURES
void
bson_decimal128_to_string (const bson_decimal128_t *dec,
                           char                    *str);


/* Note: @string must be ASCII characters only! */
bool
bson_decimal128_from_string (const char        *string,
                             bson_decimal128_t *dec);


#ifdef BSON_HAVE_DECIMAL128
static _Decimal128 BSON_INLINE
bson_decimal128_to_Decimal128 (bson_decimal128_t *dec)
{
   _Decimal128 decimal128;
   memcpy (&decimal128, dec, sizeof(decimal128));
   return decimal128;
}


static void BSON_INLINE
bson_Decimal128_to_decimal128 (_Decimal128        decimal128,
                               bson_decimal128_t *dec)
{
   memcpy (dec, &decimal128, sizeof(decimal128));
}
#endif /* BSON_HAVE_DECIMAL128 */
#endif /* BSON_EXPERIMENTAL_FEATURES */

BSON_END_DECLS


#endif /* BSON_DECIMAL128_H */
