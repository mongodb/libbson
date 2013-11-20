#include <assert.h>
#include <bson/bson.h>
#include <bson/bson-string.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "bson-tests.h"


static void
test_bson_as_json (void)
{
   bson_oid_t oid;
   bson_t *b;
   bson_t *b2;
   char *str;
   size_t len;
   int i;

   bson_oid_init_from_string(&oid, "123412341234abcdabcdabcd");

   b = bson_new();
   assert(bson_append_utf8(b, "utf8", -1, "bar", -1));
   assert(bson_append_int32(b, "int32", -1, 1234));
   assert(bson_append_int64(b, "int64", -1, 4321));
   assert(bson_append_double(b, "double", -1, 123.4));
   assert(bson_append_undefined(b, "undefined", -1));
   assert(bson_append_null(b, "null", -1));
   assert(bson_append_oid(b, "oid", -1, &oid));
   assert(bson_append_bool(b, "true", -1, TRUE));
   assert(bson_append_bool(b, "false", -1, FALSE));
   assert(bson_append_time_t(b, "date", -1, time(NULL)));
   assert(bson_append_timestamp(b, "timestamp", -1, time(NULL), 1234));
   assert(bson_append_regex(b, "regex", -1, "^abcd", "xi"));
   assert(bson_append_dbpointer(b, "dbpointer", -1, "mycollection", &oid));
   assert(bson_append_minkey(b, "minkey", -1));
   assert(bson_append_maxkey(b, "maxkey", -1));
   assert(bson_append_symbol(b, "symbol", -1, "var a = {};", -1));

   b2 = bson_new();
   assert(bson_append_int32(b2, "0", -1, 60));
   assert(bson_append_document(b, "document", -1, b2));
   assert(bson_append_array(b, "array", -1, b2));

   {
      const bson_uint8_t binary[] = { 0, 1, 2, 3, 4 };
      assert(bson_append_binary(b, "binary", -1, BSON_SUBTYPE_BINARY,
                                binary, sizeof binary));
   }

   for (i = 0; i < 1000; i++) {
      str = bson_as_json(b, &len);
      bson_free(str);
   }

   bson_destroy(b);
   bson_destroy(b2);
}


static void
test_bson_as_json_string (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new();
   assert(bson_append_utf8(b, "foo", -1, "bar", -1));
   str = bson_as_json(b, &len);
   assert(len == 17);
   assert(!strcmp("{ \"foo\" : \"bar\" }", str));
   bson_free(str);
   bson_destroy(b);
}


static void
test_bson_as_json_int32 (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new();
   assert(bson_append_int32(b, "foo", -1, 1234));
   str = bson_as_json(b, &len);
   assert(len == 16);
   assert(!strcmp("{ \"foo\" : 1234 }", str));
   bson_free(str);
   bson_destroy(b);
}


static void
test_bson_as_json_int64 (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new();
   assert(bson_append_int64(b, "foo", -1, 341234123412341234ULL));
   str = bson_as_json(b, &len);
   assert(len == 30);
   assert(!strcmp("{ \"foo\" : 341234123412341234 }", str));
   bson_free(str);
   bson_destroy(b);
}


static void
test_bson_as_json_double (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new();
   assert(bson_append_double(b, "foo", -1, 123.456));
   str = bson_as_json(b, &len);
   assert(len >= 19);
   assert(!strncmp("{ \"foo\" : 123.456", str, 17));
   assert(!strcmp(" }", str + len - 2));
   bson_free(str);
   bson_destroy(b);
}


static void
test_bson_as_json_utf8 (void)
{
   size_t len;
   bson_t *b;
   char *str;

   b = bson_new();
   assert(bson_append_utf8(b, "€€€€€", -1, "€€€€€", -1));
   str = bson_as_json(b, &len);
   assert(!strcmp(str, "{ \"€€€€€\" : \"€€€€€\" }"));
   bson_free(str);
   bson_destroy(b);
}


static void
test_bson_as_json_stack_overflow (void)
{
   bson_uint8_t *buf;
   bson_t b;
   size_t buflen = 1024 * 1024 * 17;
   char *str;
   int fd;
   int r;

   buf = bson_malloc0(buflen);

   fd = open("tests/binary/stackoverflow.bson", O_RDONLY);
   BSON_ASSERT(fd != -1);

   r = read(fd, buf, buflen);
   BSON_ASSERT(r == 16777220);

   r = bson_init_static(&b, buf, 16777220);
   BSON_ASSERT(r);

   str = bson_as_json(&b, NULL);
   BSON_ASSERT(str);

   r = !!strstr(str, "...");
   BSON_ASSERT(str);

   bson_free(str);
   bson_destroy(&b);
   bson_free(buf);
}


static void
test_bson_corrupt (void)
{
   bson_uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   int r;

   buf = bson_malloc0(buflen);

   fd = open("tests/binary/test55.bson", O_RDONLY);
   BSON_ASSERT(fd != -1);

   r = read(fd, buf, buflen);
   BSON_ASSERT(r == 24);

   r = bson_init_static(&b, buf, r);
   BSON_ASSERT(r);

   str = bson_as_json(&b, NULL);
   BSON_ASSERT(!str);

   bson_destroy(&b);
   bson_free(buf);
}

static void
test_bson_corrupt_utf8 (void)
{
   bson_uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   int r;

   buf = bson_malloc0(buflen);

   fd = open("tests/binary/test56.bson", O_RDONLY);
   BSON_ASSERT(fd != -1);

   r = read(fd, buf, buflen);
   BSON_ASSERT(r == 42);

   r = bson_init_static(&b, buf, r);
   BSON_ASSERT(r);

   str = bson_as_json(&b, NULL);
   BSON_ASSERT(!str);

   bson_destroy(&b);
   bson_free(buf);
}


static void
test_bson_corrupt_binary (void)
{
   bson_uint8_t *buf;
   bson_t b;
   size_t buflen = 1024;
   char *str;
   int fd;
   int r;

   buf = bson_malloc0(buflen);

   fd = open("tests/binary/test57.bson", O_RDONLY);
   BSON_ASSERT(fd != -1);

   r = read(fd, buf, buflen);
   BSON_ASSERT(r == 26);

   r = bson_init_static(&b, buf, r);
   BSON_ASSERT(r);

   str = bson_as_json(&b, NULL);
   BSON_ASSERT(!str);

   bson_destroy(&b);
   bson_free(buf);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/as_json/x1000", test_bson_as_json);
   run_test("/bson/as_json/string", test_bson_as_json_string);
   run_test("/bson/as_json/int32", test_bson_as_json_int32);
   run_test("/bson/as_json/int64", test_bson_as_json_int64);
   run_test("/bson/as_json/double", test_bson_as_json_double);
   run_test("/bson/as_json/utf8", test_bson_as_json_utf8);
   run_test("/bson/as_json/stack_overflow", test_bson_as_json_stack_overflow);
   run_test("/bson/as_json/corrupt", test_bson_corrupt);
   run_test("/bson/as_json/corrupt_utf8", test_bson_corrupt_utf8);
   run_test("/bson/as_json/corrupt_binary", test_bson_corrupt_binary);

   return 0;
}
