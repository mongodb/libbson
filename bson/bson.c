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


#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <unistr.h>

#include "b64_ntop.h"

#include "bson.h"
#include "bson-md5.h"
#include "bson-memory.h"
#include "bson-string.h"
#include "bson-thread.h"


/*
 * TODO:
 *
 *   - Put some consideration into if we want to handle OOM. It is a really
 *     difficult thing to do correctly. Almost nobody gets it right. D-BUS
 *     on GNU/Linux might be one of the few things that does.
 *
 *   - Determine how we want to handle outrageously sized bson_t.
 */


static const bson_uint8_t gZero;


typedef enum
{
   BSON_READER_FD = 1,
   BSON_READER_DATA = 2,
} bson_reader_type_t;


typedef struct
{
   bson_reader_type_t  type;
   int                 fd;
   bson_bool_t         close_fd : 1;
   bson_bool_t         done : 1;
   size_t              end;
   size_t              len;
   size_t              offset;
   bson_t              inline_bson;
   bson_uint8_t       *data;
} bson_reader_fd_t;


typedef struct
{
   bson_reader_type_t  type;
   const bson_uint8_t *data;
   size_t              length;
   size_t              offset;
   bson_t              inline_bson;
} bson_reader_data_t;


typedef struct
{
   bson_uint32_t  count;
   bson_bool_t    keys;
   bson_string_t *str;
} bson_json_state_t;


BSON_STATIC_ASSERT(sizeof(bson_reader_fd_t) < sizeof(bson_reader_t));
BSON_STATIC_ASSERT(sizeof(bson_reader_data_t) < sizeof(bson_reader_t));


/**
 * bson_encode_length:
 * @bson: A bson_t.
 *
 * Encodes the length of the bson into the first four bytes of @bson.
 * This should be called any time you add a field. This used to be done
 * in a bson_finish() style call, but instead we just do it every time we
 * add a field internally.
 */
static BSON_INLINE void
bson_encode_length (bson_t *bson)
{
   bson_uint32_t len = BSON_UINT32_TO_LE(bson->len);
   memcpy(bson->data, &len, 4);
}


/**
 * bson_grow_if_needed:
 * @bson: A bson_t to grow.
 * @additional_bytes: Number of additional bytes needed.
 *
 * Will check to see if there are enough bytes allocated for @additional_bytes
 * to be used. If not, it will grow the size of the bson by a power of two of
 * the current allocation size.
 *
 * Returns: @bson or a new memory location if the buffer was grown.
 */
static void
bson_grow_if_needed (bson_t *bson,
                     size_t  additional_bytes)
{
   size_t amin;
   size_t asize;

   bson_return_if_fail(bson);
   bson_return_if_fail(additional_bytes < INT_MAX);

   amin = bson->len + additional_bytes;

   if (amin <= sizeof bson->inlbuf) {
      return;
   }

   asize = 32;

   while (asize < amin) {
      asize <<= 1;
   }

   if (BSON_UNLIKELY(asize >= INT_MAX)) {
      abort();
   }

   if (bson->allocated) {
      bson->data = bson_realloc(bson->data, asize);
      bson->allocated = asize;
   } else {
      bson->data = bson_malloc0(asize);
      bson->allocated = asize;
      memcpy(bson->data, bson->inlbuf, bson->len);
   }
}


bson_t *
bson_new (void)
{
   bson_t *b;

   b = bson_malloc0(sizeof *b);
   b->allocated = 0;
   b->len = 5;
   b->data = &b->inlbuf[0];
   b->data[0] = 5;

   return b;
}


bson_t *
bson_new_from_data (const bson_uint8_t *data,
                    size_t              length)
{
   bson_uint32_t len_le;
   bson_t *b;

   bson_return_val_if_fail(data, NULL);
   bson_return_val_if_fail(length >= 5, NULL);
   bson_return_val_if_fail(length < INT_MAX, NULL);

   if (length < 5) {
      return NULL;
   }

   memcpy(&len_le, data, 4);
   if (BSON_UINT32_FROM_LE(len_le) != length) {
      return NULL;
   }

   b = bson_new();
   bson_grow_if_needed(b, length - b->len);
   memcpy(b->data, data, length);
   b->len = length;

   return b;
}


bson_t *
bson_sized_new (size_t size)
{
   bson_t *b;

   bson_return_val_if_fail(size >= 5, NULL);
   bson_return_val_if_fail(size < INT_MAX, NULL);

   b = bson_new();
   bson_grow_if_needed(b, size - b->len);

   return b;
}


void
bson_destroy (bson_t *bson)
{
   if (bson) {
      if (bson->allocated > 0) {
         bson_free(bson->data);
      }
      bson_free(bson);
   }
}


typedef struct
{
   bson_validate_flags_t flags;
   ssize_t err_offset;
} bson_validate_state_t;


static void
bson_iter_validate_utf8 (const bson_iter_t *iter,
                         const char        *key,
                         size_t             v_utf8_len,
                         const char        *v_utf8,
                         void              *data)
{
   bson_validate_state_t *state = data;

   if ((state->flags & BSON_VALIDATE_UTF8)) {
      if (!u8_check((const bson_uint8_t *)v_utf8, v_utf8_len)) {
         state->err_offset = iter->offset;
      }
   }
}


static void
bson_iter_validate_corrupt (const bson_iter_t *iter,
                            void              *data)
{
   bson_validate_state_t *state = data;
   state->err_offset = iter->err_offset;
}


static void
bson_iter_validate_before (const bson_iter_t *iter,
                           const char        *key,
                           void              *data)
{
   bson_validate_state_t *state = data;

   if ((state->flags & BSON_VALIDATE_DOLLAR_KEYS)) {
      if (key[0] == '$') {
         state->err_offset = iter->offset;
      }
   }

   if ((state->flags & BSON_VALIDATE_DOT_KEYS)) {
      if (strstr(key, ".")) {
         state->err_offset = iter->offset;
      }
   }
}


static void
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


static void
bson_iter_validate_document (const bson_iter_t *iter,
                             const char        *key,
                             const bson_t      *v_document,
                             void              *data)
{
   bson_validate_state_t *state = data;
   bson_iter_t child;

   if (!bson_iter_init(&child, v_document)) {
      /*
       * TODO: We should make it so we can abort future iteration
       *       on the parent document by returning FALSE/TRUE/etc.
       */
      state->err_offset = iter->offset;
      return;
   }

   bson_iter_visit_all(&child, &bson_validate_funcs, state);
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


static void
bson_append_va (bson_t             *bson,
                bson_uint32_t       n_params,
                bson_uint32_t       first_length,
                const bson_uint8_t *first_data,
                va_list             args)
{
   const bson_uint8_t *data = first_data;
   bson_uint32_t length = first_length;
   bson_int32_t todo = n_params;

   if (bson->allocated < 0) {
      fprintf(stderr, "Cannot append to read-only BSON.\n");
      return;
   }

   do {
      bson_grow_if_needed(bson, length);
      memcpy(&bson->data[bson->len-1], data, length);
      bson->len += length;
      if ((--todo > 0)) {
         length = va_arg(args, bson_uint32_t);
         data = va_arg(args, bson_uint8_t*);
      }
   } while (todo > 0);

   bson->data[bson->len-1] = 0;
   bson_encode_length(bson);
}


static void
bson_append (bson_t             *bson,
             bson_uint32_t       n_params,
             bson_uint32_t       first_length,
             const bson_uint8_t *first_data,
             ...)
{
   va_list args;

   va_start(args, first_data);
   bson_append_va(bson, n_params, first_length, first_data, args);
   va_end(args);
}


void
bson_append_array (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   const bson_t *array)
{
   static const bson_uint8_t type = BSON_TYPE_ARRAY;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(array);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               array->len, array->data);
}

void
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

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(binary);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length_le = BSON_UINT32_TO_LE(length);
   subtype8 = subtype;

   bson_append(bson, 6,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &length_le,
               1, &subtype8,
               length, binary);
}


void
bson_append_bool (bson_t      *bson,
                  const char  *key,
                  int          key_length,
                  bson_bool_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_BOOL;
   bson_uint8_t byte = value;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               1, &byte);
}


void
bson_append_code (bson_t     *bson,
                  const char *key,
                  int         key_length,
                  const char *javascript)
{
   static const bson_uint8_t type = BSON_TYPE_CODE;
   bson_uint32_t length;
   bson_uint32_t length_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(javascript);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length = strlen(javascript) + 1;
   length_le = BSON_UINT32_TO_LE(length);

   bson_append(bson, 5,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &length_le,
               length, javascript);
}


void
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

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(javascript);

   if (bson_empty0(scope)) {
      bson_append_code(bson, key, key_length, javascript);
      return;
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   js_length = strlen(javascript) + 1;
   js_length_le = BSON_UINT32_TO_LE(js_length);

   codews_length = 4 + 4 + js_length + scope->len;
   codews_length_le = BSON_UINT32_TO_LE(codews_length);

   bson_append(bson, 7,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &codews_length_le,
               4, &js_length_le,
               js_length, javascript,
               scope->len, scope->data);
}


void
bson_append_dbpointer (bson_t           *bson,
                       const char       *key,
                       int               key_length,
                       const char       *collection,
                       const bson_oid_t *oid)
{
   static const bson_uint8_t type = BSON_TYPE_DBPOINTER;
   bson_uint32_t length;
   bson_uint32_t length_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(collection);
   bson_return_if_fail(oid);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   length = strlen(collection) + 1;
   length_le = BSON_UINT32_TO_LE(length);

   bson_append(bson, 6,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &length_le,
               length, collection,
               12, oid);
}


void
bson_append_document (bson_t       *bson,
                      const char   *key,
                      int           key_length,
                      const bson_t *value)
{
   static const bson_uint8_t type = BSON_TYPE_DOCUMENT;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(value);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               value->len, value->data);
}


void
bson_append_double (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    double      value)
{
   static const bson_uint8_t type = BSON_TYPE_DOUBLE;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               8, &value);
}


void
bson_append_int32 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int32_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_INT32;
   bson_uint32_t value_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value_le = BSON_UINT32_TO_LE(value);

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &value_le);
}


void
bson_append_int64 (bson_t       *bson,
                   const char   *key,
                   int           key_length,
                   bson_int64_t  value)
{
   static const bson_uint8_t type = BSON_TYPE_INT64;
   bson_uint64_t value_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value_le = BSON_UINT64_TO_LE(value);

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               8, &value_le);
}


void
bson_append_maxkey (bson_t     *bson,
                    const char *key,
                    int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_MAXKEY;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 3,
               1, &type,
               key_length, key,
               1, &gZero);
}


void
bson_append_minkey (bson_t     *bson,
                    const char *key,
                    int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_MINKEY;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 3,
               1, &type,
               key_length, key,
               1, &gZero);
}


void
bson_append_null (bson_t     *bson,
                  const char *key,
                  int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_NULL;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 3,
               1, &type,
               key_length, key,
               1, &gZero);
}


void
bson_append_oid (bson_t           *bson,
                 const char       *key,
                 int               key_length,
                 const bson_oid_t *value)
{
   static const bson_uint8_t type = BSON_TYPE_OID;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(value);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               12, value);
}


void
bson_append_regex (bson_t     *bson,
                   const char *key,
                   int         key_length,
                   const char *regex,
                   const char *options)
{
   static const bson_uint8_t type = BSON_TYPE_REGEX;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (!regex) {
      regex = "";
   }

   if (!options) {
      options = "";
   }

   bson_append(bson, 5,
               1, &type,
               key_length, key,
               1, &gZero,
               strlen(regex) + 1, regex,
               strlen(options) + 1, options);
}


void
bson_append_string (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    const char *value,
                    int         length)
{
   static const bson_uint8_t zero = 0;
   static const bson_uint8_t type = BSON_TYPE_UTF8;
   bson_uint32_t length_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (!value) {
      bson_append_null(bson, key, key_length);
      return;
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (length < 0) {
      length = strlen(value);
   }

   length_le = BSON_UINT32_TO_LE(length + 1);
   bson_append(bson, 6,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &length_le,
               length, value,
               1, &zero);
}


void
bson_append_symbol (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    const char *value,
                    int         length)
{
   static const bson_uint8_t type = BSON_TYPE_SYMBOL;
   bson_uint32_t length_le;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (!value) {
      bson_append_null(bson, key, key_length);
      return;
   }

   if (key_length < 0) {
      key_length = strlen(key);
   }

   if (length < 0) {
      length = strlen(value);
   }

   length_le = BSON_UINT32_TO_LE(length + 1);
   bson_append(bson, 6,
               1, &type,
               key_length, key,
               1, &gZero,
               4, &length_le,
               length, value,
               1, &gZero);
}


void
bson_append_time_t (bson_t     *bson,
                    const char *key,
                    int         key_length,
                    time_t      value)
{
   struct timeval tv = { value, 0 };
   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_append_timeval(bson, key, key_length, &tv);
}


void
bson_append_timestamp (bson_t        *bson,
                       const char    *key,
                       int            key_length,
                       bson_uint32_t  timestamp,
                       bson_uint32_t  increment)
{
   static const bson_uint8_t type = BSON_TYPE_TIMESTAMP;
   bson_uint64_t value;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   value = ((((bson_uint64_t)timestamp) << 32) | ((bson_uint64_t)increment));
   value = BSON_UINT64_TO_LE(value);

   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               8, &value);
}


void
bson_append_timeval (bson_t         *bson,
                     const char     *key,
                     int             key_length,
                     struct timeval *value)
{
   static const bson_uint8_t type = BSON_TYPE_DATE_TIME;
   bson_uint64_t unix_msec;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);
   bson_return_if_fail(value);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   unix_msec = BSON_UINT64_TO_LE((((bson_uint64_t)value->tv_sec) * 1000UL) +
                                 (value->tv_usec / 1000UL));
   bson_append(bson, 4,
               1, &type,
               key_length, key,
               1, &gZero,
               8, &unix_msec);
}


void
bson_append_undefined (bson_t     *bson,
                       const char *key,
                       int         key_length)
{
   static const bson_uint8_t type = BSON_TYPE_UNDEFINED;

   bson_return_if_fail(bson);
   bson_return_if_fail(key);

   if (key_length < 0) {
      key_length = strlen(key);
   }

   bson_append(bson, 3,
               1, &type,
               key_length, key,
               1, &gZero);
}


int
bson_compare (const bson_t *bson,
              const bson_t *other)
{
   int cmp;

   bson_return_val_if_fail(bson, 0);
   bson_return_val_if_fail(other, 0);

   if (0 != (cmp = bson->len - other->len)) {
      return cmp;
   }

   return memcmp(bson->data, other->data, bson->len);
}


bson_bool_t
bson_equal (const bson_t *bson,
            const bson_t *other)
{
   return !bson_compare(bson, other);
}


bson_bool_t
bson_iter_init (bson_iter_t  *iter,
                const bson_t *bson)
{
   bson_return_val_if_fail(iter, FALSE);
   bson_return_val_if_fail(bson, FALSE);

   if (BSON_UNLIKELY(bson->len < 5)) {
      return FALSE;
   }

   memset(iter, 0, sizeof *iter);

   iter->bson = bson;
   iter->offset = 0;
   iter->next_offset = 4;

   return TRUE;
}


bson_bool_t
bson_iter_init_find (bson_iter_t  *iter,
                     const bson_t *bson,
                     const char   *key)
{
   bson_return_val_if_fail(iter, FALSE);
   bson_return_val_if_fail(bson, FALSE);
   bson_return_val_if_fail(key, FALSE);

   return bson_iter_init(iter, bson) && bson_iter_find(iter, key);
}


bson_bool_t
bson_iter_find (bson_iter_t *iter,
                const char  *key)
{
   bson_return_val_if_fail(iter, FALSE);
   bson_return_val_if_fail(key, FALSE);

   while (bson_iter_next(iter)) {
      if (!strcmp(key, bson_iter_key(iter))) {
         return TRUE;
      }
   }

   return FALSE;
}


const char *
bson_iter_key (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, NULL);
   return bson_iter_key_unsafe(iter);
}


bson_type_t
bson_iter_type (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   bson_return_val_if_fail(iter->type, 0);
   return bson_iter_type_unsafe(iter);
}


bson_bool_t
bson_iter_next (bson_iter_t *iter)
{
   bson_uint32_t o;
   const bson_t *b;

   b = iter->bson;

   iter->offset = iter->next_offset;
   iter->type = &b->data[iter->offset];
   iter->key = &b->data[iter->offset + 1];
   iter->data1 = NULL;
   iter->data2 = NULL;
   iter->data3 = NULL;
   iter->data4 = NULL;

   for (o = iter->offset + 1; o < b->len; o++) {
      if (!b->data[o]) {
         iter->data1 = &b->data[++o];
         goto fill_data_fields;
      }
   }

   return FALSE;

fill_data_fields:

   switch (*iter->type) {
   case BSON_TYPE_DATE_TIME:
   case BSON_TYPE_DOUBLE:
   case BSON_TYPE_INT64:
   case BSON_TYPE_TIMESTAMP:
      iter->next_offset = o + 8;
      break;
   case BSON_TYPE_CODE:
   case BSON_TYPE_SYMBOL:
   case BSON_TYPE_UTF8:
      {
         bson_uint32_t l;

         if ((o + 4) >= b->len) {
            iter->err_offset = o;
            return FALSE;
         }

         iter->data2 = &b->data[o + 4];
         memcpy(&l, iter->data1, 4);
         l = BSON_UINT32_FROM_LE(l);
         iter->next_offset = o + 4 + l;

         /*
          * Make sure the string length includes the NUL byte.
          */
         if (BSON_UNLIKELY((l < 1) || (iter->next_offset >= iter->bson->len))) {
            iter->err_offset = o;
            return FALSE;
         }

         /*
          * Make sure the last byte is a NUL byte.
          */
         if (BSON_UNLIKELY(iter->data2[l - 1] != '\0')) {
            iter->err_offset = o + 4 + l - 1;
            return FALSE;
         }
      }
      break;
   case BSON_TYPE_BINARY:
      {
         bson_uint32_t l;

         if ((o + 4) >= b->len) {
            iter->err_offset = o;
            return FALSE;
         }

         iter->data2 = &b->data[o + 4];
         iter->data3 = &b->data[o + 5];

         memcpy(&l, iter->data1, 4);
         l = BSON_UINT32_FROM_LE(l);
         iter->next_offset = o + 5 + l;
      }
      break;
   case BSON_TYPE_ARRAY:
   case BSON_TYPE_DOCUMENT:
      {
         bson_uint32_t l;

         if ((o + 4) >= b->len) {
            iter->err_offset = o;
            return FALSE;
         }

         memcpy(&l, iter->data1, 4);
         l = BSON_UINT32_FROM_LE(l);
         iter->next_offset = o + l;
      }
      break;
   case BSON_TYPE_OID:
      iter->next_offset = o + 12;
      break;
   case BSON_TYPE_BOOL:
      iter->next_offset = o + 1;
      break;
   case BSON_TYPE_REGEX:
      {
         bson_bool_t eor = FALSE;
         bson_bool_t eoo = FALSE;

         for (; o < b->len; o++) {
            if (!b->data[o]) {
               iter->data2 = &b->data[++o];
               eor = TRUE;
               break;
            }
         }

         if (!eor) {
            iter->err_offset = iter->next_offset;
            return FALSE;
         }

         for (; o < b->len; o++) {
            if (!b->data[o]) {
               eoo = TRUE;
               break;
            }
         }

         if (!eoo) {
            iter->err_offset = iter->next_offset;
            return FALSE;
         }

         iter->next_offset = o + 1;
      }
      break;
   case BSON_TYPE_DBPOINTER:
      {
         bson_uint32_t l;

         if ((o + 4) >= b->len) {
            iter->err_offset = o;
            return FALSE;
         }

         iter->data2 = &b->data[o + 4];
         memcpy(&l, iter->data1, 4);
         l = BSON_UINT32_FROM_LE(l);

         iter->data3 = &b->data[o + 4 + l];
         iter->next_offset = o + 4 + l + 12;
      }
      break;
   case BSON_TYPE_CODEWSCOPE:
      {
         bson_uint32_t l;
         bson_uint32_t doclen;

         if ((o + 8) >= b->len) {
            iter->err_offset = o;
            return FALSE;
         }

         iter->data2 = &b->data[o + 4];
         iter->data3 = &b->data[o + 8];

         memcpy(&l, iter->data1, 4);
         l = BSON_UINT32_FROM_LE(l);
         if (l < 14) {
            iter->err_offset = o;
            return FALSE;
         }

         iter->next_offset = o + l;
         if (iter->next_offset >= iter->bson->len) {
            iter->err_offset = o;
            return FALSE;
         }

         memcpy(&l, iter->data2, 4);
         l = BSON_UINT32_FROM_LE(l);
         if (BSON_UNLIKELY((o + 4 + 4 + l + 4) >= iter->next_offset)) {
            iter->err_offset = o + 4;
            return FALSE;
         }

         iter->data4 = &b->data[o + 4 + 4 + l];
         memcpy(&doclen, iter->data4, 4);
         doclen = BSON_UINT32_FROM_LE(doclen);
         if ((o + 4 + 4 + l + doclen) != iter->next_offset) {
            iter->err_offset = o + 4 + 4 + l;
            return FALSE;
         }
      }
      break;
   case BSON_TYPE_INT32:
      iter->next_offset = o + 4;
      break;
   case BSON_TYPE_MAXKEY:
   case BSON_TYPE_MINKEY:
   case BSON_TYPE_NULL:
   case BSON_TYPE_UNDEFINED:
      iter->data1 = NULL;
      iter->next_offset = o;
      break;
   case BSON_TYPE_EOD:
   default:
      iter->err_offset = o;
      return FALSE;
   }

   /*
    * Check to see if any of the field locations would overflow the
    * current BSON buffer. If so, set the error location to the offset
    * of where the field starts.
    */
   if (!(iter->next_offset < b->len)) {
      iter->err_offset = o;
      return FALSE;
   }

   iter->err_offset = 0;
   return TRUE;
}


void
bson_iter_binary (const bson_iter_t   *iter,
                  bson_subtype_t      *subtype,
                  bson_uint32_t       *binary_len,
                  const bson_uint8_t **binary)
{
   bson_return_if_fail(iter);
   bson_return_if_fail(!binary || binary_len);

   if (*iter->type == BSON_TYPE_BINARY) {
      if (subtype) {
         *subtype = (bson_subtype_t)*iter->data2;
      }
      if (binary) {
         memcpy(binary_len, iter->data1, 4);
         *binary_len = BSON_UINT32_FROM_LE(*binary_len);
         *binary = iter->data3;
      }
   }
}


bson_bool_t
bson_iter_bool (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   if (*iter->type == BSON_TYPE_BOOL) {
      return bson_iter_bool_unsafe(iter);
   }
   return 0;
}


double
bson_iter_double (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   if (*iter->type == BSON_TYPE_DOUBLE) {
      return bson_iter_double_unsafe(iter);
   }
   return 0;
}


bson_int32_t
bson_iter_int32 (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   if (*iter->type == BSON_TYPE_INT32) {
      return bson_iter_int32_unsafe(iter);
   }
   return 0;
}


bson_int64_t
bson_iter_int64 (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   if (*iter->type == BSON_TYPE_INT64) {
      return bson_iter_int64_unsafe(iter);
   }
   return 0;
}


const bson_oid_t *
bson_iter_oid (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, NULL);
   if (*iter->type == BSON_TYPE_OID)
      return bson_iter_oid_unsafe(iter);
   return NULL;
}


const char *
bson_iter_regex (const bson_iter_t  *iter,
                 const char        **options)
{
   const char *ret = NULL;
   const char *ret_options = NULL;

   bson_return_val_if_fail(iter, NULL);

   if (*iter->type == BSON_TYPE_REGEX) {
      ret = (const char *)iter->data1;
      ret_options = (const char *)iter->data2;
   }

   if (options) {
      *options = ret_options;
   }

   return ret;
}


const char *
bson_iter_string (const bson_iter_t *iter,
                  bson_uint32_t     *length)
{
   bson_return_val_if_fail(iter, NULL);

   if (*iter->type == BSON_TYPE_UTF8) {
      if (length) {
         *length = bson_iter_string_len_unsafe(iter);
      }
      return (const char *)iter->data2;
   }

   return NULL;
}


const char *
bson_iter_code (const bson_iter_t *iter,
                bson_uint32_t     *length)
{
   bson_return_val_if_fail(iter, NULL);

   if (*iter->type == BSON_TYPE_CODE) {
      if (length) {
         *length = bson_iter_string_len_unsafe(iter);
      }
      return (const char *)iter->data2;
   }

   return NULL;
}


const char *
bson_iter_codewscope (const bson_iter_t    *iter,
                      bson_uint32_t        *length,
                      bson_uint32_t        *scope_len,
                      const bson_uint8_t  **scope)
{
   bson_uint32_t len;

   bson_return_val_if_fail(iter, NULL);

   if (*iter->type == BSON_TYPE_CODEWSCOPE) {
      if (length) {
         memcpy(&len, iter->data2, 4);
         *length = BSON_UINT32_FROM_LE(len);
      }
      memcpy(&len, iter->data4, 4);
      *scope_len = BSON_UINT32_FROM_LE(len);
      *scope = iter->data4;
      return (const char *)iter->data3;
   }

   return NULL;
}


void
bson_iter_dbpointer (const bson_iter_t  *iter,
                     bson_uint32_t      *collection_len,
                     const char        **collection,
                     const bson_oid_t  **oid)
{
   bson_return_if_fail(iter);

   if (*iter->type == BSON_TYPE_DBPOINTER) {
      if (collection_len) {
         memcpy(collection_len, iter->data1, 4);
         *collection_len = BSON_UINT32_FROM_LE(*collection_len);
      }
      if (collection) {
         *collection = (const char *)iter->data2;
      }
      if (oid) {
         *oid = (const bson_oid_t *)iter->data3;
      }
   }
}


const char *
bson_iter_symbol (const bson_iter_t *iter,
                  bson_uint32_t     *length)
{
   const char *ret = NULL;
   bson_uint32_t ret_length = 0;

   bson_return_val_if_fail(iter, NULL);

   if (*iter->type == BSON_TYPE_SYMBOL) {
      ret = (const char *)iter->data2;
      ret_length = bson_iter_string_len_unsafe(iter);
   }

   if (length) {
      *length = ret_length;
   }

   return ret;
}


bson_int64_t
bson_iter_date_time (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);

   if (*iter->type == BSON_TYPE_DATE_TIME) {
      return bson_iter_int64_unsafe(iter);
   }

   return 0;
}


time_t
bson_iter_time_t (const bson_iter_t *iter)
{
   bson_return_val_if_fail(iter, 0);
   if (*iter->type == BSON_TYPE_DATE_TIME) {
      return bson_iter_time_t_unsafe(iter);
   }
   return 0;
}


void
bson_iter_timestamp (const bson_iter_t *iter,
                     bson_uint32_t     *timestamp,
                     bson_uint32_t     *increment)
{
   bson_uint64_t encoded;
   bson_uint32_t ret_timestamp = 0;
   bson_uint32_t ret_increment = 0;

   bson_return_if_fail(iter);

   if (*iter->type == BSON_TYPE_TIMESTAMP) {
      memcpy(&encoded, iter->data1, 8);
      encoded = BSON_UINT64_FROM_LE(encoded);
      ret_timestamp = (encoded >> 32) & 0xFFFFFFFF;
      ret_increment = encoded & 0xFFFFFFFF;
   }

   if (timestamp) {
      *timestamp = ret_timestamp;
   }

   if (increment) {
      *increment = ret_increment;
   }
}


void
bson_iter_timeval (const bson_iter_t *iter,
                   struct timeval    *tv)
{
   bson_return_if_fail(iter);
   if (*iter->type == BSON_TYPE_DATE_TIME) {
      bson_iter_timeval_unsafe(iter, tv);
      return;
   }
   memset(tv, 0, sizeof *tv);
}


void
bson_iter_document (const bson_iter_t   *iter,
                    bson_uint32_t       *document_len,
                    const bson_uint8_t **document)
{
   bson_return_if_fail(iter);
   bson_return_if_fail(document_len);
   bson_return_if_fail(document);

   if (*iter->type == BSON_TYPE_DOCUMENT) {
      memcpy(document_len, iter->data1, 4);
      *document_len = BSON_UINT32_FROM_LE(*document_len);
      *document = iter->data1;
   }
}


void
bson_iter_array (const bson_iter_t   *iter,
                 bson_uint32_t       *array_len,
                 const bson_uint8_t **array)
{
   bson_return_if_fail(iter);
   bson_return_if_fail(array_len);
   bson_return_if_fail(array);

   if (*iter->type == BSON_TYPE_ARRAY) {
      memcpy(array_len, iter->data1, 4);
      *array_len = BSON_UINT32_FROM_LE(*array_len);
      *array = iter->data1;
   }
}


void
bson_iter_visit_all (bson_iter_t          *iter,
                     const bson_visitor_t *visitor,
                     void                 *data)
{
   bson_return_if_fail(iter);
   bson_return_if_fail(visitor);

#define RUN_VISITOR(name) \
   if (visitor->visit_##name) visitor->visit_##name

   while (bson_iter_next(iter)) {
      RUN_VISITOR(before)(iter, bson_iter_key_unsafe(iter), data);

      switch (bson_iter_type(iter)) {
      case BSON_TYPE_DOUBLE:
         RUN_VISITOR(double)(iter,
                             bson_iter_key_unsafe(iter),
                             bson_iter_double(iter),
                             data);
         break;
      case BSON_TYPE_UTF8:
         {
            bson_uint32_t utf8_len;
            const char *utf8;
            utf8 = bson_iter_string(iter, &utf8_len);
            RUN_VISITOR(utf8)(iter,
                              bson_iter_key_unsafe(iter),
                              utf8_len,
                              utf8,
                              data);
         }
         break;
      case BSON_TYPE_DOCUMENT:
         {
            bson_t b = { 0 };

            bson_iter_document(iter, &b.len, (const bson_uint8_t **)&b.data);
            RUN_VISITOR(document)(iter,
                                  bson_iter_key_unsafe(iter),
                                  &b,
                                  data);
         }
         break;
      case BSON_TYPE_ARRAY:
         {
            bson_t b = { 0 };

            bson_iter_array(iter, &b.len, (const bson_uint8_t **)&b.data);
            RUN_VISITOR(array)(iter,
                               bson_iter_key_unsafe(iter),
                               &b,
                               data);
         }
         break;
      case BSON_TYPE_BINARY:
         {
            const bson_uint8_t *binary;
            bson_subtype_t subtype;
            bson_uint32_t binary_len;

            bson_iter_binary(iter, &subtype, &binary_len, &binary);
            RUN_VISITOR(binary)(iter,
                                bson_iter_key_unsafe(iter),
                                subtype,
                                binary_len,
                                binary,
                                data);
         }
         break;
      case BSON_TYPE_UNDEFINED:
         RUN_VISITOR(undefined)(iter,
                                bson_iter_key_unsafe(iter),
                                data);
         break;
      case BSON_TYPE_OID:
         RUN_VISITOR(oid)(iter,
                          bson_iter_key_unsafe(iter),
                          bson_iter_oid(iter),
                          data);
         break;
      case BSON_TYPE_BOOL:
         RUN_VISITOR(bool)(iter,
                           bson_iter_key_unsafe(iter),
                           bson_iter_bool(iter),
                           data);
         break;
      case BSON_TYPE_DATE_TIME:
         {
            RUN_VISITOR(date_time)(iter,
                                   bson_iter_key_unsafe(iter),
                                   bson_iter_date_time(iter),
                                   data);
         }
         break;
      case BSON_TYPE_NULL:
         RUN_VISITOR(null)(iter,
                           bson_iter_key_unsafe(iter),
                           data);
         break;
      case BSON_TYPE_REGEX:
         {
            const char *regex = NULL;
            const char *options = NULL;
            regex = bson_iter_regex(iter, &options);
            RUN_VISITOR(regex)(iter,
                               bson_iter_key_unsafe(iter),
                               regex,
                               options,
                               data);
         }
         break;
      case BSON_TYPE_DBPOINTER:
         {
            bson_uint32_t collection_len;
            const char *collection;
            const bson_oid_t *oid;

            bson_iter_dbpointer(iter, &collection_len, &collection, &oid);
            RUN_VISITOR(dbpointer)(iter,
                                   bson_iter_key_unsafe(iter),
                                   collection_len,
                                   collection,
                                   oid,
                                   data);
         }
         break;
      case BSON_TYPE_CODE:
         {
            bson_uint32_t code_len;
            const char *code;

            code = bson_iter_code(iter, &code_len);
            RUN_VISITOR(code)(iter,
                              bson_iter_key_unsafe(iter),
                              code_len,
                              code,
                              data);
         }
         break;
      case BSON_TYPE_SYMBOL:
         {
            bson_uint32_t symbol_len;
            const char *symbol;

            symbol = bson_iter_symbol(iter, &symbol_len);
            RUN_VISITOR(symbol)(iter,
                                bson_iter_key_unsafe(iter),
                                symbol_len,
                                symbol,
                                data);
         }
         break;
      case BSON_TYPE_CODEWSCOPE:
         {
            bson_uint32_t length = 0;
            const char *code;
            bson_t b = { 0 };

            code = bson_iter_codewscope(iter, &length, &b.len,
                                        (const bson_uint8_t **)&b.data);
            RUN_VISITOR(codewscope)(iter,
                                    bson_iter_key_unsafe(iter),
                                    length,
                                    code,
                                    &b,
                                    data);
         }
         break;
      case BSON_TYPE_INT32:
         RUN_VISITOR(int32)(iter,
                            bson_iter_key_unsafe(iter),
                            bson_iter_int32(iter),
                            data);
         break;
      case BSON_TYPE_TIMESTAMP:
         {
            bson_uint32_t timestamp;
            bson_uint32_t increment;
            bson_iter_timestamp(iter, &timestamp, &increment);
            RUN_VISITOR(timestamp)(iter,
                                   bson_iter_key_unsafe(iter),
                                   timestamp,
                                   increment,
                                   data);
         }
         break;
      case BSON_TYPE_INT64:
         RUN_VISITOR(int64)(iter,
                            bson_iter_key_unsafe(iter),
                            bson_iter_int64(iter),
                            data);
         break;
      case BSON_TYPE_MAXKEY:
         RUN_VISITOR(maxkey)(iter, bson_iter_key_unsafe(iter), data);
         break;
      case BSON_TYPE_MINKEY:
         RUN_VISITOR(minkey)(iter, bson_iter_key_unsafe(iter), data);
         break;
      case BSON_TYPE_EOD:
      default:
         break;
      }

      RUN_VISITOR(after)(iter, bson_iter_key_unsafe(iter), data);
   }

   if (iter->err_offset) {
      RUN_VISITOR(corrupt)(iter, data);
   }

#undef RUN_VISITOR
}


static void
bson_reader_fd_fill_buffer (bson_reader_fd_t *reader)
{
   ssize_t ret;

   bson_return_if_fail(reader);
   bson_return_if_fail(reader->fd >= 0);

   /*
    * TODO: Need to grow buffer to fit an entire BSON.
    */

   /*
    * Handle first read specially.
    */
   if ((!reader->done) && (!reader->offset) && (!reader->end)) {
      ret = read(reader->fd, &reader->data[0], reader->len);
      if (ret <= 0) {
         reader->done = TRUE;
         return;
      }
      reader->end = ret;
      return;
   }

   /*
    * Move valid data to head.
    */
   memmove(&reader->data[0],
           &reader->data[reader->offset],
           reader->end - reader->offset);
   reader->end = reader->end - reader->offset;
   reader->offset = 0;

   /*
    * Read in data to fill the buffer.
    */
   ret = read(reader->fd,
              &reader->data[reader->end],
              reader->len - reader->end);
   if (ret <= 0) {
      reader->done = TRUE;
   } else {
      reader->end += ret;
   }

   bson_return_if_fail(reader->offset == 0);
   bson_return_if_fail(reader->end <= reader->len);
}


void
bson_reader_init_from_fd (bson_reader_t *reader,
                          int            fd,
                          bson_bool_t    close_fd)
{
   bson_reader_fd_t *real = (bson_reader_fd_t *)reader;

   memset(reader, 0, sizeof *reader);
   real->type = BSON_READER_FD;
   real->data = bson_malloc0(1024);
   real->fd = fd;
   real->len = 1024;
   real->offset = 0;

   bson_reader_fd_fill_buffer(real);
}


static const bson_t *
bson_reader_fd_read (bson_reader_fd_t *reader)
{
   bson_uint32_t blen;

   bson_return_val_if_fail(reader, NULL);

   while (!reader->done) {
      if ((reader->end - reader->offset) < 4) {
         bson_reader_fd_fill_buffer(reader);
         continue;
      }

      memcpy(&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE(blen);
      if (blen > (reader->end - reader->offset)) {
         bson_reader_fd_fill_buffer(reader);
         continue;
      }

      reader->inline_bson.len = blen;
      reader->inline_bson.data = &reader->data[reader->offset];
      reader->offset += blen;

      return &reader->inline_bson;
   }

   return NULL;
}


void
bson_reader_init_from_data (bson_reader_t      *reader,
                            const bson_uint8_t *data,
                            size_t              length)
{
   bson_reader_data_t *real = (bson_reader_data_t *)reader;

   bson_return_if_fail(real);

   real->type = BSON_READER_DATA;
   real->data = data;
   real->length = length;
   real->offset = 0;
}


static const bson_t *
bson_reader_data_read (bson_reader_data_t *reader)
{
   bson_uint32_t blen;

   bson_return_val_if_fail(reader, NULL);

   if ((reader->offset + 4) < reader->length) {
      memcpy(&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE(blen);
      if ((blen + reader->offset) <= reader->length) {
         reader->inline_bson.len = blen;
         reader->inline_bson.data =
            (bson_uint8_t *)&reader->data[reader->offset];
         reader->offset += blen;
         return &reader->inline_bson;
      }
   }

   return NULL;
}


void
bson_reader_destroy (bson_reader_t *reader)
{
   bson_return_if_fail(reader);

   switch (reader->type) {
   case BSON_READER_FD:
      {
         bson_reader_fd_t *fd = (bson_reader_fd_t *)reader;
         if (fd->close_fd)
            close(fd->fd);
         bson_free(fd->data);
      }
      break;
   case BSON_READER_DATA:
      break;
   default:
      fprintf(stderr, "No such reader type: %02x\n", reader->type);
      break;
   }
}


const bson_t *
bson_reader_read (bson_reader_t *reader)
{
   bson_return_val_if_fail(reader, NULL);

   switch (reader->type) {
   case BSON_READER_FD:
      return bson_reader_fd_read((bson_reader_fd_t *)reader);
   case BSON_READER_DATA:
      return bson_reader_data_read((bson_reader_data_t *)reader);
   default:
      fprintf(stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   return NULL;
}


static void
visit_utf8 (const bson_iter_t *iter,
            const char        *key,
            size_t             v_utf8_len,
            const char        *v_utf8,
            void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_utf8);
   bson_string_append(state->str, "\"");
}


static void
visit_int32 (const bson_iter_t *iter,
             const char        *key,
             bson_int32_t       v_int32,
             void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%d", v_int32);
   bson_string_append(state->str, str);
}


static void
visit_int64 (const bson_iter_t *iter,
             const char        *key,
             bson_int64_t       v_int64,
             void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%ld", v_int64);
   bson_string_append(state->str, str);
}


static void
visit_double (const bson_iter_t *iter,
              const char        *key,
              double             v_double,
              void              *data)
{
   bson_json_state_t *state = data;
   char str[32];

   snprintf(str, sizeof str, "%lf", v_double);
   bson_string_append(state->str, str);
}


static void
visit_undefined (const bson_iter_t *iter,
                 const char        *key,
                 void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$undefined\" : true }");
}


static void
visit_null (const bson_iter_t *iter,
            const char        *key,
            void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "null");
}


static void
visit_oid (const bson_iter_t *iter,
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
}


static void
visit_binary (const bson_iter_t  *iter,
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
}


static void
visit_bool (const bson_iter_t *iter,
            const char        *key,
            bson_bool_t        v_bool,
            void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, v_bool ? "true" : "false");
}


static void
visit_date_time (const bson_iter_t    *iter,
                 const char           *key,
                 bson_int64_t          msec_since_epoch,
                 void                 *data)
{
   bson_json_state_t *state = data;
   char secstr[32];

   snprintf(secstr, sizeof secstr, "%lu", msec_since_epoch);

   bson_string_append(state->str, "{ \"$date\" : ");
   bson_string_append(state->str, secstr);
   bson_string_append(state->str, " }");
}


static void
visit_regex (const bson_iter_t *iter,
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
}


static void
visit_timestamp (const bson_iter_t *iter,
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
}


static void
visit_dbpointer (const bson_iter_t *iter,
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
}


static void
visit_minkey (const bson_iter_t *iter,
              const char        *key,
              void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$minKey\" : 1 }");
}


static void
visit_maxkey (const bson_iter_t *iter,
              const char        *key,
              void              *data)
{
   bson_json_state_t *state = data;
   bson_string_append(state->str, "{ \"$maxKey\" : 1 }");
}


static void
visit_before (const bson_iter_t *iter,
              const char        *key,
              void              *data)
{
   bson_json_state_t *state = data;

   if (state->count) bson_string_append(state->str, ", ");
   if (state->keys) {
      bson_string_append(state->str, "\"");
      bson_string_append(state->str, key);
      bson_string_append(state->str, "\" : ");
   }
   state->count++;
}


static void
visit_code (const bson_iter_t *iter,
            const char        *key,
            size_t             v_code_len,
            const char        *v_code,
            void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_code);
   bson_string_append(state->str, "\"");
}


static void
visit_symbol (const bson_iter_t *iter,
              const char        *key,
              size_t             v_symbol_len,
              const char        *v_symbol,
              void              *data)
{
   bson_json_state_t *state = data;

   bson_string_append(state->str, "\"");
   bson_string_append(state->str, v_symbol);
   bson_string_append(state->str, "\"");
}


static void
visit_codewscope (const bson_iter_t *iter,
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
}


static void
visit_array (const bson_iter_t *iter,
             const char        *key,
             const bson_t      *v_array,
             void              *data);


static void
visit_document (const bson_iter_t *iter,
                const char        *key,
                const bson_t      *v_document,
                void              *data);


static const bson_visitor_t bson_json_visitors = {
   .visit_before = visit_before,
   .visit_double = visit_double,
   .visit_utf8 = visit_utf8,
   .visit_document = visit_document,
   .visit_array = visit_array,
   .visit_binary = visit_binary,
   .visit_undefined = visit_undefined,
   .visit_oid = visit_oid,
   .visit_bool = visit_bool,
   .visit_date_time = visit_date_time,
   .visit_null = visit_null,
   .visit_regex = visit_regex,
   .visit_dbpointer = visit_dbpointer,
   .visit_code = visit_code,
   .visit_symbol = visit_symbol,
   .visit_codewscope = visit_codewscope,
   .visit_int32 = visit_int32,
   .visit_timestamp = visit_timestamp,
   .visit_int64 = visit_int64,
   .visit_minkey = visit_minkey,
   .visit_maxkey = visit_maxkey,
};


static void
visit_document (const bson_iter_t *iter,
                const char        *key,
                const bson_t      *v_document,
                void              *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = { 0, TRUE };
   bson_iter_t child;

   bson_iter_init(&child, v_document);

   child_state.str = bson_string_new("{ ");
   bson_iter_visit_all(&child, &bson_json_visitors, &child_state);
   bson_string_append(child_state.str, " }");
   bson_string_append(state->str, child_state.str->str);
   bson_string_free(child_state.str, TRUE);
}


static void
visit_array (const bson_iter_t *iter,
             const char        *key,
             const bson_t      *v_array,
             void              *data)
{
   bson_json_state_t *state = data;
   bson_json_state_t child_state = { 0, FALSE };
   bson_iter_t child;

   bson_iter_init(&child, v_array);

   child_state.str = bson_string_new("[ ");
   bson_iter_visit_all(&child, &bson_json_visitors, &child_state);
   bson_string_append(child_state.str, " ]");
   bson_string_append(state->str, child_state.str->str);
   bson_string_free(child_state.str, TRUE);
}


char *
bson_as_json (const bson_t *bson,
              size_t       *length)
{
   bson_json_state_t state;
   bson_iter_t iter;

   bson_return_val_if_fail(bson, NULL);

   if (!bson_iter_init(&iter, bson)) {
      return NULL;
   }

   state.count = 0;
   state.keys = TRUE;
   state.str = bson_string_new("{ ");
   bson_iter_visit_all(&iter, &bson_json_visitors, &state);
   bson_string_append(state.str, " }");

   if (length) {
      *length = state.str->len - 1;
   }

   return bson_string_free(state.str, FALSE);
}
