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


#include <Python.h>
#include <datetime.h>

#include <bson/bson.h>
#include <bson/bson-version.h>

#include "time64.h"


static bson_context_t gContext;


static void
cbson_loads_visit_utf8 (const bson_iter_t *iter,
                        const char        *key,
                        size_t             v_utf8_len,
                        const char        *v_utf8,
                        void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   if (*ret) {
      if (!(value = PyString_FromStringAndSize(v_utf8, v_utf8_len))) {
         /* TODO: */
      }
      if (PyDict_SetItemString(*ret, key, value) == -1) {
         /* TODO: */
      }
      Py_DECREF(value);
   }
}


static void
cbson_loads_visit_int32 (const bson_iter_t *iter,
                         const char        *key,
                         bson_int32_t       v_int32,
                         void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   if (*ret) {
      if (!(value = PyInt_FromLong(v_int32))) {
         /* TODO: */
      }
      if (PyDict_SetItemString(*ret, key, value) == -1) {
         /* TODO: */
      }
      Py_DECREF(value);
   }
}


static void
cbson_loads_visit_int64 (const bson_iter_t *iter,
                         const char        *key,
                         bson_int64_t       v_int64,
                         void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   if (*ret) {
      if (!(value = PyLong_FromLongLong(v_int64))) {
         /* TODO: */
      }
      if (PyDict_SetItemString(*ret, key, value) == -1) {
         /* TODO: */
      }
      Py_DECREF(value);
   }
}


static void
cbson_loads_visit_bool (const bson_iter_t *iter,
                        const char        *key,
                        bson_bool_t        v_bool,
                        void              *data)
{
   PyObject **ret = data;

   if (*ret) {
      if (PyDict_SetItemString(*ret, key, v_bool ? Py_True : Py_False) == -1) {
         /* TODO: */
      }
   }
}


static void
cbson_loads_visit_double (const bson_iter_t *iter,
                          const char        *key,
                          double             v_double,
                          void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   if (*ret) {
      if (!(value = PyFloat_FromDouble(v_double))) {
         /* TODO: */
      }
      if (PyDict_SetItemString(*ret, key, value) == -1) {
         /* TODO: */
      }
      Py_DECREF(value);
   }
}


static void
cbson_loads_visit_oid (const bson_iter_t *iter,
                       const char        *key,
                       const bson_oid_t  *oid,
                       void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   if (*ret) {
      /*
       * TODO: Add ObjectId python type.
       */
      if (!(value = PyString_FromStringAndSize((const char *)oid, 12))) {
         /* TODO: */
      }
      if (PyDict_SetItemString(*ret, key, value) == -1) {
         /* TODO: */
      }
      Py_DECREF(value);
   }
}


static void
cbson_loads_visit_undefined (const bson_iter_t *iter,
                             const char        *key,
                             void              *data)
{
   PyObject **ret = data;

   if (*ret) {
      if (PyDict_SetItemString(*ret, key, Py_None) == -1) {
         /* TODO: */
      }
   }
}


static void
cbson_loads_visit_null (const bson_iter_t *iter,
                        const char        *key,
                        void              *data)
{
   PyObject **ret = data;

   if (*ret) {
      if (PyDict_SetItemString(*ret, key, Py_None) == -1) {
         /* TODO: */
      }
   }
}


static void
cbson_loads_visit_date_time (const bson_iter_t *iter,
                             const char        *key,
                             bson_uint64_t      seconds,
                             bson_uint32_t      milliseconds,
                             void              *data)
{
   PyObject **ret = data;

   if (*ret) {
      struct TM tm;
      PyObject *dateTime;
      Time64_T t64 = seconds;

      gmtime64_r(&t64, &tm);
      dateTime = PyDateTime_FromDateAndTime(tm.tm_year + 1900,
                                            tm.tm_mon + 1,
                                            tm.tm_mday,
                                            tm.tm_hour,
                                            tm.tm_min,
                                            tm.tm_sec,
                                            milliseconds * 1000);

      if (PyDict_SetItemString(*ret, key, dateTime) == -1) {
         /* TODO: */
      }

      Py_DECREF(dateTime);
   }
}


static void
cbson_loads_visit_corrupt (const bson_iter_t *iter,
                           void              *data)
{
   PyObject **ret = data;

   if (*ret) {
      Py_DECREF(*ret);
      *ret = NULL;
   }
}


static const bson_visitor_t gLoadsVisitors = {
   .visit_corrupt = cbson_loads_visit_corrupt,
   .visit_double = cbson_loads_visit_double,
   .visit_utf8 = cbson_loads_visit_utf8,
   .visit_undefined = cbson_loads_visit_undefined,
   .visit_oid = cbson_loads_visit_oid,
   .visit_bool = cbson_loads_visit_bool,
   .visit_date_time = cbson_loads_visit_date_time,
   .visit_null = cbson_loads_visit_null,
   .visit_int32 = cbson_loads_visit_int32,
   .visit_int64 = cbson_loads_visit_int64,
};


static PyObject *
cbson_loads (PyObject *self,
             PyObject *args)
{
   const bson_uint8_t *buffer;
   bson_uint32_t buffer_length;
   bson_reader_t reader;
   bson_iter_t iter;
   const bson_t *b;
   PyObject *ret;

   if (!PyArg_ParseTuple(args, "s#", &buffer, &buffer_length)) {
      return NULL;
   }

   bson_reader_init_from_data(&reader, buffer, buffer_length);

   if (!(b = bson_reader_read(&reader)) || !bson_iter_init(&iter, b)) {
      PyErr_SetString(PyExc_ValueError, "Failed to parse buffer.");
      return NULL;
   }

   ret = PyDict_New();
   bson_iter_visit_all(&iter, &gLoadsVisitors, &ret);
   bson_reader_destroy(&reader);

   if (!ret) {
      PyErr_Format(PyExc_ValueError,
                   "Failed to decode buffer at offset %u.",
                   (unsigned)iter.err_offset);
      return NULL;
   }

   return ret;
}


static PyMethodDef cbson_methods[] = {
   { "loads", cbson_loads, METH_VARARGS, "Decode a BSON document." },
   { NULL }
};


PyMODINIT_FUNC
initcbson (void)
{
   bson_context_flags_t flags;
   PyObject *module;
   PyObject *version;

   if (!(module = Py_InitModule("cbson", cbson_methods))) {
      return;
   }

   flags = 0;
   flags |= BSON_CONTEXT_DISABLE_PID_CACHE;
#if defined(__linux__)
   flags |= BSON_CONTEXT_USE_TASK_ID;
#endif

   bson_context_init(&gContext, flags);

   version = PyString_FromString(BSON_VERSION_S);
   PyModule_AddObject(module, "__version__", version);

   PyDateTime_IMPORT;
   if (!PyDateTimeAPI) {
      Py_DECREF(module);
      return;
   }
}
