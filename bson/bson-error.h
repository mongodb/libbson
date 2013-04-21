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


#ifndef BSON_ERROR_H
#define BSON_ERROR_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


void
bson_error_destroy (bson_error_t *error);


void
bson_set_error (bson_error_t  *error,
                bson_uint32_t  domain,
                bson_uint32_t  code,
                const char    *format,
                ...)
   BSON_GNUC_PRINTF(4, 5);


BSON_END_DECLS


#endif /* BSON_ERROR_H */
