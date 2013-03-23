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


#ifndef BSON_TESTS_H
#define BSON_TESTS_H


#include <bson/bson.h>
#include <bson/bson-memory.h>
#include <stdio.h>


BSON_BEGIN_DECLS


#define assert_cmpstr(a, b)                                             \
   do {                                                                 \
      if (((a) != (b)) && !!cmpstr((a), (b))) {                         \
         fprintf(stderr, "FAIL\n\nAssert Failure: \"%s\" != \"%s\"\n",  \
                         a, b);                                         \
         abort();                                                       \
      }                                                                 \
   } while (0)


#define assert_cmpint(a, eq, b)                                         \
   do {                                                                 \
      if (!((a) eq (b))) {                                              \
         fprintf(stderr, "FAIL\n\nAssert Failure: "                     \
                         #a " " #eq " " #b "\n");                       \
         abort();                                                       \
      }                                                                 \
   } while (0)


#define run_test(name, func)             \
   do {                                  \
      fprintf(stdout, "%-42s : ", name); \
      fflush(stdout);                    \
      func();                            \
      fprintf(stdout, "PASS\n");         \
   } while (0)


BSON_END_DECLS

#endif /* BSON_TESTS_H */
