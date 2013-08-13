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


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#ifndef BSON_ENDIAN_H
#define BSON_ENDIAN_H


#ifndef BSON_BYTE_ORDER
#error BSON_BYTE_ORDER is not defined.
#endif


#define BSON_BIG_ENDIAN    4321
#define BSON_LITTLE_ENDIAN 1234


#if defined(__linux__)
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#include <endian.h>
#undef _BSD_SOURCE
#else
#include <endian.h>
#endif /* _BSD_SOURCE */
#endif /* __linux__ */


#if defined(__FreeBSD__)
#include <sys/endian.h>
#endif /* __FreeBSD__ */



#if defined(__linux__) || defined(__FreeBSD__)


#define BSON_UINT16_FROM_LE(v) ((bson_uint16_t) le16toh(v))
#define BSON_UINT16_TO_LE(v)   ((bson_uint16_t) le16toh(v))
#define BSON_UINT16_FROM_BE(v) ((bson_uint16_t) be16toh(v))
#define BSON_UINT16_TO_BE(v)   ((bson_uint16_t) be16toh(v))
#define BSON_UINT32_FROM_LE(v) ((bson_uint32_t) le32toh(v))
#define BSON_UINT32_TO_LE(v)   ((bson_uint32_t) htole32(v))
#define BSON_UINT32_FROM_BE(v) ((bson_uint32_t) be32toh(v))
#define BSON_UINT32_TO_BE(v)   ((bson_uint32_t) htobe32(v))
#define BSON_UINT64_FROM_LE(v) ((bson_uint64_t) le64toh(v))
#define BSON_UINT64_TO_LE(v)   ((bson_uint64_t) htole64(v))
#define BSON_UINT64_FROM_BE(v) ((bson_uint64_t) be64toh(v))
#define BSON_UINT64_TO_BE(v)   ((bson_uint64_t) htobe64(v))


#elif defined(__APPLE__)


#include <libkern/OSByteOrder.h>
#define BSON_UINT16_FROM_LE(v) ((bson_uint16_t) OSSwapLittleToHostInt16((v)))
#define BSON_UINT16_TO_LE(v)   ((bson_uint16_t) OSSwapHostToLittleInt16((v)))
#define BSON_UINT16_FROM_BE(v) ((bson_uint16_t) OSSwapBigToHostInt16((v)))
#define BSON_UINT16_TO_BE(v)   ((bson_uint16_t) OSSwapHostToBigInt16((v)))
#define BSON_UINT32_FROM_LE(v) ((bson_uint32_t) OSSwapLittleToHostInt32((v)))
#define BSON_UINT32_TO_LE(v)   ((bson_uint32_t) OSSwapHostToLittleInt32((v)))
#define BSON_UINT32_FROM_BE(v) ((bson_uint32_t) OSSwapBigToHostInt32((v)))
#define BSON_UINT32_TO_BE(v)   ((bson_uint32_t) OSSwapHostToBigInt32((v)))
#define BSON_UINT64_FROM_LE(v) ((bson_uint64_t) OSSwapLittleToHostInt64((v)))
#define BSON_UINT64_TO_LE(v)   ((bson_uint64_t) OSSwapHostToLittleInt64((v)))
#define BSON_UINT64_FROM_BE(v) ((bson_uint64_t) OSSwapBigToHostInt64((v)))
#define BSON_UINT64_TO_BE(v)   ((bson_uint64_t) OSSwapHostToBigInt64((v)))


#elif defined(_WIN32) || defined(_WIN64)


#define BSON_UINT16_FROM_LE(v) ((bson_uint16_t) (v))
#define BSON_UINT16_TO_LE(v)   ((bson_uint16_t) (v))
#define BSON_UINT16_FROM_BE(v) ((bson_uint16_t) _byteswap_ushort((v)))
#define BSON_UINT16_TO_BE(v)   ((bson_uint16_t) _byteswap_ushort((v)))
#define BSON_UINT32_FROM_LE(v) ((bson_uint32_t) (v))
#define BSON_UINT32_TO_LE(v)   ((bson_uint32_t) (v))
#define BSON_UINT32_FROM_BE(v) ((bson_uint32_t) _byteswap_ulong((v)))
#define BSON_UINT32_TO_BE(v)   ((bson_uint32_t) _byteswap_ulong((v)))
#define BSON_UINT64_FROM_LE(v) ((bson_uint64_t) (v))
#define BSON_UINT64_TO_LE(v)   ((bson_uint64_t) (v))
#define BSON_UINT64_FROM_BE(v) ((bson_uint64_t) _byteswap_uint64((v)))
#define BSON_UINT64_TO_BE(v)   ((bson_uint64_t) _byteswap_uint64((v)))


#else


#error your platform is not yet supported.


#endif


#endif /* BSON_ENDIAN_H */
