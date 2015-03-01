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
   char dataStoreName [100] = "runtime";
   char xpath[100];
   char *value;
   char newValue[1000];
   char *dsList = NULL;

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

   n = srd_deleteDataStore (sockfd, dataStoreName);
   if (n > 0){
	   printf ("Data Store %s deleted.\n", dataStoreName);
   } else if (n == 0){
	   printf ("Data Store %s not found\n", dataStoreName);
   } else {
	   printf ("Error in deleting the Data Store %s\n", dataStoreName);
   }

   // Print the list of data stores again
   if (srd_listDataStores (sockfd, &dsList)){
   	   if (dsList) printf ("Data Store list is: %s\n", dsList);
   	   else        printf ("Data Store list is empty.\n");
   } else {
   	   printf ("Error in getting list of data stores.\n");
   }
   if (dsList) free (dsList);

   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}

