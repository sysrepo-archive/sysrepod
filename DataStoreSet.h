/*
 * DataStoreSet.h
 *
 * License : Apache 2.0
 *
 *  Created on: Jan 31, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef DATASTORESET_H_
#define DATASTORESET_H_

#define PATHLEN 1024

#include "DataStore.h"
#include "ClientSet.h"

class DataStoreSet {
private:
	pthread_mutex_t dsMutex;
	int count;
	int maxNumber;
	DataStore **dataStoreList;
	DataStore *getDataStore_noLock (char *name);
public:
	DataStoreSet();
	virtual ~DataStoreSet();

	bool initialize (int number);
	bool addDataStoreFromFile (char *name, char *inFile, char *xsdDir, char *xsltDir);
	bool addDataStoreFromString (char *name, char *xml);
	DataStore *getDataStore (char *name);
	char *getList (void);
	int  deleteDataStore (struct ClientInfo *cinfo, ClientSet *cset, char *name);
};

#endif /* DATASTORESET_H_ */
