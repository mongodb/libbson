/*
 * Copyright 2016 MongoDB, Inc.
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


#include <bson.h>

#include "TestSuite.h"
#include "json-test.h"

#include <limits.h>

#ifdef _MSC_VER
#define PATH_MAX 1024
#define realpath(path, expanded) GetFullPathName (path, PATH_MAX, expanded, NULL)
#endif

typedef void (*test_bson_type_valid_cb)(const uint8_t *bson_str,
                                        uint32_t       bson_str_len,
                                        const uint8_t *canonical_bson_str,
                                        uint32_t       canonical_bson_str_len,
                                        const uint8_t *extjson_str,
                                        uint32_t       extjson_str_len,
                                        const uint8_t *canonical_extjson_str,
                                        uint32_t       canonical_extjson_str_len,
                                        bool           lossy);

#ifdef _MSC_VER
# define SSCANF sscanf_s
#else
# define SSCANF sscanf
#endif

#ifndef JSON_DIR
# define JSON_DIR "tests/json"
#endif

void
test_bson_type_decimal128 (const uint8_t *bson_str,
                           uint32_t       bson_str_len,
                           const uint8_t *canonical_bson_str,
                           uint32_t       canonical_bson_str_len,
                           const uint8_t *extjson_str,
                           uint32_t       extjson_str_len,
                           const uint8_t *canonical_extjson_str,
                           uint32_t       canonical_extjson_str_len,
                           bool           lossy)
{
   char bson_string[BSON_DECIMAL128_STRING];
   char json_string[BSON_DECIMAL128_STRING];
   bson_t bson;
   bson_t canonical_bson;
   bson_t *extjson;
   bson_t *canonical_extjson;
   bson_decimal128_t bson_decimal128;
   bson_decimal128_t json_decimal128;
   bson_iter_t iter;
   char *str1, *str2;
   bson_error_t error;

   BSON_ASSERT (bson_str);
   BSON_ASSERT (canonical_bson_str);
   BSON_ASSERT (extjson_str);
   BSON_ASSERT (canonical_extjson_str);

   bson_init_static (&bson, bson_str, bson_str_len);
   bson_init_static (&canonical_bson, canonical_bson_str, canonical_bson_str_len);


   ASSERT_CMPUINT8 (bson_get_data (&bson), bson_str);
   /* B->cB */
   ASSERT_CMPUINT8 (bson_get_data (&bson), canonical_bson_str);
   /* cB->cB */
   ASSERT_CMPUINT8 (bson_get_data (&canonical_bson), canonical_bson_str);

   extjson = bson_new_from_json (extjson_str, extjson_str_len, &error);
   canonical_extjson = bson_new_from_json (canonical_extjson_str, canonical_extjson_str_len, &error);

   /* B->cE */
   ASSERT_CMPJSON (bson_as_json (&bson, NULL), canonical_extjson_str);

   /* E->cE */
   ASSERT_CMPJSON (bson_as_json (extjson, NULL), canonical_extjson_str);
   
   /* cb->cE */
   ASSERT_CMPUINT8 (bson_get_data (&canonical_bson), bson_get_data (canonical_extjson));

   /* cE->cE */
   ASSERT_CMPJSON (bson_as_json (canonical_extjson, NULL), canonical_extjson_str);

   if (!lossy) {
   /* E->cB */
      ASSERT_CMPJSON (bson_as_json (extjson, NULL), canonical_extjson_str);

      /* cE->cB */
      ASSERT_CMPUINT8 (bson_get_data (canonical_extjson), canonical_bson_str);
   }
   bson_destroy (extjson);
   bson_destroy (canonical_extjson);
}

static void
_test_bson_type_print_description (bson_iter_t *iter)
{
   if (bson_iter_find (iter, "description") && BSON_ITER_HOLDS_UTF8 (iter)) {
      if (test_suite_debug_output ()) {
         fprintf (stderr, "  - %s\n", bson_iter_utf8 (iter, NULL));
         fflush (stderr);
      }
   }
}

#define IS_NAN(dec) (dec).high == 0x7c00000000000000ull

static void
test_bson_type (bson_t *scenario, test_bson_type_valid_cb valid)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;
   BSON_ASSERT (scenario);

   if (bson_iter_init_find (&iter, scenario, "valid")) {
      const char *expected = NULL;
      bson_t json;
      bson_t bson_input = BSON_INITIALIZER;

      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test;
         uint8_t       *bson_str;
         uint32_t       bson_str_len;
         const uint8_t *extjson_str;
         uint32_t       extjson_str_len;
         const uint8_t *canonical_extjson_str;
         uint32_t       canonical_extjson_str_len;
         bool           lossy = false;
         bool           have_extjson = false;
         bool           have_canonical_extjson = false;

         bson_iter_recurse (&inner_iter, &test);
         _test_bson_type_print_description (&test);
         while (bson_iter_next (&test)) {
            const char *key = bson_iter_key (&test);

            if (!strcmp (key, "bson") && BSON_ITER_HOLDS_UTF8 (&test)) {
               const char *input = NULL;
               unsigned int byte;
               uint32_t tmp;
               int x = 0;
               int i = 0;

               input = bson_iter_utf8 (&test, &tmp);
               bson_str_len = tmp / 2;
               bson_str = bson_malloc (bson_str_len);
               while (SSCANF (&input[i], "%2x", &byte) == 1) {
                  bson_str[x++] = (uint8_t) byte;
                  i += 2;
               }
            }

            if (!strcmp (key, "extjson") && BSON_ITER_HOLDS_UTF8 (&test)) {
               extjson_str = (const uint8_t *)bson_iter_utf8 (&test, &extjson_str_len);
               have_extjson = true;
            }

            if (!strcmp (key, "canonical_extjson") && BSON_ITER_HOLDS_UTF8 (&test)) {
               canonical_extjson_str = (const uint8_t *)bson_iter_utf8 (&test, &canonical_extjson_str_len);
               have_canonical_extjson = true;
            }
            if (!strcmp (key, "canonical_extjson") && BSON_ITER_HOLDS_BOOL (&test)) {
               lossy = bson_iter_bool (&test);
            }
         }

         valid (bson_str,
                bson_str_len,
                bson_str,
                bson_str_len,
                have_extjson ? extjson_str : NULL,
                have_extjson ?  extjson_str_len : 0,
                have_canonical_extjson ? canonical_extjson_str : extjson_str,
                have_canonical_extjson ? canonical_extjson_str_len : extjson_str_len,
                lossy);
         bson_free (bson_str);
      }
   }

   if (bson_iter_init_find (&iter, scenario, "parseErrors")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test;

         bson_iter_recurse (&inner_iter, &test);
         _test_bson_type_print_description (&test);

         if (bson_iter_find (&test, "string") && BSON_ITER_HOLDS_UTF8 (&test)) {
            bson_decimal128_t d;
            uint32_t tmp;
            const char *input = bson_iter_utf8 (&test, &tmp);

            ASSERT (!bson_decimal128_from_string (input, &d));
            ASSERT (IS_NAN (d));
         }
      }
   }
}

static void
test_bson_type_decimal128_cb (bson_t *scenario)
{
   test_bson_type (scenario, test_bson_type_decimal128);
}


void
test_add_spec_test (TestSuite *suite, const char *filename, test_hook callback)
{
   bson_t *test;
   char *skip_json;

   test = get_bson_from_json_file ((char *)filename);
   skip_json = strstr (filename, "json")+4;
   skip_json = bson_strndup (skip_json, strlen (skip_json)-5);

   TestSuite_AddWC (suite, skip_json, (void (*)(void *))callback, (void (*)(void*))bson_destroy, test);
   bson_free (skip_json);
}

void
test_bson_type_install (TestSuite *suite)
{
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-1.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-2.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-3.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-4.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-5.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-6.json", test_bson_type_decimal128_cb);
   test_add_spec_test (suite, JSON_DIR "/type/decimal128/decimal128-7.json", test_bson_type_decimal128_cb);
}

