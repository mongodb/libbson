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


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#ifndef BSON_CONTEXT_H
#define BSON_CONTEXT_H


#include "bson-macros.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


/**
 * bson_context_new:
 * @flags: Flags for initialization.
 *
 * Initializes a new context with the flags specified.
 *
 * In most cases, you want to call this with @flags set to BSON_CONTEXT_NONE.
 *
 * If you are running on Linux, BSON_CONTEXT_USE_TASK_ID can result in a
 * healthy speedup for multi-threaded scenarios.
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
 *
 * Returns: A newly allocated bson_context_t that should be freed with
 *          bson_context_destroy().
 */
bson_context_t *
bson_context_new (bson_context_flags_t flags);


/**
 * bson_context_destroy:
 * @context: A bson_context_t.
 *
 * Cleans up a bson_context_t and releases any associated resources.  This
 * should be called when you are done using @context.
 */
void
bson_context_destroy (bson_context_t *context);


/**
 * bson_get_default:
 *
 * Fetches the default, thread-safe implementation of bson_context_t.
 * If you need faster generation, it is recommended you create your
 * own bson_context_t with bson_context_new().
 *
 * Returns: A shared instance to a bson_context_t. This should not
 *    be modified or freed.
 */
bson_context_t *
bson_context_get_default (void);


BSON_END_DECLS


#endif /* BSON_CONTEXT_H */
