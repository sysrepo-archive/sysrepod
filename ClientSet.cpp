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
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "global.h"
#include "ClientSet.h"
#include "common.h"
#include "ClientSRD.h"



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
}

bool
ClientSet::initialize (int maximumClients)
{
	int i;

	maxClients = maximumClients;
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
	}
	return false;
}

int
ClientSet::newClient (int sock)
{
	int i;
	int slotIndex = -1;

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
		return -1;
	}
	clients[slotIndex].sock = sock;
	return slotIndex;
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
	 clients[index].slotFree = false;
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
			sprintf (outBuffer, "<xml><error>This protocol not supported: %s</error>", (char *) proto);
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
    printf ("Message received is : %s \n", *recvline);
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
	if (cinfo->sock != INVALID_SOCK){
		close (cinfo->sock);
		cinfo->sock = INVALID_SOCK;
	}
	free (recvline);
	free (sendline);
	cinfo->state = CLNT_ENDED;
	cinfo->slotFree = true;
	cinfo->dataStore = NULL;
	cinfo->protocol = INVALID_PROTOCOL;
	delete (cinfo->client);
	cinfo->client = NULL;
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
