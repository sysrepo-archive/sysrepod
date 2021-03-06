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
	int globalTransactionCounter;
public:
	DataStoreSet();
	virtual ~DataStoreSet();

	bool initialize (int number);
	bool addDataStoreFromFile (char *name, char *inFile, char *checkDir, char *yangDir);
	bool addDataStoreFromString (char *name, char *xml);
	DataStore *getDataStore (char *name);
	char *getList (void);
	int  deleteDataStore (struct ClientInfo *cinfo, ClientSet *cset, char *name);
	int copyDataStore (struct ClientInfo *cinfo, char *fromDS, char *toDS, char *printBuff, int printBuffSize);
	int getNextTransactionId (char *log);
};

#endif /* DATASTORESET_H_ */
