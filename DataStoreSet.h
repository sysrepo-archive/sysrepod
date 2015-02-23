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
	bool addDataStore (char *name, char *inFile, char *xsdDir, char *xsltDir);
	DataStore *getDataStore (char *name);
};

#endif /* DATASTORESET_H_ */
