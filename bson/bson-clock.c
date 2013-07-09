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


#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include <time.h>

#include "bson-clock.h"


bson_int64_t
bson_get_monotonic_time (void)
{
#ifdef CLOCK_MONOTONIC
   {
      struct timespec ts;

      clock_gettime(CLOCK_MONOTONIC, &ts);
      return (ts.tv_sec * 1000000UL) + (ts.tv_nsec / 1000UL);
   }
#elif defined(__APPLE__)
   {
      clock_serv_t cclock;
      mach_timespec_t mts;

      host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
      clock_get_time(cclock, &mts);
      mach_port_deallocate(mach_task_self(), cclock);

      return (mts.tv_sec * 1000000UL) + (ts.tv_nsec / 1000UL);
   }
#else
   {
      struct timeval tv;

      gettimeofday(&tv, NULL);
      return (tv.tv_sec * 1000000UL) + tv.tv_usec;
   }
#endif
}
