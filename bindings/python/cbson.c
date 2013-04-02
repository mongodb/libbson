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

#include <bson/bson.h>

#include "cbson-dbref.h"
#include "cbson-oid.h"
#include "cbson-util.h"


static bson_context_t gContext;


static BSON_INLINE void
cbson_loads_set_item (PyObject   *obj,
                      const char *key,
                      PyObject   *value)
{
   PyObject *keyobj;

   if (PyDict_Check(obj)) {
      keyobj = PyUnicode_FromString(key);
      PyDict_SetItem(obj, keyobj, value);
      Py_DECREF(keyobj);
   } else {
      PyList_Append(obj, value);
   }
}


static PyObject *
cbson_regex_new (const char *regex,
                 const char *options)
{
   bson_uint32_t flags = 0;
   bson_uint32_t i;
   PyObject *module;
   PyObject *compile;
   PyObject *strobj;
   PyObject *value;

   /*
    * NOTE: Typically, you would want to cache the module from
    *       PyImport_ImportModule(). However, it is really not typical to
    *       have regexes stored so loading it each time is not a common
    *       case.
    */

   if (!(module = PyImport_ImportModule("re"))) {
      return NULL;
   }

   if (!(compile = PyObject_GetAttrString(module, "compile"))) {
      Py_DECREF(module);
      return NULL;
   }

   for (i = 0; options[i]; i++) {
      switch (options[i]) {
      case 'i':
         flags |= 2;
         break;
      case 'l':
         flags |= 4;
         break;
      case 'm':
         flags |= 8;
         break;
      case 's':
         flags |= 16;
         break;
      case 'u':
         flags |= 32;
         break;
      case 'x':
         flags |= 64;
         break;
      default:
         break;
      }
   }

   strobj = PyUnicode_FromString(regex);
   value = PyObject_CallFunction(compile, (char *)"Oi", strobj, flags);
   Py_DECREF(compile);
   Py_DECREF(strobj);
   Py_DECREF(module);

   return value;
}


static bson_bool_t
cbson_loads_visit_utf8 (const bson_iter_t *iter,
                        const char        *key,
                        size_t             v_utf8_len,
                        const char        *v_utf8,
                        void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   value = PyUnicode_FromStringAndSize(v_utf8, v_utf8_len);
   cbson_loads_set_item(*ret, key, value);
   Py_DECREF(value);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_int32 (const bson_iter_t *iter,
                         const char        *key,
                         bson_int32_t       v_int32,
                         void              *data)
{
   PyObject **ret = data;
   PyObject *value;

#if PY_MAJOR_VERSION >= 3
   value = PyLong_FromLong(v_int32);
#else
   value = PyInt_FromLong(v_int32);
#endif
   cbson_loads_set_item(*ret, key, value);
   Py_DECREF(value);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_int64 (const bson_iter_t *iter,
                         const char        *key,
                         bson_int64_t       v_int64,
                         void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   value = PyLong_FromLongLong(v_int64);
   cbson_loads_set_item(*ret, key, value);
   Py_DECREF(value);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_bool (const bson_iter_t *iter,
                        const char        *key,
                        bson_bool_t        v_bool,
                        void              *data)
{
   PyObject **ret = data;
   cbson_loads_set_item(*ret, key, v_bool ? Py_True : Py_False);
   return FALSE;
}


static bson_bool_t
cbson_loads_visit_double (const bson_iter_t *iter,
                          const char        *key,
                          double             v_double,
                          void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   value = PyFloat_FromDouble(v_double);
   cbson_loads_set_item(*ret, key, value);
   Py_DECREF(value);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_oid (const bson_iter_t *iter,
                       const char        *key,
                       const bson_oid_t  *oid,
                       void              *data)
{
   PyObject **ret = data;
   PyObject *value;

   value = cbson_oid_new(oid);
   cbson_loads_set_item(*ret, key, value);
   Py_DECREF(value);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_undefined (const bson_iter_t *iter,
                             const char        *key,
                             void              *data)
{
   PyObject **ret = data;
   cbson_loads_set_item(*ret, key, Py_None);
   return FALSE;
}


static bson_bool_t
cbson_loads_visit_null (const bson_iter_t *iter,
                        const char        *key,
                        void              *data)
{
   PyObject **ret = data;
   cbson_loads_set_item(*ret, key, Py_None);
   return FALSE;
}


static bson_bool_t
cbson_loads_visit_date_time (const bson_iter_t *iter,
                             const char        *key,
                             bson_int64_t       msec_since_epoch,
                             void              *data)
{
   PyObject **ret = data;
   PyObject *date_time;

   date_time = cbson_date_time_from_msec(msec_since_epoch);
   cbson_loads_set_item(*ret, key, date_time);
   Py_DECREF(date_time);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_regex (const bson_iter_t *iter,
                         const char        *key,
                         const char        *regex,
                         const char        *options,
                         void              *data)
{
   PyObject **ret = data;
   PyObject *re;

   re = cbson_regex_new(regex, options);
   cbson_loads_set_item(*ret, key, re);
   Py_DECREF(re);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_dbpointer (const bson_iter_t *iter,
                             const char        *key,
                             size_t             v_collection_len,
                             const char        *v_collection,
                             const bson_oid_t  *v_oid,
                             void              *data)
{
   PyObject **ret = data;
   PyObject *dbref;

   dbref = cbson_dbref_new(v_collection,
                           v_collection_len,
                           NULL,
                           0,
                           v_oid);
   cbson_loads_set_item(*ret, key, dbref);
   Py_DECREF(dbref);

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_document (const bson_iter_t *iter,
                            const char        *key,
                            const bson_t      *v_document,
                            void              *data);


static bson_bool_t
cbson_loads_visit_array (const bson_iter_t *iter,
                         const char        *key,
                         const bson_t      *v_array,
                         void              *data);


static void
cbson_loads_visit_corrupt (const bson_iter_t *iter,
                           void              *data)
{
   PyObject **ret = data;
   Py_DECREF(*ret);
   *ret = NULL;
}


static const bson_visitor_t gLoadsVisitors = {
   .visit_corrupt = cbson_loads_visit_corrupt,
   .visit_double = cbson_loads_visit_double,
   .visit_utf8 = cbson_loads_visit_utf8,
   .visit_document = cbson_loads_visit_document,
   .visit_array = cbson_loads_visit_array,
   .visit_undefined = cbson_loads_visit_undefined,
   .visit_oid = cbson_loads_visit_oid,
   .visit_bool = cbson_loads_visit_bool,
   .visit_date_time = cbson_loads_visit_date_time,
   .visit_null = cbson_loads_visit_null,
   .visit_regex = cbson_loads_visit_regex,
   .visit_dbpointer = cbson_loads_visit_dbpointer,
   .visit_int32 = cbson_loads_visit_int32,
   .visit_int64 = cbson_loads_visit_int64,
};


static bson_bool_t
cbson_loads_visit_document (const bson_iter_t *iter,
                            const char        *key,
                            const bson_t      *v_document,
                            void              *data)
{
   bson_iter_t child;
   PyObject **ret = data;
   PyObject *obj;

   bson_return_val_if_fail(iter, TRUE);
   bson_return_val_if_fail(key, TRUE);
   bson_return_val_if_fail(v_document, TRUE);

   if (bson_iter_init(&child, v_document)) {
      obj = PyDict_New();
      if (!bson_iter_visit_all(&child, &gLoadsVisitors, &obj)) {
         cbson_loads_set_item(*ret, key, obj);
      }
      Py_XDECREF(obj);
   }

   return FALSE;
}


static bson_bool_t
cbson_loads_visit_array (const bson_iter_t *iter,
                         const char        *key,
                         const bson_t      *v_array,
                         void              *data)
{
   bson_iter_t child;
   PyObject **ret = data;
   PyObject *obj;

   bson_return_val_if_fail(iter, TRUE);
   bson_return_val_if_fail(key, TRUE);
   bson_return_val_if_fail(v_array, TRUE);

   if (bson_iter_init(&child, v_array)) {
      obj = PyList_New(0);
      if (!bson_iter_visit_all(&child, &gLoadsVisitors, &obj)) {
         cbson_loads_set_item(*ret, key, obj);
      }
      Py_XDECREF(obj);
   }

   return FALSE;
}


static PyObject *
cbson_loads (PyObject *self,
             PyObject *args)
{
   const bson_uint8_t *buffer;
   bson_uint32_t buffer_length;
   bson_reader_t reader;
   bson_iter_t iter;
   const bson_t *b;
   bson_bool_t eof;
   PyObject *ret;

   if (!PyArg_ParseTuple(args, "s#", &buffer, &buffer_length)) {
      return NULL;
   }

   bson_reader_init_from_data(&reader, buffer, buffer_length);

   if (!(b = bson_reader_read(&reader, &eof)) || !bson_iter_init(&iter, b)) {
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

   /*
    * Initialize our module with class methods.
    */
   if (!(module = Py_InitModule("cbson", cbson_methods))) {
      return;
   }

   /*
    * Initialize our context for certain library functions.
    */
   flags = 0;
   flags |= BSON_CONTEXT_DISABLE_PID_CACHE;
#if defined(__linux__)
   flags |= BSON_CONTEXT_USE_TASK_ID;
#endif
   bson_context_init(&gContext, flags);

   /*
    * Register the library version as __version__.
    */
   version = PyString_FromString(BSON_VERSION_S);
   PyModule_AddObject(module, "__version__", version);

   /*
    * Initialize utilities (datetime).
    */
   if (!cbson_util_init(module)) {
      Py_DECREF(module);
      return;
   }

   /*
    * Register cbson types.
    */
   PyModule_AddObject(module, "ObjectId", (PyObject *)cbson_oid_get_type(&gContext));
   PyModule_AddObject(module, "InvalidId", cbson_invalid_id_get_type());
   PyModule_AddObject(module, "DBRef", (PyObject *)cbson_dbref_get_type());
}
