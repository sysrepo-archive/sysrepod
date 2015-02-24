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

int  srd_connect (char *serverIP, int serverPort, int *sockfd);
int  srd_setDataStore (int sockfd, char *dsname); // Client specify which data store it is going to work with.
void srd_disconnect (int sockfd);       // disconnect this client from server, server keeps running
void srd_terminateServer  (int sockfd); // teminate server
void srd_applyXPath (int sockfd, char *xpath, char **value); // get the results of apply XPath expression, after the call the pointer *value must be freed
int  srd_lockDataStore (int sockfd);          // lock data store, returns 1 on success, 0 on failure
int  srd_unlockDataStore (int sockfd);        // unlock data store, returns 1 on success, 0 on failure
int  srd_updateNodes (int sockfd, char *xpath, char *value); // update the value of xml nodes specified by XPATH. Returns number of nodes modified. -1 means error.
int  srd_createDataStore (int sockfd, char *name, char *value, char *xsdDir, char *xsltDir); // It adds a new data store. Returns 1 on success, 0 on failure.
bool srd_listDataStores (int sockfd, char **result); // Retrieves the list of active data stores. On failure return false.
int  srd_deleteDataStore (int sockfd, char *name); //Deletes a data store. On success returns 0 or +ve number, on error -1.

#endif /* SRD_H_ */
