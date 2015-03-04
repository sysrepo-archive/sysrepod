/*
 * OpDataStore.h
 *
 *  Created on: Mar 3, 2015
 *      Author: niraj
 */

#ifndef OPDATASTORE_H_
#define OPDATASTORE_H_

#include "application.h"
#include "global.h"
#include "Client.h"

class OpDataStore {
private:
	// Need a mutex to control access to the op data store from multiple threads
    pthread_mutex_t dsMutex;

public:
	clientInfo *owner;
	char name[MAXOPDATASTORENAMELEN+1];

	OpDataStore();
	virtual ~OpDataStore();

	bool initialize (char *dsname);
	int lockDS (void);
	int unlockDS(void);
};

#endif /* OPDATASTORE_H_ */
