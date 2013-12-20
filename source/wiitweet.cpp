/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2012
 *
 * wiitweet.cpp
 *
 * This file controls overall program flow. Most things start and end here!
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <fat.h>
#include <debug.h>
#include <sys/iosupport.h>

#include "wiitweet.h"
#include "networkop.h"
#include "video.h"
#include "menu.h"
#include "fileop.h"
#include "profilebrowser.h"
#include "input.h"
#include "utils/FreeTypeGX.h"
#include "wt_profile.h"
#include "twitcurl/twitcurl.h"
#include "utils/mem2_manager.h"
#include "utils/ssl.h"

int ShutdownRequested = 0;
int ResetRequested = 0;
int ExitRequested = 0;
char appPath[1024] = { 0 };

struct t_settings Settings;

extern "C" {
extern void __exception_setreload(int t);
}

/****************************************************************************
 * Shutdown / Reboot / Exit
 ***************************************************************************/

void ExitApp()
{

	ShutoffRumble();
	StopGX();
	UnmountAllFAT();

	if(Settings.ExitAction == 0) // Auto
	{
		char * sig = (char *)0x80001804;
		if(
			sig[0] == 'S' &&
			sig[1] == 'T' &&
			sig[2] == 'U' &&
			sig[3] == 'B' &&
			sig[4] == 'H' &&
			sig[5] == 'A' &&
			sig[6] == 'X' &&
			sig[7] == 'X')
			Settings.ExitAction = 3; // Exit to HBC
		else
			Settings.ExitAction = 1; // HBC not found
	}
	if( ShutdownRequested || Settings.ExitAction == 2) // Shutdown Wii
	{
		SYS_ResetSystem(SYS_POWEROFF, 0, 0);
	}
	else if(ResetRequested || Settings.ExitAction == 1) // Exit to Menu
	{
		SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0);
	}

	exit(0);
}

void ShutdownCB()
{
	ExitRequested = 1;
	ShutdownRequested = 1;
}
void ResetCB()
{
	ExitRequested = 1;
	ResetRequested = 1;
}

int
main(int argc, char *argv[])
{
	L2Enhance();
	InitVideo();
	WII_Initialize();
	SetupPads();
	MountAllFAT(); // Initialize libFAT for SD and USB
	ssl_init();

	// Wii Power/Reset buttons
	__STM_Close();
	__STM_Init();
	__STM_Close();
	__STM_Init();
	WPAD_SetPowerButtonCallback((WPADShutdownCallback)ShutdownCB);
	SYS_SetPowerCallback(ShutdownCB);
	SYS_SetResetCallback(ResetCB);

	__exception_setreload(2);

	// store path app was loaded from
	if(argc > 0 && argv[0] != NULL){ 
		CreateAppPath(argv[0]);
	}
	if(appPath[0] == '\0'){
		FindAppPath();
	}

	AddMem2Area (7*1024*1024, MEM2_OTHER);
	AddMem2Area (2*1024*1024, MEM2_BROWSER);
	AddMem2Area (32*1024*1024, MEM2_GUI);

	InitFreeType((u8*)font_ttf, font_ttf_size);

	browserList = (BROWSERENTRY *)mem2_malloc(sizeof(BROWSERENTRY)*MAX_BROWSER_SIZE, MEM2_OTHER);

	InitGUIThreads();

	MainMenu(MENU_FIRSTSCREEN);
	ExitApp();
}
