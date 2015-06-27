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

#include "application.h"

#define MAXDATASTORENAMELEN 200

/* Do not change the value 500 below as it is based on the length of outBuff length in code at
 * various places. Where ever you use this value, make sure that the content to be put is
 * limited to this value.
 */
#define MAXYANGERRORLEN 500

class DataStore {
private:
	// Need a mutex to control access to the data store from multiple threads
	pthread_mutex_t dsMutex;
	char fileName[PATHLEN + 1];
	char checkDir [PATHLEN + 1]; // dir that contains XSD, DSDL, and XSLT files for
	                             // checking constraints
	char yangDir [PATHLEN + 1]; // dir that contains Yang filed.
	int udpateSelectedNodes (xmlNodeSetPtr nodes, xmlChar *newValue);

	xmlXPathObjectPtr getNodeSet (xmlChar *xpath, char *log);
	xmlXPathObjectPtr getNodeSet (xmlDocPtr doc, xmlChar *xpath);
	void removeChar(char *str, char ch);
public:
	struct ClientInfo *lockedBy;
	// DOM treee that contains the data
	xmlDocPtr doc;
	// If a transaction is in progress, we need to save the pre-transaction dom tree in case transaction get aborted.
	xmlDocPtr preTransactionStartDoc;
	// To rollback last committed transaction, we need to save the pre-LastTransaction dom tree.
	xmlDocPtr preLastCommittedTransactionDoc;
	int lastCommittedTransactionId;
	char name[MAXDATASTORENAMELEN + 1];

	DataStore(char *filename, char *dsname, char * checkdir, char *yangDir);
	DataStore (char *dsname);
	virtual ~DataStore();
	bool initialize (void);
	bool initialize (char *xml);
	int lockDS (struct ClientInfo *cinfo);
	int startTransaction  (struct ClientInfo *cinfo, char *log, int logLen);
	int commitTransaction (struct ClientInfo *cinfo, char *log, int logLen);
	int abortTransaction (struct ClientInfo *cinfo, char *log, int logLen);
	int rollbackTransaction (struct ClientInfo *cinfo, int transID, char *log, int logLen);
	int unlockDS(struct ClientInfo *cinfo);
	int applyXPath (struct ClientInfo *cinfo, xmlChar *xpath, char **printBuffPtr, int printBuffSize, int offset);
	int applyXSLT (struct ClientInfo *cinfo, char *xpath, char **printBuffPtr, int printBuffSize, int offset);
	int printElementSet (xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize, int initialOffset);
	int updateNodes (struct ClientInfo *cinfo, xmlChar *xpath, xmlChar *newValue, ModifyOption mo, char *log, int logLen);
	int printXPathAtomicResult (xmlXPathObjectPtr objset, char **printBuffPtr, int printBuffSize, int offset);
	int addNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *nodeSetXML, ModifyOption mo, char *log, int logLen);
	int replaceNodes (struct ClientInfo *cinfo, xmlChar *xpath, char *nodeSetXML, ModifyOption mo, char *log, int logLen);
	int deleteNodes (struct ClientInfo *cinfo, xmlChar *xpath, ModifyOption mo, char *log, int logLen);
	int deleteSelectedNodes (xmlNodeSetPtr nodes);
	bool applyConstraints (char *log, int logLen);
	bool applyXSLT (char *filePath);
	bool applyYang (char *log, int logLen);
};

#endif /* DATASTORE_H_ */
