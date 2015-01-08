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
#include <sys/types.h>
#include <regex.h>

#define MAX_LINES 100
#define SLEEPER 60
#define REGEX "[A-Za-z] auctions, '"

struct WriteThis {
	const char *readptr;
	int sizeleft;
};

void genOutfile(void *fn) {
	//File stuff
	FILE *fp, *fo;
	char line[255];
	char *c;
	int tot = 0, b = 0;

	//Regex stuff
	regex_t *preg = calloc(1, sizeof(regex_t));
	regcomp(preg, REGEX, REG_EXTENDED | REG_NOSUB);

	printf("Opening %s for parsing\n", fn);

	fp = fopen(fn, "r");
	fo = fopen("outtext", "w");

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
	fp = fopen(fn, "r");

	fputs("dump=", fo);
	do {
		c = fgets(line, 255, fp);
		if(c != NULL && b++ >= tot - MAX_LINES && !regexec(preg, c, 0,0,0))
			fputs(c, fo);
	} while (c != NULL);

	fclose(fp);
	fclose(fo);
	regfree(preg);
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp)
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

int main(int argc, char *argv[])
{
	if(argc < 2 || argv[1] == NULL) {
		printf("No files passed in\n");
		exit(EXIT_FAILURE);
	}

	for(;;) {
	genOutfile(argv[1]);

	//Grab entire outtext for sending
	FILE *fp;
	long lSize;
	char *buffer;
	size_t result;
	fp = fopen("outtext", "r");
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
		fputs("Read error", stderr);
		exit(3);
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

		/* Get verbose debug output */
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

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
	sleep(SLEEPER);
	}
	return 0;
}
