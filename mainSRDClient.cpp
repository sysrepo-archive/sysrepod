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
   char xpath[] = "/hosts/host/interfaces/interface/name";

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
   srd_applyXPath (sockfd, xpath);

   //srd_disconnect (sockfd);
   srd_terminateServer (sockfd);
   exit (0);
}

