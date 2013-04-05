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
#include <string.h>
#include <unistd.h>

#include "b64_ntop.h"

#include "bson.h"
#include "bson-md5.h"
#include "bson-memory.h"
#include "bson-string.h"
#include "bson-thread.h"
#include "bson-utf8.h"


/*
 * TODO:
 *
 *   - Put some consideration into if we want to handle OOM. It is a really
 *     difficult thing to do correctly. Almost nobody gets it right. D-BUS
 *     on GNU/Linux might be one of the few things that does.
 */


static const bson_uint8_t gZero;


static bson_bool_t bson_as_json_visit_array    (const bson_iter_t *iter,
                                                const char        *key,
                                                const bson_t      *v_array,
                                                void              *data);
static bson_bool_t bson_as_json_visit_document (const bson_iter_t *iter,
                                                const char        *key,
                                                const bson_t      *v_document,
                                                void              *data);


typedef struct
{
   bson_uint32_t  count;
   bson_bool_t    keys;
   bson_string_t *str;
} bson_json_state_t;


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
      /*
       * TODO: We should make it so we can abort future iteration
       *       on the parent document by returning FALSE/TRUE/etc.
       */
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
bson_append_utf8 (bson_t     *bson,
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
   bson_string_append(state->str, v_utf8);
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
