/*
 * ClientSRD.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: niraj
 */

#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "common.h"
#include "ClientSet.h"
#include "ClientSRD.h"
#include "DataStoreSet.h"

#include <libxml/xmlreader.h>

Client_SRD::Client_SRD()
{

}

Client_SRD::~Client_SRD() {
	// TODO Auto-generated destructor stub
}

static int
print_element_set(xmlDocPtr doc, xmlNodeSet * nodeSet, char **printBuffPtr, int initialOffset, int printBuffSize)
{
    xmlNode *cur_node = NULL;
    int n, i;
    int offset = initialOffset;
    xmlChar *value;
    xmlBuffer *buff;
    int lastOffset = initialOffset + 2; // length of </ok></xml>
    int size = printBuffSize;
    char *newSpace;

    for (i=0; i < nodeSet->nodeNr; i++) {
    	cur_node = nodeSet->nodeTab[i];
        if (cur_node->type == XML_ELEMENT_NODE) {
           buff = xmlBufferCreate ();
           xmlNodeDump (buff, doc,cur_node, 0, 1 );
           if (size < (offset + strlen((char *)buff->content) + lastOffset + 1)){
        	   size = offset + strlen((char *)buff->content) + lastOffset + 1;
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
    return (offset-initialOffset);
}


int
Client_SRD::processCommand (char *commandXML, char *outBuffer, int outBufferSize, struct clientInfo *cinfo)
{
	xmlDocPtr doc;
	xmlChar xpath[100];
	xmlChar *command;
	xmlChar *param1;
	int retValue = 0;

	strcpy((char *)xpath, "/xml/command");
	doc = xmlReadMemory(commandXML, strlen(commandXML), "noname.xml", NULL, 0);
    if (doc == NULL){
			sprintf (outBuffer, "<xml><error>XML Document not correct</error></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
			return 0;
	}
    command = ClientSet::GetFirstNodeValue(doc, xpath);
    if (command == NULL){
			sprintf (outBuffer, "<xml><error>No command found.</error></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
			xmlFreeDoc (doc);
			return 0;
	}
	if (strcmp((char *)command, "set_datastore") == 0){
		    strcpy ((char *) xpath, "/xml/param1");
            param1 = ClientSet::GetFirstNodeValue(doc, xpath);
            if(param1 == NULL){
               sprintf (outBuffer, "<xml><error>Value of data store not found</error></xml>");
			   common::SendMessage(cinfo->sock, outBuffer);
            } else {
               cinfo->dataStore = DataStores->getDataStore ((char *)param1);
               if (cinfo->dataStore){
                  sprintf (outBuffer, "<xml><ok/></xml>");
               } else {
            	   sprintf (outBuffer, "<xml><error>Data Store %s not found</error></xml>", param1);
               }
               common::SendMessage(cinfo->sock, outBuffer);
               xmlFree (param1);
            }
	} else if(strcmp ((char *)command, "apply_xpath")==0){
		char *printBuff = NULL;
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
	    if(param1 == NULL){
		     sprintf (outBuffer, "<xml><error>XPath expression not found</error></xml>");
		} else {
            xmlXPathObjectPtr objset;
            char log [200];
            objset =  ClientSet::GetNodeSet (cinfo->dataStore->doc, param1, log);
            if (!objset){
            	// error
            	sprintf (outBuffer, "<xml><error>%s</error></xml>", log);
            } else {
            	int offset1 = 0, n;
            	int printBuffSize = 100;

            	printBuff = (char *)malloc (printBuffSize);
            	offset1 = sprintf (printBuff, "<xml><ok>");
            	n = print_element_set (cinfo->dataStore->doc, objset->nodesetval, &printBuff, offset1, printBuffSize);
            	if (n < 0){
            		// error
            		sprintf (outBuffer, "<xml><error>Unable to print contents.</error></xml>");
            		free (printBuff);
            		printBuff = NULL;
            	} else {
            	   strcat (printBuff, "</ok></xml>");
            	}
            	xmlXPathFreeObject (objset);
            }
			xmlFree(param1);
	    }
	    if (printBuff == NULL){
		   common::SendMessage(cinfo->sock, outBuffer);
	    } else {
		   common::SendMessage(cinfo->sock, printBuff);
		   free (printBuff);
		}
	}else if (strcmp ((char *)command, "terminate") == 0){ // terminate this server
		retValue = -1;
		sprintf (outBuffer, "<xml><ok/></xml>");
		common::SendMessage(cinfo->sock, outBuffer);
	}else if (strcmp ((char *)command, "disconnect") == 0){ // disconnect this client
			retValue = -2;
			sprintf (outBuffer, "<xml><ok/></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
	} else {
			sprintf (outBuffer, "<xml><error>Command not supported: %s</error></xml>", (char *) command);
			common::SendMessage(cinfo->sock, outBuffer);
	}
	xmlFree (command);
	xmlFreeDoc(doc);
	return retValue;
}

