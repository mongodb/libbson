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
#include "corpus-test.h"
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

#define IS_NAN(dec) (dec).high == 0x7c00000000000000ull

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
      bson_new_from_json ((const uint8_t *) test->E, -1, &error);
   canonical_extjson =
      bson_new_from_json ((const uint8_t *) test->cE, -1, &error);

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

static void
test_bson_type_decimal128_cb (bson_t *scenario)
{
   bson_iter_t iter;
   bson_iter_t inner_iter;

   corpus_test (scenario, test_bson_type_decimal128);

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
               corpus_test_print_description (description);
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
