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


#include <stdio.h>
#include <stdarg.h>

#include "bson-error.h"
#include "bson-memory.h"
#include "bson-types.h"


/**
 * bson_set_error:
 * @error: (out): A #bson_error_t.
 * @domain: (in): The error domain.
 * @code: (in): The error code.
 * @format: (in): A printf style format string.
 *
 * Initializes @error using the parameters specified.
 *
 * @domain is an application specific error domain which should describe
 * which module initiated the error. Think of this as the exception type.
 *
 * @code is the @domain specific error code.
 *
 * @format is used to generate the format string. It uses vsnprintf()
 * internally so the format should match what you would use there.
 */
void
bson_set_error (bson_error_t *error,
                bson_uint32_t domain,
                bson_uint32_t code,
                const char   *format,
                ...)
{
   va_list args;

   if (error) {
      error->domain = domain;
      error->code = code;

      va_start (args, format);
      vsnprintf (error->message, sizeof error->message, format, args);
      va_end (args);

      error->message[sizeof error->message - 1] = '\0';
   }
}
