/*
 * srd.cpp
 *
 *  Created on: Feb 5, 2015
 *      Author: niraj
 */

#include "srd.h"

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

#define MSGLENFIELDWIDTH 7
int  srd_isServerResponseOK (int sockfd, char **OKcontent);

// local functions
int
sendServer (int sockfd, char *message, int msgSize)
{
   char fmt[20];
   char msgSizeStr[MSGLENFIELDWIDTH + 10];
   int sent = 0;
   int toSend;
   int n;

   sprintf (fmt, "%%.%dd ", MSGLENFIELDWIDTH);
   sprintf (msgSizeStr, fmt, msgSize);
   toSend = MSGLENFIELDWIDTH + 1;
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
   return 1;
}

int
recvServer (int sockfd, char **buffPtr, int *buffSize)
{
	int len = 0;
	int waitCount = 0;
	int msgSize = 0;

	len = read (sockfd, *buffPtr, MSGLENFIELDWIDTH + 1);
	(*buffPtr)[MSGLENFIELDWIDTH + 1] = '\0';
    msgSize = atoi (*buffPtr);
    len = read (sockfd, *buffPtr, msgSize);
    return msgSize;


   // read message length first
   ioctl (sockfd, FIONREAD, &len);
   printf ("sock content len is %d, want it to be %d\n", len, MSGLENFIELDWIDTH+1);
   while (len < MSGLENFIELDWIDTH + 1 ){
		sleep (2);
                ioctl (sockfd, FIONREAD, &len);
                printf ("READ size: sock content len is %d, want it to be %d\n", len, MSGLENFIELDWIDTH+1);
		waitCount++;
		if (waitCount == 3) break;
   }
   if (len >= MSGLENFIELDWIDTH + 1){
	  len = read (sockfd, *buffPtr, MSGLENFIELDWIDTH + 1);
	  if (len != MSGLENFIELDWIDTH + 1){ // error
		  return 0;
	  }
	  (*buffPtr)[MSGLENFIELDWIDTH + 1] = '\0';
	  msgSize = atoi (*buffPtr);
	  if (msgSize < 1) return 0; // error
	  if (msgSize > *buffSize){
		  // given buffer is too small. Allocate a bigger one.
		  *buffPtr = (char *)malloc (msgSize + 1);
		  if (*buffPtr == NULL) return 0; // error
		  *buffSize = msgSize + 1;
	  }
	  ioctl (sockfd, FIONREAD, &len);
	  waitCount = 0;
	  while (len < msgSize){
	  	 sleep (2);
                 ioctl (sockfd, FIONREAD, &len);
                 printf ("READ content: sock content len is %d, want it to be %d\n", len, msgSize);
	  	 waitCount++;
	  	 if (waitCount == 3) break;
	  }
	  if (len >= msgSize){
		  len = read (sockfd, *buffPtr, msgSize);
		  if (len < msgSize) return 0; // error
	  } else {
		  return 0; // error
	  }
   } else {
		return 0;
   }
   return msgSize;
}

xmlXPathObjectPtr
getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log)
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
getFirstNodeValue (xmlDocPtr doc, xmlChar *xpath)
{
	char log[200];
	xmlXPathObjectPtr result;
	xmlNodeSetPtr nodes;
	xmlChar *retValue;

	if (doc == NULL){
		return NULL; // error
	}
	result = getNodeSet(doc, xpath, log);
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
printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize)
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
   if (!sendServer (*sockfd, setProtoMsg, strlen(setProtoMsg))){
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
   char *msg;

   msg = (char *)malloc (strlen(dsname) + 100);
   sprintf (msg, "<xml><command>set_datastore</command><param1>%s</param1></xml>", dsname);
   if (!sendServer (sockfd, msg, strlen(msg))){
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
srd_applyXPath (int sockfd, char *xpath, char **buffPtr)
{
   char *msg;

   if (buffPtr == NULL) {
	   printf ("libsrd.a: Buffer Pointer is null. No place to return results.\n");
	   return;
   }
   msg = (char *)malloc (strlen(xpath) + 100);
   if (!msg){
	   printf ("libsrd.a: Unable to allocate space.\n");
	   return;
   }
   sprintf (msg, "<xml><command>apply_xpath</command><param1>%s</param1></xml>", xpath);
   if (!sendServer (sockfd, msg, strlen(msg))){
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
	if (!sendServer (sockfd, msg, strlen(msg))){
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
	if (!sendServer (sockfd, msg, strlen(msg))){
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
    if (!sendServer (sockfd, msg, strlen(msg))){
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
    if (!sendServer (sockfd, msg, strlen(msg))){
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
	if (!sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return -1;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response to apply XPATH is not OK.\n");
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
	char buff[500];
	char log[100];
	char *buffPtr = buff;
	int buffSize = sizeof(buff) - 1;
	xmlDocPtr doc;
	xmlXPathObjectPtr  xpathObj;
	xmlChar xpathExpr[100];
	xmlNodeSetPtr nodes;

	if (OKcontent != NULL) *OKcontent = NULL;
	ret = recvServer (sockfd, &buffPtr, &buffSize);
	if (!ret){
		printf ("libaapi.a: Call to read response from server returned 0 - failure\n");
		if(buffPtr != buff) free (buffPtr);
		return 0;
	}
	buffPtr[ret] = '\0';// make contents a string, not necessary
    printf("libsrd.a: Response from server is:\n %s\n", buffPtr);
	doc = xmlReadMemory(buffPtr, ret, "noname.xml", NULL, 0);
	if (buffPtr != buff) free (buffPtr);
	if (doc == NULL){
            printf("libsrd.a: Error in forming xml-doc out of server response : doc is NULL\n");
	    return 0;
	}
	strcpy ((char *)xpathExpr, "/xml/ok");
	xpathObj = getNodeSet (doc, (xmlChar *)xpathExpr, log);
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
    			xpathObj_local = getNodeSet (doc, (xmlChar *) xpathExpr, log);
    			if (xpathObj_local != NULL){
    				nodes_local = xpathObj_local->nodesetval;
    				if (nodes_local->nodeNr > 0){
    					len = printElementSet (doc, nodes_local, &contentBuff, contentBuffSize);
    					if (len < 0){
    					    printf ("libsrd.a: Unable to read content of <ok> node in server response.\n");
    					    ret = 0;
    					} else {
    					    *OKcontent = contentBuff; // caller needs to free this space
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
    				okValue = (char *)getFirstNodeValue(doc, xpathExpr);
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
	char *result;
	int n = -1;
	int intValue;

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
	if (!sendServer (sockfd, msg, strlen(msg))){
	    printf ("libsrd.a: Error in sending msg: %s\n", msg);
	    free (msg);
	    return 0;
	}
	if (!srd_isServerResponseOK (sockfd, &result)){
		printf ("libsrd.a: Server response to apply XPATH is not OK.\n");
		free (msg);
		return 0;
	}
	free (msg);
	return 1;
}

int
srd_listDataStores (int sockfd, char *name)
{
	return 1;
}
