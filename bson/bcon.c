/*
 * @file bcon.c
 * @brief BCON (BSON C Object Notation) Implementation
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

#include "bcon.h"
#include <error.h>

/* These stack manipulation macros are used to manage append recursion in
 * bcon_append_ctx_va().  They take care of some awkward dereference rules (the
 * real bson object isn't in the stack, but accessed by pointer) and add in run
 * time asserts to make sure we don't blow the stack in either direction */

#define STACK_ELE(_delta, _name) (ctx->stack[(_delta) + ctx->n]._name)

#define STACK_BSON(_delta) ( \
      ((_delta) + ctx->n) == 0 \
      ? bson \
      : &STACK_ELE (_delta, bson) \
      )

#define STACK_BSON_PARENT STACK_BSON (-1)
#define STACK_BSON_CHILD STACK_BSON (0)

#define STACK_I STACK_ELE (0, i)
#define STACK_IS_ARRAY STACK_ELE (0, is_array)

#define STACK_PUSH_ARRAY() \
   do { \
      assert(ctx->n < (BCON_STACK_MAX - 1)); \
      ctx->n++; \
      STACK_I = 0; \
      STACK_IS_ARRAY = 1; \
      bson_append_array_begin (STACK_BSON_PARENT, key, -1, STACK_BSON_CHILD); \
   } while (0)

#define STACK_PUSH_DOC() \
   do { \
      assert(ctx->n < (BCON_STACK_MAX - 1)); \
      ctx->n++; \
      STACK_IS_ARRAY = 0; \
      bson_append_document_begin (STACK_BSON_PARENT, key, -1, STACK_BSON_CHILD); \
   } while (0)

#define STACK_POP_ARRAY() \
   do { \
      assert(STACK_IS_ARRAY); \
      assert(ctx->n != 0); \
      bson_append_array_end (STACK_BSON_PARENT, STACK_BSON_CHILD); \
      ctx->n--; \
   } while (0)

#define STACK_POP_DOC() \
   do { \
      assert(! STACK_IS_ARRAY); \
      assert(ctx->n != 0); \
      bson_append_document_end (STACK_BSON_PARENT, STACK_BSON_CHILD); \
      ctx->n--; \
   } while (0)

/* This is a landing pad union for all of the types we can process with bcon.
 * We need actual storage for this to capture the return value of va_arg, which
 * takes multiple calls to get everything we need for some complex types */
typedef union bcon_append {
   char   *UTF8;
   double  DOUBLE;
   bson_t *DOCUMENT;
   bson_t *ARRAY;
   bson_t *BCON;

   struct
   {
      bson_subtype_t subtype;
      bson_uint8_t  *binary;
      bson_uint32_t  length;
   } BIN;

   bson_oid_t    *OID;
   bson_bool_t    BOOL;
   struct timeval DATE_TIME;

   struct
   {
      char *regex;
      char *flags;
   } REGEX;

   struct
   {
      char       *collection;
      bson_oid_t *oid;
   } DBPOINTER;

   const char *CODE;

   char *SYMBOL;

   struct
   {
      const char *js;
      bson_t     *scope;
   } CODEWSCOPE;

   bson_int32_t INT32;

   struct
   {
      bson_uint32_t timestamp;
      bson_uint32_t increment;
   } TIMESTAMP;

   bson_int64_t INT64;
} bcon_append_t;

char *BCON_MAGIC = "BCON_MAGIC";

/* appends val to the passed bson object.  Meant to be a super simple dispatch
 * table */
static void
_bcon_append_single (bson_t            *bson,
                     bcon_append_type_t type,
                     const char        *key,
                     bcon_append_t     *val)
{
   switch (type) {
   case BCON_APPEND_TYPE_UTF8:
      bson_append_utf8 (bson, key, -1, val->UTF8, -1);
      break;
   case BCON_APPEND_TYPE_DOUBLE:
      bson_append_double (bson, key, -1, val->DOUBLE);
      break;
   case BCON_APPEND_TYPE_BIN: {
         bson_append_binary (bson, key, -1, val->BIN.subtype, val->BIN.binary,
                             val->BIN.length);
         break;
      }
   case BCON_APPEND_TYPE_UNDEFINED:
      bson_append_undefined (bson, key, -1);
      break;
   case BCON_APPEND_TYPE_OID:
      bson_append_oid (bson, key, -1, val->OID);
      break;
   case BCON_APPEND_TYPE_BOOL:
      bson_append_bool (bson, key, -1, val->BOOL);
      break;
   case BCON_APPEND_TYPE_DATE_TIME:
      bson_append_timeval (bson, key, -1, &val->DATE_TIME);
      break;
   case BCON_APPEND_TYPE_NULL:
      bson_append_null (bson, key, -1);
      break;
   case BCON_APPEND_TYPE_REGEX: {
         bson_append_regex (bson, key, -1, val->REGEX.regex, val->REGEX.flags);
         break;
      }
   case BCON_APPEND_TYPE_DBPOINTER: {
         bson_append_dbpointer (bson, key, -1, val->DBPOINTER.collection,
                                val->DBPOINTER.oid);
         break;
      }
   case BCON_APPEND_TYPE_CODE:
      bson_append_code (bson, key, -1, val->CODE);
      break;
   case BCON_APPEND_TYPE_SYMBOL:
      bson_append_symbol (bson, key, -1, val->SYMBOL, -1);
      break;
   case BCON_APPEND_TYPE_CODEWSCOPE:
      bson_append_code_with_scope (bson, key, -1, val->CODEWSCOPE.js,
                                   val->CODEWSCOPE.scope);
      break;
   case BCON_APPEND_TYPE_INT32:
      bson_append_int32 (bson, key, -1, val->INT32);
      break;
   case BCON_APPEND_TYPE_TIMESTAMP: {
         bson_append_timestamp (bson, key, -1, val->TIMESTAMP.timestamp,
                                val->TIMESTAMP.increment);
         break;
      }
   case BCON_APPEND_TYPE_INT64:
      bson_append_int64 (bson, key, -1, val->INT64);
      break;
   case BCON_APPEND_TYPE_MAXKEY:
      bson_append_maxkey (bson, key, -1);
      break;
   case BCON_APPEND_TYPE_MINKEY:
      bson_append_minkey (bson, key, -1);
      break;
   case BCON_APPEND_TYPE_ARRAY: {
         bson_append_array (bson, key, -1, val->ARRAY);
         break;
      }
   case BCON_APPEND_TYPE_DOCUMENT: {
         bson_append_document (bson, key, -1, val->DOCUMENT);
         break;
      }
   default:
      assert (0);
      break;
   }
}


/* Consumes ap, storing output values into u and returning the type of the
 * captured token.
 *
 * The basic workflow goes like this:
 *
 * 1. Look at the current arg.  It will be a char *
 *    a. If it's a NULL, we're done processing.
 *    b. If it's BCON_MAGIC (a symbol with storage in this module)
 *       I. The next token is the type
 *       II. The type specifies how many args to eat and their types
 *    c. Otherwise it's either recursion related or a raw string
 *       I. If the first byte is '{', '}', '[', or ']' pass back an
 *          appropriate recursion token
 *       II. If not, just call it a UTF8 token and pass that back
 */
bcon_append_type_t
_bcon_append_tokenize (va_list       *ap,
                       bcon_append_t *u)
{
   char *mark;
   bcon_append_type_t type;

   mark = va_arg (*ap, char *);

   if (mark == NULL) {
      type = BCON_APPEND_TYPE_END;
   } else if (mark == BCON_MAGIC) {
      type = va_arg (*ap, bcon_append_type_t);

      switch (type) {
      case BCON_APPEND_TYPE_UTF8:
         u->UTF8 = va_arg (*ap, char *);
         break;
      case BCON_APPEND_TYPE_DOUBLE:
         u->DOUBLE = va_arg (*ap, double);
         break;
      case BCON_APPEND_TYPE_DOCUMENT:
         u->DOCUMENT = va_arg (*ap, bson_t *);
         break;
      case BCON_APPEND_TYPE_ARRAY:
         u->ARRAY = va_arg (*ap, bson_t *);
         break;
      case BCON_APPEND_TYPE_BIN:
         u->BIN.subtype = va_arg (*ap, bson_subtype_t);
         u->BIN.binary = va_arg (*ap, bson_uint8_t *);
         u->BIN.length = va_arg (*ap, bson_uint32_t);
         break;
      case BCON_APPEND_TYPE_UNDEFINED:
         break;
      case BCON_APPEND_TYPE_OID:
         u->OID = va_arg (*ap, bson_oid_t *);
         break;
      case BCON_APPEND_TYPE_BOOL:
         u->BOOL = va_arg (*ap, bson_bool_t);
         break;
      case BCON_APPEND_TYPE_DATE_TIME:
         u->DATE_TIME = va_arg (*ap, struct timeval);
         break;
      case BCON_APPEND_TYPE_NULL:
         break;
      case BCON_APPEND_TYPE_REGEX:
         u->REGEX.regex = va_arg (*ap, char *);
         u->REGEX.flags = va_arg (*ap, char *);
         break;
      case BCON_APPEND_TYPE_DBPOINTER:
         u->DBPOINTER.collection = va_arg (*ap, char *);
         u->DBPOINTER.oid = va_arg (*ap, bson_oid_t *);
         break;
      case BCON_APPEND_TYPE_CODE:
         u->CODE = va_arg (*ap, char *);
         break;
      case BCON_APPEND_TYPE_SYMBOL:
         u->SYMBOL = va_arg (*ap, char *);
         break;
      case BCON_APPEND_TYPE_CODEWSCOPE:
         u->CODEWSCOPE.js = va_arg (*ap, char *);
         u->CODEWSCOPE.scope = va_arg (*ap, bson_t *);
         break;
      case BCON_APPEND_TYPE_INT32:
         u->INT32 = va_arg (*ap, bson_int32_t);
         break;
      case BCON_APPEND_TYPE_TIMESTAMP:
         u->TIMESTAMP.timestamp = va_arg (*ap, bson_uint32_t);
         u->TIMESTAMP.increment = va_arg (*ap, bson_uint32_t);
         break;
      case BCON_APPEND_TYPE_INT64:
         u->INT64 = va_arg (*ap, bson_int64_t);
         break;
      case BCON_APPEND_TYPE_MAXKEY:
         break;
      case BCON_APPEND_TYPE_MINKEY:
         break;
      case BCON_APPEND_TYPE_BCON:
         u->BCON = va_arg (*ap, bson_t *);
         break;
      default:
         assert (0);
         break;
      }
   } else {
      switch (mark[0]) {
      case '{':
         type = BCON_APPEND_TYPE_DOC_START;
         break;
      case '}':
         type = BCON_APPEND_TYPE_DOC_END;
         break;
      case '[':
         type = BCON_APPEND_TYPE_ARRAY_START;
         break;
      case ']':
         type = BCON_APPEND_TYPE_ARRAY_END;
         break;

      default:
         type = BCON_APPEND_TYPE_UTF8;
         u->UTF8 = mark;
         break;
      }
   }

   return type;
}


/* This trivial utility function is useful for concatenating a bson object onto
 * the end of another, ignoring the keys from the source bson object and
 * continuing to use and increment the keys from the source.  It's only useful
 * when called from bcon_append_ctx_va */
static void
_bson_concat_array (bson_t            *dest,
                    const bson_t      *src,
                    bcon_append_ctx_t *ctx)
{
   const char * key;
   char i_str[11];
   bson_iter_t iter;

   bson_iter_init(&iter, src);

   STACK_I--;

   while (bson_iter_next(&iter)) {
      bson_uint32_to_string (STACK_I, &key, i_str, 11);
      STACK_I++;

      bson_append_iter(dest, key, -1, &iter);
   }
}


/* Append_ctx_va consumes the va_list until NULL is found, appending into bson
 * as tokens are found.  It can receive or return an in-progress bson object
 * via the ctx param.  It can also operate on the middle of a va_list, and so
 * can be wrapped inside of another varargs function.
 *
 * Note that passing in a va_list that isn't perferectly formatted for BCON
 * ingestion will almost certainly result in undefined behavior
 *
 * The workflow relies on the passed ctx object, which holds a stack of bson
 * objects, along with metadata (if the emedded layer is an array, and which
 * element it is on if so).  We iterate, generating tokens from the va_list,
 * until we reach an END token.  If any errors occur, we just blow up (the
 * var_args stuff is already incredibly fragile to mistakes, and we have no way
 * of introspecting, so just don't screw it up).
 *
 * There are also a few STACK_* macros in here which manimpulate ctx that are
 * defined up top.
 * */
void
bcon_append_ctx_va (bson_t            *bson,
                    bcon_append_ctx_t *ctx,
                    va_list           *ap)
{
   bcon_append_type_t type;
   const char *key;

   char i_str[11];

   bcon_append_t u;

   while (1) {
      if (STACK_IS_ARRAY) {
         bson_uint32_to_string (STACK_I, &key, i_str, 11);
         STACK_I++;
      } else {
         type = _bcon_append_tokenize (ap, &u);

         if (type == BCON_APPEND_TYPE_END) {
            return;
         }

         if (type == BCON_APPEND_TYPE_DOC_END) {
            STACK_POP_DOC();
            continue;
         }

         if (type == BCON_APPEND_TYPE_BCON) {
            bson_concat (STACK_BSON_CHILD, u.BCON);
            continue;
         }

         assert (type == BCON_APPEND_TYPE_UTF8);

         key = u.UTF8;
      }

      type = _bcon_append_tokenize (ap, &u);
      assert (type != BCON_APPEND_TYPE_END);

      switch (type) {
      case BCON_APPEND_TYPE_BCON:
         assert(STACK_IS_ARRAY);
         _bson_concat_array(STACK_BSON_CHILD, u.BCON, ctx);

         break;
      case BCON_APPEND_TYPE_DOC_START:
         STACK_PUSH_DOC();
         break;
      case BCON_APPEND_TYPE_DOC_END:
         STACK_POP_DOC();
         break;
      case BCON_APPEND_TYPE_ARRAY_START:
         STACK_PUSH_ARRAY();
         break;
      case BCON_APPEND_TYPE_ARRAY_END:
         STACK_POP_ARRAY();
         break;
      default:
         _bcon_append_single (STACK_BSON_CHILD, type, key, &u);

         break;
      }
   }
}

void
bcon_append (bson_t *bson,
             ...)
{
   va_list ap;
   bcon_append_ctx_t ctx;

   bcon_append_ctx_init (&ctx);

   va_start (ap, bson);

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_end (ap);
}


void
bcon_append_ctx (bson_t            *bson,
                 bcon_append_ctx_t *ctx,
                 ...)
{
   va_list ap;

   va_start (ap, ctx);

   bcon_append_ctx_va (bson, ctx, &ap);

   va_end (ap);
}


void
bcon_append_ctx_init (bcon_append_ctx_t *ctx)
{
   ctx->n = 0;
   ctx->stack[0].is_array = 0;
}


bson_t *
bcon_new (void * unused, ...)
{
   va_list ap;
   bcon_append_ctx_t ctx;
   bson_t * bson;

   bcon_append_ctx_init (&ctx);

   bson = bson_new();

   va_start (ap, unused);

   bcon_append_ctx_va (bson, &ctx, &ap);

   va_end (ap);

   return bson;
}
