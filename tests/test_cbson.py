#!/usr/bin/env python

#
# Copyright 2013 10gen Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

"""Performance tests comparing cbson and bson Python modules."""

import bson
import cbson
import sys
import threading
import timeit

def compare_modules(name, bson_func, cbson_func, number=1, thread_count=None):
    """
    Runs two performance tests. One using the bson module and one
    using the cbson module. The resulting times are output for
    comparison.
    """
    sys.stdout.write('%-42s : %6d passes : ' % (name, number))
    sys.stdout.flush()

    bson_wrapper = lambda v: v.append(timeit.timeit(bson_func, number=number))
    cbson_wrapper = lambda v: v.append(timeit.timeit(cbson_func, number=number))

    if thread_count is not None:
        results = []
        threads = [threading.Thread(target=bson_wrapper, args=[results]) for i in range(thread_count)]
        [t.start() for t in threads]
        [t.join() for t in threads]
        bson_time = sum(results)

        results = []
        threads = [threading.Thread(target=cbson_wrapper, args=[results]) for i in range(thread_count)]
        [t.start() for t in threads]
        [t.join() for t in threads]
        cbson_time = sum(results)
    else:
        results = []
        bson_wrapper(results)
        bson_time = sum(results)

        results = []
        cbson_wrapper(results)
        cbson_time = sum(results)

    speedup = bson_time / cbson_time
    print >> sys.stdout, 'bson %0.4lf : cbson %0.4lf : %0.3lf x faster' % (bson_time, cbson_time, speedup)

def bson_generate_object_id():
    bson.ObjectId()

def cbson_generate_object_id():
    cbson.ObjectId()

if __name__ == '__main__':
    compare_modules('Generate ObjectId', bson_generate_object_id, cbson_generate_object_id, number=10000)
    compare_modules('Generate ObjectId (threaded)', bson_generate_object_id, cbson_generate_object_id, number=10000, thread_count=4)
