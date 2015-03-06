/*
 * DataStore.cpp
 *
 *  Created on: Jan 30, 2015
 *      Author: niraj
 */

#include <string.h>
//#include <stdio.h>
//#include <libxml/parser.h>
//#include <libxml/tree.h>

#include "common.h"
#include "DataStore.h"

// DO NOT USE GLOBAL LOGGING MECHANISM IN THIS FILE

DataStore::DataStore(char *dsname, char * filename, char *xsddir, char *xsltdir)
{
   doc = NULL;
   if (filename == NULL || strlen(filename) > PATHLEN){
	   fileName[0] = '\0';
	   printf ("File path for Data Store '%s' is longer than the limit %d.\n", filename, PATHLEN);
   } else {
      strcpy (fileName, filename);
   }
   if (dsname == NULL || strlen (dsname) > MAXDATASTORENAMELEN){
	   printf ("Data Store name '%s' is longer than the limit %d.\n", dsname, MAXDATASTORENAMELEN);
	   name[0] = '\0';
   } else {
	   strcpy (name, dsname);
   }
   if (xsddir == NULL || strlen(xsddir) > PATHLEN){
	   printf ("XSD Dir path for Data Store '%s' is longer than the limit %d.\n", xsddir, PATHLEN);
   	   xsdDir[0] = '\0';
   } else {
       strcpy (xsdDir, xsddir);
   }
   if (xsltdir == NULL || strlen(xsltdir) > PATHLEN){
	   printf ("XSLT Dir path for Data Store '%s' is longer than the limit %d.\n", xsltdir, PATHLEN);
   	   xsltDir[0] = '\0';
   } else {
       strcpy (xsltDir, xsltdir);
   }
}

DataStore::DataStore(char *dsname)
{
   doc = NULL;
   fileName[0] = '\0';
   if (dsname == NULL || strlen (dsname) > MAXDATASTORENAMELEN){
	   printf ("Data Store name '%s' is longer than the limit %d.\n", dsname, MAXDATASTORENAMELEN);
	   name[0] = '\0';
   } else {
	   strcpy (name, dsname);
   }
}

DataStore::~DataStore()
{
   if (doc) xmlFreeDoc (doc);
   pthread_mutex_destroy (&dsMutex);
}

bool
DataStore::initialize (char *xml)
{
	if (xml == NULL || name == NULL || strlen (xml) == 0 || strlen(name) == 0) return false;
	doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
	if (doc == NULL)
	{
	   printf("Error: could not parse xml %s to create DOM XML tree.\n", xml);
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

	if(!doc) {
		sprintf (log, "Doc is NULL, why?");
		return NULL;
	}
	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		sprintf(log, "Error in xmlXPathNewContext");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		sprintf(log, "Error in xmlXPathEvalExpression");
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

    if (nodeSet == NULL) return 0;
    if(xmlXPathNodeSetIsEmpty(nodeSet)) return 0;
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
        } else if (cur_node->type == XML_TEXT_NODE){
        	xmlChar *curr_value;
        	curr_value = xmlNodeGetContent (cur_node);
        	if (curr_value && strlen ((char *)curr_value) > 0){
        		if (size < (offset + strlen((char *)curr_value) + lastOffset + 1 + 2)){ // need 2 byte space for '; ' the value separator
        		   size = offset + strlen((char *)curr_value) + lastOffset + 1 + 2;
        		   newSpace = (char *)realloc (*printBuffPtr, size);
        		   if (newSpace){
        		       *printBuffPtr = newSpace;
        		   } else {
        		       // unable to allocate space
        		       xmlFree (curr_value);
        		       return -1;
        		   }
        		}
        		if (i==0){ // first time no need to precede by value separator
        		   n = sprintf (*printBuffPtr+offset, "%s", curr_value);
        		} else {
        		   n = sprintf (*printBuffPtr+offset, "; %s", curr_value);
        		}
        		offset = offset + n;
        		xmlFree (curr_value);
        	}
        }
    }
    return (offset-initialOffset);

}

int
DataStore::printXPathAtomicResult (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize, int initialOffset)
{
	int retValue;
	char *res = NULL;
	int lastOffset = initialOffset + 2; // length of </ok></xml>
	int size = printBuffSize;
	char *newSpace;
	int offset = initialOffset;
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
	if (size < (offset + strlen(res) + lastOffset + 1)){
	    size = offset + strlen(res) + lastOffset + 1;
	    newSpace = (char *)realloc (*printBuffPtr, size);
	    if (newSpace){
	        *printBuffPtr = newSpace;
	    } else {
	        // unable to allocate space
	        if (res) free (res);
	        return -1;
	    }
	}
	n = sprintf (*printBuffPtr+offset, "%s", res);
	if (res) free (res);
	return n;
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
	   // error
		retValue = 0;
        strcpy (*printBuffPtr, log);
	} else if(objset->type == XPATH_STRING || objset->type == XPATH_NUMBER || objset->type == XPATH_BOOLEAN){
		// put the text form of XPATH result in *printBuffPtr
		n = printXPathAtomicResult (objset, printBuffPtr, printBuffSize, offset);
		if (n < 0){
			// error
			sprintf (*printBuffPtr, "<xml><error>Unable to print XPath result</error></xml>");
			retValue = 0;
		}
	} else if (objset->type == XPATH_NODESET || objset->type == XPATH_XSLT_TREE){
		// put serialized object set in *printBuffPtr
	    n = printElementSet (objset->nodesetval, printBuffPtr, printBuffSize, offset);
	    if (n < 0){
	       // error
   		   sprintf (*printBuffPtr, "<xml><error>Unable to print contents</error></xml>");
   		   retValue = 0;
	    }
    } else if (objset->type == XPATH_UNDEFINED){
    	strcpy (*printBuffPtr, "XPATH STRING: undefined");
    	retValue = 0;
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
		// error or result is an empty set
		if (strcmp(log, "No result") == 0){
			retValue = 0;
		}
	} else {
		// update chosen nodes
	    retValue = udpateSelectedNodes (objset->nodesetval, newValue);
	}
	if(unlockDS()){
		sprintf (log, "Error in unlocking data store, but %d values in data store modified. Inconsistant state reached.", retValue);
		retValue = -1;
	}
    return retValue;
}

int
DataStore::addNodes (xmlChar *xpath, char *nodeSetXML, char *log)
{
	xmlNodePtr parent, newNode;
	char *newNodesStrWrapped;
	xmlDocPtr newDoc;
	xmlXPathObjectPtr objset;
	xmlNodeSet *nodeSet;
	int i;
	xmlNode *cur_node;
	int count = 0;

	if (xpath == NULL || nodeSetXML == NULL || strlen ((char *)xpath) < 1 || strlen(nodeSetXML) < 1){
		sprintf (log, "All required parameters are not provided for DataStore::addNodes()");
		return -1;
	}
	newNodesStrWrapped = (char *) malloc (strlen (nodeSetXML) + 20);
	if (!newNodesStrWrapped){
		sprintf (log, "Unable to allocate space");
		return -1;
	}
	sprintf (newNodesStrWrapped, "<a>%s</a>", nodeSetXML);
	newDoc = xmlReadMemory (newNodesStrWrapped, strlen (newNodesStrWrapped), NULL, NULL, 0);
	free (newNodesStrWrapped);
	if (!newDoc){
		sprintf (log, "Failed to make DOM");
		return -1;
	}
	// apply xpath and get nodeset
	objset =  getNodeSet (xpath, log);
	if (!objset){
		xmlFreeDoc (newDoc);
		return 0;
	}
	nodeSet = objset->nodesetval;
	if (nodeSet == NULL || xmlXPathNodeSetIsEmpty(nodeSet)){
		return 0;
	}
	for (i=0; i < nodeSet->nodeNr; i++) {
	    cur_node = nodeSet->nodeTab[i];
	    if (cur_node->type == XML_ELEMENT_NODE) {
	    	// add new nodes to it
	    	newNode = xmlDocCopyNode(xmlDocGetRootElement(newDoc), doc, 1);
	    	if (!newNode){
	    		continue;
	    	}
	    	else {
	    		xmlNodePtr addedNode = xmlAddChildList (cur_node, newNode->children);
	    		if (addedNode){
	    		   count++;
	    		} else {
	    			xmlFreeNode (newNode);
	    		}
	    	}

	    }
	}
	// for testing dump doc
	// xmlDocDump (stdout, doc);
	return count;
}
