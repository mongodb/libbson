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


#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <limits.h>
#include <stdarg.h>
#include <string.h>

#include "bson-compat.h"
#include "bson-string.h"
#include "bson-memory.h"
#include "bson-utf8.h"


/*
 *--------------------------------------------------------------------------
 *
 * bson_string_new --
 *
 *       Create a new bson_string_t.
 *
 *       bson_string_t is a power-of-2 allocation growing string. Every
 *       time data is appended the next power of two size is chosen for
 *       the allocation. Pretty standard stuff.
 *
 *       It is UTF-8 aware through the use of bson_string_append_unichar().
 *       The proper UTF-8 character sequence will be used.
 *
 * Parameters:
 *       @str: a string to copy or NULL.
 *
 * Returns:
 *       A newly allocated bson_string_t that should be freed with
 *       bson_string_free().
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

bson_string_t *
bson_string_new (const char *str) /* IN */
{
   bson_string_t *ret;

   ret = bson_malloc0 (sizeof *ret);
   ret->len = str ? (int)strlen (str) : 0;
   ret->alloc = ret->len + 1;

   if (!bson_is_power_of_two (ret->alloc)) {
      ret->alloc = bson_next_power_of_two (ret->alloc);
   }

   BSON_ASSERT (ret->alloc >= 1);

   ret->str = bson_malloc (ret->alloc);

   if (str) {
      memcpy (ret->str, str, ret->len);
   }
   ret->str [ret->len] = '\0';

   ret->str [ret->len] = '\0';

   return ret;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_string_free --
 *
 *       Free the bson_string_t @string and related allocations.
 *
 *       If @free_segment is false, then the strings buffer will be
 *       returned and is not freed. Otherwise, NULL is returned.
 *
 * Returns:
 *       The string->str if free_segment is false.
 *       Otherwise NULL.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

char *
bson_string_free (bson_string_t *string,       /* IN */
                  bool           free_segment) /* IN */
{
   char *ret = NULL;

   bson_return_val_if_fail (string, NULL);

   if (!free_segment) {
      ret = string->str;
   } else {
      bson_free (string->str);
   }

   bson_free (string);

   return ret;
}


void
bson_string_append (bson_string_t *string,
                    const char    *str)
{
   uint32_t len;

   bson_return_if_fail (string);
   bson_return_if_fail (str);

   len = (uint32_t)strlen (str);

   if ((string->alloc - string->len - 1) < len) {
      string->alloc += len;
      if (!bson_is_power_of_two (string->alloc)) {
         string->alloc = bson_next_power_of_two (string->alloc);
      }
      string->str = bson_realloc (string->str, string->alloc);
   }

   memcpy (string->str + string->len, str, len);
   string->len += len;
   string->str [string->len] = '\0';
}


void
bson_string_append_c (bson_string_t *string,
                      char           c)
{
   char cc[2];

   BSON_ASSERT (string);

   if (BSON_UNLIKELY (string->alloc == (string->len + 1))) {
      cc [0] = c;
      cc [1] = '\0';
      bson_string_append (string, cc);
      return;
   }

   string->str [string->len++] = c;
   string->str [string->len] = '\0';
}


void
bson_string_append_unichar (bson_string_t *string,
                            bson_unichar_t unichar)
{
   uint32_t len;
   char str [8];

   BSON_ASSERT (string);
   BSON_ASSERT (unichar);

   bson_utf8_from_unichar (unichar, str, &len);

   if (len <= 6) {
      str [len] = '\0';
      bson_string_append (string, str);
   }
}


void
bson_string_append_printf (bson_string_t *string,
                           const char    *format,
                           ...)
{
   va_list args;
   char *ret;

   BSON_ASSERT (string);
   BSON_ASSERT (format);

   va_start (args, format);
   ret = bson_strdupv_printf (format, args);
   va_end (args);
   bson_string_append (string, ret);
   bson_free (ret);
}


void
bson_string_truncate (bson_string_t *string,
                      uint32_t  len)
{
   uint32_t alloc;

   bson_return_if_fail (string);
   bson_return_if_fail (len < INT_MAX);

   alloc = len + 1;

   if (alloc < 16) {
      alloc = 16;
   }

   if (!bson_is_power_of_two (alloc)) {
      alloc = bson_next_power_of_two (alloc);
   }

   string->str = bson_realloc (string->str, alloc);
   string->alloc = alloc;
   string->len = len;

   string->str [string->len] = '\0';
}


char *
bson_strdup (const char *str)
{
   long len;
   char * out;

   if (!str) {
      return NULL;
   }

   len = (long)strlen(str);

   out = malloc(len + 1);

   if (! out) {
       return NULL;
   }

   memcpy(out, str, len + 1);

   return out;
}


char *
bson_strdupv_printf (const char *format,
                     va_list     args)
{
   va_list my_args;
   char *buf;
   int len = 32;
   int n;

   bson_return_val_if_fail (format, NULL);

   buf = bson_malloc0 (len);

   while (true) {
      va_copy (my_args, args);
      n = bson_vsnprintf (buf, len, format, my_args);
      va_end (my_args);

      if (n > -1 && n < len) {
         return buf;
      }

      if (n > -1) {
         len = n + 1;
      } else {
         len *= 2;
      }

      buf = bson_realloc (buf, len);
   }
}


char *
bson_strdup_printf (const char *format,
                    ...)
{
   va_list args;
   char *ret;

   va_start (args, format);
   ret = bson_strdupv_printf (format, args);
   va_end (args);

   return ret;
}


char *
bson_strndup (const char *str,
              size_t      n_bytes)
{
   char *ret;

   ret = bson_malloc0 (n_bytes + 1);
   memcpy (ret, str, n_bytes);
   ret[n_bytes] = '\0';

   return ret;
}

void
bson_strfreev (char **str)
{
   int i;

   if (str) {
      for (i = 0; str [i]; i++)
         bson_free (str [i]);
      bson_free (str);
   }
}


size_t
bson_strnlen (const char *s,
              size_t      maxlen)
{
#ifdef HAVE_STRNLEN
   return strnlen (s, maxlen);
#else
   size_t i;

   for (i = 0; i < maxlen; i++) {
      if (s [i] == '\0') {
         return i + 1;
      }
   }

   return maxlen;
#endif
}


void
bson_strncpy (char       *dst,  /* IN */
              const char *src,  /* IN */
              size_t      size) /* IN */
{
#ifdef _MSC_VER
   strcpy_s (dst, size, src);
#else
   strncpy (dst, src, size);
   dst[size - 1] = '\0';
#endif
}


int
bson_vsnprintf (char       *str,
                size_t      size,
                const char *format,
                va_list     ap)
{
#ifdef BSON_OS_WIN32
   int r = -1;

   BSON_ASSERT (str);

   if (size != 0) {
       r = _vsnprintf_s (str, size, _TRUNCATE, format, ap);
   }

   if (r == -1) {
      r = _vscprintf (format, ap);
   }

   return r;
#else
   return vsnprintf (str, size, format, ap);
#endif
}


int
bson_snprintf (char       *str,
               size_t      size,
               const char *format,
               ...)
{
   int r;
   va_list ap;

   BSON_ASSERT (str);

   va_start (ap, format);
   r = bson_vsnprintf (str, size, format, ap);
   va_end (ap);

   return r;
}
