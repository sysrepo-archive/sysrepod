/*
 * OpDataStoreSet.cpp
 *
 *  Created on: Mar 3, 2015
 *      Author: niraj
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common.h"
#include "OpDataStoreSet.h"

OpDataStoreSet::OpDataStoreSet() {
	count = maxNumber = 0;
	opDataStoreList = NULL;
}

OpDataStoreSet::~OpDataStoreSet()
{
	int i;

	pthread_mutex_destroy (&dsMutex);
	for (i=0; i < maxNumber; i++){
		   delete opDataStoreList[i];
	   }
	   delete opDataStoreList;
}

// do not use log in this member function, log system is not yet created.
bool
OpDataStoreSet::initialize (int number)
{
	int i;
   if (pthread_mutex_init(&dsMutex, NULL) != 0)
   {
		// Op Data store mutex init failed.
		printf ("Error: Failed to init Mutex for Op DataStore Set.\n");
		return false;
	}
	opDataStoreList = new OpDataStore *[number];
	if (opDataStoreList == NULL) {
		printf ("Error: Unable to allocate memory for op data store set.\n");
		return false;
	}
	maxNumber = number;
	for (i = 0; i < maxNumber; i++){
		opDataStoreList[i] = NULL;
	}
	return true;
}

bool
OpDataStoreSet::applyXPathOpDataStore (char *commandXML, char *dsname, char **printBuffPtr, int printBuffSize)
{
	clientInfo *owner;
	int n;
	char localBuff[200];

	// Find the owner of the data store 'dsname'
	owner = findOwner (dsname);
	if (owner){
		// First clean the read buffer of the owner's socket, there might be some left over bytes in there.
		// Do not want to read junk - Not doing it at this time
		// while ((n = recv(owner->sock, localBuff, 200, MSG_DONTWAIT )) > 0);

		if (common::SendMessage(owner->sock, commandXML)){
		   // read response from the owner
		   n = common::ReadSock (owner->sock, printBuffPtr, printBuffSize);
		   if (n){
			   return true;
		   } else {
			   sprintf (*printBuffPtr, "Unable to read response from the owner of the Operational Data Store '%s'", dsname);
			   return false;
		   }
		   return true;
		} else {
		   sprintf (*printBuffPtr, "Unable to send request to the owner of the Operational Data Store '%s'", dsname);
		   return false;
		}
	} else {
		sprintf (*printBuffPtr, "No client owns the Operational Data Store '%s'", dsname);
		return false;
	}
}

clientInfo *
OpDataStoreSet::findOwner (char *dsname)
{
	int i;
	if(pthread_mutex_lock(&dsMutex)){
	   printf ("Unable to lock op data store set.\n");
	   return NULL;
	}
	for (i=0; i < count; i++){
		if (strcmp(opDataStoreList[i]->name, dsname) == 0){
			pthread_mutex_unlock(&dsMutex);
			return (opDataStoreList[i]->owner);
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return NULL;
}

bool
OpDataStoreSet:: addOpDataStore (char *name)
{
	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock op data store set.\n");
		return false;
	}
	if (count == maxNumber) {
		printf ("Limit of %d op data stores reached. Can not add more op data stores.\n", maxNumber);
		pthread_mutex_unlock(&dsMutex);
		return false;
	}
	if(getOpDataStore_noLock(name)){
		printf ("Duplicate Op Data Store Names are not allowed: %s\n", name);
		pthread_mutex_unlock(&dsMutex);
		return false;
	}
    opDataStoreList[count] = new OpDataStore();
    if (opDataStoreList[count] == NULL || !opDataStoreList[count]->initialize(name)){
    	delete opDataStoreList[count];
    	opDataStoreList[count] = NULL;
    	printf ("Error in creating a new op data store: %s\n", name);
    	pthread_mutex_unlock(&dsMutex);
    	return false;
    }
    count++;
    pthread_mutex_unlock(&dsMutex);
    return true;
}

OpDataStore *
OpDataStoreSet::getOpDataStore_noLock (char *name)
{
	int i;
	for (i=0; i < count; i++){
		if (strcmp(name, opDataStoreList[i]->name) == 0){
			return opDataStoreList[i];
		}
	}
	return NULL;
}

char *
OpDataStoreSet::getList (void)
{
	int i;
	char *list;
	int requiredSize = 0;

	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock op data store set.\n");
		return NULL;
	}
	if (count == 0){
		pthread_mutex_unlock(&dsMutex);
		return NULL;
	}
	for (i = 0; i < count; i++){
		requiredSize = requiredSize + strlen (opDataStoreList[i]->name) + strlen ("<opDataStore></opDataStore>");
	}
	requiredSize = requiredSize + strlen ("<opDataStores></opDataStores>") + 10;
	list = (char *) malloc (requiredSize);
	if (list){
	   sprintf (list, "<opDataStores>");
	   for (i=0; i < count; i++){
		   strcat (list, "<opDataStore>");
		   strcat (list, opDataStoreList[i]->name);
		   strcat (list, "</opDataStore>");
	   }
	   strcat (list, "</opDataStores>");
	} else {
		printf ("Unable to allocate space for Op Data Store list.\n");
	}
	pthread_mutex_unlock(&dsMutex);
	return list;
}

char *
OpDataStoreSet::listMyUsage (clientInfo *cinfo)
{
	int i;
	char *list;
	int requiredSize = 0;

	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock op data store set.\n");
		return NULL;
	}
	if (count == 0){
		pthread_mutex_unlock(&dsMutex);
		return NULL;
	}
	for (i = 0; i < count; i++){
		requiredSize = requiredSize + strlen (opDataStoreList[i]->name) + strlen ("<opDataStore></opDataStore>");
	}
	requiredSize = requiredSize + strlen ("<opDataStores></opDataStores>") + 10;
	list = (char *) malloc (requiredSize);
	if (list){
	   sprintf (list, "<opDataStores>");
	   for (i=0; i < count; i++){
		   if (opDataStoreList[i]->owner == cinfo){
		      strcat (list, "<opDataStore>");
		      strcat (list, opDataStoreList[i]->name);
		      strcat (list, "</opDataStore>");
		   }
	   }
	   strcat (list, "</opDataStores>");
	} else {
		printf ("Unable to allocate space for Op Data Store list.\n");
	}
	pthread_mutex_unlock(&dsMutex);
	return list;
}

int
OpDataStoreSet::deleteOpDataStore (char *name, clientInfo *cinfo)
{
	int i;
	int retValue = 1;
	OpDataStore *ds;

	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock data store set.\n");
		return -1;
	}
	for (i=0; i < count; i++){
		if (strcmp(name, opDataStoreList[i]->name) == 0){
			if (opDataStoreList[i]->lockDS()){
				retValue = -1;
				break;
			}
			if (opDataStoreList[i]->owner == NULL || opDataStoreList[i]->owner == cinfo){
			   // remove from the list
			   ds = opDataStoreList[i];
			   opDataStoreList[i] = NULL;
			   // shift array up
			   if (i < count -1){
			      memmove (&(opDataStoreList[i]), &(opDataStoreList[i+1]), sizeof (OpDataStore *)* (count - i - 1));
			   }
               count --;
               ds->unlockDS();
               delete (ds);
               break;
			} else {
				printf ("Client does not own data store: can not delete it.\n");
				retValue = -1; // client does not own it, can not delete it
			}
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return retValue;
}

int
OpDataStoreSet::removeOwner (char *name, clientInfo *cinfo)
{
	int retValue = 0;
	int i;
	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock data store set.\n");
		return -1;
	}
	for (i=0; i < count; i++){
		if (strcmp(name, opDataStoreList[i]->name) == 0){
			if (opDataStoreList[i]->owner == cinfo){
			   opDataStoreList[i]->owner = NULL;
			   retValue = 1;
			   break;
			}
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return retValue;
}

int
OpDataStoreSet::removeOwner (clientInfo *cinfo)
{
	int i;
	int count = 0;
	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock data store set.\n");
		return -1;
	}
	for (i=0; i < count; i++){
		if (opDataStoreList[i]->owner == cinfo){
			  opDataStoreList[i]->owner = NULL;
			  count++;
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return count;
}

bool
OpDataStoreSet::setOwner (char *name, clientInfo *cinfo)
{
	int i;
	bool retValue = false;

	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock data store set.\n");
		return false;
	}
	for (i=0; i < count; i++){
		if (strcmp(name, opDataStoreList[i]->name) == 0){
			if (opDataStoreList[i]->owner == NULL){
				opDataStoreList[i]->owner = cinfo;
				retValue = true;
				break;
			}
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return retValue;
}




