/*
 * License : apache 2.0
 *
 * ClientSet.cpp
 *
 *  Created on: Jan 28, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "global.h"
#include "ClientSet.h"
#include "common.h"
#include "ClientSRD.h"
#include "OpDataStoreSet.h"
#include "DataStore.h"



int OutBufferSize = 1024;

ClientSet::ClientSet()
{
   maxClients = 0;
   numClients = 0;
   clients = NULL;
}

ClientSet::~ClientSet()
{
	free (clients);
	pthread_mutex_destroy (&csMutex);
}

bool
ClientSet::initialize (int maximumClients)
{
	int i;

	maxClients = maximumClients;
	if (pthread_mutex_init(&csMutex, NULL) != 0)
	{
		// mutex init failed.
		printf ("Error: Failed to init Mutex for ClientSet.\n");
		return true;
	}
	clients = (clientInfo *) calloc (maxClients, sizeof (clientInfo));
	if (clients == NULL){
		return true;
	}
	for (i=0; i < maxClients; i++){
		clients[i].slotFree = true;
	    clients[i].sock = INVALID_SOCK;
	    clients[i].client = NULL;
	    clients[i].protocol = INVALID_PROTOCOL;
	    clients[i].clientSet = this;
	    clients[i].dataStore = NULL;
	    clients[i].index = i;
	    clients[i].clientBackSock = -1;
	    clients[i].clientIPAddress[0] = '\0';
	    clients[i].clientPort = -1;
	    clients[i].clientPID = -1;
	    clients[i].signalType = -1;
	}
	return false;
}

int
ClientSet::newClient (int sock)
{
	int i;
	int slotIndex = -1;

	if (pthread_mutex_lock(&csMutex)) return -1;
	if (maxClients == numClients) return -1; // no space left
	// find a free slot
	for (i=0; i < maxClients; i++){
		if (clients[i].slotFree){
			slotIndex = i;
			break;
		}
	}
	if (slotIndex == -1){
		sprintf (LogLine, "Logical error\n");
		common::LogMsg (1, LogLine, false);
		pthread_mutex_unlock(&csMutex);
		return -1;
	}
	// initialize client slot
    clients[i].sock = INVALID_SOCK;
	clients[i].client = NULL;
	clients[i].protocol = INVALID_PROTOCOL;
	clients[i].dataStore = NULL;
	clients[i].clientBackSock = -1;
    clients[i].clientIPAddress[0] = '\0';
	clients[i].clientPort = -1;
	clients[i].clientPID = -1;
	clients[i].signalType = -1;

	clients[slotIndex].sock = sock;
	clients[slotIndex].slotFree = false;
	numClients++;
	pthread_mutex_unlock(&csMutex);
	return slotIndex;
}

void
ClientSet::deleteClient (int index)
{
	pthread_mutex_lock(&csMutex);
	clients[index].state = CLNT_ENDED;
    clients[index].slotFree = true;
	clients[index].dataStore = NULL;
    clients[index].protocol = INVALID_PROTOCOL;
	delete (clients[index].client);
	clients[index].client = NULL;
	numClients --;
	pthread_mutex_unlock(&csMutex);
}

void
ClientSet::checkStaleClients (void)
{
	// check socket value to find if stale, may be state of thread can also be checked.
	// In the thread while doing IO operaton on socket if error occurs, set socket = -1 to declare it stale. Stop thread.

}

void
ClientSet::terminateClient (int index)
{

}

bool
ClientSet::startClient (int index)
{
	void *threadMain(void *);
	int  ret;

	 clients[index].state = CLNT_ACTIVE;
	 clients[index].protocol = INVALID_PROTOCOL;
	 clients[index].client = NULL;
	 clients[index].dataStore = NULL;

    if ((ret = pthread_create (&(clients[index].thrdId), NULL, ClientSet::thrdMain, (void *) &(clients[index]))) != 0) {
	   // could not create the thread.
	   sprintf (LogLine, "Error in creating a thread.\n");
	   common::LogMsg(1, LogLine, false);
	   clients[index].sock = INVALID_SOCK;
	   return false; // error
	}
	return true;
}

// return true if not all clients done. Implement a loop to check client states. After a few iterations, return true or false
bool
ClientSet::waitToFinish (void)
{
	return true;

}

void
ClientSet::forceTermination (void)
{

}

xmlXPathObjectPtr
ClientSet:: GetNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	if(!doc) return NULL;
	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		sprintf(log, "Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		sprintf(log, "Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
        sprintf(log, "No result\n");
		return NULL;
	}
	return result;
}

int
ClientSet:: processFirstMessage (struct clientInfo *cinfo, char *command, char *outBuffer, int outBuffSize)
{
	xmlDocPtr doc;
	xmlNode *root_element = NULL;
	char *nodevalue = NULL;
	int ret = 0;
	xmlXPathObjectPtr result;
	xmlChar xpath[100];
	char log[200];
	xmlNodeSetPtr nodes;
	xmlChar *proto;

	doc = xmlReadMemory(command, strlen(command), "noname.xml", NULL, 0);
	if (doc == NULL){
		sprintf (outBuffer, "<xml><error>XML Document not correct</error></xml>");
		common::SendMessage(cinfo->sock, outBuffer);
		return ret;
	}
	// find protocol node
	strcpy ((char *)xpath, "/xml/protocol");
	result = ClientSet::GetNodeSet (doc, xpath, log);
	if (result == NULL){
		sprintf (outBuffer, "<xml><error>Portocol value not found.</error></xml>");
		common::SendMessage(cinfo->sock, outBuffer);
		xmlFreeDoc (doc);
		return ret;
	}
	nodes = result->nodesetval;
	if(nodes->nodeNr > 0){
		proto = xmlNodeListGetString(doc, nodes->nodeTab[0]->xmlChildrenNode,1);
		if (strcmp((char *)proto, "srd") == 0){
			cinfo->client = new Client_SRD ();
			cinfo->protocol = srd;
			sprintf (outBuffer, "<xml><ok/></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
		}else {
			if ((strlen((char *)proto) + 50) > outBuffSize){
				// buffer is too small, just print a short message
				sprintf (outBuffer, "<xml><error>This protocol not supported</error>");
			} else {
			    sprintf (outBuffer, "<xml><error>This protocol not supported: %s</error>", (char *) proto);
			}
			common::SendMessage(cinfo->sock, outBuffer);
		}
		xmlFree (proto);
	} else {
		sprintf (outBuffer, "<xml><error>Protocol value not found.</error></xml>");
		common::SendMessage(cinfo->sock, outBuffer);
	}
	xmlXPathFreeObject(result);
	xmlFreeDoc(doc);
	return ret;
}

int
ClientSet::ProcessMessage (
		clientInfo *cinfo,
		char **recvline,
		int   *maxInMsgSize,
		char *sendline,
		int   outBuffSize,
		int  *msgSize,
		int  *numBytesRead)
{
	int n;
	char fmt[20];
	bool ret;
	char *newBuffer;
	bool justReadSize = false;

	// First few chars in all messages reserved for message size followed by one white space.
    if (*msgSize == 0 && *numBytesRead < MSGLENFIELDWIDTH + 1){
    	justReadSize = true;
    	n= recv(cinfo->sock, &((*recvline)[*numBytesRead]), *maxInMsgSize - *numBytesRead, 0);
    	if (n < 1) return -2; // client is bad
    	*numBytesRead = *numBytesRead + n;
    	if (*numBytesRead < MSGLENFIELDWIDTH + 1) return 0; // need more bytes to proceed
        // read message size
        n = sscanf(*recvline, "%d", msgSize);
        if (n != 1){ // msg len not found
        	sprintf (sendline, "<error>Message length not found</error>");
        	ret = common::SendMessage (cinfo->sock, sendline);
        	*msgSize = 0;
        	*numBytesRead = 0;
        	return 0;
        }
        if (*msgSize+1 > *maxInMsgSize){ // input buffer is too small
    	   newBuffer = (char *) realloc (*recvline, *msgSize + 1);
    	   if (newBuffer == NULL) return -2; // client should fail
    	   *recvline = newBuffer;
           *maxInMsgSize = *msgSize;
        }
    }
    if ((MSGLENFIELDWIDTH + 1 + *msgSize)> *numBytesRead){
    	 // need to read more bytes - if cotrol passed through the first IF statement above, can not call
    	 // recv as the process will get get stuck at it. We can call recv only if the control reached here without going
    	 // through the first IF body.
    	if(justReadSize){
    		return 0; // need more bytes to proceed
    	} else {
    		// read more bytes - bytes must be present to read on socket
    		n = recv(cinfo->sock, &((*recvline)[*numBytesRead]), *maxInMsgSize - *numBytesRead, 0);
    		if (n < 1) return -2; // client is bad
    		*numBytesRead = *numBytesRead + n;
    	}
    }
    if ((MSGLENFIELDWIDTH + 1 + *msgSize)> *numBytesRead) return 0; // need more bytes
    // complete message read
    (*recvline)[*numBytesRead] = '\0';
    printf ("Message received on Socket %d is : %s \n",cinfo->sock,  *recvline);
    if (cinfo->client == NULL){ // do not know what protocol client is going to use
    	ret = cinfo->clientSet->processFirstMessage (cinfo, &((*recvline)[MSGLENFIELDWIDTH + 1]), sendline, outBuffSize); // First message could be Prototcol or Exit
    } else {
    	ret = cinfo->client->processCommand (&((*recvline)[MSGLENFIELDWIDTH+1]), sendline, outBuffSize, cinfo);
    }
    *msgSize = 0;
    *numBytesRead = 0;
	return ret;
}

void *
ClientSet::thrdMain (void *arg)
{
	clientInfo *cinfo = (clientInfo *) arg;
	struct timeval tv;
	int result;
	fd_set rfds;
	int maxInMsgSize = 1024;
	char *recvline = NULL;
	char *sendline = NULL;
	int numBytesRead = 0;
	int msgSize = 0;
	int ret;
	int n;

	sprintf (cinfo->logLine, "thread starting\n");
	common::LogMsg(1, cinfo->logLine, false);

	signal( SIGPIPE, SIG_IGN );
	// make thread cancellable
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	// The following call is to make THREAD CANCEL work
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
	recvline = (char *)malloc (maxInMsgSize);
	sendline = (char *)malloc (OutBufferSize);
	if (recvline == NULL || sendline == NULL){
		if (recvline) free(recvline);
		if (sendline) free(sendline);
		sprintf (cinfo->logLine, "Error: Failed to allocate memory for input and output buffers.\n");
		common::LogMsg (1, cinfo->logLine, false);
		cinfo->state = CLNT_ENDED;
		cinfo->slotFree = true;
		pthread_exit (NULL);
		return NULL;
	}

	while (!TerminateNow){
       FD_ZERO(&rfds);
	   FD_SET(cinfo->sock, &rfds);
	   tv.tv_sec = 4; // 4 seconds
	   tv.tv_usec = 0;
	   // printf ("My thread id is %ld. My socket is %d \n", cinfo->thrdId, cinfo->sock);
	   result = select(cinfo->sock+1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
	   if(result > 0){
		   // read socket; the first message must be protocol type
		   ret = cinfo->clientSet->ProcessMessage (cinfo, &recvline, &maxInMsgSize, sendline,
				   OutBufferSize, &msgSize, &numBytesRead);
		   if (ret == -1){ // 'terminate' command received: terminate this server
			   TerminateNow = true;
		   } else if (ret == -2){// 'disconnect' command received: disconnect this client, server keeps running
			   break;
		   }
	   } else if (result < 0){          // (result == 0) -- timeout: do nothing, just repeat loop
		  // error in select call
		  break;
	   }
	}
	if (cinfo->dataStore != NULL){
		// if data store is locked, free it
		cinfo->dataStore->unlockDS((struct ClientInfo *)cinfo);
	}


	// remove this client's ownership of all Op Data Stores
	OpDataStores->removeOwner (cinfo);
	if (cinfo->sock != INVALID_SOCK){
		close (cinfo->sock);
		cinfo->sock = INVALID_SOCK;
	}
	free (recvline);
	free (sendline);
	cinfo->clientSet->deleteClient(cinfo->index);
	pthread_exit (NULL);
	return NULL;
}

xmlChar *
ClientSet::GetFirstNodeValue (xmlDocPtr doc, xmlChar *xpath)
{
	char log[200];
	xmlXPathObjectPtr result;
	xmlNodeSetPtr nodes;
	xmlChar *retValue;

	if (doc == NULL){
		return NULL; // error
	}
	result = ClientSet::GetNodeSet(doc, xpath, log);
	if (result == NULL){
		return NULL; // error
	}
	nodes = result->nodesetval;
	if (nodes->nodeNr > 0){
		retValue = xmlNodeListGetString(doc, nodes->nodeTab[0]->xmlChildrenNode,1);

	} else {
		retValue = NULL;
	}
	xmlXPathFreeObject (result);
	return retValue;
}

bool
ClientSet::isDataStoreInUse (DataStore *ds)
{
	int i;
	bool retValue = false;
	pthread_mutex_lock(&csMutex);
	if (clients == NULL || numClients == 0) return false;
	for (i = 0; i < numClients; i++){
		if (clients[i].slotFree == false && clients[i].dataStore == ds){
			retValue = true;
		}
	}
	pthread_mutex_unlock(&csMutex);
	return retValue;
}

void
ClientSet::saveClientBackConnectionInfo (struct clientInfo *cinfo, char *IPAddress, int port)
{
   cinfo->clientPort = port;
   strcpy(cinfo->clientIPAddress, IPAddress);
   cinfo->clientBackSock = -1;
}

void
ClientSet::saveClientBackSignalInfo (struct clientInfo *cinfo, pid_t clientPID, int signalType)
{
	cinfo->clientPID = clientPID;
	cinfo->signalType = signalType;
}

bool
ClientSet::openBackConnection (struct clientInfo *cinfo)
{
	int ret;
	struct sockaddr_in servaddr;

	if (cinfo->clientBackSock > -1) return true; // socket is already open
	if (cinfo->clientPort < 0){
		printf ("Client port is not known.\n");
		return false;
	}
	if (strlen (cinfo->clientIPAddress) < 4){
		printf ("Client IP Address is not known.\n");
		return false;
	}
	cinfo->clientBackSock = socket(AF_INET,SOCK_STREAM,0);

	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(cinfo->clientIPAddress);
	servaddr.sin_port=htons(cinfo->clientPort);

	ret = connect(cinfo->clientBackSock, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (ret != 0) {
		cinfo->clientBackSock = -1;
		printf ("Error in connecting to client at %s and port %d\n", cinfo->clientIPAddress, cinfo->clientPort);
	    return false;
	}
	return true;
}

void
ClientSet::closeBackConnection (struct clientInfo *cinfo)
{
    if (cinfo->clientBackSock != -1) {
    	close (cinfo->clientBackSock);
    	cinfo->clientBackSock = -1;
    }
}

void
ClientSet::signalClients (DataStore *ds)
{
	int i;
	pthread_mutex_lock(&csMutex);
	if (clients == NULL || numClients == 0) return;
	for (i = 0; i < numClients; i++){
		if (clients[i].slotFree == false && clients[i].dataStore == ds
				&& clients[i].clientPID >= 0 && clients[i].signalType >=0){
			kill (clients[i].clientPID, clients[i].signalType);
		}
	}
	pthread_mutex_unlock(&csMutex);
}
