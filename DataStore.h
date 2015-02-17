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

	xmlXPathObjectPtr getNodeSet (xmlChar *xpath, char *log);
public:
	// DOM treee that contains the data
	xmlDocPtr doc;
	char name[MAXDATASTORENAMELEN + 1];

	DataStore(char *filename, char *dsname, char * xsddir, char *xsltdir);
	virtual ~DataStore();
	bool initialize (void);
	void lockDS (void);
	void unlockDS(void);
};

#endif /* DATASTORE_H_ */
