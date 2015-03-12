/*
 * OpDataStoreSet.cpp
 *
  * License : Apache 2.0
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common.h"
#include "OpDataStoreSet.h"
#include "ClientSet.h"

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
	char *newBuffer;
	int   buffSize = printBuffSize;

	if ((strlen (dsname) + 201) > buffSize){
		// The buffer size is too small to even print the error messages in this routine
		newBuffer = (char *)realloc (*printBuffPtr, strlen(dsname) + 201);
		if (newBuffer == NULL){
			sprintf (*printBuffPtr, "Unable to allocate space.");
			return false;
		} else {
			*printBuffPtr = newBuffer;
			buffSize = strlen(dsname) + 201;
		}
	}
	// Find the owner of the data store 'dsname'
	owner = findOwner (dsname);
	if (owner){
		if (!owner->clientSet->openBackConnection (owner)){
			sprintf (*printBuffPtr, "Unable to establish connection with the entity that manages the Operational Data store '%s'", dsname);
			return false;
		}
		if (common::SendMessage(owner->clientBackSock, commandXML)){
		   // read response from the owner
		   n = common::ReadSock (owner->clientBackSock, printBuffPtr, buffSize);
		   owner->clientSet->closeBackConnection (owner);
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

int
OpDataStoreSet:: addOpDataStore (char *name)
{
	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock op data store set.\n");
		return 0;
	}
	if (count == maxNumber) {
		printf ("Limit of %d op data stores reached. Can not add more op data stores.\n", maxNumber);
		pthread_mutex_unlock(&dsMutex);
		return 0;
	}
	if(getOpDataStore_noLock(name)){
		printf ("Duplicate Op Data Store Names are not allowed: %s\n", name);
		pthread_mutex_unlock(&dsMutex);
		return 2;
	}
    opDataStoreList[count] = new OpDataStore();
    if (opDataStoreList[count] == NULL || !opDataStoreList[count]->initialize(name)){
    	delete opDataStoreList[count];
    	opDataStoreList[count] = NULL;
    	printf ("Error in creating a new op data store: %s\n", name);
    	pthread_mutex_unlock(&dsMutex);
    	return 0;
    }
    count++;
    pthread_mutex_unlock(&dsMutex);
    return 1;
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
	char *list = NULL;
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
	int localcount = 0;
	if(pthread_mutex_lock(&dsMutex)){
		printf ("Unable to lock data store set.\n");
		return -1;
	}
	for (i=0; i < count; i++){
		if (opDataStoreList[i]->owner == cinfo){
			  opDataStoreList[i]->owner = NULL;
			  localcount++;
		}
	}
	pthread_mutex_unlock(&dsMutex);
	return localcount;
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




