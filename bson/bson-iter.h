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

#ifndef BSON_ITER_H
#define BSON_ITER_H

#include "bson.h"
#include "bson-endian.h"
#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


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
   return BSON_UINT32_FROM_LE(val) - 1;
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
 * bson_iter_date_time:
 * @iter: A bson_iter_t.
 *
 * Fetches the number of milliseconds elapsed since the UNIX epoch. This value
 * can be negative as times before 1970 are valid.
 *
 * Returns: A signed 64-bit integer containing the number of milliseconds.
 */
bson_int64_t
bson_iter_date_time (const bson_iter_t *iter);


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
   return (time_t)(bson_iter_int64_unsafe(iter) / 1000UL);
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
 *
 * Returns: TRUE if the visitor was pre-maturely ended; otherwise FALSE.
 */
bson_bool_t
bson_iter_visit_all (bson_iter_t          *iter,
                     const bson_visitor_t *visitor,
                     void                 *data);


BSON_END_DECLS


#endif /* BSON_ITER_H */
