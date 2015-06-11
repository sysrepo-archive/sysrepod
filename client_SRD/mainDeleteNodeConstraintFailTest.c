/*
 * mainXSLTExample.c
 *
 * License: Apache 2.0
 *
 *  Created on: April 25, 2015
 *      Creator: Niraj Sharma
 *      Cisco Systems, Inc.
 *
 */

#define MAIN_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "srd.h"

#define MSGLENFIELDWIDTH 7

int main(int argc, char**argv)
{
   int sockfd, n;
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "runtime";
   char xpath[50];

   if (argc != 2)
      serverIP = defaultServerIP;
   else
      serverIP = argv[1];

   if (!srd_connect (serverIP, serverPort, &sockfd)){
	   printf ("Error in connecting to server %s at port %d\n", serverIP, serverPort);
	   exit (1);
   }

   if (!srd_setDataStore(sockfd, dataStoreName)){
	   printf ("Error in setting Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (1);
   }

   // Delete all nodes under <hosts> in 'configure' data store and print contents again
   strcpy (xpath, "/hosts/*");
   n = srd_deleteNodes (sockfd, xpath);

   printf ("Going to sleep for 10 seconds before exiting.......\n");
   fflush (stdout);
   sleep (10);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   xmlCleanupParser();
   exit (0);
}
