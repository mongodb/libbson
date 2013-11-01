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
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "bson-memory.h"


/**
 * bson_malloc:
 * @num_bytes: The number of bytes to allocate.
 *
 * Allocates @num_bytes of memory and returns a pointer to it.
 * If malloc failed to allocate the memory, abort() is called.
 *
 * Libbson does not try to handle OOM conditions as it is beyond
 * the scope of this library to handle so appropriately.
 *
 * Returns: A pointer if successful; otherwise abort() is called
 *   and this function will never return.
 */
void *
bson_malloc (size_t num_bytes)
{
   void *mem;

   if (!(mem = malloc (num_bytes))) {
      abort ();
   }

   return mem;
}


/**
 * bson_malloc0:
 * @num_bytes: The number of bytes to allocate.
 *
 * Like bson_malloc() except the memory is zeroed first. This is similar
 * to calloc() except that abort() is called in case of failure to allocate
 * memory.
 *
 * Returns: A pointer if successful; otherwise abort() is called
 *   and this function will never return.
 */
void *
bson_malloc0 (size_t num_bytes)
{
   void *mem;

   if (!(mem = calloc (1, num_bytes))) {
      abort ();
   }

   return mem;
}


/**
 * bson_memalign0:
 * @alignment: The alignment (such as 8 for 64-bit).
 * @size: The size of the allocation.
 *
 * Like posix_memalign() except abort() is called in the case of
 * failure to allocate memory.
 *
 * Returns: A pointer if successful; otherwise abort() is called
 *   and this function will never return.
 */
void *
bson_memalign0 (size_t alignment,
                size_t size)
{
   void *mem;

#if HAVE_POSIX_MEMALIGN

   if (0 != posix_memalign (&mem, alignment, size)) {
      perror ("posix_memalign() failure:");
      abort ();
   }

#elif HAVE_MEMALIGN
   mem = memalign (alignment, size);

   if (!mem) {
      perror ("memalign() failure:");
      abort ();
   }

#else
   mem = bson_malloc (size);
#endif

   memset (mem, 0, size);

   return mem;
}


/**
 * bson_realloc:
 * @mem: The memory to realloc, or NULL.
 * @num_bytes: The size of the new allocation or 0 to free.
 *
 * This function behaves similar to realloc() except that if there
 * is a failure abort() is called.
 *
 * Returns: The new allocation if successful; otherwise abort() is
 *   called and this function never returns.
 */
void *
bson_realloc (void  *mem,
              size_t num_bytes)
{
   if (!(mem = realloc (mem, num_bytes))) {
      if (!num_bytes) {
         return mem;
      }

      abort ();
   }

   return mem;
}


/**
 * bson_free:
 * @mem: An allocation to free.
 *
 * Frees @mem using the underlying allocator.
 *
 * Currently, this only calls free() directly, but that is subject to change.
 */
void
bson_free (void *mem)
{
   free (mem);
}


/**
 * bson_zero_free:
 * @mem: An allocation to free.
 * @size: The number of bytes in @mem.
 *
 * Frees @mem using the underlying allocator. @size bytes of @mem will be
 * zeroed before freeing the memory. This is useful in scenarios where
 * @mem contains passwords or other sensitive information.
 */
void
bson_zero_free (void  *mem,
                size_t size)
{
   if (mem) {
      memset (mem, 0, size);
      bson_free (mem);
   }
}
