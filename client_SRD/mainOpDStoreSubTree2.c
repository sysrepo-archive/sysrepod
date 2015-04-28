/*
 * mainOpDstoreSubtree2.c
 *
 * License: Apache 2.0
 *
 *  Created on: March 25, 2015
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
   char dataStoreName [100] = "runtime";
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

   /******** Demonstrate the API usage for Operational Data Stores ******************/
   /*
    * This client will maintain only a small tree out of the whole standard IETF tree.
    * This subtree is at the following path:
    *  /data/interfaces-state/interface[2]
    *
    *  The first interface will be maintained by other client.
    */

   strcpy (dataStoreName, "urn:ietf:params:xml:ns:yang:ietf-interfaces/interfaces-state");
   n = srd_createOpDataStore (sockfd, dataStoreName);
   if (n == 1){
	   printf ("Successfully added Operational Data Store %s\n", dataStoreName);
   } else {
	   printf ("Failed to add Operational Data Store : %s\n", dataStoreName);
   }
   // For this client, specify the usage of the Operational Data Store created
   n = srd_useOpDataStore (sockfd, dataStoreName);
   if (!n){
	   printf ("Failed to set the usage of Operational Data Store %s for this client.\n", dataStoreName);
   }

   /****  Example Code to receive a request from SYSREPOD for Operational Data and respond to it. *****/
   // The code to illustrate the usage of Operational Data Store is for demonstration purpose only, there is no non-terminating loop
   // etc. to form a daemon. It just reads one request for Op Data Store and exits. In a real daemon, the code will be quite different.

   printf ("Waiting for one sample request to apply XPath on one of my Operational Data Stores.....\n");
   fflush (stdout);
   // assume there is one operation data store is being maintained by this South Client.
   strcpy (myOpDataStoreXML, "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
		   "<data xmlns=\"urn:ietf:params:xml:ns:netconf:base:1.0\">"
		   "  <interfaces xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
		   "    <interface>"
		   "      <name>eth0</name>"
		   "      <description/>"
		   "      <type>ethernet</type>"
		   "      <link-up-down-trap-enable/>"
		   "    </interface>"
		   "  </interfaces>"
		   "  <interfaces-state xmlns=\"urn:ietf:params:xml:ns:yang:ietf-interfaces\">"
		   "    <interface>"
		   "      <name/>"
		   "      <type/>"
		   "      <admin-status/>"
		   "      <oper-status/>"
		   "      <last-change/>"
		   "      <if-index/>"
		   "      <phys-address/>"
		   "      <higher-layer-if/>"
		   "      <lower-layer-if/>"
		   "      <speed>1500</speed>"
		   "      <statistics>"
		   "        <discontinuity-time/>"
		   "        <in-octets/>"
		   "        <in-unicast-pkts/>"
		   "        <in-broadcast-pkts/>"
		   "        <in-multicast-pkts/>"
		   "        <in-discards/>"
		   "        <in-errors/>"
		   "        <in-unknown-protos/>"
		   "        <out-octets/>"
		   "        <out-unicast-pkts/>"
		   "        <out-broadcast-pkts/>"
		   "        <out-multicast-pkts/>"
		   "        <out-discards/>"
		   "        <out-errors/>"
		   "      </statistics>"
		   "    </interface>"
		   "  </interfaces-state>"
		   "</data>");
   myOpDataStore = xmlReadMemory (myOpDataStoreXML, strlen (myOpDataStoreXML), "noname.xml", NULL, 0);
   if (myOpDataStore == NULL){
	   printf ("Local error: Failed to make DOM for my Op Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (0);
   }

   // Need to give a socket to the SysrepoD on which this daemon will be listening on to receive Op Data Store related commands.
   char myIPAddress[100] = "127.0.0.1";
   int  myPort = 3522;
   int  listenfd, connfd;

   if (!srd_registerClientSocket (sockfd, myIPAddress, myPort)){ // register my socket info with SysrepoD
       printf ("Unable to tell SysrepoD about my listening port.\n");
       srd_disconnect (sockfd);
       xmlFreeDoc (myOpDataStore);
       exit (0);
   }
   // Start listening for connection request on 'myPort'
   createListeningSocket (&listenfd, myPort);
   if(listenfd != -1){
	   printf("Listening on Port %d .....\n", myPort);
       listen(listenfd, SOMAXCONN); // SOMAXCONN defined by socket.h: Max # of clients
       connfd = acceptClients (listenfd); // No Time out
       if(connfd > -1){
          printf("Received a connection request.\n");
          fflush (stdout);
       } else {
          // error
    	   printf ("Error in accepting a connection.\n");
          srd_disconnect (sockfd);
          xmlFreeDoc (myOpDataStore);
          exit (0);
       }
   } else {
	   printf ("Error in listening for connections.\n");
	   srd_disconnect (sockfd);
	   xmlFreeDoc (myOpDataStore);
	   exit (0);
   }
   buffPtr = (char *) malloc (buffSize);
   if (buffPtr == NULL){
	   printf ("Local error: unable to allocate memory.\n");
	   srd_disconnect (sockfd);
	   xmlFreeDoc (myOpDataStore);
	   close (connfd);
	   close (listenfd);
	   exit (0);
   }
   n = srd_recvServer (connfd, &buffPtr, &buffSize);
   if (n == 0){
   		printf ("Call to read command from server returned 0 - failure\n");
   } else {
	  xmlDocPtr doc = NULL;
      xmlChar *command;
      xmlChar *param1, *param2;

   	  doc = xmlReadMemory(&(buffPtr[MSGLENFIELDWIDTH + 1]), strlen (&(buffPtr[MSGLENFIELDWIDTH + 1])), "noname.xml", NULL, 0);
      if (doc == NULL){
   			sprintf (sendline, "<xml><error>XML Document not correct</error></xml>");
   			srd_sendServer (connfd, sendline, strlen (sendline));
   	  } else {
   	     strcpy((char *)xpath, "/xml/command");
         command = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
         if (command == NULL){
   			sprintf (sendline, "<xml><error>No command found.</error></xml>");
   			srd_sendServer(connfd, sendline, strlen (sendline));
   	     } else if (strcmp ((char *)command, "apply_xpathOpDataStore") == 0){
   	        // param1 contains Op Data Store Name, param2 contains XPath
   	    	strcpy ((char *) xpath, "/xml/param1");
   	    	param1 = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
   	    	if(param1 == NULL){
   	    		sprintf (sendline, "<xml><error>Value for Op Data Store name not found</error></xml>");
   	    	    srd_sendServer(connfd, sendline, strlen(sendline));
   	    	} else if (strcmp((char *)param1, dataStoreName) != 0){
   	    		sprintf (sendline, "<xml><error>Unknown Operational Data Store name: %s</error></xml>", (char *)param1);
   	    		xmlFree (param1);
   	    		srd_sendServer (connfd, sendline, strlen (sendline));
   	    	} else {
   	    		char log[100];
   	    		strcpy ((char *) xpath, "/xml/param2");
   	    		param2 = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
   	    		if(param2 == NULL){
   	    			sprintf (sendline, "<xml><error>XPath not found</error></xml>");
   	    			srd_sendServer(connfd, sendline, strlen(sendline));
   	    	    } else {
   	    		    // apply xpath and return results back to sysrepod
   	    		    srd_DOMHandleXPath (connfd, myOpDataStore, param2);
   	    		    xmlFree (param2);
   	    		}
   	    	    xmlFree (param1);
   	    	}
   	    	free (command);
   	     }
         xmlFreeDoc (doc);
   	  }
   }

   /************  END of Example Code to receive a request from SYSREPOD for Operational Data and respond to it. *******/

   /********* END of examples for Operational Data Stores ***************************/

   printf ("Going to sleep for 20 seconds before exiting.......\n");
   fflush (stdout);
   sleep (20);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   close (connfd);
   close (listenfd);
   xmlFreeDoc (myOpDataStore);
   free (buffPtr);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}
