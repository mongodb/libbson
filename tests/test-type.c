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
#include "test-type.h"
#include "bson-private.h"

#include <limits.h>

#ifdef _MSC_VER
#define PATH_MAX 1024
#define realpath(path, expanded) \
   GetFullPathName (path, PATH_MAX, expanded, NULL)
#endif

#ifdef _MSC_VER
#define SSCANF sscanf_s
#else
#define SSCANF sscanf
#endif

#ifndef JSON_DIR
#define JSON_DIR "tests/json"
#endif

BEGIN_IGNORE_DEPRECATIONS

void
test_bson_type_decimal128 (test_bson_type_t *test)
{
   bson_t bson;
   bson_t canonical_bson;
   bson_t *extjson;
   bson_t *canonical_extjson;
   bson_error_t error;

   BSON_ASSERT (test->B);
   BSON_ASSERT (test->cB);
   BSON_ASSERT (test->E);
   BSON_ASSERT (test->cE);

   bson_init_static (&bson, test->B, test->B_len);
   bson_init_static (&canonical_bson, test->cB, test->cB_len);

   ASSERT_CMPUINT8 (bson_get_data (&bson), test->B);
   /* B->cB */
   ASSERT_CMPUINT8 (bson_get_data (&bson), test->cB);
   /* cB->cB */
   ASSERT_CMPUINT8 (bson_get_data (&canonical_bson), test->cB);

   extjson =
      bson_new_from_json ((const uint8_t *) test->E, test->E_len, &error);
   canonical_extjson =
      bson_new_from_json ((const uint8_t *) test->cE, test->cE_len, &error);

   /* B->cE */
   ASSERT_CMPJSON (bson_as_json (&bson, NULL), test->cE);

   /* E->cE */
   ASSERT_CMPJSON (bson_as_json (extjson, NULL), test->cE);

   /* cb->cE */
   ASSERT_CMPUINT8 (bson_get_data (&canonical_bson),
                    bson_get_data (canonical_extjson));

   /* cE->cE */
   ASSERT_CMPJSON (bson_as_json (canonical_extjson, NULL), test->cE);

   if (!test->lossy) {
      /* E->cB */
      ASSERT_CMPJSON (bson_as_json (extjson, NULL), test->cE);

      /* cE->cB */
      ASSERT_CMPUINT8 (bson_get_data (canonical_extjson), test->cB);
   }

   bson_destroy (extjson);
   bson_destroy (canonical_extjson);
}

void
test_bson_type_print_description (const char *description)
{
   if (test_suite_debug_output ()) {
      printf ("  - %s\n", description);
      fflush (stdout);
   }
}

#define IS_NAN(dec) (dec).high == 0x7c00000000000000ull

uint8_t *
test_bson_type_unhexlify (bson_iter_t *iter, uint32_t *bson_str_len)
{
   const char *hex_str;
   uint8_t *data = NULL;
   unsigned int byte;
   uint32_t tmp;
   int x = 0;
   int i = 0;

   ASSERT (BSON_ITER_HOLDS_UTF8 (iter));
   hex_str = bson_iter_utf8 (iter, &tmp);
   *bson_str_len = tmp / 2;
   data = bson_malloc (*bson_str_len);
   while (SSCANF (&hex_str[i], "%2x", &byte) == 1) {
      data[x++] = (uint8_t) byte;
      i += 2;
   }

   return data;
}


void
test_bson_type (bson_t *scenario, test_bson_type_valid_cb valid)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;
   const char *scenario_description = NULL;
   bson_type_t bson_type;

   BSON_ASSERT (scenario);

   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "description"));
   scenario_description = bson_iter_utf8 (&iter, NULL);

   BSON_ASSERT (bson_iter_init_find (&iter, scenario, "bson_type"));
   /* like "0x0C */
   if (sscanf (bson_iter_utf8 (&iter, NULL), "%i", (int *) &bson_type) != 1) {
      fprintf (
         stderr, "Couldn't parse bson_type %s\n", bson_iter_utf8 (&iter, NULL));
      abort ();
   }

   if (bson_iter_init_find (&iter, scenario, "valid")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test_iter;
         test_bson_type_t test = {scenario_description, bson_type, NULL};

         bson_iter_recurse (&inner_iter, &test_iter);
         while (bson_iter_next (&test_iter)) {
            const char *key = bson_iter_key (&test_iter);

            if (!strcmp (key, "description")) {
               test.test_description = bson_iter_utf8 (&test_iter, NULL);
               test_bson_type_print_description (test.test_description);
            }

            if (!strcmp (key, "bson")) {
               test.B = test_bson_type_unhexlify (&test_iter, &test.B_len);
            }

            if (!strcmp (key, "canonical_bson")) {
               test.cB = test_bson_type_unhexlify (&test_iter, &test.cB_len);
            }

            if (!strcmp (key, "extjson")) {
               test.E = bson_iter_utf8 (&test_iter, &test.E_len);
            }

            if (!strcmp (key, "canonical_extjson")) {
               test.cE = bson_iter_utf8 (&test_iter, &test.cE_len);
            }

            if (!strcmp (key, "lossy")) {
               test.lossy = bson_iter_bool (&test_iter);
            }
         }

         ASSERT (test.B);

#define SET_DEFAULT(a, b) \
   if (!test.a) {         \
      test.a = test.b;    \
   }

         SET_DEFAULT (cB, B);
         SET_DEFAULT (cB_len, B_len);
         SET_DEFAULT (cE, E);
         SET_DEFAULT (cE_len, E_len);

         /* execute the test callback */
         valid (&test);

         if (test.cB != test.B) {
            bson_free (test.cB);
         }

         bson_free (test.B);
      }
   }
}

static void
test_bson_type_decimal128_cb (bson_t *scenario)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;

   test_bson_type (scenario, test_bson_type_decimal128);

   if (bson_iter_init_find (&iter, scenario, "parseErrors")) {
      bson_iter_recurse (&iter, &inner_iter);
      while (bson_iter_next (&inner_iter)) {
         bson_iter_t test;
         const char *description = NULL;
         const char *input = NULL;
         bson_decimal128_t d;
         uint32_t tmp;

         bson_iter_recurse (&inner_iter, &test);
         while (bson_iter_next (&test)) {
            if (!strcmp (bson_iter_key (&test), "description")) {
               description = bson_iter_utf8 (&test, NULL);
               test_bson_type_print_description (description);
            }

            if (!strcmp (bson_iter_key (&test), "string")) {
               input = bson_iter_utf8 (&test, &tmp);
            }
         }

         ASSERT (input);
         ASSERT (!bson_decimal128_from_string (input, &d));
         ASSERT (IS_NAN (d));
      }
   }
}

void
test_bson_type_install (TestSuite *suite)
{
   install_json_test_suite (
      suite, JSON_DIR "/type/decimal128", test_bson_type_decimal128_cb);
}

END_IGNORE_DEPRECATIONS
