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

#define MAXDATASTORENAMELEN 200

class DataStore {
private:
	// Need a mutex to control access to the data store from multiple threads
	pthread_mutex_t dsMutex;
	char fileName[PATHLEN + 1];
	char xsdDir [PATHLEN + 1];
	char xsltDir[PATHLEN +1];
	int udpateSelectedNodes (xmlNodeSetPtr nodes, xmlChar *newValue);

	xmlXPathObjectPtr getNodeSet (xmlChar *xpath, char *log);
public:
	// DOM treee that contains the data
	xmlDocPtr doc;
	char name[MAXDATASTORENAMELEN + 1];

	DataStore(char *filename, char *dsname, char * xsddir, char *xsltdir);
	DataStore (char *dsname);
	virtual ~DataStore();
	bool initialize (void);
	bool initialize (char *xml);
	int lockDS (void);
	int unlockDS(void);
	int applyXPath (xmlChar *xpath, char **printBuffPtr, int printBuffSize, int offset);
	int printElementSet (xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize, int initialOffset);
	int updateNodes (xmlChar *xpath, xmlChar *newValue, char *log);
	int printXPathAtomicResult (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize, int offset);
};

#endif /* DATASTORE_H_ */
