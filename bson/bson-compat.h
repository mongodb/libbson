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
#  error "Only <bson.h> can be included directly."
#endif

#include "bson-config.h"
#include "bson-macros.h"

#ifndef BSON_COMPAT_H
#define BSON_COMPAT_H

#if BSON_OS == 1
#  define BSON_OS_UNIX
#elif BSON_OS == 2
#  define BSON_OS_WIN32
#else
#  error Unknown operating system.
#endif

#ifdef BSON_OS_WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    undef  WIN32_LEAN_AND_MEAN
#  else
#    include <windows.h>
#  endif
#include <winsock.h>
#include <io.h>
#include <share.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <bson-stdint.h>
#endif

#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BSON_OS_WIN32
typedef UINT32 bson_bool_t;
typedef INT8 bson_int8_t;
typedef INT16 bson_int16_t;
typedef INT32 bson_int32_t;
typedef INT64 bson_int64_t;
typedef UINT8 bson_uint8_t;
typedef UINT16 bson_uint16_t;
typedef UINT32 bson_uint32_t;
typedef UINT64 bson_uint64_t;
typedef UINT32 bson_unichar_t;
typedef SIZE_T bson_size_t;
typedef SSIZE_T bson_off_t;
typedef SSIZE_T bson_ssize_t;
#else
typedef uint32_t bson_bool_t;
typedef int8_t bson_int8_t;
typedef int16_t bson_int16_t;
typedef int32_t bson_int32_t;
typedef int64_t bson_int64_t;
typedef uint8_t bson_uint8_t;
typedef uint16_t bson_uint16_t;
typedef uint32_t bson_uint32_t;
typedef uint64_t bson_uint64_t;
typedef uint32_t bson_unichar_t;
typedef size_t bson_size_t;
typedef off_t bson_off_t;
typedef ssize_t  bson_ssize_t;
#endif

#ifdef BSON_OS_WIN32

static BSON_INLINE int
bson_vsnprintf (char * str, bson_size_t size, const char * format, va_list ap)
{
    int r = -1;

    if (size != 0) {
        r = _vsnprintf_s(str, size, _TRUNCATE, format, ap);
    }

    if (r == -1) {
        r = _vscprintf(format, ap);
    }

    return r;
}

static BSON_INLINE int
bson_snprintf (char * str, bson_size_t size, const char * format, ...)
{
    int r;
    va_list ap;

    va_start(ap, format);
    r = bson_vsnprintf(str, size, format, ap);
    va_end(ap);

    return r;
}
#define bson_strcasecmp _stricmp
#else
#define bson_vsnprintf vsnprintf
#define bson_snprintf snprintf
#define bson_strcasecmp strcasecmp
#endif

#ifdef BSON_OS_WIN32

static BSON_INLINE int bson_open(const char * filename, int flags)
{
    int fd;
    errno_t err;

    err = _sopen_s(&fd, filename, flags | _O_BINARY, _SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (err) {
        errno = err;
        return -1;
    }

    return fd;
}

static BSON_INLINE bson_ssize_t bson_read(int fd, void * buf, bson_size_t count)
{
    return (bson_ssize_t)_read(fd, buf, (int)count);
}

#define bson_lseek _lseek
#define bson_close _close
#define BSON_O_RDONLY _O_RDONLY
#else
#define bson_open open
#define bson_read read
#define bson_close close
#define bson_lseek lseek
#define BSON_O_RDONLY O_RDONLY
#endif

#endif /* BSON_COMPAT_H */
