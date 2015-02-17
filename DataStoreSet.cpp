/*
 * DataStoreSet.cpp
 *
 * License : Apache 2.0
 *
 *  Created on: Jan 31, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

// DO NOT USE GLOBAL LOGGING MECHANISM IN THIS FILE

#include <string.h>
#include "DataStoreSet.h"


DataStoreSet::DataStoreSet() {
	dataStoreList = NULL;
	count = 0;
	maxNumber = 0;

}

DataStoreSet::~DataStoreSet()
{
   int i;
   for (i=0; i < maxNumber; i++){
	   delete dataStoreList[i];
   }
   delete dataStoreList;
}

bool
DataStoreSet::initialize(int number)
{
	int i;

	dataStoreList = new DataStore *[number];
	if (dataStoreList == NULL) return false;
	maxNumber = number;
	for (i = 0; i < maxNumber; i++){
		dataStoreList[i] = NULL;
	}
	return true;
}

bool
DataStoreSet:: addDataStore (char *name, char *inFile, char *xsdDir, char *xsltDir)
{
	if (count == maxNumber) {
		printf ("Limit of %d data stores reached. Can not add more data stores.\n", maxNumber);
		return false;
	}
	if(getDataStore(name)){
		printf ("Duplicate Data Store Names are not allowed: %s\n", name);
		return false;
	}
    dataStoreList[count] = new DataStore(name, inFile, xsdDir, xsltDir);
    if (dataStoreList[count] == NULL || !dataStoreList[count]->initialize()){
    	delete dataStoreList[count];
    	dataStoreList[count] = NULL;
    	printf ("Error in creating a new data store: %s\n", name);
    	return false;
    }
    count++;
    return true;
}

DataStore *
DataStoreSet::getDataStore (char *name)
{
	int i;
	for (i=0; i < count; i++){
		if (strcmp(name, dataStoreList[i]->name) == 0){
			return dataStoreList[i];
		}
	}
	return NULL;
}

