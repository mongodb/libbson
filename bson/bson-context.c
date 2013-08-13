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


#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#if defined(__linux__)
#include <sys/syscall.h>
#endif

#if defined WITH_OID32_PT || defined WITH_OID64_PT
#include "bson-thread.h"
#endif

#include "bson-context.h"
#include "bson-context-private.h"
#include "bson-md5.h"
#include "bson-memory.h"

#if defined(HAVE_WINDOWS)
#include <winsock2.h>
#endif


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif


#if defined(__linux__)
static bson_uint16_t
gettid (void)
{
   return syscall(SYS_gettid);
}
#endif


static void
bson_context_get_oid_host (bson_context_t *context,
                           bson_oid_t     *oid)
{
   bson_uint8_t *bytes = (bson_uint8_t *)oid;
   bson_uint8_t digest[16];
   bson_md5_t md5;
   char hostname[HOST_NAME_MAX];

   gethostname(hostname, sizeof hostname);
   hostname[HOST_NAME_MAX-1] = '\0';

   bson_md5_init(&md5);
   bson_md5_append(&md5, (const bson_uint8_t *)hostname, strlen(hostname));
   bson_md5_finish(&md5, &digest[0]);

   bytes[4] = digest[0];
   bytes[5] = digest[1];
   bytes[6] = digest[2];
}


static void
bson_context_get_oid_host_cached (bson_context_t *context,
                                  bson_oid_t     *oid)
{
   oid->bytes[4] = context->md5[0];
   oid->bytes[5] = context->md5[1];
   oid->bytes[6] = context->md5[2];
}


static void
bson_context_get_oid_pid (bson_context_t *context,
                          bson_oid_t     *oid)
{
   bson_uint16_t pid = getpid();
   bson_uint8_t *bytes = (bson_uint8_t *)&pid;

   pid = BSON_UINT16_TO_BE(pid);
   oid->bytes[7] = bytes[0];
   oid->bytes[8] = bytes[1];
}


static void
bson_context_get_oid_pid_cached (bson_context_t *context,
                                 bson_oid_t     *oid)
{
   oid->bytes[7] = context->pidbe[0];
   oid->bytes[8] = context->pidbe[1];
}


static void
bson_context_get_oid_seq32 (bson_context_t *context,
                            bson_oid_t     *oid)
{
   bson_uint32_t seq = context->seq32++;

   seq = BSON_UINT32_TO_BE(seq);
   memcpy(&oid->bytes[9], ((bson_uint8_t *)&seq) + 1, 3);
}


static void
bson_context_get_oid_seq32_threadsafe (bson_context_t *context,
                                       bson_oid_t     *oid)
{
#if defined WITH_OID32_PT
   bson_uint32_t seq;
   bson_mutex_lock(&context->_m32);
   seq = context->seq32++;
   bson_mutex_unlock(&context->_m32);
#else
   bson_uint32_t seq = __sync_fetch_and_add_4(&context->seq32, 1);
#endif

   seq = BSON_UINT32_TO_BE(seq);
   memcpy(&oid->bytes[9], ((bson_uint8_t *)&seq) + 1, 3);
}


static void
bson_context_get_oid_seq64 (bson_context_t *context,
                            bson_oid_t     *oid)
{
   bson_uint64_t seq = context->seq64++;

   seq = BSON_UINT64_TO_BE(seq);
   memcpy(&oid->bytes[4], &seq, 8);
}


static void
bson_context_get_oid_seq64_threadsafe (bson_context_t *context,
                                       bson_oid_t     *oid)
{
#if defined WITH_OID64_PT
   bson_uint64_t seq;
   bson_mutex_lock(&context->_m64);
   seq = context->seq64++;
   bson_mutex_unlock(&context->_m64);
#else
   bson_uint64_t seq = __sync_fetch_and_add_8(&context->seq64, 1);
#endif

   seq = BSON_UINT64_TO_BE(seq);
   memcpy(&oid->bytes[4], &seq, 8);
}


bson_context_t *
bson_context_new (bson_context_flags_t  flags)
{
   bson_context_t *context;
   struct timeval tv;
   bson_uint16_t pid;
   unsigned int seed[3];
   unsigned int real_seed;
   bson_oid_t oid;

   context = bson_malloc0(sizeof *context);

   context->flags = flags;
   context->oid_get_host = bson_context_get_oid_host_cached;
   context->oid_get_pid = bson_context_get_oid_pid_cached;
   context->oid_get_seq32 = bson_context_get_oid_seq32;
   context->oid_get_seq64 = bson_context_get_oid_seq64;

   /*
    * Generate a seed for our the random starting position of our increment
    * bytes. We mask off the last nibble so that the last digit of the OID will
    * start at zero. Just to be nice.
    *
    * The seed itself is made up of the current time in seconds, milliseconds,
    * and pid xored together. I welcome better solutions if at all necessary.
    */
   gettimeofday(&tv, NULL);
   seed[0] = tv.tv_sec;
   seed[1] = tv.tv_usec;
   seed[2] = getpid();
   real_seed = seed[0] ^ seed[1] ^ seed[2];
   context->seq32 = rand_r(&real_seed) & 0x007FFFF0;

   if ((flags & BSON_CONTEXT_DISABLE_HOST_CACHE)) {
      context->oid_get_host = bson_context_get_oid_host;
   } else {
      bson_context_get_oid_host(context, &oid);
      context->md5[0] = oid.bytes[4];
      context->md5[1] = oid.bytes[5];
      context->md5[2] = oid.bytes[6];
   }

   if ((flags & BSON_CONTEXT_THREAD_SAFE)) {
#if defined WITH_OID32_PT
      bson_mutex_init(&context->_m32);
#endif
#if defined WITH_OID32_PT
      bson_mutex_init(&context->_m64);
#endif
      context->oid_get_seq32 = bson_context_get_oid_seq32_threadsafe;
      context->oid_get_seq64 = bson_context_get_oid_seq64_threadsafe;
   }

   if ((flags & BSON_CONTEXT_DISABLE_PID_CACHE)) {
      context->oid_get_pid = bson_context_get_oid_pid;
   } else {
      pid = getpid();
      pid = BSON_UINT16_TO_BE(pid);
#if defined(__linux__)
      if ((flags & BSON_CONTEXT_USE_TASK_ID)) {
         bson_int32_t tid;
         if ((tid = gettid())) {
            pid = BSON_UINT16_TO_BE(tid);
         }
      }
#endif
      memcpy(&context->pidbe[0], &pid, 2);
   }

   return context;
}


void
bson_context_destroy (bson_context_t *context)
{
#if defined WITH_OID32_PT
   bson_mutex_destroy(&context->_m32);
#endif
#if defined WITH_OID32_PT
   bson_mutex_destroy(&context->_m64);
#endif
   memset(context, 0, sizeof *context);
   bson_free(context);
}
