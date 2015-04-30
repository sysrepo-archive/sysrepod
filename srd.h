/*
 * srd.h
 *
 * LICENSE: Apache 2.0
 *
 *  Created on: Feb 5, 2015
 *      Author: Niraj Sharma
 *      Copyright Cisco Systems, Inc.
 *      All rights reserved.
 */

/* SysrepoD server maintains a set of arbitrarily large DOM trees representing configuration data for devices.
 * The north-side applications (Management Applications) and south-side daemons (running in switches and routers),
 * both can access SysrepoD to get or set values stored in DOM trees concurrently. Operational Data is maintained
 * by south-side Daemons and is not maintained by SysrepoD. If a north-side applications desires to get Operational
 * Data, it can send a request to SysrepoD and it will in-turn get the data from daemons to forward to north-side
 * applications.
 *
 * This file describes the API that can be used by north-side applications and south-side daemons to communicate with
 * SysrepoD or as a set of utility functions.
 *
 */
#ifndef SRD_H_
#define SRD_H_

#include <libxml/xpathInternals.h>
#define bool int
#define true 1
#define false 0

#define SRD_DEFAULTSERVERPORT 3500

/***********************************************
 * Function : srd_getNodeSet
 *
 * Description: It is a utility function that applies the given XPath expression on a DOM tree. On error,
 * a message is provided through the parameter 'log'.
 *
 * Parameters:
 * 		doc   - The XML DOM tree on which the XPath is to be applied
 * 		xpath - The XPath expression to be applied on 'doc'
 * 		log   - Place to printf error messages
 *
 * Return Value: Pointer to a set of XPath Objects that conatains Node Set. Returns NULL if there is an
 * error or the computed result is a NULL set.
 ************************************************/
xmlXPathObjectPtr srd_getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log);

/***********************************************
 * Function : srd_printElementSet
 *
 * Description: It is a utility function that serializes a NodeSet in a string form in the provided buffer.
 * If buffer is not large enough, it extends it. The caller needs to free the buffer after use.
 *
 * Parameters:
 * 		doc     - The XML DOM tree to which the NodeSet belongs
 * 		nodeSet - The NodeSet to be serialized
 * 		printBuffPtr  - Pointer to the address of an allocated buffer to store NodeSet contents
 * 		printBuffSize - Sice of the allocated buffer
 *
 * Return Value: Returns -1 on error and the length of generated string on success.
 *
 ************************************************/
int srd_printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize);

/***********************************************
 * Function : srd_getFirstNodeValue
 *
 * Description: It is a utility function that applies the given XPath expression on a DOM tree and
 * returns the value of the first node.
 *
 * Parameters:
 * 		doc   - The XML DOM tree on which the XPath is to be applied
 * 		xpath - The XPath expression to be applied on 'doc'
 *
 * Return Value: Pointer to the value of the first Node in the result of applying XPath.  Returns NULL if there is an
 * error or the computed result is a NULL set.
 *
 ************************************************/
xmlChar *srd_getFirstNodeValue (xmlDocPtr doc, xmlChar *xpath);

/***********************************************
 * Function : srd_setServer
 *
 * Description: It sends a message to the server.
 *
 * Parameters:
 * 		sockfd   - An already opened socket connected to the server
 * 		message  - The message to be sent to the server
 * 		msgSize  - The length of the message
 *
 * Return Value: Returns false on error and true on success
 *
 ************************************************/
bool srd_sendServer (int sockfd, char *message, int msgSize);

/***********************************************
 * Function : srd_recvServer
 *
 * Description: It reads a message from the server in the provided buffer. If buffer is small for the read
 * content, it is extended automatically. The caller needs to free the buffer after use.
 *
 * Parameters:
 * 		sockfd   - An already opened socket connected to the server
 * 		buffPtr  - An already allocated buffer.
 * 		buffSize - The length of the buffer
 *
 * Return Value: Returns 0 on error and the size of message on success
 *
 ************************************************/
int  srd_recvServer (int sockfd, char **buffPtr, int *buffSize);

/***********************************************
 * Function : srd_connect
 *
 * Description: It connects to the server and creates a TCP/IP socket.
 *
 * Parameters:
 * 		serverIP   - IP address of the server
 * 		serverPort - The listening server port
 * 		sockfd     - The created socket that is connected to the server
 *
 * Return Value: Returns 0 on error and 1 on success
 *
 ************************************************/
int  srd_connect (char *serverIP, int serverPort, int *sockfd);

/***********************************************
 * Function : srd_setDataStore
 *
 * Description: It tells server that this client will use the specified Data Store (represented by a DOM tree in server).
 * A client can use only one Data Store at a time but can use a different one at any time.
 * All subsequent commands are assumed to be in the context of Data Set set using this command.
 * One Data Store may be used by many clients concurrently.
 *
 * Parameters:
 * 		sockfd   - An already opened socket connected to the server
 * 		dsname   - The name of the Data Store to be used for this client
 *
 * Return Value: Returns 0 on error and 1 on success
 *
 ************************************************/
int  srd_setDataStore (int sockfd, char *dsname);

/***********************************************
 * Function : srd_disconnect
 *
 * Description: It breaks the connection to the server. The server keeps running.
 *
 * Parameters:
 * 		sockfd   - An already opened socket connected to the server
 *
 * Return Value: void
 *
 ************************************************/
void srd_disconnect (int sockfd);

/***********************************************
 * Function : srd_terminateServer
 *
 * Description: It breaks the connection to the server and also stops the server.
 *
 * Parameters:
 * 		sockfd   - An already opened socket connected to the server
 *
 * Return Value: void
 *
 ************************************************/
void srd_terminateServer  (int sockfd);

/***********************************************
 * Function : srd_applyXPath
 *
 * Description: It applies the given XPath expression on the Data Store already set for this client. The
 * serialized result in string form is returned.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 * 		xpath  - The XPath expression to be applied
 * 		value  - Used to return the result. After use, need to free it. It can be NULL on error or if
 * 		         the result of XPath is an empty set.
 *
 * Return Value: void
 *
 ************************************************/
void srd_applyXPath (int sockfd, char *xpath, char **value);

/***********************************************
 * Function : srd_lockDataStore
 *
 * Description: It locks the Data Store being used by the client. Only this client can modify or read the
 * Data Store.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 *
 * Return Value: Returns 1 on success and 0 on failure.
 *
 ************************************************/
int  srd_lockDataStore (int sockfd);

/***********************************************
 * Function : srd_unlockDataStore
 *
 * Description: It unlocks the Data Store being used by the client.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 *
 * Return Value: Returns 1 on success and 0 on failure.
 *
 ************************************************/
int  srd_unlockDataStore (int sockfd);

/***********************************************
 * Function : srd_updateNodes
 *
 * Description: It assigns a new value to all nodes selected by the given XPath.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 * 		xpath  - The XPath expression to select nodes whose values is to be changed
 * 		value  - The new value to be assigned to the nodes seleted by the XPath expression
 *
 * Return Value: The number of nodes that got modified. Returns -1 on error.
 *
 ************************************************/
int  srd_updateNodes (int sockfd, char *xpath, char *value);

/***********************************************
 * Function : srd_addNodes
 *
 * Description: It adds a subtree to all nodes selected by the given XPath.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 * 		xpath  - The XPath expression to select nodes where the new sub-tree is to be added
 * 		value  - An XML string representing a set of nodes, e.g. "<street>1 Infinity Loop</street><city>Cupertino</city><state>CA</state>".
 * 		         There is no need to have a root element.
 *
 * Return Value: The number of nodes where the new sub-tree was added. Returns -1 on error.
 *
 ************************************************/
int  srd_addNodes (int sockfd, char *xpath, char *value);

/***********************************************
 * Function : srd_deleteNodes
 *
 * Description: It deletes subtrees selected by the given XPath.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 * 		xpath  - The XPath expression to select nodes to be deleted
 *
 * Return Value: The number of nodes deleted. Returns -1 on error.
 *
 ************************************************/
int srd_deleteNodes (int sockfd, char *xpath);

/***********************************************
 * Function : srd_createDataStore
 *
 * Description: It creates a new Data Store at the server.
 *
 * Parameters:
 * 		sockfd - The socket connected to the server
 * 	    name   - The name of the Data Store to be created
 * 		value  - The XML to be used to built the DOM tree for the Data StoreUsed to return the result.
 * 		xsdDir - The syntactic constraints for the Data Store
 * 		xsltDir- The semantic constraints for the Data Store
 *
 * Return Value: 1 on success and 0 on failure.
 *
 ************************************************/
int  srd_createDataStore (int sockfd, char *name, char *value, char *xsdDir, char *xsltDir);

/***********************************************
 * Function : srd_listDataStores
 *
 * Description: It retrieves the list of all Data Stores at the server.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		result  - The list of all Data Stores. Need to free it after use.
 *
 * Return Value: On success returns true and false on failure.
 *
 ************************************************/
bool srd_listDataStores (int sockfd, char **result);

/***********************************************
 * Function : srd_deleteDataStore
 *
 * Description: It deletes a Data Store at the server.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The Data Store to be deleted.
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_deleteDataStore (int sockfd, char *name);

/***********************************************
 * Function : srd_copyDataStore
 *
 * Description: It copies one data store on another one. Both data store must exist before this call.
 * For example, if you want to copy 'configuration' data store on 'runtime', you can use this call.
 * The data store being modified must not be in LOCKED state.
 *
 * Parameters:
 * 		sockfd   - The socket connected to the server
 * 		fromName - The Data Store to be copied from.
 * 		toName   - The Data Store over which the copy will be made
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_copyDataStore (int sockfd, char *fromName, char *toName);

/***********************************************
 * Function : srd_createOpDataStores
 *
 * Description: It creates a new Operational Data Store at the server. Later on a south-side client (daemon)
 * may become its owner so that any queries for this Operational Data Store at the server get forwarded to
 * its owner.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The name of the new Operational Data Store
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_createOpDataStore (int sockfd, char *name);

/***********************************************
 * Function : srd_deleteOpDataStore
 *
 * Description: It deletes an Operational Data Store at the server even if some other client is its owner.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The name of the Operational Data Store to be deleted
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_deleteOpDataStore (int sockfd, char *name);

/***********************************************
 * Function : srd_listOpDataStores
 *
 * Description: It retrieves the list of all Operational Data Stores at the server.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		result  - The list of all Operational Data Stores. Need to free it after use.
 *
 * Return Value: On success returns true and false on failure.
 *
 ************************************************/
bool srd_listOpDataStores (int sockfd, char **result);

/***********************************************
 * Function : srd_listMyUsageOpDataStores
 *
 * Description: It retrieves the list of all Operational Data Stores at the server being used by this client.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		result  - The list of all Operational Data Stores being used by this client. Need to free it after use.
 *
 * Return Value: On success returns true and false on failure.
 *
 ************************************************/
bool srd_listMyUsageOpDataStores (int sockfd, char **result);

/***********************************************
 * Function : srd_useOpDataStore
 *
 * Description: It informs the server that this client is the owner of the named Operational Data Store.
 * One client may be the owner of many Operational Data Stores, but one Operational Data Store can be owned
 * by only one client.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The name of the Operational Data Store that this client wants to own
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_useOpDataStore (int sockfd, char *name);

/***********************************************
 * Function : srd_stopUsingOpDataStore
 *
 * Description: It informs the server that this client is not the owner of the named Operational Data Store.
 * If the 'name' parameter is NULL, the client is removed as the owner of all Operational Data Stores that it
 * currently owns.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The name of the Operational Data Store that this client does not want to own
 *
 * Return Value: On success returns 1 and 0 on failure.
 *
 ************************************************/
int  srd_stopUsingOpDataStore (int sockfd, char *name);

/***********************************************
 * Function : srd_applyXPathOpDataStore
 *
 * Description: It applies the given XPath on an Operational Data Store and returns the results. It first finds
 * the owner of the store and then requests it to compute the result. This call is typically made by a north-side
 * client and the result is computed by a south-side client (daemon). The server is just a pass through. Need to free
 * the result after use.
 *
 * Parameters:
 * 		sockfd  - The socket connected to the server
 * 		name    - The name of the Operational Data Store on which the XPath is to be applied
 * 		xpath   - The xpath expression to be applied to the store
 * 		value   - The output parameter to return results. Need to free it after use. The returned value
 * 		          may be NULL on error or in case the result is an empty set.
 *
 * Return Value: void
 *
 ************************************************/
void srd_applyXPathOpDataStore (int sockfd, char *opDataStoreName, char *xpath, char **value);

/***********************************************
 * Function : srd_registerClientSocket
 *
 * Description: It informs the server that this client may be the owner of a set of Operational Data Stores.
 * To compute the result of applying XPaths on its Operational Data Stores, the server should use the
 * information provided in this call to forward the requests to this client.
 *
 * Parameters:
 * 		sockfd      - The socket connected to the server
 * 		myIPAddress - Client's IP Address
 * 		myPort      - The listening port of the client
 *
 * Return Value: On success returns true and false on failure.
 *
 ************************************************/
bool srd_registerClientSocket (int sockfd, char *myIPAddress, int myPort);

/***********************************************
 * Function : srd_DOMHandleXPath
 *
 * Description: It computes the result of applying the given XPath on the given DOM tree. It serializes the results in the form
 * of an XML string and sends the result on the given socket.
 *
 * Parameters:
 * 		sockfd      - The socket connected to the server
 * 		doc         - DOM Tree on which the XPath is to be applied
 * 		xpathExpr   - The XPath expression to be applied on the given DOM Tree
 *
 * Return Value: void
 *
 ************************************************/
void srd_DOMHandleXPath (int sockfd, xmlDocPtr ds, xmlChar *xpathExpr);

/***********************************************
 * Function : srd_registerClientSignal
 *
 * Description: It registers a PID and a signal with SysrepoD server. When the DOM tree being used
 * by the calling client changes, the registered signal will be sent to the process identified by PID.
 *
 * Parameters:
 * 		sockfd      - The socket connected to the server
 * 		clientPID   - PID of the process that should receive a signal if DOM tree changes
 * 		signalType  - The type of the signal that is to be sent to the process identified by PID.
 *
 * Return Value: true on success and false otherwise.
 *
 ************************************************/
bool srd_registerClientSignal (int sockfd, pid_t clientPID, int signalType);

/***********************************************
 * Function : srd_applyXSLT
 *
 * Description: It applies the given XSLT on the Data Store already set for this client. The
 * serialized result in string form is returned.
 *
 * Parameters:
 * 		sockfd   - The socket connected to the server
 * 		xsltTexT - The XSLT to be applied. NOTE: XSLT must be enclosed in CDATA clause.
 * 		value    - Used to return the result. After use, need to free it. It can be NULL on error or if
 * 		           the result of applying XSLT is an empty set.
 *
 * Return Value: void
 *
 ************************************************/
void srd_applyXSLT (int sockfd, char *xslt, char **value);

#endif /* SRD_H_ */
