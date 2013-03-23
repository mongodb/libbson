#include <assert.h>
#include <bson/bson.h>
#include <bson/bson-string.h>

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
   bson_append_string(b, "utf8", -1, "bar", -1);
   bson_append_int32(b, "int32", -1, 1234);
   bson_append_int64(b, "int64", -1, 4321);
   bson_append_double(b, "double", -1, 123.4);
   bson_append_undefined(b, "undefined", -1);
   bson_append_null(b, "null", -1);
   bson_append_oid(b, "oid", -1, &oid);
   bson_append_bool(b, "true", -1, TRUE);
   bson_append_bool(b, "false", -1, FALSE);
   bson_append_time_t(b, "date", -1, time(NULL));
   bson_append_timestamp(b, "timestamp", -1, time(NULL), 1234);
   bson_append_regex(b, "regex", -1, "^abcd", "xi");
   bson_append_dbpointer(b, "dbpointer", -1, "mycollection", &oid);
   bson_append_minkey(b, "minkey", -1);
   bson_append_maxkey(b, "maxkey", -1);
   bson_append_symbol(b, "symbol", -1, "var a = {};", -1);

   b2 = bson_new();
   bson_append_int32(b2, "0", -1, 60);
   bson_append_document(b, "document", -1, b2);
   bson_append_array(b, "array", -1, b2);

   {
      const bson_uint8_t binary[] = { 0, 1, 2, 3, 4 };
      bson_append_binary(b, "binary", -1, BSON_SUBTYPE_BINARY,
                         binary, sizeof binary);
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
   bson_append_string(b, "foo", -1, "bar", -1);
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
   bson_append_int32(b, "foo", -1, 1234);
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
   bson_append_int64(b, "foo", -1, 341234123412341234UL);
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
   bson_append_double(b, "foo", -1, 123.456);
   str = bson_as_json(b, &len);
   assert(len == 22);
   assert(!strcmp("{ \"foo\" : 123.456000 }", str));
   bson_free(str);
   bson_destroy(b);
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

   return 0;
}
