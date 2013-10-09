/*
 * Copyright 2013 MongoDB Inc.
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


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#ifndef BSON_ENDIAN_H
#define BSON_ENDIAN_H


#include "bson-stdint.h"


#ifndef BSON_BYTE_ORDER
#error BSON_BYTE_ORDER is not defined.
#endif


#define BSON_BIG_ENDIAN    4321
#define BSON_LITTLE_ENDIAN 1234


static inline uint16_t
__bson_uint16_swap_slow (uint16_t v)
{
   return ((v & 0xFF) << 8) | ((v & 0xFF00) >> 8);
}


static inline uint32_t
__bson_uint32_swap_slow (uint32_t v)
{
   return (((v & 0xFF000000) >> 24) |
           ((v & 0x00FF0000) >> 8)  |
           ((v & 0x0000FF00) << 8)  |
           ((v & 0x000000FF) << 24));
}


static inline uint64_t
__bson_uint64_swap_slow (uint64_t v)
{
   return (((v & 0xFF00000000000000UL) >> 56) |
           ((v & 0x00FF000000000000UL) >> 40) |
           ((v & 0x0000FF0000000000UL) >> 24) |
           ((v & 0x000000FF00000000UL) >> 8)  |
           ((v & 0x00000000FF000000UL) << 8)  |
           ((v & 0x0000000000FF0000UL) << 24) |
           ((v & 0x000000000000FF00UL) << 40) |
           ((v & 0x00000000000000FFUL) << 56));
}


/*
 * TODO: Someone please take the time to add optimized swap functions
 *       for other architectures if you would like them. I imagine
 *       we need some intrinsics for clang at least.
 */


#if defined (__GNUC__) && (__GNUC__ >= 4)
#  if __GNUC__ >= 4 && defined (__GNUC_MINOR__) && __GNUC_MINOR__ >= 3
#    define BSON_UINT32_SWAP_LE_BE(v) __builtin_bswap32((uint32_t)v)
#    define BSON_UINT64_SWAP_LE_BE(v) __builtin_bswap64((uint64_t)v)
#  endif
#  if __GNUC__ >= 4 && defined (__GNUC_MINOR__) && __GNUC_MINOR__ >= 8
#    define BSON_UINT16_SWAP_LE_BE(v) __builtin_bswap16((uint32_t)v)
#  endif
#endif


#ifndef BSON_UINT16_SWAP_LE_BE
# define BSON_UINT16_SWAP_LE_BE(v) __bson_uint16_swap_slow(v)
#endif


#ifndef BSON_UINT32_SWAP_LE_BE
# define BSON_UINT32_SWAP_LE_BE(v) __bson_uint32_swap_slow(v)
#endif


#ifndef BSON_UINT64_SWAP_LE_BE
# define BSON_UINT64_SWAP_LE_BE(v) __bson_uint64_swap_slow(v)
#endif


#if BSON_BYTE_ORDER == BSON_LITTLE_ENDIAN
#    define BSON_UINT16_FROM_LE(v)  ((uint16_t) v)
#    define BSON_UINT16_TO_LE(v)    ((uint16_t) v)
#    define BSON_UINT16_FROM_BE(v)  BSON_UINT16_SWAP_LE_BE(v)
#    define BSON_UINT16_TO_BE(v)    BSON_UINT16_SWAP_LE_BE(v)
#    define BSON_UINT32_FROM_LE(v)  ((uint32_t) v)
#    define BSON_UINT32_TO_LE(v)    ((uint32_t) v)
#    define BSON_UINT32_FROM_BE(v)  BSON_UINT32_SWAP_LE_BE(v)
#    define BSON_UINT32_TO_BE(v)    BSON_UINT32_SWAP_LE_BE(v)
#    define BSON_UINT64_FROM_LE(v)  ((uint64_t) v)
#    define BSON_UINT64_TO_LE(v)    ((uint64_t) v)
#    define BSON_UINT64_FROM_BE(v)  BSON_UINT64_SWAP_LE_BE(v)
#    define BSON_UINT64_TO_BE(v)    BSON_UINT64_SWAP_LE_BE(v)
#elif BSON_BYTE_ORDER == BSON_BIG_ENDIAN
#    define BSON_UINT16_FROM_LE(v)  BSON_UINT16_SWAP_LE_BE(v)
#    define BSON_UINT16_TO_LE(v)    BSON_UINT16_SWAP_LE_BE(v)
#    define BSON_UINT16_FROM_BE(v)  ((uint16_t) v)
#    define BSON_UINT16_TO_BE(v)    ((uint16_t) v)
#    define BSON_UINT32_FROM_LE(v)  BSON_UINT32_SWAP_LE_BE(v)
#    define BSON_UINT32_TO_LE(v)    BSON_UINT32_SWAP_LE_BE(v)
#    define BSON_UINT32_FROM_BE(v)  ((uint32_t) v)
#    define BSON_UINT32_TO_BE(v)    ((uint32_t) v)
#    define BSON_UINT64_FROM_LE(v)  BSON_UINT64_SWAP_LE_BE(v)
#    define BSON_UINT64_TO_LE(v)    BSON_UINT64_SWAP_LE_BE(v)
#    define BSON_UINT64_FROM_BE(v)  ((uint64_t) v)
#    define BSON_UINT64_TO_BE(v)    ((uint64_t) v)
#else
#error Unknown platform detected
#endif


#endif /* BSON_ENDIAN_H */
