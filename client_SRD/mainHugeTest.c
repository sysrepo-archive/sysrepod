/*
 * mainHugeTest.c
 *
 * License: Apache 2.0
 *
 *  Created on: Jan 25, 2015
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
   char dataStoreName [100] = "huge";
   char xpath[100];
   char *value;

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

   // Example of a simple XPATH
   strcpy (xpath, "//leaf1");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
	   printf ("Result of XPATH is: %s\n", value);
	   free (value);
   } else {
	   printf ("Result of XPATH not found OR is NULL.\n");
   }

   printf ("Going to sleep for 20 seconds before exiting.......\n");
   fflush (stdout);
   sleep (20);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}
