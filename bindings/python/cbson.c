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
#include <bson/bson-version.h>


static bson_context_t gContext;


static PyObject *
cbson_oid_init (PyObject *self,
                PyObject *args)
{
   bson_oid_t oid;

   bson_oid_init(&oid, &gContext);
   return PyString_FromStringAndSize((const char *)&oid, sizeof oid);
}


static PyObject *
cbson_oid_to_string (PyObject *self,
                     PyObject *args)
{
   const char *str;
   char outstr[25];
   int str_length;

   if (!PyArg_ParseTuple(args, "s#", &str, &str_length)) {
      return NULL;
   }

   if (str_length != 12) {
      PyErr_SetString(PyExc_ValueError, "must be 12 byte string.");
      return NULL;
   }

   bson_oid_to_string((const bson_oid_t *)str, outstr);
   return PyString_FromStringAndSize(outstr, 24);
}


static PyMethodDef cbson_methods[] = {
   { "oid_init", cbson_oid_init, METH_VARARGS, "Generate an OID." },
   { "oid_to_string", cbson_oid_to_string, METH_VARARGS, "Convert OID bytes to a string." },
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
   flags |= BSON_CONTEXT_THREAD_SAFE;
   flags |= BSON_CONTEXT_DISABLE_PID_CACHE;
#if defined(__linux__)
   flags |= BSON_CONTEXT_USE_TASK_ID;
#endif

   bson_context_init(&gContext, flags);

   version = PyString_FromString(BSON_VERSION_S);
   PyModule_AddObject(module, "__version__", version);
}
