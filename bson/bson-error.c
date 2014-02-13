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

#include "bson-compat.h"

#include <stdio.h>
#include <stdarg.h>

#include "bson-string.h"
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
                uint32_t domain,
                uint32_t code,
                const char   *format,
                ...)
{
   va_list args;

   if (error) {
      error->domain = domain;
      error->code = code;

      va_start (args, format);
      bson_vsnprintf (error->message, sizeof error->message, format, args);
      va_end (args);

      error->message[sizeof error->message - 1] = '\0';
   }
}

char *
bson_strerror_r (int         err_code,
                 char       *buf,
                 size_t buflen)
{
   const char *unknown_msg = "Unknown error";

   assert (buflen > strlen (unknown_msg) + 1);
#if defined(__GNUC__) && defined(_GNU_SOURCE)
   /* put "Unknown error" in buf for us if unknown*/
   return strerror_r (err_code, buf, buflen);
#else
# if defined BSON_OS_WIN32

   if (strerror_s (buf, buflen, err_code) != 0)
# else

   /* XSI strerror_r */
   if (strerror_r (err_code, buf, buflen) != 0)
# endif
   {
      memcpy (buf, unknown_msg, strlen (unknown_msg) + 1);
   }

#endif

   return buf;
}

