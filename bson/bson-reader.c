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


#include <string.h>
#include <unistd.h>

#include "bson.h"
#include "bson-reader.h"
#include "bson-memory.h"


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
   bson_bool_t         failed : 1;
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


BSON_STATIC_ASSERT(sizeof(bson_reader_fd_t) < sizeof(bson_reader_t));
BSON_STATIC_ASSERT(sizeof(bson_reader_data_t) < sizeof(bson_reader_t));


static void
bson_reader_fd_fill_buffer (bson_reader_fd_t *reader)
{
   ssize_t ret;

   bson_return_if_fail(reader);
   bson_return_if_fail(reader->fd >= 0);

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
      reader->failed = (ret < 0);
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


static void
bson_reader_fd_grow_buffer (bson_reader_fd_t *reader)
{
   size_t size;

   bson_return_if_fail(reader);

   size = reader->len * 2;
   reader->data = bson_realloc(reader->data, size);
   reader->len = size;
}


static off_t
bson_reader_fd_tell (bson_reader_fd_t *reader)
{
   off_t off;

   bson_return_val_if_fail(reader, -1);

   off = lseek(reader->fd, 0, SEEK_CUR);
   off -= reader->end;
   off += reader->offset;

   return off;
}


static const bson_t *
bson_reader_fd_read (bson_reader_fd_t *reader,
                     bson_bool_t      *reached_eof)
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
         if (blen > reader->len) {
            bson_reader_fd_grow_buffer(reader);
         }
         bson_reader_fd_fill_buffer(reader);
         continue;
      }

      if (!bson_init_static(&reader->inline_bson,
                            &reader->data[reader->offset],
                            blen)) {
         return FALSE;
      }

      reader->offset += blen;

      return &reader->inline_bson;
   }

   if (reached_eof) {
      *reached_eof = reader->done && !reader->failed;
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
bson_reader_data_read (bson_reader_data_t *reader,
                       bson_bool_t        *reached_eof)
{
   bson_uint32_t blen;

   bson_return_val_if_fail(reader, NULL);

   if (reached_eof) {
      *reached_eof = FALSE;
   }

   if ((reader->offset + 4) < reader->length) {
      memcpy(&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE(blen);
      if ((blen + reader->offset) <= reader->length) {
         if (!bson_init_static(&reader->inline_bson,
                               &reader->data[reader->offset],
                               blen)) {
            if (reached_eof) {
               *reached_eof = FALSE;
            }
            return NULL;
         }
         reader->offset += blen;
         return &reader->inline_bson;
      }
   }

   if (reached_eof) {
      *reached_eof = (reader->offset == reader->length);
   }

   return NULL;
}


static off_t
bson_reader_data_tell (bson_reader_data_t *reader)
{
   bson_return_val_if_fail(reader, -1);
   return reader->offset;
}


void
bson_reader_destroy (bson_reader_t *reader)
{
   bson_return_if_fail(reader);

   switch (reader->type) {
   case 0:
      break;
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

   reader->type = 0;
}


const bson_t *
bson_reader_read (bson_reader_t *reader,
                  bson_bool_t   *reached_eof)
{
   bson_return_val_if_fail(reader, NULL);

   switch (reader->type) {
   case BSON_READER_FD:
      return bson_reader_fd_read((bson_reader_fd_t *)reader, reached_eof);
   case BSON_READER_DATA:
      return bson_reader_data_read((bson_reader_data_t *)reader, reached_eof);
   default:
      fprintf(stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   return NULL;
}


off_t
bson_reader_tell (bson_reader_t *reader)
{
   bson_return_val_if_fail(reader, -1);

   switch (reader->type) {
   case BSON_READER_FD:
      return bson_reader_fd_tell((bson_reader_fd_t *)reader);
   case BSON_READER_DATA:
      return bson_reader_data_tell((bson_reader_data_t *)reader);
   default:
      fprintf(stderr, "No such reader type: %02x\n", reader->type);
      return -1;
   }
}
