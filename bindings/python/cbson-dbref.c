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


#include "cbson-dbref.h"
#include "cbson-oid.h"


#define cbson_dbref_check(op) (Py_TYPE(op) == &cbson_dbref_type)


static PyObject *cbson_dbref_tp_repr        (PyObject *obj);
static int       cbson_dbref_tp_compare     (PyObject *obj1,
                                             PyObject *obj2);
static long      cbson_dbref_tp_hash        (PyObject *obj);
static PyObject *cbson_dbref_get_collection (PyObject *obj,
                                             void     *data);
static PyObject *cbson_dbref_get_database   (PyObject *obj,
                                             void     *data);
static PyObject *cbson_dbref_get_id         (PyObject *obj,
                                             void     *data);


static PyTypeObject cbson_dbref_type = {
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cbson.DBRef",             /*tp_name*/
    sizeof(cbson_dbref_t),     /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    cbson_dbref_tp_compare,    /*tp_compare*/
    cbson_dbref_tp_repr,       /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    cbson_dbref_tp_hash,       /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "A BSON DBRef.",           /*tp_doc*/
};


static PyGetSetDef cbson_dbref_getset[] = {
   { (char *)"collection", cbson_dbref_get_collection, NULL,
     (char *)"Get the name of this DBRef's collection as unicode." },
   { (char *)"database", cbson_dbref_get_database, NULL,
     (char *)"Get the name of this DBRef's database.\n\n"
             "Returns None if this DBRef doesn't specify a database." },
   { (char *)"id", cbson_dbref_get_id, NULL,
     (char *)"Get this DBRef's _id." },
   { NULL }
};


static PyMethodDef cbson_dbref_methods[] = {
   { NULL }
};


static PyObject *
cbson_dbref_get_collection (PyObject *object,
                            void     *data)
{
   cbson_dbref_t *dbref = (cbson_dbref_t *)object;
   PyObject *ret = Py_None;

   if (dbref->collection) {
      ret = PyUnicode_FromStringAndSize(dbref->collection,
                                        dbref->collection_len);
   } else {
      Py_INCREF(ret);
   }

   return ret;
}


static PyObject *
cbson_dbref_get_database (PyObject *object,
                          void     *data)
{
   cbson_dbref_t *dbref = (cbson_dbref_t *)object;
   PyObject *ret = Py_None;

   if (dbref->database) {
      ret = PyUnicode_FromStringAndSize(dbref->database,
                                        dbref->database_len);
   } else {
      Py_INCREF(ret);
   }

   return ret;
}


static PyObject *
cbson_dbref_get_id (PyObject *object,
                    void     *data)
{
   cbson_dbref_t *dbref = (cbson_dbref_t *)object;
   return cbson_oid_new(&dbref->oid);
}


static PyObject *
cbson_dbref_tp_repr (PyObject *obj)
{
   return NULL;
}


static int
cbson_dbref_tp_compare (PyObject *obj1,
                        PyObject *obj2)
{
   return 0;
}


static long
cbson_dbref_tp_hash (PyObject *obj)
{
   return 0;
}


static PyObject *
cbson_dbref_tp_new (PyTypeObject *self,
                    PyObject     *args,
                    PyObject     *kwargs)
{
   /*
    * TODO: Handle args and kwargs.
    */
   return PyType_GenericNew(&cbson_dbref_type, args, kwargs);
}


PyObject *
cbson_dbref_new (const char       *collection,
                 size_t            collection_len,
                 const char       *database,
                 size_t            database_len,
                 const bson_oid_t *oid)
{
   cbson_dbref_t *dbref;
   PyObject *ret;

   ret = cbson_dbref_tp_new(NULL, NULL, NULL);
   dbref = (cbson_dbref_t *)ret;

   dbref->collection = collection_len ? strndup(collection, collection_len) : NULL;
   dbref->collection_len = collection_len;

   dbref->database = database_len ? strndup(database, database_len) : NULL;
   dbref->database_len = database_len;

   if (oid) {
      bson_oid_copy(oid, &dbref->oid);
   }

   return ret;
}


PyTypeObject *
cbson_dbref_get_type (void)
{
   static bson_bool_t initialized;

   if (!initialized) {
      cbson_dbref_type.tp_new = cbson_dbref_tp_new;
      cbson_dbref_type.tp_getset = cbson_dbref_getset;
      cbson_dbref_type.tp_methods = cbson_dbref_methods;
      if (PyType_Ready(&cbson_dbref_type) < 0) {
         return NULL;
      }
      initialized = TRUE;
   }

   return &cbson_dbref_type;
}
