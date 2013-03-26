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


#include "cbson-oid.h"
#include "cbson-util.h"


static bson_context_t *gContext;


#define cbson_oid_check(op) (Py_TYPE(op) == &cbson_oid_type)


static PyObject *cbson_oid_tp_repr             (PyObject *obj);
static PyObject *cbson_oid_tp_str              (PyObject *obj);
static int       cbson_oid_tp_compare          (PyObject *obj1,
                                                PyObject *obj2);
static long      cbson_oid_tp_hash             (PyObject *obj);
static PyObject *cbson_oid_get_binary          (PyObject *obj,
                                                void     *data);
static PyObject *cbson_oid_get_generation_time (PyObject *obj,
                                                void     *data);


static PyTypeObject cbson_oid_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cbson.ObjectId",          /*tp_name*/
    sizeof(cbson_oid_t),       /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    cbson_oid_tp_compare,      /*tp_compare*/
    cbson_oid_tp_repr,         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    cbson_oid_tp_hash,         /*tp_hash */
    0,                         /*tp_call*/
    cbson_oid_tp_str,          /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "A BSON ObjectId.",        /*tp_doc*/
};


static PyGetSetDef cbson_oid_getset[] = {
   { (char *)"binary",
     cbson_oid_get_binary,
     NULL,
     (char *)"12-byte binary representation of this ObjectId.",
     NULL },
   { (char *)"generation_time",
     cbson_oid_get_generation_time,
     NULL,
     (char *)
      "A :class:`datetime.datetime` instance representing the time of\n"
      "generation for this :class:`ObjectId`.\n"
      "\n"
      "The :class:`datetime.datetime` is timezone aware, and\n"
      "represents the generation time in UTC. It is precise to the\n"
      "second." },
   { NULL }
};


static PyObject *
cbson_oid_tp_repr (PyObject *obj)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;
   char repr[37];
   char str[25];

   bson_oid_to_string(&oid->oid, str);
   snprintf(repr, sizeof repr, "ObjectId(\"%s\")", str);
   repr[36] = '\0';

   return PyString_FromStringAndSize(repr, 36);
}


static PyObject *
cbson_oid_tp_str (PyObject *obj)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;
   char str[25];

   bson_oid_to_string(&oid->oid, str);
   return PyString_FromStringAndSize(str, 24);
}


static int
cbson_oid_tp_compare (PyObject *obj1,
                      PyObject *obj2)
{
   cbson_oid_t *oid1 = (cbson_oid_t *)obj1;
   cbson_oid_t *oid2 = (cbson_oid_t *)obj2;
   int ret;

   /*
    * Bonkers, but CPython requires that the return value from compare is -1,
    * 0, or 1 (rather than < 0, 0, > 0) like qsort().
    */

   ret = bson_oid_compare(&oid1->oid, &oid2->oid);
   if (ret < 0) {
      return -1;
   } else if (ret > 0) {
      return 1;
   } else {
      return 0;
   }
}


static long
cbson_oid_tp_hash (PyObject *obj)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;
   return bson_oid_hash(&oid->oid);
}


static PyObject *
cbson_oid_get_binary (PyObject *obj,
                      void     *data)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;

#if PY_MAJOR_VERSION >= 3
   return PyBytes_FromStringAndSize((const char *)&oid->oid, sizeof oid->oid);
#else
   return PyString_FromStringAndSize((const char *)&oid->oid, sizeof oid->oid);
#endif
}


static PyObject *
cbson_oid_get_generation_time (PyObject *obj,
                               void     *data)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;
   bson_int64_t gentime;

   gentime = bson_oid_get_time_t(&oid->oid);
   return cbson_date_time_from_msec(gentime * 1000L);
}


static PyObject *
cbson_oid_tp_new (PyTypeObject *self,
                  PyObject     *args,
                  PyObject     *kwargs)
{
   cbson_oid_t *ret;

   ret = (cbson_oid_t *)PyType_GenericNew(&cbson_oid_type, args, kwargs);
   bson_oid_init(&ret->oid, gContext);
   return (PyObject *)ret;
}


PyObject *
cbson_oid_new (const bson_oid_t *oid)
{
   cbson_oid_t *ret;

   bson_return_val_if_fail(oid, NULL);

   ret = (cbson_oid_t *)PyType_GenericNew(&cbson_oid_type, NULL, NULL);
   bson_oid_copy(oid, &ret->oid);
   return (PyObject *)ret;
}


PyTypeObject *
cbson_oid_get_type (bson_context_t *context)
{
   static bson_bool_t initialized;

   if (!initialized) {
      gContext = context;
      cbson_oid_type.tp_new = cbson_oid_tp_new;
      cbson_oid_type.tp_getset = cbson_oid_getset;
      if (PyType_Ready(&cbson_oid_type) < 0) {
         return NULL;
      }
      initialized = TRUE;
   }

   return &cbson_oid_type;
}
