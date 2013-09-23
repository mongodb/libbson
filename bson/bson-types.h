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


#ifndef BSON_TYPES_H
#define BSON_TYPES_H


#include <stdlib.h>

#include "bson-macros.h"
#include "bson-memory.h"
#include "bson-stdint.h"


BSON_BEGIN_DECLS


typedef uint32_t bson_bool_t;
typedef int8_t   bson_int8_t;
typedef int16_t  bson_int16_t;
typedef int32_t  bson_int32_t;
typedef int64_t  bson_int64_t;
typedef uint8_t  bson_uint8_t;
typedef uint16_t bson_uint16_t;
typedef uint32_t bson_uint32_t;
typedef uint64_t bson_uint64_t;
typedef uint32_t bson_unichar_t;


/**
 * bson_read_func_t:
 * @fd: The file descriptor to read from.
 * @buf: The buffer to read into.
 * @count: The number of bytes to read.
 *
 * This function describes a read function that can be used to read from a file
 * descriptor.
 *
 * Returns: -1 on failure and errno is set, otherwise the number of bytes read.
 *    0 may be returned on end of stream.
 */
typedef ssize_t (*bson_read_func_t) (int     fd,
                                     void   *buf,
                                     size_t  count);

/**
 * bson_context_flags_t:
 *
 * This enumeration is used to configure a bson_context_t.
 *
 * %BSON_CONTEXT_NONE: Use default options.
 * %BSON_CONTEXT_THREAD_SAFE: Context will be called from multiple threads.
 * %BSON_CONTEXT_DISABLE_PID_CACHE: Call getpid() instead of caching the
 *   result of getpid() when initializing the context.
 * %BSON_CONTEXT_DISABLE_HOST_CACHE: Call gethostname() instead of caching the
 *   result of gethostname() when initializing the context.
 */
typedef enum
{
   BSON_CONTEXT_NONE               = 0,
   BSON_CONTEXT_THREAD_SAFE        = 1 << 0,
   BSON_CONTEXT_DISABLE_HOST_CACHE = 1 << 1,
   BSON_CONTEXT_DISABLE_PID_CACHE  = 1 << 2,
#if defined(__linux__)
   BSON_CONTEXT_USE_TASK_ID        = 1 << 3,
#endif
} bson_context_flags_t;


/**
 * bson_context_t:
 *
 * This structure manages context for the bson library. It handles
 * configuration for thread-safety and other performance related requirements.
 * Consumers will create a context and may use multiple under a variety of
 * situations.
 *
 * If your program calls fork(), you should initialize a new bson_context_t
 * using bson_context_init().
 *
 * If you are using threading, it is suggested that you use a bson_context_t
 * per thread for best performance. Alternatively, you can initialize the
 * bson_context_t with BSON_CONTEXT_THREAD_SAFE, although a performance penalty
 * will be incurred.
 *
 * Many functions will require that you provide a bson_context_t such as OID
 * generation.
 *
 * This structure is oqaque in that you cannot see the contents of the
 * structure. However, it is stack allocatable in that enough padding is
 * provided in _bson_context_t to hold the structure.
 */
typedef struct _bson_context_t bson_context_t;


/**
 * bson_t:
 *
 * This structure manages a buffer whose contents are a properly formatted
 * BSON document. You may perform various transforms on the BSON documents.
 * Additionally, it can be iterated over using bson_iter_t.
 *
 * See bson_iter_init() for iterating the contents of a bson_t.
 *
 * When building a bson_t structure using the various append functions,
 * memory allocations may occur. That is performed using power of two
 * allocations and realloc().
 *
 * See http://bsonspec.org for the BSON document spec.
 *
 * This structure is meant to fit in two sequential 64-byte cachelines.
 */
BSON_ALIGNED_BEGIN(128)
typedef struct
{
   bson_uint32_t flags;        /* Internal flags for the bson_t. */
   bson_uint32_t len;          /* Length of BSON data. */
   bson_uint8_t  padding[120]; /* Padding for stack allocation. */
} bson_t
BSON_ALIGNED_END(128);


#define BSON_INITIALIZER {3, 5, {5}}


BSON_STATIC_ASSERT(sizeof(bson_t) == 128);


/**
 * bson_oid_t:
 *
 * This structure contains the binary form of a BSON Object Id as specified
 * on http://bsonspec.org. If you would like the bson_oid_t in string form
 * see bson_oid_to_string() or bson_oid_to_string_r().
 */
typedef struct
{
   bson_uint8_t bytes[12];
} bson_oid_t;


BSON_STATIC_ASSERT(sizeof(bson_oid_t) == 12);


/**
 * bson_validate_flags_t:
 *
 * This enumeration is used for validation of BSON documents. It allows
 * selective control on what you wish to validate.
 *
 * %BSON_VALIDATE_NONE: No additional validation occurs.
 * %BSON_VALIDATE_UTF8: Check that strings are valid UTF-8.
 * %BSON_VALIDATE_DOLLAR_KEYS: Check that keys do not start with $.
 * %BSON_VALIDATE_DOT_KEYS: Check that keys do not contain a period.
 * %BSON_VALIDATE_UTF8_ALLOW_NULL: Allow NUL bytes in UTF-8 text.
 */
typedef enum
{
   BSON_VALIDATE_NONE            = 0,
   BSON_VALIDATE_UTF8            = 1 << 0,
   BSON_VALIDATE_DOLLAR_KEYS     = 1 << 1,
   BSON_VALIDATE_DOT_KEYS        = 1 << 2,
   BSON_VALIDATE_UTF8_ALLOW_NULL = 1 << 3,
} bson_validate_flags_t;


/**
 * bson_type_t:
 *
 * This enumeration contains all of the possible types within a BSON document.
 * Use bson_iter_type() to fetch the type of a field while iterating over it.
 */
typedef enum
{
   BSON_TYPE_EOD        = 0x00,
   BSON_TYPE_DOUBLE     = 0x01,
   BSON_TYPE_UTF8       = 0x02,
   BSON_TYPE_DOCUMENT   = 0x03,
   BSON_TYPE_ARRAY      = 0x04,
   BSON_TYPE_BINARY     = 0x05,
   BSON_TYPE_UNDEFINED  = 0x06,
   BSON_TYPE_OID        = 0x07,
   BSON_TYPE_BOOL       = 0x08,
   BSON_TYPE_DATE_TIME  = 0x09,
   BSON_TYPE_NULL       = 0x0A,
   BSON_TYPE_REGEX      = 0x0B,
   BSON_TYPE_DBPOINTER  = 0x0C,
   BSON_TYPE_CODE       = 0x0D,
   BSON_TYPE_SYMBOL     = 0x0E,
   BSON_TYPE_CODEWSCOPE = 0x0F,
   BSON_TYPE_INT32      = 0x10,
   BSON_TYPE_TIMESTAMP  = 0x11,
   BSON_TYPE_INT64      = 0x12,
   BSON_TYPE_MAXKEY     = 0x7F,
   BSON_TYPE_MINKEY     = 0xFF,
} bson_type_t;


/**
 * bson_subtype_t:
 *
 * This enumeration contains the various subtypes that may be used in a binary
 * field. See http://bsonspec.org for more information.
 */
typedef enum
{
   BSON_SUBTYPE_BINARY             = 0x00,
   BSON_SUBTYPE_FUNCTION           = 0x01,
   BSON_SUBTYPE_BINARY_DEPRECATED  = 0x02,
   BSON_SUBTYPE_UUID_DEPRECATED    = 0x03,
   BSON_SUBTYPE_UUID               = 0x04,
   BSON_SUBTYPE_MD5                = 0x05,
   BSON_SUBTYPE_USER               = 0x80,
} bson_subtype_t;


/**
 * bson_iter_t:
 *
 * This structure manages iteration over a bson_t structure. It keeps track
 * of the location of the current key and value within the buffer. Using the
 * various functions to get the value of the iter will read from these
 * locations.
 *
 * This structure is safe to discard on the stack. No cleanup is necessary
 * after using it.
 */
typedef struct
{
   const bson_t       *bson;        /* The bson_t being iterated. */
   size_t              offset;      /* The current offset in the bson_t. */
   const bson_uint8_t *type;        /* Pointer to current type field. */
   const bson_uint8_t *key;         /* Pointer to current key field. */
   const bson_uint8_t *data1;       /* Pointer to first chunk of element. */
   const bson_uint8_t *data2;       /* Pointer to second chunk of element. */
   const bson_uint8_t *data3;       /* Pointer to third chunk of element. */
   const bson_uint8_t *data4;       /* Pointer to fourth chunk of element. */
   size_t              next_offset; /* Offset of next element. */
   size_t              err_offset;  /* Location of decoding error. */
   bson_t              inl_bson;    /* Used for recursing into children. */
   void               *padding[6];  /* For future use. */
} bson_iter_t;


/**
 * bson_reader_t:
 *
 * This structure is used to iterate over a sequence of BSON documents. It
 * allows for them to be iterated with the possibility of no additional
 * memory allocations under certain circumstances such as reading from an
 * incoming mongo packet.
 */
BSON_ALIGNED_BEGIN(128)
typedef struct
{
   bson_uint32_t type;
   /*< private >*/
} bson_reader_t
BSON_ALIGNED_END(128);


/**
 * bson_visitor_t:
 *
 * This structure contains a series of pointers that can be executed for
 * each field of a BSON document based on the field type.
 *
 * For example, if an int32 field is found, visit_int32 will be called.
 *
 * When visiting each field using bson_iter_visit_all(), you may provide a
 * data pointer that will be provided with each callback. This might be useful
 * if you are marshaling to another language.
 *
 * You may pre-maturely stop the visitation of fields by returning TRUE in your
 * visitor. Returning FALSE will continue visitation to further fields.
 */
typedef struct
{
   bson_bool_t (*visit_before)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   bson_bool_t (*visit_after)      (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   void        (*visit_corrupt)    (const bson_iter_t    *iter,
                                    void                 *data);

   bson_bool_t (*visit_double)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    double                v_double,
                                    void                 *data);

   bson_bool_t (*visit_utf8)       (const bson_iter_t    *iter,
                                    const char           *key,
                                    size_t                v_utf8_len,
                                    const char           *v_utf8,
                                    void                 *data);

   bson_bool_t (*visit_document)   (const bson_iter_t    *iter,
                                    const char           *key,
                                    const bson_t         *v_document,
                                    void                 *data);

   bson_bool_t (*visit_array)      (const bson_iter_t    *iter,
                                    const char           *key,
                                    const bson_t         *v_array,
                                    void                 *data);

   bson_bool_t (*visit_binary)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_subtype_t        v_subtype,
                                    size_t                v_binary_len,
                                    const bson_uint8_t   *v_binary,
                                    void                 *data);

   bson_bool_t (*visit_undefined)  (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   bson_bool_t (*visit_oid)        (const bson_iter_t    *iter,
                                    const char           *key,
                                    const bson_oid_t     *v_oid,
                                    void                 *data);

   bson_bool_t (*visit_bool)       (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_bool_t           v_bool,
                                    void                 *data);

   bson_bool_t (*visit_date_time)  (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_int64_t          msec_since_epoch,
                                    void                 *data);

   bson_bool_t (*visit_null)       (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   bson_bool_t (*visit_regex)      (const bson_iter_t    *iter,
                                    const char           *key,
                                    const char           *v_regex,
                                    const char           *v_options,
                                    void                 *data);

   bson_bool_t (*visit_dbpointer)  (const bson_iter_t    *iter,
                                    const char           *key,
                                    size_t                v_collection_len,
                                    const char           *v_collection,
                                    const bson_oid_t     *v_oid,
                                    void                 *data);

   bson_bool_t (*visit_code)       (const bson_iter_t    *iter,
                                    const char           *key,
                                    size_t                v_code_len,
                                    const char           *v_code,
                                    void                 *data);

   bson_bool_t (*visit_symbol)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    size_t                v_symbol_len,
                                    const char           *v_symbol,
                                    void                 *data);

   bson_bool_t (*visit_codewscope) (const bson_iter_t    *iter,
                                    const char           *key,
                                    size_t                v_code_len,
                                    const char           *v_code,
                                    const bson_t         *v_scope,
                                    void                 *data);

   bson_bool_t (*visit_int32)      (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_int32_t          v_int32,
                                    void                 *data);

   bson_bool_t (*visit_timestamp)  (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_uint32_t         v_timestamp,
                                    bson_uint32_t         v_increment,
                                    void                 *data);

   bson_bool_t (*visit_int64)      (const bson_iter_t    *iter,
                                    const char           *key,
                                    bson_int64_t          v_int64,
                                    void                 *data);

   bson_bool_t (*visit_maxkey)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   bson_bool_t (*visit_minkey)     (const bson_iter_t    *iter,
                                    const char           *key,
                                    void                 *data);

   void *padding[9];
} bson_visitor_t;


typedef struct
{
   bson_uint32_t  domain;
   bson_uint32_t  code;
   char           message[504];
} bson_error_t;


BSON_STATIC_ASSERT(sizeof(bson_error_t) == 512);


/**
 * bson_next_power_of_two:
 * @v: A 32-bit unsigned integer of required bytes.
 *
 * Determines the next larger power of two for the value of @v
 * in a constant number of operations.
 *
 * It is up to the caller to guarantee this will not overflow.
 *
 * Returns: The next power of 2 from @v.
 */
static BSON_INLINE bson_uint32_t
bson_next_power_of_two (bson_uint32_t v)
{
   v--;
   v |= v >> 1;
   v |= v >> 2;
   v |= v >> 4;
   v |= v >> 8;
   v |= v >> 16;
   v++;

   return v;
}


BSON_END_DECLS


#endif /* BSON_TYPES_H */
