/*
 * ClientSRD.cpp
 *
 *  Created on: Feb 4, 2015
 *      Author: niraj
 */

#include <string.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <unistd.h>

#include "common.h"
#include "ClientSet.h"
#include "ClientSRD.h"
#include "DataStoreSet.h"
#include "OpDataStoreSet.h"

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
	int n;

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
	if (strcmp((char *)command, "set_dataStore") == 0){
		    strcpy ((char *) xpath, "/xml/param1");
            param1 = ClientSet::GetFirstNodeValue(doc, xpath);
            if(param1 == NULL){
               cinfo->dataStore = NULL;
               sprintf (outBuffer, "<xml><ok>Data Store set to NULL for this client</ok></xml>");
			   common::SendMessage(cinfo->sock, outBuffer);
            } else {
               cinfo->dataStore = DataStores->getDataStore ((char *)param1);
               if (cinfo->dataStore){
                  sprintf (outBuffer, "<xml><ok/></xml>");
               } else {
            	   sprintf (outBuffer, "<xml><error>Data Store not found</error></xml>");
               }
               common::SendMessage(cinfo->sock, outBuffer);
               xmlFree (param1);
            }
	} else if (strcmp((char *)command, "stopUsing_opDataStore") == 0){
	    strcpy ((char *) xpath, "/xml/param1");
        param1 = ClientSet::GetFirstNodeValue(doc, xpath);
        if(param1 == NULL){
        	n = OpDataStores->removeOwner (cinfo);
        	if (n < 0){
        	    sprintf (outBuffer, "<xml><error>Failed to lock Op DataStoreSet</error></xml>");
        	} else {
        	    sprintf (outBuffer, "<xml><ok>%d</ok></xml>", n);
        	}
        	common::SendMessage(cinfo->sock, outBuffer);
        } else {
           n = OpDataStores->removeOwner ((char *)param1, cinfo);
           if (n < 0){
        	  sprintf (outBuffer, "<xml><error>Either Op Data Store not found or client does not own it</error></xml>");
           } else {
              sprintf (outBuffer, "<xml><ok>%d</ok></xml>", n);
           }
           common::SendMessage(cinfo->sock, outBuffer);
           xmlFree (param1);
        }
	} else if (strcmp ((char *)command, "use_opDataStore") == 0){
	    strcpy ((char *) xpath, "/xml/param1");
        param1 = ClientSet::GetFirstNodeValue(doc, xpath);
        if(param1 == NULL){
           sprintf (outBuffer, "<xml><error>Op Data Store name missing</error></xml>");
		   common::SendMessage(cinfo->sock, outBuffer);
        } else {
           if(!OpDataStores->setOwner ((char *)param1, cinfo)){
        	  sprintf (outBuffer, "<xml><error>Either Op Data Store not found or it is already owned by some other client</error></xml>");
           } else {
              sprintf (outBuffer, "<xml><ok/></xml>");
           }
           common::SendMessage(cinfo->sock, outBuffer);
           xmlFree (param1);
        }
	} else if (strcmp((char *)command, "create_opDataStore") == 0){
	    strcpy ((char *) xpath, "/xml/param1");
        param1 = ClientSet::GetFirstNodeValue(doc, xpath);
        if(param1 == NULL){
           sprintf (outBuffer, "<xml><error>Op Data Store name missing</error></xml>");
		   common::SendMessage(cinfo->sock, outBuffer);
        } else {
           n = OpDataStores->addOpDataStore ((char *)param1);
           if (n == 0){
        	  sprintf (outBuffer, "<xml><error>Op Data Store not added</error></xml>");
           } else if (n == 1){
              sprintf (outBuffer, "<xml><ok/></xml>");
           } else if (n == 2){
        	   sprintf (outBuffer, "<xml><ok>Op Data Store already exists - no action</ok></xml>");
           }
           common::SendMessage(cinfo->sock, outBuffer);
           xmlFree (param1);
        }
	} else if (strcmp ((char *)command, "delete_opDataStore") == 0){
	    strcpy ((char *) xpath, "/xml/param1");
        param1 = ClientSet::GetFirstNodeValue(doc, xpath);
        if(param1 == NULL){
           sprintf (outBuffer, "<xml><error>Op Data Store name missing</error></xml>");
		   common::SendMessage(cinfo->sock, outBuffer);
        } else {
           n = OpDataStores->deleteOpDataStore ((char *)param1, cinfo);
           if ( n < 0){
        	  sprintf (outBuffer, "<xml><error>Either Op Data Store does not exist or you do not own it</error></xml>");
           } else {
              sprintf (outBuffer, "<xml><ok>%d</ok></xml>", n);
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
			} else if (cinfo->dataStore == NULL){
				sprintf (printBuff, "<xml><error>Data Store not set. Use srd_setDataStore() first</error></xml>");
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
	} else if (strcmp ((char *)command, "apply_xpathOpDataStore") == 0){
		char *printBuff = NULL;
		// need to read 2 parameters: OpDataStore Name and XPath to apply
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
		if(param1 == NULL){
			sprintf (outBuffer, "<xml><error>Op DataStore name not found</error></xml>");
		} else {
			char log[100];
			strcpy ((char *) xpath, "/xml/param2");
			param2 = ClientSet::GetFirstNodeValue(doc, xpath);
			if(param2 == NULL){
				sprintf (outBuffer, "<xml><error>XPath not found</error></xml>");
		    } else {
			    int printBuffSize = 100;
			    printBuff = (char *)malloc (printBuffSize);
			    if (printBuff == NULL) {
			       sprintf (outBuffer, "<xml><error>Unable to allocate buffer space</error></xml>");
			    } else if (!OpDataStores->applyXPathOpDataStore (commandXML, (char *)param1, &printBuff, printBuffSize)){
		        	sprintf (outBuffer, "<xml><error>%s</error></xml>", printBuff);
		        	free (printBuff);
		        	printBuff = NULL;
		        }
			    xmlFree (param2);
			}
		    xmlFree (param1);
		}
        if (printBuff == NULL){
        	common::SendMessage(cinfo->sock, outBuffer);
        }else {
        	common::SendMessage(cinfo->sock, &(printBuff[MSGLENFIELDWIDTH + 1]));
        	free (printBuff);
        }
	} else if (strcmp ((char *)command, "add_nodesDataStore") == 0){
        // param1 expected to contain XPath to select a set of nodes and param2 will have a set of XML nodes that are
		// to be put under the nodes selected by XPath.
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
		if(param1 == NULL){
			sprintf (outBuffer, "<xml><error>XPath not found</error></xml>");
		} else {
			char  log[100];
			int   param2BuffSize = 100;
			char *param2Value = NULL;

			strcpy ((char *) xpath, "/xml/param2/*");
			// value of param2 is a node set, not a string like in other cases
			param2Value = (char *) malloc (param2BuffSize + 1);
			if (param2Value == NULL){
				sprintf (outBuffer, "<xml><error>Unable to allocate space to extract value of param2</error></xml>");
			} else {
			    if (!applyXPath(doc, xpath, &param2Value, param2BuffSize)){
				   sprintf (outBuffer, "<xml><error>Unable to find param2 value</error></xml>");
			    } else if (cinfo->dataStore == NULL){
			   	   sprintf (outBuffer, "<xml><error>Data Store not set. Use srd_setDataStore() first</error></xml>");
			    } else {
			   	   if ((n = (cinfo->dataStore->addNodes (param1, param2Value, log))) < 0){
			   			sprintf (outBuffer, "<xml><error>%s</error></xml>", log);
			   	   } else {
			   			sprintf (outBuffer, "<xml><ok>%d</ok></xml>", n);
				   }
				}
			    free (param2Value);
			}
			xmlFree (param1);
		}
		common::SendMessage(cinfo->sock, outBuffer);
	} else if (strcmp ((char *)command, "lock_dataStore") == 0){
		if (cinfo->dataStore == NULL){
			sprintf (outBuffer, "<xml><error>Data Store not set. Use srd_setDataStore() first</error></xml>");
		} else if(!cinfo->dataStore->lockDS()){
        	sprintf (outBuffer, "<xml><ok/></xml>");
        } else {
        	sprintf (outBuffer, "<xml><error>Unable to lock data store %s</error></xml>", cinfo->dataStore->name);
        }
		common::SendMessage(cinfo->sock, outBuffer);
	} else if (strcmp ((char *)command, "unlock_dataStore") == 0){
		if (cinfo->dataStore == NULL){
			sprintf (outBuffer, "<xml><error>Data Store not set. Use srd_setDataStore() first</error></xml>");
		}else if(!cinfo->dataStore->unlockDS()){
		    sprintf (outBuffer, "<xml><ok/></xml>");
		} else {
		    sprintf (outBuffer, "<xml><error>Unable to unlock data store %s</error></xml>", cinfo->dataStore->name);
		}
		common::SendMessage(cinfo->sock, outBuffer);
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
			} else if (cinfo->dataStore == NULL){
				sprintf (outBuffer, "<xml><error>Data Store not set. Use srd_setDataStore() first</error></xml>");
				common::SendMessage(cinfo->sock, outBuffer);
				xmlFree (param2);
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
	} else if (strcmp((char *)command, "register_clientSocket") == 0){
		// param1 contains IP Address and param2 contains Port Number
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
		if(param1 == NULL){
		    sprintf (outBuffer, "<xml><error>Value of IP Address not found</error></xml>");
		    common::SendMessage(cinfo->sock, outBuffer);
		} else {
			char log[100];
			strcpy ((char *) xpath, "/xml/param2");
			param2 = ClientSet::GetFirstNodeValue(doc, xpath);
			if(param2 == NULL){
			    sprintf (outBuffer, "<xml><error>Port Number not found</error></xml>");
			    common::SendMessage(cinfo->sock, outBuffer);
			} else {
				int n = 0;
				int portNum = -1;
				n = sscanf ((char *)param2, "%d", &portNum);
				if (n != 1) {
					portNum = -1;
					printf ("Port number is not a proper integer.\n");
				}
				cinfo->clientSet->saveClientBackConnectionInfo (cinfo, (char *)param1, portNum);
                sprintf (outBuffer, "<xml><ok/></xml>");
                common::SendMessage(cinfo->sock, outBuffer);
				xmlFree (param2);
			}
		    xmlFree (param1);
		}
	} else if (strcmp ((char *)command, "create_dataStore") == 0){
		// param1 contains the name of the new data store and param2 contains XML content of the data store
		strcpy ((char *) xpath, "/xml/param1");
		param1 = ClientSet::GetFirstNodeValue(doc, xpath);
		if(param1 == NULL){
		   sprintf (outBuffer, "<xml><error>Data Store name not found.</error></xml>");
		   common::SendMessage(cinfo->sock, outBuffer);
		} else {
			char log[100];
			int param2InitialSize = 100;

			param2 = (xmlChar *)malloc (param2InitialSize);
			strcpy ((char *) xpath, "/xml/param2/*");
			if (!param2){
				sprintf (outBuffer, "<xml><error>Unable to allocate buffer space</error></xml>");
				common::SendMessage (cinfo->sock, outBuffer);
			} else {
				if (!applyXPath (doc, xpath, (char **)&param2, param2InitialSize)){
					sprintf (outBuffer, "<xml><error>XML Tree for Data Store missing</error></xml>");
					common::SendMessage(cinfo->sock, outBuffer);
				} else {
				    if (!DataStores->addDataStoreFromString ((char *)param1, (char *)param2)){
					    sprintf (outBuffer, "<xml><error>Failed to make DOM</error></xml>");
					    common::SendMessage(cinfo->sock, outBuffer);
				    } else {
		                sprintf (outBuffer, "<xml><ok/></xml>");
		                common::SendMessage(cinfo->sock, outBuffer);
				    }
				}
		        xmlFree (param2);
			}
			xmlFree (param1);
		}
	} else if (strcmp ((char *)command, "list_dataStores") == 0){
		char *list = NULL;
		char *msg = NULL;
		char  localBuff[100];
		list = DataStores->getList ();
		if (list && strlen (list) > 0){
			msg = (char *)malloc (strlen(list) + 100);
			if (!msg){
				sprintf (localBuff, "<xml><error>Unable to allocate space</error></xml>");
			} else {
			    sprintf (msg, "<xml><ok>%s</ok></xml>", list);
			}
		} else {
			// list is empty
			    sprintf (localBuff, "<xml><ok><dataStores></dataStores></ok></xml>");
		}
		if (msg){
            common::SendMessage(cinfo->sock, msg);
            free (msg);
		} else {
			common::SendMessage(cinfo->sock, localBuff);
		}
        if (list) free (list);
	} else if (strcmp ((char *)command, "list_opDataStores") == 0){
		char *list = NULL;
		char *msg = NULL;
		char  localBuff[100];
		list = OpDataStores->getList ();
		if (list && strlen (list) > 0){
			msg = (char *)malloc (strlen(list) + 100);
			if (!msg){
				sprintf (localBuff, "<xml><error>Unable to allocate space</error></xml>");
			} else {
			    sprintf (msg, "<xml><ok>%s</ok></xml>", list);
			}
		} else {
			// list is empty
			sprintf (localBuff, "<xml><ok><opDataStores></opDataStores></ok></xml>");
		}
		if (msg){
            common::SendMessage(cinfo->sock, msg);
            free (msg);
		} else {
			common::SendMessage(cinfo->sock, localBuff);
		}
        if (list) free (list);
	} else if (strcmp((char *)command, "list_myUsageOpDataStores") == 0){
		char *list = NULL;
		char *msg = NULL;
		char  localBuff[100];
		list = OpDataStores->listMyUsage (cinfo);
		if (list && strlen (list) > 0){
			msg = (char *)malloc (strlen(list) + 100);
			if (!msg){
				sprintf (localBuff, "<xml><error>Unable to allocate space</error></xml>");
			} else {
			    sprintf (msg, "<xml><ok>%s</ok></xml>", list);
			}
		} else {
			// list is empty
			sprintf (localBuff, "<xml><ok><opDataStores></opDataStores></ok></xml>");
		}
		if (msg){
            common::SendMessage(cinfo->sock, msg);
            free (msg);
		} else {
			common::SendMessage(cinfo->sock, localBuff);
		}
        if (list) free (list);
	} else if (strcmp((char *)command, "delete_dataStore") == 0){
		int n;
	    strcpy ((char *) xpath, "/xml/param1");
        param1 = ClientSet::GetFirstNodeValue(doc, xpath);
        if(param1 == NULL){
           sprintf (outBuffer, "<xml><error>Parameter - data store not found</error></xml>");
        } else {
           if ((n = DataStores->deleteDataStore (cinfo->clientSet, (char *)param1)) >=0){
              sprintf (outBuffer, "<xml><ok>%d</ok></xml>", n);
           } else {
        	   sprintf (outBuffer, "<xml><error>Error in deleting Data Store</error></xml>");
           }
        }
        common::SendMessage(cinfo->sock, outBuffer);
        xmlFree (param1);
	}else if (strcmp ((char *)command, "terminate") == 0){ // terminate this server
		retValue = -1;
		sprintf (outBuffer, "<xml><ok/></xml>");
		common::SendMessage(cinfo->sock, outBuffer);
	}else if (strcmp ((char *)command, "disconnect") == 0){ // disconnect this client
			retValue = -2;
			sprintf (outBuffer, "<xml><ok/></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
			sleep (3); // Do not want to kill socket too quickly, otherwise client will not be able to read response
	} else {
			sprintf (outBuffer, "<xml><error>Command not supported</error></xml>");
			common::SendMessage(cinfo->sock, outBuffer);
	}
	xmlFree (command);
	xmlFreeDoc(doc);
	return retValue;
}

int
Client_SRD::printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize)
{
    xmlNode *cur_node = NULL;
    int n, i;
    int offset = 0;
    xmlChar *value;
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
Client_SRD::applyXPath(xmlDocPtr doc, xmlChar *xpath, char **printBuffPtr, int printBuffSize)
{
	xmlXPathObjectPtr objset;
	char log[100];
	int retValue = 1;
	int n;

	objset =  getNodeSet (doc, xpath, log);
	if (!objset){
	   // error or no result-empty set
	   if (strcmp (log, "No result") != 0){
		   retValue = 0;
           strcpy (*printBuffPtr, log);
	   }
	} else {
		// put serialized object set in *printBuffPtr
	    n = printElementSet (doc, objset->nodesetval, printBuffPtr, printBuffSize);
	    if (n < 0){
	       // error
   		   sprintf (*printBuffPtr, "<xml><error>Unable to print contents</error></xml>");
   		   retValue = 0;
	    }
    }
	xmlXPathFreeObject (objset);
	return retValue;
}

xmlXPathObjectPtr
Client_SRD::getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log)
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


