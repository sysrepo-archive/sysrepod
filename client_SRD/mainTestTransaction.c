/*
 * mainReplaceSubtree.c
 *
 * License: Apache 2.0
 *
 *  Created on: May 15, 2015
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

void
modifyDatastore (int sockfd)
{
	char xpath [100];
	char newValue [1000];
	int n;

	printf ("Let us replace all <interface> nodes by <address> node.\n");

	strcpy (xpath, "/hosts/host/interfaces/interface");
	strcpy (newValue, "<address><street>1 Infinity Loop</street><city>Cupertino</city><state>CA</state></address>");
	if ((n=srd_replaceNodes (sockfd, xpath, newValue, MODIFY_NO_VALIDATION)) < 0){
	   printf ("Error in replacing the nodes selected by XPath %s\n", xpath);
	} else {
		printf ("Added new node:\n%s\nto %d number of nodes selected using XPath %s\n", newValue, n, xpath );
	}
}

void
printDatastore (int sockfd)
{
	char xpath [100];
	char *value;

	// printf the contents of the Data Store to show the new content
	strcpy (xpath, "/*");
    srd_applyXPath (sockfd, xpath, &value);
	if (value){
	   printf ("Updated Tree content is: \n%s\n", value);
	   free (value);
	} else {
	   printf ("Result of XPATH not found.\n");
    }
}

int main(int argc, char**argv)
{
   int sockfd, n;
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "runtime";
   char dataStoreName1[100] = "huge";
   char xpath[100];
   char newValue[1000];
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

   if (!srd_startTransaction (sockfd)){
	   printf ("Error in starting a transaciton.\n");
	   srd_disconnect (sockfd);
	   exit (1);
   }

   printf ("\n>>>>> Data Store is set to %s. Transaction is started. Now if we switch Data Store, it should fail.\n\n", dataStoreName);
   if (!srd_setDataStore (sockfd, dataStoreName1)){
	   printf ("Error in setting data store to %s\n\n", dataStoreName1);
   }
   modifyDatastore (sockfd);
   printDatastore (sockfd);


   printf ("\n>>>>>> ABORT Transaction and print data store to check that all modification are undone.\n\n");
   if (!srd_abortTransaction(sockfd)){
	   printf ("Error - Failed to abort transaction.\n");
   } else {
	   printf ("ABORTED transaction.\n");
   }
   printDatastore (sockfd);

   printf ("\n>>>>>> Modify Data store again under a transacion, this time commit and print contents -> rollback transaction and print data store\n\n");
   if (!srd_startTransaction (sockfd)){
   	   printf ("Error in starting a transaciton.\n");
   	   srd_disconnect (sockfd);
   	   exit (1);
   }
   modifyDatastore (sockfd);
   n = srd_commitTransaction (sockfd);
   if (n > 0){
	   printf ("Transaction committed successfully.\n");
   } else {
	   printf ("Transaction commit FAILED.\n");
   }
   printf ("Rollback transaction and then printing data store.\n");
   if (!srd_rollbackTransaction(sockfd, n)){
	   printf ("Failed to rollback transaction\n");
   } else {
	   printf ("Transaction rolled back.\n");
   }

   printDatastore (sockfd);

   printf ("Going to sleep for 3 seconds before exiting.......\n");
   fflush (stdout);
   sleep (3);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   xmlCleanupParser();
   exit (0);
}
