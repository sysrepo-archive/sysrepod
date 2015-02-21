/*
 * DataStore.cpp
 *
 *  Created on: Jan 30, 2015
 *      Author: niraj
 */

#include <string.h>

#include "common.h"
#include "DataStore.h"

// DO NOT USE GLOBAL LOGGING MECHANISM IN THIS FILE

DataStore::DataStore(char *dsname, char * filename, char *xsddir, char *xsltdir)
{
   doc = NULL;
   if (filename == NULL || strlen(filename) > PATHLEN){
	   fileName[0] = '\0';
   } else {
      strcpy (fileName, filename);
   }
   if (dsname == NULL || strlen (dsname) > MAXDATASTORENAMELEN){
	   name[0] = '\0';
   } else {
	   strcpy (name, dsname);
   }
   if (xsddir == NULL || strlen(xsddir) > PATHLEN){
   	   xsdDir[0] = '\0';
   } else {
       strcpy (xsdDir, xsddir);
   }
   if (xsltdir == NULL || strlen(xsltdir) > PATHLEN){
   	   xsltDir[0] = '\0';
   } else {
       strcpy (xsltDir, xsltdir);
   }
}

DataStore::~DataStore()
{
   if (doc) xmlFreeDoc (doc);
   pthread_mutex_destroy (&dsMutex);
}

bool
DataStore::initialize ()
{
	if (strlen (fileName) == 0 || strlen(name) == 0) return false;
	doc = xmlReadFile(fileName, NULL, 0);
	if (doc == NULL)
	{
	   printf("Error: could not parse file %s to create DOM XML tree.\n", fileName);
	   return false;

	}

   if (pthread_mutex_init(&dsMutex, NULL) != 0)
   {
	   // Data store mutex init failed.
	   xmlFreeDoc (doc);
	   doc = NULL;
	   printf ("Error: Failed to init Mutex for DataStore.\n");
	   return false;
   }
   return true;
}

xmlXPathObjectPtr
DataStore:: getNodeSet (xmlChar *xpath, char *log)
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

// lock and unlock return 0 on success
int
DataStore::lockDS(void)
{
	return pthread_mutex_lock(&dsMutex);
}

int
DataStore::unlockDS(void)
{
	return pthread_mutex_unlock(&dsMutex);
}

int
DataStore::printElementSet (xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize, int initialOffset)
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
DataStore::applyXPath(xmlChar *xpath, char **printBuffPtr, int printBuffSize, int offset)
{
	xmlXPathObjectPtr objset;
	char log[100];
	int retValue = 1;
	int n;

	if (lockDS()){
		sprintf (*printBuffPtr, "Error in locking data store");
		return 0;
	}
	objset =  getNodeSet (xpath, log);
	if (!objset){
	   // error or no result-empty set
	   if (strcmp (log, "No result") != 0){
		   retValue = 0;
           strcpy (*printBuffPtr, log);
	   }
	} else {
		// put serialized object set in *printBuffPtr
	    n = printElementSet (objset->nodesetval, printBuffPtr, printBuffSize, offset);
	    if (n < 0){
	       // error
   		   sprintf (*printBuffPtr, "<xml><error>Unable to print contents</error></xml>");
   		   retValue = 0;
	    }
    }
	xmlXPathFreeObject (objset);
	if(unlockDS()){
		sprintf (*printBuffPtr, "Error in unlocking data store");
		retValue = 0;
	}
	return retValue;
}

int
DataStore::udpateSelectedNodes (xmlNodeSetPtr nodes, xmlChar *newValue)
{
	int size;
	int i;
	int n = 0;

	if (!newValue) return -1;
	if (!nodes)    return -1;
	size = nodes->nodeNr;

	/*
	 * Note: Nodes are processed in reverse order, i.e. reverse document
	 *       order because xmlNodeSetContent can actually free up descendant
	 *       of the node.
	 */
	 for(i = size - 1; i >= 0; i--) {
		if (!nodes->nodeTab[i]) continue;
		xmlNodeSetContent(nodes->nodeTab[i], newValue);
		n++;
	 }
	 return n;
}

int
DataStore::updateNodes (xmlChar *xpath, xmlChar *newValue, char *log)
{
	int retValue = -1;
	xmlXPathObjectPtr objset;

	if (lockDS()){
		sprintf (log, "Error in locking data store");
		return -1;
    }
	objset = getNodeSet (xpath, log);
	if (!objset){
		// error or not result-empty set
		if (strcmp(log, "No result") == 0){
			retValue = 0;
		}
	} else {
		// update chosen nodes
	    retValue = udpateSelectedNodes (objset->nodesetval, newValue);
	}
	if(unlockDS()){
		sprintf (log, "Error in unlocking data store");
		retValue = -1;
	}
    return retValue;
}
