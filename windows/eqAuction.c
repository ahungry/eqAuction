/*
 * Author:  Matthew Carter
 * Purpose: Send periodic EQ logs to server for auction searching
 * Date:    20120220
 * License: GPLv3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <windows.h>

#define MAX_LINES 100
#define SLEEPER 60000

struct 
WriteThis {
	const char *readptr;
	int sizeleft;
};

void
readINI(char *logFile)
{
	FILE *fp;
	char line[255];
	char *c;
	int n;

	fp = fopen("eqAuction.ini", "r+t");
	if(fp == NULL) {
		printf("Missing eqAuction.ini file, using current directory\n");
		strcpy(line, ".");
	} else {
		fgets(line, 255, fp);
		do
		{
			c = fgets(line, 255, fp);
		} while (c != NULL && line[0] == '#');

		fclose(fp);
	}

	if(strlen(line) < 1 || line[0] == '\n' || line[0] == '\r' || line[0] == '#')
		strcpy(line, ".");
	else
		for(n = 0; n < strlen(line); n++)
			if(line[n] == '\n' || line[n] == '\r')
				line[n] = '\\';

	//Set into passed in pointer
	strcpy(logFile, line);
}

void 
genOutfile(void *fn) {
	//File stuff
	FILE *fp, *fo;
	char line[255];
	char *c;
	int tot = 0, b = 0;

	printf("Opening %s for parsing\n", fn);

	fp = fopen(fn, "r+t");
	fo = fopen("outtext", "w+t");

	if(fp == NULL || fo == NULL) {
		printf("Missing file\n");
	  	exit(EXIT_FAILURE);
	}

	do {
		c = fgets(line, 255, fp);
		if(c != NULL)
			tot++;
	} while (c != NULL);

	fclose(fp);
	fp = fopen(fn, "r+t");

	fputs("dump=", fo);
	do {
		c = fgets(line, 255, fp);
		if(c != NULL && b++ >= tot - MAX_LINES)
			fputs(c, fo);
	} while (c != NULL);

	fclose(fp);
	fclose(fo);
}

static size_t 
read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
{
	struct WriteThis *pooh = (struct WriteThis *)userp;

	if(size*nmemb < 1)
		return 0;

	if(pooh->sizeleft) {
		*(char *)ptr = pooh->readptr[0]; /* copy one single byte */
		pooh->readptr++;						/* advance pointer */
		pooh->sizeleft--;						/* less data left */
		return 1;
	}

	return 0;
}

char
*getLogFile(char *dir)
{
	char *fullPath = NULL;
	char *folderName = NULL;
	char *name;
	static char namex[255];
	int oY=0, nY=0, oM=0, nM=0, oD=0, nD=0, oh=0, nh=0, om=0, nm=0, os=0, ns=0;
	int odate = 0;
	int level = 0;
	int newhigh = 0;

	char findpath[_MAX_PATH], temppath[_MAX_PATH];
	HANDLE fh;
	WIN32_FIND_DATA wfd;
	FILETIME ft;
	SYSTEMTIME stUTC;
	int i;

	printf("SCANNING DIR (%s)...\n", dir);

	strcpy(findpath, dir);
	strcat(findpath, "\\*eqlog*");

	printf("Using findpath of %s\n", findpath);

	fh = FindFirstFile(findpath, &wfd);
	if(fh != INVALID_HANDLE_VALUE)
	{
		printf("Found the following eqlog files in that directory:\n");
		do
		{
			if(!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				for(i=0; i<level; i++) printf(" ");
				name = wfd.cFileName;
				printf("+--%s\n", name);
				ft = wfd.ftLastWriteTime;
				FileTimeToSystemTime(&ft, &stUTC);
				nY = stUTC.wYear;
				nM = stUTC.wMonth;
				nD = stUTC.wDay;
				nh = stUTC.wHour;
				nm = stUTC.wMinute;
				ns = stUTC.wSecond;

				//printf("Date: %d-%d-%d %d:%d:%d\n", nY, nM, nD, nh, nm, ns);
				if(nY > oY)
					newhigh = 1;
				else if(nY == oY && nM > oM)
					newhigh = 1;
				else if(nY == oY && nM == oM && nD > oD)
					newhigh = 1;
				else if(nY == oY && nM == oM && nD == oD && nh > oh)
					newhigh = 1;
				else if(nY == oY && nM == oM && nD == oD && nh == oh && nm > om)
					newhigh = 1;
				else if(nY == oY && nM == oM && nD == oD && nh == oh && nm == om && ns > os)
					newhigh = 1;
				else
					newhigh = 0;

				if(newhigh == 1) {
					//strcpy(namex, name);
					strcpy(namex, dir);
					strcat(namex, name);
					oY = nY;
					oM = nM;
					oD = nD;
					oh = nh;
					om = nm;
					os = ns;
				}
			}
		} while (FindNextFile (fh, &wfd));
		FindClose(fh);
	}
	else {
		printf("No log files found in that directory - exiting\n");
		exit(1);
	}

	return namex;
}

int 
main(int argc, char *argv[])
{
	char logFile[255];
	char logDir[255];

	readINI(logDir);

	if(strlen(logDir) < 1) {
		printf("Missing log dir (%s), exiting\n", logDir);
		exit(0);
	}

	printf("Using log directory: %s\n", logDir);

	for(;;) {

	strcpy(logFile, getLogFile(logDir));
	printf("Newest log file is %s, working with that...\n", logFile);

	genOutfile(logFile);

	//Grab entire outtext for sending
	FILE *fp;
	long lSize;
	char *buffer;
	size_t result;
	fp = fopen("outtext", "r+t");
	fseek(fp, 0, SEEK_END);
	lSize = ftell(fp);
	rewind(fp);

	//put in memory
	buffer = (char *) malloc (sizeof(char)*lSize);
	if(buffer == NULL) {
		fputs("Memory error", stderr);
		exit(2);
	}

	result = fread(buffer, 1, lSize, fp);
	if(result != lSize) {
		fputs("Buffer and filesize are not identical but close enough.\n",  stderr);
		//exit(3);
	}

	CURL *curl;
	CURLcode res;

	struct WriteThis pooh;

	pooh.readptr = buffer;
	pooh.sizeleft = lSize;

	curl = curl_easy_init();
	if(curl) {
		/* First set the URL that is about to receive our POST. */
		curl_easy_setopt(curl, CURLOPT_URL, "http://ahungry.com/aucDump.php");

		/* Now specify we want to POST data */
		curl_easy_setopt(curl, CURLOPT_POST, 1L);

		/* We want to use our own read function */
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);

		/* Pointer to pass to our read function */
		curl_easy_setopt(curl, CURLOPT_READDATA, &pooh);

#ifdef CURLSPAM
		/* Get verbose debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
#endif

		/*
		 * If you use POST to a HTTP 1.1 server, you can send data without knowing
		 * the size before starting the POST if you used chunked encoding.  You
		 * enable this by adding a header like "Transfer-Encoding: chunked" with
		 * CURLOPT_HTTPHEADER.  With HTTP 1.0 or without chunked transfer, you
		 * must specify the size in the request.
		 */
#ifdef USE_CHUNKED
		{
			struct curl_slist *chunk = NULL;

			chunk = curl_slist_append(chunk, "Transfer-Encoding: chunked");
			res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			/* use curl_slist_free_all() after the *perform() call to free this list
			 * again */
		}
#else
		/* Set the expected POST size.  If you want to POST large amounts of data,
		 * consider CURLOPT_POSTFIELDSIZE_LARGE */
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)pooh.sizeleft);
#endif

#ifdef DISABLE_EXPECT
		/*
		 * Using POST with HTTP 1.1 implies the user of a "Expect: 100-continue"
		 * header.  You can disable this header with CURLOPT_HTTPHEADER as usual.
		 * NOTE: if you want chunked transfer too, you need to combine these two
		 * since you can only set one list of headers with CURLOPT_HTTPHEADER. */

		/* A less good option would be to enforce HTTP 1.0, but that might also
		 * have other implications. */
		{
			struct curl_slist *chunk = NULL;

			chunk = curl_slist_append(chunk, "Expect:");
			res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
			/* use curl_slist_free_all() after the *perform() call to free this
			 * list again */
		}
#endif

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);

		/* always cleanup */
		curl_easy_cleanup(curl);
	}
	fclose(fp);
	free(buffer);
	printf("Now we wait %d seconds...\n", SLEEPER/1000);
	int x;
	for(x = 0; x < 60; x++) {
		printf(".");
		Sleep(SLEEPER/60);
	}
	//Sleep(SLEEPER);
	printf("\nContinuing...\n");
	}
	return 0;
}
