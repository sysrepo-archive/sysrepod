/*
 * OpDataStore.cpp
 *
 *  Created on: Mar 3, 2015
 *      Author: niraj
 */

#include <string.h>
#include "OpDataStore.h"

OpDataStore::OpDataStore()
{
	owner = NULL;
    name[0] = '\0';
}

OpDataStore::~OpDataStore()
{
	pthread_mutex_destroy (&dsMutex);
}

bool
OpDataStore::initialize (char *dsname)
{
	if (strlen(dsname) > MAXOPDATASTORENAMELEN){
		return false;
	}
	strcpy (name, dsname);
	if (pthread_mutex_init (&dsMutex, NULL) != 0)
	{
		// Op Data store mutex init failed.
		printf ("Error: Failed to init Mutex for Op DataStore.\n");
		return false;
	}
	return true;
}

// lock and unlock return 0 on success
int
OpDataStore::lockDS(void)
{
	return pthread_mutex_lock(&dsMutex);
}

int
OpDataStore::unlockDS(void)
{
	return pthread_mutex_unlock(&dsMutex);
}

