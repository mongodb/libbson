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

typedef void (*test_bson_type_valid_cb) (const uint8_t *bson_str,
                                         uint32_t bson_str_len,
                                         const uint8_t *canonical_bson_str,
                                         uint32_t canonical_bson_str_len,
                                         const uint8_t *extjson_str,
                                         uint32_t extjson_str_len,
                                         const uint8_t *canonical_extjson_str,
                                         uint32_t canonical_extjson_str_len,
                                         bool lossy);

void
test_bson_type (bson_t *scenario, test_bson_type_valid_cb valid);


#endif
