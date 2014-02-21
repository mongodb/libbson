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
 * This program will print each BSON document contained in the provided files
 * as a JSON string to STDOUT.
 */


#include <bson.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#ifdef BSON_OS_WIN32
#include <share.h>
int
bson_open (const char *filename,
           int         flags)
{
   int fd;
   errno_t err;

   err = _sopen_s (&fd, filename, flags | _O_BINARY, _SH_DENYNO,
                   _S_IREAD | _S_IWRITE);

   if (err) {
      errno = err;
      return -1;
   }

   return fd;
}

ssize_t
bson_read (int    fd,
           void  *buf,
           size_t count)
{
   return (ssize_t)_read (fd, buf, (int)count);
}

#define bson_close _close
#else
#define bson_open open
#define bson_read read
#define bson_close close
#endif

static ssize_t
_read_cb(void * handle, uint8_t * buf, size_t len)
{
   return bson_read(*(int *)handle, buf, len);
}

void
_destroy_cb(void * handle)
{
   bson_close(*(int *)handle);
}

int
main (int   argc,
      char *argv[])
{
   bson_json_reader_t *reader;
   bson_t *b_out;
   bson_error_t err;
   const char *filename;
   int fd;
   int i;
   int b;

   /*
    * Print program usage if no arguments are provided.
    */
   if (argc == 1) {
      fprintf(stderr, "usage: %s FILE...\n", argv[0]);
      return 1;
   }

   b_out = bson_new();

   /*
    * Process command line arguments expecting each to be a filename.
    */
   for (i = 1; i < argc; i++) {
      filename = argv[i];

      /*
       * Open the filename provided in command line arguments.
       */
      if (0 == strcmp(filename, "-")) {
         fd = 0;
      } else {
         errno = 0;
         fd = bson_open(filename, O_RDONLY);
         if (! (-1 != fd)) {
            fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
            continue;
         }
      }

      reader = bson_json_reader_new(&fd, &_read_cb, &_destroy_cb, true, 4096);

      /*
       * Convert each incoming document to JSON and print to stdout.
       */
      while ((b = bson_json_reader_read (reader, b_out, &err))) {
         if (b < 0) {
            fprintf(stderr, "Error in json parsing:\n%s\n", err.message);
            abort();
         }

         fwrite (bson_get_data(b_out), 1, b_out->len, stdout);
         bson_reinit (b_out);
      }

      bson_json_reader_destroy (reader);
      bson_destroy (b_out);
   }

   return 0;
}
