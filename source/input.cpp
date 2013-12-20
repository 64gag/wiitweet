/****************************************************************************
 * WiiMC
 * Tantric 2009-2012
 *
 * input.cpp
 * Wii/GameCube controller management
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogcsys.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include <ogc/lwp_watchdog.h>

#include "wiitweet.h"
#include "menu.h"
#include "video.h"
#include "input.h"
#include "libwiigui/gui.h"

#define RUMBLE_MAX			60000
#define RUMBLE_COOLOFF		10000000

static bool rumbleDisabled = true;
static int rumbleOn[4] = {0,0,0,0};
static u64 prev[4];
static u64 now[4];

GuiTrigger userInput[4];

/****************************************************************************
 * UpdatePads
 *
 * Scans wpad
 ***************************************************************************/
void UpdatePads()
{
	WPAD_ReadPending(0, NULL); // only wiimote 1
}

/****************************************************************************
 * SetupPads
 *
 * Sets up userInput triggers for use
 ***************************************************************************/
void SetupPads()
{
	WPAD_Init();
	WPAD_SetIdleTimeout(45);

	// read wiimote accelerometer and IR data
	WPAD_SetDataFormat(WPAD_CHAN_0, WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_0, screenwidth, screenheight);

	userInput[0].chan = 0;
	userInput[0].wpad = WPAD_Data(0);
}

/****************************************************************************
 * ShutoffRumble
 ***************************************************************************/

static void ShutoffRumble(int i, int cooloff)
{
	if(CONF_GetPadMotorMode() == 0)
		return;

	prev[i] = gettime() + cooloff;
	WPAD_Rumble(i, 0); // rumble off
	rumbleOn[i] = 0;
}

void ShutoffRumble()
{
	ShutoffRumble(0, RUMBLE_COOLOFF*3);
}

void DisableRumble()
{
	rumbleDisabled = true;
	ShutoffRumble();
}

void EnableRumble()
{
	rumbleDisabled = false;
}

void RequestRumble(int i)
{
	if(CONF_GetPadMotorMode() == 0 || rumbleDisabled || i < 0) // !WiiSettings.rumble
		return;

	now[i] = gettime();

	if(prev[i] > now[i])
		return;

	if(diff_usec(prev[i], now[i]) > RUMBLE_MAX)
	{
		rumbleOn[i] = 1;
		WPAD_Rumble(i, 1); // rumble on
		prev[i] = now[i];
	}
}

/****************************************************************************
 * DoRumble
 ***************************************************************************/

void DoRumble(int i)
{
	if(rumbleOn[i])
	{
		now[i] = gettime();

		if(diff_usec(prev[i], now[i]) > RUMBLE_MAX)
			ShutoffRumble(i, RUMBLE_COOLOFF);
	}
}


