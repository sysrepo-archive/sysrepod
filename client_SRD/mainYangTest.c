/*
 * mainYangTest.c
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

int main(int argc, char**argv)
{
   int sockfd, n;
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "sshd_config";
   char xpath[100];
   char newValue[1000];

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

   printf ("Currently, sshd_config data store does not have any error.\n");
   printf ("Let us add a new node <HostKeyAgent/> that will violate Yang model.\n");

   // Add a new node
   //strcpy (xpath, "/config/sshd_config_options/*");
   strcpy (xpath, "/*");
   strcpy (newValue, "<HostKeyAgent/>");
   if ((n=srd_addNodes (sockfd, xpath, newValue, MODIFY_WITH_VALIDATION)) < 0){
	   printf ("Error in adding a new node to the nodes selected by XPath %s\n", xpath);
   } else {
	   printf ("Added new node:\n%s\nto %d number of nodes selected using XPath %s\n", newValue, n, xpath );
   }

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
