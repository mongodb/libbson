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
#include <bson/bson-memory.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "bson-tests.h"


static bson_t *
get_bson (const char *filename)
{
   bson_uint32_t len;
   bson_uint8_t buf[4096];
   bson_t *b;
   int fd;

   if (-1 == (fd = open(filename, O_RDONLY))) {
      fprintf(stderr, "Failed to open: %s\n", filename);
      abort();
   }
   len = read(fd, buf, sizeof buf);
   b = bson_new_from_data(buf, len);
   close(fd);

   return b;
}


static void
test_bson_iter_string (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = bson_new();
   bson_append_string(b, "foo", -1, "bar", -1);
   bson_append_string(b, "bar", -1, "baz", -1);
   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_UTF8(&iter));
   assert(!strcmp(bson_iter_key(&iter), "foo"));
   assert(!strcmp(bson_iter_string(&iter, NULL), "bar"));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_UTF8(&iter));
   assert(!strcmp(bson_iter_key(&iter), "bar"));
   assert(!strcmp(bson_iter_string(&iter, NULL), "baz"));
   assert(!bson_iter_next(&iter));
   bson_destroy(b);
}


static void
test_bson_iter_mixed (void)
{
   bson_iter_t iter;
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   b2 = bson_new();
   bson_append_string(b2, "foo", -1, "bar", -1);
   bson_append_code(b, "0", -1, "var a = {};");
   bson_append_code_with_scope(b, "1", -1, "var b = {};", b2);
   bson_append_int32(b, "2", -1, 1234);
   bson_append_int64(b, "3", -1, 4567);
   bson_append_time_t(b, "4", -1, 123456);
   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_CODE(&iter));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_CODEWSCOPE(&iter));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_INT32(&iter));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_INT64(&iter));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_DATE_TIME(&iter));
   assert(!bson_iter_next(&iter));
   assert(bson_iter_init_find(&iter, b, "3"));
   assert(!strcmp(bson_iter_key(&iter), "3"));
   assert(bson_iter_int64(&iter) == 4567);
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_DATE_TIME(&iter));
   assert(bson_iter_time_t(&iter) == 123456);
   assert(bson_iter_date_time(&iter) == 123456000);
   assert(!bson_iter_next(&iter));
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_iter_overflow (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = get_bson("tests/binary/overflow1.bson");
   assert(!b);

   b = get_bson("tests/binary/overflow2.bson");
   assert(b);
   bson_iter_init(&iter, b);
   assert(!bson_iter_next(&iter));
   bson_destroy(b);
}


static void
test_bson_iter_trailing_null (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = get_bson("tests/binary/trailingnull.bson");
   assert(b);
   bson_iter_init(&iter, b);
   assert(!bson_iter_next(&iter));
   bson_destroy(b);
}


static void
test_bson_iter_fuzz (void)
{
   bson_uint8_t *data;
   bson_uint32_t len = 512;
   bson_uint32_t len_le;
   bson_uint32_t r;
   bson_iter_t iter;
   bson_t *b;
   int i;
   int pass;

   len_le = BSON_UINT32_TO_LE(len);

   for (pass = 0; pass < 10000; pass++) {
      data = bson_malloc0(len);
      memcpy(data, &len_le, 4);

      for (i = 4; i < len; i += 4) {
         r = rand();
         memcpy(&data[i], &r, 4);
      }

      b = bson_new_from_data(data, len);
      assert(b);

      assert(bson_iter_init(&iter, b));
      while (bson_iter_next(&iter)) {
         assert(iter.next_offset < len);
      }

      bson_destroy(b);
      bson_free(data);
   }
}


static void
test_bson_iter_regex (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = bson_new();
   bson_append_regex(b, "foo", -1, "^abcd", "");
   bson_append_regex(b, "foo", -1, "^abcd", NULL);
   bson_append_regex(b, "foo", -1, "^abcd", "ix");

   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   assert(bson_iter_next(&iter));
   assert(bson_iter_next(&iter));

   bson_destroy(b);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/iter/test_string", test_bson_iter_string);
   run_test("/bson/iter/test_mixed", test_bson_iter_mixed);
   run_test("/bson/iter/test_overflow", test_bson_iter_overflow);
   run_test("/bson/iter/test_trailing_null", test_bson_iter_trailing_null);
   run_test("/bson/iter/test_fuzz", test_bson_iter_fuzz);
   run_test("/bson/iter/test_regex", test_bson_iter_regex);

   return 0;
}
