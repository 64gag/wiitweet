/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar
 *
 * wt_profile.cpp
 *
 * Saving/Loading user data support class.
 ***************************************************************************/
#ifndef WT_PROFILE_H
#define WT_PROFILE_H

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <ogcsys.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <string>
#include <fat.h>
#include "twitcurl/twitcurl.h"

#define DONT_SAVE 0
#define SAVE 1
#define TOKENMAXLEN 63

class wt_profile{
	public:
	char tokenKeyLen;
	char tokenKey[TOKENMAXLEN];
	char tokenSecretLen;
	char tokenSecret[TOKENMAXLEN];
	char screenName[TOKENMAXLEN];
	char userName[TOKENMAXLEN+1]; //Local username used to identify the profile, it is also the filename

	char version; //Profile version, in case something changes and I need to be aware of which info do I have
	char oauth; //Authorization step
	char NIP; //True (non-zero) if the user has a NIP
	char save;
	char img;

	class twitCurl * twitterObj;

	wt_profile(const char * name); //First-time constructor
	wt_profile(char * filename, int savechanges); //Load-from-file constructor
	~wt_profile();

	void getTokenKey(std::string& buf, const char * typedNIP);
	void setTokenKey(std::string& buf, const char * typedNIP);

	void getTokenSecret(std::string& buf, const char * typedNIP);
	void setTokenSecret(std::string& buf, const char * typedNIP);

	void getScreenName(std::string& buf);
	void setScreenName(std::string& buf);
};
#endif
