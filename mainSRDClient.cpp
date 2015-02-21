/*
 * mainSRDClient.cpp
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

#include "srd.h"

#define MSGLENFIELDWIDTH 7

int main(int argc, char**argv)
{
   int sockfd, n;
   char sendline[1000];
   char payload [1000];
   char recvline[1000];
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int  toread, readSoFar;
   int  msgSize, totalMsgSize;
   int serverPort = 3500;
   char dataStoreName [] = "runtime";
   char xpath[100];
   char *value;
   char newValue[100];

   if (argc != 2)
      serverIP = defaultServerIP;
   else
      serverIP = argv[1];

   if (!srd_connect (serverIP, serverPort, &sockfd)){
	   printf ("Error in connecting to server %s at port %d\n", serverIP, serverPort);
	   exit (1);
   }

   if (!srd_setDatastore(sockfd, dataStoreName)){
	   printf ("Error in setting Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (1);
   }
   strcpy (xpath, "/hosts/host/interfaces/interface/name");
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
	   printf ("Result of XPATH is: %s\n", value);
	   free (value);
   } else {
	   printf ("Result of XPATH not found.\n");
   }

   // change the name of the first interface to 'new_eth0'
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name");
   strcpy (newValue, "new_eth0");
   n = srd_updateNodes (sockfd, xpath, newValue);
   if (n < 0){
	   printf ("Error in updating the value of %s.\n", xpath);
   } else {
	   printf ("Successfully updated value of '%s' \n     with a new value = '%s'\nNumber of nodes modified = %d\n", xpath, newValue, n);
   }
   // Print content again
   strcpy (xpath, "/hosts/host/interfaces/interface/name");
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
   	   printf ("Result of XPATH is: %s\n", value);
   	   free (value);
   } else {
   	   printf ("Result of XPATH not found.\n");
   }

   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}

