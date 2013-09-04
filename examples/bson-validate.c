/*
 * Copyright 2013 MongoDB Inc.
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


/*
 * This program will validate each BSON document contained in the provided
 * files.
 */


#include <bson.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>


int
main (int   argc,
      char *argv[])
{
   bson_reader_t *reader;
   const bson_t *b;
   const char *filename;
   size_t offset;
   int docnum;
   int fd;
   int i;

   /*
    * Print program usage if no arguments are provided.
    */
   if (argc == 1) {
      fprintf(stderr, "usage: %s FILE...\n", argv[0]);
      return 1;
   }

   /*
    * Process command line arguments expecting each to be a filename.
    */
   for (i = 1; i < argc; i++) {
      filename = argv[i];
      docnum = 0;

      /*
       * Open the filename provided in command line arguments.
       */
      errno = 0;
      fd = open(filename, O_RDONLY);
      if (fd == -1) {
         fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
         continue;
      }

      /*
       * Initialize a new reader for this file descriptor.
       */
      reader = bson_reader_new_from_fd(fd, TRUE);

      /*
       * Convert each incoming document to JSON and print to stdout.
       */
      while ((b = bson_reader_read(reader, NULL))) {
         docnum++;
         if (!bson_validate(b,
                            (BSON_VALIDATE_UTF8 |
                             BSON_VALIDATE_DOLLAR_KEYS |
                             BSON_VALIDATE_DOT_KEYS |
                             BSON_VALIDATE_UTF8_ALLOW_NULL),
                            &offset)) {
            fprintf(stderr,
                    "Document %u in \"%s\" is invalid at offset %u.\n",
                    docnum, filename, (int)offset);
            return 1;
         }
      }

      /*
       * Cleanup after our reader, which closes the file descriptor.
       */
      bson_reader_destroy(reader);
   }

   return 0;
}
