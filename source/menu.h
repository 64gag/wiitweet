/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2010
 * Pedro Aguiar 2012
 *
 * menu.h
 *
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#ifndef _MENU_H_
#define _MENU_H_

#include <ogcsys.h>

void InitGUIThreads();
void MainMenu (int menuitem);
void ErrorPrompt(const char * msg);
int ErrorPromptRetry(const char * msg);
void InfoPrompt(const char * msg);
void ShowAction (const char *msg);
void CancelAction();

extern "C" { void ShowProgress (const char *msg, int done, int total); }

void ResetText();

enum
{
	MENU_EXIT = -1,
	MENU_NONE,
	MENU_FIRSTSCREEN,
	MENU_BROWSE_PROFILES,
};

#endif
