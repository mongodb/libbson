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


#include "bson-atomic.h"

#include "bson-tests.h"


static void
test1 (void)
{
   bson_int32_t v = 0;

   bson_atomic_int_add (&v, 1);
   assert (v == 1);
}


static void
test2 (void)
{
   bson_int64_t v = 0;

   bson_atomic_int64_add (&v, 1);
   assert (v == 1);
}


static void
test3 (void)
{
   bson_memory_barrier ();
}


int
main (int argc,
      char *argv[])
{
   run_test("/atomic/int/add", test1);
   run_test("/atomic/int64/add", test2);
   run_test("/atomic/memory_barrier", test3);

   return 0;
}
