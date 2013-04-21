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


#include <stdio.h>
#include <stdarg.h>

#include "bson-error.h"
#include "bson-memory.h"
#include "bson-types.h"


void
bson_set_error (bson_error_t  *error,
                bson_uint32_t  domain,
                bson_uint32_t  code,
                const char    *format,
                ...)
{
   va_list args;
   char *buf;
   int len = 32;
   int n;

   if (error) {
      error->domain = domain;
      error->code = code;
      error->message = NULL;

      if (format) {
         buf = bson_malloc0(len);
         while (TRUE) {
            va_start(args, format);
            n = vsnprintf(buf, len, format, args);
            va_end(args);

            if (n > -1 && n < len) {
               error->message = buf;
               return;
            }

            if (n > -1) {
               len = n + 1;
            } else {
               len *= 2;
            }

            buf = bson_realloc(buf, len);
         }
      }
   }
}


void
bson_error_destroy (bson_error_t *error)
{
   if (error) {
      bson_free(error->message);
   }
}
