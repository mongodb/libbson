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


#ifndef BSON_H
#define BSON_H


#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "bson-endian.h"
#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


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
 */
typedef struct
{
   bson_int32_t   allocated;  /* Length of entire allocation. */
   bson_uint32_t  len;        /* Length of data[]. */
   bson_uint8_t  *data;       /* Pointer to data buffer. */
   bson_uint8_t   inlbuf[16]; /* Inline buffer for small BSON. */
} bson_t;


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
   void               *padding[6];  /* For future use. */
} bson_iter_t;


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
 * bson_validate_flags_t:
 *
 * This enumeration is used for validation of BSON documents. It allows
 * selective control on what you wish to validate.
 *
 * %BSON_VALIDATE_NONE: No additional validation occurs.
 * %BSON_VALIDATE_UTF8: Check that strings are valid UTF-8.
 * %BSON_VALIDATE_DOLLAR_KEYS: Check that keys do not start with $.
 * %BSON_VALIDATE_DOT_KEYS: Check that keys do not contain a period.
 */
typedef enum
{
   BSON_VALIDATE_NONE        = 0,
   BSON_VALIDATE_UTF8        = 1 << 0,
   BSON_VALIDATE_DOLLAR_KEYS = 1 << 1,
   BSON_VALIDATE_DOT_KEYS    = 1 << 2,
} bson_validate_flags_t;


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
 */
typedef struct
{
   void (*visit_before)     (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void (*visit_after)      (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void (*visit_corrupt)    (const bson_iter_t    *iter,
                             void                 *data);

   void (*visit_double)     (const bson_iter_t    *iter,
                             const char           *key,
                             double                v_double,
                             void                 *data);

   void (*visit_utf8)       (const bson_iter_t    *iter,
                             const char           *key,
                             size_t                v_utf8_len,
                             const char           *v_utf8,
                             void                 *data);

   void (*visit_document)   (const bson_iter_t    *iter,
                             const char           *key,
                             const bson_t         *v_document,
                             void                 *data);

   void (*visit_array)      (const bson_iter_t    *iter,
                             const char           *key,
                             const bson_t         *v_array,
                             void                 *data);

   void (*visit_binary)     (const bson_iter_t    *iter,
                             const char           *key,
                             bson_subtype_t        v_subtype,
                             size_t                v_binary_len,
                             const bson_uint8_t   *v_binary,
                             void                 *data);

   void (*visit_undefined)  (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void (*visit_oid)        (const bson_iter_t    *iter,
                             const char           *key,
                             const bson_oid_t     *v_oid,
                             void                 *data);

   void (*visit_bool)       (const bson_iter_t    *iter,
                             const char           *key,
                             bson_bool_t           v_bool,
                             void                 *data);

   void (*visit_date_time)  (const bson_iter_t    *iter,
                             const char           *key,
                             const struct timeval *v_date_time,
                             void                 *data);

   void (*visit_null)       (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void (*visit_regex)      (const bson_iter_t    *iter,
                             const char           *key,
                             const char           *v_regex,
                             const char           *v_options,
                             void                 *data);

   void (*visit_dbpointer)  (const bson_iter_t    *iter,
                             const char           *key,
                             size_t                v_collection_len,
                             const char           *v_collection,
                             const bson_oid_t     *v_oid,
                             void                 *data);

   void (*visit_code)       (const bson_iter_t    *iter,
                             const char           *key,
                             size_t                v_code_len,
                             const char           *v_code,
                             void                 *data);

   void (*visit_symbol)     (const bson_iter_t    *iter,
                             const char           *key,
                             size_t                v_symbol_len,
                             const char           *v_symbol,
                             void                 *data);

   void (*visit_codewscope) (const bson_iter_t    *iter,
                             const char           *key,
                             size_t                v_code_len,
                             const char           *v_code,
                             const bson_t         *v_scope,
                             void                 *data);

   void (*visit_int32)      (const bson_iter_t    *iter,
                             const char           *key,
                             bson_int32_t          v_int32,
                             void                 *data);

   void (*visit_timestamp)  (const bson_iter_t    *iter,
                             const char           *key,
                             bson_uint32_t         v_timestamp,
                             bson_uint32_t         v_increment,
                             void                 *data);

   void (*visit_int64)      (const bson_iter_t    *iter,
                             const char           *key,
                             bson_int64_t          v_int64,
                             void                 *data);

   void (*visit_maxkey)     (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void (*visit_minkey)     (const bson_iter_t    *iter,
                             const char           *key,
                             void                 *data);

   void *padding[9];
} bson_visitor_t;


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
typedef struct
{
   void *opaque[16];
} bson_context_t;


/**
 * bson_reader_t:
 *
 * This structure is used to iterate over a sequence of BSON documents. It
 * allows for them to be iterated with the possibility of no additional
 * memory allocations under certain circumstances such as reading from an
 * incoming mongo packet.
 */
typedef struct
{
   bson_uint32_t  type;
   void          *padding[15];
} bson_reader_t;


/**
 * bson_context_init:
 * @context: A bson_context_t.
 * @flags: Flags for initialization.
 *
 * Initializes @context with the flags specified.
 *
 * In most cases, you want to call this with @flags set to BSON_CONTEXT_NONE
 * and create once instance per-thread.
 *
 * If you absolutely must have a single context for your application and use
 * more than one thread, then BSON_CONTEXT_THREAD_SAFE should be bitwise or'd
 * with your flags. This requires synchronization between threads.
 *
 * If you expect your hostname to change often, you may consider specifying
 * BSON_CONTEXT_DISABLE_HOST_CACHE so that gethostname() is called for every
 * OID generated. This is much slower.
 *
 * If you expect your pid to change without notice, such as from an unexpected
 * call to fork(), then specify BSON_CONTEXT_DISABLE_PID_CACHE.
 */
void
bson_context_init (bson_context_t       *context,
                   bson_context_flags_t  flags);



/**
 * bson_context_destroy:
 * @context: A bson_context_t.
 *
 * Cleans up a bson_context_t and releases any associated resources.  This
 * should be called when you are done using @context.
 */
void
bson_context_destroy (bson_context_t *context);


/**
 * bson_new:
 *
 * Allocates a new bson_t structure. Call the various bson_append_*()
 * functions to add fields to the bson. You can iterate the bson_t at any
 * time using a bson_iter_t and bson_iter_init().
 *
 * Returns: A newly allocated bson_t that should be freed with bson_destroy().
 */
bson_t *
bson_new (void);


/**
 * bson_new_from_data:
 * @data: A buffer containing a serialized bson document.
 * @length: The length of the document in bytes.
 *
 * Creates a new bson_t structure using the data provided. @data should contain
 * at least @length bytes that can be copied into the new bson_t structure.
 *
 * Returns: A newly allocate bson_t that should be freed with bson_destroy().
 *   If the first four bytes (little-endian) of data do not match @length,
 *   then NULL will be returned.
 */
bson_t *
bson_new_from_data (const bson_uint8_t *data,
                    size_t              length);


/**
 * bson_sized_new:
 * @size: A size_t containing the number of bytes to allocate.
 *
 * This will allocate a new bson_t with enough bytes to hold a buffer
 * sized @size. @size must be smaller than INT_MAX bytes.
 *
 * Returns: A newly allocated bson_t that should be freed with bson_destroy().
 */
bson_t *
bson_sized_new (size_t bytes);


/**
 * bson_destroy:
 * @bson: A bson_t.
 *
 * Frees the resources associated with @bson.
 */
void
bson_destroy (bson_t *bson);


/**
 * bson_compare:
 * @bson: A bson_t.
 * @other: A bson_t.
 *
 * Compares @bson to @other in a qsort() style comparison.
 * See qsort() for information on how this function works.
 *
 * Returns: Less than zero, zero, or greater than zero.
 */
int
bson_compare (const bson_t *bson,
              const bson_t *other);

/*
 * bson_compare:
 * @bson: A bson_t.
 * @other: A bson_t.
 *
 * Checks to see if @bson and @other are equal.
 *
 * Returns: TRUE if equal; otherwise FALSE.
 */
bson_bool_t
bson_equal (const bson_t *bson,
            const bson_t *other);


/**
 * bson_validate:
 * @bson: A bson_t.
 * @offset: A location for the error offset.
 *
 * Validates a BSON document by walking through the document and inspecting
 * the fields for valid content.
 *
 * Returns: TRUE if @bson is valid; otherwise FALSE and @offset is set.
 */
bson_bool_t
bson_validate (const bson_t          *bson,
               bson_validate_flags_t  flags,
               size_t                *offset);


/**
 * bson_as_json:
 * @bson: A bson_t.
 * @length: A location for the string length, or NULL.
 *
 * Creates a new string containing @bson in extended JSON format. The caller
 * is responsible for freeing the resulting string. If @length is non-NULL,
 * then the length of the resulting string will be placed in @length.
 *
 * See http://docs.mongodb.org/manual/reference/mongodb-extended-json/ for
 * more information on extended JSON.
 *
 * Returns: A newly allocated string that should be freed with bson_free().
 */
char *
bson_as_json (const bson_t *bson,
              size_t       *length);


/**
 * bson_append_array:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @array: A bson_t containing the array.
 *
 * Appends a BSON array to @bson. BSON arrays are like documents where the
 * key is the string version of the index. For example, the first item of the
 * array would have the key "0". The second item would have the index "1".
 */
void
bson_append_array (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   const bson_t *array);


/**
 * bson_append_binary:
 * @bson: A bson_t to append.
 * @key: The key for the field.
 * @subtype: The bson_subtype_t of the binary.
 * @binary: The binary buffer to append.
 * @length: The length of @binary.
 *
 * Appends a binary buffer to the BSON document.
 */
void
bson_append_binary (bson_t             *bson,
                    const char         *key,
                    int                 key_length,
                    bson_subtype_t      subtype,
                    const bson_uint8_t *binary,
                    bson_uint32_t       length);


/**
 * bson_append_bool:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: The boolean value.
 *
 * Appends a new field to @bson of type BSON_TYPE_BOOL.
 */
void
bson_append_bool (bson_t      *bson,
                  const char  *key,
                  int          key_length,
                  bson_bool_t  value);


/**
 * bson_append_code:
 * @bson: A bson_t.
 * @key: The key for the document.
 * @javascript: JavaScript code to be executed.
 *
 * Appends a field of type BSON_TYPE_CODE to the BSON document. @javascript
 * should contain a script in javascript to be executed.
 */
void
bson_append_code (bson_t     *bson,
                  const char *key,
                  int         key_length,
                  const char *javascript);


/**
 * bson_append_code_with_scope:
 * @bson: A bson_t.
 * @key: The key for the document.
 * @javascript: JavaScript code to be executed.
 * @scope: A bson_t containing the scope for @javascript.
 *
 * Appends a field of type BSON_TYPE_CODEWSCOPE to the BSON document.
 * @javascript should contain a script in javascript to be executed.
 */
void
bson_append_code_with_scope (bson_t       *bson,
                             const char   *key,
                             int           key_length,
                             const char   *javascript,
                             const bson_t *scope);


/**
 * bson_append_dbpointer:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @collection: The collection name.
 * @oid: The oid to the reference.
 *
 * Appends a new field of type BSON_TYPE_DBPOINTER. This datum type is
 * deprecated in the BSON spec and should not be used in new code.
 */
void
bson_append_dbpointer (bson_t           *bson,
                       const char       *key,
                       int               key_length,
                       const char       *collection,
                       const bson_oid_t *oid);


/**
 * bson_append_double:
 * @bson: A bson_t.
 * @key: The key for the field.
 *
 * Appends a new field to @bson of the type BSON_TYPE_DOUBLE.
 */
void
bson_append_double (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    double      value);


/**
 * bson_append_document:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: A bson_t containing the subdocument.
 *
 * Appends a new field to @bson of the type BSON_TYPE_DOCUMENT.
 * The documents contents will be copied into @bson.
 */
void
bson_append_document (bson_t       *bson,
                      const char   *key,
                      int           key_length,
                      const bson_t *value);


/**
 * bson_append_int32:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: The bson_int32_t 32-bit integer value.
 *
 * Appends a new field of type BSON_TYPE_INT32 to @bson.
 */
void
bson_append_int32 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int32_t  value);


/**
 * bson_append_int64:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: The bson_int64_t 64-bit integer value.
 *
 * Appends a new field of type BSON_TYPE_INT64 to @bson.
 */
void
bson_append_int64 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int64_t  value);


/**
 * bson_append_minkey:
 * @bson: A bson_t.
 * @key: The key for the field.
 *
 * Appends a new field of type BSON_TYPE_MINKEY to @bson. This is a special
 * type that compares lower than all other possible BSON element values.
 *
 * See http://bsonspec.org for more information on this type.
 */
void
bson_append_minkey (bson_t     *bson,
                    const char *key,
                    int         key_length);


/**
 * bson_append_maxkey:
 * @bson: A bson_t.
 * @key: The key for the field.
 *
 * Appends a new field of type BSON_TYPE_MAXKEY to @bson. This is a special
 * type that compares higher than all other possible BSON element values.
 *
 * See http://bsonspec.org for more information on this type.
 */
void
bson_append_maxkey (bson_t     *bson,
                    const char *key,
                    int         key_length);


/**
 * bson_append_null:
 * @bson: A bson_t.
 * @key: The key for the field.
 *
 * Appends a new field to @bson with NULL for the value.
 */
void
bson_append_null (bson_t     *bson,
                  const char *key,
                  int         key_length);


/**
 * bson_append_oid:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @oid: bson_oid_t.
 *
 * Appends a new field to the @bson of type BSON_TYPE_OID using
 * the contents of @oid.
 *
 * Returns: @bson or a newly allcoated structure if a memory
 *   allocation was required.
 */
void
bson_append_oid (bson_t           *bson,
                 const char       *key,
                 int               key_length,
                 const bson_oid_t *oid);


/**
 * bson_append_regex:
 * @bson: A bson_t.
 * @key: The key of the field.
 * @regex: The regex to append to the bson.
 * @options: Options for @regex.
 *
 * Appends a new field to @bson of type BSON_TYPE_REGEX. @regex should
 * be the regex string. @options should contain the options for the regex.
 *
 * Valid options for @options are:
 *
 *   'i' for case-insensitive.
 *   'm' for multiple matching.
 *   'x' for verbose mode.
 *   'l' to make \w and \W locale dependent.
 *   's' for dotall mode ('.' matches everything)
 *   'u' to make \w and \W match unicode.
 *
 * For more information on what comprimises a BSON regex, see bsonspec.org.
 */
void
bson_append_regex (bson_t     *bson,
                   const char *key,
                   int         key_length,
                   const char *regex,
                   const char *options);


/**
 * bson_append_string:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: A UTF-8 encoded string.
 * @length: The length of @value or -1 if it is NUL terminated.
 *
 * Appends a new field to @bson using @key as the key and @value as the UTF-8
 * encoded value.
 *
 * It is the callers responsibility to ensure @value is valid UTF-8. You can
 * use bson_utf8_validate() to perform this check.
 *
 * Returns: @bson or a newly allcoated structure if a memory
 *   allocation was required.
 */
void
bson_append_string (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    const char *value,
                    int         length);


/**
 * bson_append_symbol:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: The symbol as a string.
 * @length: The length of @value or -1 if NUL-terminated.
 *
 * Appends a new field to @bson of type BSON_TYPE_SYMBOL. This BSON type is
 * deprecated and should not be used in new code.
 *
 * See http://bsonspec.org for more information on this type.
 */
void
bson_append_symbol (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    const char *value,
                    int         length);


/**
 * bson_append_time_t:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: A time_t.
 *
 * Appends a BSON_TYPE_DATE_TIME field to @bson using the time_t @value for
 * the number of seconds since UNIX epoch in UTC.
 */
void
bson_append_time_t (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    time_t      value);


/**
 * bson_append_timeval:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @value: A struct timeval containing the date and time.
 *
 * Appends a BSON_TYPE_DATE_TIME field to @bson using the struct timeval
 * provided. The time is persisted in milliseconds since the UNIX epoch in
 * UTC.
 */
void
bson_append_timeval (bson_t         *bson,
                     const char     *key,
                     int             key_length,
                     struct timeval *value);


/**
 * bson_append_timestamp:
 * @bson: A bson_t.
 * @key: The key for the field.
 * @timestamp: 4 byte timestamp.
 * @increment: 4 byte increment for timestamp.
 *
 * Appends a field of type BSON_TYPE_TIMESTAMP to @bson. This is a special
 * type used by MongoDB replication and sharding. If you need generic time
 * and date fields use bson_append_time_t() or bson_append_timeval().
 *
 * Setting @increment and @timestamp to zero has special semantics. See
 * http://bsonspec.org for more information on this field type.
 */
void
bson_append_timestamp (bson_t        *bson,
                       const char    *key,
                       int            key_length,
                       bson_uint32_t  timestamp,
                       bson_uint32_t  increment);


/**
 * bson_append_undefined:
 * @bson: A bson_t.
 * @key: The key for the field.
 *
 * Appends a field of type BSON_TYPE_UNDEFINED. This type is deprecated in
 * the spec and should not be used for new code. However, it is provided for
 * those needing to interact with legacy systems.
 */
void
bson_append_undefined (bson_t     *bson,
                       const char *key,
                       int         key_length);


/**
 * bson_empty:
 * @b: a bson_t.
 *
 * Checks to see if @b is an empty BSON document. An empty BSON document is
 * a 5 byte document which contains the length (4 bytes) and a single NUL
 * byte indicating end of fields.
 */
#define bson_empty(b) ((b)->len == 5)


/**
 * bson_empty0:
 *
 * Like bson_empty() but treats NULL the same as an empty bson_t document.
 */
#define bson_empty0(b) (!(b) || bson_empty(b))


/**
 * bson_oid_compare:
 * @oid1: The bson_oid_t to compare as the left.
 * @oid2: The bson_oid_t to compare as the right.
 *
 * A qsort() style compare function that will return less than zero
 * if @oid1 is less than @oid2, zero if they are the same, and greater
 * than zero if @oid2 is greater than @oid1.
 *
 * Returns: A qsort() style compare integer.
 */
int
bson_oid_compare (const bson_oid_t *oid1,
                  const bson_oid_t *oid2);


/**
 * bson_oid_copy:
 * @src: A bson_oid_t to copy from.
 * @dst: A bson_oid_t to copy to.
 *
 * Copies the contents of @src to @dst.
 */
void
bson_oid_copy (const bson_oid_t *src,
               bson_oid_t       *dst);


/**
 * bson_oid_equal:
 * @oid1: A bson_oid_t.
 * @oid2: A bson_oid_t.
 *
 * Compares for equality of @oid1 and @oid2. If they are equal, then
 * TRUE is returned, otherwise FALSE.
 *
 * Returns: A boolean indicating the equality of @oid1 and @oid2.
 */
bson_bool_t
bson_oid_equal (const bson_oid_t *oid1,
                const bson_oid_t *oid2);


/**
 * bson_oid_get_time_t:
 * @oid: A bson_oid_t.
 *
 * Fetches the time for which @oid was created.
 *
 * Returns: A time_t.
 */
time_t
bson_oid_get_time_t (const bson_oid_t *oid);


/**
 * bson_oid_hash:
 * @oid: A bson_oid_t to hash.
 *
 * Hashes the bytes of the provided bson_oid_t using DJB hash.
 * This allows bson_oid_t to be used as keys in a hash table.
 *
 * Returns: A hash value corresponding to @oid.
 */
bson_uint32_t
bson_oid_hash (const bson_oid_t *oid);


/**
 * bson_oid_init:
 * @oid: A bson_oid_t.
 *
 * Generates bytes for a new bson_oid_t and stores them in @oid. The bytes
 * will be generated according to the specification and includes the current
 * time, first 3 bytes of MD5(hostname), pid (or tid), and monotonic counter.
 */
void
bson_oid_init (bson_oid_t     *oid,
               bson_context_t *context);


/**
 * bson_oid_init_from_string:
 * @oid: A bson_oid_t
 * @str: A string containing at least 24 characters.
 *
 * Parses @str containing hex formatted bytes of an object id and places
 * the bytes in @oid.
 */
void
bson_oid_init_from_string (bson_oid_t *oid,
                           const char *str);


/**
 * bson_oid_init_sequence:
 * @oid: A bson_oid_t.
 * @context: A bson_context_t.
 *
 * Initializes @oid with the next oid in the sequence. The first 4 bytes
 * contain the current time and the following 8 contain a 64-bit integer
 * in big-endian format.
 *
 * The bson_oid_t generated by this function is not guaranteed to be globally
 * unique. Only unique within this context. It is however, guaranteed to be
 * sequential.
 */
void
bson_oid_init_sequence (bson_oid_t     *oid,
                        bson_context_t *context);


/**
 * bson_oid_to_string:
 * @oid: A bson_oid_t.
 * @str: A location to store the resulting string.
 *
 * Formats a bson_oid_t into a string. @str must contain enough bytes
 * for the resulting string which is 25 bytes with a terminating NUL-byte.
 */
void
bson_oid_to_string (const bson_oid_t *oid,
                    char              str[static 25]);


#define BSON_ITER_HOLDS_DOUBLE(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_DOUBLE)

#define BSON_ITER_HOLDS_UTF8(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_UTF8)

#define BSON_ITER_HOLDS_DOCUMENT(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_DOCUMENT)

#define BSON_ITER_HOLDS_ARRAY(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_ARRAY)

#define BSON_ITER_HOLDS_BINARY(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_BINARY)

#define BSON_ITER_HOLDS_UNDEFINED(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_UNDEFINED)

#define BSON_ITER_HOLDS_OID(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_OID)

#define BSON_ITER_HOLDS_BOOL(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_BOOL)

#define BSON_ITER_HOLDS_DATE_TIME(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_DATE_TIME)

#define BSON_ITER_HOLDS_NULL(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_NULL)

#define BSON_ITER_HOLDS_REGEX(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_REGEX)

#define BSON_ITER_HOLDS_DBPOINTER(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_DBPOINTER)

#define BSON_ITER_HOLDS_CODE(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_CODE)

#define BSON_ITER_HOLDS_SYMBOL(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_SYMBOL)

#define BSON_ITER_HOLDS_CODEWSCOPE(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_CODEWSCOPE)

#define BSON_ITER_HOLDS_INT32(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_INT32)

#define BSON_ITER_HOLDS_TIMESTAMP(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_TIMESTAMP)

#define BSON_ITER_HOLDS_INT64(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_INT64)

#define BSON_ITER_HOLDS_MAXKEY(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_MAXKEY)

#define BSON_ITER_HOLDS_MINKEY(iter) \
   (bson_iter_type((iter)) == BSON_TYPE_MINKEY)


/**
 * bson_iter_string_len_unsafe:
 * @iter: a bson_iter_t.
 *
 * Returns the length of a string currently pointed to by @iter. This performs
 * no validation so the is responsible for knowing the BSON is valid. Calling
 * bson_validate() is one way to do this ahead of time.
 */
static BSON_INLINE bson_uint32_t
bson_iter_string_len_unsafe (const bson_iter_t *iter)
{
   bson_uint32_t val;
   memcpy(&val, iter->data1, 4);
   return BSON_UINT32_FROM_LE(val);
}


/**
 * bson_iter_array:
 * @iter: a bson_iter_t.
 * @array_len: A location for the array length.
 * @array: A location for a pointer to the array buffer.
 *
 * Retrieves the data to the array BSON structure and stores the length
 * of the array buffer in @array_len and the array buffer in @array.
 *
 * If you would like to iterate over the child contents, you might consider
 * creating a bson_t on the stack such as the following. It allows you to
 * call functions taking a const bson_t* only.
 *
 * bson_t b = { 0 };
 * bson_iter_array(iter, &b.len, &b.data);
 *
 * There is no need to cleanup the bson_t structure as no data can be modified
 * in the process of its use.
 */
void
bson_iter_array (const bson_iter_t   *iter,
                 bson_uint32_t       *array_len,
                 const bson_uint8_t **array);


/**
 * bson_iter_binary:
 * @iter: A bson_iter_t
 * @subtype: The binary subtype.
 * @binary_len: A location for the length of @binary.
 * @binary: A location for a pointer to the binary data.
 *
 * Retrieves the BSON_TYPE_BINARY field. The subtype is stored in @subtype.
 * The length of @binary in bytes is stored in @binary_len.
 *
 * @binary should not be modified or freed and is only valid while @iter is
 * on the current field.
 */
void
bson_iter_binary (const bson_iter_t   *iter,
                  bson_subtype_t      *subtype,
                  bson_uint32_t       *binary_len,
                  const bson_uint8_t **binary);


/**
 * bson_iter_code:
 * @iter: A bson_iter_t.
 * @length: A location for the code length.
 *
 * Retrieves the current field of type BSON_TYPE_CODE. The length of the
 * resulting string is stored in @length.
 */
const char *
bson_iter_code (const bson_iter_t *iter,
                bson_uint32_t     *length);


/**
 * bson_iter_code_unsafe:
 * @iter: A bson_iter_t.
 * @length: A location for the length of the resulting string.
 *
 * Like bson_iter_code() but performs no integrity checks.
 *
 * Returns: A string that should not be modified or freed.
 */
static BSON_INLINE const char *
bson_iter_code_unsafe (const bson_iter_t *iter,
                       bson_uint32_t     *length)
{
   *length = bson_iter_string_len_unsafe(iter);
   return (const char *)iter->data2;
}


/**
 * bson_iter_codewscope:
 * @iter: A bson_iter_t.
 * @length: A location for the length of resulting string.
 * @scope_len: A location for the length of @scope.
 * @scope: A location for the scope encoded as BSON.
 *
 * Similar to bson_iter_code() but with a scope associated encoded as a
 * BSON document. @scope should not be modified or freed. It is valid while
 * @iter is valid.
 *
 * Returns: The string that should not be modified or freed.
 */
const char *
bson_iter_codewscope (const bson_iter_t   *iter,
                      bson_uint32_t       *length,
                      bson_uint32_t       *scope_len,
                      const bson_uint8_t **scope);


/**
 * bson_iter_dbpointer:
 * @iter: A bson_iter_t.
 * @collection_len: A location for the length of @collection.
 * @collection: A location for the collection name.
 * @oid: A location for the oid.
 *
 * Retrieves a BSON_TYPE_DBPOINTER field. @collection_len will be set to the
 * length of the collection name. The collection name will be placed into
 * @collection. The oid will be placed into @oid.
 *
 * @collection and @oid should not be modified.
 */
void
bson_iter_dbpointer (const bson_iter_t  *iter,
                     bson_uint32_t      *collection_len,
                     const char        **collection,
                     const bson_oid_t  **oid);


/**
 * bson_iter_document:
 * @iter: a bson_iter_t.
 * @document_len: A location for the document length.
 * @document: A location for a pointer to the document buffer.
 *
 * Retrieves the data to the document BSON structure and stores the length of
 * the document buffer in @document_len and the document buffer in @document.
 *
 * If you would like to iterate over the child contents, you might consider
 * creating a bson_t on the stack such as the following. It allows you to call
 * functions taking a const bson_t* only.
 *
 * bson_t b = { 0 };
 * bson_iter_document(iter, &b.len, &b.data);
 *
 * There is no need to cleanup the bson_t structure as no data can be modified
 * in the process of its use.
 */
void
bson_iter_document (const bson_iter_t   *iter,
                    bson_uint32_t       *document_len,
                    const bson_uint8_t **document);


/**
 * bson_iter_double:
 * @iter: A bson_iter_t.
 *
 * Retrieves the current field of type BSON_TYPE_DOUBLE.
 *
 * Returns: A double.
 */
double
bson_iter_double (const bson_iter_t *iter);


/**
 * bson_iter_double_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_double() but does not perform an integrity checking.
 *
 * Returns: A double.
 */
static BSON_INLINE double
bson_iter_double_unsafe (const bson_iter_t *iter)
{
   double val;
   memcpy(&val, iter->data1, 8);
   return val;
}


/**
 * bson_iter_init:
 *
 */
bson_bool_t
bson_iter_init (bson_iter_t  *iter,
                const bson_t *bson);


/**
 * bson_iter_init_find:
 *
 */
bson_bool_t
bson_iter_init_find (bson_iter_t  *iter,
                     const bson_t *bson,
                     const char   *key);


/**
 * bson_iter_int32:
 * @iter: A bson_iter_t.
 *
 * Retrieves the value of the field of type BSON_TYPE_INT32.
 *
 * Returns: A 32-bit signed integer.
 */
bson_int32_t
bson_iter_int32 (const bson_iter_t *iter);


/**
 * bson_iter_int32_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_int32() but with no integrity checking.
 *
 * Returns: A 32-bit signed integer.
 */
static BSON_INLINE bson_int32_t
bson_iter_int32_unsafe (const bson_iter_t *iter)
{
   bson_int32_t val;
   memcpy(&val, iter->data1, 4);
   return BSON_UINT32_FROM_LE(val);
}


/**
 * bson_iter_int64:
 * @iter: A bson_iter_t.
 *
 * Retrieves a 64-bit signed integer for the current BSON_TYPE_INT64 field.
 *
 * Returns: A 64-bit signed integer.
 */
bson_int64_t
bson_iter_int64 (const bson_iter_t *iter);


/**
 * bson_iter_int64_unsafe:
 * @iter: a bson_iter_t.
 *
 * Similar to bson_iter_int64() but without integrity checking.
 *
 * Returns: A 64-bit signed integer.
 */
static BSON_INLINE bson_int64_t
bson_iter_int64_unsafe (const bson_iter_t *iter)
{
   bson_int64_t val;
   memcpy(&val, iter->data1, 8);
   return BSON_UINT64_FROM_LE(val);
}


/**
 * bson_iter_find:
 *
 */
bson_bool_t
bson_iter_find (bson_iter_t *iter,
                const char  *key);


/**
 * bson_iter_next:
 *
 */
bson_bool_t
bson_iter_next (bson_iter_t *iter);


/**
 * bson_iter_oid:
 * @iter: A bson_iter_t.
 *
 * Retrieves the current field of type BSON_TYPE_OID. The result is valid
 * while @iter is valid.
 *
 * Returns: A bson_oid_t that should not be modified or freed.
 */
const bson_oid_t *
bson_iter_oid (const bson_iter_t *iter);


/**
 * bson_iter_oid_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_oid() but performs no integrity checks.
 *
 * Returns: A bson_oid_t that should not be modified or freed.
 */
static BSON_INLINE const bson_oid_t *
bson_iter_oid_unsafe (const bson_iter_t *iter)
{
   return (const bson_oid_t *)iter->data1;
}


/**
 * bson_iter_key:
 * @iter: A bson_iter_t.
 *
 * Retrieves the key of the current field. The resulting key is valid while
 * @iter is valid.
 *
 * Returns: A string that should not be modified or freed.
 */
const char *
bson_iter_key (const bson_iter_t *iter);


/**
 * bson_iter_key_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_key() but performs no integrity checking.
 *
 * Returns: A string that should not be modified or freed.
 */
static BSON_INLINE const char *
bson_iter_key_unsafe (const bson_iter_t *iter)
{
   return (const char *)iter->key;
}


/**
 * bson_iter_string:
 * @iter: A bson_iter_t.
 * @length: A location for the length of the string.
 *
 * Retrieves the current field of type BSON_TYPE_UTF8 as a UTF-8 encoded
 * string.
 *
 * Returns: A string that should not be modified or freed.
 */
const char *
bson_iter_string (const bson_iter_t *iter,
                  bson_uint32_t     *length);


/**
 * bson_iter_string_unsafe:
 *
 * Similar to bson_iter_string() but performs no integrity checking.
 *
 * Returns: A string that should not be modified or freed.
 */
static BSON_INLINE const char *
bson_iter_string_unsafe (const bson_iter_t *iter,
                         bson_uint32_t     *length)
{
   *length = bson_iter_string_len_unsafe(iter);
   return (const char *)iter->data2;
}


/**
 * bson_iter_time_t:
 * @iter: A bson_iter_t.
 *
 * Retrieves the current field of type BSON_TYPE_DATE_TIME as a time_t.
 *
 * Returns: A time_t containing the number of seconds since UNIX epoch
 *          in UTC.
 */
time_t
bson_iter_time_t (const bson_iter_t *iter);


/**
 * bson_iter_time_t_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_time_t() but performs no integrity checking.
 *
 * Returns: A time_t containing the number of seconds since UNIX epoch
 *          in UTC.
 */
static BSON_INLINE time_t
bson_iter_time_t_unsafe (const bson_iter_t *iter)
{
   return (time_t)bson_iter_int64_unsafe(iter);
}


/**
 * bson_iter_timeval:
 * @iter: A bson_iter_t.
 * @tv: A struct timeval.
 *
 * Retrieves the current field of type BSON_TYPE_DATE_TIME and stores it into
 * the struct timeval provided. tv->tv_sec is set to the number of seconds
 * since the UNIX epoch in UTC.
 */
void
bson_iter_timeval (const bson_iter_t *iter,
                   struct timeval    *tv);


/**
 * bson_iter_timeval_unsafe:
 * @iter: A bson_iter_t.
 * @tv: A struct timeval.
 *
 * Similar to bson_iter_timeval() but performs no integrity checking.
 */
static BSON_INLINE void
bson_iter_timeval_unsafe (const bson_iter_t *iter,
                          struct timeval    *tv)
{
   tv->tv_sec = bson_iter_int64_unsafe(iter);
   tv->tv_usec = 0;
}


/**
 * bson_iter_bool:
 * @iter: A bson_iter_t.
 *
 * Retrieves the current field of type BSON_TYPE_BOOL.
 *
 * Returns: TRUE or FALSE.
 */
bson_bool_t
bson_iter_bool (const bson_iter_t *iter);


/**
 * bson_iter_bool_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_bool() but performs no integrity checking.
 *
 * Returns: TRUE or FALSE.
 */
static BSON_INLINE bson_bool_t
bson_iter_bool_unsafe (const bson_iter_t *iter)
{
   char val;
   memcpy(&val, iter->data1, 1);
   return !!val;
}


/**
 * bson_iter_symbol:
 * @iter: A bson_iter_t.
 * @length: A location for the length of the symbol.
 *
 * Retrieves the symbol of the current field of type BSON_TYPE_SYMBOL.
 *
 * Returns: A string containing the symbol as UTF-8. The value should not be
 *   modified or freed.
 */
const char *
bson_iter_symbol (const bson_iter_t *iter,
                  bson_uint32_t     *length);


/**
 * bson_iter_type:
 * @iter: A bson_iter_t.
 *
 * Retrieves the type of the current field.  It may be useful to check the
 * type using the BSON_ITER_HOLDS_*() macros.
 *
 * Returns: A bson_type_t.
 */
bson_type_t
bson_iter_type (const bson_iter_t *iter);


/**
 * bson_iter_type_unsafe:
 * @iter: A bson_iter_t.
 *
 * Similar to bson_iter_type() but performs no integrity checking.
 *
 * Returns: A bson_type_t.
 */
static BSON_INLINE bson_type_t
bson_iter_type_unsafe (const bson_iter_t *iter)
{
   return iter->type[0];
}


/**
 * bson_iter_visit_all:
 * @iter: A bson_iter_t.
 * @visitor: A bson_visitor_t containing the visitors.
 * @data: User data for @visitor data parameters.
 *
 * Visits all fields forward from the current position of @iter. For each
 * field found a function in @visitor will be called. Typically you will
 * use this immediately after initializing a bson_iter_t.
 *
 *   bson_iter_init(&iter, b);
 *   bson_iter_visit_all(&iter, my_visitor, NULL);
 *
 * @iter will no longer be valid after this function has executed and will
 * need to be reinitialized if intending to reuse.
 */
void
bson_iter_visit_all (bson_iter_t          *iter,
                     const bson_visitor_t *visitor,
                     void                 *data);


/**
 * bson_oid_compare_unsafe:
 * @oid1: A bson_oid_t.
 * @oid2: A bson_oid_t.
 *
 * Performs a qsort() style comparison between @oid1 and @oid2.
 *
 * This function is meant to be as fast as possible and therefore performs
 * no argument validation. That is the callers responsibility.
 *
 * Returns: An integer < 0 if @oid1 is less than @oid2. Zero if they are equal.
 *          An integer > 0 if @oid1 is greater than @oid2.
 */
static BSON_INLINE int
bson_oid_compare_unsafe (const bson_oid_t *oid1,
                         const bson_oid_t *oid2)
{
   return memcmp(oid1, oid2, sizeof *oid1);
}


/**
 * bson_oid_equal_unsafe:
 * @oid1: A bson_oid_t.
 * @oid2: A bson_oid_t.
 *
 * Checks the equality of @oid1 and @oid2.
 *
 * This function is meant to be as fast as possible and therefore performs
 * no checks for argument validity. That is the callers responsibility.
 *
 * Returns: TRUE if @oid1 and @oid2 are equal; otherwise FALSE.
 */
static BSON_INLINE bson_bool_t
bson_oid_equal_unsafe (const bson_oid_t *oid1,
                       const bson_oid_t *oid2)
{
   return !memcmp(oid1, oid2, sizeof *oid1);
}

/**
 * bson_oid_hash_unsafe:
 * @oid: A bson_oid_t.
 *
 * This function performs a DJB style hash upon the bytes contained in @oid.
 * The result is a hash key suitable for use in a hashtable.
 *
 * This function is meant to be as fast as possible and therefore performs no
 * validation of arguments. The caller is responsible to ensure they are
 * passing valid arguments.
 *
 * Returns: A bson_uint32_t containing a hash code.
 */
static BSON_INLINE bson_uint32_t
bson_oid_hash_unsafe (const bson_oid_t *oid)
{
   bson_uint32_t hash = 5381;
   int i;

   for (i = 0; i < sizeof oid->bytes; i++) {
      hash = ((hash << 5) + hash) + oid->bytes[i];
   }

   return hash;
}


/**
 * bson_oid_copy_unsafe:
 * @src: A bson_oid_t to copy from.
 * @dst: A bson_oid_t to copy into.
 *
 * Copies the contents of @src into @dst. This function is meant to be as
 * fast as possible and therefore performs no argument checking. It is the
 * callers responsibility to ensure they are passing valid data into the
 * function.
 */
static BSON_INLINE void
bson_oid_copy_unsafe (const bson_oid_t *src,
                      bson_oid_t       *dst)
{
   memcpy(dst, src, sizeof *src);
}


/**
 * bson_parse_hex_char:
 * @hex: A character to parse to its integer value.
 *
 * This function contains a jump table to return the integer value for a
 * character containing a hexidecimal value (0-9, a-f, A-F). If the character
 * is not a hexidecimal character then zero is returned.
 *
 * Returns: An integer between 0 and 15.
 */
static BSON_INLINE bson_uint8_t
bson_parse_hex_char (char hex)
{
   switch (hex) {
   case '0': return 0;
   case '1': return 1;
   case '2': return 2;
   case '3': return 3;
   case '4': return 4;
   case '5': return 5;
   case '6': return 6;
   case '7': return 7;
   case '8': return 8;
   case '9': return 9;
   case 'a': case 'A': return 0xa;
   case 'b': case 'B': return 0xb;
   case 'c': case 'C': return 0xc;
   case 'd': case 'D': return 0xd;
   case 'e': case 'E': return 0xe;
   case 'f': case 'F': return 0xf;
   default: return 0;
   }
}


/**
 * bson_oid_init_from_string_unsafe:
 * @oid: A bson_oid_t to store the result.
 * @str: A 24-character hexidecimal encoded string.
 *
 * Parses a string containing 24 hexidecimal encoded bytes into a bson_oid_t.
 * This function is meant to be as fast as possible and inlined into your
 * code. For that purpose, the function does not perform any sort of bounds
 * checking and it is the callers responsibility to ensure they are passing
 * valid input to the function.
 */
static BSON_INLINE void
bson_oid_init_from_string_unsafe (bson_oid_t *oid,
                                  const char *str)
{
   int i;

   for (i = 0; i < 12; i++) {
      oid->bytes[i] = ((bson_parse_hex_char(str[2*i]) << 4) |
                       (bson_parse_hex_char(str[2*i+1])));
   }
}


/**
 * bson_oid_get_time_t_unsafe:
 * @oid: A bson_oid_t.
 *
 * Fetches the time @oid was generated.
 *
 * Returns: A time_t containing the UNIX timestamp of generation.
 */
static BSON_INLINE time_t
bson_oid_get_time_t_unsafe (const bson_oid_t *oid)
{
   bson_uint32_t t;
   memcpy(&t, oid, 4);
   return BSON_UINT32_FROM_BE(t);
}


/**
 * bson_reader_init_from_fd:
 * @reader: A bson_reader_t
 * @fd: A file-descriptor to read from.
 * @close_fd: If the file-descriptor should be closed when done.
 *
 * Initializes new bson_reader_t that will read BSON documents into bson_t
 * structures from an underlying file-descriptor.
 *
 * If you would like the reader to call close() on @fd in
 * bson_reader_destroy(), then specify TRUE for close_fd.
 */
void
bson_reader_init_from_fd (bson_reader_t *reader,
                          int            fd,
                          bson_bool_t    close_fd);


/**
 * bson_reader_t:
 * @reader: A bson_reader_t.
 * @data: A buffer to read BSON documents from.
 * @length: The length of @data.
 *
 * Initializes bson_reader_t that will read BSON documents from a memory
 * buffer.
 */
void
bson_reader_init_from_data (bson_reader_t      *reader,
                            const bson_uint8_t *data,
                            size_t              length);



/**
 * bson_reader_destroy:
 * @reader: An initialized bson_reader_t.
 *
 * Releases resources that were allocated during the use of a bson_reader_t.
 * This should be called after you have finished using the structure.
 */
void
bson_reader_destroy (bson_reader_t *reader);


/**
 * bson_reader_read:
 * @reader: A bson_reader_t.
 *
 * Reads the next bson_t in the underlying memory or storage.
 * The resulting bson_t should not be modified or freed. You may
 * copy it and iterate over it. Functions that take a
 * const bson_t* are safe to use.
 *
 * This structure does not survive calls to bson_reader_next() or
 * bson_reader_destroy() as it uses memory allocated by the reader
 * or underlying storage/memory.
 *
 * Returns: A const bson_t that should not be modified or freed.
 */
const bson_t *
bson_reader_read (bson_reader_t *reader);


BSON_END_DECLS


#endif /* BSON_H */
