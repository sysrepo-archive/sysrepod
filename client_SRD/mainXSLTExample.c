/*
 * mainHugeTest.c
 *
 * License: Apache 2.0
 *
 *  Created on: Jan 25, 2015
 *      Creator: Niraj Sharma
 *      Cisco Systems, Inc.
 *
 */

#define MAIN_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "srd.h"

#define MSGLENFIELDWIDTH 7

int main(int argc, char**argv)
{
   int sockfd, n;
   char defaultServerIP []="127.0.0.1";
   char *serverIP;
   int serverPort = SRD_DEFAULTSERVERPORT;
   char dataStoreName [100] = "runtime";
   char xslt [500];
   char *buff = NULL;

   if (argc != 2)
      serverIP = defaultServerIP;
   else
      serverIP = argv[1];

   if (!srd_connect (serverIP, serverPort, &sockfd)){
	   printf ("Error in connecting to server %s at port %d\n", serverIP, serverPort);
	   exit (1);
   }

   if (!srd_setDataStore(sockfd, dataStoreName)){
	   printf ("Error in setting Data Store.\n");
	   srd_disconnect (sockfd);
	   exit (1);
   }

   // Instruct SysrepoD to apply given XSLT on data store and return result
   // NOTE: XSLT must be enclosed in CDATA Clause.
   strcpy (xslt, "<![CDATA[<?xml version=\"1.0\"?><xsl:stylesheet version=\"1.0\" xmlns:xsl=\"http://www.w3.org/1999/XSL/Transform\"><xsl:output method=\"text\"/>"
"	   <xsl:template match=\"/\">"
"		    <xsl:for-each select=\"//*[not(*)]\">"
"		        <xsl:value-of select=\"local-name()\"/> : <xsl:value-of select=\"concat(., '&#10;')\"/>"
"		    </xsl:for-each>"
"	   </xsl:template>"
"	   </xsl:stylesheet>]]>"
   );

   srd_applyXSLT (sockfd, xslt, &buff);
   if (buff){
      printf ("Result of applying XSLT is :::::::\n\n%s\n", buff);
      free (buff);
   } else {
      printf ("Result of XSLT not found OR it is NULL.\n");
   }

   printf ("Going to sleep for 10 seconds before exiting.......\n");
   fflush (stdout);
   sleep (10);
   printf ("About to disconnect from SYSREPOD server\n");
   fflush (stdout);
   srd_disconnect (sockfd); // disconnect this client, leave server running
   // srd_terminateServer (sockfd); // terminate server and disconnect this client
   exit (0);
}
