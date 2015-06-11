/*
 * mainSignalExample.c
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
#include <sys/types.h>
#include <signal.h>

#include "srd.h"


// Define the function to be called when SIGHUP signal is sent to this sprocess
void
signal_callback_handler(int signum)
{
	printf(">>>>>>>>>>>>>> CAUGHT SIGNAL %d <<<<<<<<<<<<<<<<<<<<\n", signum);
}

void
modifyDataStore (int sockfd)
{
	char xpath[200];
	char newValue[200];
	int n;

	// Add a new sub-tree to all <interface> nodes
	strcpy (xpath, "/hosts/host/interfaces/*");
	strcpy (newValue, "<street>1 Infinity Loop</street><city>Cupertino</city><state>CA</state>");
    if ((n=srd_addNodes (sockfd, xpath, newValue)) < 0){
		printf ("Error in adding a subtree to the nodes selected by XPath %s\n", xpath);
	} else {
		printf ("Added new subtree formed using the XML:\n%s\nto %d number of nodes selected using XPath %s\n", newValue, n, xpath );
		// printf the contents of the Data Store again to show the new content
	}
}

#define MSGLENFIELDWIDTH 7

int main(int argc, char**argv)
{
   int sockfd, n;
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "runtime";

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

   // Register signal and signal handler
   signal(SIGHUP, signal_callback_handler);

   // Instruct SysrepoD to send singal SIGHUP when the data store changes
   if (!srd_registerClientSignal (sockfd, getpid (), SIGHUP)){
       printf ("Failed to register signal with server. \n");
   } else {
	   // modify data store to generate a signal for this process
	   modifyDataStore (sockfd);
   }

   printf ("Going to sleep for 20 seconds before exiting.......\n");
   fflush (stdout);
   sleep (20);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   xmlCleanupParser();
   exit (0);
}
