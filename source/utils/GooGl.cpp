/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar 2012
 *
 * GooGl.cpp
 *
 * Google's URL shortening service
 * This should work with real a real CURL except for the CURLOPT_JSON option.
 ***************************************************************************/

#include "twitcurl/curl/curl.h"
#include "menu.h"
std::string shortUrl;

void googlCallback( char* data, size_t size, size_t nmemb, void* nada )
{
	std::string response(data);
	size_t pos_start, pos_end;
	pos_start = response.find("\"id\": \"http") + 7; //Avoid JSON parser!
	pos_end = response.find('"', pos_start);
	shortUrl = response.substr(pos_start, pos_end - pos_start);
}

void shortenUrl(std::string &longUrl, std::string &shortedUrl){
	CURL curlobj;
	curl_easy_setopt(&curlobj, CURLOPT_POST, 1);
	curl_easy_setopt(&curlobj, CURLOPT_URL, "https://www.googleapis.com/urlshortener/v1/url?key=AIzaSyDTUz7nZhpTyhyreCigDpJEf7fqc2Fb_rw"); //This is my google API key use it gently (if at all)! @aruskano

	std::string jsonPost("{\"longUrl\": \""); jsonPost += longUrl; jsonPost += "\"}";
	curl_easy_setopt( &curlobj, CURLOPT_COPYPOSTFIELDS, jsonPost.c_str());
	curl_easy_setopt(&curlobj, CURLOPT_JSON, 1);

	curl_easy_setopt( &curlobj, CURLOPT_WRITEFUNCTION, googlCallback);
	curl_easy_setopt( &curlobj, CURLOPT_WRITEDATA, NULL);

	curl_easy_perform( &curlobj );

	shortedUrl = shortUrl;
}


