/*
 * License : Apache 2.0
 *
 * ClientSet.h
 *
 *  Created on: Jan 28, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef CLIENTSET_H_
#define CLIENTSET_H_

#include "application.h"
#include "ClientSRD.h"

class ClientSet {
private:
	pthread_mutex_t csMutex;
    clientInfo *clients;
    int maxClients;
    int numClients;
public:
	ClientSet();
	virtual ~ClientSet();

	bool initialize (int maximumClients);
	int newClient (int sock); // Adds a new client
	bool startClient (int index);
	bool waitToFinish (void); // returns only when all clients have finished their work
	void checkStaleClients (void);
	void terminateClient (int index);
	int  processFirstMessage (struct clientInfo *cinfo, char *command, char *outBuffer, int outBufferSize);
	void forceTermination (void);
	bool isDataStoreInUse (DataStore *ds);
	void deleteClient (int index);
	void saveClientBackConnectionInfo (struct clientInfo *cinfo, char *IPAddress, int port);
	bool openBackConnection (struct clientInfo *cinfo);
	void closeBackConnection (struct clientInfo *cinfo);
	static void *thrdMain (void *arg); // main routine for every client thread
	static int ProcessMessage (
			clientInfo *cinfo,
			char **recvline,
			int *maxInMsgSize,
			char *sendline,
			int maxOutMsgSize,
			int *msgSize,
			int *numBytesRead);
	static xmlXPathObjectPtr GetNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log);
	static xmlChar *GetFirstNodeValue (xmlDocPtr doc, xmlChar *xpath);
};

#endif /* CLIENTSET_H_ */
