/*
 * srd.h
 *
 *  Created on: Feb 5, 2015
 *      Author: niraj
 */

#ifndef SRD_H_
#define SRD_H_

int  srd_connect (char *serverIP, int serverPort, int *sockfd);
int  srd_setDatastore (int sockfd, char *dsname);
void srd_disconnect (int sockfd); // disconnect this client from server
void srd_terminateServer  (int sockfd); // teminate server
int  srd_isServerResponseOK (int sockfd);
void srd_applyXPath (int sockfd, char *xpath);


#endif /* SRD_H_ */
