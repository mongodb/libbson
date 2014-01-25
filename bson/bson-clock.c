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


#ifdef __APPLE__
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "bson.h"
#include <time.h>

/**
 * bson_get_monotonic_time:
 *
 * Returns the monotonic system time, if available. A best effort is made to
 * use the monotonic clock. However, some systems may not support such a
 * feature.
 *
 * Returns: A monotonic clock in microseconds.
 */
int64_t
bson_get_monotonic_time (void)
{
#if defined(CLOCK_MONOTONIC) && ! defined(BSON_OS_WIN32)
   {
      struct timespec ts;

      clock_gettime (CLOCK_MONOTONIC, &ts);
      return (ts.tv_sec * 1000000UL) + (ts.tv_nsec / 1000UL);
   }
#elif defined(__APPLE__)
   {
      clock_serv_t cclock;
      mach_timespec_t mts;

      host_get_clock_service (mach_host_self (), SYSTEM_CLOCK, &cclock);
      clock_get_time (cclock, &mts);
      mach_port_deallocate (mach_task_self (), cclock);

      return (mts.tv_sec * 1000000UL) + (mts.tv_nsec / 1000UL);
   }
#else
   {
      struct timeval tv;

      bson_gettimeofday (&tv, NULL);
      return (tv.tv_sec * 1000000UL) + tv.tv_usec;
   }
#endif
}


/* The const value is shamelessy stolen from
 * http://www.boost.org/doc/libs/1_55_0/boost/chrono/detail/inlined/win/chrono.hpp
 *
 * File times are the number of 100 nanosecond intervals elapsed since 12:00 am
 * Jan 1, 1601 UTC.  I haven't check the math particularly hard
 *
 * ...  good luck
 */
int
bson_gettimeofday(struct timeval *tv, struct timezone *tz)
{
#ifdef BSON_OS_WIN32
#ifdef _MSC_VER
#define DELTA_EPOCH_IN_MICROSEC 11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSEC 11644473600000000ULL
#endif

  FILETIME ft;
  uint64_t tmp = 0;
 
  if (NULL != tv)
  {
    GetSystemTimeAsFileTime(&ft);

    /* pull out of the filetime into a 64 bit uint */
    tmp |= ft.dwHighDateTime;
    tmp <<= 32;
    tmp |= ft.dwLowDateTime;

    /* convert from 100's of nanosecs to microsecs */
    tmp /= 10; 
 
    /* adjust to unix epoch */
    tmp -= DELTA_EPOCH_IN_MICROSEC;

    tv->tv_sec = (long)(tmp / 1000000UL);
    tv->tv_usec = (long)(tmp % 1000000UL);
  }
 
  assert (NULL == tz);
 
  return 0;
#else
   return gettimeofday(tv, tz);
#endif
}


