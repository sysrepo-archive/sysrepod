/*
 * OpDataStoreSet.h
 *
 *  Created on: Mar 3, 2015
 *      Author: niraj
 */

#ifndef OPDATASTORESET_H_
#define OPDATASTORESET_H_

#include "OpDataStore.h"

class OpDataStoreSet {
private:
	pthread_mutex_t dsMutex;
	int count;
	int maxNumber;
	OpDataStore **opDataStoreList;
public:
	OpDataStoreSet();
	virtual ~OpDataStoreSet();

	bool initialize (int number);
	bool applyXPathOpDataStore (char *commandXML, char *dsname, char **printBuff, int printBuffSize);
	clientInfo *findOwner (char *dsname);
	bool addOpDataStore (char *name);
	char *getList (void);
	char *listMyUsage (clientInfo *cinfo);
	bool  deleteOpDataStore (char *name, clientInfo *cinfo);
	OpDataStore *getOpDataStore_noLock (char *name);
	bool removeOwner (char *name, clientInfo *cinfo);
	bool setOwner (char *name, clientInfo *cinfo);
	bool removeOwner (clientInfo *cinfo);
};

#endif /* OPDATASTORESET_H_ */
