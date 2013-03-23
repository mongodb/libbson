/*
 * Copyright 2013 10gen Inc.
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


#ifndef BSON_TYPES_H
#define BSON_TYPES_H


#include <stdlib.h>

#include "bson-config.h"
#include "bson-macros.h"


BSON_BEGIN_DECLS


/*
 * Standard type definitions.
 * TODO: Determine long/long long/etc from build system.
 */
#ifdef HAVE_STDINT
#include <stdint.h>
typedef int            bson_bool_t;
typedef int8_t         bson_int8_t;
typedef int16_t        bson_int16_t;
typedef int32_t        bson_int32_t;
typedef int64_t        bson_int64_t;
typedef uint8_t        bson_uint8_t;
typedef uint16_t       bson_uint16_t;
typedef uint32_t       bson_uint32_t;
typedef uint64_t       bson_uint64_t;
#else
/*
 * TODO: long long vs long for 64-bit
 */
typedef int            bson_bool_t;
typedef signed char    bson_int8_t;
typedef signed short   bson_int16_t;
typedef signed int     bson_int32_t;
typedef signed long    bson_int64_t;
typedef unsigned char  bson_uint8_t;
typedef unsigned short bson_uint16_t;
typedef unsigned int   bson_uint32_t;
typedef unsigned long  bson_uint64_t;
#endif /* HAVE_STDINT */


BSON_END_DECLS


#endif /* BSON_TYPES_H */
