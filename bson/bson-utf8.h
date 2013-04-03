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


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#error "Only <bson/bson.h> can be included directly."
#endif


#ifndef BSON_UTF8_H
#define BSON_UTF8_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


/**
 * bson_utf8_validate:
 * @utf8: A UTF-8 encoded string.
 * @utf8_len: The length of @utf8 in bytes.
 * @allow_null: If \0 is allowed within @utf8, exclusing trailing \0.
 *
 * Validates that @utf8 is a valid UTF-8 string.
 *
 * If @allow_null is TRUE, then \0 is allowed within @utf8_len bytes of @utf8.
 * Generally, this is bad practice since the main point of UTF-8 strings is
 * that they can be used with strlen() and friends. However, some languages
 * such as Python can send UTF-8 encoded strings with NUL's in them.
 *
 * Returns: TRUE if @utf8 is valid UTF-8.
 */
bson_bool_t bson_utf8_validate (const char  *utf8,
                                size_t       utf8_len,
                                bson_bool_t  allow_null);


BSON_END_DECLS


#endif /* BSON_UTF8_H */
