#include <assert.h>

#include "bson-tests.h"


static void
test_get_monotonic_time (void)
{
   bson_int64_t t;
   bson_int64_t t2;

   t = bson_get_monotonic_time();
   t2 = bson_get_monotonic_time();
   assert(t);
   assert(t2);
   assert_cmpint(t, <=, t2);
}


int
main (int   argc,
      char *argv[])
{
   run_test("/bson/clock/get_monotonic_time", test_get_monotonic_time);

   return 0;
}
