/*
 * application.h
 *
 * License: Apache 2.0
 *
 *  Created on: Jan 25, 2015
 *      Creator: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <pthread.h>

// For clients, there are two states: CLNT_ENDED, CLNT_ACTIVE
enum clientState {CLNT_ENDED, CLNT_ACTIVE};

// Different clients may speak different protocols. The following protocols are supported
enum protocolValue {INVALID_PROTOCOL, srd, cdb, MAX_PROTOCOL_VALUE};

class Client;
class ClientSet;
class DataStore;

typedef struct clientInfo{
	int sock;
	enum clientState state;
	pthread_t thrdId; // The thread handling communication for the client
    bool slotFree;
    char logLine[100];
    enum protocolValue protocol;
    DataStore *dataStore;
    Client *client;
    ClientSet *clientSet;
    int index;
    char clientIPAddress[50];
    int  clientPort;
    int  clientBackSock;
} clientInfo;

#ifdef MAIN_C_

#else

#endif /* MAIN_C */

#endif /* APPLICATION_H_ */
