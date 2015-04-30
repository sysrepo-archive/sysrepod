/*
 * srd.cpp
 *
 * License : Apache 2.0
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "srd.h"

#define MSGLENFIELDWIDTH 7
int  srd_isServerResponseOK (int sockfd, char **OKcontent);

bool
srd_sendServer (int sockfd, char *message, int msgSize)
{
   char fmt[20];
   char msgSizeStr[MSGLENFIELDWIDTH + 10];
   int sent = 0;
   int toSend;
   int n;

   sprintf (fmt, "%%.%dd ", MSGLENFIELDWIDTH);
   sprintf (msgSizeStr, fmt, msgSize);
   toSend = MSGLENFIELDWIDTH + 1;
   printf ("libsrd.a: About to send the following message to server >>>>>>>\n%s%s\n", msgSizeStr, message);
   // send size str
   while (sent < MSGLENFIELDWIDTH+1){
       n = send (sockfd, &(msgSizeStr[sent]), toSend, 0);
       if (n < 1) return false;
       sent = sent + n;
       toSend = toSend - n;
   }
   // send payload
   sent = 0;
   toSend = msgSize;
   while (sent < msgSize){
      n = send (sockfd, &(message[sent]), toSend, 0);
      if (n < 1) return false;
      sent = sent + n;
      toSend = toSend - n;
   }
   return true;
}

int
srd_recvServer (int sockfd, char **buffPtr, int *buffSize)
{
	int len = 0;
	int waitCount = 0;
	int msgSize = 0;
	int numBytesRead = 0;
	int n;
	char *newBuffer;

	// First few chars in all messages reserved for message size followed by one white space.
	while (msgSize == 0 && numBytesRead < MSGLENFIELDWIDTH + 1){
	    n= recv(sockfd, &((*buffPtr)[numBytesRead]), *buffSize - numBytesRead, 0);
	    if (n < 1) return 0; // client is bad
	    numBytesRead = numBytesRead + n;
	    if (numBytesRead < MSGLENFIELDWIDTH + 1) continue; // need more bytes to proceed
	    // read message size
	    n = sscanf(*buffPtr, "%d", &msgSize);
	    if (n != 1){ // msg len not found
	        printf ("libsrd.a: Unable to find message length of the response from server.\n");
	        return 0;
	    }
	    if (MSGLENFIELDWIDTH + 1 + msgSize + 1 > *buffSize){ // input buffer is too small
	    	newBuffer = (char *) realloc (*buffPtr, MSGLENFIELDWIDTH + 1 + msgSize + 1);
	    	if (newBuffer == NULL){
	    		printf ("libsrd.a: Unable to allocate space.\n");
	    		return 0; // client should fail
	    	}
	    	*buffPtr = newBuffer;
	        *buffSize = MSGLENFIELDWIDTH + 1 + msgSize + 1;
	    }
	}
	while ((MSGLENFIELDWIDTH + 1 + msgSize) > numBytesRead){
	    // need to read more bytes
	    n = recv(sockfd, &((*buffPtr)[numBytesRead]), *buffSize - numBytesRead, 0);
	    if (n < 1) return 0; // client is bad
	    numBytesRead = numBytesRead + n;
	}
	// complete message read
	(*buffPtr)[numBytesRead] = '\0';
	return msgSize;
}

xmlXPathObjectPtr
srd_getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	if(!doc) return NULL;
	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		sprintf(log, "Error in xmlXPathNewContext");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		sprintf(log, "Error in xmlXPathEvalExpression");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
        sprintf(log, "No result");
		return NULL;
	}
	return result;
}

xmlChar *
srd_getFirstNodeValue (xmlDocPtr doc, xmlChar *xpath)
{
	char log[200];
	xmlXPathObjectPtr result;
	xmlNodeSetPtr nodes;
	xmlChar *retValue;

	if (doc == NULL){
		return NULL; // error
	}
	result = srd_getNodeSet(doc, xpath, log);
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

int
srd_printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize)
{
    xmlNode *cur_node = NULL;
    int n, i;
    xmlChar *value;
    int offset = 0;
    xmlBuffer *buff;
    int size = printBuffSize;
    char *newSpace;

    for (i=0; i < nodeSet->nodeNr; i++) {
    	cur_node = nodeSet->nodeTab[i];
        if (cur_node->type == XML_ELEMENT_NODE) {
           buff = xmlBufferCreate ();
           xmlNodeDump (buff, doc,cur_node, 0, 1 );
           if (size < (offset + strlen((char *)buff->content) + 1)){
        	   size = offset + strlen((char *)buff->content) + 1;
        	   newSpace = (char *)realloc (*printBuffPtr, size);
        	   if (newSpace){
        		   *printBuffPtr = newSpace;
        	   } else {
        		   // unable to allocate space
        		   xmlBufferFree (buff);
        		   return -1;
        	   }
           }
           n = sprintf (*printBuffPtr+offset, "%s", buff->content);
           offset = offset + n;
           xmlBufferFree (buff);
        }
    }
    return (offset);

}

int
printXPathAtomic (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize)
{
	int retValue;
	char *res = NULL;
	int size = printBuffSize;
	char *newSpace;
	int n;

	if (objset == NULL) return -1;
	switch (objset->type){
	case XPATH_STRING:
        res = strdup ((char *)objset->stringval);
        break;
    case XPATH_BOOLEAN:
        res = (char *)xmlXPathCastBooleanToString(objset->boolval);
        break;
    case XPATH_NUMBER:
        res = (char *) xmlXPathCastNumberToString(objset->floatval);
        break;
	}
	if (res == NULL || strlen (res) == 0){
		if (res) free (res);
		return 0;
	}
	if (size < (strlen(res) + 1)){
	    size = strlen(res) + 1;
	    newSpace = (char *)realloc (*printBuffPtr, size);
	    if (newSpace){
	        *printBuffPtr = newSpace;
	    } else {
	        // unable to allocate space
	        if (res) free (res);
	        return -1;
	    }
	}
	n = sprintf (*printBuffPtr, "%s", res);
	if (res) free (res);
	return n;
}

int
printXPathAtomicResult (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize)
{
	int retValue;

	if (!objset){
	    // no value
		retValue = 0;
        strcpy (*printBuffPtr, "");
	} else if(objset->type == XPATH_STRING || objset->type == XPATH_NUMBER || objset->type == XPATH_BOOLEAN){
		// put the text form of XPATH result in *printBuffPtr
		retValue = printXPathAtomic (objset, printBuffPtr, printBuffSize);
		if (retValue < 0){
			// error
			sprintf (*printBuffPtr, "<xml><error>Unable to print XPath result</error></xml>");
		}
    } else if (objset->type == XPATH_UNDEFINED){
    	retValue = sprintf (*printBuffPtr, "XPATH STRING: undefined");
    }
	return retValue;
}

// END local functions

int
srd_connect (char *serverIP, int serverPort, int *sockfd)
{
   int ret;
   struct sockaddr_in servaddr;
   char setProtoMsg [] = "<xml><protocol>srd</protocol></xml>";

   *sockfd=socket(AF_INET,SOCK_STREAM,0);

   bzero(&servaddr,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=inet_addr(serverIP);
   servaddr.sin_port=htons(serverPort);

   ret = connect(*sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
   if (ret != 0) {
	   *sockfd = -1;
	   printf ("libsrd.a: Error in connecting to server at %s and port %d\n", serverIP, serverPort);
       return 0;
   }
   // send protocol info to server
   if (!srd_sendServer (*sockfd, setProtoMsg, strlen(setProtoMsg))){
      printf ("libsrd.a: Error in sending mes#include <libxml/xpathInternals.h>sage to set Protocol to server.\n");
      close (*sockfd);
      *sockfd = -1;
      return 0;
   }
   if (!srd_isServerResponseOK (*sockfd, NULL)){
	   printf ("libsrd.a: Server response to Protocol command is not OK.\n");
	   close (*sockfd);
	   *sockfd = -1;
	   return 0;
   }
   xmlInitParser();
   return 1;
}

int
srd_setDataStore (int sockfd, char *dsname)
{
   char *msg = NULL;

   if (dsname && strlen(dsname) > 0){
	  msg = (char *)malloc (strlen(dsname) + 100);
	  if (!msg){
		  printf ("libsrd.a: Unable to allocate space.\n");
		  return 0;
	  }
      sprintf (msg, "<xml><command>set_dataStore</command><param1>%s</param1></xml>", dsname);
   } else {
	   msg = (char *)malloc (100);
	   if (!msg){
		   printf ("libsrd.a: Unable to allocate space.\n");
		   return 0;
	   }
	   sprintf (msg, "<xml><command>set_dataStore</command><param1></param1></xml>");
   }
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
      free (msg);
      return 0;
   }
   if (!srd_isServerResponseOK (sockfd, NULL)){
	       printf ("libsrd.a: Server response to set data store is not OK.\n");
           free (msg);
	   return 0;
   }
   free (msg);
   return 1;
}

void
srd_applyXSLT (int sockfd, char *xsltText, char **buffPtr)
{
   char *msg;

   if (buffPtr == NULL) {
	   printf ("libsrd.a: Buffer Pointer is null. No place to return results.\n");
	   return;
   }
   if (xsltText == NULL){
	   printf ("libsrd.a: XSLT pointer can not be NULL.\n");
	   return;
   }
   if (strlen (xsltText) < 1){
	   printf ("linsrd.a: XSLT content is missing.\n");
	   return;
   }
   // Once can not specify XSLT without CDATA clause
   if(strstr(xsltText, "CDATA") == NULL) {
	   printf ("libsrd.a: XSLT can not be specifed without CDATA clause with an XML message.\n");
	   return;
   }
   msg = (char *)malloc (strlen(xsltText) + 100);
   if (!msg){
	   printf ("libsrd.a: Unable to allocate space.\n");
	   return;
   }
   sprintf (msg, "<xml><command>apply_xslt</command><param1>%s</param1></xml>", xsltText);
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
   }
   if (!srd_isServerResponseOK (sockfd, buffPtr)){
	       printf ("libsrd.a: Server response to apply XPATH is not OK.\n");
   }
   free (msg);
}

void
srd_applyXPath (int sockfd, char *xpath, char **buffPtr)
{
   char *msg;

   if (buffPtr == NULL) {
	   printf ("libsrd.a: Buffer Pointer is null. No place to return results.\n");
	   return;
   }
   if (xpath == NULL){
	   printf ("libsrd.a: XPath pointer can not be NULL.\n");
	   return;
   }
   if (strlen (xpath) < 1){
	   printf ("linsrd.a: XPath content is missing.\n");
	   return;
   }
   msg = (char *)malloc (strlen(xpath) + 100);
   if (!msg){
	   printf ("libsrd.a: Unable to allocate space.\n");
	   return;
   }
   sprintf (msg, "<xml><command>apply_xpath</command><param1>%s</param1></xml>", xpath);
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
   }
   if (!srd_isServerResponseOK (sockfd, buffPtr)){
	       printf ("libsrd.a: Server response to apply XPATH is not OK.\n");
   }
   free (msg);
}

// Instructs this client to send termination command to the server.
void
srd_terminateServer (int sockfd)
{
	char msg[50];
	sprintf (msg, "<xml><command>terminate</command></xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	   printf ("libsrd.a: Error in sending msg: %s\n", msg);
    }
	if (!srd_isServerResponseOK (sockfd, NULL)){
	   printf ("libsrd.a: Server response to terminate is not OK.\n");
	}
	xmlCleanupParser();
    close (sockfd);
}

// Instructs this client to disconnect from the server.
void
srd_disconnect (int sockfd)
{
	char msg[50];
	sprintf (msg, "<xml><command>disconnect</command></xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	   printf ("libsrd.a: Error in sending msg: %s\n", msg);
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
		   printf ("libsrd.a: Server response to disconnect is not OK.\n");
	}
	xmlCleanupParser();
	close (sockfd);
}

int
srd_lockDataStore (int sockfd)
{
	char msg[50];
    sprintf (msg, "<xml><command>lock_dataStore</command></xml>");
    if (!srd_sendServer (sockfd, msg, strlen(msg))){
		printf ("libsrd.a: Error in sending msg: %s\n", msg);
		return 0;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
	    printf ("libsrd.a: Server response to lock data store is not OK.\n");
	    return 0;
	}
	return 1;
}

int
srd_unlockDataStore (int sockfd)
{
	char msg[50];
    sprintf (msg, "<xml><command>unlock_dataStore</command></xml>");
    if (!srd_sendServer (sockfd, msg, strlen(msg))){
		printf ("libsrd.a: Error in sending msg: %s\n", msg);
		return 0;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
	    printf ("libsrd.a: Server response to unlock data store is not OK.\n");
	    return 0;
	}
	return 1;
}

int
srd_updateNodes (int sockfd, char *xpath, char *value)
{
	char *msg;
	char *result;
	int n = -1;
	int intValue;

	msg = (char *)malloc (strlen(xpath) + strlen (value) + 100);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return -1;
	}
	sprintf (msg, "<xml><command>update_nodes</command><param1>%s</param1><param2>%s</param2></xml>", xpath, value);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return -1;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response to apple XPath is not OK.\n");
		free (msg);
		return -1;
	}
	if (result) {
		   n = sscanf (result, "%d", &intValue);
		   if (n == 0) {
			   n = -1; // error condition.
		   } else {
			   n = intValue;
		   }
		   free (result);
	} else {
        printf ("libsrd.a: Unable to read how many nodes got modified. Unpredictable modificatons done in data store.\n");
	}
	free (msg);
	return n;
}

int
srd_isServerResponseOK(int sockfd, char **OKcontent)
{
	int ret;
	char log[100];
	char *buffPtr = NULL;
	int buffSize = 100;
	xmlDocPtr doc;
	xmlXPathObjectPtr  xpathObj;
	xmlChar xpathExpr[100];
	xmlNodeSetPtr nodes;

	if (OKcontent != NULL) *OKcontent = NULL;
	buffPtr = (char *) malloc (buffSize);
	if (!buffPtr){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	ret = srd_recvServer (sockfd, &buffPtr, &buffSize);
	if (!ret){
		printf ("libsrd.a: Call to read response from server returned 0 - failure\n");
		if(buffPtr) free (buffPtr);
		return 0;
	}
    printf("libsrd.a: Response from server is: <<<<<<\n %s\n", buffPtr);
	doc = xmlReadMemory(&(buffPtr[MSGLENFIELDWIDTH + 1]), ret, "noname.xml", NULL, 0);
	if (buffPtr){
		free (buffPtr);
		buffPtr = NULL;
	}
	if (doc == NULL){
            printf("libsrd.a: Error in forming xml-doc out of server response : doc is NULL\n");
	    return 0;
	}
	strcpy ((char *)xpathExpr, "/xml/ok");
	xpathObj = srd_getNodeSet (doc, (xmlChar *)xpathExpr, log);
    if(xpathObj == NULL) {
	   // Error: unable to evaluate xpath expression OR OK node does not exit
	   xmlFreeDoc(doc);
	   if (strcmp (log, "No result") == 0){
		   printf ("libsrd.a: Response from server is not OK.\n");
	   } else {
           printf("libsrd.a: Unable to parse server response: xpath failed\n");
	   }
	   return(0);
	}
    nodes = xpathObj->nodesetval;
    if(nodes->nodeNr > 0){
    	if (OKcontent != NULL){
    		char *contentBuff;
    		int   contentBuffSize = 100;
    		int len = 0;
    		contentBuff = (char *)malloc (contentBuffSize);
    		if (!contentBuff){
    			printf ("libsrd.a: Unable to allocate buffer to read content of <ok> node present in server response.\n");
    			ret = 0;
    		} else {
    			xmlXPathObjectPtr xpathObj_local;
    			xmlNodeSetPtr nodes_local;

    			// get the value of OK node from server-response
    			strcpy ((char *)xpathExpr, "/xml/ok/*");
    			xpathObj_local = srd_getNodeSet (doc, (xmlChar *) xpathExpr, log);
    			if (xpathObj_local != NULL){
    				nodes_local = xpathObj_local->nodesetval;
    				if (nodes_local->nodeNr > 0){
    					len = srd_printElementSet (doc, nodes_local, &contentBuff, contentBuffSize);
    					if (len < 0){
    					    printf ("libsrd.a: Unable to read content of <ok> node in server response.\n");
    					    ret = 0;
    					} else {
    					    // caller needs to free this space
    						*OKcontent = contentBuff;
    					    ret = 1;
    					}
    				} else {
    					printf ("libsrd.a: <ok> node in server response does not have any content.\n");
    					ret = 1;
    				}
    				xmlXPathFreeObject (xpathObj_local);
    			} else { // <ok> node may contain value: extract that
    				char *okValue;
    				strcpy ((char *)xpathExpr, "/xml/ok");
    				okValue = (char *)srd_getFirstNodeValue(doc, xpathExpr);
    				if(okValue != NULL){
    				   *OKcontent = okValue; // caller need to free it.
    				}
    				ret = 1;
    			}
    	    }
    	} else {
    		ret = 1;
    	}
    }else{
        // error result from server
    	printf ("libsrd.a: No xml nodes found in the response from the server.\n");
    	ret = 0;
    }
    xmlXPathFreeObject (xpathObj);
    xmlFreeDoc(doc);
    return ret;
}

int  srd_createDataStore (int sockfd, char *name, char *value, char *xsdDir, char *xsltDir)
{
	char *msg;
	int   msgSpace;

	if (name == NULL || strlen (name) == 0 || value == NULL || strlen(value) == 0){
		printf ("libsrd.a: Name and/or Value of data store can not be absent.");
		return (0);
	}

	msgSpace = strlen(name) + strlen (value) + 100;
	if (xsdDir)  msgSpace = msgSpace + strlen (xsdDir);
	if (xsltDir) msgSpace = msgSpace + strlen (xsltDir);
	msg = (char *)malloc (msgSpace);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	sprintf (msg, "<xml><command>create_dataStore</command><param1>%s</param1><param2>%s</param2>", name, value);
	if (xsdDir){
		strcat (msg, "<param3>");
		strcat (msg, xsdDir);
		strcat (msg, "</param3>");
	}
	if (xsdDir){
		strcat (msg, "<param4>");
		strcat (msg, xsltDir);
		strcat (msg, "</param4>");
	}
	strcat (msg, "</xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
		printf ("libsrd.a: Server response is not OK.\n");
		free (msg);
		return 0;
	}
	free (msg);
	return 1;
}

bool
srd_listDataStores (int sockfd, char **result)
{
	char msg[100];

	sprintf (msg, "<xml><command>list_dataStores</command></xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    return false;
	}
	if (!srd_isServerResponseOK (sockfd, result)){
		printf ("libsrd.a: Server response is not OK.\n");
		return false;
	}
	return true;
}

int
srd_deleteDataStore (int sockfd, char *name)
{
	char *msg;
	char *result;
	int n;
	int intValue;
	bool retValue;

	if (name == NULL || strlen (name) == 0){
		printf ("libsrd.a: Name of Data Store can not be absent.\n");
		return 0;
	}
	msg = (char *)malloc (strlen(name) + 100);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	sprintf (msg, "<xml><command>delete_dataStore</command><param1>%s</param1></xml>", name);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response is not OK.\n");
		free (msg);
		return 0;
	}
	if (result) {
		   n = sscanf (result, "%d", &intValue);
		   if (n != 0) {
			   n = 1;
		   }
		   free (result);
	} else {
        printf ("libsrd.a: Unable to read how many data stores deleted. Unpredictable modificatons done in data store.\n");
        n = 1;
	}
	free (msg);
	return n;
}

int
srd_copyDataStore (int sockfd, char *fromName, char *toName)
{
	char *msg;
	char *result;
	int n;
	int intValue;
	bool retValue;

	if (fromName == NULL || strlen (fromName) == 0){
		printf ("libsrd.a: Name of FROM Data Store can not be absent.\n");
		return 0;
	}
	if (toName == NULL || strlen (toName) == 0){
		printf ("libsrd.a: Name of TO Data Store can not be absent.\n");
		return 0;
	}
	msg = (char *)malloc (strlen(fromName) + strlen (toName) + 100);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	sprintf (msg, "<xml><command>copy_dataStore</command><param1>%s</param1><param2>%s</param2></xml>", fromName, toName);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response is not OK.\n");
		free (msg);
		return 0;
	}
	if (result) {
		   n = sscanf (result, "%d", &intValue);
		   if (n != 0) {
			   n = 1;
		   }
		   free (result);
	} else {
        printf ("libsrd.a: Unable to read result. Unpredictable modificatons done in data store.\n");
        n = 1;
	}
	free (msg);
	return n;
}


int
srd_createOpDataStore (int sockfd, char *name)
{
	char *msg;
	int   msgSpace;

	if (name == NULL || strlen (name) == 0){
		printf ("libsrd.a: Name of Operational Data Store can not be absent.\n");
		return (0);
	}

	msgSpace = strlen(name) + 100;
	msg = (char *)malloc (msgSpace);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	sprintf (msg, "<xml><command>create_opDataStore</command><param1>%s</param1></xml>", name);

	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
		printf ("libsrd.a: Server response is not OK.\n");
		free (msg);
		return 0;
	}
	free (msg);
	return 1;

}
int
srd_deleteOpDataStore (int sockfd, char *name)
{
	char *msg;
	char *result;
	int n;
	int intValue;
	bool retValue;

	if (name == NULL || strlen (name) == 0){
		printf ("libsrd.a: Name of Operational Data Store can not be absent.\n");
		return 0;
	}

	msg = (char *)malloc (strlen(name) + 100);
	if (!msg){
		printf ("libsrd.a: Unable to allocate space.\n");
		return 0;
	}
	sprintf (msg, "<xml><command>delete_opDataStore</command><param1>%s</param1></xml>", name);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response is not OK.\n");
		free (msg);
		return 0;
	}
	if (result) {
		   n = sscanf (result, "%d", &intValue);
		   if (n != 0) {
			   n = 1;
		   }
		   free (result);
	} else {
        printf ("libsrd.a: Unable to read how many Operational Data Stores deleted. Unpredictable modificatons done in data store.\n");
        n = 1;
	}
	free (msg);
	return n;

}
bool
srd_listOpDataStores (int sockfd, char **result)
{
	char msg[100];

	sprintf (msg, "<xml><command>list_opDataStores</command></xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    return false;
	}
	if (!srd_isServerResponseOK (sockfd, result)){
		printf ("libsrd.a: Server response is not OK.\n");
		return false;
	}
	return true;

}
bool
srd_listMyUsageOpDataStores (int sockfd, char **result)
{
	char msg[100];

	sprintf (msg, "<xml><command>list_myUsageOpDataStores</command></xml>");
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    return false;
	}
	if (!srd_isServerResponseOK (sockfd, result)){
		printf ("libsrd.a: Server response is not OK.\n");
		return false;
	}
	return true;
}

int
srd_useOpDataStore (int sockfd, char *name)
{
   char *msg;

   if (name && strlen(name) > 0){
	  msg = (char *)malloc (strlen(name) + 100);
	  sprintf (msg, "<xml><command>use_opDataStore</command><param1>%s</param1></xml>", name);
   } else {
	  printf ("Error: Operational Data Store name can not be absent.\n");
	  return 0;
   }
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
	  free (msg);
	  return 0;
   }
   if (!srd_isServerResponseOK (sockfd, NULL)){
      printf ("libsrd.a: Server response to set data store is not OK.\n");
	  free (msg);
	  return 0;
   }
   free (msg);
   return 1;
}

int
srd_stopUsingOpDataStore (int sockfd, char *name)
{
   char *msg;

   if (name && strlen(name) > 0){
      msg = (char *)malloc (strlen(name) + 100);
	  sprintf (msg, "<xml><command>stopUsing_opDataStore</command><param1>%s</param1></xml>", name);
   } else {
	  printf ("Error: Operational Data Store name can not be absent.\n");
	  return 0;
   }
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
	  printf ("libsrd.a: Error in sending msg: %s\n", msg);
	  free (msg);
	  return 0;
   }
   if (!srd_isServerResponseOK (sockfd, NULL)){
	  printf ("libsrd.a: Server response to set data store is not OK.\n");
	  free (msg);
	  return 0;
   }
   free (msg);
   return 1;
}

void
srd_applyXPathOpDataStore (int sockfd, char *opDataStoreName, char *xpath, char **buffPtr)
{
   char *msg;

   if (buffPtr == NULL) {
	  printf ("libsrd.a: Buffer Pointer is null. No place to return results.\n");
	  return;
   }
   if (xpath == NULL){
   	  printf ("libsrd.a: XPath pointer can not be NULL.\n");
   	  return;
   }
   if (strlen (xpath) < 1){
   	  printf ("linsrd.a: XPath content is missing.\n");
   	  return;
   }
   if (opDataStoreName == NULL){
	   printf ("libsrd.a: Operational Data Store name is missing.\n");
	   return;
   }
   if (strlen (opDataStoreName) < 1){
	   printf ("libsrd.a: Operational Data Store name is not present.\n");
	   return;
   }
   msg = (char *)malloc (strlen(xpath) + strlen (opDataStoreName) + 100);
   if (!msg){
	   printf ("libsrd.a: Unable to allocate space.\n");
	   return;
   }
   sprintf (msg, "<xml><command>apply_xpathOpDataStore</command><param1>%s</param1><param2>%s</param2></xml>", opDataStoreName, xpath);
   if (!srd_sendServer (sockfd, msg, strlen(msg))){
	   printf ("libsrd.a: Error in sending msg: %s\n", msg);
   }
   if (!srd_isServerResponseOK (sockfd, buffPtr)){
	   printf ("libsrd.a: Server response to apply XPATH on Operational Data Store is not OK.\n");
   }
   free (msg);
}

bool
srd_registerClientSocket (int sockfd, char *myIPAddress, int myPort)
{
	char  msg[150];

	if (myIPAddress == NULL || strlen (myIPAddress) == 0 || strlen(myIPAddress) > 20 || myPort <= 0){
		printf ("libsrd.a: IP Address and/or Port Number can not be absent or are wrong.");
		return false;
	}
	sprintf (msg, "<xml><command>register_clientSocket</command><param1>%s</param1><param2>%d</param2></xml>", myIPAddress, myPort);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    return false;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
		printf ("libsrd.a: Server response is not OK.\n");
		return false;
	}
	return true;
}

bool
srd_registerClientSignal (int sockfd, pid_t clientPID, int signalType)
{
	char  msg[150];

	sprintf (msg, "<xml><command>register_clientSignal</command><param1>%d</param1><param2>%d</param2></xml>",
			clientPID, signalType);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    return false;
	}
	if (!srd_isServerResponseOK (sockfd, NULL)){
		printf ("libsrd.a: Server response is not OK.\n");
		return false;
	}
	return true;
}

int
srd_deleteNodes (int sockfd, char *xpath)
{
	char *msg = NULL;
	char *result = NULL;
	int  retValue = -1;
	int  n, intValue;

	if (xpath == NULL){
	   	  printf ("libsrd.a: XPath can not be NULL.\n");
	   	  return -1;
	}
	if (strlen (xpath) < 1){
	   	  printf ("linsrd.a: XPath content is missing.\n");
	   	  return -1;
	}
	msg = (char *)malloc (strlen(xpath) + 100);
	if (!msg){
		   printf ("libsrd.a: Unable to allocate space.\n");
		   return -1;
	}
	sprintf (msg, "<xml><command>delete_nodesDataStore</command><param1>%s</param1></xml>", xpath);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
		   printf ("libsrd.a: Error in sending msg: %s\n", msg);
		   free (msg);
		   return -1;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		   printf ("libsrd.a: Server response to apply XPATH on Operational Data Store is not OK.\n");
	} else {
		   if (result) {
		   		n = sscanf (result, "%d", &intValue);
		   		if (n == 0) {
		   			   retValue = 0;
		   			   printf ("libsrd.a: Unable to read how many nodes were changed. Unpredictable modificatons done in data store.\n");
		   		} else {
		   			   retValue = intValue;
		   		}
		   	} else {
		   		retValue = 0;
		        printf ("libsrd.a: Unable to read how many nodes were changed. Unpredictable modificatons done in data store.\n");
		   	}
	}
	if (result) free (result);
	free (msg);
	return retValue;
}

int
srd_addNodes (int sockfd, char *xpath, char *value)
{
	char *msg = NULL;
	char *result = NULL;
	int  retValue = -1;
	int  n, intValue;

	if (value == NULL || strlen (value) < 1) {
		  printf ("libsrd.a: No new value provided for the new nodes to be added.\n");
		  return -1;
	}
	if (xpath == NULL){
	   	  printf ("libsrd.a: XPath can not be NULL.\n");
	   	  return -1;
	}
	if (strlen (xpath) < 1){
	   	  printf ("linsrd.a: XPath content is missing.\n");
	   	  return -1;
	}
	msg = (char *)malloc (strlen(xpath) + strlen (value) + 100);
	if (!msg){
		   printf ("libsrd.a: Unable to allocate space.\n");
		   return -1;
	}
	sprintf (msg, "<xml><command>add_nodesDataStore</command><param1>%s</param1><param2>%s</param2></xml>", xpath, value);
	if (!srd_sendServer (sockfd, msg, strlen(msg))){
		   printf ("libsrd.a: Error in sending msg: %s\n", msg);
		   free (msg);
		   return -1;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		   printf ("libsrd.a: Server response to apply XPATH on Operational Data Store is not OK.\n");
	} else {
		   if (result) {
		   		n = sscanf (result, "%d", &intValue);
		   		if (n == 0) {
		   			   retValue = 0;
		   			   printf ("libsrd.a: Unable to read how many nodes were changed. Unpredictable modificatons done in data store.\n");
		   		} else {
		   			   retValue = intValue;
		   		}
		   	} else {
		   		retValue = 0;
		        printf ("libsrd.a: Unable to read how many nodes were changed. Unpredictable modificatons done in data store.\n");
		   	}
	}
	if (result) free (result);
	free (msg);
	return retValue;
}

void
srd_DOMHandleXPath (int sockfd, xmlDocPtr ds, xmlChar *xpathExpr)
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
		if (xpathObj_local->type == XPATH_NODESET || xpathObj_local->type == XPATH_XSLT_TREE){
	        nodes_local = xpathObj_local->nodesetval;
	        if (nodes_local->nodeNr > 0){
	    	    len = srd_printElementSet (ds, nodes_local, &contentBuff, contentBuffSize);
	        }
		}else {
			len = printXPathAtomicResult(xpathObj_local, &contentBuff, contentBuffSize);
		}
	    if (len < 0){
	    		printf ("Unable to read XPath result.\n");
	    		sprintf (sendline, "<xml><error>Unable to read XPath result</error></xml>");
	    		srd_sendServer(sockfd, sendline, strlen(sendline));
	    } else if (len == 0){
	    		int n;
	    	    n = sprintf (contentBuff, "<xml><ok/></xml>");
	    		srd_sendServer (sockfd, contentBuff, n);
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
	    xmlXPathFreeObject (xpathObj_local);
   } else {
	   int n;
	   n =sprintf (contentBuff, "<xml><ok/></xml>");
	   srd_sendServer (sockfd, contentBuff, n);
   }
   free (contentBuff);
}
