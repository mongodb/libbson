:man_page: bson_as_json

bson_as_json()
==============

Synopsis
--------

.. code-block:: c

  char *
  bson_as_json (const bson_t *bson, size_t *length);

Parameters
----------

* ``bson``: A :symbol:`bson_t`.
* ``length``: An optional location for the length of the resulting string.

Description
-----------

The :symbol:`bson_as_json()` function shall encode ``bson`` as a UTF-8 string using libbson's legacy JSON format. This function is superseded by :symbol:`bson_as_extended_json()`, which uses the same `MongoDB Extended JSON format`_ as all other MongoDB drivers.

The caller is responsible for freeing the resulting UTF-8 encoded string by calling :symbol:`bson_free()` with the result.

If non-NULL, ``length`` will be set to the length of the result in bytes.

Returns
-------

If successful, a newly allocated UTF-8 encoded string and ``length`` is set.

Upon failure, NULL is returned.

Example
-------

.. code-block:: c

  char *str = bson_as_json (doc, NULL);
  printf ("%s\n", str);
  bson_free (str);

.. _MongoDB Extended JSON format: https://github.com/mongodb/specifications/blob/master/source/extended-json.rst
