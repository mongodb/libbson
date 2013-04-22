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


#include <stdarg.h>
#include <string.h>

#include "bson-string.h"
#include "bson-memory.h"
#include "bson-utf8.h"


bson_string_t *
bson_string_new (const char *str)
{
   bson_string_t *ret;

   ret = bson_malloc0(sizeof *ret);
   ret->len = str ? strlen(str) + 1: 1;
   ret->str = bson_malloc0(ret->len);

   if (str) {
      memcpy(ret->str, str, ret->len);
   }

   return ret;
}


char *
bson_string_free (bson_string_t *string,
                  bson_bool_t    free_segment)
{
   char *ret = NULL;

   bson_return_val_if_fail(string, NULL);

   if (!free_segment) {
      ret = string->str;
   } else {
      bson_free(string->str);
   }

   bson_free(string);

   return ret;
}


void
bson_string_append (bson_string_t *string,
                    const char    *str)
{
   bson_uint32_t len;

   bson_return_if_fail(string);
   bson_return_if_fail(str);

   len = strlen(str);
   string->str = bson_realloc(string->str, string->len + len);
   memcpy(&string->str[string->len - 1], str, len);
   string->len += len;
   string->str[string->len-1] = '\0';
}


void
bson_string_append_c (bson_string_t *string,
                      char           c)
{
   bson_return_if_fail(string);

   string->str = bson_realloc(string->str, string->len + 1);
   string->str[string->len-1] = c;
   string->len++;
   string->str[string->len-1] = '\0';
}


void
bson_string_append_unichar (bson_string_t  *string,
                            bson_unichar_t  unichar)
{
   bson_uint32_t len;
   char str[7];

   bson_return_if_fail(string);
   bson_return_if_fail(unichar);

   bson_utf8_from_unichar(unichar, str, &len);

   if (len <= 6) {
      str[len] = '\0';
      bson_string_append(string, str);
   }
}


char *
bson_strdup (const char *str)
{
   if (!str)
      return NULL;
   return strdup(str);
}


char *
bson_strdup_printf (const char *format,
                    ...)
{
   va_list args;
   char *buf;
   int len = 32;
   int n;

   bson_return_val_if_fail(format, NULL);

   buf = bson_malloc0(len);

   while (TRUE) {
      va_start(args, format);
      n = vsnprintf(buf, len, format, args);
      va_end(args);

      if (n > -1 && n < len) {
         return buf;
      }

      if (n > -1) {
         len = n + 1;
      } else {
         len *= 2;
      }

      buf = bson_realloc(buf, len);
   }
}


char *
bson_strndup (const char *str,
              size_t      n_bytes)
{
   char *ret;

   ret = bson_malloc0(n_bytes + 1);
   memcpy(ret, str, n_bytes);
   ret[n_bytes] = '\0';

   return ret;
}
