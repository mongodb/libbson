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
#include <bson/bson-string.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#include "bson-tests.h"


static void
test_bson_string_new (void)
{
   bson_string_t *str;
   char *s;

   str = bson_string_new(NULL);
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, ""));
   bson_free(s);

   str = bson_string_new("");
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, ""));
   bson_free(s);

   str = bson_string_new("abcdef");
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, "abcdef"));
   bson_free(s);

   str = bson_string_new("");
   s = bson_string_free(str, TRUE);
   assert(!s);
}


static void
test_bson_string_append (void)
{
   bson_string_t *str;
   char *s;

   str = bson_string_new(NULL);
   bson_string_append(str, "christian was here");
   bson_string_append(str, "\n");
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, "christian was here\n"));
   bson_free(s);

   str = bson_string_new(">>>");
   bson_string_append(str, "^^^");
   bson_string_append(str, "<<<");
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, ">>>^^^<<<"));
   bson_free(s);
}


static void
test_bson_string_append_c (void)
{
   bson_string_t *str;
   char *s;

   str = bson_string_new(NULL);
   bson_string_append_c(str, 'c');
   bson_string_append_c(str, 'h');
   bson_string_append_c(str, 'r');
   bson_string_append_c(str, 'i');
   bson_string_append_c(str, 's');
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, "chris"));
   bson_free(s);
}


static void
test_bson_string_append_unichar (void)
{
   static const char test1[] = {0xe2, 0x82, 0xac, 0};
   bson_string_t *str;
   char *s;

   str = bson_string_new(NULL);
   bson_string_append_unichar(str, 0x20AC);
   s = bson_string_free(str, FALSE);
   assert(s);
   assert(!strcmp(s, test1));
   bson_free(s);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/string/new", test_bson_string_new);
   run_test("/bson/string/append", test_bson_string_append);
   run_test("/bson/string/append_c", test_bson_string_append_c);
   run_test("/bson/string/append_unichar", test_bson_string_append_unichar);

   return 0;
}
