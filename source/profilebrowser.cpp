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

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <sys/dir.h>
#include <malloc.h>
#include <di/di.h>

#include "wiitweet.h"
#include "profilebrowser.h"
#include "menu.h"
#include "video.h"
#include "networkop.h"
#include "fileop.h"
#include "input.h"

static DIR *dir = NULL;
BROWSERINFO browser;
BROWSERENTRY * browserList = NULL; // list of files/folders in browser

/****************************************************************************
 * ResetBrowser()
 * Clears the file browser memory, and allocates one initial entry
 ***************************************************************************/
void ResetBrowser()
{
	browser.numEntries = 0;
	browser.size = 0;
}

bool AddBrowserEntry()
{
	if(browser.size >= MAX_BROWSER_SIZE)
	{
		ErrorPrompt("Out of memory: too many files!");
		return false; // out of space
	}

	memset(&(browserList[browser.size]), 0, sizeof(BROWSERENTRY)); // clear the new entry
	browser.size++;
	return true;
}


static char *GetExt(char *file)
{
	if(!file)
		return NULL;

	char *ext = strrchr(file,'.');
	if(ext != NULL)
	{
		ext++;
		int extlen = strlen(ext);
		if(extlen > 5)
			return NULL;
	}
	return ext;
}

static bool ParseProfilesDir()
{
	if(!dir)
		return false;

	char *ext;
	struct dirent *entry = NULL;

	int i = 0;

	while(i < MAX_BROWSER_SIZE)
	{
		entry = readdir(dir);

		if(entry == NULL)
			break;

		if(entry->d_name[0] == '.' && entry->d_name[1] != '.')
			continue;

		if(strcmp(entry->d_name, "..") == 0)
			continue;

		if(entry->d_type==DT_DIR) continue;

		ext = GetExt(entry->d_name);
		if(ext == NULL || stricmp(ext, "wtp") != 0)
			continue;

		if(!AddBrowserEntry())
		{
			break;
		}

		snprintf(browserList[browser.numEntries+i].filename, MAXJOLIET, "%s", entry->d_name);
		StripExt(browserList[browser.numEntries+i].displayname, browserList[browser.numEntries+i].filename); // hide file extension

		i++;
	}

	// Sort the file list
	if(i >= 0)
		qsort(browserList, browser.numEntries+i, sizeof(BROWSERENTRY), FileSortCallback);

	browser.numEntries += i;

	if(entry == NULL)
	{
		closedir(dir); // close directory
		dir = NULL;
		return false; // no more entries
	}
	return true; // more entries
}

int
ParseDirectory()
{
	bool mounted = false;
	
	ResetBrowser(); // reset browser
	
	// add trailing slash
	if(browser.dir[strlen(browser.dir)-1] != '/')
		strcat(browser.dir, "/");

	mounted = ChangeInterface(browser.dir, SILENT);

	if(mounted){
		dir = opendir(browser.dir);
		if(dir == NULL){		//Profile folder not found, attempt to create it.
			char temp[128];
			strcpy(temp, browser.dir);
			temp[strlen(browser.dir)-1] = 0;
			mkdir(temp, 0777);
			dir = opendir(browser.dir);
		}
	}else{
		return -1;
	}

	if (dir == NULL) return -1;

	ParseProfilesDir(); // index first 20 entries

	return browser.numEntries;
}

/****************************************************************************
 * CleanupPath()
 * Cleans up the filepath, removing double // and replacing \ with /
 ***************************************************************************/
static void CleanupPath(char * path)
{
	if(!path || path[0] == 0)
		return;
	
	int pathlen = strlen(path);
	int j = 0;
	for(int i=0; i < pathlen && i < MAXPATHLEN; i++)
	{
		if(path[i] == '\\')
			path[i] = '/';

		if(j == 0 || !(path[j-1] == '/' && path[i] == '/'))
			path[j++] = path[i];
	}
	path[j] = 0;
}

bool IsDeviceRoot(char * path)
{
	if(path == NULL || path[0] == 0)
		return false;

	if( strcmp(path, "sd:/") == 0 || strcmp(path, "usb:/")   == 0 )
	{
		return true;
	}
	return false;
}


/****************************************************************************
 * FileSortCallback
 *
 * Quick sort callback to sort file entries with the following order:
 *   .
 *   ..
 *   <dirs>
 *   <files>
 ***************************************************************************/
int FileSortCallback(const void *f1, const void *f2)
{
	return stricmp(((BROWSERENTRY *)f1)->filename, ((BROWSERENTRY *)f2)->filename);
}

/****************************************************************************
 * StripExt
 *
 * Strips an extension from a filename
 ***************************************************************************/
void StripExt(char* returnstring, char * inputstring)
{
	char* loc_dot;

	snprintf (returnstring, MAXJOLIET, "%s", inputstring);

	if(inputstring == NULL || strlen(inputstring) < 4)
		return;

	loc_dot = strrchr(returnstring,'.');
	if (loc_dot != NULL)
		*loc_dot = 0; // strip file extension
}

int OpenProfilesFolder()
{
	sprintf(browser.dir, "%s/profiles/", appPath);
	int device = 0;
	FindDevice(browser.dir, &device);
	
	CleanupPath(browser.dir);
	ResetBrowser(); // reset browser

	return ParseDirectory();
}
