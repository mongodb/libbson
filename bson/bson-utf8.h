/*
 * Copyright 2013 MongoDB Inc.
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
#error "Only <bson.h> can be included directly."
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
bson_bool_t
bson_utf8_validate (const char  *utf8,
                    size_t       utf8_len,
                    bson_bool_t  allow_null);


/**
 * bson_utf8_escape_for_json:
 * @utf8: A UTF-8 encoded string.
 * @utf8_len: The length of @utf8 in bytes or -1 if NUL terminated.
 *
 * Allocates a new string matching @utf8 except that special characters
 * in JSON will be escaped. The resulting string is also UTF-8 encoded.
 *
 * Both " and \ characters will be escaped. Additionally, if a NUL byte
 * is found before @utf8_len bytes, it will be converted to the two byte
 * UTF-8 sequence.
 *
 * Returns: A newly allocated string that should be freed with bson_free().
 */
char *
bson_utf8_escape_for_json (const char *utf8,
                           ssize_t     utf8_len);


/**
 * bson_utf8_get_char:
 * @utf8: A string containing validated UTF-8.
 *
 * Fetches the next UTF-8 character from the UTF-8 sequence.
 *
 * Returns: A 32-bit bson_unichar_t reprsenting the multi-byte sequence.
 */
bson_unichar_t
bson_utf8_get_char (const char *utf8);


/**
 * bson_utf8_next_char:
 * @utf8: A string containing validated UTF-8.
 *
 * Returns an incremented pointer to the beginning of the next multi-byte
 * sequence in @utf8.
 *
 * Returns: An incremented pointer in @utf8.
 */
const char *
bson_utf8_next_char (const char *utf8);


/**
 * bson_utf8_from_unichar:
 * @unichar: A bson_unichar_t.
 * @utf8: A location for the multi-byte sequence.
 * @len: A location for number of bytes stored in @utf8.
 *
 * Converts the unichar to a sequence of utf8 bytes and stores those
 * in @utf8. The number of bytes in the sequence are stored in @len.
 */
void
bson_utf8_from_unichar (bson_unichar_t  unichar,
                        char            utf8[6],
                        bson_uint32_t  *len);


BSON_END_DECLS


#endif /* BSON_UTF8_H */
