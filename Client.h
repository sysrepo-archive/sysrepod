/*
 * Client.h
 *
 * License : Apache 2.0
 *
 *  Created on: Jan 30, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include "application.h"

class Client {
private:


public:
	Client();
	virtual ~Client();
	virtual int processCommand (char *command, char *outBuffer, int outBufferSize, struct clientInfo *cinfo);
};

#endif /* CLIENT_H_ */
