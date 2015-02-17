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

//xmlDocPtr pDoc = ... // your xml docuemnt

//xmlCharPtr psOutput;
//int iSize;
//xmlDocDumpFormatMemoryEnc(pDoc, &psOutput, &iSize, "UTF-8", 1);

// psOutput should point to the string.

// Don't forget to free the memory.
//xmlFree(psOutput);
// xmlSaveFormatFileEnc(argc > 1 ? argv[1] : "-", doc, "UTF-8", 1);
//int	xmlDocDump			(FILE * f,
					 // xmlDocPtr cur)


xmlXPathObjectPtr
DataStore:: getNodeSet (xmlChar *xpath, char *log)
{
	xmlXPathContextPtr context;
	xmlXPathObjectPtr result;

	if(!doc) return NULL;
	context = xmlXPathNewContext(doc);
	if (context == NULL) {
		sprintf(log, "Error in xmlXPathNewContext\n");
		return NULL;
	}
	result = xmlXPathEvalExpression(xpath, context);
	xmlXPathFreeContext(context);
	if (result == NULL) {
		sprintf(log, "Error in xmlXPathEvalExpression\n");
		return NULL;
	}
	if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
		xmlXPathFreeObject(result);
        sprintf(log, "No result\n");
		return NULL;
	}
	return result;
}

void
DataStore::lockDS(void)
{
	pthread_mutex_lock(&dsMutex);
}

void
DataStore::unlockDS(void)
{
	pthread_mutex_unlock(&dsMutex);
}
