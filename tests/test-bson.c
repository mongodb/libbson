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
#include <bson/bson-private.h>
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
   char real_filename[256];
   int fd;

   snprintf(real_filename, sizeof real_filename,
            "tests/binary/%s", filename);
   real_filename[sizeof real_filename - 1] = '\0';

   if (-1 == (fd = open(real_filename, O_RDONLY))) {
      fprintf(stderr, "Failed to open: %s\n", real_filename);
      abort();
   }
   len = read(fd, buf, sizeof buf);
   b = bson_new_from_data(buf, len);
   close(fd);

   return b;
}


static void
test_bson_new (void)
{
   bson_t *b;

   b = bson_new();
   assert_cmpint(b->len, ==, 5);
   bson_destroy(b);

   b = bson_sized_new(32);
   assert_cmpint(b->len, ==, 5);
   bson_destroy(b);
}


static void
test_bson_alloc (void)
{
   static const bson_uint8_t empty_bson[] = { 5, 0, 0, 0, 0 };
   bson_t *b;

   b = bson_new();
   assert_cmpint(b->len, ==, 5);
   assert((b->flags & BSON_FLAG_INLINE));
   assert(!(b->flags & BSON_FLAG_CHILD));
   assert(!(b->flags & BSON_FLAG_STATIC));
   assert(!(b->flags & BSON_FLAG_NO_FREE));
   bson_destroy(b);

   /*
    * This checks that we fit in the inline buffer size.
    */
   b = bson_sized_new(44);
   assert_cmpint(b->len, ==, 5);
   assert((b->flags & BSON_FLAG_INLINE));
   bson_destroy(b);

   /*
    * Make sure we grow to next power of 2.
    */
   b = bson_sized_new(121);
   assert_cmpint(b->len, ==, 5);
   assert(!(b->flags & BSON_FLAG_INLINE));
   bson_destroy(b);

   /*
    * Make sure we grow to next power of 2.
    */
   b = bson_sized_new(129);
   assert_cmpint(b->len, ==, 5);
   assert(!(b->flags & BSON_FLAG_INLINE));
   bson_destroy(b);

   b = bson_new_from_data(empty_bson, sizeof empty_bson);
   assert_cmpint(b->len, ==, sizeof empty_bson);
   assert((b->flags & BSON_FLAG_INLINE));
   assert(!memcmp(bson_get_data(b), empty_bson, sizeof empty_bson));
   bson_destroy(b);
}


static void
assert_bson_equal (const bson_t *a,
                   const bson_t *b)
{
   const bson_uint8_t *data1 = bson_get_data(a);
   const bson_uint8_t *data2 = bson_get_data(b);
   bson_uint32_t i;

   if (!bson_equal(a, b)) {
      for (i = 0; i < MAX(a->len, b->len); i++) {
         if (i >= a->len) {
            printf("a is too short len=%u\n", a->len);
            abort();
         } else if (i >= b->len) {
            printf("b is too short len=%u\n", b->len);
            abort();
         }
         if (data1[i] != data2[i]) {
            printf("a[%u](0x%02x,%u) != b[%u](0x%02x,%u)\n",
                   i, data1[i], data1[i], i, data2[i], data2[i]);
            abort();
         }
      }
   }
}


static void
assert_bson_equal_file (const bson_t *b,
                        const char   *filename)
{
   bson_t *b2 = get_bson(filename);
   assert_bson_equal(b, b2);
   bson_destroy(b2);
}


static void
test_bson_append_utf8 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   b2 = get_bson("test11.bson");
   assert(bson_append_utf8(b, "hello", -1, "world", -1));
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_symbol (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   b2 = get_bson("test32.bson");
   assert(bson_append_symbol(b, "hello", -1, "world", -1));
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_null (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_null(b, "hello", -1));
   b2 = get_bson("test18.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_bool (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_bool(b, "bool", -1, TRUE));
   b2 = get_bson("test19.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_double (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_double(b, "double", -1, 123.4567));
   b2 = get_bson("test20.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_document (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   b2 = bson_new();
   assert(bson_append_document(b, "document", -1, b2));
   bson_destroy(b2);
   b2 = get_bson("test21.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_oid (void)
{
   bson_oid_t oid;
   bson_t *b;
   bson_t *b2;

   bson_oid_init_from_string(&oid, "1234567890abcdef1234abcd");

   b = bson_new();
   assert(bson_append_oid(b, "oid", -1, &oid));
   b2 = get_bson("test22.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_array (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   b2 = bson_new();
   assert(bson_append_utf8(b2, "0", -1, "hello", -1));
   assert(bson_append_utf8(b2, "1", -1, "world", -1));
   assert(bson_append_array(b, "array", -1, b2));
   bson_destroy(b2);
   b2 = get_bson("test23.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_binary (void)
{
   const static bson_uint8_t binary[] = { '1', '2', '3', '4' };
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_binary(b, "binary", -1, BSON_SUBTYPE_USER, binary, 4));
   b2 = get_bson("test24.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_time_t (void)
{
   bson_t *b;
   bson_t *b2;
   time_t t;

   t = 1234567890;

   b = bson_new();
   assert(bson_append_time_t(b, "time_t", -1, t));
   b2 = get_bson("test26.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_timeval (void)
{
   struct timeval tv = { 0 };
   bson_t *b;
   bson_t *b2;

   tv.tv_sec = 1234567890;
   tv.tv_usec = 0;

   b = bson_new();
   assert(bson_append_timeval(b, "time_t", -1, &tv));
   b2 = get_bson("test26.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_undefined (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_undefined(b, "undefined", -1));
   b2 = get_bson("test25.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_regex (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_regex(b, "regex", -1, "^abcd", "ilx"));
   b2 = get_bson("test27.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_code (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_code(b, "code", -1, "var a = {};"));
   b2 = get_bson("test29.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_code_with_scope (void)
{
   bson_t *b;
   bson_t *b2;
   bson_t *scope;

   /* Test with empty bson, which converts to just CODE type. */
   b = bson_new();
   scope = bson_new();
   assert(bson_append_code_with_scope(b, "code", -1, "var a = {};", scope));
   b2 = get_bson("test30.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
   bson_destroy(scope);


   /* Test with non-empty scope */
   b = bson_new();
   scope = bson_new();
   assert(bson_append_utf8(scope, "foo", -1, "bar", -1));
   assert(bson_append_code_with_scope(b, "code", -1, "var a = {};", scope));
   b2 = get_bson("test31.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
   bson_destroy(scope);
}


static void
test_bson_append_dbpointer (void)
{
   bson_oid_t oid;
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   bson_oid_init_from_string(&oid, "0123abcd0123abcd0123abcd");
   assert(bson_append_dbpointer(b, "dbpointer", -1, "foo", &oid));
   b2 = get_bson("test28.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_int32 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_int32(b, "a", -1, -123));
   assert(bson_append_int32(b, "c", -1, 0));
   assert(bson_append_int32(b, "b", -1, 123));
   b2 = get_bson("test33.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_int64 (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_int64(b, "a", -1, 100000000000000));
   b2 = get_bson("test34.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_iter (void)
{
   bson_iter_t iter;
   bson_bool_t r;
   bson_t b;
   bson_t c;

   bson_init(&b);
   bson_append_int32(&b, "a", 1, 1);
   bson_append_int32(&b, "b", 1, 2);
   bson_append_int32(&b, "c", 1, 3);
   bson_append_utf8(&b, "d", 1, "hello", 5);

   bson_init(&c);

   r = bson_iter_init_find(&iter, &b, "a");
   assert(r);
   r = bson_append_iter(&c, NULL, 0, &iter);
   assert(r);

   r = bson_iter_init_find(&iter, &b, "c");
   assert(r);
   r = bson_append_iter(&c, NULL, 0, &iter);
   assert(r);

   r = bson_iter_init_find(&iter, &b, "d");
   assert(r);
   r = bson_append_iter(&c, "world", -1, &iter);
   assert(r);

   bson_iter_init(&iter, &c);
   r = bson_iter_next(&iter);
   assert(r);
   assert_cmpstr("a", bson_iter_key(&iter));
   assert_cmpint(BSON_TYPE_INT32, ==, bson_iter_type(&iter));
   assert_cmpint(1, ==, bson_iter_int32(&iter));
   r = bson_iter_next(&iter);
   assert(r);
   assert_cmpstr("c", bson_iter_key(&iter));
   assert_cmpint(BSON_TYPE_INT32, ==, bson_iter_type(&iter));
   assert_cmpint(3, ==, bson_iter_int32(&iter));
   r = bson_iter_next(&iter);
   assert(r);
   assert_cmpint(BSON_TYPE_UTF8, ==, bson_iter_type(&iter));
   assert_cmpstr("world", bson_iter_key(&iter));
   assert_cmpstr("hello", bson_iter_utf8(&iter, NULL));

   bson_destroy(&b);
   bson_destroy(&c);
}


static void
test_bson_append_timestamp (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_timestamp(b, "timestamp", -1, 1234, 9876));
   b2 = get_bson("test35.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_maxkey (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_maxkey(b, "maxkey", -1));
   b2 = get_bson("test37.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_minkey (void)
{
   bson_t *b;
   bson_t *b2;

   b = bson_new();
   assert(bson_append_minkey(b, "minkey", -1));
   b2 = get_bson("test36.bson");
   assert_bson_equal(b, b2);
   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_append_general (void)
{
   bson_uint8_t bytes[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x01, 0x23, 0x45 };
   bson_oid_t oid;
   bson_t *bson;
   bson_t *array;
   bson_t *subdoc;

   bson = bson_new();
   assert(bson_append_int32(bson, "int", -1, 1));
   assert_bson_equal_file(bson, "test1.bson");
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_int64(bson, "int64", -1, 1));
   assert_bson_equal_file(bson, "test2.bson");
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_double(bson, "double", -1, 1.123));
   assert_bson_equal_file(bson, "test3.bson");
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_utf8(bson, "string", -1, "some string", -1));
   assert_bson_equal_file(bson, "test5.bson");
   bson_destroy(bson);

   bson = bson_new();
   array = bson_new();
   assert(bson_append_int32(array, "0", -1, 1));
   assert(bson_append_int32(array, "1", -1, 2));
   assert(bson_append_int32(array, "2", -1, 3));
   assert(bson_append_int32(array, "3", -1, 4));
   assert(bson_append_int32(array, "4", -1, 5));
   assert(bson_append_int32(array, "5", -1, 6));
   assert(bson_append_array(bson, "array[int]", -1, array));
   assert_bson_equal_file(bson, "test6.bson");
   bson_destroy(array);
   bson_destroy(bson);

   bson = bson_new();
   array = bson_new();
   assert(bson_append_double(array, "0", -1, 1.123));
   assert(bson_append_double(array, "1", -1, 2.123));
   assert(bson_append_array(bson, "array[double]", -1, array));
   assert_bson_equal_file(bson, "test7.bson");
   bson_destroy(array);
   bson_destroy(bson);

   bson = bson_new();
   subdoc = bson_new();
   assert(bson_append_int32(subdoc, "int", -1, 1));
   assert(bson_append_document(bson, "document", -1, subdoc));
   assert_bson_equal_file(bson, "test8.bson");
   bson_destroy(subdoc);
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_null(bson, "null", -1));
   assert_bson_equal_file(bson, "test9.bson");
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_regex(bson, "regex", -1, "1234", "i"));
   assert_bson_equal_file(bson, "test10.bson");
   bson_destroy(bson);

   bson = bson_new();
   assert(bson_append_utf8(bson, "hello", -1, "world", -1));
   assert_bson_equal_file(bson, "test11.bson");
   bson_destroy(bson);

   bson = bson_new();
   array = bson_new();
   assert(bson_append_utf8(array, "0", -1, "awesome", -1));
   assert(bson_append_double(array, "1", -1, 5.05));
   assert(bson_append_int32(array, "2", -1, 1986));
   assert(bson_append_array(bson, "BSON", -1, array));
   assert_bson_equal_file(bson, "test12.bson");
   bson_destroy(bson);
   bson_destroy(array);

   bson = bson_new();
   memcpy(&oid, bytes, sizeof oid);
   assert(bson_append_oid(bson, "_id", -1, &oid));
   subdoc = bson_new();
   assert(bson_append_oid(subdoc, "_id", -1, &oid));
   array = bson_new();
   assert(bson_append_utf8(array, "0", -1, "1", -1));
   assert(bson_append_utf8(array, "1", -1, "2", -1));
   assert(bson_append_utf8(array, "2", -1, "3", -1));
   assert(bson_append_utf8(array, "3", -1, "4", -1));
   assert(bson_append_array(subdoc, "tags", -1, array));
   bson_destroy(array);
   assert(bson_append_utf8(subdoc, "text", -1, "asdfanother", -1));
   array = bson_new();
   assert(bson_append_utf8(array, "name", -1, "blah", -1));
   assert(bson_append_document(subdoc, "source", -1, array));
   bson_destroy(array);
   assert(bson_append_document(bson, "document", -1, subdoc));
   bson_destroy(subdoc);
   array = bson_new();
   assert(bson_append_utf8(array, "0", -1, "source", -1));
   assert(bson_append_array(bson, "type", -1, array));
   bson_destroy(array);
   array = bson_new();
   assert(bson_append_utf8(array, "0", -1, "server_created_at", -1));
   assert(bson_append_array(bson, "missing", -1, array));
   bson_destroy(array);
   assert_bson_equal_file(bson, "test17.bson");
   bson_destroy(bson);
}


static void
test_bson_append_deep (void)
{
   bson_t *a;
   bson_t *tmp;
   int i;

   a = bson_new();

   for (i = 0; i < 100; i++) {
      tmp = a;
      a = bson_new();
      assert(bson_append_document(a, "a", -1, tmp));
      bson_destroy(tmp);
   }

   assert_bson_equal_file(a, "test38.bson");

   bson_destroy(a);
}


static void
test_bson_validate (void)
{
   char filename[64];
   size_t offset;
   bson_t *b;
   int i;

   for (i = 1; i <= 38; i++) {
      snprintf(filename, sizeof filename, "test%u.bson", i);
      b = get_bson(filename);
      assert(bson_validate(b, BSON_VALIDATE_NONE, &offset));
      bson_destroy(b);
   }

   b = get_bson("overflow2.bson");
   assert(!bson_validate(b, BSON_VALIDATE_NONE, &offset));
   assert(offset == 9);
   bson_destroy(b);

   b = get_bson("trailingnull.bson");
   assert(!bson_validate(b, BSON_VALIDATE_NONE, &offset));
   assert(offset == 14);
   bson_destroy(b);

   b = get_bson("dollarquery.bson");
   assert(!bson_validate(b, BSON_VALIDATE_DOLLAR_KEYS, &offset));
   bson_destroy(b);

   b = get_bson("dotquery.bson");
   assert(!bson_validate(b, BSON_VALIDATE_DOT_KEYS, &offset));
   bson_destroy(b);

   b = get_bson("overflow3.bson");
   assert(!bson_validate(b, BSON_VALIDATE_NONE, &offset));
   assert(offset == 9);
   bson_destroy(b);

   b = get_bson("overflow4.bson");
   assert(!bson_validate(b, BSON_VALIDATE_NONE, &offset));
   bson_destroy(b);
}


static void
test_bson_init (void)
{
   bson_t b;
   char key[12];
   int i;

   bson_init(&b);
   assert((b.flags & BSON_FLAG_INLINE));
   assert((b.flags & BSON_FLAG_STATIC));
   assert(!(b.flags & BSON_FLAG_RDONLY));
   for (i = 0; i < 100; i++) {
      snprintf(key, sizeof key, "%d", i);
      assert(bson_append_utf8(&b, key, -1, "bar", -1));
   }
   assert(!(b.flags & BSON_FLAG_INLINE));
   bson_destroy(&b);
}


static void
test_bson_init_static (void)
{
   static const bson_uint8_t data[5] = { 5 };
   bson_t b;

   bson_init_static(&b, data, sizeof data);
   assert((b.flags & BSON_FLAG_RDONLY));
   bson_destroy(&b);
}


static void
test_bson_utf8_key (void)
{
   bson_uint32_t length;
   bson_iter_t iter;
   const char *str;
   bson_t *b;
   size_t offset;

   b = get_bson("eurokey.bson");
   assert(bson_validate(b, BSON_VALIDATE_NONE, &offset));
   assert(bson_iter_init(&iter, b));
   assert(bson_iter_next(&iter));
   assert(!strcmp(bson_iter_key(&iter), "€€€€€"));
   assert((str = bson_iter_utf8(&iter, &length)));
   assert(length == 15); /* 5 3-byte sequences. */
   assert(!strcmp(str, "€€€€€"));
   bson_destroy(b);
}


static void
test_bson_new_1mm (void)
{
   bson_t *b;
   int i;

   for (i = 0; i < 1000000; i++) {
      b = bson_new();
      bson_destroy(b);
   }
}


static void
test_bson_init_1mm (void)
{
   bson_t b;
   int i;

   for (i = 0; i < 1000000; i++) {
      bson_init(&b);
      bson_destroy(&b);
   }
}


static void
test_bson_build_child (void)
{
   bson_t b;
   bson_t child;
   bson_t *b2;
   bson_t *child2;

   bson_init(&b);
   assert(bson_append_document_begin(&b, "foo", -1, &child));
   assert(bson_append_utf8(&child, "bar", -1, "baz", -1));
   assert(bson_append_document_end(&b, &child));

   b2 = bson_new();
   child2 = bson_new();
   assert(bson_append_utf8(child2, "bar", -1, "baz", -1));
   assert(bson_append_document(b2, "foo", -1, child2));
   bson_destroy(child2);

   assert(b.len == b2->len);
   assert_bson_equal(&b, b2);

   bson_destroy(&b);
   bson_destroy(b2);
}


static void
test_bson_build_child_array (void)
{
   bson_t b;
   bson_t child;
   bson_t *b2;
   bson_t *child2;

   bson_init(&b);
   assert(bson_append_array_begin(&b, "foo", -1, &child));
   assert(bson_append_utf8(&child, "0", -1, "baz", -1));
   assert(bson_append_array_end(&b, &child));

   b2 = bson_new();
   child2 = bson_new();
   assert(bson_append_utf8(child2, "0", -1, "baz", -1));
   assert(bson_append_array(b2, "foo", -1, child2));
   bson_destroy(child2);

   assert(b.len == b2->len);
   assert_bson_equal(&b, b2);

   bson_destroy(&b);
   bson_destroy(b2);
}


static void
test_bson_build_child_deep_1 (bson_t *b,
                              int    *count)
{
   bson_t child;

   (*count)++;

   assert(bson_append_document_begin(b, "b", -1, &child));
   assert(!(b->flags & BSON_FLAG_INLINE));
   assert((b->flags & BSON_FLAG_IN_CHILD));
   assert(!(child.flags & BSON_FLAG_INLINE));
   assert((child.flags & BSON_FLAG_STATIC));
   assert((child.flags & BSON_FLAG_CHILD));

   if (*count < 100) {
      test_bson_build_child_deep_1(&child, count);
   } else {
      assert(bson_append_int32(&child, "b", -1, 1234));
   }

   assert(bson_append_document_end(b, &child));
   assert(!(b->flags & BSON_FLAG_IN_CHILD));
}


static void
test_bson_build_child_deep (void)
{
   bson_t b;
   int count = 0;

   bson_init(&b);
   assert((b.flags & BSON_FLAG_INLINE));
   test_bson_build_child_deep_1(&b, &count);
   assert(!(b.flags & BSON_FLAG_INLINE));
   assert((b.flags & BSON_FLAG_STATIC));
   assert(!(b.flags & BSON_FLAG_NO_FREE));
   assert(!(b.flags & BSON_FLAG_RDONLY));
   assert(bson_validate(&b, BSON_VALIDATE_NONE, NULL));
   assert(((bson_impl_alloc_t *)&b)->alloclen == 1024);
   assert_bson_equal_file(&b, "test39.bson");
   bson_destroy(&b);
}


static void
test_bson_build_child_deep_no_begin_end_1 (bson_t *b,
                                           int    *count)
{
   bson_t child;

   (*count)++;

   bson_init(&child);
   if (*count < 100) {
      test_bson_build_child_deep_1(&child, count);
   } else {
      assert(bson_append_int32(&child, "b", -1, 1234));
   }
   assert(bson_append_document(b, "b", -1, &child));
   bson_destroy(&child);
}


static void
test_bson_build_child_deep_no_begin_end (void)
{
   bson_t b;
   int count = 0;

   bson_init(&b);
   test_bson_build_child_deep_no_begin_end_1(&b, &count);
   assert(bson_validate(&b, BSON_VALIDATE_NONE, NULL));
   assert(((bson_impl_alloc_t *)&b)->alloclen == 1024);
   assert_bson_equal_file(&b, "test39.bson");
   bson_destroy(&b);
}


static void
test_bson_count_keys (void)
{
   bson_t b;

   bson_init(&b);
   assert(bson_append_int32(&b, "0", -1, 0));
   assert(bson_append_int32(&b, "1", -1, 1));
   assert(bson_append_int32(&b, "2", -1, 2));
   assert_cmpint(bson_count_keys(&b), ==, 3);
   bson_destroy(&b);
}


static void
test_bson_copy (void)
{
   bson_t b;
   bson_t *c;

   bson_init(&b);
   assert(bson_append_int32(&b, "foobar", -1, 1234));
   c = bson_copy(&b);
   assert_bson_equal(&b, c);
   bson_destroy(c);
   bson_destroy(&b);
}


static void
test_bson_copy_to (void)
{
   bson_t b;
   bson_t c;
   int i;

   /*
    * Test inline structure copy.
    */
   bson_init(&b);
   assert(bson_append_int32(&b, "foobar", -1, 1234));
   bson_copy_to(&b, &c);
   assert_bson_equal(&b, &c);
   bson_destroy(&c);
   bson_destroy(&b);

   /*
    * Test malloced copy.
    */
   bson_init(&b);
   for (i = 0; i < 1000; i++) {
      assert(bson_append_int32(&b, "foobar", -1, 1234));
   }
   bson_copy_to(&b, &c);
   assert_bson_equal(&b, &c);
   bson_destroy(&c);
   bson_destroy(&b);
}


static void
test_bson_copy_to_excluding (void)
{
   bson_iter_t iter;
   bson_bool_t r;
   bson_t b;
   bson_t c;
   int i;

   bson_init(&b);
   bson_append_int32(&b, "a", 1, 1);
   bson_append_int32(&b, "b", 1, 2);

   bson_copy_to_excluding(&b, &c, "b", NULL);
   r = bson_iter_init_find(&iter, &c, "a");
   assert(r);
   r = bson_iter_init_find(&iter, &c, "b");
   assert(!r);

   i = bson_count_keys(&b);
   assert_cmpint(i, ==, 2);

   i = bson_count_keys(&c);
   assert_cmpint(i, ==, 1);

   bson_destroy(&b);
   bson_destroy(&c);
}


static void
test_bson_append_overflow (void)
{
   const char *key = "a";
   size_t len;
   bson_t b;

   len = BSON_MAX_SIZE;
   len -= 4; /* len */
   len -= 1; /* type */
   len -= 1; /* value */
   len -= 1; /* end byte */

   bson_init(&b);
   assert(!bson_append_bool(&b, key, len, TRUE));
   bson_destroy(&b);
}


static void
test_bson_initializer (void)
{
   bson_t b = BSON_INITIALIZER;

   assert(bson_empty(&b));
   bson_append_bool(&b, "foo", -1, TRUE);
   assert(!bson_empty(&b));
   bson_destroy(&b);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/new", test_bson_new);
   run_test("/bson/init", test_bson_init);
   run_test("/bson/init_static", test_bson_init_static);
   run_test("/bson/basic", test_bson_alloc);
   run_test("/bson/append_overflow", test_bson_append_overflow);
   run_test("/bson/append_array", test_bson_append_array);
   run_test("/bson/append_binary", test_bson_append_binary);
   run_test("/bson/append_bool", test_bson_append_bool);
   run_test("/bson/append_code", test_bson_append_code);
   run_test("/bson/append_code_with_scope", test_bson_append_code_with_scope);
   run_test("/bson/append_dbpointer", test_bson_append_dbpointer);
   run_test("/bson/append_document", test_bson_append_document);
   run_test("/bson/append_double", test_bson_append_double);
   run_test("/bson/append_int32", test_bson_append_int32);
   run_test("/bson/append_int64", test_bson_append_int64);
   run_test("/bson/append_iter", test_bson_append_iter);
   run_test("/bson/append_maxkey", test_bson_append_maxkey);
   run_test("/bson/append_minkey", test_bson_append_minkey);
   run_test("/bson/append_null", test_bson_append_null);
   run_test("/bson/append_oid", test_bson_append_oid);
   run_test("/bson/append_regex", test_bson_append_regex);
   run_test("/bson/append_utf8", test_bson_append_utf8);
   run_test("/bson/append_symbol", test_bson_append_symbol);
   run_test("/bson/append_time_t", test_bson_append_time_t);
   run_test("/bson/append_timestamp", test_bson_append_timestamp);
   run_test("/bson/append_timeval", test_bson_append_timeval);
   run_test("/bson/append_undefined", test_bson_append_undefined);
   run_test("/bson/append_general", test_bson_append_general);
   run_test("/bson/append_deep", test_bson_append_deep);
   run_test("/bson/utf8_key", test_bson_utf8_key);
   run_test("/bson/validate", test_bson_validate);
   run_test("/bson/new_1mm", test_bson_new_1mm);
   run_test("/bson/init_1mm", test_bson_init_1mm);
   run_test("/bson/build_child", test_bson_build_child);
   run_test("/bson/build_child_deep", test_bson_build_child_deep);
   run_test("/bson/build_child_deep_no_begin_end", test_bson_build_child_deep_no_begin_end);
   run_test("/bson/build_child_array", test_bson_build_child_array);
   run_test("/bson/count", test_bson_count_keys);
   run_test("/bson/copy", test_bson_copy);
   run_test("/bson/copy_to", test_bson_copy_to);
   run_test("/bson/copy_to_excluding", test_bson_copy_to_excluding);
   run_test("/bson/initializer", test_bson_initializer);

   return 0;
}
