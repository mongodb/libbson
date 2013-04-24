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
   bson_bool_t eof = FALSE;

   buffer = bson_malloc0(4095);
   for (i = 0; i < 4095; i += 5) {
      buffer[i] = 5;
   }

   bson_reader_init_from_data(&reader, buffer, 4095);

   for (i = 0; (b = bson_reader_read(&reader, &eof)); i++) {
      /* do nothing */
      assert(b->len == 5);
      assert(b->u.top.data[0] == 5);
      assert(b->u.top.data[1] == 0);
      assert(b->u.top.data[2] == 0);
      assert(b->u.top.data[3] == 0);
      assert(b->u.top.data[4] == 0);
   }

   assert(i == (4095/5));

   assert_cmpint(eof, ==, TRUE);

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
   bson_bool_t eof = FALSE;

   buffer = bson_malloc0(4096);
   for (i = 0; i < 4095; i += 5) {
      buffer[i] = 5;
   }

   buffer[4095] = 5;

   bson_reader_init_from_data(&reader, buffer, 4096);

   for (i = 0; (b = bson_reader_read(&reader, &eof)); i++) {
      /* do nothing */
      assert(b->len == 5);
      assert(b->u.top.data[0] == 5);
      assert(b->u.top.data[1] == 0);
      assert(b->u.top.data[2] == 0);
      assert(b->u.top.data[3] == 0);
      assert(b->u.top.data[4] == 0);
      eof = FALSE;
   }

   assert(i == (4095/5));

   assert_cmpint(eof, ==, FALSE);

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
   bson_bool_t eof;
   int fd;

   fd  = open("tests/binary/stream.bson", O_RDONLY);
   assert(fd >= 0);

   bson_reader_init_from_fd(&reader, fd, TRUE);

   for (i = 0; i < 1000; i++) {
      eof = FALSE;
      b = bson_reader_read(&reader, &eof);
      assert(b);
      assert(bson_iter_init(&iter, b));
      assert(!bson_iter_next(&iter));
   }

   assert_cmpint(eof, ==, FALSE);
   b = bson_reader_read(&reader, &eof);
   assert(!b);
   assert_cmpint(eof, ==, TRUE);
   bson_reader_destroy(&reader);
}


static void
test_reader_from_fd_corrupt (void)
{
   bson_reader_t reader;
   const bson_t *b;
   bson_uint32_t i;
   bson_iter_t iter;
   bson_bool_t eof;
   int fd;

   fd  = open("tests/binary/stream_corrupt.bson", O_RDONLY);
   assert(fd >= 0);

   bson_reader_init_from_fd(&reader, fd, TRUE);

   for (i = 0; i < 1000; i++) {
      b = bson_reader_read(&reader, &eof);
      assert(b);
      assert(bson_iter_init(&iter, b));
      assert(!bson_iter_next(&iter));
   }

   b = bson_reader_read(&reader, &eof);
   assert(!b);
   bson_reader_destroy(&reader);
}


static void
test_reader_grow_buffer (void)
{
   bson_reader_t reader;
   const bson_t *b;
   bson_bool_t eof = FALSE;
   int fd;

   fd  = open("tests/binary/readergrow.bson", O_RDONLY);
   assert(fd >= 0);

   bson_reader_init_from_fd(&reader, fd, TRUE);

   b = bson_reader_read(&reader, &eof);
   assert(b);
   assert(!eof);

   b = bson_reader_read(&reader, &eof);
   assert(!b);
   assert(eof);

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
   run_test("/bson/reader/grow_buffer", test_reader_grow_buffer);

   return 0;
}
