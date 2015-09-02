/*
 * Copyright 2015 MongoDB, Inc.
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

#include <bson.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define DEFAULT_PORT 5000
#define DEFAULT_HOST "localhost"


/*
 * bson-streaming-remote-read --
 *
 *       Makes a connection to the specified host and port over TCP. This
 *       abstracts away most of the socket details required to make a
 *       connection.
 *
 * Parameters:
 *       @hostname: The name of the host to connect to.
 *       @port: The port number of the server on the host.
 *
 * Returns:
 *       A non-NULL file descriptor ready for reading on success.
 *       NULL on error.
 */
FILE *
bson_streaming_remote_read (const char *hostname,
                            int         port)
{
   FILE              *fd;
   int                sock;
   struct hostent    *server_host;
   struct sockaddr_in my_addr;
   struct sockaddr_in server_addr;

   if (port == 0) {
      port = DEFAULT_PORT;
   }

   /*
    * Get the hostname, look up the host's address and find the appropriate
    * server information required to make a connection.
    */
   if (!hostname || *hostname == '\0') {
      server_host = gethostbyname (DEFAULT_HOST);
   } else {
      server_host = gethostbyname (hostname);
   }

   if (!server_host) {
      herror ("bson-streaming-remote-read: failed to gethostbyname");
      return NULL;
   }

   memset (&server_addr, 0, sizeof server_addr);
   memcpy ((char *)&server_addr.sin_addr,
           server_host->h_addr_list[0],
           server_host->h_length);
   server_addr.sin_family = AF_INET;
   server_addr.sin_port = htons (port);

   /*
    * Obtain and bind to a socket for communicating.
    */
   sock = socket (AF_INET, SOCK_STREAM, 0);
   if (sock < 0) {
      perror ("bson-streaming-remote-read: socket creation failed");
      return NULL;
   }
   memset (&my_addr, 0, sizeof my_addr);
   my_addr.sin_family = AF_INET;
   my_addr.sin_port = htons (port);
   my_addr.sin_addr.s_addr = htonl (INADDR_ANY);

   if (bind (*sock, (struct sockaddr *)&my_addr,
             sizeof (struct sockaddr_in) < -1)) {
      perror ("bson-streaming-remote-read: bind(2) failure");
      return NULL;
   }

   /*
    * Connect to the server.
    */
   if (connect (sock, (struct sockaddr *)&server_addr,
                sizeof (server_addr)) < 0) {
      perror ("bson-streaming-remote-read: connect failure");
      return NULL;
   }

   /* 
    * Open a stream on the file descriptor for reading. This stream will be used
    * as our handle from which to read BSON.
    */
   fd = fdopen (sock, "r");
   if (!fd) {
      perror ("bson-streaming-remote-read: fdopen failure");
   }

   return fd;
}


/*
 * main --
 *
 *       Connects to a server and reads BSON from it. This program takes the
 *       following command line options:
 *
 *          -h              Print this help and exit.
 *          -s SERVER_NAME  Specify the host name of the server.
 *          -p PORT_NUM     Specify the port number to connect to on the server.
 */
int
main (int   argc,
      char *argv[])
{
   FILE           *fd;
   bson_reader_t  *reader;
   const bson_t   *document;
   char           *hostname = NULL;
   char           *json;
   int             opt;
   int             port;

   opterr = 1;

   /* 
    * Parse command line arguments.
    */
   while ((opt = getopt (argc, argv, "hs:p:")) != -1) {
      switch (opt) {
      case "h":
         printf ("Usage: bson-streaming-reader [-s SERVER_NAME] [-p PORT_NUM]");
         return EXIT_SUCCESS;
      case "p":
         port = (int)(strtol (optarg, NULL, 0));
         break;
      case "s":
         free (hostname);
         hostname = (char *)malloc (strlen (optarg));
         strcpy (hostname, optarg);
         break;
      default:
         fprintf (stderr, "Unknown option: %s\n", optarg);
      }
   }

   /*
    * Open a file descriptor on the remote and read in BSON documents, one by
    * one. As an example of processing, this prints the incoming documents as
    * JSON onto STDOUT.
    *
    * TODO Perhaps talk about &read and &fclose? Or at least mention that these
    * are the bson_reader_read_func_t and bson_reader_destroy_func_t functions,
    * respectively, that exist in the documentation
    */
   fd = bson_streaming_remote_read (hostname, port);
   reader = bson_reader_new_from_handle (fd, &read, &fclose);
   while ((document = bson_reader_read (reader, NULL))) {
      json = bson_as_json (document, NULL);
      fprintf (stdout, "%s\n", json);
      bson_free (str);
   }

   /*
    * Destroy the reader, which performs cleanup. This calls the
    * bson_reader_destroy_func_t callback fclose(2) to close the file
    * descriptor.
    */
   bson_reader_destroy (reader);
   free (hostname);
   return EXIT_SUCCESS;
}
