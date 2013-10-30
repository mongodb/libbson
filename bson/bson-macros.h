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


#ifndef BSON_MACROS_H
#define BSON_MACROS_H


#include <assert.h>
#include <stdio.h>

#include "bson-config.h"


#ifdef __cplusplus
#  define BSON_BEGIN_DECLS extern "C" {
#  define BSON_END_DECLS   }
#else
#  define BSON_BEGIN_DECLS
#  define BSON_END_DECLS
#endif


#ifndef MIN
#  define MIN(a,b) ({     \
     typeof (a) _a = (a); \
     typeof (b) _b = (b); \
     _a < _b ? _a : _b;   \
})
#endif


#ifndef MAX
#  define MAX(a,b) ({     \
     typeof (a) _a = (a); \
     typeof (b) _b = (b); \
     _a > _b ? _a : _b;   \
})
#endif


#ifndef ABS
#  define ABS(a) (((a) < 0) ? ((a) * -1) : (a))
#endif


#ifndef TRUE
#  define TRUE 1
#endif


#ifndef FALSE
#  define FALSE (!TRUE)
#endif


#if defined(_MSC_VER)
#  define BSON_ALIGNED_BEGIN(_N) __declspec(align(_N))
#  define BSON_ALIGNED_END(_N)
#else
#  define BSON_ALIGNED_BEGIN(_N)
#  define BSON_ALIGNED_END(_N) __attribute__((aligned (_N)))
#endif


#define bson_str_empty(s)  (!s[0])
#define bson_str_empty0(s) (!s || !s[0])


#ifndef BSON_DISABLE_ASSERT
#  define BSON_ASSERT(s) assert((s))
#else
#  define BSON_ASSERT(s)
#endif


#define BSON_STATIC_ASSERT(s) BSON_STATIC_ASSERT_(s, __LINE__)
#define BSON_STATIC_ASSERT_JOIN(a,b) BSON_STATIC_ASSERT_JOIN2(a, b)
#define BSON_STATIC_ASSERT_JOIN2(a,b) a##b
#define BSON_STATIC_ASSERT_(s, l) \
   typedef char BSON_STATIC_ASSERT_JOIN(static_assert_test_, \
                                        __LINE__)[(s) ? 0 : -1]


#if defined(__GNUC__)
#  define BSON_GNUC_CONST __attribute__((const))
#  define BSON_GNUC_WARN_UNUSED_RESULT __attribute__((warn_unused_result))
#else
#  define BSON_GNUC_CONST
#  define BSON_GNUC_WARN_UNUSED_RESULT
#endif


#if defined(__GNUC__) && (__GNUC__ >= 4)
#  define BSON_GNUC_NULL_TERMINATED __attribute__((sentinel))
#else
#  define BSON_GNUC_NULL_TERMINATED
#endif


#if defined(__GNUC__)
#  define BSON_LIKELY(x)    __builtin_expect (!!(x), 1)
#  define BSON_UNLIKELY(x)  __builtin_expect (!!(x), 0)
#else
#  define BSON_LIKELY(v)   v
#  define BSON_UNLIKELY(v) v
#endif


#if defined(__GNUC__)
#  define GCC_VERSION (__GNUC__ * 10000 \
                       + __GNUC_MINOR__ * 100 \
                       + __GNUC_PATCHLEVEL__)
#  if GCC_VERSION > 40400
#    define BSON_GNUC_PRINTF(f,v) __attribute__((format(gnu_printf, f, v)))
#  else
#    define BSON_GNUC_PRINTF(f,v)
#  endif /* GCC_VERSION > 40400 */
#else
#  define BSON_GNUC_PRINTF(f,v)
#endif /* __GNUC__ */


#if defined(__LP64__) || defined(_LP64)
#  define BSON_WORD_SIZE 64
#else
#  define BSON_WORD_SIZE 32
#endif


#if defined(_MSC_VER)
#  define BSON_INLINE __inline
#else
#  define BSON_INLINE inline
#endif


#ifndef BSON_DISABLE_CHECKS
#  define bson_return_if_fail(test) \
     do { \
        if (!(test)) { \
           fprintf(stderr, "%s(): precondition failed: %s\n", \
                   __FUNCTION__, #test); \
           return; \
        } \
     } while (0)
#else
#  define bson_return_if_fail(test)
#endif


#ifndef BSON_DISABLE_CHECKS
#  define bson_return_val_if_fail(test, val) \
     do { \
        if (!(test)) { \
           fprintf(stderr, "%s(): precondition failed: %s\n", \
                   __FUNCTION__, #test); \
           return (val); \
        } \
     } while (0)
#else
#  define bson_return_val_if_fail(test, val)
#endif


#if BSON_OS == 1
#  define BSON_OS_UNIX
#elif BSON_OS == 2
#  define BSON_OS_WIN32
#else
#  error Unknown operating system.
#endif


#endif /* BSON_MACROS_H */
