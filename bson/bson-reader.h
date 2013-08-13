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


#if !defined (BSON_INSIDE) && !defined (BSON_COMPILATION)
#error "Only <bson.h> can be included directly."
#endif


#ifndef BSON_READER_H
#define BSON_READER_H


#include "bson-oid.h"
#include "bson-types.h"


BSON_BEGIN_DECLS


/**
 * bson_reader_init_from_fd:
 * @reader: A bson_reader_t
 * @fd: A file-descriptor to read from.
 * @close_fd: If the file-descriptor should be closed when done.
 *
 * Initializes new bson_reader_t that will read BSON documents into bson_t
 * structures from an underlying file-descriptor.
 *
 * If you would like the reader to call close() on @fd in
 * bson_reader_destroy(), then specify TRUE for close_fd.
 */
void
bson_reader_init_from_fd (bson_reader_t *reader,
                          int            fd,
                          bson_bool_t    close_fd);


/**
 * bson_reader_t:
 * @reader: A bson_reader_t.
 * @data: A buffer to read BSON documents from.
 * @length: The length of @data.
 *
 * Initializes bson_reader_t that will read BSON documents from a memory
 * buffer.
 */
void
bson_reader_init_from_data (bson_reader_t      *reader,
                            const bson_uint8_t *data,
                            size_t              length);



/**
 * bson_reader_destroy:
 * @reader: An initialized bson_reader_t.
 *
 * Releases resources that were allocated during the use of a bson_reader_t.
 * This should be called after you have finished using the structure.
 */
void
bson_reader_destroy (bson_reader_t *reader);


/**
 * bson_reader_set_read_func:
 * @reader: A bson_reader_t.
 *
 * Tell @reader to use a customized read(). By default, @reader uses read() in
 * libc.
 *
 * Note that @reader must be initialized by bson_reader_init_from_fd(), or data
 * will be destroyed.
 */
void
bson_reader_set_read_func (bson_reader_t    *reader,
                           bson_read_func_t  func);

/**
 * bson_reader_read:
 * @reader: A bson_reader_t.
 * @reached_eof: A location for a bson_bool_t.
 *
 * Reads the next bson_t in the underlying memory or storage.  The resulting
 * bson_t should not be modified or freed. You may copy it and iterate over it.
 * Functions that take a const bson_t* are safe to use.
 *
 * This structure does not survive calls to bson_reader_read() or
 * bson_reader_destroy() as it uses memory allocated by the reader or
 * underlying storage/memory.
 *
 * If NULL is returned then @reached_eof will be set to TRUE if the end of the
 * file or buffer was reached. This indicates if there was an error parsing the
 * document stream.
 *
 * Returns: A const bson_t that should not be modified or freed.
 */
const bson_t *
bson_reader_read (bson_reader_t *reader,
                  bson_bool_t   *reached_eof);


/**
 * bson_reader_tell:
 * @reader: A bson_reader_t.
 *
 * Return the current position in the underlying file. This will always
 * be at the beginning of a bson document or end of file.
 */
off_t
bson_reader_tell (bson_reader_t *reader);


BSON_END_DECLS


#endif /* BSON_READER_H */
