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


#ifndef BSON_CONTEXT_H
#define BSON_CONTEXT_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


/**
 * bson_context_flags_t:
 *
 * This enumeration is used to configure a bson_context_t.
 *
 * %BSON_CONTEXT_NONE: Use default options.
 * %BSON_CONTEXT_THREAD_SAFE: Context will be called from multiple threads.
 * %BSON_CONTEXT_DISABLE_PID_CACHE: Call getpid() instead of caching the
 *   result of getpid() when initializing the context.
 * %BSON_CONTEXT_DISABLE_HOST_CACHE: Call gethostname() instead of caching the
 *   result of gethostname() when initializing the context.
 */
typedef enum
{
   BSON_CONTEXT_NONE               = 0,
   BSON_CONTEXT_THREAD_SAFE        = 1 << 0,
   BSON_CONTEXT_DISABLE_HOST_CACHE = 1 << 1,
   BSON_CONTEXT_DISABLE_PID_CACHE  = 1 << 2,
#if defined(__linux__)
   BSON_CONTEXT_USE_TASK_ID        = 1 << 3,
#endif
} bson_context_flags_t;


/**
 * bson_context_t:
 *
 * This structure manages context for the bson library. It handles
 * configuration for thread-safety and other performance related requirements.
 * Consumers will create a context and may use multiple under a variety of
 * situations.
 *
 * If your program calls fork(), you should initialize a new bson_context_t
 * using bson_context_init().
 *
 * If you are using threading, it is suggested that you use a bson_context_t
 * per thread for best performance. Alternatively, you can initialize the
 * bson_context_t with BSON_CONTEXT_THREAD_SAFE, although a performance penalty
 * will be incurred.
 *
 * Many functions will require that you provide a bson_context_t such as OID
 * generation.
 *
 * This structure is oqaque in that you cannot see the contents of the
 * structure. However, it is stack allocatable in that enough padding is
 * provided in _bson_context_t to hold the structure.
 */
typedef struct
{
   void *opaque[16];
} bson_context_t;


/**
 * bson_context_init:
 * @context: A bson_context_t.
 * @flags: Flags for initialization.
 *
 * Initializes @context with the flags specified.
 *
 * In most cases, you want to call this with @flags set to BSON_CONTEXT_NONE
 * and create once instance per-thread.
 *
 * If you absolutely must have a single context for your application and use
 * more than one thread, then BSON_CONTEXT_THREAD_SAFE should be bitwise or'd
 * with your flags. This requires synchronization between threads.
 *
 * If you expect your hostname to change often, you may consider specifying
 * BSON_CONTEXT_DISABLE_HOST_CACHE so that gethostname() is called for every
 * OID generated. This is much slower.
 *
 * If you expect your pid to change without notice, such as from an unexpected
 * call to fork(), then specify BSON_CONTEXT_DISABLE_PID_CACHE.
 */
void
bson_context_init (bson_context_t       *context,
                   bson_context_flags_t  flags);


/**
 * bson_context_destroy:
 * @context: A bson_context_t.
 *
 * Cleans up a bson_context_t and releases any associated resources.  This
 * should be called when you are done using @context.
 */
void
bson_context_destroy (bson_context_t *context);


BSON_END_DECLS


#endif /* BSON_CONTEXT_H */
