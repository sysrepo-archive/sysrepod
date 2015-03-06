/*
 * mainSysRepoD.cpp
 *
 * License: Apache 2.0
 *
 *  Created on: Jan 25, 2015
 *      Creator: Niraj Sharma
 *      Cisco Systems, Inc.
 *
 * SysRepoD is a server that maintains configuration data for Network Devices
 * (e.g. Routers and Switches). It provides North and South communication channels
 * for Management Applications (e.g., NETCONF) and Application programmers
 * (implementers of Router and Switch software), respectively.
 */
#define MAIN_C_

#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>

#include "global.h"
#include "common.h"
#include "ClientSet.h"
#include "DataStoreSet.h"
#include "OpDataStoreSet.h"

/* TO DO: Add Signal Handlers */

void
createLogDir (void)
{
	bool ret;

	ret = common::Mkpath(LogDir, 0777);
	if (!ret) {
		fprintf(stderr, "%d: failed to create log directory (%d: %s)\n", (int) getpid(), errno, strerror(errno));
		exit (1);
	}
	// open first log file; If did not get created, logging will stop
	common::OpenNewLog ();
}

void
applyDefaultSettings(void)
{
	strcpy (LogDir, LOGDIR);
	MaxLogSize = 1024 * 1000; // 1 MegaBytes-default value
	NumLogs = 100;
	ServerPort = 3500;
	MaxClients = 20;
	MaxDataStores = 50;
	MaxOpDataStores = 50;
}

void
createDataStoreSet ()
{
	if (DataStores) return;
	DataStores = new DataStoreSet();
	if (!DataStores->initialize (MaxDataStores)){
		printf ("Fatal Error: Unable to allocate memeory for DataStores.\n");
		exit (1);
	}

}

void
createOpDataStoreSet ()
{
	OpDataStores = new OpDataStoreSet();
	if (!OpDataStores->initialize (MaxOpDataStores)){
		printf ("Fatal Error: Unable to allocate memeory for Op DataStores.\n");
		exit (1);
	}
}

// parameter contains DataStore Name, XML File, XSD Dir, and XSLT Dir. Last two values are
// optional.

void
addDataStore (char *value)
{
    int num;
    char name   [PATHLEN*4+1];
    char xmlFile[PATHLEN*4+1];
    char xsdDir [PATHLEN*4+1];
    char xsltDir[PATHLEN*4+1];

    name[0]    = '\0';
    xmlFile[0] = '\0';
    xsdDir[0]  = '\0';
    xsltDir[0] = '\0';

    num = sscanf (value, "%s %s %s %s", name, xmlFile, xsdDir, xsltDir);
    if (num < 2){
    	printf("Unable to add data store. Format: Name xmlFile XSDDir XSLTDir.\nLast two values optional.\n");
    	return;
    } else {
    	if (!DataStores->addDataStoreFromFile(name, xmlFile, xsdDir, xsltDir)){
    		printf ("Error in adding data store %s\n", name);
    	}
    }
}

void
applySettings(int argc, char **argv)
{
	FILE *paramFile;
	char line [PATHLEN*4+1];
	char name [PATHLEN+102];
	char value[PATHLEN*4+1];
	char *ret;
	int retInt;
	int lineSize;
	int count = 0;

	applyDefaultSettings();
	if (argc < 2) {
		printf ("No parameter file specified. Using default parameter values.\n");
		createDataStoreSet ();
		createOpDataStoreSet();
		return;
	}

	// Read parameter file present as second word on command line
	if ((paramFile = fopen (argv[1], "r")) == NULL){
			fprintf (stderr, "FATAL ERROR: Could not read the parameter file : %s.\n", argv[1]);
			exit (1);
	}
	printf ("Using the parameter file '%s'\n", argv[1]);

	while (true){
		if((ret = fgets(line, PATHLEN*4+1, paramFile)) == NULL) break;
		count++;
		// if the first char is #, it is a comment - skip it
		if (line[0] == '#') continue;
		lineSize = strlen (line);

		// read name value pair  "name value;"
		if((retInt = sscanf(line, "%s %[^;]s", name, value)) != 2){
			continue;
		} else {
			if (line[lineSize-2] != ';'){
				fprintf(stderr, "Syntax Error: at line # %d; The last char must be ';'.\n", count);
				exit (1);
			}
		    if (strcmp (name, "LOGDIR") == 0){
				strcpy(LogDir, value);
			} else if (strcmp(name, "LOGSIZE") == 0){
				MaxLogSize = atoi(value);
			} else if (strcmp(name, "NUMLOGS") == 0){
				NumLogs = atoi(value);
			} else if (strcmp(name, "SERVERPORT") == 0){
				ServerPort = atoi (value);
			} else if (strcmp(name, "LOGLEVEL") == 0){
				LogLevel = atoi (value);
			} else if (strcmp(name, "MAXCLIENTS") == 0){
				MaxClients = atoi (value);
			} else if (strcmp(name, "MAXDATASTORES") == 0){
				MaxDataStores = atoi (value);
				createDataStoreSet();
			} else if (strcmp(name, "DATASTORE") == 0){
				createDataStoreSet();
				addDataStore (value);
			} else if (strcmp(name, "MAXOPDATASTORES") == 0){
				MaxOpDataStores = atoi (value);
			}
		}
	}
	// Data Store-Set may not be present. Make sure it is created.
	createDataStoreSet();
	createOpDataStoreSet();
	fclose (paramFile);
}

void
initializeMutexes (void)
{
	if (pthread_mutex_init(&LogMutex, NULL) != 0)
	{
		fprintf(stderr, "log mutex init failed\n");
		exit(1);
	}
}

void
destroyMutexes (void)
{
	pthread_mutex_destroy (&LogMutex);
}

void
logWelcomeMessage (int argc, char **argv)
{
	 sprintf (LogLine, "Starting server %s . . . . . .\n", argv[0]);
	 common::LogMsg(1, LogLine, false);
	 if (argc > 1)
		 sprintf (LogLine, "Parameter file is : %s\n", argv[1]);
	 else sprintf (LogLine, "No parameter file. Using defaults.\n");
	 common::LogMsg(1, LogLine, false);
}

void
createListeningSocket(int *listenfd)
{
	int sock;
	struct sockaddr_in servaddr;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1) {
	   sprintf (LogLine,  "Error in creating listening socket.\n");
	   common::LogMsg (1, LogLine, false);
	   *listenfd = -1;
	} else {
	   bzero(&servaddr, sizeof(servaddr));
	   servaddr.sin_family = AF_INET;
	   servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	   servaddr.sin_port =  htons(ServerPort);
	   if (bind(sock, (SA *) &servaddr, sizeof(servaddr))){
		   printf ("Fatal Error: Could not bind to the server port %d. May be it is in use or in TIME_WAIT state.\n", ServerPort);
		   printf ("Use 'netstat -nap | grep %d' command to see its status.\n", ServerPort);
		   *listenfd = -1;
	   } else {
		   *listenfd = sock;
	   }
	}
}

int acceptClients(int listenfd, int timeout)
{
   int iResult;
   struct timeval tv = { 0, timeout };  // 100 ms = 100000
   fd_set rfds;
   int s;

   FD_ZERO(&rfds);
   FD_SET(listenfd, &rfds);
   iResult = select(listenfd+1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv);
   if(iResult > 0){
      s =  accept(listenfd, NULL, NULL);
      return s;
   } else if (iResult == 0){  /// timeout
      return -1;
   }
   // error
   return -2;
}


int
main (int argc, char **argv)
{
	int listenfd, connfd, n, index;
	socklen_t clilen;
	bool error;
	ClientSet *clientSet = NULL;
	int terminationIterations = 0;

	char recvline[300];
	char sendline[300];

	applySettings(argc, argv); // read parameter file and initialize
	initializeMutexes ();
    createLogDir ();           // create the dir where all log files will get created.
    logWelcomeMessage (argc, argv);
    clientSet = new ClientSet (); // where info about all clients will be stored
    if (clientSet){
    	error = clientSet->initialize(MaxClients);
    } else {
    	error = true;
    }
    /* Init libxml */
    xmlInitParser();
    createListeningSocket (&listenfd);
    if(listenfd != -1 && !error){
       listen(listenfd, SOMAXCONN); // SOMAXCONN defined by socket.h: Max # of clients
       while(!TerminateNow)
       {
          sprintf(LogLine, "Listening on Port %d .....\n", ServerPort);
          common::LogMsg(9, LogLine, false);
          connfd = acceptClients (listenfd, 1000000); // timeout = 1 sec
          if(connfd > -1){
             sprintf(LogLine, "Received a connection request.\n");
             common::LogMsg(9, LogLine, false);
             // Register a new client
             index = clientSet->newClient (connfd);
             if(index < 0){
            	 sprintf (LogLine, "Error in registering the new client. Client not connected.\n");
            	 common::LogMsg (1, LogLine, false);
            	 close (connfd);
             } else if (!clientSet->startClient(index)){
            		 // error: close socket
            		 close (connfd);
             }
          } else if (connfd == -1){
        	  // time out
        	  continue;
          } else {
        	  // error
        	  TerminateNow = true;
          }
          clientSet->checkStaleClients();
       }
       // wait for all threads to finish
       while (clientSet->waitToFinish()){
    	   terminationIterations++;
    	   clientSet->checkStaleClients();
    	   if (terminationIterations == 500){
    		   clientSet->forceTermination ();
    		   break;
    	   }
       }
    }
    delete (clientSet);
    close (listenfd);
    delete (DataStores); // called only at one palce here.
	destroyMutexes ();
	common::CloseLog();
	xmlCleanupParser(); // To be called only once when exiting, no where else.
	                     // It cleans up global space allocated by XML parser.
	return 0;
}




