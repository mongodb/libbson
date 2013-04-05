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

#include "bson-tests.h"


static void
test_bson_utf8_validate (void)
{
   assert(bson_utf8_validate("asdf", -1, FALSE));
   assert(bson_utf8_validate("asdf", 4, FALSE));
   assert(bson_utf8_validate("asdf", 5, TRUE));
   assert(!bson_utf8_validate("asdf", 5, FALSE));
}


static void
test_bson_utf8_escape_for_json (void)
{
   char *str;

   str = bson_utf8_escape_for_json("my\0key", 6);
   assert(0 == memcmp(str, "my\0key", 7));
   bson_free(str);

   str = bson_utf8_escape_for_json("my\"key", 6);
   assert(0 == memcmp(str, "my\\\"key", 8));
   bson_free(str);

   str = bson_utf8_escape_for_json("my\\key", 6);
   assert(0 == memcmp(str, "my\\\\key", 8));
   bson_free(str);

   str = bson_utf8_escape_for_json("\\\"\\\"", 5);
   assert(0 == memcmp(str, "\\\\\\\"\\\\\\\"", 9));
   bson_free(str);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/utf8/validate", test_bson_utf8_validate);
   run_test("/bson/utf8/escape_for_json", test_bson_utf8_escape_for_json);

   return 0;
}
