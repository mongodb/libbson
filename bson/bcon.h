/*
 * @file bcon.h
 * @brief BCON (BSON C Object Notation) Declarations
 */

/*    Copyright 2009-2013 MongoDB Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#ifndef BCON_H_
#define BCON_H_

#include <bson.h>

#define BCON_STACK_MAX 100

#define BCON_UTF8(val) BCON_MAGIC, BCON_APPEND_TYPE_UTF8, (val)
#define BCON_DOUBLE(val) BCON_MAGIC, BCON_APPEND_TYPE_DOUBLE, (val)
#define BCON_DOCUMENT(val) BCON_MAGIC, BCON_APPEND_TYPE_DOCUMENT, (val)
#define BCON_ARRAY(val) BCON_MAGIC, BCON_APPEND_TYPE_ARRAY, (val)
#define BCON_BIN(subtype, binary, length) \
   BCON_MAGIC, BCON_APPEND_TYPE_BIN, (subtype), (binary), (length)
#define BCON_UNDEFINED BCON_MAGIC, BCON_APPEND_TYPE_UNDEFINED
#define BCON_OID(val) BCON_MAGIC, BCON_APPEND_TYPE_OID, (val)
#define BCON_BOOL(val) BCON_MAGIC, BCON_APPEND_TYPE_BOOL, (val)
#define BCON_DATE_TIME(val) BCON_MAGIC, BCON_APPEND_TYPE_DATE_TIME, (val)
#define BCON_NULL BCON_MAGIC, BCON_APPEND_TYPE_NULL
#define BCON_REGEX(regex, flags) \
   BCON_MAGIC, BCON_APPEND_TYPE_REGEX, (regex), (flags)
#define BCON_DBPOINTER(collection, oid) \
   BCON_MAGIC, BCON_APPEND_TYPE_DBPOINTER, (collection), (oid)
#define BCON_CODE(val) BCON_MAGIC, BCON_APPEND_TYPE_CODE, (val)
#define BCON_SYMBOL(val) BCON_MAGIC, BCON_APPEND_TYPE_SYMBOL, (val)
#define BCON_CODEWSCOPE(js, scope) \
   BCON_MAGIC, BCON_APPEND_TYPE_CODEWSCOPE, (js), (scope)
#define BCON_INT32(val) BCON_MAGIC, BCON_APPEND_TYPE_INT32, (val)
#define BCON_TIMESTAMP(timestamp, increment) \
   BCON_MAGIC, BCON_APPEND_TYPE_TIMESTAMP, (timestamp), (increment)
#define BCON_INT64(val) BCON_MAGIC, BCON_APPEND_TYPE_INT64, (val)
#define BCON_MAXKEY BCON_MAGIC, BCON_APPEND_TYPE_MAXKEY
#define BCON_MINKEY BCON_MAGIC, BCON_APPEND_TYPE_MINKEY
#define BCON(val) BCON_MAGIC, BCON_APPEND_TYPE_BCON, (val)

extern char *BCON_MAGIC;

typedef enum
{
   BCON_APPEND_TYPE_UTF8,
   BCON_APPEND_TYPE_DOUBLE,
   BCON_APPEND_TYPE_DOCUMENT,
   BCON_APPEND_TYPE_ARRAY,
   BCON_APPEND_TYPE_BIN,
   BCON_APPEND_TYPE_UNDEFINED,
   BCON_APPEND_TYPE_OID,
   BCON_APPEND_TYPE_BOOL,
   BCON_APPEND_TYPE_DATE_TIME,
   BCON_APPEND_TYPE_NULL,
   BCON_APPEND_TYPE_REGEX,
   BCON_APPEND_TYPE_DBPOINTER,
   BCON_APPEND_TYPE_CODE,
   BCON_APPEND_TYPE_SYMBOL,
   BCON_APPEND_TYPE_CODEWSCOPE,
   BCON_APPEND_TYPE_INT32,
   BCON_APPEND_TYPE_TIMESTAMP,
   BCON_APPEND_TYPE_INT64,
   BCON_APPEND_TYPE_MAXKEY,
   BCON_APPEND_TYPE_MINKEY,
   BCON_APPEND_TYPE_BCON,
   BCON_APPEND_TYPE_ARRAY_START,
   BCON_APPEND_TYPE_ARRAY_END,
   BCON_APPEND_TYPE_DOC_START,
   BCON_APPEND_TYPE_DOC_END,
   BCON_APPEND_TYPE_END,
   BCON_APPEND_TYPE_ERROR,
} bcon_append_type_t;

typedef struct bcon_append_ctx_frame
{
   int         i;
   bson_bool_t is_array;
   bson_t      bson;
} bcon_append_ctx_frame_t;

typedef struct bcon_append_ctx
{
   bcon_append_ctx_frame_t stack[BCON_STACK_MAX];
   int                     n;
} bcon_append_ctx_t;

void
bcon_append (bson_t *bson,
             ...) BSON_GNUC_NULL_TERMINATED;
void
bcon_append_ctx (bson_t            *bson,
                 bcon_append_ctx_t *ctx,
                 ...) BSON_GNUC_NULL_TERMINATED;
void
bcon_append_ctx_va (bson_t            *bson,
                    bcon_append_ctx_t *ctx,
                    va_list           *va);
void
bcon_append_ctx_init (bcon_append_ctx_t *ctx);

bson_t *
bcon_new (void *unused,
          ...) BSON_GNUC_NULL_TERMINATED;

#define BCON_APPEND(_bson, ...) \
   bcon_append (_bson, __VA_ARGS__, NULL)
#define BCON_APPEND_CTX(_bson, _ctx, ...) \
   bcon_append_ctx (_bson, _ctx, __VA_ARGS__, NULL)

#define BCON_NEW(...) \
   bcon_new (NULL, __VA_ARGS__, NULL)

#endif
