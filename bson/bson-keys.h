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


#ifndef BSON_KEYS_H
#define BSON_KEYS_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


/**
 * bson_uint32_to_string:
 * @value: A bson_uint32_t to convert to string.
 * @strptr: (out): A pointer to the resulting string.
 * @str: (out): Storage for a string made with snprintf.
 * @size: Size of @str.
 *
 * Converts @value to a string.
 *
 * If @value is from 0 to 1000, it will use a constant string in the data
 * section of the library.
 *
 * If not, a string will be formatted using @str and snprintf(). This is much
 * slower, of course and therefore we try to optimize it out.
 *
 * @strptr will always be set. It will either point to @str or a constant
 * string. You will want to use this as your key.
 */
void
bson_uint32_to_string (bson_uint32_t value,
                       const char  **strptr,
                       char         *str,
                       size_t        size);


BSON_END_DECLS


#endif /* BSON_KEYS_H */
