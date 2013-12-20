/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar
 *
 * wt_profile.cpp
 *
 * Saving/Loading user data support class.
 ***************************************************************************/
#include "fileop.h"
#include "wt_profile.h"
#include "menu.h"
#include "twitcurl/SHA1.h"
#include "wiitweet.h"

char * xorEncrypt(const char * key, const char * plain, char plength){

	char * encrypted = NULL;

	int shas = 0, i;

	while((++shas)*40 < plength);

	encrypted = (char *)calloc(plength+1, 1);

	u8 keyHash[40*shas+1];
	keyHash[0] = 0; //CSHA1 uses strcat... 

		for(i=0; i<shas; i++){
	
			{
				CSHA1 sha1;
				if(!i)
					sha1.Update((unsigned char*)key, strlen(key));
				else
					sha1.Update(keyHash, 40*i);
	
				sha1.Final();
				sha1.ReportHash((char *)keyHash, 0);
			}
		}

		i = 0;
		while(i < plength){
			encrypted[i] = keyHash[i]^plain[i];
			i++;
		}
		encrypted[i] = '\0';

return encrypted;
}

wt_profile::wt_profile(const char * name){ //Only one argument specified, create new profile
	memset(this, 0, sizeof(wt_profile));
	strcpy(userName, name);
	twitterObj = new twitCurl;
}

wt_profile::wt_profile(char * filename, int savechanges){ //filepath specified, load profile
	memset(this, 0, sizeof(wt_profile));
	char pathtofile[256];
	sprintf(pathtofile, "%s/profiles/%s", appPath, filename);
	if(LoadFile(reinterpret_cast<char*>(this), pathtofile, 0, NOTSILENT) <= 0){
		delete this; //suicide!
	}else{
		save = savechanges;
		twitterObj = new twitCurl;
	}
}

wt_profile::~wt_profile(){ //Save the file before destroying
	delete twitterObj;
	if(save){
		twitterObj = NULL;
		save = 0;
		char path[256];
		sprintf(path,"%s/profiles/%s.wtp",appPath,userName);
		SaveFile (reinterpret_cast<char*>(this), path, sizeof(wt_profile), NOTSILENT);
	}

}


void wt_profile::getTokenKey(std::string& buf, const char * typedNIP){
	if(NIP){
		char * decrypt = xorEncrypt(typedNIP, tokenKey, tokenKeyLen);
		buf = decrypt;
		free(decrypt);
	}else{
		buf = tokenKey;
	}
}

void wt_profile::setTokenKey(std::string& buf, const char * typedNIP){
	if(NIP){
		memset(&tokenKey, 0, TOKENMAXLEN);
		char * encrypt = xorEncrypt(typedNIP, buf.c_str(), buf.length());
		memcpy(&tokenKey, encrypt, buf.length());
		free(encrypt);
		tokenKeyLen = buf.length();
	}else{
		memset(&tokenKey, 0, TOKENMAXLEN);
		strcpy(tokenKey, buf.c_str());
	}
}

void wt_profile::getTokenSecret(std::string& buf, const char * typedNIP){
	if(NIP){
		char * decrypt = xorEncrypt(typedNIP, tokenSecret, tokenSecretLen);
		buf = decrypt;
		free(decrypt);
	}else{
		buf = tokenSecret;
	}
}

void wt_profile::setTokenSecret(std::string& buf, const char * typedNIP){
	if(NIP){
		memset(&tokenSecret, 0, TOKENMAXLEN);
		char * encrypt = xorEncrypt(typedNIP, buf.c_str(), buf.length());
		memcpy(&tokenSecret, encrypt, buf.length());
		free(encrypt);
		tokenSecretLen = buf.length();
	}else{
		memset(&tokenSecret, 0, TOKENMAXLEN);
		strcpy(tokenSecret, buf.c_str());
	}
}

void wt_profile::getScreenName(std::string& buf){
	buf = screenName;
}

void wt_profile::setScreenName(std::string& buf){
	memset(&screenName,0,TOKENMAXLEN);
	strcpy(screenName, buf.c_str());
}
