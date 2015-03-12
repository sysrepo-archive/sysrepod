/*
 * ClientSRD.h
 *
 * License : Apache 2.0
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef CLIENTSRD_H_
#define CLIENTSRD_H_

#include "Client.h"
#include <libxml/xpathInternals.h>

class Client_SRD: public Client {
public:
	Client_SRD();
	virtual ~Client_SRD();
	virtual int processCommand (char *command, char *outBuffer, int outBufferSize, struct clientInfo *cinfo);
	int printElementSet (xmlDocPtr doc, xmlNodeSet *nodeSet, char **printBuffPtr, int printBuffSize);
	int applyXPath(xmlDocPtr doc, xmlChar *xpath, char **printBuffPtr, int printBuffSize);
	xmlXPathObjectPtr getNodeSet (xmlDocPtr doc, xmlChar *xpath, char *log);

};

#endif /* CLIENTSRD_H_ */
