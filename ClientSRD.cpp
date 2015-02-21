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

int
Client_SRD::processCommand (char *commandXML, char *outBuffer, int outBufferSize, struct clientInfo *cinfo)
{
	xmlDocPtr doc;
	xmlChar xpath[100];
	xmlChar *command;
	xmlChar *param1, *param2;
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
			int offset1 = 0;
			int printBuffSize = 100;
			printBuff = (char *)malloc (printBuffSize);
			if (!printBuff){
				sprintf (outBuffer, "<xml><error>Unable to allocate buffer space</error></xml>");
			} else {
				offset1 = sprintf(printBuff, "<xml><ok>");
				if (!cinfo->dataStore->applyXPath (param1, &printBuff, printBuffSize, offset1)){
					sprintf (outBuffer, "<xml><error>%s</error></xml>", printBuff);
					free (printBuff);
					printBuff = NULL;
				} else {
					strcat (printBuff, "</ok></xml>");
				}
			}
			xmlFree(param1);
	    }
	    if (printBuff == NULL){
		   common::SendMessage(cinfo->sock, outBuffer);
	    } else {
		   common::SendMessage(cinfo->sock, printBuff);
		   free (printBuff);
		}
	} else if (strcmp ((char *)command, "lock_dataStore") == 0){
        if(!cinfo->dataStore->lockDS()){
        	sprintf (outBuffer, "<xml><ok/></xml>");
        	common::SendMessage(cinfo->sock, outBuffer);
        } else {
        	sprintf (outBuffer, "<xml><error>Unable to lock data store %s</error></xml>", cinfo->dataStore->name);
        	common::SendMessage(cinfo->sock, outBuffer);
        }
	} else if (strcmp ((char *)command, "unlock_dataStore") == 0){
		if(!cinfo->dataStore->unlockDS()){
		    sprintf (outBuffer, "<xml><ok/></xml>");
		    common::SendMessage(cinfo->sock, outBuffer);
		} else {
		    sprintf (outBuffer, "<xml><error>Unable to unlock data store %s</error></xml>", cinfo->dataStore->name);
		    common::SendMessage(cinfo->sock, outBuffer);
		}
	} else if (strcmp ((char *)command, "update_nodes") == 0){
		// param1 contains xpath, param2 contains new value
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
		if(param1 == NULL){
		    sprintf (outBuffer, "<xml><error>Value of xpath not found</error></xml>");
		    common::SendMessage(cinfo->sock, outBuffer);
		} else {
			char log[100];

			strcpy ((char *) xpath, "/xml/param2");
			param2 = ClientSet::GetFirstNodeValue(doc, xpath);
			if(param2 == NULL){
			    sprintf (outBuffer, "<xml><error>New value not found</error></xml>");
			    common::SendMessage(cinfo->sock, outBuffer);
			} else {
				int numNodesModified;
                numNodesModified = cinfo->dataStore->updateNodes (param1, param2, log);
                if (numNodesModified < 0){
                	sprintf (outBuffer, "<xml><error>Error in modifying nodes: %s</error></xml>", log);
                } else {
                	sprintf (outBuffer, "<xml><ok>%d</ok></xml>",numNodesModified);
                }
                common::SendMessage(cinfo->sock, outBuffer);
				xmlFree (param2);
			}
		    xmlFree (param1);
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

