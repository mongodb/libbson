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


static long
cbson_oid_tp_hash (PyObject *obj)
{
   cbson_oid_t *oid = (cbson_oid_t *)obj;
   return bson_oid_hash(&oid->oid);
}


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
    0,                         /*tp_compare*/
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


static PyObject *
cbson_oid_tp_new (PyTypeObject *self,
                  PyObject     *args,
                  PyObject     *kwargs)
{
   return NULL;
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
cbson_oid_get_type (void)
{
   static bson_bool_t initialized;

   if (!initialized) {
      cbson_oid_type.tp_new = cbson_oid_tp_new;
      if (PyType_Ready(&cbson_oid_type) < 0) {
         return NULL;
      }
      initialized = TRUE;
   }

   return &cbson_oid_type;
}
