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


void *
bson_malloc (size_t num_bytes)
{
   void *mem;

   if (!(mem = malloc(num_bytes))) {
      abort();
   }

   return mem;
}


void *
bson_malloc0 (size_t num_bytes)
{
   void *mem;

   if (!(mem = calloc(1, num_bytes))) {
      abort();
   }

   return mem;
}


void *
bson_memalign0 (size_t alignment,
                size_t size)
{
   void *mem;

#if HAVE_POSIX_MEMALIGN
   if (0 != posix_memalign(&mem, alignment, size)) {
      perror("posix_memalign() failure:");
      abort();
   }
#elif HAVE_MEMALIGN
   mem = memalign(alignment, size);
   if (!mem) {
      perror("memalign() failure:");
      abort();
   }
#else
   mem = bson_malloc(size);
#endif

   memset(mem, 0, size);

   return mem;
}


void *
bson_realloc (void   *mem,
              size_t  num_bytes)
{
   if (!(mem = realloc(mem, num_bytes))) {
      if (!num_bytes) {
         return mem;
      }
      abort();
   }
   return mem;
}


void
bson_free (void *mem)
{
   free(mem);
}


void
bson_zero_free (void   *mem,
                size_t  size)
{
   if (mem) {
      memset(mem, 0, size);
      bson_free(mem);
   }
}
