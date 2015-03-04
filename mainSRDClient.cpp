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
#include <unistd.h>

#include "srd.h"

#define MSGLENFIELDWIDTH 7

void
opDataStoreHandleXPath (int sockfd, xmlDocPtr ds, xmlChar *xpathExpr)
{
	char *contentBuff = NULL;
	int   contentBuffSize = 100;
	int len = 0;
	char sendline[100];
	xmlXPathObjectPtr xpathObj_local;
	xmlNodeSetPtr nodes_local;
	char log[200];

	contentBuff = (char *)malloc (contentBuffSize);
	if (!contentBuff){
	    printf ("Unable to allocate buffer to apply xpath.\n");
	    sprintf (sendline, "<xml><error>Error in memory allocation</error></xml>");
	    srd_sendServer(sockfd, sendline, strlen(sendline));
	    return;
	}
	xpathObj_local = srd_getNodeSet (ds, (xmlChar *) xpathExpr, log);
	if (xpathObj_local != NULL){
	    nodes_local = xpathObj_local->nodesetval;
	    if (nodes_local->nodeNr > 0){
	    	len = srd_printElementSet (ds, nodes_local, &contentBuff, contentBuffSize);
	    	if (len < 0){
	    		printf ("Unable to read content XPath result.\n");
	    		sprintf (sendline, "<xml><error>Unable to read content XPath result</error></xml>");
	    		srd_sendServer(sockfd, sendline, strlen(sendline));
	    	} else {
	    		// send the XPath result to server
	    		char *msg;
	    		msg = (char *) malloc (strlen(contentBuff) + strlen ("<xml><ok></ok></xml>") + 2);
	    		if (msg == NULL){
	    			sprintf (sendline, "<xml><error>Error in memory allocation</error></xml>");
	    		    srd_sendServer(sockfd, sendline, strlen(sendline));
	    		} else {
	    			sprintf (msg, "<xml><ok>%s</ok></xml>", contentBuff);
	    			srd_sendServer (sockfd, msg, strlen (msg));
	    			free (msg);
	    		}
	    	}
	    } else {
	    	int n;
	    	n = sprintf (contentBuff, "<xml><ok/></xml>");
	    	srd_sendServer (sockfd, contentBuff, n);
	    }
	    xmlXPathFreeObject (xpathObj_local);
   } else {
	   int n;
	   n =sprintf (contentBuff, "<xml><ok/></xml>");
	   srd_sendServer (sockfd, contentBuff, n);
   }
   free (contentBuff);
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
   int serverPort = 3500;
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

   // Example of a simple XPATH
   strcpy (xpath, "/hosts/host/interfaces/interface/name");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
	   printf ("Result of XPATH is: %s\n", value);
	   free (value);
   } else {
	   printf ("Result of XPATH not found.\n");
   }
   // Example of getting the name node of the first interface
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found.\n");
   }
   // Example of extracting the atomic value of a single node
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name/text()");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found.\n");
   }
   // Example of extracting multiple atomic values, values will be separated by '; '
   strcpy (xpath, "/hosts/host/interfaces/interface/name/text()");
   printf ("About to send xpath to server : %s\n", xpath);
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
        printf ("Result of XPATH is: %s\n", value);
        free (value);
   } else {
        printf ("Result of XPATH not found.\n");
   }

   // Find the count of 'interface' nodes and print their names
    strcpy (xpath, "count(/hosts/host/interfaces/interface)");
    printf ("About to send xpath to server : %s\n", xpath);
    srd_applyXPath (sockfd, xpath, &value);
    if (value){
 	   int num, i;
        printf ("The count of interface nodes is: %s\n", value);
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
   strcpy (xpath, "/hosts/host/interfaces/interface[1]/name");
   srd_applyXPath (sockfd, xpath, &value);
   if (value){
   	   printf ("Result of XPATH is: %s\n", value);
   	   free (value);
   } else {
   	   printf ("Result of XPATH not found.\n");
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
		   printf ("The contents of the new data store are: %s\n", value);
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

   // The following command should fail as Data Store is in use.
   n = srd_deleteDataStore (sockfd, dataStoreName);
   if (n > 0){
	   printf ("Data Store %s deleted.\n", dataStoreName);
   } else if (n == 0){
	   printf ("Data Store %s not found\n", dataStoreName);
   } else {
	   printf ("Error in deleting the Data Store '%s' as it is in use.\n", dataStoreName);
   }
   // Try to delete 'configure' data store again after setting a different data store for this client so that
   // the data store 'configure' is not in use.
   strcpy (dataStoreName, "runtime");
   srd_setDataStore (sockfd, dataStoreName);
   // It is also possible to set the Data Store for a client to be NULL
   srd_setDataStore (sockfd, NULL);
   strcpy (dataStoreName, " ");
   srd_setDataStore (sockfd, dataStoreName);
   strcpy (dataStoreName, "configure");
   srd_deleteDataStore (sockfd, dataStoreName);

   // Print the list of data stores again
   if (srd_listDataStores (sockfd, &dsList)){
   	   if (dsList) printf ("Data Store list is: %s\n", dsList);
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

   //Due to bug in the last section: EXIT NOW.
   //srd_disconnect (sockfd);
   //exit(0);

   // The following code can be read as it is approximately correct, there is some BUG that is being fixed.
   /*  Example Code to receive a request from SYSREPOD for Operational Data and respond to it. */

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
   buffPtr = (char *) malloc (buffSize);
   if (buffPtr == NULL){
	   printf ("Local error: unable to allocate memory.\n");
	   srd_disconnect (sockfd);
	   xmlFreeDoc (myOpDataStore);
	   exit (0);
   }
   n = srd_recvServer (sockfd, &buffPtr, &buffSize);
   if (n == 0){
   		printf ("Call to read command from server returned 0 - failure\n");
   } else {
	  xmlDocPtr doc = NULL;
      xmlChar *command;
      xmlChar *param1, *param2;

   	  doc = xmlReadMemory(&(buffPtr[MSGLENFIELDWIDTH + 1]), strlen (&(buffPtr[MSGLENFIELDWIDTH + 1])), "noname.xml", NULL, 0);
      if (doc == NULL){
   			sprintf (sendline, "<xml><error>XML Document not correct</error></xml>");
   			srd_sendServer (sockfd, sendline, strlen (sendline));
   	  } else {
   	     strcpy((char *)xpath, "/xml/command");
         command = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
         if (command == NULL){
   			sprintf (sendline, "<xml><error>No command found.</error></xml>");
   			srd_sendServer(sockfd, sendline, strlen (sendline));
   	     } else if (strcmp ((char *)command, "apply_xpathOpDataStore") == 0){
   	    			// param1 contains Op Data Store Name, param2 contains XPath
   	    			strcpy ((char *) xpath, "/xml/param1");
   	    			param1 = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
   	    			if(param1 == NULL){
   	    			    sprintf (sendline, "<xml><error>Value for Op Data Store name not found</error></xml>");
   	    			    srd_sendServer(sockfd, sendline, strlen(sendline));
   	    			} else if (strcmp((char *)param1, dataStoreName) != 0){
   	    				sprintf (sendline, "<xml><error>Unknown Operational Data Store name: %s</error></xml>", (char *)param1);
   	    				xmlFree (param1);
   	    				srd_sendServer (sockfd, sendline, strlen (sendline));
   	    			} else {
   	    				char log[100];
   	    				strcpy ((char *) xpath, "/xml/param2");
   	    				param2 = srd_getFirstNodeValue(doc, (xmlChar *)xpath);
   	    				if(param2 == NULL){
   	    				    sprintf (sendline, "<xml><error>XPath not found</error></xml>");
   	    				    srd_sendServer(sockfd, sendline, strlen(sendline));
   	    				} else {
   	    					// apply xpath and return results back to sysrepod
   	    					opDataStoreHandleXPath (sockfd, myOpDataStore, param2);
   	    					xmlFree (param2);
   	    				}
   	    			    xmlFree (param1);
   	    			}
   	    	free (command);
   	     }
         xmlFreeDoc (doc);
   	  }
   }
   xmlFreeDoc (myOpDataStore);
   free (buffPtr);
   /************  END of Example Code to receive a request from SYSREPOD for Operational Data and respond to it. *******/

   /********* END of examples for Operational Data Stores ***************************/

   while (1) sleep (2);
   fflush (stdout);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}

