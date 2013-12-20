/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2012
 *
 * wiitweet.cpp
 *
 * This file controls overall program flow. Most things start and end here!
 ***************************************************************************/

#ifndef _WIITWEET_H_
#define _WIITWEET_H_

#include <unistd.h>

#include "utils/FreeTypeGX.h"
#include "filelist.h"

#define APPNAME 		"WiiTweet"
#define APPVERSION 		"0.3.2"
#define APPFOLDER 		"wiitweet"

#define NOTSILENT 0
#define SILENT 1

const char pathPrefix[3][8] =
{ "", "sd:/", "usb:/"};

enum {
	DEVICE_AUTO,
	DEVICE_SD,
	DEVICE_USB,
};

struct t_settings{ 
	int	Rumble;
	int	ExitAction;
};

void ExitApp();
extern struct t_settings Settings;
extern int ShutdownRequested;
extern int ExitRequested;
extern char appPath[];
extern FreeTypeGX *fontSystem[];

#endif
