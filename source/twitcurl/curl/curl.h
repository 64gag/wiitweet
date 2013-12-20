/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar 2012
 *
 * curl.h
 *
 * A class that emulates curl features used by twitcurl
 ***************************************************************************/

#ifndef _FCURL_H_
#define _FCURL_H_

#include <network.h>
#include <ogcsys.h>
#include <stdio.h>
#include <string>
#include <errno.h>
#include <memory>
#include <new>

#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include <iostream>
#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <gctypes.h>

/****** The following defines are passed to curl_easy_setopt to specify which option you are setting/modifying ******/
/* These are proxy specific, I dont care about them. */
#define CURLAUTH_ANY 0
#define CURLOPT_PROXY 0
#define CURLOPT_PROXYUSERPWD 0
#define CURLOPT_PROXYAUTH 0
/* Options approached by hardcoding or ignoring them */
#define CURLINFO_HTTP_CODE -1		/* Specify you want the HTTP response status code. I always get the HTTP response status code. */
#define CURLOPT_USERPWD -2		/* USER/PASSWORD for the HTTP Auth header. This is not needed to use oauth. */
#define CURLOPT_ERRORBUFFER -3   	/* Used in prepareCurlCallback(), it sets the char * to store the error messages. This won't be implemented. */
/* Implemented options */
#define CURLOPT_HTTPHEADER 1		/* Passes a list of headers to be used in the next request. This one is only used to pass the oAuth header. */
#define CURLOPT_URL 2			/* The target URL */
#define CURLOPT_HTTPGET 3		/* GET request */
#define CURLOPT_POST 4			/* POST request */
#define CURLOPT_COPYPOSTFIELDS 5	/* char * pointing to the data to POST */
#define CURLOPT_CUSTOMREQUEST 6		/* Used twice in the whole library, once to set DELETE as the request method and the other to NULL it (to go back to GETing after DELETEing) */
#define CURLOPT_WRITEFUNCTION 7 	/* Used in prepareCurlCallback(), it specifies the callback function */
#define CURLOPT_WRITEDATA 8		/* Specifies the fourth argument of the callback function */
#define CURLOPT_JSON 777		/* Made up by me to specify I'm sending JSON data to the google API without implementing header overriding*/

#define CURLE_OK 0 			/* 0 means ok! */

struct curl_slist {
	char * data;
//	struct curl_slist * next; //Unimplemented as it is never used by twitcurl!
};

typedef int CURLoption;
typedef int CURLcode;
typedef int CURLINFO;
typedef int (*cbptr)(char* data, size_t size, size_t nmemb, void *userdata);

class CURL {
	public:
		CURL();
		~CURL();

		void *userdata;
		cbptr callback;

		std::string url;
		std::string host;
		std::string path;
		std::string method;
		std::string headers;
		std::string postdata;
		std::string req;
		int accept_encoding;
		int json;

		void setMethod(int option);
		void buildHttpRequest();
		void URLsplit();
};

//These are the functions twitcurl attempts to use, the CURL class has to make twitcurl think it is actually "talking" to libcurl
CURL *curl_easy_init(void);
CURLcode curl_easy_setopt(CURL *curl, CURLoption option, ...);
CURLcode curl_easy_perform(CURL *curl);
void curl_easy_cleanup(CURL *curl);
CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, long unsigned int * stat);
struct curl_slist *curl_slist_append(struct curl_slist * clist, const char * str);
void curl_slist_free_all(struct curl_slist * clist);

#endif /* _FCURL_H_ */
