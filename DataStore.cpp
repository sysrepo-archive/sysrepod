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
#include <dirent.h>

#include "common.h"
#include "DataStore.h"

// DO NOT USE GLOBAL LOGGING MECHANISM IN THIS FILE

DataStore::DataStore(char *dsname, char * filename, char *checkdir, char *yangdir)
{
   doc = NULL;
   if (filename == NULL || strlen(filename) > PATHLEN){
	   fileName[0] = '\0';
	   printf ("File path for Data Store is longer than the limit %d or NULL.\n", PATHLEN);
   } else {
      strcpy (fileName, filename);
   }
   if (dsname == NULL || strlen (dsname) > MAXDATASTORENAMELEN){
	   printf ("Data Store name is longer than the limit %d or NULL.\n", MAXDATASTORENAMELEN);
	   name[0] = '\0';
   } else {
	   strcpy (name, dsname);
   }
   if (checkdir == NULL || strlen(checkdir) > PATHLEN){
	   printf ("CHECK Dir path for Data Store '%s' is longer than the limit %d. or NULL\n", name, PATHLEN);
   	   checkDir[0] = '\0';
   } else {
       strcpy (checkDir, checkdir);
   }
   if (yangdir == NULL || strlen(yangdir) > PATHLEN){
   	   printf ("YANG Dir path for Data Store '%s' is longer than the limit %d. or NULL\n", name, PATHLEN);
       yangDir[0] = '\0';
   } else {
       strcpy (yangDir, yangdir);
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
   checkDir[0] = '\0';
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
	char log[MAXYANGERRORLEN+100];
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

	// Apply constraints on doc to make sure it is correct
	log[0] = '\0';
	if (!applyConstraints (log, MAXYANGERRORLEN)){
		printf ("Warning: Constraints failed. Data store will not be created.\n");
		if (strlen(log) > 0) printf ("%s\n", log);
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
DataStore::applyConstraints (char *log, int logLen)
{
	DIR * dirp;
	struct dirent *entry;
	char *extPtr;
	bool retValue = true;
	char fullPath [2*PATHLEN+1];

	if (strlen(checkDir) < 1) return true;
	dirp = opendir(checkDir);
	if (dirp == NULL){
		sprintf(log, "Could not find number of files in the directory.\n");
		return false;
	}
	while ((entry = readdir(dirp)) != NULL) {
		if (entry->d_name[0] != '.'){ // want to skip . and ..
			extPtr = NULL;
			extPtr = strchr (entry->d_name, '.');
			if (extPtr == NULL || *extPtr != '.') {
				printf ("Do not know how to use the file %s to enforce constraints. Skipping it.\n", entry->d_name);
				continue;
			}
            strcpy (fullPath, checkDir);
            strcat (fullPath, "/");
            strcat (fullPath, entry->d_name);
            if (strcmp(extPtr, ".xsl") == 0){
            	if (!applyXSLT (fullPath)) {
            		if (strlen(fullPath)+50 < logLen){
            		    sprintf (log, "XSL sheet %s generated semantic error.\n", fullPath);
            		}
            		retValue = false;
            		break;
            	}
            }
		}
    }
	closedir(dirp);
	xsltCleanupGlobals();
	xmlCleanupParser();
	if (retValue == false) return retValue;

	// so far all is good: let us test syntax and semantics using YANG model if sepcified
	if (strlen(yangDir) < 1){
		// yang dir not specified
		return true;
	}
	dirp = opendir (yangDir);
	if (dirp == NULL){
		sprintf(log, "Could not open YANG directory. Could not check data store against Yang Model.\n");
		return false;
	}
	while ((entry = readdir(dirp)) != NULL) {
		extPtr = NULL;
		extPtr = strchr (entry->d_name, '.');
		if (extPtr == NULL || *extPtr != '.') {
					continue;
		}
	    strcpy (fullPath, entry->d_name);
	    if (strcmp(extPtr, ".yang") == 0){
	    	int len = strlen (entry->d_name);
	    	// check if base part of file before '.' matches data store name
	    	fullPath[len-5] = '\0';
	    	if (strcmp (fullPath, name) != 0){
	    		printf ("Yang model in %s is not applicable to this data store: %s\n", entry->d_name, name);
	    		break;
	    	}
	    	log[0] = '\0';
	        if (!applyYang (log, logLen)) {
	            strcat (log, "Yang Model generated semantic error.\n");
	            retValue = false;
	        }
	        break;
	    }
	}
	closedir(dirp);
	return retValue;
}

bool
DataStore::applyYang (char *log, int logLen)
{
	// on Ubuntu, the command 'getconf ARG_MAX' gives the max size of a command which is very large ~ 2097152
	// But we will use a smaller limit below
	char command [PATHLEN*4 + 500];
	FILE *fd;
	char localPath [PATHLEN *2];
	bool retValue;
	int  numErrors, count;

	// save DOM in a file to run Makefile on it
	strcpy (localPath, yangDir);
	strcat (localPath, "/");
	strcat (localPath, name);
	strcat (localPath, ".xml");
	if ((fd = fopen (localPath, "w")) == NULL){
		sprintf (log, "Unable to write DOC to an XML file.\n");
		return false;
	}
    xmlDocDump (fd, doc);
    fclose (fd);
    // Apply Makefile
    sprintf (command, "cp ./dsdlMakefile %s/Makefile;", yangDir);
	//printf ("command to execute is : '%s'\n", command);
	system (command);
	sprintf (command, "cd %s; make BASE=%s > result1;", yangDir, name);
	//printf ("command to execute is : '%s'\n", command);
	system (command);
	sprintf (command, "cd %s;  grep -E 'error|Failed|Error' result1 | wc -l > result;", yangDir);
	//printf ("command to execute is : '%s'\n", command);
	system (command);
	// First read number of erros from the file result.
	strcpy (localPath, yangDir);
	strcat (localPath, "/");
	strcat (localPath, "result");
	if ((fd = fopen (localPath, "r")) == NULL){
		sprintf (log, "Unable to generate results in the process of applying YANG.\n");
		retValue = false;
	} else {
		count = fscanf (fd, "%d", &numErrors);
		fclose (fd);
		if (count != 1){
			sprintf (log, "Unable to generate results in the process of applying YANG.\n");
			retValue = false;
		} else if (numErrors > 0){
			retValue = false;
			// open result to read errors and put them in 'log'
			strcat (localPath, "1");
			if ((fd = fopen (localPath, "r")) == NULL){
				sprintf (log, "There are %d number of erros, but could not read the error text.\n", numErrors);
			} else {
				int i;
				// file containing errors is open - read it
				i = fread (log, 1, logLen-1, fd );
				fclose (fd);
				if (i == 0){
					sprintf (log, "There are %d number of errors, but could not read the error text.\n", numErrors);
				} else {
					log[i] = '\0';
				}
			}

		} else {
			sprintf (log, "No errors.\n");
			retValue = true;
		}
	}
	sprintf (command, "cd %s; make clean BASE=%s;", yangDir, name);
	system (command);
	sprintf (command, "cd %s; rm -f Makefile;", yangDir);
	system (command);
	return retValue;
}

bool
DataStore::applyXSLT (char *filePath)
{
	xmlDocPtr res = NULL;
	const char *params[16+1];
	int numparams = 0;
	xmlChar xpathExpr [10] = "/ok";
	xmlXPathObjectPtr xpathObj;
	bool retValue = true;
	xmlNodeSetPtr nodes_local;

	params[numparams] = NULL;
	xsltStylesheetPtr stylesheet;
	stylesheet = xsltParseStylesheetFile((const xmlChar *)filePath);
	res = xsltApplyStylesheet(stylesheet, doc, params);
	if (res == NULL){
		printf ("NULL result generated by applying XSLT %s on data store.\n", filePath);
		return false;
	}
	xpathObj = getNodeSet (res, xpathExpr);
	if (xpathObj){
		nodes_local = xpathObj->nodesetval;
		if (nodes_local->nodeNr < 1) retValue = false; // <ok> node not found
		xmlXPathFreeObject (xpathObj);
	} else {
		retValue = false;
	}
	xsltFreeStylesheet(stylesheet);
	xmlFreeDoc (res);
	return retValue;
}

xmlXPathObjectPtr
DataStore:: getNodeSet (xmlDocPtr docm, xmlChar *xpath)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	if(!docm) {
		//sprintf (log, "Doc is NULL, why?");
		return NULL;
	}
	context = xmlXPathNewContext(docm);
	if (context == NULL) {
		//sprintf(log, "Error in xmlXPathNewContext");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		//sprintf(log, "Error in xmlXPathEvalExpression");
	}
	return result;
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
DataStore::deleteNodes(struct ClientInfo *cinfo, xmlChar *xpath, char *log, int logLen)
{
	int retValue;
	xmlXPathObjectPtr objset;
	xmlDocPtr orgDoc;

	log[0] = '\0';
	if (lockDS(cinfo)){
		sprintf (log, "Error in locking data store");
		return -1;
    }
	orgDoc = doc;
	// create a duplicate of orgDoc
    doc = xmlCopyDoc (orgDoc, 1);
	if (doc == NULL){
		doc = orgDoc;
		sprintf (log, "Error in creating a duplicate DOM tree.");
		unlockDS (cinfo);
		return -1;
	}

	objset = getNodeSet (xpath, log);
	if (!objset){
		xmlFreeDoc (doc);
		doc = orgDoc;
		// error or result is an empty set
		if (strcmp(log, "No result") == 0){
			retValue = 0;
		}
	} else {
		// update chosen nodes
	    retValue = deleteSelectedNodes (objset->nodesetval);
	    if (!retValue){
	    	sprintf (log, "No result");
	    	xmlFreeDoc (doc);
	    	doc = orgDoc;
	    } else {
	    	// doc modified - apply contraints
	    	log[0] = '\0';
	    	if (applyConstraints (log, logLen)){
	    		xmlFreeDoc (orgDoc);
	    	} else {
	    		xmlFreeDoc (doc);
	    		doc = orgDoc;
	    		strcat (log, " One or more contraint failed. No changes made.");
	    		retValue = -1;
	    	}
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
DataStore::updateNodes (struct ClientInfo *cinfo, xmlChar *xpath, xmlChar *newValue, char *log, int logLen)
{
	int retValue = -1;
	xmlXPathObjectPtr objset;
	xmlDocPtr orgDoc;

	log[0] = '\0';
	if (lockDS(cinfo)){
		sprintf (log, "Error in locking data store");
		return -1;
    }
	orgDoc = doc;
    // create a duplicate of orgDoc
	doc = xmlCopyDoc (orgDoc, 1);
	if (doc == NULL){
		doc = orgDoc;
		sprintf (log, "Error in creating a duplicate DOM tree.");
		unlockDS (cinfo);
		return -1;
	}
	objset = getNodeSet (xpath, log);
	if (!objset){
		xmlFreeDoc (doc);
		doc = orgDoc;
		// error or result is an empty set
		if (strcmp(log, "No result") == 0){
			retValue = 0;
		}
	} else {
		// update chosen nodes
	    retValue = udpateSelectedNodes (objset->nodesetval, newValue);
	    if (retValue == 0){
	    	xmlFreeDoc (doc);
	    	doc = orgDoc;
	    	sprintf (log, "No result");
	    } else if (retValue < 0){
	    	xmlFreeDoc (doc);
	    	doc = orgDoc;
	    	sprintf (log, "Error in adding nodes");
	    } else {
	    	// doc successfully modified - apply contraints
	    	if (applyConstraints(log, logLen)){
	    		xmlFreeDoc (orgDoc);
	    	} else {
	    		xmlFreeDoc (doc);
	    		doc = orgDoc;
	    		retValue = -1;
	    		strcat (log, "One or more constraints failed. No changes made.\n");
	    	}
	    }
	}

	if(unlockDS(cinfo)){
		sprintf (log, "Error in unlocking data store, but %d values in data store modified. Inconsistant state reached.", retValue);
		retValue = -1;
	}
    return retValue;
}

int
DataStore::addNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *nodeSetXML, char *log, int logLen)
{
	xmlNodePtr parent, newNode;
	char *newNodesStrWrapped;
	xmlDocPtr newDoc;
	xmlXPathObjectPtr objset;
	xmlNodeSet *nodeSet;
	int i;
	xmlNode *cur_node;
	int count = 0;
	xmlDocPtr orgDoc;

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
		xmlFreeDoc (newDoc);
		return -1;
	}

	orgDoc = doc;
	// create a duplicate of orgDoc
    doc = xmlCopyDoc (orgDoc, 1);
	if (doc == NULL){
		doc = orgDoc;
		sprintf (log, "Error in creating a duplicate DOM tree.");
		xmlFreeDoc (newDoc);
		unlockDS (cinfo);
		return -1;
	}

	// apply xpath and get nodeset
	objset =  getNodeSet (xpath, log);
	if (!objset){
		xmlFreeDoc (doc);
		doc = orgDoc;
		xmlFreeDoc (newDoc);
		unlockDS(cinfo);
		return 0;
	}
	nodeSet = objset->nodesetval;
	if (nodeSet == NULL || xmlXPathNodeSetIsEmpty(nodeSet)){
		xmlFreeDoc (doc);
		doc = orgDoc;
		xmlFreeDoc (newDoc);
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
	xmlFreeDoc (newDoc);
	// check constraints
	if (applyConstraints (log, logLen)){
		xmlFreeDoc (orgDoc);
	} else {
		xmlFreeDoc (doc);
		doc = orgDoc;
		count = -1;
		strcat (log, "One or more constraints failed. No changes done to Data Store.\n");
	}
	// for testing dump doc
	// xmlDocDump (stdout, doc);
	unlockDS(cinfo);
	return count;
}
