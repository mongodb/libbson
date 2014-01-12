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


#include <string.h>

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
   bson_reader_type_t type;
   int                fd;
   bson_bool_t        close_fd : 1;
   bson_bool_t        done     : 1;
   bson_bool_t        failed   : 1;
   bson_size_t             end;
   bson_size_t             len;
   bson_size_t             offset;
   bson_t             inline_bson;
   bson_uint8_t      *data;
   bson_read_func_t   read_func;
} bson_reader_fd_t;


typedef struct
{
   bson_reader_type_t  type;
   const bson_uint8_t *data;
   bson_size_t              length;
   bson_size_t              offset;
   bson_t              inline_bson;
} bson_reader_data_t;


static void
_bson_reader_fd_fill_buffer (bson_reader_fd_t *reader)
{
   bson_ssize_t ret;

   bson_return_if_fail (reader);
   bson_return_if_fail (reader->fd >= 0);

   /*
    * Handle first read specially.
    */
   if ((!reader->done) && (!reader->offset) && (!reader->end)) {
      ret = reader->read_func (reader->fd, &reader->data[0], reader->len);

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
   memmove (&reader->data[0],
            &reader->data[reader->offset],
            reader->end - reader->offset);
   reader->end = reader->end - reader->offset;
   reader->offset = 0;

   /*
    * Read in data to fill the buffer.
    */
   ret = reader->read_func (reader->fd,
                            &reader->data[reader->end],
                            reader->len - reader->end);

   if (ret <= 0) {
      reader->done = TRUE;
      reader->failed = (ret < 0);
   } else {
      reader->end += ret;
   }

   bson_return_if_fail (reader->offset == 0);
   bson_return_if_fail (reader->end <= reader->len);
}

/**
 * bson_reader_new_from_fd:
 * @fd: A file-descriptor to read from.
 * @close_fd: If the file-descriptor should be closed when done.
 *
 * Allocates and initializes a new bson_reader_t that will read BSON documents
 * into bson_t structures from an underlying file-descriptor.
 *
 * If you would like the reader to call close() on @fd in
 * bson_reader_destroy(), then specify TRUE for close_fd.
 *
 * Returns: (transfer full): A newly allocated bson_reader_t that should be
 *   freed with bson_reader_destroy().
 */
bson_reader_t *
bson_reader_new_from_fd (int         fd,
                         bson_bool_t close_fd)
{
   bson_reader_fd_t *real;

   real = bson_malloc0 (sizeof *real);
   real->type = BSON_READER_FD;
   real->data = bson_malloc0 (1024);
   real->fd = fd;
   real->len = 1024;
   real->offset = 0;

   bson_reader_set_read_func ((bson_reader_t *)real, bson_read);

   _bson_reader_fd_fill_buffer (real);

   return (bson_reader_t *)real;
}


/**
 * bson_reader_set_read_func:
 * @reader: A bson_reader_t.
 *
 * Tell @reader to use a customized read(). By default, @reader uses read() in
 * libc.
 *
 * Note that @reader must be initialized by bson_reader_init_from_fd(), or data
 * will be destroyed.
 */
void
bson_reader_set_read_func (bson_reader_t   *reader,
                           bson_read_func_t func)
{
   bson_reader_fd_t *real = (bson_reader_fd_t *)reader;

   bson_return_if_fail (reader);
   bson_return_if_fail (reader->type == BSON_READER_FD);
   bson_return_if_fail (func);

   real->read_func = func;
}


static void
_bson_reader_fd_grow_buffer (bson_reader_fd_t *reader)
{
   bson_size_t size;

   bson_return_if_fail (reader);

   size = reader->len * 2;
   reader->data = bson_realloc (reader->data, size);
   reader->len = size;
}


static bson_off_t
_bson_reader_fd_tell (bson_reader_fd_t *reader)
{
   bson_off_t off;

   bson_return_val_if_fail (reader, -1);

   off = bson_lseek (reader->fd, 0, SEEK_CUR);
   off -= reader->end;
   off += reader->offset;

   return off;
}


static const bson_t *
_bson_reader_fd_read (bson_reader_fd_t *reader,
                      bson_bool_t      *reached_eof)
{
   bson_uint32_t blen;

   bson_return_val_if_fail (reader, NULL);

   while (!reader->done) {
      if ((reader->end - reader->offset) < 4) {
         _bson_reader_fd_fill_buffer (reader);
         continue;
      }

      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if (blen > (reader->end - reader->offset)) {
         if (blen > reader->len) {
            _bson_reader_fd_grow_buffer (reader);
         }

         _bson_reader_fd_fill_buffer (reader);
         continue;
      }

      if (!bson_init_static (&reader->inline_bson,
                             &reader->data[reader->offset],
                             blen)) {
         return NULL;
      }

      reader->offset += blen;

      return &reader->inline_bson;
   }

   if (reached_eof) {
      *reached_eof = reader->done && !reader->failed;
   }

   return NULL;
}


/**
 * bson_reader_new_from_data:
 * @data: A buffer to read BSON documents from.
 * @length: The length of @data.
 *
 * Allocates and initializes a new bson_reader_t that will the memory
 * provided as a stream of BSON documents.
 *
 * Returns: (transfer full): A newly allocated bson_reader_t that should be
 *   freed with bson_reader_destroy().
 */
bson_reader_t *
bson_reader_new_from_data (const bson_uint8_t *data,
                           bson_size_t              length)
{
   bson_reader_data_t *real;

   real = bson_malloc0 (sizeof *real);
   real->type = BSON_READER_DATA;
   real->data = data;
   real->length = length;
   real->offset = 0;

   return (bson_reader_t *)real;
}


static const bson_t *
_bson_reader_data_read (bson_reader_data_t *reader,
                        bson_bool_t        *reached_eof)
{
   bson_uint32_t blen;

   bson_return_val_if_fail (reader, NULL);

   if (reached_eof) {
      *reached_eof = FALSE;
   }

   if ((reader->offset + 4) < reader->length) {
      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if ((blen + reader->offset) <= reader->length) {
         if (!bson_init_static (&reader->inline_bson,
                                &reader->data[reader->offset],
                                blen)) {
            if (reached_eof) {
               *reached_eof = FALSE;
            }

            return NULL;
         }

         reader->offset += blen;

         if (reached_eof) {
            *reached_eof = (reader->offset == reader->length);
         }

         return &reader->inline_bson;
      }
   }

   if (reached_eof) {
      *reached_eof = (reader->offset == reader->length);
   }

   return NULL;
}


static bson_off_t
_bson_reader_data_tell (bson_reader_data_t *reader)
{
   bson_return_val_if_fail (reader, -1);

   return reader->offset;
}


/**
 * bson_reader_destroy:
 * @reader: An initialized bson_reader_t.
 *
 * Releases resources that were allocated during the use of a bson_reader_t.
 * This should be called after you have finished using the structure.
 */
void
bson_reader_destroy (bson_reader_t *reader)
{
   bson_return_if_fail (reader);

   switch (reader->type) {
   case 0:
      break;
   case BSON_READER_FD:
      {
         bson_reader_fd_t *fd = (bson_reader_fd_t *)reader;

         if (fd->close_fd) {
            bson_close (fd->fd);
         }

         bson_free (fd->data);
      }
      break;
   case BSON_READER_DATA:
      break;
   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   reader->type = 0;

   bson_free (reader);
}


/**
 * bson_reader_read:
 * @reader: A bson_reader_t.
 * @reached_eof: A location for a bson_bool_t.
 *
 * Reads the next bson_t in the underlying memory or storage.  The resulting
 * bson_t should not be modified or freed. You may copy it and iterate over it.
 * Functions that take a const bson_t* are safe to use.
 *
 * This structure does not survive calls to bson_reader_read() or
 * bson_reader_destroy() as it uses memory allocated by the reader or
 * underlying storage/memory.
 *
 * If NULL is returned then @reached_eof will be set to TRUE if the end of the
 * file or buffer was reached. This indicates if there was an error parsing the
 * document stream.
 *
 * Returns: A const bson_t that should not be modified or freed.
 */
const bson_t *
bson_reader_read (bson_reader_t *reader,
                  bson_bool_t   *reached_eof)
{
   bson_return_val_if_fail (reader, NULL);

   switch (reader->type) {
   case BSON_READER_FD:
      return _bson_reader_fd_read ((bson_reader_fd_t *)reader, reached_eof);

   case BSON_READER_DATA:
      return _bson_reader_data_read ((bson_reader_data_t *)reader, reached_eof);

   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      break;
   }

   return NULL;
}


/**
 * bson_reader_tell:
 * @reader: A bson_reader_t.
 *
 * Return the current position in the underlying file. This will always
 * be at the beginning of a bson document or end of file.
 */
bson_off_t
bson_reader_tell (bson_reader_t *reader)
{
   bson_return_val_if_fail (reader, -1);

   switch (reader->type) {
   case BSON_READER_FD:
      return _bson_reader_fd_tell ((bson_reader_fd_t *)reader);

   case BSON_READER_DATA:
      return _bson_reader_data_tell ((bson_reader_data_t *)reader);

   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      return -1;
   }
}
