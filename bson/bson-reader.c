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

#include "bson.h"

#include <string.h>

#include "bson-reader.h"
#include "bson-memory.h"


typedef enum
{
   BSON_READER_HANDLE = 1,
   BSON_READER_DATA = 2,
} bson_reader_type_t;


typedef struct
{
   bson_reader_type_t         type;
   void                      *handle;
   bool                       done   : 1;
   bool                       failed : 1;
   size_t                     end;
   size_t                     len;
   size_t                     offset;
   size_t                     bytes_read;
   bson_t                     inline_bson;
   uint8_t                   *data;
   bson_reader_read_func_t    read_func;
   bson_reader_destroy_func_t destroy_func;
} bson_reader_handle_t;


typedef struct
{
   int fd;
   bool do_close;
} bson_reader_handle_fd_t;


typedef struct
{
   bson_reader_type_t type;
   const uint8_t     *data;
   size_t             length;
   size_t             offset;
   bson_t             inline_bson;
} bson_reader_data_t;


static void
_bson_reader_handle_fill_buffer (bson_reader_handle_t *reader)
{
   ssize_t ret;

   bson_return_if_fail (reader);

   /*
    * Handle first read specially.
    */
   if ((!reader->done) && (!reader->offset) && (!reader->end)) {
      ret = reader->read_func (reader->handle, &reader->data[0], reader->len);

      if (ret <= 0) {
         reader->done = true;
         return;
      }
      reader->bytes_read += ret;

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
   ret = reader->read_func (reader->handle,
                            &reader->data[reader->end],
                            reader->len - reader->end);

   if (ret <= 0) {
      reader->done = true;
      reader->failed = (ret < 0);
   } else {
      reader->bytes_read += ret;
      reader->end += ret;
   }

   bson_return_if_fail (reader->offset == 0);
   bson_return_if_fail (reader->end <= reader->len);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_reader_new_from_handle --
 *
 *       Allocates and initializes a new bson_reader_t using the opaque
 *       handle provided.
 *
 * Parameters:
 *       @handle: an opaque handle to use to read data.
 *       @rf: a function to perform reads on @handle.
 *       @df: a function to release @handle, or NULL.
 *
 * Returns:
 *       A newly allocated bson_reader_t if successful, otherwise NULL.
 *       Free the successful result with bson_reader_destroy().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_reader_t *
bson_reader_new_from_handle (void                       *handle,
                             bson_reader_read_func_t     rf,
                             bson_reader_destroy_func_t  df)
{
   bson_reader_handle_t *real;

   assert(handle);
   assert(rf);

   real = bson_malloc0 (sizeof *real);
   real->type = BSON_READER_HANDLE;
   real->data = bson_malloc0 (1024);
   real->handle = handle;
   real->len = 1024;
   real->offset = 0;

   bson_reader_set_read_func ((bson_reader_t *)real, rf);

   if (df) {
      bson_reader_set_destroy_func ((bson_reader_t *)real, df);
   }

   _bson_reader_handle_fill_buffer (real);

   return (bson_reader_t *)real;
}


static void
_bson_reader_handle_fd_destroy (void *handle) /* IN */
{
   bson_reader_handle_fd_t *fd = handle;

   if (fd) {
      if ((fd->fd != -1) && fd->do_close) {
         close (fd->fd);
      }
      bson_free (fd);
   }
}


static ssize_t
_bson_reader_handle_fd_read (void   *handle, /* IN */
                             void   *buf,    /* IN */
                             size_t  len)    /* IN */
{
   bson_reader_handle_fd_t *fd = handle;

   if (fd && (fd->fd != -1)) {
      return read (fd->fd, buf, len);
   }

   return -1;
}


bson_reader_t *
bson_reader_new_from_fd (int  fd,               /* IN */
                         bool close_on_destroy) /* IN */
{
   bson_reader_handle_fd_t *handle;

   BSON_ASSERT (fd != -1);

   handle = bson_malloc0 (sizeof *handle);
   handle->fd = fd;
   handle->do_close = close_on_destroy;

   return bson_reader_new_from_handle (handle,
                                       _bson_reader_handle_fd_read,
                                       _bson_reader_handle_fd_destroy);
}


/**
 * bson_reader_set_read_func:
 * @reader: A bson_reader_t.
 *
 * Note that @reader must be initialized by bson_reader_init_from_handle(), or data
 * will be destroyed.
 */
void
bson_reader_set_read_func (bson_reader_t          *reader,
                           bson_reader_read_func_t func)
{
   bson_reader_handle_t *real = (bson_reader_handle_t *)reader;

   assert(reader->type == BSON_READER_HANDLE);

   real->read_func = func;
}


/**
 * bson_reader_set_destroy_func:
 * @reader: A bson_reader_t.
 *
 * Note that @reader must be initialized by bson_reader_init_from_handle(), or data
 * will be destroyed.
 */
void
bson_reader_set_destroy_func (bson_reader_t             *reader,
                              bson_reader_destroy_func_t func)
{
   bson_reader_handle_t *real = (bson_reader_handle_t *)reader;

   assert(reader->type == BSON_READER_HANDLE);

   real->destroy_func = func;
}


static void
_bson_reader_handle_grow_buffer (bson_reader_handle_t *reader)
{
   size_t size;

   bson_return_if_fail (reader);

   size = reader->len * 2;
   reader->data = bson_realloc (reader->data, size);
   reader->len = size;
}


static off_t
_bson_reader_handle_tell (bson_reader_handle_t *reader)
{
   off_t off;

   bson_return_val_if_fail (reader, -1);

   off = (off_t)reader->bytes_read;
   off -= (off_t)reader->end;
   off += (off_t)reader->offset;

   return off;
}


static const bson_t *
_bson_reader_handle_read (bson_reader_handle_t *reader,
                      bool      *reached_eof)
{
   uint32_t blen;

   bson_return_val_if_fail (reader, NULL);

   while (!reader->done) {
      if ((reader->end - reader->offset) < 4) {
         _bson_reader_handle_fill_buffer (reader);
         continue;
      }

      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if (blen > (reader->end - reader->offset)) {
         if (blen > reader->len) {
            _bson_reader_handle_grow_buffer (reader);
         }

         _bson_reader_handle_fill_buffer (reader);
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
bson_reader_new_from_data (const uint8_t *data,
                           size_t              length)
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
                        bool        *reached_eof)
{
   uint32_t blen;

   bson_return_val_if_fail (reader, NULL);

   if (reached_eof) {
      *reached_eof = false;
   }

   if ((reader->offset + 4) < reader->length) {
      memcpy (&blen, &reader->data[reader->offset], sizeof blen);
      blen = BSON_UINT32_FROM_LE (blen);

      if ((blen + reader->offset) <= reader->length) {
         if (!bson_init_static (&reader->inline_bson,
                                &reader->data[reader->offset],
                                blen)) {
            if (reached_eof) {
               *reached_eof = false;
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


static off_t
_bson_reader_data_tell (bson_reader_data_t *reader)
{
   bson_return_val_if_fail (reader, -1);

   return (off_t)reader->offset;
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
   case BSON_READER_HANDLE:
      {
         bson_reader_handle_t *handle = (bson_reader_handle_t *)reader;

         if (handle->destroy_func) {

            handle->destroy_func(handle->handle);
         }

         bson_free (handle->data);
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
 * @reached_eof: A location for a bool.
 *
 * Reads the next bson_t in the underlying memory or storage.  The resulting
 * bson_t should not be modified or freed. You may copy it and iterate over it.
 * Functions that take a const bson_t* are safe to use.
 *
 * This structure does not survive calls to bson_reader_read() or
 * bson_reader_destroy() as it uses memory allocated by the reader or
 * underlying storage/memory.
 *
 * If NULL is returned then @reached_eof will be set to true if the end of the
 * file or buffer was reached. This indicates if there was an error parsing the
 * document stream.
 *
 * Returns: A const bson_t that should not be modified or freed.
 */
const bson_t *
bson_reader_read (bson_reader_t *reader,
                  bool   *reached_eof)
{
   bson_return_val_if_fail (reader, NULL);

   switch (reader->type) {
   case BSON_READER_HANDLE:
      return _bson_reader_handle_read ((bson_reader_handle_t *)reader, reached_eof);

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
 * Return the current position in the underlying reader. This will always
 * be at the beginning of a bson document or end of file.
 */
off_t
bson_reader_tell (bson_reader_t *reader)
{
   bson_return_val_if_fail (reader, -1);

   switch (reader->type) {
   case BSON_READER_HANDLE:
      return _bson_reader_handle_tell ((bson_reader_handle_t *)reader);

   case BSON_READER_DATA:
      return _bson_reader_data_tell ((bson_reader_data_t *)reader);

   default:
      fprintf (stderr, "No such reader type: %02x\n", reader->type);
      return -1;
   }
}
