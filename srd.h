/*
 * srd.h
 *
 * LICENSE: Apache 2.0
 *
 *  Created on: Feb 5, 2015
 *      Author: Niraj Sharma
 *      Copyright Cisco Systems, Inc.
 *      All rights reserved.
 */

#ifndef SRD_H_
#define SRD_H_

#include <libxml/xpathInternals.h>

xmlXPathObjectPtr srd_getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log);
int srd_printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize);
xmlChar *srd_getFirstNodeValue (xmlDocPtr doc, xmlChar *xpath);// applies xpath on a DOM tree and gets the value of first node
bool srd_sendServer (int sockfd, char *message, int msgSize); // To send contents to server. Returns true on success, else false.
int  srd_recvServer (int sockfd, char **buffPtr, int *buffSize);// It reads an SRD command. Returns 0 on error. On success, returns number of bytes.
int  srd_connect (char *serverIP, int serverPort, int *sockfd);
int  srd_setDataStore (int sockfd, char *dsname); // Client specify which data store it is going to work with. Returns 1 on success, 0 on failure
void srd_disconnect (int sockfd);       // disconnect this client from server, server keeps running
void srd_terminateServer  (int sockfd); // teminate server
void srd_applyXPath (int sockfd, char *xpath, char **value); // get the results of apply XPath expression, after the call the pointer *value must be freed
int  srd_lockDataStore (int sockfd);          // lock data store, returns 1 on success, 0 on failure
int  srd_unlockDataStore (int sockfd);        // unlock data store, returns 1 on success, 0 on failure
int  srd_updateNodes (int sockfd, char *xpath, char *value); // update the value of xml nodes specified by XPATH. Returns number of nodes modified. -1 means error.
int  srd_createDataStore (int sockfd, char *name, char *value, char *xsdDir, char *xsltDir); // It adds a new data store. Returns 1 on success, 0 on failure.
bool srd_listDataStores (int sockfd, char **result); // Retrieves the list of active data stores. On failure return false.
int  srd_deleteDataStore (int sockfd, char *name); //Deletes a data store. On success returns 0 or +ve number, on error -1.

int  srd_createOpDataStore (int sockfd, char *name); // it adds a new Operational Data Store. Returns 1 on success, 0 on failure.
int  srd_deleteOpDataStore (int sockfd, char *name); // it deletes an Operational Data Store even if other clients are using it. Returns 1 on success, 0 on failure.
bool srd_listOpDataStores (int sockfd, char **result); // Retrieves the list of Operational Data Stores. On failure, returns false.
bool srd_listMyUsageOpDataStores (int sockfd, char **result); // Lists which Op Data Stores are used by the client. On failure, returns false.
int  srd_useOpDataStore (int sockfd, char *name); // Client wants to use an Operational Data Store. Returns 1 on success, 0 on failure.
int  srd_stopUsingOpDataStore (int sockfd, char *name); // Clients wants to stop using an Operational Data Store. Return 1 on success, 0 on failure.
void srd_applyXPathOpDataStore (int sockfd, char *opDataStoreName, char *xpath, char **value);// apply XPath on a Operational Data Store, free 'value' after use
bool srd_registerClientSocket (int sockfd, char *myIPAddress, int myPort);

#endif /* SRD_H_ */
