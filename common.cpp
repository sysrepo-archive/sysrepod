/*
 * License: Apache 2.0
 *
 * common.cpp
 * Author: Niraj Sharma
 * Cisco Systems, Inc.
 *
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>

#include "global.h"
#include "common.h"

common::common() {
	// TODO Auto-generated constructor stub
}

common::~common() {
	// TODO Auto-generated destructor stub
}

bool common::OpenNewLog ()
{
	time_t timesec;
	int numFiles;

	CloseLog();

	numFiles = FileCount(LogDir);
	if (numFiles >= NumLogs || numFiles == -1){
		// remove one file from log-dir.
		sprintf(LogBuffer, "rm %s/`ls -t %s | tail -1`", LogDir, LogDir);
		system(LogBuffer);
	}

	// construct new log file name using current time.
	timesec = time (NULL);
	if (timesec < 0) {
		printf ("Warning: Could not get current time. Using 444444.\n");
		timesec = 444444;
	}
	sprintf (LogFileName, "%s/log_%ld", LogDir, (long) timesec);

	// create this log file
	if ((LogFD = fopen (LogFileName, "w")) == NULL){
		printf ("Warning: Could not create Log File %s. Logging has stopped. Check disk space.\n", LogFileName);
		return false;
	} else {
		CurrLogSize = 0;
		LogFileSize = 0;
		return true;
	}
}

void common::CloseLog (void)
{
	if (!LogFD) return;
	ftruncate (fileno(LogFD), CurrLogSize);
	fclose (LogFD);
	LogFD = NULL;
}

// For better efficiency of appending content in a file, increase its size beyond what is needed now.
void common::addLogAndSpace (char *logline)
{
	time_t currtime = time(NULL);
	int size;
	char timestr[50];
	size_t num;

	strcpy(timestr, ctime(&currtime));
	timestr[strlen(timestr)-1] = '\0';
	size = sprintf (LogBuffer, "%s ::: %s\n", timestr, logline);
	// write LogBuffer to log file so that more space than required by the current message gets written, this in turn will increase the file size.
	num = fwrite (LogBuffer, 1, sizeof(LogBuffer), LogFD);
	if (num < sizeof(LogBuffer)) {
		// some error in writing log
		LogFileSize += num;
		if (num > size){
			CurrLogSize = CurrLogSize + size;
			// Since more than required data was written, we need to set file next-write-position to correct place for next log message
			fseek (LogFD, CurrLogSize, SEEK_SET);
		} else {
			CurrLogSize = CurrLogSize + num;
		}

	} else {
	    CurrLogSize = CurrLogSize + size;
	    // Since more than required data was written, we need to set file next-write-position to correct place for next log message
	    fseek (LogFD, CurrLogSize, SEEK_SET);
	    LogFileSize += sizeof(LogBuffer);
	}
}

// The system command  rm `ls -t | tail -1` will delete the oldest file in the current dir.
// To delete the oldest file in a dir absolute path use:
//     rm /Users/nsharma/temp/`ls -t /Users/nsharma/temp | tail -1`

// DO NOT call LogMsg in this Logging related functions. It will cause recursive calls.
// Every thread must have their own log buffer to pass as a parameter into this routine.

// If fatal is true, exit after writing log

void common::LogMsg (int logLevel, char * logline, bool fatal)
{
	int size;
	char timestr[50];
	int msgSize;
	time_t currtime = time(NULL);

	if (LogLevel < logLevel) return;

	// Lock logging system
	pthread_mutex_lock(&LogMutex);

	// If LogFD is not open, try to open it
	if (LogFD == NULL) OpenNewLog();
	// Due to errors, LogFD might still be NULL. We take care of that case below.
	msgSize = strlen (logline);
	if (msgSize >= LOGBUFFERSIZE || // The line to be logged is too big for Log Buffer.
		LogFD == NULL){
		if(msgSize >= LOGBUFFERSIZE){
			fprintf (stderr, "LOG MESSAGE IS TOO BIG. Current size: %d, Max size allowed: %d\n", msgSize, LOGBUFFERSIZE);
		} else {
			fprintf (stderr, "Log File is not OPEN.\n");
		}
		strcpy(timestr, ctime(&currtime));
		timestr[strlen(timestr)-1] = '\0';
		printf ( "%s ::: %s\n", timestr, logline);
		pthread_mutex_unlock(&LogMutex);
		if (fatal) exit (1);
		return;
	}

	// if there is too little space in file is left or message is bigger than the space left, extend file.
	if ((LogFileSize - CurrLogSize) < 1024 || (msgSize + 100) > (LogFileSize - CurrLogSize)){
		addLogAndSpace (logline);
	} else {
		strcpy(timestr, ctime(&currtime));
		timestr[strlen(timestr)-1] = '\0';
		size = fprintf (LogFD, "%s ::: %s\n", timestr, logline);
		CurrLogSize = CurrLogSize + size;
	}

    fflush (LogFD);

	if (fatal) {
		pthread_mutex_unlock(&LogMutex);
		CloseLog();
		exit (1);
	}
	if (CurrLogSize >= MaxLogSize){
		// There might be extra space at end of log file, truncate it.
		ftruncate (fileno(LogFD), CurrLogSize);
		OpenNewLog();
	}
	pthread_mutex_unlock(&LogMutex);
}

bool common::Do_mkdir(const char *path, mode_t mode)
{
    struct stat     st;
    bool            status = true;

    if (stat(path, &st) != 0)
    {
        /* Directory does not exist. */
        if (mkdir(path, mode) != 0)
            status = false;
    } else if (!S_ISDIR(st.st_mode))
    {
        errno = ENOTDIR;
        status = false;
    }
    return(status);
}

/*
 *   mkpath - It works top-down to ensure each directory in path exists.
 *   Example of its usage: bool rc = mkpath(argv[i], 0777);
 *                         if (!rc) fprintf(stderr, "%d: failed to create (%d: %s)\n",
 *                                              (int)getpid(), errno, strerror(errno));
 */
bool common::Mkpath(const char *path, mode_t mode)
{
    char           *pp;
    char           *sp;
    bool             status;
    char           copypath[PATH_MAX+1];

    strcpy (copypath, path);
    status = true;
    pp = copypath;
    while (status && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = Do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status)
        status = Do_mkdir(path, mode);
    return (status);
}

// calculate the number of files in a directory.
// NOTE that opendir and closedir system calls MALLOC and FREE memory. Could not avoid these calls at this time. LOOK AT IT LATER : nks
int common::FileCount (char *path)
{
	int file_count = 0;
	DIR * dirp;
	struct dirent *entry;

	dirp = opendir(path);
	if (dirp == NULL){
		fprintf(stderr, "Could not find number of files in the directory.\n");
		return -1;
	}
	while ((entry = readdir(dirp)) != NULL) {
         file_count++;
    }
	closedir(dirp);
	return file_count-2; // '.' and '..' are also get counted, you need to disregard these two
}

bool common::IsDir (char *path)
{
	struct stat status;
	if (stat(path ,&status) == 0){
		if (status.st_mode & S_IFDIR)
			return true;
	}
	return false;
}

bool common::SendMessage (int sock, char *message)
{
	int msgSize = strlen (message);
	char fmt[20];
	char msgSizeStr[20];
	int sent = 0;
	int toSend;
	int n;
	char sendInfo [100];

	sprintf (fmt, "%%.%dd ", MSGLENFIELDWIDTH);
	sprintf (msgSizeStr, fmt, msgSize);
	sprintf (sendInfo, "Message SENT on Socket %d is : %s\n", sock, msgSizeStr);
	common::LogMsg (4, sendInfo, false);
	common::LogMsg (4, message, false);
	toSend = MSGLENFIELDWIDTH + 1;
	// send size str
	while (sent < toSend){
		n = send(sock, &(msgSizeStr[sent]), strlen(&(msgSizeStr[sent])), 0);
		if (n < 1) return false;
		sent = sent + n;
	}
	// send payload
	sent = 0;
	toSend = msgSize;
	while (sent < msgSize){
		n = send(sock, &(message[sent]), toSend, 0);
		if (n < 1) return false;
		sent = sent + n;
		toSend = toSend - n;
	}

	return true;
}

int common::ReadSock (int sockfd, char **buffPtr, int orgBuffSize)
{
	int len = 0;
	int msgSize = 0;
	int numBytesRead = 0;
	int n;
	char *newBuffer;
	int buffSize;

	// Do not read more than the message length at any point, otherwise you will end up reading next command
	// from client.
	// First Read a number only
	buffSize = MSGLENFIELDWIDTH + 1;

	// First few chars in all messages reserved for message size followed by one white space.
	while (msgSize == 0 && numBytesRead < MSGLENFIELDWIDTH + 1){
	    n= recv(sockfd, &((*buffPtr)[numBytesRead]), buffSize - numBytesRead, 0);
	    if (n < 1) return 0; // client is bad
	    numBytesRead = numBytesRead + n;
	    if (numBytesRead < MSGLENFIELDWIDTH + 1) continue; // need more bytes to proceed
	    // read message size
	    (*buffPtr)[buffSize] = '\0';
	    n = sscanf(*buffPtr, "%d", &msgSize);
	    if (n != 1){ // msg len not found
	        printf ("Unable to find message length.\n");
	        return 0;
	    }
	    if (MSGLENFIELDWIDTH + 1 + msgSize + 1 > orgBuffSize){ // input buffer is too small
	    	newBuffer = (char *) realloc (*buffPtr, MSGLENFIELDWIDTH + 1 + msgSize + 1);
	    	if (newBuffer == NULL){
	    		printf ("Unable to allocate space.\n");
	    		return 0; // error
	    	}
	    	*buffPtr = newBuffer;
	        buffSize = MSGLENFIELDWIDTH + 1 + msgSize;
	    } else {
	    	buffSize = orgBuffSize;
	    }
	}
	while ((MSGLENFIELDWIDTH + 1 + msgSize) > numBytesRead){
	    // need to read more bytes, but not beyond message size
	    n = recv(sockfd, &((*buffPtr)[numBytesRead]), buffSize - numBytesRead, 0);
	    if (n < 1) return 0; // client is bad
	    numBytesRead = numBytesRead + n;
	}
	// complete message read
	(*buffPtr)[numBytesRead] = '\0';
	return msgSize;
}
