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


#include <assert.h>
#include <fcntl.h>

#include "bson-tests.h"


static void
test_reader_from_data (void)
{
   bson_reader_t reader;
   bson_uint8_t *buffer;
   const bson_t *b;
   bson_uint32_t i;

   buffer = bson_malloc0(4095);
   for (i = 0; i < 4095; i += 5) {
      buffer[i] = 5;
   }

   bson_reader_init_from_data(&reader, buffer, 4095);

   for (i = 0; (b = bson_reader_read(&reader)); i++) {
      /* do nothing */
      assert(b->len == 5);
      assert(b->data[0] == 5);
      assert(b->data[1] == 0);
      assert(b->data[2] == 0);
      assert(b->data[3] == 0);
      assert(b->data[4] == 0);
   }

   assert(i == (4095/5));

   bson_free(buffer);

   bson_reader_destroy(&reader);
}


static void
test_reader_from_data_overflow (void)
{
   bson_reader_t reader;
   bson_uint8_t *buffer;
   const bson_t *b;
   bson_uint32_t i;

   buffer = bson_malloc0(4096);
   for (i = 0; i < 4095; i += 5) {
      buffer[i] = 5;
   }

   buffer[4095] = 5;

   bson_reader_init_from_data(&reader, buffer, 4096);

   for (i = 0; (b = bson_reader_read(&reader)); i++) {
      /* do nothing */
      assert(b->len == 5);
      assert(b->data[0] == 5);
      assert(b->data[1] == 0);
      assert(b->data[2] == 0);
      assert(b->data[3] == 0);
      assert(b->data[4] == 0);
   }

   assert(i == (4095/5));

   bson_free(buffer);

   bson_reader_destroy(&reader);
}


static void
test_reader_from_fd (void)
{
   bson_reader_t reader;
   const bson_t *b;
   bson_uint32_t i;
   bson_iter_t iter;
   int fd;

   fd  = open("tests/binary/stream.bson", O_RDONLY);
   assert(fd >= 0);

   bson_reader_init_from_fd(&reader, fd, TRUE);

   for (i = 0; i < 1000; i++) {
      b = bson_reader_read(&reader);
      assert(b);
      assert(bson_iter_init(&iter, b));
      assert(!bson_iter_next(&iter));
   }

   b = bson_reader_read(&reader);
   assert(!b);
   bson_reader_destroy(&reader);
}


static void
test_reader_from_fd_corrupt (void)
{
   bson_reader_t reader;
   const bson_t *b;
   bson_uint32_t i;
   bson_iter_t iter;
   int fd;

   fd  = open("tests/binary/stream_corrupt.bson", O_RDONLY);
   assert(fd >= 0);

   bson_reader_init_from_fd(&reader, fd, TRUE);

   for (i = 0; i < 1000; i++) {
      b = bson_reader_read(&reader);
      assert(b);
      assert(bson_iter_init(&iter, b));
      assert(!bson_iter_next(&iter));
   }

   b = bson_reader_read(&reader);
   assert(!b);
   bson_reader_destroy(&reader);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/reader/init_from_data", test_reader_from_data);
   run_test("/bson/reader/init_from_data_overflow",
            test_reader_from_data_overflow);
   run_test("/bson/reader/init_from_fd", test_reader_from_fd);
   run_test("/bson/reader/init_from_fd_corrupt",
            test_reader_from_fd_corrupt);

   return 0;
}
