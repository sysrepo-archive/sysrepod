/*
 * mainSRDClient.c
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

   if (!srd_setDataStore(sockfd, dataStoreName)){
	   printf ("Error in setting Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (1);
   }

   printf ("Testing unlock without locking data store.\n");
   srd_unlockDataStore (sockfd);
   srd_unlockDataStore (sockfd);

   // Example of a simple XPATH
   strcpy (xpath, "/hosts/host/interfaces/interface/name");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
	   printf ("Result of XPATH is: %s\n", value);
	   free (value);
   } else {
	   printf ("Result of XPATH not found OR is NULL.\n");
   }
   // Example of getting the name node of the first interface
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found OR is NULL.\n");
   }
   // Example of extracting the atomic value of a single node
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name/text()");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found OR is NULL.\n");
   }
   // Example of extracting multiple atomic values, values will be separated by '; '
   strcpy (xpath, "/hosts/host/interfaces/interface/name/text()");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found OR is NULL.\n");
   }

   // Find the count of 'interface' nodes and print their names
    strcpy (xpath, "count(/hosts/host/interfaces/interface)");
    printf ("About to send xpath to server : %s\n", xpath);
    srd_applyXPath (sockfd, xpath, &value);
    if (value){
 	   int num, i;
        printf ("The count of <interface> nodes is: %s\n", value);
        num = atoi (value);
        free (value);
        for (i=1; i <= num; i++){
     	   sprintf (xpath, "/hosts/host/interfaces/interface[%d]/name/text()", i);
     	   srd_applyXPath (sockfd, xpath, &value);
     	   if (value){
     	      printf ("Name of interface at position %d is: %s\n", i, value);
     	      free (value);
     	   } else {
     	      printf ("Name of interface not found.\n");
     	   }
        }
    } else {
       	   printf ("Result of XPATH not found OR is NULL.\n");
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
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name");
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
   	   printf ("Result of XPATH is: %s\n", value);
   	   free (value);
   } else {
   	   printf ("Result of XPATH not found OR is NULL.\n");
   }

   // Add a new sub-tree to all <interface> nodes
   strcpy (xpath, "/hosts/host/interfaces/*");
   strcpy (newValue, "<street>1 Infinity Loop</street><city>Cupertino</city><state>CA</state>");
   if ((n=srd_addNodes (sockfd, xpath, newValue)) < 0){
	   printf ("Error in adding a subtree to the nodes selected by XPath %s\n", xpath);
   } else {
	   printf ("Added new subtree formed using the XML:\n%s\nto %d number of nodes selected using XPath %s\n", newValue, n, xpath );
	   // printf the contents of the Data Store again to show the new content
	   strcpy (xpath, "/*");
	   srd_applyXPath (sockfd, xpath, &value);
	   if (value){
	      printf ("Updated Tree content is: \n%s\n", value);
	      free (value);
	   } else {
	      printf ("Result of XPATH not found.\n");
	   }
   }

   // create a new data store and retrieve its contents
   strcpy (dataStoreName, "configure");
   strcpy (newValue, "<hosts><host><name>buzz</name><domain>tail-f.com</domain><defgw>192.168.1.1</defgw><interfaces><interface><name>eth0</name><ip>192.168.1.61</ip><mask>255.255.255.0</mask><enabled>true</enabled></interface><interface><name>eth1</name><ip>10.77.1.44</ip><mask>255.255.0.0</mask><enabled>false</enabled></interface></interfaces></host><host><name>jorba</name><domain>cisco.com</domain><defgw>192.168.111.111</defgw><interfaces><interface><name>atm0</name><ip>192.168.111.61</ip><mask>255.255.255.0</mask><enabled>true</enabled></interface></interfaces></host></hosts>");
   if (!srd_createDataStore (sockfd, dataStoreName, newValue, NULL, NULL)){
	   printf ("Error in creating a new data store : %s\n", dataStoreName);
   } else {
	   // print the contents of the new data store
	   strcpy (xpath, "/*");
	   srd_setDataStore (sockfd, dataStoreName);
	   srd_applyXPath (sockfd, xpath, &value);
	   if (value){
		   printf ("The contents of the new data store are: \n%s\n", value);
		   free (value);
	   } else {
		   printf ("Unable to get the contents of the new data store: %s.\n", dataStoreName);
	   }
   }
   if (srd_listDataStores (sockfd, &dsList)){
	   if (dsList) printf ("Data Store list is: %s\n", dsList);
	   else        printf ("Data Store list is empty.\n");
   } else {
	   printf ("Error in getting list of data stores.\n");
   }
   if (dsList) free (dsList);
   dsList = NULL;

   // Delete all nodes under <hosts> in 'configure' data store and print contents again
   strcpy (xpath, "/hosts/*");
   n = srd_deleteNodes (sockfd, xpath);
   printf ("From 'configure' data store, deleted %d number of nodes.\n\n", n);
   printf ("The updated contents of 'configure' are:\n");
   strcpy (xpath, "/*");
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
   		   printf ("The contents of the data store are: \n%s\n", value);
   		   free (value);
   } else {
   		   printf ("Unable to get the contents of the data store: %s.\n", dataStoreName);
   }

   // The following command should fail as Data Store is in use.
   n = srd_deleteDataStore (sockfd, dataStoreName);
   if (n > 0){
	   printf ("Data Store %s deleted.\n", dataStoreName);
   } else if (n == 0){
	   printf ("Data Store %s not found\n", dataStoreName);
   } else {
	   printf ("Unable to delete the Data Store '%s' as it is in use.\n", dataStoreName);
   }
   // Try to delete 'configure' data store again after setting a different data store for this client so that
   // the data store 'configure' is not in use.
   strcpy (dataStoreName, "runtime");
   srd_setDataStore (sockfd, dataStoreName);
   // It is also possible to set the Data Store for a client to be NULL
   srd_setDataStore (sockfd, NULL);

   strcpy (dataStoreName, "configure");
   srd_deleteDataStore (sockfd, dataStoreName);

   // Print the list of data stores again
   if (srd_listDataStores (sockfd, &dsList)){
   	   if (dsList) printf ("Data Store list is: %s after deleting 'configure' data store\n", dsList);
   	   else        printf ("Data Store list is empty.\n");
   } else {
   	   printf ("Error in getting list of data stores.\n");
   }
   if (dsList) free (dsList);

   /******** Demonstrate the API usage for Operational Data Stores ******************/

   strcpy (dataStoreName, "op_01");
   n = srd_createOpDataStore (sockfd, dataStoreName);
   if (n == 1){
	   printf ("Successfully added Operational Data Store %s\n", dataStoreName);
   } else {
	   printf ("Failed to add Operational Data Store : %s\n", dataStoreName);
   }
   // add another one, not checking error this time
   strcpy (dataStoreName, "op_02");
   srd_createOpDataStore (sockfd, dataStoreName);

   // Print a list of the Operational Data Stores
   if (srd_listOpDataStores (sockfd, &value)){
	  printf ("The list of Operational Data Stores is: %s\n", value);
	  free (value);
   } else {
	  printf ("Failed to get the list of Operational Data Stores\n");
   }

   // For this client, specify the usage of these two Operational Data Stores
   strcpy (dataStoreName, "op_01");
   n = srd_useOpDataStore (sockfd, dataStoreName);
   if (!n){
	   printf ("Failed to set the usage of Operational Data Store %s for this client.\n", dataStoreName);
   }
   strcpy (dataStoreName, "op_02");
   n = srd_useOpDataStore (sockfd, dataStoreName);
   if (!n){
   	   printf ("Failed to set the usage of Operational Data Store %s for this client.\n", dataStoreName);
   }
   if (srd_listMyUsageOpDataStores (sockfd, &value)){
	   printf ("The list of Operational Data Stores used by me are : %s\n", value);
	   free (value);
   } else {
	   printf ("Failed to get the usage of Operational Data Stores by this client.\n");
   }
   if ((n = srd_stopUsingOpDataStore (sockfd, dataStoreName)) == 0){
	   printf ("Failed to stop the usage of %s Operational Data Store by this client.\n", dataStoreName);
   }
   // list usage again
   if (srd_listMyUsageOpDataStores (sockfd, &value)){
	   printf ("The list of Operational Data Stores used by me are : %s\n", value);
	   free (value);
   }

   n = srd_deleteOpDataStore (sockfd, dataStoreName);
   if (n >= 0){
   	 printf ("Successfully deleted Operational Data Store %s\n", dataStoreName);
   } else {
   	 printf ("Failed to delete Operational Data Store : %s\n", dataStoreName);
   }

   // Print a list of the Operational Data Stores
   if (srd_listOpDataStores (sockfd, &value)){
   	  printf ("The list of Operational Data Stores is: %s\n", value);
   	  free (value);
   } else {
   	  printf ("Failed to get the list of Operational Data Stores\n");
   }

   /****  Example Code to receive a request from SYSREPOD for Operational Data and respond to it. *****/
   // The code to illustrate the usage of Operational Data Store is for demonstration purpose only, there is no non-terminating loop
   // etc. to form a daemon. It just reads one request for Op Data Store and exits. In a real daemon, the code will be quite different.

   strcpy (dataStoreName, "op_01"); // assuming that I have one Op Data Store called 'op_01'
   printf ("Waiting for one sample request to apply XPath on one of my Operational Data Stores.....\n");
   fflush (stdout);
   // assume there is one operation data store 'op_01' is being maintained by this South Client.
   strcpy (myOpDataStoreXML, "<data><interfaces-state><interface><name></name><type></type><admin-status></admin-status><oper-status></oper-status><last-change></last-change><if-index></if-index><phys-address></phys-address><higher-layer-if></higher-layer-if><lower-layer-if></lower-layer-if><speed></speed><statistics><discontinuity-time></discontinuity-time><in-octets></in-octets><in-unicast-pkts></in-unicast-pkts><in-broadcast-pkts></in-broadcast-pkts> <in-multicast-pkts></in-multicast-pkts><in-discards></in-discards> <in-errors></in-errors> <in-unknown-protos></in-unknown-protos> <out-octets></out-octets> <out-unicast-pkts></out-unicast-pkts> <out-broadcast-pkts></out-broadcast-pkts> <out-multicast-pkts></out-multicast-pkts> <out-discards></out-discards> <out-errors/> </statistics> </interface> </interfaces-state> </data>");
   myOpDataStore = xmlReadMemory (myOpDataStoreXML, strlen (myOpDataStoreXML), "noname.xml", NULL, 0);
   if (myOpDataStore == NULL){
	   printf ("Local error: Failed to make DOM for my Op Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (0);
   }

   // Need to give a socket to the SysrepoD on which this daemon will be listening on to receive Op Data Store related commands.
   char myIPAddress[100] = "127.0.0.1";
   int  myPort = 3510;
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
