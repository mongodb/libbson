# bson_t

## Creating a BSON document

The bson_t structure encapsulates a BSON document.
It manages the memory allocations required for the document as well as trying to avoid them when possible.

## Memory Management

The bson_t structure tries to avoid heap allocations by reserving 120 bytes of space within the bson_t structure.
This space is used until the document grows past 120 bytes at which point heap allocations will be made.
To avoid using stack allocations you can call `bson_sized_new()`.

## Creating Sub-Documents

Often when working with BSON or MongoDB, the use of sub-documents is required.
Libbson helps you to avoid unncessary allocations by providing access to build a document within another BSON document.

To do this, you can use the pair of functions `bson_append_document_begin()` and `bson_append_document_end()`.
See also `bson_append_array_begin()` and `bson_append_array_end()` for sub-array equivalent.

```c
bson_t doc;
bson_t child;
char *str;

bson_init(&doc);
bson_append_document_begin(&doc, "sub", -1, &child);
bson_append_utf8(&child, "hello", -1, "world", -1);
bson_append_document_end(&doc, &child);

str = bson_as_json(&doc, NULL);
printf("%s\n", str);
bson_free(str);

bson_destroy(&doc);
```

Which would result in the output:

```js
{ "sub" : { "hello" : "world" } }
```

## Overwriting Fields

Fast mutation of BSON documents can be performed if the field type is a fixed size.
For example, `int32`, `int64`, `double`, and `boolean` fields are prime candates.

To perform this, we must iterate to the target field and use the `bson_iter_t` to change the value.

```c
void
overwrite_int32_key (bson_t *doc, const char *key, int value)
{
  bson_iter_t iter;
  
  if (bson_iter_init_find(&iter, doc, key) && BSON_ITER_HOLDS_INT32(&iter)) {
    bson_iter_overwrite_int32(&iter, value);
  }
}
```

## Validating BSON Documents

Libbson comes with routines to help you validate a BSON document such as those received from unsafe peers like over the network.
The efficient way to do this is to create a static `bson_t` structure to represent the network data.
Creating this document will only succeed if the encoded BSON length matches the length provided by the caller.
Aferwards, we can simply call the `bson_validate()` routine to check the validity.

```c
bson_t doc;
size_t err_off;

if (!bson_init_static(&doc, network_data, network_data_length)) {
	fprintf(stderr, "Invalid length of bson document.\n");
}

if (!bson_validate(&doc, (BSON_VALIDATE_UTF8 | BSON_VALIDATE_UTF8_ALLOW_NULL), &err_off)) {
	fprintf(stderr, "Invalid bson document at offset %u\n", (unsigned)err_off);
}
```
