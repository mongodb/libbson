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


#include "bson-private.h"
#include "bson-writer.h"


struct _bson_writer_t
{
   bson_bool_t         ready;
   bson_uint8_t      **buf;
   size_t             *buflen;
   size_t              offset;
   bson_realloc_func   realloc_func;
   bson_t              b;
};


bson_writer_t *
bson_writer_new (bson_uint8_t      **buf,
                 size_t             *buflen,
                 size_t              offset,
                 bson_realloc_func   realloc_func)
{
   bson_writer_t *writer;

   writer = bson_malloc0(sizeof *writer);
   writer->buf = buf;
   writer->buflen = buflen;
   writer->offset = offset;
   writer->realloc_func = realloc_func;
   writer->ready = TRUE;

   return writer;
}


void
bson_writer_destroy (bson_writer_t *writer)
{
   bson_free(writer);
}


size_t
bson_writer_get_length (bson_writer_t *writer)
{
   return writer->offset + writer->b.len;
}


void
bson_writer_begin (bson_writer_t  *writer,
                   bson_t        **bson)
{
   bson_impl_alloc_t *b;
   bson_bool_t grown = FALSE;

   bson_return_if_fail(writer);
   bson_return_if_fail(writer->ready);
   bson_return_if_fail(bson);

   writer->ready = FALSE;

   memset(&writer->b, 0, sizeof(bson_t));

   b = (bson_impl_alloc_t *)&writer->b;
   b->flags = BSON_FLAG_STATIC | BSON_FLAG_NO_FREE;
   b->len = 5;
   b->parent = NULL;
   b->buf = writer->buf;
   b->buflen = writer->buflen;
   b->offset = writer->offset;
   b->alloc = NULL;
   b->alloclen = 0;
   b->realloc = writer->realloc_func;

   while ((writer->offset + writer->b.len) > *writer->buflen) {
      grown = TRUE;
      if (!*writer->buflen) {
         *writer->buflen = 64;
      } else {
         (*writer->buflen) *= 2;
      }
   }

   if (grown) {
      *writer->buf = writer->realloc_func(*writer->buf, *writer->buflen);
   }

   memset((*writer->buf) + writer->offset + 1, 0, 5);
   (*writer->buf)[writer->offset] = 5;

   *bson = &writer->b;
}


void
bson_writer_end (bson_writer_t *writer)
{
   bson_return_if_fail(writer);
   bson_return_if_fail(!writer->ready);

   writer->offset += writer->b.len;
   memset(&writer->b, 0, sizeof(bson_t));
   writer->ready = TRUE;
}


void
bson_writer_rollback (bson_writer_t *writer)
{
   bson_return_if_fail(writer);

   if (writer->b.len) {
      memset(&writer->b, 0, sizeof(bson_t));
   }
   writer->ready = TRUE;
}
