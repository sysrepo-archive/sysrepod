/*
 * DataStore.cpp
 *
  * License : Apache 2.0
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#include <string.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>

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
   lockedBy = NULL;
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
   lockedBy = NULL;
}

DataStore::~DataStore()
{
   if (doc) xmlFreeDoc (doc);
   if (lockedBy) {
	   unlockDS(lockedBy);
	   printf ("Looks like a harmless logical error. Control should not have reached here.\n");
   }
   pthread_mutex_destroy (&dsMutex);
}

bool
DataStore::initialize (char *xml)
{
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_ERRORCHECK);

	if (xml == NULL || name == NULL || strlen (xml) == 0 || strlen(name) == 0) return false;
	doc = xmlReadMemory(xml, strlen(xml), "noname.xml", NULL, 0);
	if (doc == NULL)
	{
	   printf("Error: could not parse xml %s to create DOM XML tree.\n", xml);
	   return false;

	}

   if (pthread_mutex_init(&dsMutex, &Attr) != 0)
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
	pthread_mutexattr_t Attr;
	pthread_mutexattr_init(&Attr);
	pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_ERRORCHECK);

	if (strlen (fileName) == 0 || strlen(name) == 0) return false;
	doc = xmlReadFile(fileName, NULL, 0);
	if (doc == NULL)
	{
	   printf("Warning: could not parse file %s to create DOM XML tree.\n", fileName);
	   return false;

	}

   if (pthread_mutex_init(&dsMutex, &Attr) != 0)
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

	log[0] = '\0';
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
DataStore::lockDS(struct ClientInfo *cinfo)
{
	int n = 1;
	n = pthread_mutex_lock(&dsMutex);
	if (n == 0){
		lockedBy = cinfo;
	}
	return n;
}

int
DataStore::unlockDS(struct ClientInfo *cinfo)
{
	int n = 1;
	if (lockedBy == cinfo){
	    n = pthread_mutex_unlock(&dsMutex);
	    if (n == 0) lockedBy = NULL;
	}
	return n;
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
DataStore::applyXPath(struct ClientInfo *cinfo, xmlChar *xpath, char **printBuffPtr, int printBuffSize, int offset)
{
	xmlXPathObjectPtr objset;
	char log[100];
	int retValue = 1;
	int n;

	if (lockDS(cinfo)){
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
	if(unlockDS(cinfo)){
		sprintf (*printBuffPtr, "Error in unlocking data store");
		retValue = 0;
	}
	return retValue;
}

void
DataStore::removeChar(char *str, char garbage) {

    char *src, *dst;
    for (src = dst = str; *src != '\0'; src++) {
        *dst = *src;
        if (*dst != garbage) dst++;
    }
    *dst = '\0';
}

int
DataStore::applyXSLT(struct ClientInfo *cinfo, char *xslt, char **printBuffPtr, int printBuffSize, int offset)
{
	xsltStylesheetPtr cur = NULL;
	char log[100];
	int retValue = 1, rc;
	int n;
	xmlDocPtr sheetDoc, res;
	char *buf = NULL;
	int   size;
	char *newSpace;
	int lastOffset = offset + 2; // length of </ok></xml>

	if (lockDS(cinfo)){
		sprintf (*printBuffPtr, "Error in locking data store");
		return 0;
	}
	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;
	removeChar (xslt, '\\');
	sheetDoc = xmlReadMemory(xslt, strlen(xslt), "noname.xml", NULL, 0);
	if (sheetDoc == NULL){
		sprintf (*printBuffPtr, "<xml><error>Unable to form doc from xslt string</error></xml>");
		printf ("Style Sheet is : \n%s\n", xslt);
		unlockDS (cinfo);
		return 0;
	}
	cur = xsltParseStylesheetDoc(sheetDoc);
	if (cur == NULL){
		sprintf (*printBuffPtr, "<xml><error>Unable to form style sheet</error></xml>");
		xmlFreeDoc (sheetDoc);
		unlockDS (cinfo);
		return 0;
	}
	res = xsltApplyStylesheet (cur, doc, NULL);
	if (res == NULL){
		sprintf (*printBuffPtr, "<xml><error>Error in applying xslt on data store</error></xml>");
		xsltFreeStylesheet (cur); // do not free sheetDoc, it is done intrnally by this call
		unlockDS (cinfo);
		return 0;
	}
	rc = xsltSaveResultToString((xmlChar **)&buf, &size, res, cur);
	if (rc >=0 && buf != NULL){ // rc == -1 means error
		if (size > 0){
			// copy buffer to *printBuffPtr
			if (printBuffSize < (offset + size + lastOffset + 2)){
				printBuffSize = offset + size + lastOffset + 2;
                newSpace = (char *)realloc (*printBuffPtr, printBuffSize);
                if (!newSpace){
                	sprintf (*printBuffPtr, "<xml><error>Unable to allocate space to store results</error></xml>");
                	xmlFree (buf);
                	xsltFreeStylesheet(cur); // Do not free sheetDoc, it is implicitly freed by this call
                	xmlFreeDoc(res);
                	xsltCleanupGlobals();
                	xmlCleanupParser();
                	unlockDS (cinfo);
                	return 0;
                }
                *printBuffPtr = newSpace;
			}
			// actual copy
			memcpy(*printBuffPtr+offset, buf, size);
			*(*printBuffPtr+offset+size) = '\0';
		}
		xmlFree (buf);
	}
	xsltFreeStylesheet(cur); // Do not free sheetDoc, it is implicitly freed by this call
	xmlFreeDoc(res);
	xsltCleanupGlobals();
	xmlCleanupParser();
	if(unlockDS(cinfo)){
		sprintf (*printBuffPtr, "Error in unlocking data store");
		retValue = 0;
	}
	return retValue;
}

int
DataStore::deleteNodes(struct ClientInfo *cinfo, xmlChar *xpath, char *log)
{
	int retValue;
	xmlXPathObjectPtr objset;

	log[0] = '\0';
	if (lockDS(cinfo)){
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
	    retValue = deleteSelectedNodes (objset->nodesetval);
	    if (!retValue){
	    	sprintf (log, "No result");
	    }
	}

	if(unlockDS(cinfo)){
		sprintf (log, "Error in unlocking data store, but %d values in data store modified. Inconsistant state reached.", retValue);
		retValue = -1;
	}
    return retValue;
}

int
DataStore::deleteSelectedNodes (xmlNodeSetPtr nodes)
{
	int size;
	int i;
	int n = 0;
	xmlNode *curr_node;

	if (!nodes)    return -1;
	size = nodes->nodeNr;

	/*
	 * Note: Nodes are processed in reverse order, i.e. reverse document
	 *       order because xmlNodeSetContent can actually free up descendant
	 *       of the node.
	 */
	 for(i = size - 1; i >= 0; i--) {
		 curr_node = nodes->nodeTab[i];
		 if (curr_node) {
			 xmlUnlinkNode (curr_node);
			 xmlFreeNode (curr_node); // Removed node needs to be freed explicitly
		     n++;
		 }
	 }
	 return n;
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
DataStore::updateNodes (struct ClientInfo *cinfo, xmlChar *xpath, xmlChar *newValue, char *log)
{
	int retValue = -1;
	xmlXPathObjectPtr objset;

	log[0] = '\0';
	if (lockDS(cinfo)){
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
	    if (retValue == 0){
	    	sprintf (log, "No result");
	    } else if (retValue < 0){
	    	sprintf (log, "Error in adding nodes");
	    }
	}

	if(unlockDS(cinfo)){
		sprintf (log, "Error in unlocking data store, but %d values in data store modified. Inconsistant state reached.", retValue);
		retValue = -1;
	}
    return retValue;
}

int
DataStore::addNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *nodeSetXML, char *log)
{
	xmlNodePtr parent, newNode;
	char *newNodesStrWrapped;
	xmlDocPtr newDoc;
	xmlXPathObjectPtr objset;
	xmlNodeSet *nodeSet;
	int i;
	xmlNode *cur_node;
	int count = 0;

	log[0] = '\0';
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
	if (lockDS(cinfo)){
		sprintf (log, "Error in locking data store");
		return -1;
	}
	// apply xpath and get nodeset
	objset =  getNodeSet (xpath, log);
	if (!objset){
		xmlFreeDoc (newDoc);
		unlockDS(cinfo);
		return 0;
	}
	nodeSet = objset->nodesetval;
	if (nodeSet == NULL || xmlXPathNodeSetIsEmpty(nodeSet)){
		unlockDS(cinfo);
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
	unlockDS(cinfo);
	return count;
}
