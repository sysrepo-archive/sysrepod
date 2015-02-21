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
int  srd_setDatastore (int sockfd, char *dsname);
void srd_disconnect (int sockfd);       // disconnect this client from server, server keeps running
void srd_terminateServer  (int sockfd); // teminate server
void srd_applyXPath (int sockfd, char *xpath, char **value); // get the results of apply XPath expression, after the call the pointer *value must be freed
int  srd_lockDataStore (int sockfd);          // lock data store, returns 1 on success, 0 on failure
int  srd_unlockDataStore (int sockfd);        // unlock data store, returns 1 on success, 0 on failure
int  srd_updateNodes (int sockfd, char *xpath, char *value); // update the value of xml nodes specified by XPATH. Returns number of nodes modified. -1 means error.

#endif /* SRD_H_ */
