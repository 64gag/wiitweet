/****************************************************************************
 * WiiTweet
 * Tantric 2009-2012
 *
 * profilebrowser.cpp
 *
 * Generic file routines - reading, writing, browsing
 *
 * Pedro Aguiar adapted this from tantric's filebrowser
 ***************************************************************************/

#ifndef _FILEBROWSER_H_
#define _FILEBROWSER_H_

#include <unistd.h>
#include <gccore.h>

#define MAXJOLIET 255
#define MAX_BROWSER_SIZE	50

typedef struct
{
	char dir[MAXPATHLEN + 1]; // directory path of browserList
	int numEntries; // # of entries in browserList
	int size; // # of entries browerList has space allocated to store
} BROWSERINFO;

typedef struct
{
	char oAuth;
	char NIP;
	char displayname[MAXJOLIET + 1]; // name for browser display
	char filename[MAXJOLIET + 1]; // full filename
} BROWSERENTRY;

extern BROWSERINFO browser;
extern BROWSERENTRY * browserList;

int FileSortCallback(const void *f1, const void *f2);
void StripExt(char* returnstring, char * inputstring);
void ResetBrowser();
bool AddBrowserEntry();
bool IsDeviceRoot(char * path);
int OpenProfilesFolder();
#endif
