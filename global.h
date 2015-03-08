/*
 * global.h
 *
 * License: Apache 2.0
 *
 * Creator: Niraj Sharma
 * Cisco Systems, Inc.
 *
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <pthread.h>

#define bool int
#define true  1
#define false 0

#define LOGDIR "logs"
#define PATHLEN 1024
#define MAXCMDSIZE 1024
#define SA      struct sockaddr

// Every message will have message size as the first value. Its field width is given below
#define MSGLENFIELDWIDTH 7

// command OP CODES
#define ERROR_OPCODE 404

// General purpose buffer, size = 16 KBytes
#define LOGBUFFERSIZE 16*1024

#define INVALID_SOCK -1

#define MAXOPDATASTORENAMELEN 1025

#include "application.h"

class DataStoreSet;
class OpDataStoreSet;

#ifdef MAIN_C_
long MaxLogSize;
int  NumLogs;
FILE *LogFD = NULL;
const char MyName [] = "SysRepoD";  // Name of this process
char CWDPath    [PATHLEN + 1];
char LogFileName[PATHLEN + 1];
char LogLine    [PATHLEN + 1];  // Buffer used by MAIN thread to log messages.
char LogDir     [PATHLEN + 1];  // Dir where log files are created.
int  CurrLogSize;               // Log file is bigger. How much log-content resides in it.
long LogFileSize;               // Log file size will be bigger than the log content in it.
char LogBuffer [LOGBUFFERSIZE]; // Using this large log buffer to write log + large amount of empty space into logfile so
                                // that next write in logfile is more efficient.
pthread_mutex_t LogMutex;       // Mutex to control access to logging system.
int  ServerPort; // Listening Port number for the server
bool TerminateNow = false;
int  LogLevel = 10;   // Level of log, higher the number more log messages
int  MaxClients = 20; // Max number of clients that can connect to this server
int  MaxDataStores; // Max number of data store that this server can maintain
int  MaxOpDataStores;
DataStoreSet *DataStores = NULL;
OpDataStoreSet *OpDataStores = NULL;

#else

extern long MaxLogSize;
extern FILE *LogFD;
extern const char MyName [];
extern char CWDPath    [PATHLEN + 1];
extern char LogFileName[PATHLEN + 1];
extern char LogLine    [PATHLEN + 1];
extern char LogDir     [PATHLEN + 1];
extern int  NumLogs;
extern int  CurrLogSize;
extern long LogFileSize;
extern char LogBuffer [LOGBUFFERSIZE];
extern pthread_mutex_t LogMutex;
extern int  ServerPort;
extern bool TerminateNow;
extern int  LogLevel;
extern int  MaxClients;
extern int  MaxDataStores;
extern int  MaxOpDataStores;
extern DataStoreSet *DataStores;
extern OpDataStoreSet *OpDataStores;

#endif


#endif /* GLOBAL_H_ */
