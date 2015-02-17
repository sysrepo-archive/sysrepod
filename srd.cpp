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
//int sendServer (int sockfd, char * message, int msgSize);

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
   if (!srd_isServerResponseOK (*sockfd)){
	   printf ("libsrd.a: Server response to Protocol command is not OK.\n");
	   close (*sockfd);
	   *sockfd = -1;
	   return 0;
   }
   xmlInitParser();
   return 1;
}

int
srd_setDatastore (int sockfd, char *dsname)
{
   char *msg;

   msg = (char *)malloc (strlen(dsname) + 100);
   sprintf (msg, "<xml><command>set_datastore</command><param1>%s</param1></xml>", dsname);
   if (!sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
      free (msg);
      return 0;
   }
   if (!srd_isServerResponseOK (sockfd)){
	       printf ("libsrd.a: Server response to set data store is not OK.\n");
           free (msg);
	   return 0;
   }
   free (msg);
   return 1;
}

void
srd_applyXPath (int sockfd, char *xpath)
{
   char *msg;

   msg = (char *)malloc (strlen(xpath) + 100);
   sprintf (msg, "<xml><command>apply_xpath</command><param1>%s</param1></xml>", xpath);
   if (!sendServer (sockfd, msg, strlen(msg))){
      printf ("libsrd.a: Error in sending msg: %s\n", msg);
   }
   if (!srd_isServerResponseOK (sockfd)){
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
	if (!srd_isServerResponseOK (sockfd)){
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
	if (!srd_isServerResponseOK (sockfd)){
		   printf ("libsrd.a: Server response to disconnect is not OK.\n");
	}
	xmlCleanupParser();
	close (sockfd);
}

int
srd_isServerResponseOK(int sockfd)
{
	int ret;
	char buff[500];
	char *buffPtr = buff;
	int buffSize = sizeof(buff) - 1;
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr  xpathObj;
	xmlChar xpathExpr[100];
	xmlNodeSetPtr nodes;

	//printf ("libsrd.a: entered srd_isServerResponseOK function.\n");
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
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
	   // Error: unable to create new XPath context
       printf("libsrd.a: Unable to create XML Context\n");
	   xmlFreeDoc(doc);
	   return(0);
	}
	strcpy ((char *)xpathExpr, "/xml/ok");
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    if(xpathObj == NULL) {
	   // Error: unable to evaluate xpath expression
	   xmlXPathFreeContext(xpathCtx);
	   xmlFreeDoc(doc);
       printf("libsrd.a: Unable to parse server response: xpath failed\n");
	   return(0);
	}
    nodes = xpathObj->nodesetval;
    if(nodes->nodeNr > 0){
       // printf("libsrd.a: OK response : return 1\n");
    	ret = 1;
    }else{
        //printf("libsrd.a: OK response : return 0\n");
    	ret = 0;
    }
    xmlXPathFreeContext(xpathCtx);
    xmlFreeDoc(doc);
    return ret;
}
