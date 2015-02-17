/*
 * ClientSRD.h
 *
 *  Created on: Feb 4, 2015
 *      Author: niraj
 */

#ifndef CLIENTSRD_H_
#define CLIENTSRD_H_

#include "Client.h"

class Client_SRD: public Client {
public:
	Client_SRD();
	virtual ~Client_SRD();
	virtual int processCommand (char *command, char *outBuffer, int outBufferSize, struct clientInfo *cinfo);
};

#endif /* CLIENTSRD_H_ */
