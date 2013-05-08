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
#include <bson/bson.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "bson-tests.h"


static bson_t *
get_bson (const char *filename)
{
   bson_uint8_t buf[4096];
   bson_t *b;
   int len;
   int fd;

   if (-1 == (fd = open(filename, O_RDONLY))) {
      fprintf(stderr, "Failed to open: %s\n", filename);
      abort();
   }
   if ((len = read(fd, buf, sizeof buf)) < 0) {
      fprintf(stderr, "Failed to read: %s\n", filename);
      abort();
   }
   b = bson_new_from_data(buf, len);
   close(fd);

   return b;
}


static void
test_bson_iter_utf8 (void)
{
   bson_iter_t iter;
   bson_t *b;

   b = bson_new();
   bson_append_utf8(b, "foo", -1, "bar", -1);
   bson_append_utf8(b, "bar", -1, "baz", -1);
   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_UTF8(&iter));
   assert(!strcmp(bson_iter_key(&iter), "foo"));
   assert(!strcmp(bson_iter_utf8(&iter, NULL), "bar"));
   assert(bson_iter_next(&iter));
   assert(BSON_ITER_HOLDS_UTF8(&iter));
   assert(!strcmp(bson_iter_key(&iter), "bar"));
   assert(!strcmp(bson_iter_utf8(&iter, NULL), "baz"));
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
   bson_append_utf8(b2, "foo", -1, "bar", -1);
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

      /*
       * TODO: Most of the following ignores the key. That should be fixed
       *       but has it's own perils too.
       */

      assert(bson_iter_init(&iter, b));
      while (bson_iter_next(&iter)) {
         assert(iter.next_offset < len);
         switch (bson_iter_type(&iter)) {
         case BSON_TYPE_ARRAY:
         case BSON_TYPE_DOCUMENT:
            {
               const bson_uint8_t *child = NULL;
               bson_uint32_t child_len = -1;

               bson_iter_document(&iter, &child_len, &child);
               assert(child);
               assert(child_len >= 5);
               assert((iter.offset + child_len) < b->len);
               assert(child_len < (bson_uint32_t)-1);
               memcpy(&child_len, child, 4);
               child_len = BSON_UINT32_FROM_LE(child_len);
               assert(child_len >= 5);
            }
            break;
         case BSON_TYPE_DOUBLE:
         case BSON_TYPE_UTF8:
         case BSON_TYPE_BINARY:
         case BSON_TYPE_UNDEFINED:
            break;
         case BSON_TYPE_OID:
            assert(iter.offset + 12 < iter.bson->len);
            break;
         case BSON_TYPE_BOOL:
         case BSON_TYPE_DATE_TIME:
         case BSON_TYPE_NULL:
         case BSON_TYPE_REGEX:
            /* TODO: check for 2 valid cstring. */
         case BSON_TYPE_DBPOINTER:
         case BSON_TYPE_CODE:
         case BSON_TYPE_SYMBOL:
         case BSON_TYPE_CODEWSCOPE:
         case BSON_TYPE_INT32:
         case BSON_TYPE_TIMESTAMP:
         case BSON_TYPE_INT64:
         case BSON_TYPE_MAXKEY:
         case BSON_TYPE_MINKEY:
            break;
         case BSON_TYPE_EOD:
         default:
            /* Code should not be reached. */
            assert(FALSE);
            break;
         }
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


static void
test_bson_iter_next_after_finish (void)
{
   bson_iter_t iter;
   bson_t *b;
   int i;

   b = bson_new();
   bson_append_int32(b, "key", -1, 1234);
   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   for (i = 0; i < 1000; i++) {
      assert(!bson_iter_next(&iter));
   }
   bson_destroy(b);
}


static void
test_bson_iter_find_case (void)
{
   bson_t b;
   bson_iter_t iter;

   bson_init(&b);
   bson_append_utf8(&b, "key", -1, "value", -1);
   assert(bson_iter_init(&iter, &b));
   assert(bson_iter_find_case(&iter, "KEY"));
   assert(bson_iter_init(&iter, &b));
   assert(!bson_iter_find(&iter, "KEY"));
   bson_destroy(&b);
}


static void
test_bson_iter_overwrite_int32 (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init(&b);
   bson_append_int32(&b, "key", -1, 1234);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_INT32(&iter));
   bson_iter_overwrite_int32(&iter, 4321);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_INT32(&iter));
   assert_cmpint(bson_iter_int32(&iter), ==, 4321);
   bson_destroy(&b);
}


static void
test_bson_iter_overwrite_int64 (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init(&b);
   bson_append_int64(&b, "key", -1, 1234);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_INT64(&iter));
   bson_iter_overwrite_int64(&iter, 4641);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_INT64(&iter));
   assert_cmpint(bson_iter_int64(&iter), ==, 4641);
   bson_destroy(&b);
}


static void
test_bson_iter_overwrite_double (void)
{
   bson_iter_t iter;
   bson_t b;

   bson_init(&b);
   bson_append_double(&b, "key", -1, 1234.1234);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_DOUBLE(&iter));
   bson_iter_overwrite_double(&iter, 4641.1234);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_DOUBLE(&iter));
   assert_cmpint(bson_iter_double(&iter), ==, 4641.1234);
   bson_destroy(&b);
}


static void
test_bson_iter_recurse (void)
{
   bson_iter_t iter;
   bson_iter_t child;
   bson_t b;
   bson_t cb;

   bson_init(&b);
   bson_init(&cb);
   bson_append_int32(&cb, "0", 1, 0);
   bson_append_int32(&cb, "1", 1, 1);
   bson_append_int32(&cb, "2", 1, 2);
   bson_append_array(&b, "key", -1, &cb);
   assert(bson_iter_init_find(&iter, &b, "key"));
   assert(BSON_ITER_HOLDS_ARRAY(&iter));
   assert(bson_iter_recurse(&iter, &child));
   assert(bson_iter_find(&child, "0"));
   assert(bson_iter_find(&child, "1"));
   assert(bson_iter_find(&child, "2"));
   assert(!bson_iter_next(&child));
   bson_destroy(&b);
   bson_destroy(&cb);
}


static void
test_bson_iter_init_find_case (void)
{
   bson_t b;
   bson_iter_t iter;

   bson_init(&b);
   bson_append_int32(&b, "FOO", -1, 1234);
   assert(bson_iter_init_find_case(&iter, &b, "foo"));
   assert_cmpint(bson_iter_int32(&iter), ==, 1234);
   bson_destroy(&b);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/iter/test_string", test_bson_iter_utf8);
   run_test("/bson/iter/test_mixed", test_bson_iter_mixed);
   run_test("/bson/iter/test_overflow", test_bson_iter_overflow);
   run_test("/bson/iter/test_trailing_null", test_bson_iter_trailing_null);
   run_test("/bson/iter/test_fuzz", test_bson_iter_fuzz);
   run_test("/bson/iter/test_regex", test_bson_iter_regex);
   run_test("/bson/iter/test_next_after_finish", test_bson_iter_next_after_finish);
   run_test("/bson/iter/test_find_case", test_bson_iter_find_case);
   run_test("/bson/iter/test_overwrite_int32", test_bson_iter_overwrite_int32);
   run_test("/bson/iter/test_overwrite_int64", test_bson_iter_overwrite_int64);
   run_test("/bson/iter/test_overwrite_double", test_bson_iter_overwrite_double);
   run_test("/bson/iter/recurse", test_bson_iter_recurse);
   run_test("/bson/iter/init_find_case", test_bson_iter_init_find_case);

   return 0;
}
