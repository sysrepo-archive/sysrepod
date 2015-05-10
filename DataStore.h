/*
 * License: Apache 2.0
 *
 * DataStore.h
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef DATASTORE_H_
#define DATASTORE_H_

#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <pthread.h>

#include "application.h"

#define MAXDATASTORENAMELEN 200

class DataStore {
private:
	// Need a mutex to control access to the data store from multiple threads
	pthread_mutex_t dsMutex;
	char fileName[PATHLEN + 1];
	char checkDir [PATHLEN + 1]; // dir that contains XSD, DSDL, and XSLT files for
	                             // checking constraints
	int udpateSelectedNodes (xmlNodeSetPtr nodes, xmlChar *newValue);
	struct ClientInfo *lockedBy;

	xmlXPathObjectPtr getNodeSet (xmlChar *xpath, char *log);
	xmlXPathObjectPtr getNodeSet (xmlDocPtr doc, xmlChar *xpath);
	void removeChar(char *str, char ch);
public:
	// DOM treee that contains the data
	xmlDocPtr doc;
	char name[MAXDATASTORENAMELEN + 1];

	DataStore(char *filename, char *dsname, char * checkdir);
	DataStore (char *dsname);
	virtual ~DataStore();
	bool initialize (void);
	bool initialize (char *xml);
	int lockDS (struct ClientInfo *cinfo);
	int unlockDS(struct ClientInfo *cinfo);
	int applyXPath (struct ClientInfo *cinfo, xmlChar *xpath, char **printBuffPtr, int printBuffSize, int offset);
	int applyXSLT (struct ClientInfo *cinfo, char *xpath, char **printBuffPtr, int printBuffSize, int offset);
	int printElementSet (xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize, int initialOffset);
	int updateNodes (struct ClientInfo *cinfo, xmlChar *xpath, xmlChar *newValue, char *log);
	int printXPathAtomicResult (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize, int offset);
	int addNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *nodeSetXML, char *log);
	int deleteNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *log);
	int deleteSelectedNodes (xmlNodeSetPtr nodes);
	bool applyConstraints (void);
	bool applyXSLT (char *filePath);
};

#endif /* DATASTORE_H_ */
