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

void
createListeningSocket(int *listenfd, int myPort)
{
	int sock;
	struct sockaddr_in servaddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
	   printf ("Error in creating listening socket.\n");
	   *listenfd = -1;
	} else {
	   bzero(&servaddr, sizeof(servaddr));
	   servaddr.sin_family = AF_INET;
	   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	   servaddr.sin_port =  htons(myPort);
	   if (bind(sock, (struct sockaddr *) &servaddr, sizeof(servaddr))){
		   printf ("Fatal Error: Could not bind to the server port %d. May be it is in use or in TIME_WAIT state.\n", myPort);
		   printf ("Use 'netstat -nap | grep %d' command to see its status.\n", myPort);
		   *listenfd = -1;
	   } else {
		   *listenfd = sock;
	   }
	}
}

int
acceptClients(int listenfd)
{
   int iResult;
   fd_set rfds;
   int s;

   FD_ZERO(&rfds);
   FD_SET(listenfd, &rfds);
   iResult = select(listenfd+1, &rfds, (fd_set *) 0, (fd_set *) 0, NULL); // Last param == NULL means No Timeout
   if(iResult > 0){
      s =  accept(listenfd, NULL, NULL);
      return s;
   } else{
      return -1; // error
   }
}

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
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "huge";
   char xpath[100];
   char *value;
   char newValue[1000];
   char *dsList = NULL;
   char *buffPtr = NULL;
   	int buffSize = 100;
   	char myOpDataStoreXML[3000];
   	xmlDocPtr myOpDataStore;

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
   free (buffPtr);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}
