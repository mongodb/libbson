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


#ifndef BSON_THREAD_H
#define BSON_THREAD_H


#include "bson-compat.h"
#include "bson-config.h"
#include "bson-macros.h"


BSON_BEGIN_DECLS


/*
 * The following tries to abstract the native threading implementation for
 * the current system using macros.
 */


#if defined(BSON_OS_WIN32)
#  define bson_mutex_t                 HANDLE
#  define bson_mutex_init(m, x)         CreateMutex (NULL, FALSE, NULL)
#  define bson_mutex_lock(m)           WaitforSingleObject ((m))
#  define bson_mutex_unlock(m)         ReleaseMutex ((m))
#  define bson_mutex_destroy(m)        CloseHandle ((m))
#  define bson_thread_t                HANDLE
#  define bson_thread_create(t, a, f, d)  (*(t) = \
                                              CreateThread (NULL, 0, (void *)f, \
                                                            d, 0, NULL))
#  define bson_thread_join(t, v)        WaitForSingleObject (t, 0)
#elif _MSC_VER
#else
#  include <pthread.h>
#  define BSON_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#  define bson_cond_t            pthread_cond_t
#  define bson_cond_init         pthread_cond_init
#  define bson_cond_wait         pthread_cond_wait
#  define bson_cond_signal       pthread_cond_signal
#  define bson_cond_destroy      pthread_cond_destroy
#  define bson_mutex_t           pthread_mutex_t
#  define bson_mutex_init        pthread_mutex_init
#  define bson_mutex_lock        pthread_mutex_lock
#  define bson_mutex_unlock      pthread_mutex_unlock
#  define bson_mutex_destroy     pthread_mutex_destroy
#  define bson_thread_t          pthread_t
#  define bson_thread_create     pthread_create
#  define bson_thread_join       pthread_join
#endif

#if defined(BSON_OS_WIN32)
#  if _WIN32_WINNT >= _WIN32_WINNT_VISTA
#    define bson_once_t                   INIT_ONCE
#    define BSON_ONCE_INIT                INIT_ONCE_STATIC_INIT
#    define bson_once(o, c)               InitOnceExecuteOnce(o, c, NULL, NULL)
#    define BSON_ONCE_FUN(n)              BOOL CALLBACK n(PINIT_ONCE _ignored_a, PVOID _ignored_b, PVOID *_ignored_c)
#    define BSON_ONCE_RETURN              return TRUE
#  else
     typedef struct bson_once
     {
        volatile long init;
        volatile long complete;
     } bson_once_t;

#    define BSON_ONCE_INIT       { 0, 0 }

     static BSON_INLINE int
     bson_once(bson_once_t * once, void (*cb)(void))
     {
        if (!once->complete) {
           if (InterlockedIncrement(&once->init) == 1) {
              cb();
              once->complete = 1;
           } else {
              InterlockedDecrement(&once->init);
              while (!once->complete) {
                 Sleep(0);
              }
           }
        }

        return 0;
     }

#    define BSON_ONCE_FUN(n)     void n(void)
#    define BSON_ONCE_RETURN     return
#  endif
#else
#  define bson_once_t            pthread_once_t
#  define bson_once              pthread_once
#  define BSON_ONCE_FUN(n)       void n(void)
#  define BSON_ONCE_RETURN       return

#  ifdef _PTHREAD_ONCE_INIT_NEEDS_BRACES
#    define BSON_ONCE_INIT       { PTHREAD_ONCE_INIT }
#  else
#    define BSON_ONCE_INIT       PTHREAD_ONCE_INIT
#  endif

#endif


BSON_END_DECLS


#endif /* BSON_THREAD_H */
