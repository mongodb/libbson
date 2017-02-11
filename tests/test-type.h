/*
 * Copyright 2017 MongoDB, Inc.
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

#ifndef TEST_TYPE_H
#define TEST_TYPE_H

#include "TestSuite.h"

#include <bson.h>

/*
See:
github.com/mongodb/specifications/blob/master/source/bson-corpus/bson-corpus.rst
#testing-validity
*/
typedef struct _test_bson_type_t {
   const char *scenario_description;
   bson_type_t bson_type;
   const char *test_description;
   uint8_t *B;          /* "bson" */
   uint32_t B_len;
   uint8_t *cB;         /* "canonical_bson" */
   uint32_t cB_len;
   const char *E;       /* "extjson" */
   uint32_t E_len;
   const char *cE;      /* "canonical_extjson" */
   uint32_t cE_len;
   bool lossy;
} test_bson_type_t;


typedef void (*test_bson_type_valid_cb) (test_bson_type_t *test);

void
test_bson_type_print_description (const char *description);
uint8_t *
test_bson_type_unhexlify (bson_iter_t *iter, uint32_t *bson_str_len);
void
test_bson_type (bson_t *scenario, test_bson_type_valid_cb valid);


#endif
