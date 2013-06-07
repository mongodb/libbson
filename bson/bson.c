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


#include <inttypes.h>
#include <stdarg.h>

#include "b64_ntop.h"
#include "bson.h"
#include "bson-private.h"


typedef struct
{
   bson_validate_flags_t flags;
   ssize_t err_offset;
} bson_validate_state_t;


typedef struct
{
   bson_uint32_t  count;
   bson_bool_t    keys;
   bson_string_t *str;
} bson_json_state_t;


static bson_bool_t
bson_as_json_visit_array (const bson_iter_t *iter,
                          const char        *key,
                          const bson_t      *v_array,
                          void              *data);


static bson_bool_t
bson_as_json_visit_document (const bson_iter_t *iter,
                             const char        *key,
                             const bson_t      *v_document,
                             void              *data);


static const bson_uint8_t gZero;


/**
 * npow2:
 * @v: A 32-bit unsigned integer of requested bytes.
 *
 * Determines the next larger power of two for the value of @v.
 *
 * Returns: The next power of 2 from @v.
 */
static BSON_INLINE bson_uint32_t
npow2 (bson_uint32_t v)
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


static bson_bool_t
bson_impl_inline_grow (bson_impl_inline_t *impl,
                       bson_uint32_t       size)
{
   bson_impl_alloc_t *alloc = (bson_impl_alloc_t *)impl;
   bson_uint8_t *data;
   size_t req;

   BSON_ASSERT(impl);
   BSON_ASSERT(!(impl->flags & BSON_FLAG_RDONLY));

   if ((impl->len + size) <= sizeof impl->data) {
      return TRUE;
   }

   if ((req = npow2(impl->len + size)) <= INT32_MAX) {
      data = bson_malloc0(req);
      memcpy(data, impl->data, impl->len);
      alloc->flags &= ~BSON_FLAG_INLINE;
      alloc->parent = NULL;
      alloc->buf = &alloc->alloc;
      alloc->buflen = &alloc->alloclen;
      alloc->alloc = data;
      alloc->alloclen = req;
      alloc->offset = 0;
      alloc->realloc = bson_realloc;
      return TRUE;
   }

   return FALSE;
}


static bson_bool_t
bson_impl_alloc_grow (bson_impl_alloc_t *impl,
                      bson_uint32_t      size)
{
   bson_impl_alloc_t *tmp = impl;
   size_t req;

   BSON_ASSERT(impl);

   /*
    * Determine how many bytes we need for this document in the buffer.
    */
   req = impl->offset + impl->len + size;

   /*
    * Add enough bytes for trainling byte of each parent document. This
    * could be optimized out with a "depth" parameter in the child
    * document.
    */
   while ((tmp->flags & BSON_FLAG_CHILD) &&
          (tmp = (bson_impl_alloc_t *)tmp->parent)) {
      req++;
   }

   if (req <= *impl->buflen) {
      return TRUE;
   }

   req = npow2(req);

   if (req <= INT32_MAX) {
      *impl->buf = impl->realloc(*impl->buf, req);
      *impl->buflen = req;
      return TRUE;
   }

   return FALSE;
}


static bson_bool_t
bson_grow (bson_t        *bson,
           bson_uint32_t  size)
{
   BSON_ASSERT(bson);
   BSON_ASSERT(!(bson->flags & BSON_FLAG_RDONLY));

   if ((bson->flags & BSON_FLAG_INLINE)) {
      return bson_impl_inline_grow((bson_impl_inline_t *)bson, size);
   } else {
      return bson_impl_alloc_grow((bson_impl_alloc_t *)bson, size);
   }
}


static BSON_INLINE bson_uint8_t *
bson_data (const bson_t *bson)
{
   BSON_ASSERT(bson);

   if ((bson->flags & BSON_FLAG_INLINE)) {
      return ((bson_impl_inline_t *)bson)->data;
   } else {
      bson_impl_alloc_t *impl = (bson_impl_alloc_t *)bson;
      return (*impl->buf) + impl->offset;
   }
}


static size_t
bson_append_count_bytes (bson_t             *bson,
                         bson_uint32_t       n_pairs,
                         bson_uint32_t       first_len,
                         const bson_uint8_t *first_data,
                         va_list             args)
{
   size_t count = first_len;

   BSON_ASSERT(bson);
   BSON_ASSERT(n_pairs);
   BSON_ASSERT(first_len);
   BSON_ASSERT(first_data);

   while (--n_pairs) {
      count += va_arg(args, bson_uint32_t);
      va_arg(args, const bson_uint8_t *);
   }

   return count;
}


static BSON_INLINE void
bson_encode_length (bson_t *bson)
{
   bson_uint32_t length_le = BSON_UINT32_TO_LE(bson->len);
   memcpy(bson_data(bson), &length_le, 4);
}


/**
 * bson_append_va:
 * @bson: A bson_t
 * @n_bytes: The number of bytes to append to the document.
 * @n_pairs: The number of length,buffer pairs.
 * @first_len: Length of first buffer.
 * @first_data: First buffer.
 * @args: va_list of additional tuples.
 *
 * Appends the length,buffer pairs to the bson_t. @n_bytes is an optimization
 * to perform one array growth rather than many small growths.
 */
static void
bson_append_va (bson_t             *bson,
                bson_uint32_t       n_bytes,
                bson_uint32_t       n_pairs,
                bson_uint32_t       first_len,
                const bson_uint8_t *first_data,
                va_list             args)
{
   const bson_uint8_t *data;
   bson_uint32_t data_len;

   BSON_ASSERT(bson);
   BSON_ASSERT(!(bson->flags & BSON_FLAG_IN_CHILD));
   BSON_ASSERT(!(bson->flags & BSON_FLAG_RDONLY));
   BSON_ASSERT(n_pairs);
   BSON_ASSERT(first_len);
   BSON_ASSERT(first_data);

   bson_grow(bson, n_bytes);

   data = first_data;
   data_len = first_len;

   do {
      n_pairs--;
      memcpy(bson_data(bson) + bson->len - 1, data, data_len);
      bson->len += data_len;
      if (n_pairs) {
         data_len = va_arg(args, bson_uint32_t);
         data = va_arg(args, const bson_uint8_t *);
      }
   } while (n_pairs);

   bson_encode_length(bson);
   bson_data(bson)[bson->len - 1] = '\0';

   while ((bson->flags & BSON_FLAG_CHILD) &&
          (bson = ((bson_impl_alloc_t *)bson)->parent)) {
      bson->len += n_bytes;
      bson_data(bson)[bson->len - 1] = '\0';
      bson_encode_length(bson);
   }
}


/**
 * bson_append:
 * @bson: A bson_t.
 * @n_pairs: Number of length,buffer pairs.
 * @first_len: Length of first buffer.
 * @first_data: First buffer.
 *
 * Variadic function to append length,buffer pairs to a bson_t. If the
 * append would cause the bson_t to overflow a 32-bit length, it will return
 * FALSE and no append will have occurred.
 *
 * Returns: TRUE if successful; otherwise FALSE.
 */
static bson_bool_t
bson_append (bson_t             *bson,
             bson_uint32_t       n_pairs,
             bson_uint32_t       first_len,
             const bson_uint8_t *first_data,
             ...)
{
   va_list args;
   size_t count;

   BSON_ASSERT(bson);
   BSON_ASSERT(n_pairs);
   BSON_ASSERT(first_len);
   BSON_ASSERT(first_data);

   va_start(args, first_data);
   count = bson_append_count_bytes(bson, n_pairs, first_len, first_data, args);
   va_end(args);

   /*
    * Check to see if this append would overflow 32-bit signed integer. I know
    * what you're thinking. BSON uses a signed 32-bit length field? Yeah. It
    * does.
    */
   if (BSON_UNLIKELY(count > (BSON_MAX_SIZE - bson->len))) {
      return FALSE;
   }

   va_start(args, first_data);
   bson_append_va(bson, count, n_pairs, first_len, first_data, args);
   va_end(args);

   return TRUE;
}


static bson_bool_t
bson_append_bson_begin (bson_t      *bson,
                        const char  *key,
                        int          key_length,
                        bson_type_t  child_type,
                        bson_t      *child)
{
   const bson_uint8_t type = child_type;
   const bson_uint8_t empty[5] = { 5 };
   bson_impl_alloc_t *aparent = (bson_impl_alloc_t *)bson;
   bson_impl_alloc_t *achild = (bson_impl_alloc_t *)child;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(!(bson->flags & BSON_FLAG_RDONLY), FALSE);
   bson_return_val_if_fail(!(bson->flags & BSON_FLAG_IN_CHILD), FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(child, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   /*
    * If the parent is an inline bson_t, then we need to convert
    * it to a heap allocated buffer. This makes extending buffers
    * of child bson documents much simpler logic, as they can just
    * realloc the *buf pointer.
    */
   if ((bson->flags & BSON_FLAG_INLINE)) {
      BSON_ASSERT(bson->len <= 120);
      bson_grow(bson, 128 - bson->len);
      BSON_ASSERT(!(bson->flags & BSON_FLAG_INLINE));
   }

   /*
    * Append the type and key for the field.
    */
   if (!bson_append(bson, 4,
                    1, &type,
                    key_length, key,
                    1, &gZero,
                    5, empty)) {
      return FALSE;
   }

   /*
    * Mark the document as working on a child document so that no
    * further modifications can happen until the caller has called
    * bson_append_{document,array}_end().
    */
   bson->flags |= BSON_FLAG_IN_CHILD;

   /*
    * Initialize the child bson_t structure and point it at the parents
    * buffers. This allows us to realloc directly from the child without
    * walking up to the parent bson_t.
    */
   memset(child, 0, sizeof *child);
   achild->flags = (BSON_FLAG_CHILD |
                    BSON_FLAG_NO_FREE |
                    BSON_FLAG_STATIC);
   achild->parent = bson;
   achild->buf = aparent->buf;
   achild->buflen = aparent->buflen;
   achild->offset = aparent->offset + aparent->len - 1 - 5;
   achild->len = 5;
   achild->alloc = NULL;
   achild->alloclen = 0;
   achild->realloc = aparent->realloc;

   return TRUE;
}


bson_bool_t
bson_append_bson_end (bson_t *bson,
                      bson_t *child)
{
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail((bson->flags & BSON_FLAG_IN_CHILD), FALSE);
   bson_return_val_if_fail(!(child->flags & BSON_FLAG_IN_CHILD), FALSE);

   bson->flags &= ~BSON_FLAG_IN_CHILD;

   return TRUE;
}


bson_bool_t
bson_append_array_begin (bson_t     *bson,
                         const char *key,
                         int         key_length,
                         bson_t     *child)
{
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(child, FALSE);

   return bson_append_bson_begin(bson, key, key_length, BSON_TYPE_ARRAY,
                                 child);
}


bson_bool_t
bson_append_array_end (bson_t *bson,
                       bson_t *child)
{
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(child, FALSE);

   return bson_append_bson_end(bson, child);
}


bson_bool_t
bson_append_document_begin (bson_t     *bson,
                            const char *key,
                            int         key_length,
                            bson_t     *child)
{
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(child, FALSE);

   return bson_append_bson_begin(bson, key, key_length, BSON_TYPE_DOCUMENT,
                                 child);
}


bson_bool_t
bson_append_document_end (bson_t *bson,
                          bson_t *child)
{
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(child, FALSE);

   return bson_append_bson_end(bson, child);
}


bson_bool_t
bson_append_array (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   const bson_t *array)
{
   static const bson_uint8_t type = BSON_TYPE_ARRAY;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(array, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      array->len, bson_data(array));
}


bson_bool_t
bson_append_binary (bson_t             *bson,
                    const char         *key,
                    int                 key_length,
                    bson_subtype_t      subtype,
                    const bson_uint8_t *binary,
                    bson_uint32_t       length)
{
   static const bson_uint8_t type = BSON_TYPE_BINARY;
   bson_uint32_t length_le;
   bson_uint8_t subtype8;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(binary, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length_le = BSON_UINT32_TO_LE(length);
   subtype8 = subtype;

   return bson_append(bson, 6,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &length_le,
                      1, &subtype8,
                      length, binary);
}


bson_bool_t
bson_append_bool (bson_t      *bson,
                  const char  *key,
                  int          key_length,
                  bson_bool_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_BOOL;
   bson_uint8_t byte = !!value;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      1, &byte);
}


bson_bool_t
bson_append_code (bson_t     *bson,
                  const char *key,
                  int         key_length,
                  const char *javascript)
{
   static const bson_uint8_t type = BSON_TYPE_CODE;
   bson_uint32_t length;
   bson_uint32_t length_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(javascript, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length = strlen(javascript) + 1;
   length_le = BSON_UINT32_TO_LE(length);

   return bson_append(bson, 5,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &length_le,
                      length, javascript);
}


bson_bool_t
bson_append_code_with_scope (bson_t       *bson,
                             const char   *key,
                             int           key_length,
                             const char   *javascript,
                             const bson_t *scope)
{
   static const bson_uint8_t type = BSON_TYPE_CODEWSCOPE;
   bson_uint32_t codews_length_le;
   bson_uint32_t codews_length;
   bson_uint32_t js_length_le;
   bson_uint32_t js_length;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(javascript, FALSE);

   if (bson_empty0(scope)) {
      return bson_append_code(bson, key, key_length, javascript);
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   js_length = strlen(javascript) + 1;
   js_length_le = BSON_UINT32_TO_LE(js_length);

   codews_length = 4 + 4 + js_length + scope->len;
   codews_length_le = BSON_UINT32_TO_LE(codews_length);

   return bson_append(bson, 7,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &codews_length_le,
                      4, &js_length_le,
                      js_length, javascript,
                      scope->len, bson_data(scope));
}


bson_bool_t
bson_append_dbpointer (bson_t           *bson,
                       const char       *key,
                       int               key_length,
                       const char       *collection,
                       const bson_oid_t *oid)
{
   static const bson_uint8_t type = BSON_TYPE_DBPOINTER;
   bson_uint32_t length;
   bson_uint32_t length_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(collection, FALSE);
   bson_return_val_if_fail(oid, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length = strlen(collection) + 1;
   length_le = BSON_UINT32_TO_LE(length);

   return bson_append(bson, 6,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &length_le,
                      length, collection,
                      12, oid);
}


bson_bool_t
bson_append_document (bson_t       *bson,
                      const char   *key,
                      int           key_length,
                      const bson_t *value)
{
   static const bson_uint8_t type = BSON_TYPE_DOCUMENT;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(value, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      value->len, bson_data(value));
}


bson_bool_t
bson_append_double (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    double      value)
{
   static const bson_uint8_t type = BSON_TYPE_DOUBLE;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      8, &value);
}


bson_bool_t
bson_append_int32 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int32_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_INT32;
   bson_uint32_t value_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value_le = BSON_UINT32_TO_LE(value);

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &value_le);
}


bson_bool_t
bson_append_int64 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int64_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_INT64;
   bson_uint64_t value_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value_le = BSON_UINT64_TO_LE(value);

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      8, &value_le);
}


bson_bool_t
bson_append_maxkey (bson_t     *bson,
                    const char *key,
                    int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_MAXKEY;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 3,
                      1, &type,
                      key_length, key,
                      1, &gZero);
}


bson_bool_t
bson_append_minkey (bson_t     *bson,
                    const char *key,
                    int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_MINKEY;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 3,
                      1, &type,
                      key_length, key,
                      1, &gZero);
}


bson_bool_t
bson_append_null (bson_t     *bson,
                  const char *key,
                  int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_NULL;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 3,
                      1, &type,
                      key_length, key,
                      1, &gZero);
}


bson_bool_t
bson_append_oid (bson_t           *bson,
                 const char       *key,
                 int               key_length,
                 const bson_oid_t *value)
{
   static const bson_uint8_t type = BSON_TYPE_OID;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(value, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      12, value);
}


bson_bool_t
bson_append_regex (bson_t     *bson,
                   const char *key,
                   int         key_length,
                   const char *regex,
                   const char *options)
{
   static const bson_uint8_t type = BSON_TYPE_REGEX;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (!regex) {
      regex = "";
   }

   if (!options) {
      options = "";
   }

   return bson_append(bson, 5,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      strlen(regex) + 1, regex,
                      strlen(options) + 1, options);
}


bson_bool_t
bson_append_utf8 (bson_t     *bson,
                  const char *key,
                  int         key_length,
                  const char *value,
                  int         length)
{
   static const bson_uint8_t zero = 0;
   static const bson_uint8_t type = BSON_TYPE_UTF8;
   bson_uint32_t length_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (!value) {
      return bson_append_null(bson, key, key_length);
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (length < 0) {
      length = strlen(value);
   }

   length_le = BSON_UINT32_TO_LE(length + 1);

   return bson_append(bson, 6,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &length_le,
                      length, value,
                      1, &zero);
}


bson_bool_t
bson_append_symbol (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    const char *value,
                    int         length)
{
   static const bson_uint8_t type = BSON_TYPE_SYMBOL;
   bson_uint32_t length_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (!value) {
      return bson_append_null(bson, key, key_length);
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (length < 0) {
      length = strlen(value);
   }

   length_le = BSON_UINT32_TO_LE(length + 1);

   return bson_append(bson, 6,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      4, &length_le,
                      length, value,
                      1, &gZero);
}


bson_bool_t
bson_append_time_t (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    time_t      value)
{
   struct timeval tv = { value, 0 };

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   return bson_append_timeval(bson, key, key_length, &tv);
}


bson_bool_t
bson_append_timestamp (bson_t        *bson,
                       const char    *key,
                       int            key_length,
                       bson_uint32_t  timestamp,
                       bson_uint32_t  increment)
{
   static const bson_uint8_t type = BSON_TYPE_TIMESTAMP;
   bson_uint64_t value;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value = ((((bson_uint64_t)timestamp) << 32) | ((bson_uint64_t)increment));
   value = BSON_UINT64_TO_LE(value);

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      8, &value);
}


bson_bool_t
bson_append_timeval (bson_t         *bson,
                     const char     *key,
                     int             key_length,
                     struct timeval *value)
{
   static const bson_uint8_t type = BSON_TYPE_DATE_TIME;
   bson_uint64_t unix_msec;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);
   bson_return_val_if_fail(value, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   unix_msec = BSON_UINT64_TO_LE((((bson_uint64_t)value->tv_sec) * 1000UL) +
                                 (value->tv_usec / 1000UL));

   return bson_append(bson, 4,
                      1, &type,
                      key_length, key,
                      1, &gZero,
                      8, &unix_msec);
}


bson_bool_t
bson_append_undefined (bson_t     *bson,
                       const char *key,
                       int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_UNDEFINED;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   return bson_append(bson, 3,
                      1, &type,
                      key_length, key,
                      1, &gZero);
}


void
bson_init (bson_t *bson)
{
   bson_impl_inline_t *impl = (bson_impl_inline_t *)bson;

   bson_return_if_fail(bson);

   memset(bson, 0, sizeof *bson);

   impl->flags = BSON_FLAG_INLINE | BSON_FLAG_STATIC;
   impl->len = 5;
   impl->data[0] = 5;
}


bson_bool_t
bson_init_static (bson_t             *bson,
                  const bson_uint8_t *data,
                  bson_uint32_t       length)
{
   bson_impl_alloc_t *impl = (bson_impl_alloc_t *)bson;
   bson_uint32_t len_le;

   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(data, FALSE);

   if (length < 5) {
      return FALSE;
   }

   memcpy(&len_le, data, 4);
   if (BSON_UINT32_FROM_LE(len_le) != length) {
      return FALSE;
   }

   memset(bson, 0, sizeof *bson);
   impl->flags = BSON_FLAG_STATIC | BSON_FLAG_RDONLY;
   impl->len = length;
   impl->buf = &impl->alloc;
   impl->buflen = &impl->alloclen;
   impl->alloc = (bson_uint8_t *)data;
   impl->alloclen = length;

   return TRUE;
}


bson_t *
bson_new (void)
{
   bson_impl_inline_t *impl;
   bson_t *bson;

   bson = bson_malloc0(sizeof *bson);

   impl = (bson_impl_inline_t *)bson;
   impl->flags = BSON_FLAG_INLINE;
   impl->len = 5;
   impl->data[0] = 5;

   return bson;
}


bson_t *
bson_sized_new (size_t size)
{
   bson_impl_alloc_t *impl_a;
   bson_impl_inline_t *impl_i;
   bson_t *b;

   bson_return_val_if_fail(size <= INT32_MAX, NULL);

   b = bson_malloc0(sizeof *b);
   impl_a = (bson_impl_alloc_t *)b;
   impl_i = (bson_impl_inline_t *)b;

   if (size <= sizeof impl_i->data) {
      bson_init(b);
      b->flags &= ~BSON_FLAG_STATIC;
   } else {
      impl_a->flags = 0;
      impl_a->len = 5;
      impl_a->buf = &impl_a->alloc;
      impl_a->buflen = &impl_a->alloclen;
      impl_a->alloclen = MAX(5, size);
      impl_a->alloc = bson_malloc0(impl_a->alloclen);
      impl_a->alloc[0] = 5;
      impl_a->realloc = bson_realloc;
   }

   return b;
}


bson_t *
bson_new_from_data (const bson_uint8_t *data,
                    size_t              length)
{
   bson_uint32_t len_le;
   bson_t *bson;

   bson_return_val_if_fail(data, NULL);

   if (length < 5) {
      return NULL;
   }

   memcpy(&len_le, data, 4);
   if (length != BSON_UINT32_FROM_LE(len_le)) {
      return NULL;
   }

   bson = bson_sized_new(length);
   memcpy(bson_data(bson), data, length);
   bson->len = length;

   return bson;
}


bson_t *
bson_copy (const bson_t *bson)
{
   const bson_uint8_t *data;

   bson_return_val_if_fail(bson, NULL);

   data = bson_data(bson);
   return bson_new_from_data(data, bson->len);
}


void
bson_destroy (bson_t *bson)
{
   const bson_uint32_t nofree = (BSON_FLAG_RDONLY |
                                 BSON_FLAG_INLINE |
                                 BSON_FLAG_NO_FREE);

   BSON_ASSERT(bson);

   if (!(bson->flags & nofree)) {
      bson_free(*((bson_impl_alloc_t *)bson)->buf);
   }
   if (!(bson->flags & BSON_FLAG_STATIC)) {
      bson_free(bson);
   }
}


const bson_uint8_t *
bson_get_data (const bson_t *bson)
{
   bson_return_val_if_fail(bson, NULL);
   return bson_data(bson);
}


bson_uint32_t
bson_count_keys (const bson_t *bson)
{
   bson_uint32_t count = 0;
   bson_iter_t iter;

   bson_return_val_if_fail(bson, 0);

   if (bson_iter_init(&iter, bson)) {
      while (bson_iter_next(&iter)) {
         count++;
      }
   }

   return count;
}


int
bson_compare (const bson_t *bson,
              const bson_t *other)
{
   bson_uint32_t len;
   int ret;

   if (bson->len != other->len) {
      len = MIN(bson->len, other->len);
      if (!(ret = memcmp(bson_data(bson), bson_data(other), len))) {
         ret = bson->len - other->len;
      }
   } else {
      ret = memcmp(bson_data(bson), bson_data(other), bson->len);
   }

   return ret;
}


bson_bool_t
bson_equal (const bson_t *bson,
            const bson_t *other)
{
   return !bson_compare(bson, other);
}


static bson_bool_t
bson_as_json_visit_utf8 (const bson_iter_t *iter,
                         const char        *key,
                         size_t             v_utf8_len,
                         const char        *v_utf8,
                         void              *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   escaped = bson_utf8_escape_for_json(v_utf8, v_utf8_len);
   bson_string_append(state->str, "\"");
   bson_string_append(state->str, escaped);
   bson_string_append(state->str, "\"");
   bson_free(escaped);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_int32 (const bson_iter_t *iter,
                          const char        *key,
                          bson_int32_t       v_int32,
                          void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%"PRId32, v_int32);
   bson_string_append(state->str, str);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_int64 (const bson_iter_t *iter,
                          const char        *key,
                          bson_int64_t       v_int64,
                          void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%"PRIi64, v_int64);
   bson_string_append(state->str, str);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_double (const bson_iter_t *iter,
                           const char        *key,
                           double             v_double,
                           void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%lf", v_double);
   bson_string_append(state->str, str);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_undefined (const bson_iter_t *iter,
                              const char        *key,
                              void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$undefined\" : true }");
   return FALSE;
}


static bson_bool_t
bson_as_json_visit_null (const bson_iter_t *iter,
                         const char        *key,
                         void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "null");
   return FALSE;
}


static bson_bool_t
bson_as_json_visit_oid (const bson_iter_t *iter,
                        const char        *key,
                        const bson_oid_t  *oid,
                        void              *data)
{
   bson_json_state_t *state = data;
   char str[25];

   bson_oid_to_string(oid, str);
   bson_string_append(state->str, "{ \"$oid\" : \"");
   bson_string_append(state->str, str);
   bson_string_append(state->str, "\" }");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_binary (const bson_iter_t  *iter,
                           const char         *key,
                           bson_subtype_t      v_subtype,
                           size_t              v_binary_len,
                           const bson_uint8_t *v_binary,
                           void               *data)
{
   bson_json_state_t *state = data;
   size_t b64_len;
   char *b64;
   char str[3];

   b64_len = (v_binary_len / 3 + 1) * 4 + 1;
   b64 = bson_malloc0(b64_len);
   b64_ntop(v_binary, v_binary_len, b64, b64_len);

   bson_string_append(state->str, "{ \"$type\" : \"");
   snprintf(str, sizeof str, "%02x", v_subtype);
   bson_string_append(state->str, str);
   bson_string_append(state->str, "\", \"$binary\" : \"");
   bson_string_append(state->str, b64);
   bson_string_append(state->str, "\" }");
   bson_free(b64);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_bool (const bson_iter_t *iter,
                         const char        *key,
                         bson_bool_t        v_bool,
                         void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, v_bool ? "true" : "false");
   return FALSE;
}


static bson_bool_t
bson_as_json_visit_date_time (const bson_iter_t *iter,
                              const char        *key,
                              bson_int64_t       msec_since_epoch,
                              void              *data)
{
   bson_json_state_t *state = data;
   char secstr[32];

   snprintf(secstr, sizeof secstr, "%"PRIi64, msec_since_epoch);

   bson_string_append(state->str, "{ \"$date\" : ");
   bson_string_append(state->str, secstr);
   bson_string_append(state->str, " }");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_regex (const bson_iter_t *iter,
                          const char        *key,
                          const char        *v_regex,
                          const char        *v_options,
                          void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "{ \"$regex\" : \"");
   bson_string_append(state->str, v_regex);
   bson_string_append(state->str, "\", \"$options\" : \"");
   bson_string_append(state->str, v_options);
   bson_string_append(state->str, "\" }");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_timestamp (const bson_iter_t *iter,
                              const char        *key,
                              bson_uint32_t      v_timestamp,
                              bson_uint32_t      v_increment,
                              void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   bson_string_append(state->str, "{ \"$timestamp\" : { \"t\": ");
   snprintf(str, sizeof str, "%u", v_timestamp);
   bson_string_append(state->str, str);
   bson_string_append(state->str, ", \"i\": ");
   snprintf(str, sizeof str, "%u", v_increment);
   bson_string_append(state->str, str);
   bson_string_append(state->str, " } }");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_dbpointer (const bson_iter_t *iter,
                              const char        *key,
                              size_t             v_collection_len,
                              const char        *v_collection,
                              const bson_oid_t  *v_oid,
                              void              *data)
{
   bson_json_state_t *state = data;
   char str[25];

   bson_oid_to_string(v_oid, str);
   bson_string_append(state->str, "{ \"$ref\" : \"");
   bson_string_append(state->str, v_collection);
   bson_string_append(state->str, "\", \"$id\" : \"");
   bson_string_append(state->str, str);
   bson_string_append(state->str, "\" }");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_minkey (const bson_iter_t *iter,
                           const char        *key,
                           void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$minKey\" : 1 }");
   return FALSE;
}


static bson_bool_t
bson_as_json_visit_maxkey (const bson_iter_t *iter,
                           const char        *key,
                           void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$maxKey\" : 1 }");
   return FALSE;
}




static bson_bool_t
bson_as_json_visit_before (const bson_iter_t *iter,
                           const char        *key,
                           void              *data)
{
   bson_json_state_t *state = data;
   char *escaped;

   if (state->count) {
      bson_string_append(state->str, ", ");
   }

   if (state->keys) {
      escaped = bson_utf8_escape_for_json(key, -1);
      bson_string_append(state->str, "\"");
      bson_string_append(state->str, escaped);
      bson_string_append(state->str, "\" : ");
      bson_free(escaped);
   }

   state->count++;

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_code (const bson_iter_t *iter,
                         const char        *key,
                         size_t             v_code_len,
                         const char        *v_code,
                         void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_code);
   bson_string_append(state->str, "\"");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_symbol (const bson_iter_t *iter,
                           const char        *key,
                           size_t             v_symbol_len,
                           const char        *v_symbol,
                           void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_symbol);
   bson_string_append(state->str, "\"");

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_codewscope (const bson_iter_t *iter,
                               const char        *key,
                               size_t             v_code_len,
                               const char        *v_code,
                               const bson_t      *v_scope,
                               void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_code);
   bson_string_append(state->str, "\"");

   return FALSE;
}


static const bson_visitor_t bson_as_json_visitors = {
   .visit_before     = bson_as_json_visit_before,
   .visit_double     = bson_as_json_visit_double,
   .visit_utf8       = bson_as_json_visit_utf8,
   .visit_document   = bson_as_json_visit_document,
   .visit_array      = bson_as_json_visit_array,
   .visit_binary     = bson_as_json_visit_binary,
   .visit_undefined  = bson_as_json_visit_undefined,
   .visit_oid        = bson_as_json_visit_oid,
   .visit_bool       = bson_as_json_visit_bool,
   .visit_date_time  = bson_as_json_visit_date_time,
   .visit_null       = bson_as_json_visit_null,
   .visit_regex      = bson_as_json_visit_regex,
   .visit_dbpointer  = bson_as_json_visit_dbpointer,
   .visit_code       = bson_as_json_visit_code,
   .visit_symbol     = bson_as_json_visit_symbol,
   .visit_codewscope = bson_as_json_visit_codewscope,
   .visit_int32      = bson_as_json_visit_int32,
   .visit_timestamp  = bson_as_json_visit_timestamp,
   .visit_int64      = bson_as_json_visit_int64,
   .visit_minkey     = bson_as_json_visit_minkey,
   .visit_maxkey     = bson_as_json_visit_maxkey,
};


static bson_bool_t
bson_as_json_visit_document (const bson_iter_t *iter,
                             const char        *key,
                             const bson_t      *v_document,
                             void              *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = { 0, TRUE };
   bson_iter_t child;

   bson_iter_init(&child, v_document);

   child_state.str = bson_string_new("{ ");
   bson_iter_visit_all(&child, &bson_as_json_visitors, &child_state);
   bson_string_append(child_state.str, " }");
   bson_string_append(state->str, child_state.str->str);
   bson_string_free(child_state.str, TRUE);

   return FALSE;
}


static bson_bool_t
bson_as_json_visit_array (const bson_iter_t *iter,
                          const char        *key,
                          const bson_t      *v_array,
                          void              *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = { 0, FALSE };
   bson_iter_t child;

   bson_iter_init(&child, v_array);

   child_state.str = bson_string_new("[ ");
   bson_iter_visit_all(&child, &bson_as_json_visitors, &child_state);
   bson_string_append(child_state.str, " ]");
   bson_string_append(state->str, child_state.str->str);
   bson_string_free(child_state.str, TRUE);

   return FALSE;
}


char *
bson_as_json (const bson_t *bson,
              size_t       *length)
{
   bson_json_state_t state;
   bson_iter_t iter;

   bson_return_val_if_fail(bson, NULL);

   if (bson_empty0(bson)) {
      if (length) {
         *length = 2;
      }
      return strdup("{}");
   }

   if (!bson_iter_init(&iter, bson)) {
      return NULL;
   }

   state.count = 0;
   state.keys = TRUE;
   state.str = bson_string_new("{ ");
   bson_iter_visit_all(&iter, &bson_as_json_visitors, &state);
   bson_string_append(state.str, " }");

   if (length) {
      *length = state.str->len - 1;
   }

   return bson_string_free(state.str, FALSE);
}


static bson_bool_t
bson_iter_validate_utf8 (const bson_iter_t *iter,
                         const char        *key,
                         size_t             v_utf8_len,
                         const char        *v_utf8,
                         void              *data)
{
   bson_validate_state_t *state = data;
   bson_bool_t allow_null;

   if ((state->flags & BSON_VALIDATE_UTF8)) {
      allow_null = !!(state->flags & BSON_VALIDATE_UTF8_ALLOW_NULL);
      if (!bson_utf8_validate(v_utf8, v_utf8_len, allow_null)) {
         state->err_offset = iter->offset;
         return TRUE;
      }
   }

   return FALSE;
}


static void
bson_iter_validate_corrupt (const bson_iter_t *iter,
                            void              *data)
{
   bson_validate_state_t *state = data;
   state->err_offset = iter->err_offset;
}


static bson_bool_t
bson_iter_validate_before (const bson_iter_t *iter,
                           const char        *key,
                           void              *data)
{
   bson_validate_state_t *state = data;

   if ((state->flags & BSON_VALIDATE_DOLLAR_KEYS)) {
      if (key[0] == '$') {
         state->err_offset = iter->offset;
         return TRUE;
      }
   }

   if ((state->flags & BSON_VALIDATE_DOT_KEYS)) {
      if (strstr(key, ".")) {
         state->err_offset = iter->offset;
         return TRUE;
      }
   }

   return FALSE;
}


static bson_bool_t
bson_iter_validate_document (const bson_iter_t *iter,
                             const char        *key,
                             const bson_t      *v_document,
                             void              *data);


static const bson_visitor_t bson_validate_funcs = {
   .visit_before = bson_iter_validate_before,
   .visit_corrupt = bson_iter_validate_corrupt,
   .visit_utf8 = bson_iter_validate_utf8,
   .visit_document = bson_iter_validate_document,
   .visit_array = bson_iter_validate_document,
};


static bson_bool_t
bson_iter_validate_document (const bson_iter_t *iter,
                             const char        *key,
                             const bson_t      *v_document,
                             void              *data)
{
   bson_validate_state_t *state = data;
   bson_iter_t child;

   if (!bson_iter_init(&child, v_document)) {
      state->err_offset = iter->offset;
      return TRUE;
   }

   bson_iter_visit_all(&child, &bson_validate_funcs, state);

   return FALSE;
}


bson_bool_t
bson_validate (const bson_t          *bson,
               bson_validate_flags_t  flags,
               size_t                *offset)
{
   bson_validate_state_t state = { flags, -1 };
   bson_iter_t iter;

   if (!bson_iter_init(&iter, bson)) {
      state.err_offset = 0;
      goto failure;
   }

   bson_iter_validate_document(&iter, NULL, bson, &state);

failure:
   if (offset) {
      *offset = state.err_offset;
   }

   return (state.err_offset < 0);
}
