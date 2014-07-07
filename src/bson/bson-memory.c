/*
 * Copyright 2013 MongoDB, Inc.
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


#include <stdlib.h>
#include <string.h>

#include "bson-config.h"
#include "bson-memory.h"

/* standard function pointers. */
bson_custom_malloc_func_t bson_malloc_func_p = malloc;
bson_custom_calloc_func_t bson_calloc_func_p = calloc;
#ifdef __APPLE__
   bson_custom_realloc_func_t bson_realloc_func_p = reallocf;
#else
   bson_custom_realloc_func_t bson_realloc_func_p = realloc;
#endif
bson_custom_free_func_t bson_free_func_p = free;

void bson_set_mem_functions(bson_custom_malloc_func_t custom_bson_malloc_func, /* IN */
                            bson_custom_calloc_func_t custom_bson_calloc_func, /* IN */
                            bson_custom_realloc_func_t custom_bson_realloc_func, /* IN */
                            bson_custom_free_func_t custom_bson_free_func) /* IN */
{
   bson_malloc_func_p = custom_bson_malloc_func ? custom_bson_malloc_func : malloc;
   bson_calloc_func_p = custom_bson_calloc_func ? custom_bson_calloc_func : calloc;
   if (custom_bson_realloc_func)
      bson_realloc_func_p = custom_bson_realloc_func;
   else
#ifdef __APPLE__
      bson_realloc_func_p = reallocf;
#else
      bson_realloc_func_p = realloc;
#endif
   bson_free_func_p = custom_bson_free_func ? custom_bson_free_func : free;
}

/*
 *--------------------------------------------------------------------------
 *
 * bson_malloc --
 *
 *       Allocates @num_bytes of memory and returns a pointer to it.  If
 *       malloc failed to allocate the memory, abort() is called.
 *
 *       Libbson does not try to handle OOM conditions as it is beyond the
 *       scope of this library to handle so appropriately.
 *
 * Parameters:
 *       @num_bytes: The number of bytes to allocate.
 *
 * Returns:
 *       A pointer if successful; otherwise abort() is called and this
 *       function will never return.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_malloc (size_t num_bytes) /* IN */
{
   void *mem;

   if (!(mem = bson_malloc_func_p (num_bytes))) {
      abort ();
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_malloc0 --
 *
 *       Like bson_malloc() except the memory is zeroed first. This is
 *       similar to calloc() except that abort() is called in case of
 *       failure to allocate memory.
 *
 * Parameters:
 *       @num_bytes: The number of bytes to allocate.
 *
 * Returns:
 *       A pointer if successful; otherwise abort() is called and this
 *       function will never return.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_malloc0 (size_t num_bytes) /* IN */
{
   void *mem = NULL;

   if (BSON_LIKELY (num_bytes)) {
      if (BSON_UNLIKELY (!(mem = bson_calloc_func_p (1, num_bytes)))) {
         abort ();
      }
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_realloc --
 *
 *       This function behaves similar to realloc() except that if there is
 *       a failure abort() is called.
 *
 * Parameters:
 *       @mem: The memory to realloc, or NULL.
 *       @num_bytes: The size of the new allocation or 0 to free.
 *
 * Returns:
 *       The new allocation if successful; otherwise abort() is called and
 *       this function never returns.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void *
bson_realloc (void   *mem,        /* IN */
              size_t  num_bytes)  /* IN */
{
   /*
    * Not all platforms are guaranteed to free() the memory if a call to
    * realloc() with a size of zero occurs. Windows, Linux, and FreeBSD do,
    * however, OS X does not.
    */
   if (BSON_UNLIKELY (num_bytes == 0)) {
      bson_free (mem);
      return NULL;
   }

   mem = bson_realloc_func_p (mem, num_bytes);

   if (BSON_UNLIKELY (!mem)) {
      abort ();
   }

   return mem;
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_realloc_ctx --
 *
 *       This wraps bson_realloc and provides a compatible api for similar
 *       functions with a context
 *
 * Parameters:
 *       @mem: The memory to realloc, or NULL.
 *       @num_bytes: The size of the new allocation or 0 to free.
 *       @ctx: Ignored
 *
 * Returns:
 *       The new allocation if successful; otherwise abort() is called and
 *       this function never returns.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */


void *
bson_realloc_ctx (void   *mem,        /* IN */
                  size_t  num_bytes,  /* IN */
                  void   *ctx)        /* IN */
{
   return bson_realloc(mem, num_bytes);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_free --
 *
 *       Frees @mem using the underlying allocator.
 *
 *       Currently, this only calls free() directly, but that is subject to
 *       change.
 *
 * Parameters:
 *       @mem: An allocation to free.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_free (void *mem) /* IN */
{
   bson_free_func_p (mem);
}


/*
 *--------------------------------------------------------------------------
 *
 * bson_zero_free --
 *
 *       Frees @mem using the underlying allocator. @size bytes of @mem will
 *       be zeroed before freeing the memory. This is useful in scenarios
 *       where @mem contains passwords or other sensitive information.
 *
 * Parameters:
 *       @mem: An allocation to free.
 *       @size: The number of bytes in @mem.
 *
 * Returns:
 *       None.
 *
 * Side effects:
 *       None.
 *
 *--------------------------------------------------------------------------
 */

void
bson_zero_free (void  *mem,  /* IN */
                size_t size) /* IN */
{
   if (BSON_LIKELY (mem)) {
      memset (mem, 0, size);
      bson_free (mem);
   }
}