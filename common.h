/*
 * License: Apache 2.0
 *
 * common.h
 *
 *  Created on: Jan 25, 2015
 *      Author: Niraj Sharma
 *      Cisco Systems, Inc.
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <sys/types.h>
#include "global.h"

class common {
private:
	static void addLogAndSpace (char *logline);
	static bool openNewLog (void);
public:
	common();
   ~common();
   static void LogMsg   (int logLevel, char * logline, bool fatal);
   static bool OpenNewLog ();
   static int  Do_mkdir (const char *path, mode_t mode);
   static int  Mkpath   (const char *path, mode_t mode);
   static void CloseLog (void);
   static int  FileCount (char *path);
   static bool IsDir (char *path);
   static bool SendMessage (int sock, char *message);
};

#endif /* COMMON_H_ */
