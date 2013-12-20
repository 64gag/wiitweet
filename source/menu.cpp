/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2010
 * Pedro Aguiar 2012
 *
 * menu.cpp
 *
 * Menu flow routines - handles all menu logic
 ***************************************************************************/

#include <gccore.h>
#include <ogcsys.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiiuse/wpad.h>
#include <sys/stat.h>
#include <time.h>
#include <mxml.h>
#include "wiitweet.h"
#include "video.h"
#include "networkop.h"
#include "fileop.h"
#include "input.h"
#include "filelist.h"
#include "libwiigui/gui.h"
#include "libwiigui/gui_riverunit.h"
#include "menu.h"

#include "utils/mem2_manager.h"
#include "utils/FreeTypeGX.h"

#include "wt_profile.h"
#include "profilebrowser.h"
#include "twitcurl/twitcurl.h"
#include "twitter.h"
#include "cache.h"
#include "arl.h"
#include "objects.h"


#define THREAD_SLEEP 	200
#define GSTACK 		(16384)
#define GUITH_STACK 	(16384)
#define RIVERSPEED 75
#define TB_MARGIN 10

//Commonly used strings
const char sending_msg[] = "Sending data...";
const char successful_msg[] = "Successful operation!";
const char published_msg[] = "Your tweet has been published";
const char notpublished_msg[] = "Your tweet has not been published";
const char lostcreds_msg[] = "If you do not have it delete this profile and restart the authorization process";
const char pleasewait_msg[] = "Please Wait";
const char yes_msg[] = "Yes";
const char no_msg[] = "No";
const char ok_msg[] = "OK";
const char cancel_msg[] = "Cancel";
const char authorizing_msg[] = "Authorizing WiiTweet...";
const char oktype_msg[] = "OK, I'll type it";
const char unablecon_msg[] = "Unable to communicate with Twitter";
const char exit_msg[] = "Exit";
const char authfail_msg[] = "Authorization failed. You are welcome to try again.";

void shortenUrl(std::string &longUrl, std::string &shortedUrl);
void LoadKeyboardMaps();
int http_request (const char *url, FILE * hfile, u8 * buffer, const u32 max_size, bool silent, bool accept_encoding);

static u8 guistack[GSTACK] ATTRIBUTE_ALIGN (32);
static u8 progressstack[GUITH_STACK] ATTRIBUTE_ALIGN (32);
static lwp_t guithread = LWP_THREAD_NULL;
static lwp_t progressthread = LWP_THREAD_NULL;
static int guiHalt = 1;

static int progressThreadHalt = 1;
bool guiShutdown = true;

static int showProgress = 0;
static char progressTitle[101];
static char progressMsg[201];
static int progressDone = 0;
static int progressTotal = 0;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
ResumeGui()
{
	if(guithread == LWP_THREAD_NULL || guiShutdown)
		return;

	guiHalt = 0;
	LWP_ResumeThread (guithread);
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
HaltGui()
{
	if(guithread == LWP_THREAD_NULL)
		return;

	guiHalt = 1;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(guithread))
		usleep(THREAD_SLEEP);
}

static void StopGuiThreads()
{
	showProgress = 0;
	progressThreadHalt = 2;

	if(progressthread != LWP_THREAD_NULL)
	{
		if(LWP_ThreadIsSuspended(progressthread))
			LWP_ResumeThread (progressthread);

		// wait for thread to finish
		LWP_JoinThread(progressthread, NULL);
		progressthread = LWP_THREAD_NULL;
	}

	guiHalt = 2;

	if(guithread != LWP_THREAD_NULL)
	{
		if(LWP_ThreadIsSuspended(guithread))
			LWP_ResumeThread (guithread);

		// wait for thread to finish
		LWP_JoinThread(guithread, NULL);
		guithread = LWP_THREAD_NULL;
	}
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label)
{
	if(!mainWindow || ExitRequested)
		return 0;

	int choice = -1;

	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);

	GuiImage dialogBoxImg(dialogBox);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 10, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,20);
	GuiText msgTxt(msg, 26, (GXColor){0, 0, 0, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	msgTxt.SetPosition(0,-12);
	msgTxt.SetWrap(true, 430);

	GuiText btn1Txt(btn1Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn1Img(btnOutline);
	GuiImage btn1ImgOver(btnOutlineOver);
	GuiButton btn1(btnOutline->GetWidth(), btnOutline->GetHeight());

	if(btn2Label)
	{
		btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
		btn1.SetPosition(20, -25);
	}
	else
	{
		btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
		btn1.SetPosition(0, -25);
	}

	btn1.SetLabel(&btn1Txt);
	btn1.SetImage(&btn1Img);
	btn1.SetImageOver(&btn1ImgOver);
	btn1.SetTrigger(trigA);
	btn1.SetEffectGrow();

	GuiText btn2Txt(btn2Label, 22, (GXColor){0, 0, 0, 255});
	GuiImage btn2Img(btnOutline);
	GuiImage btn2ImgOver(btnOutlineOver);
	GuiButton btn2(btnOutline->GetWidth(), btnOutline->GetHeight());
	btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	btn2.SetPosition(-20, -25);
	btn2.SetLabel(&btn2Txt);
	btn2.SetImage(&btn2Img);
	btn2.SetImageOver(&btn2ImgOver);
	btn2.SetTrigger(trigA);
	btn2.SetEffectGrow();

	promptWindow.Append(&dialogBoxImg);
	promptWindow.Append(&titleTxt);
	promptWindow.Append(&msgTxt);
	promptWindow.Append(&btn1);

	if(btn2Label)
		promptWindow.Append(&btn2);

	promptWindow.SetEffect(EFFECT_FADE, 50);
	CancelAction();
	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&promptWindow);
	mainWindow->ChangeFocus(&promptWindow);
	if(btn2Label)
	{
		btn1.ResetState();
	}
	ResumeGui();

	while(choice == -1)
	{
		usleep(THREAD_SLEEP);

		if(btn1.GetState() == STATE_CLICKED)
			choice = 1;
		else if(btn2.GetState() == STATE_CLICKED)
			choice = 0;

		if(guiShutdown)
			choice = 0;
	}

	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();
	return choice;
}



/****************************************************************************
 * ProgressWindow
 *
 * Opens a window, which displays progress to the user. Can either display a
 * progress bar showing % completion, or a throbber that only shows that an
 * action is in progress.
 ***************************************************************************/
static int progsleep = 0;

static void
ProgressWindow(char *title, char *msg)
{
	GuiWindow promptWindow(448,288);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);

	GuiImage dialogBoxImg(dialogBox);

	GuiImage progressbarOutlineImg(progressbarOutline);
	progressbarOutlineImg.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
	progressbarOutlineImg.SetPosition(25, 40);

	GuiImage progressbarEmptyImg(progressbarEmpty);
	progressbarEmptyImg.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
	progressbarEmptyImg.SetPosition(25, 40);
	progressbarEmptyImg.SetTile(100);

	GuiImage progressbarImg(progressbar);
	progressbarImg.SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
	progressbarImg.SetPosition(25, 40);

	GuiImage throbberImg(throbber);
	throbberImg.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	throbberImg.SetPosition(0, 40);

	GuiText titleTxt(title, 26, (GXColor){0, 0, 10, 255});
	titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	titleTxt.SetPosition(0,20);
	GuiText msgTxt(msg, 26, (GXColor){0, 0, 0, 255});
	msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	msgTxt.SetPosition(0,80);
	int throbberonly = strcmp(msg, "throbber");

	if(throbberonly)
	{
		promptWindow.Append(&dialogBoxImg);
		promptWindow.Append(&titleTxt);
		promptWindow.Append(&msgTxt);
	}

	if(showProgress == 1)
	{
		promptWindow.Append(&progressbarEmptyImg);
		promptWindow.Append(&progressbarImg);
		promptWindow.Append(&progressbarOutlineImg);
	}
	else
	{
		promptWindow.Append(&throbberImg);
	}

	// wait to see if progress flag changes soon
	progsleep = 400000;

	while(progsleep > 0)
	{
		if(!showProgress)
			break;
		usleep(THREAD_SLEEP);
		progsleep -= THREAD_SLEEP;
	}

	if(!showProgress)
		return;

	HaltGui();
	int oldState = mainWindow->GetState();
	mainWindow->Append(&promptWindow);

	if(throbberonly)
	{
		mainWindow->SetState(STATE_DISABLED);
		mainWindow->ChangeFocus(&promptWindow);
	}

	ResumeGui();

	float angle = 0;
	u32 count = 0;

	while(showProgress && !guiShutdown)
	{
		progsleep = 20000;

		while(progsleep > 0)
		{
			if(!showProgress)
				break;
			usleep(THREAD_SLEEP);
			progsleep -= THREAD_SLEEP;
		}

		if(showProgress == 1)
		{
			progressbarImg.SetTile(100*progressDone/progressTotal);
		}
		else if(showProgress == 2)
		{
			if(count % 5 == 0)
			{
				angle-=45.0f;
				if(angle <= 0)
					angle = 360.0f;
				throbberImg.SetAngle(angle);
			}
			++count;
		}
	}

	HaltGui();
	mainWindow->Remove(&promptWindow);
	mainWindow->SetState(oldState);
	ResumeGui();
}

/****************************************************************************
 * GuiThread
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

static void *
GuiThread (void *arg)
{
	int i;

	while(1)
	{
		if(guiHalt == 1)
			LWP_SuspendThread(guithread);

		if(guiHalt == 2)
			break;

		UpdatePads();
		mainWindow->Draw();

		if (mainWindow->GetState() != STATE_DISABLED)
			mainWindow->DrawTooltip();

		if(userInput[0].wpad->ir.valid)
			Menu_DrawImg(userInput[0].wpad->ir.x-48, userInput[0].wpad->ir.y-48,
				96, 96, pointer->GetImage(), userInput[0].wpad->ir.angle, 1, 1, 255, GX_TF_RGBA8);

		Menu_Render();

		DoRumble(0);
		mainWindow->Update(&userInput[0]);

		if(ExitRequested)
		{
			for(i = 0; i <= 255; i += 15)
			{
				mainWindow->Draw();
				Menu_DrawRectangle(0,0,screenwidth,screenheight,(GXColor){0, 0, 0, i},1);
				Menu_Render();
			}
			guiShutdown = true;
		}
		usleep(THREAD_SLEEP);
	}
	return NULL;
}

static void * ProgressThread (void *arg)
{
	while(1)
	{
		if(progressThreadHalt == 1)
			LWP_SuspendThread (progressthread);
		if(progressThreadHalt == 2)
			return NULL;

		ProgressWindow(progressTitle, progressMsg);
		usleep(THREAD_SLEEP);
	}
	return NULL;
}

/****************************************************************************
 * InitGUIThreads
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads()
{

	showProgress = 0;
	guiHalt = 1;
	progressThreadHalt = 1;

	LWP_CreateThread (&guithread, GuiThread, NULL, guistack, GSTACK, 70);
	LWP_CreateThread (&progressthread, ProgressThread, NULL, progressstack, GUITH_STACK, 60);
}
/****************************************************************************
 * CancelAction
 *
 * Signals the GUI progress window thread to halt, and waits for it to
 * finish. Prevents multiple progress window events from interfering /
 * overriding each other.
 ***************************************************************************/
void
CancelAction()
{
	progressThreadHalt = 1;
	showProgress = 0;

	if(progressthread == LWP_THREAD_NULL)
		return;

	// wait for thread to finish
	while(!LWP_ThreadIsSuspended(progressthread))
		usleep(THREAD_SLEEP);
}

/****************************************************************************
 * ShowProgress
 *
 * Updates the variables used by the progress window for drawing a progress
 * bar. Also resumes the progress window thread if it is suspended.
 ***************************************************************************/
extern "C" {
void
ShowProgress (const char *msg, int done, int total)
{
	if(progressthread == LWP_THREAD_NULL || guiShutdown)
		return;

	if(!mainWindow || ExitRequested )
		return;

	if(total <= 0 || done < 0) // invalid values
		return;

	if(done > total) // this shouldn't happen
		done = total;

	if(showProgress != 1)
		CancelAction(); // wait for previous progress window to finish

	snprintf(progressMsg, 200, "%s", msg);
	sprintf(progressTitle, pleasewait_msg);
	showProgress = 1;
	progressThreadHalt = 0;
	progressTotal = total;
	progressDone = done;
	LWP_ResumeThread (progressthread);
}
}

/****************************************************************************
 * ShowAction
 *
 * Shows that an action is underway. Also resumes the progress window thread
 * if it is suspended.
 ***************************************************************************/
void
ShowAction (const char *msg)
{
	if(!mainWindow || ExitRequested)
		return;

	if(progressthread == LWP_THREAD_NULL || guiShutdown)
		return;

	if(showProgress != 0)
		CancelAction(); // wait for previous progress window to finish

	if(msg){
		snprintf(progressMsg, 200, "%s", msg);
	}else{
		sprintf(progressMsg, "throbber");
	}
	sprintf(progressTitle, pleasewait_msg);
	progressThreadHalt = 0;
	showProgress = 2;
	progressDone = 0;
	progressTotal = 0;
	LWP_ResumeThread (progressthread);
}

void ErrorPrompt(const char *msg)
{
	WindowPrompt("Error", msg, ok_msg, NULL);
}

int ErrorPromptRetry(const char *msg)
{
	return WindowPrompt("Error", msg, "Retry", cancel_msg);
}

void InfoPrompt(const char *msg)
{
	WindowPrompt("Information", msg, ok_msg, NULL);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
int OnScreenKeyboard(char * var, u32 maxlen, int t = 0)
{
	if(!mainWindow || ExitRequested)
		return 0;

	int save = -1;

	GuiKeyboard keyboard(var, maxlen, t);

	GuiText okBtnTxt(ok_msg, 22, (GXColor){0, 0, 0, 255});
	if(t) okBtnTxt.SetText("Tweet");
	GuiText cancelBtnTxt(cancel_msg, 22, (GXColor){0, 0, 0, 255});

	GuiImage okBtnImg(btnOutline);
	GuiImage okBtnImgOver(btnOutlineOver);
	GuiButton okBtn(btnOutline->GetWidth(), btnOutline->GetHeight());

	okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	okBtn.SetPosition(30, -25);
	okBtn.SetLabel(&okBtnTxt);
	okBtn.SetImage(&okBtnImg);
	okBtn.SetImageOver(&okBtnImgOver);
	okBtn.SetTrigger(trigA);

	GuiImage cancelBtnImg(btnOutline);
	GuiImage cancelBtnImgOver(btnOutlineOver);
	GuiButton cancelBtn(btnOutline->GetWidth(), btnOutline->GetHeight());
	cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	cancelBtn.SetPosition(-30, -25);
	cancelBtn.SetLabel(&cancelBtnTxt);
	cancelBtn.SetImage(&cancelBtnImg);
	cancelBtn.SetImageOver(&cancelBtnImgOver);
	cancelBtn.SetTrigger(trigA);

	keyboard.Append(&okBtn);
	keyboard.Append(&cancelBtn);

	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(disabled);
	mainWindow->Append(&keyboard);
	ResumeGui();

	while(save == -1)
	{
		usleep(THREAD_SLEEP);

		if(okBtn.GetState() == STATE_CLICKED)
			save = 1;
		else if(cancelBtn.GetState() == STATE_CLICKED)
			save = 0;
		else if(ExitRequested || guiShutdown)
			save = 0;
	}

	if(save || t)
	{
		snprintf(var, maxlen, "%s", keyboard.kbtextstr);
	}

	HaltGui();
	mainWindow->Remove(&keyboard);
	mainWindow->Remove(disabled);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();

return save;
}

int nonEmptyOSK(char * var, u32 maxlen, int t = 0){
	while(!guiShutdown){
		memset(var, 0, maxlen);
		if(OnScreenKeyboard(var, maxlen, t)){
			if(var[0] == '\0'){
				ErrorPrompt("This field can not be empty. Please type something or press Cancel.");
				continue;
			}else{
				return 1;
			}
		}else{
			break;
		}
	}
return 0;
}

bool OnScreenKeypad(char *var, u32 maxlen, bool enableNegative = false)
{
	if(!mainWindow || ExitRequested)
		return 0;

	int save = -1;

	GuiKeypad keypad(var, maxlen);

	if(enableNegative)
		keypad.EnableNegative();

	GuiText okBtnTxt(ok_msg, 20, (GXColor){0, 0, 0, 255});
	GuiImage okBtnImg(btnOutline);
	GuiImage okBtnImgOver(btnOutlineOver);
	GuiButton okBtn(btnOutline->GetWidth(), btnOutline->GetHeight());

	okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	okBtn.SetPosition(25, -25);

	okBtn.SetLabel(&okBtnTxt);
	okBtn.SetImage(&okBtnImg);
	okBtn.SetImageOver(&okBtnImgOver);
	okBtn.SetTrigger(trigA);
	okBtn.SetEffectGrow();

	GuiText cancelBtnTxt(cancel_msg, 20, (GXColor){0, 0, 0, 255});
	GuiImage cancelBtnImg(btnOutline);
	GuiImage cancelBtnImgOver(btnOutlineOver);
	GuiButton cancelBtn(btnOutline->GetWidth(), btnOutline->GetHeight());
	cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	cancelBtn.SetPosition(-25, -25);
	cancelBtn.SetLabel(&cancelBtnTxt);
	cancelBtn.SetImage(&cancelBtnImg);
	cancelBtn.SetImageOver(&cancelBtnImgOver);
	cancelBtn.SetTrigger(trigA);
	cancelBtn.SetEffectGrow();

	keypad.Append(&okBtn);
	keypad.Append(&cancelBtn);

	HaltGui();
	mainWindow->SetState(STATE_DISABLED);
	mainWindow->Append(&keypad);
	ResumeGui();

	while(save == -1)
	{
		usleep(THREAD_SLEEP);

		if(okBtn.GetState() == STATE_CLICKED)
			save = 1;
		else if(cancelBtn.GetState() == STATE_CLICKED)
			save = 0;
		else if(ExitRequested || guiShutdown)
			save = 0;
	}

	if(save)
		snprintf(var, maxlen+1, "%s", keypad.kptextstr);

	HaltGui();
	mainWindow->Remove(&keypad);
	mainWindow->SetState(STATE_DEFAULT);
	ResumeGui();

	return save;
}

void setNIP(class wt_profile *profile){
	char NIP[21]= { 0 };

	if( WindowPrompt("Set local password?", "It is used to secure your local profile.", yes_msg, no_msg))
	{
			OnScreenKeyboard(NIP, 20);

			if(NIP[0]){
				profile->NIP = 0xff;
			}
	}

	if(guiShutdown) return;
	std::string myOAuthAccessTokenKey("");
	std::string myOAuthAccessTokenSecret("");
	profile->twitterObj->getOAuth().getOAuthTokenKey( myOAuthAccessTokenKey );
	profile->twitterObj->getOAuth().getOAuthTokenSecret( myOAuthAccessTokenSecret );
	profile->tokenKeyLen = myOAuthAccessTokenKey.length();
	profile->tokenSecretLen = myOAuthAccessTokenSecret.length();

	if(profile->NIP){//HAVE NIP
		profile->setTokenSecret(myOAuthAccessTokenSecret, NIP);
		profile->setTokenKey(myOAuthAccessTokenKey, NIP);
	}else{//NO NIP
		profile->setTokenSecret(myOAuthAccessTokenSecret, 0);
		profile->setTokenKey(myOAuthAccessTokenKey, 0);
	}
}

void createProfile( class wt_profile **profileptr, int havedevice = 1 ){

	int stay = 1;
	char newname[65] = {0};
	if(havedevice){
		WindowPrompt("WiiTweet authorization", "The following steps are only required once", ok_msg, 0);
		WindowPrompt("Local username", "This will be used to recognize you within WiiTweet","Type it",0);

		while(stay && !guiShutdown){
			stay = 0;
			memset(newname, 0, 65);
			if(nonEmptyOSK(newname, 64)){
				for(int i=0; i<browser.numEntries; i++){
					if(!strcmp(browserList[i].displayname, newname)){
						stay = WindowPrompt("Existing name", "Overwrite the profile using it?", "No, retype name", "Yes, overwrite it");
						break;
					}
				}
			}else{
				stay = 0;
			}
		}
	}else{
		WindowPrompt("No SD/USB mode", "Could not find witweet folder. You are still able to use WiiTweet but your profile won't be saved.", ok_msg, 0); //wiiload or did changed the wiitweet folder name
	}

	if(newname[0] == '\0' && havedevice){
		return;
	}

	class wt_profile * profile = new wt_profile(newname);
	profile->oauth = 0;
	{
		std::string longUrl;
		if(guiShutdown){ profile->save = 0; delete profile; return; }

		ShowAction("Connecting to Twitter...");
		profile->twitterObj->oAuthRequestToken(longUrl);
		CancelAction();

		if(WindowPrompt("Authorization options", "You can enter your Twitter's username/password or visit an URL to get a PIN.", "URL way", "Password way")){
		//PIN authorization
			int opera = havedevice && !WindowPrompt("URL options", "How do you want to visit the URL?", "External device", "Internet Channel");

			if(opera && WindowPrompt("Internet channel confirmation", "Are you sure you have the channel installed?", "Yes, continue","No, I don't know")){
				std::string tStr;
				profile->twitterObj->getOAuth().getOAuthTokenKey(tStr);
				profile->setTokenKey(tStr, 0);
				profile->twitterObj->getOAuth().getOAuthTokenSecret(tStr);
				profile->setTokenSecret(tStr, 0);
				profile->twitterObj->getOAuth().getOAuthScreenName(tStr);
				profile->setScreenName(tStr);

				profile->oauth = 1;
				profile->save = 1;
				delete profile;  profile = 0;//Save before leaving

				if(guiShutdown){ profile->save = 0; delete profile; return; }
				WII_OpenURL(longUrl.c_str());

				profile = new wt_profile(newname, 0); //Reopen profile - WiiOpera is not installed TODO: This crashes when WiiOpera is not installed ask around...
				WindowPrompt("WiiOpera error", "Failed to load Internet Channel. Bounced onto the external device method.", "Get the URL", 0);
			}

			std::string shortURL;

			if(guiShutdown){ profile->save = 0; delete profile; return; }
			shortenUrl(longUrl, shortURL);
			WindowPrompt("Get your PIN at:", shortURL.c_str(), "Got it", 0);
			std::string strPIN;
			char PIN[21]="";
			if(OnScreenKeypad(PIN,20) && strlen(PIN)){
				strPIN = PIN;
				profile->twitterObj->getOAuth().setOAuthPin(strPIN);
				ShowAction(authorizing_msg);
			}else{
				profile->save = 0; delete profile; return;
			}
		}else{
		//Password authorization
			char buf[65];
			std::string TuserName("");
			std::string TpassWord("");

			if(havedevice){
				if(!WindowPrompt(newname, "Is the name above your Twitter username?", yes_msg, "No, I'll type it")){

					if(nonEmptyOSK(buf, 64)){
						TuserName.assign(buf);					
					}else{
						profile->save = 0; delete profile; return;
					}

				}else{
					TuserName.assign(newname);
				}
			}else{
				WindowPrompt("Twitter username", "WiiTweet needs your username.", oktype_msg, 0);
				if(nonEmptyOSK(buf, 64)){
					TuserName.assign(buf);					
				}else{
					profile->save = 0; delete profile; return;
				}
			}
			WindowPrompt("Twitter password", "WiiTweet needs your password. It's not stored anywhere!", oktype_msg, 0);

			if(nonEmptyOSK(buf, 64)){
				TpassWord.assign(buf);
			}else{
				profile->save = 0; delete profile; return;
			}

			profile->twitterObj->setTwitterUsername( TuserName );
			profile->twitterObj->setTwitterPassword( TpassWord );
			if(guiShutdown){ profile->save = 0; delete profile; return; }
			ShowAction(authorizing_msg);
			if(!profile->twitterObj->oAuthHandlePIN(longUrl)){
				ErrorPrompt(authfail_msg);
				profile->save = 0; delete profile; return;
			}
		}

		if(profile){
			if(guiShutdown){ profile->save = 0; delete profile; return; }

			if(profile->twitterObj->oAuthAccessToken()){
				CancelAction();
				profile->oauth = 2;
				if(havedevice){
					setNIP(profile);
					profile->save = 1;
					InfoPrompt("Looks like everything went fine. This account should be ready to use.");
				}
			}else{
				CancelAction();
				profile->save = 0;
				ErrorPrompt(authfail_msg);
			}
		}
	}

	*profileptr = profile;
}


int AppendLoadMore(char &endpoint){
if(endpoint == CREDITS) return -1;

return 1;
}

struct t_arl * BriefTweet(struct t_tweet * tweet, class wt_profile * profile, class ARLs * history, struct t_login * login)
{
	if(!mainWindow || ExitRequested)
		return 0;

	GuiInfomsg Info;
	struct t_arl * temp = NULL;

	int height;
	int entities = tweet->hashtags_count+tweet->mentions_count;

	GuiButton entitiesbtns[entities];
	GuiText entitiestxts[entities];
	GuiImage entitiesimgs[entities*2];

	GuiRiverUnit UserBlock(UNIT_USER | BRIEFMODE, &(tweet->user), NULL);
	UserBlock.SetPosition(0, 0);
	UserBlock.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	GuiRiverUnit TweetBlock(UNIT_TWEET | BRIEFMODE, tweet, NULL);
	TweetBlock.SetPosition(0, UserBlock.GetHeight() + 5);
	TweetBlock.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);

	GuiTrigger backTrigger;
	backTrigger.SetButtonOnlyTrigger(-1, WPAD_BUTTON_HOME, 0);
	GuiButton backBtn(0, 0);
	backBtn.SetTrigger(&backTrigger);

	int entitiesrows = 0;
	if(entities){
		entitiesrows = entities/3;
		if(entities % 3){
			entitiesrows++;
		}
	}
	height = UserBlock.GetHeight() + TweetBlock.GetHeight() + entitiesrows * (sbtnOutline->GetHeight() + 2) + 10;

	GuiImage top(riverunitTop);
	GuiImage tile(riverunitTile);
	GuiImage bottom(riverunitBottom);
	top.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	top.SetPosition(0, 0);
	tile.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	tile.SetPosition(0, top.GetHeight());
	int tiletimes = (height-top.GetHeight()*2)/tile.GetHeight();
	tile.SetTileVertical(tiletimes);
	bottom.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
	bottom.SetPosition(0, top.GetHeight() + tiletimes*tile.GetHeight());

	GuiWindow promptWindow(500, height);
	promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	promptWindow.SetPosition(0, -10);

	promptWindow.Append(&top); promptWindow.Append(&tile); promptWindow.Append(&bottom);
	promptWindow.Append(&UserBlock);
	promptWindow.Append(&TweetBlock);
	promptWindow.Append(&backBtn);
	promptWindow.SetEffect(EFFECT_SLIDE_IN | EFFECT_SLIDE_TOP, 50);

	for(int i = 0; i < entities; i++){
		if(i < tweet->hashtags_count){
			entitiestxts[i].SetText(tweet->hashtags[i].text);
		}else{
			entitiestxts[i].SetText(tweet->mentions[i-tweet->hashtags_count].screenname);
		}
		entitiestxts[i].SetFontSize(16);
		entitiesimgs[2*i].SetImage(sbtnOutline);
		entitiesimgs[2*i+1].SetImage(sbtnOutlineOver);
		entitiestxts[i].SetMaxWidth(sbtnOutline->GetWidth()-6);
		entitiestxts[i].SetScroll(SCROLL_HORIZONTAL);
		entitiesbtns[i].SetLabel(&entitiestxts[i]);
		entitiesbtns[i].SetTrigger(trigA);
		entitiesbtns[i].SetImage(&entitiesimgs[2*i]);
		entitiesbtns[i].SetImageOver(&entitiesimgs[2*i+1]);
		entitiesbtns[i].SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
		entitiesbtns[i].SetPosition( i % 3 == 0 ? -(sbtnOutlineOver->GetWidth()+4) : ( i % 3 == 1 ? 0 : (sbtnOutlineOver->GetWidth()+4)), UserBlock.GetHeight() + TweetBlock.GetHeight() + 10 + ((2 + sbtnOutline->GetHeight()) * (int)(i / 3)));
		entitiesbtns[i].SetSize(sbtnOutline->GetWidth(), sbtnOutline->GetHeight());
		promptWindow.Append(&entitiesbtns[i]);
	}

	HaltGui();
	mainWindow->SetFocus(0);
	mainWindow->Append(disabled);
	mainWindow->Append(&promptWindow);
	mainWindow->Append(&Info);
	ResumeGui();

	int update = 0;
	CancelAction();
	if(!(tweet->user.flags & FOLLOWMASK)){ //Only check if we don't know if we are following it...
		std::string id_a, id_b;
		char itoa[64];
		sprintf(itoa, "%llu", login->userid); id_a = itoa;
		sprintf(itoa, "%llu", tweet->user.id); id_b = itoa;

		ShowAction("throbber");
		if(profile->twitterObj->friendshipExists(id_a, id_b)){
			UserBlock.SetFollow(1, &(tweet->user)); 
		}else{
			UserBlock.SetFollow(0, &(tweet->user));
		}
		CancelAction();
	}

	while(!guiShutdown && !update)
	{
		if(backBtn.GetState() == STATE_CLICKED ){
			usleep(THREAD_SLEEP*10);
			backBtn.ResetState();
			break;
		}else if(TweetBlock.buttons[T_BTN_FAV].GetState() == STATE_CLICKED){
			ShowAction(sending_msg);
			TweetBlock.DoFavorite(profile, tweet);
			TweetBlock.buttons[T_BTN_FAV].ResetState();
			CancelAction();
		}else if(TweetBlock.buttons[T_BTN_RT].GetState() == STATE_CLICKED){
			ShowAction(sending_msg);
			TweetBlock.DoRetweet(profile, tweet);
			TweetBlock.buttons[T_BTN_RT].ResetState();
			CancelAction();
		}else if(TweetBlock.buttons[T_BTN_REPLY].GetState() == STATE_CLICKED){
			char Tweet[512] = { 0 };
			sprintf(Tweet,"%s ", tweet->user.screenname);
			for(int r = 0; r < tweet->mentions_count; r++){
				if(strcmp(tweet->mentions[r].screenname, login->screenname)) //Do not include the current user if it is mentioned
					sprintf(Tweet,"%s%s ",Tweet, tweet->mentions[r].screenname);
			}
			if(OnScreenKeyboard(Tweet, 140, 1) && Tweet[0]){
				std::string tweetmsg(Tweet);
				ShowAction(sending_msg);
				char itoa[64];
				sprintf(itoa, "%llu", tweet->id);
				std::string replyTo(itoa);
				if(profile->twitterObj->statusUpdate( tweetmsg, &replyTo )){
					Info.Display(published_msg);
					strcpy(Tweet,"");
				}else{
					ErrorPrompt(unablecon_msg);
				}
				CancelAction();
			}else{
					Info.Display(notpublished_msg);
			}
			TweetBlock.buttons[T_BTN_REPLY].ResetState();
		}else if(UserBlock.buttons[U_BTN_FOLLOW].GetState() == STATE_CLICKED){
			ShowAction(sending_msg);
			UserBlock.DoFollow(profile, &(tweet->user));
			UserBlock.buttons[U_BTN_FOLLOW].ResetState();
			UserBlock.buttons[U_BTN_FG].ResetState(); //This overlaps
			CancelAction();
		}else if(UserBlock.buttons[U_BTN_FAVOURITES].GetState() == STATE_CLICKED){
			temp = history->Build(SHOW_FAVOURITES, tweet->user.id, 0, 0, NULL);
			UserBlock.buttons[U_BTN_FAVOURITES].ResetState();
			update = 1;
			break;
		}else if(UserBlock.buttons[U_BTN_FOLLOWING].GetState() == STATE_CLICKED){
			temp = history->Build(SHOW_FOLLOWING, tweet->user.id, 0, 0, NULL);
			UserBlock.buttons[U_BTN_FOLLOWING].ResetState();
			update = 1;
			break;
		}else if(UserBlock.buttons[U_BTN_FOLLOWERS].GetState() == STATE_CLICKED){
			temp = history->Build(SHOW_FOLLOWERS, tweet->user.id, 0, 0, NULL);
			UserBlock.buttons[U_BTN_FOLLOWERS].ResetState();
			update = 1;
			break;
		}else if(UserBlock.buttons[U_BTN_FG].GetState() == STATE_CLICKED || TweetBlock.buttons[T_BTN_TIMELINE].GetState() == STATE_CLICKED ){
			temp = history->Build(TIMELINE_USER, tweet->user.id, 0, 0, NULL);
			UserBlock.buttons[U_BTN_FG].ResetState();
			TweetBlock.buttons[T_BTN_TIMELINE].ResetState();
			update = 1;
			break;
		}

		for(int i = 0; i < entities; i++){
			if(entitiesbtns[i].GetState() == STATE_CLICKED){
				if(i < tweet->hashtags_count){
					temp = history->Build(SEARCH_TWEET, 0, 0, 0, tweet->hashtags[i].text);
					update = 1;
					break;
				}else{
					temp = history->Build(TIMELINE_USER, tweet->mentions[i-tweet->hashtags_count].id, 0, 0, NULL);
					update = 1;
					break;
				}
			}
		}
		usleep(THREAD_SLEEP);

	}

	promptWindow.SetEffect(EFFECT_SLIDE_OUT | EFFECT_SLIDE_BOTTOM, 50);
	while(promptWindow.GetEffect() > 0 && !guiShutdown) usleep(THREAD_SLEEP);

	HaltGui();
	mainWindow->SetFocus(1);
	mainWindow->Remove(disabled);
	mainWindow->Remove(&promptWindow);
	mainWindow->Remove(&Info);
	ResumeGui();

	return temp;
}


void BrowseTwitter(class wt_profile * profile, struct t_login * login){
 u8 logout = 0;
 std::string http_res;
 	GuiRiver river;
 	river.SetClickable(1); //This flag is used to tell GuiRiver there is a Toolbar
	GuiInfomsg Info;
	GuiToolbar toolbar;

	GuiTrigger backTrigger;
	backTrigger.SetButtonOnlyTrigger(-1, WPAD_BUTTON_MINUS | WPAD_CLASSIC_BUTTON_MINUS, 0);
	GuiButton backBtn(0, 0);
	backBtn.SetTrigger(&backTrigger);

	GuiTrigger fwdTrigger;
	fwdTrigger.SetButtonOnlyTrigger(-1, WPAD_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_PLUS, 0);
	GuiButton fwdBtn(0, 0);
	fwdBtn.SetTrigger(&fwdTrigger);

	GuiImage TweetBtnImg(tbTweet);
	GuiImage TweetBtnImgOver(tbTweetOver);
	GuiTooltip TweetBtnTTip("Tweet");
	GuiButton TweetBtn(48, 48);
	TweetBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
	TweetBtn.SetPosition(-100, -10);
	TweetBtn.SetImage(&TweetBtnImg);
	TweetBtn.SetImageOver(&TweetBtnImgOver);
	TweetBtn.SetTooltip(&TweetBtnTTip);
	TweetBtn.SetTrigger(trigA);
	TweetBtn.SetEffectGrow();

	int btn_x = 0;
	GuiImage HomeBtnImg(tbHome);
	GuiImage HomeBtnImgOver(tbHomeOver);
	GuiTooltip HomeBtnTTip("Home");
	GuiButton HomeBtn(48, 48);
	HomeBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	HomeBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	HomeBtn.SetImage(&HomeBtnImg);
	HomeBtn.SetImageOver(&HomeBtnImgOver);
	HomeBtn.SetTooltip(&HomeBtnTTip);
	HomeBtn.SetTrigger(trigA);
	HomeBtn.SetEffectGrow();

	GuiImage MentionsBtnImg(tbMentions);
	GuiImage MentionsBtnImgOver(tbMentionsOver);
	GuiTooltip MentionsBtnTTip("Mentions");
	GuiButton MentionsBtn(48, 48);
	MentionsBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	MentionsBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	MentionsBtn.SetImage(&MentionsBtnImg);
	MentionsBtn.SetImageOver(&MentionsBtnImgOver);
	MentionsBtn.SetTooltip(&MentionsBtnTTip);
	MentionsBtn.SetTrigger(trigA);
	MentionsBtn.SetEffectGrow();

	GuiImage SearchBtnImg(tbSearch);
	GuiImage SearchBtnImgOver(tbSearchOver);
	GuiTooltip SearchBtnTTip("Search");
	GuiButton SearchBtn(48, 48);
	SearchBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	SearchBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	SearchBtn.SetImage(&SearchBtnImg);
	SearchBtn.SetImageOver(&SearchBtnImgOver);
	SearchBtn.SetTooltip(&SearchBtnTTip);
	SearchBtn.SetTrigger(trigA);
	SearchBtn.SetEffectGrow();

	GuiImage SwitchuserBtnImg(tbSwitchuser);
	GuiImage SwitchuserBtnImgOver(tbSwitchuserOver);
	GuiTooltip SwitchuserBtnTTip("Log out");
	GuiButton SwitchuserBtn(48, 48);
	SwitchuserBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	SwitchuserBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	SwitchuserBtn.SetImage(&SwitchuserBtnImg);
	SwitchuserBtn.SetImageOver(&SwitchuserBtnImgOver);
	SwitchuserBtn.SetTooltip(&SwitchuserBtnTTip);
	SwitchuserBtn.SetTrigger(trigA);
	SwitchuserBtn.SetEffectGrow();

/*
	GuiImage SettingsBtnImg(tbSettings);
	GuiImage SettingsBtnImgOver(tbSettingsOver);
	GuiTooltip SettingsBtnTTip("Settings");
	GuiButton SettingsBtn(48, 48);
	SettingsBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	SettingsBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	SettingsBtn.SetImage(&SettingsBtnImg);
	SettingsBtn.SetImageOver(&SettingsBtnImgOver);
	SettingsBtn.SetTooltip(&SettingsBtnTTip);
	SettingsBtn.SetTrigger(trigA);
	SettingsBtn.SetEffectGrow();
*/

	GuiImage AboutBtnImg(tbCredits);
	GuiImage AboutBtnImgOver(tbCreditsOver);
	GuiTooltip AboutBtnTTip("About");
	GuiButton AboutBtn(48, 48);
	AboutBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
	AboutBtn.SetPosition(100+(btn_x++)*(48+TB_MARGIN), -10);
	AboutBtn.SetImage(&AboutBtnImg);
	AboutBtn.SetImageOver(&AboutBtnImgOver);
	AboutBtn.SetTooltip(&AboutBtnTTip);
	AboutBtn.SetTrigger(trigA);
	AboutBtn.SetEffectGrow();

	GuiText LoadmoreTxt("Load more", 24, (GXColor){0, 0, 0, 255});
	GuiButton LoadmoreBtn(btnOutline->GetWidth(), btnOutline->GetHeight());
	LoadmoreBtn.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	LoadmoreBtn.SetPosition(0, 0);
	LoadmoreBtn.SetLabel(&LoadmoreTxt);
	GuiImage LoadmoreBtnImg(btnOutline);
	GuiImage LoadmoreBtnImgOver(btnOutlineOver);
	LoadmoreBtn.SetImage(&LoadmoreBtnImg);
	LoadmoreBtn.SetImageOver(&LoadmoreBtnImgOver);
	LoadmoreBtn.SetTrigger(trigA);
	LoadmoreBtn.SetEffectGrow();

	GuiText EmptyTxt("No tweets/users found! Press (-) to go back.", 24, (GXColor){0x10, 0x10, 0x10, 255});
	EmptyTxt.SetWrap(true, 450);
	EmptyTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
	EmptyTxt.SetPosition(0, 0);

	HaltGui();
	toolbar.Append(&TweetBtn);
	toolbar.Append(&SwitchuserBtn);
	toolbar.Append(&HomeBtn);
	toolbar.Append(&MentionsBtn);
	toolbar.Append(&AboutBtn);
//	toolbar.Append(&SettingsBtn);
	toolbar.Append(&SearchBtn);

	toolbar.Append(&fwdBtn);
	toolbar.Append(&backBtn);
	mainWindow->Append(&river);
	mainWindow->Append(&toolbar);
	mainWindow->Append(&Info);
	ResumeGui();

	struct t_tweet * tweets = NULL;
	struct t_user * users = NULL;
	struct cached_imgs * cache_session = NULL;
	struct t_arl * arl = NULL;
	class ARLs * history = new ARLs(25);
	arl = history->Build(TIMELINE_HOME, 0, 0, 0, NULL);
	GuiRiverUnit ** RUnits = NULL;
	char Tweet[512]="";
	u8 backwards = 0;
	u8 unsuccess = 0;
	while(!guiShutdown && !logout){
		ShowAction(sending_msg);

		u8 success = 0;
		u8 update = 0;
		u8 rivertweets = 0;
		u8 riverusers = 0;

		std::string str_id(""), str_aid(""), str_aid2(""), str_txt("");
		{
			char itoa[64];

			if(arl->user_id){
				sprintf(itoa, "%llu", (arl->user_id));
				str_id = itoa;
			}

			if(arl->arg_id){
				sprintf(itoa, "%llu", (arl->arg_id));
				str_aid = itoa;
			}

			if(arl->arg_id_2){
				sprintf(itoa, "%llu", (arl->arg_id_2));
				str_aid2 = itoa;
			}
		}
		if(arl->text){
			str_txt = arl->text;
		}
		switch(arl->endpoint){ //This describes the target endpoint of the API to query

			case TIMELINE_HOME:
				success = profile->twitterObj->timelineFriendsGet(str_aid);
			break;
			case TIMELINE_USER:
				if(arl->user_id){
					success = profile->twitterObj->timelineUserGet(0, 20, str_aid, str_id, 1);
				}else{
					success = profile->twitterObj->timelineUserGet(0, 20, str_aid, str_txt, 0);
				}
			break;
			case SHOW_FAVOURITES:
				success = profile->twitterObj->favoriteGet(str_id, 1, str_aid);
			break;
			case SEARCH_TWEET:
				success = profile->twitterObj->search(str_txt, str_aid);
			break;
			case SEARCH_USER:
				success = profile->twitterObj->usersSearch(str_txt, str_aid);
			break;
			case SHOW_FOLLOWING:
			case CREDITS:
				if(profile->twitterObj->friendsIdsGet(str_id, 1)){
				profile->twitterObj->getLastWebResponse(http_res);
				char * ids = parseIDs(http_res.c_str(), arl->arg_id);
					if(ids){
						str_id = ids;
						mem2_free(ids, MEM2_OTHER);
						success = profile->twitterObj->usersLookup(str_id, 1);
					}
				}
			break;
			case SHOW_FOLLOWERS:
				if(profile->twitterObj->followersIdsGet(str_id, 1)){
				profile->twitterObj->getLastWebResponse(http_res);
				char * ids = parseIDs(http_res.c_str(), arl->arg_id);
					if(ids){
						str_id = ids;
						mem2_free(ids, MEM2_OTHER);
						success = profile->twitterObj->usersLookup(str_id, 1);
					}
				}
			break;
			case USER_MENTIONS:
				success = profile->twitterObj->mentionsGet(str_aid);
			break;
		}
		if(success){
			profile->twitterObj->getLastWebResponse(http_res);
			switch(arl->endpoint){
				case TIMELINE_HOME: //We are going to display tweets
				case TIMELINE_USER:
				case SHOW_FAVOURITES:
				case USER_MENTIONS:
					rivertweets = parseTweets(http_res, &tweets);
				break;
				case SHOW_FOLLOWING:	//We are going to display users
				case SHOW_FOLLOWERS:
				case SEARCH_USER:
				case CREDITS:
					riverusers = parseTweets(http_res, &tweets, 1);
				break;
				case SEARCH_TWEET:
					rivertweets = parseSearch(http_res, &tweets);
				break;
			}
			CancelAction();

		}else{
			if(unsuccess++ == 0){ //Error warn the user
				if(WindowPrompt("Error", "Display debug information? This may be a connection problem but let @_paguiar know about this if you can reproduce it.", yes_msg, no_msg)){
					char itoa[256];
					sprintf(itoa, "%d", arl->endpoint);
					InfoPrompt(itoa); InfoPrompt(str_id.c_str()); InfoPrompt(str_aid.c_str()); InfoPrompt(str_aid2.c_str());
					if(arl->text){ InfoPrompt(arl->text); }else{ InfoPrompt("!arg->text"); }
				}
				continue;
			}else if(unsuccess == 2){
				unsuccess = 0;
				history->Clear();
				arl = history->Build(TIMELINE_HOME, 0, 0, 0, NULL);
			}
		}

		int riverunits = rivertweets+riverusers;

		if(!riverunits){
			river.Append(&EmptyTxt);
		}

		int rus = 0;
		river.SetPosition(0, 45); //Return to the top

		while(!guiShutdown && !logout && !update){
		/* Appending happens here so the user can click things while images are being downloaded */
			if(rus < riverunits){
				ShowAction("throbber");
				if(rus == 0){
					cache_session_init(&cache_session);
					river.SetEffect((EFFECT_SLIDE_RIGHT << backwards) | EFFECT_SLIDE_IN, RIVERSPEED);
					RUnits = (GuiRiverUnit **)mem2_calloc(riverusers+rivertweets, sizeof(GuiRiverUnit *), MEM2_OTHER);
					backwards = 0;
				}

				if(rus < riverusers){
					RUnits[rus] = new GuiRiverUnit(UNIT_USER, &(tweets[rus].user), cache_session);
				}else{
					RUnits[rus] = new GuiRiverUnit(UNIT_TWEET, &tweets[rus-riverusers], cache_session);
				}
				river.Append(RUnits[rus]);
				if(rus + 1 == riverunits){
					if(AppendLoadMore(arl->endpoint) == 1){
						river.Append(&LoadmoreBtn);
					}else{
						river.Append(disclaimer);
					}
					cache_session_destroy(&cache_session);
					CancelAction();
				}
				rus++;
			}

		/* < Non RU Buttons > */
			if(TweetBtn.GetState() == STATE_CLICKED){
				CancelAction();
				
				if(OnScreenKeyboard(Tweet, 140, 1) && Tweet[0]){
					std::string tweet(Tweet);
					ShowAction(sending_msg);
					if(profile->twitterObj->statusUpdate( tweet, NULL)){
						Info.Display(published_msg);
						strcpy(Tweet,"");
					}else{
						ErrorPrompt(unablecon_msg);
					}
					CancelAction();
				}else{
					Info.Display(notpublished_msg);
				}
				TweetBtn.ResetState();
			}else if(SwitchuserBtn.GetState() == STATE_CLICKED){
				if(WindowPrompt("Exiting", "Do you want to log out from this profile?", "Log out", cancel_msg)){
					logout = 1;
				}
				SwitchuserBtn.ResetState();
				break;
			}else if(HomeBtn.GetState() == STATE_CLICKED){
				arl = history->Build(TIMELINE_HOME, 0, 0, 0, NULL);
				HomeBtn.ResetState();
				update = 1;
				break;
			}else if(MentionsBtn.GetState() == STATE_CLICKED){
				arl = history->Build(USER_MENTIONS, 0, 0, 0, NULL);
				MentionsBtn.ResetState();
				update = 1;
				break;
			}else if(SearchBtn.GetState() == STATE_CLICKED){
				CancelAction();
				char query[256]="";
				if(nonEmptyOSK(query, 150)){
					if(WindowPrompt("Search target", "What are you looking for?", "People", "Tweets")){
						arl = history->Build(SEARCH_USER, 0, 1, 0, query);
					}else{
						arl = history->Build(SEARCH_TWEET, 0, 0, 0, query);
					}
					SearchBtn.ResetState();
					update = 1;
					break;
				}
				SearchBtn.ResetState();
			}else if(AboutBtn.GetState() == STATE_CLICKED){
				u64 wiitwiity = 392315277;
				arl = history->Build(CREDITS, wiitwiity, 0, 0, NULL);
				AboutBtn.ResetState();
				update = 1;
				break;
		/* </ Toolbar buttons > */
			}else if(backBtn.GetState() == STATE_CLICKED){ 
				arl = history->Previous();
				if(arl){
					update = 1;
					backwards = 1;
				}
				backBtn.ResetState();
			}else if(fwdBtn.GetState() == STATE_CLICKED){
				arl = history->Next();
				if(arl){
					update = 1;
				}
				fwdBtn.ResetState();
			}else if(LoadmoreBtn.GetState() == STATE_CLICKED){
				switch(arl->endpoint){
					case SHOW_FOLLOWING:
					case SHOW_FOLLOWERS:
					case SEARCH_USER:
						arl = history->Build(arl->endpoint, arl->user_id, arl->arg_id + 1, arl->arg_id_2, arl->text);
					break;
					default:
						arl = history->Build(arl->endpoint, arl->user_id, tweets[rivertweets-1].id - 1, arl->arg_id_2, arl->text);
					break;
				}
				update = 1;
				LoadmoreBtn.ResetState();
			}
		/* </ Non RU Buttons >*/
			int i = 0;
			for(i = 0; i < rus; i++){
				if(RUnits[i]->IsVisible()){ break; }
			}

			for(; i < rus; i++){
				switch(RUnits[i]->ruType & 0xf){
					case UNIT_TWEET:
						if(RUnits[i]->buttons[T_BTN_FAV].GetState() == STATE_CLICKED){
							ShowAction(sending_msg);
							if(RUnits[i]->DoFavorite(profile, &(tweets[i-riverusers]))){
								Info.Display(successful_msg);
							}
							RUnits[i]->buttons[T_BTN_FAV].ResetState();
							CancelAction();
						}else if(RUnits[i]->buttons[T_BTN_RT].GetState() == STATE_CLICKED){
							ShowAction(sending_msg);
							if(RUnits[i]->DoRetweet(profile, &(tweets[i-riverusers]))){
								Info.Display(successful_msg);
							}
							RUnits[i]->buttons[T_BTN_RT].ResetState();
							CancelAction();
						}else if(RUnits[i]->buttons[T_BTN_REPLY].GetState() == STATE_CLICKED){
							sprintf(Tweet,"%s ", tweets[i-riverusers].user.screenname);
							for(int r = 0; r < tweets[i-riverusers].mentions_count; r++){
								if(strcmp(tweets[i-riverusers].mentions[r].screenname, login->screenname)) //Do not include the current user if it is mentioned
									sprintf(Tweet,"%s%s ",Tweet, tweets[i-riverusers].mentions[r].screenname);
							}
							if(OnScreenKeyboard(Tweet, 140, 1) && Tweet[0]){
								std::string tweet(Tweet);
								ShowAction(sending_msg);
								char itoa[64];
								sprintf(itoa, "%llu", tweets[i-riverusers].id);
								std::string replyTo(itoa);
								if(profile->twitterObj->statusUpdate( tweet, &replyTo )){
									Info.Display(published_msg);
									strcpy(Tweet,"");
								}else{
									ErrorPrompt(unablecon_msg);
								}
								CancelAction();
							}else{
									Info.Display(notpublished_msg);
							}
							RUnits[i]->buttons[T_BTN_REPLY].ResetState();
						}else if(RUnits[i]->buttons[T_BTN_TWEET].GetState() == STATE_CLICKED){
							ShowAction(sending_msg);
							if(tweets[i-riverusers].user.flags & SEARCHTWEET){ //We don't know anything!
								ShowAction(sending_msg);
								char itoa[64];
								sprintf(itoa, "%llu", tweets[i-riverusers].id);
								std::string str_id = itoa;
								profile->twitterObj->statusShowById(str_id);
								profile->twitterObj->getLastWebResponse(http_res);
								struct t_tweet * searchtweet = NULL;
								parseTweets(http_res, &searchtweet);
								memcpy(&tweets[i-riverusers], &searchtweet[0], sizeof(struct t_tweet));
								if(searchtweet) { mem2_free(searchtweet, MEM2_OTHER); }
								CancelAction();
							}
							arl = BriefTweet(&tweets[i-riverusers], profile, history, login);
							if(arl){
								update = 1;
							}else{ //Got friendship info update other units... this is magic when stalking someone's timeline
								for(int j = 0; j < rus; j++){
									if(tweets[j].user.id == tweets[i-riverusers].user.id){
										if(j != i - riverusers){
											tweets[j].user.flags &= ~FOLLOWMASK;
											tweets[j].user.flags |= (tweets[i-riverusers].user.flags & FOLLOWMASK);
										}
									}
								}
								arl = history->GetSelected();
							}

							RUnits[i]->buttons[T_BTN_USER].ResetState();
							RUnits[i]->buttons[T_BTN_TWEET].ResetState();
						}else if(RUnits[i]->buttons[T_BTN_USER].GetState() == STATE_CLICKED || RUnits[i]->buttons[T_BTN_TIMELINE].GetState() == STATE_CLICKED){
							if(tweets[i-riverusers].user.flags & SEARCHTWEET){
								arl = history->Build(TIMELINE_USER, 0, 0, 0, tweets[i-riverusers].user.screenname);
							}else{
								arl = history->Build(TIMELINE_USER, tweets[i-riverusers].user.id, 0, 0, NULL);
							}
							RUnits[i]->buttons[T_BTN_USER].ResetState();
							RUnits[i]->buttons[T_BTN_TIMELINE].ResetState();
							update = 1;
						}
					break;
					case UNIT_USER:
						if(RUnits[i]->buttons[U_BTN_FOLLOW].GetState() == STATE_CLICKED){
							if(!(tweets[i].user.flags & FOLLOWMASK)){ //Only check if we don't know if we are following it...
								std::string id_a, id_b;
								char itoa[64];
								sprintf(itoa, "%llu", login->userid); id_a = itoa;
								sprintf(itoa, "%llu", tweets[i].user.id); id_b = itoa;

								ShowAction("Getting current relationship");
								if(profile->twitterObj->friendshipExists(id_a, id_b)){
									RUnits[i]->SetFollow(1, &(tweets[i].user)); 
									if(WindowPrompt("Following!", "You are following this user. Would you like to unfollow him/her?", "Unfollow", no_msg)){
										ShowAction(sending_msg);
										RUnits[i]->DoFollow(profile, &(tweets[i].user));
										CancelAction();
									}
								}else{
									RUnits[i]->SetFollow(0, &(tweets[i].user));
									if(WindowPrompt("Not following!", "You are not following this user. Would you like to follow him/her?", "Follow", no_msg)){
										ShowAction(sending_msg);
										RUnits[i]->DoFollow(profile, &(tweets[i].user));
										CancelAction();
									}
								}
								CancelAction();
							}else{
								ShowAction(sending_msg);
								RUnits[i]->DoFollow(profile, &(tweets[i].user));
								CancelAction();
							}

							RUnits[i]->buttons[U_BTN_FOLLOW].ResetState();
							RUnits[i]->buttons[U_BTN_FG].ResetState(); //This overlaps
						}else if(RUnits[i]->buttons[U_BTN_FG].GetState() == STATE_CLICKED){
							arl = history->Build(TIMELINE_USER, tweets[i].user.id, 0, 0, NULL);
							RUnits[i]->buttons[U_BTN_FG].ResetState();
							update = 1;
						}else if(RUnits[i]->buttons[U_BTN_FAVOURITES].GetState() == STATE_CLICKED){
							arl = history->Build(SHOW_FAVOURITES, tweets[i].user.id, 0, 0, NULL);
							RUnits[i]->buttons[U_BTN_FAVOURITES].ResetState();
							update = 1;
							break;
						}else if(RUnits[i]->buttons[U_BTN_FOLLOWING].GetState() == STATE_CLICKED){
							arl = history->Build(SHOW_FOLLOWING, tweets[i].user.id, 0, 0, NULL);
							RUnits[i]->buttons[U_BTN_FOLLOWING].ResetState();
							update = 1;
							break;
						}else if(RUnits[i]->buttons[U_BTN_FOLLOWERS].GetState() == STATE_CLICKED){
							arl = history->Build(SHOW_FOLLOWERS, tweets[i].user.id, 0, 0, NULL);
							RUnits[i]->buttons[U_BTN_FOLLOWERS].ResetState();
							update = 1;
							break;
						}
					break;
				}

				if(!RUnits[i]->IsVisible()){ break; }
			}
			usleep(THREAD_SLEEP);
		}

		if(!guiShutdown){
			river.SetEffect((EFFECT_SLIDE_LEFT >> backwards) | EFFECT_SLIDE_OUT, RIVERSPEED); //Slide to the right if we are going backwards
			while(river.GetEffect() > 0) usleep(THREAD_SLEEP);
		}

		HaltGui();
			river.RemoveAll();
		ResumeGui();
		if(rus){
			for(int i = 0; i < rus; i++){
				delete RUnits[i];
			}
			mem2_free(RUnits, MEM2_OTHER);
		}
		if(tweets){ mem2_free(tweets, MEM2_OTHER); tweets = NULL; }
		if(users){ mem2_free(users, MEM2_OTHER); users = NULL; }
	}

	CancelAction();
	history->Clear();
	delete history;
	HaltGui();
	mainWindow->Remove(&toolbar);
	mainWindow->Remove(&river);
	mainWindow->Remove(&Info);
	ResumeGui();
}

/****************************************************************************
 * Profiles_WiiTweet
 ***************************************************************************/
static int Profiles_WiiTweet()
{
	class wt_profile *profile = NULL;
	int i;

	ShutoffRumble();

	int menu = MENU_NONE;

	ResumeGui();
	ShowAction("Loading profiles");

	GuiButton exitBtn(0, 0);
	exitBtn.SetTrigger(trigHome);

	GuiText newBtnTxt("Add New", 24, (GXColor){0, 0, 0, 255});
	GuiButton newBtn(btnOutline->GetWidth(), btnOutline->GetHeight());
	newBtn.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
	newBtn.SetPosition(0, 0);
	newBtn.SetLabel(&newBtnTxt);
	GuiImage newBtnImg(btnOutline);
	GuiImage newBtnImgOver(btnOutlineOver);
	newBtn.SetImage(&newBtnImg);
	newBtn.SetImageOver(&newBtnImgOver);
	newBtn.SetTrigger(trigA);
	newBtn.SetEffectGrow();

	GuiRiver profileRiver;

	GuiRiverUnit ** RUnits = NULL;
	struct ProfileData * p_data = NULL;

	int profiles_num = OpenProfilesFolder();
	if(profiles_num > 0){
		p_data = (struct ProfileData *)mem2_calloc(browser.numEntries, sizeof(struct ProfileData), MEM2_OTHER);
		RUnits = (GuiRiverUnit **)mem2_calloc(browser.numEntries, sizeof(void *), MEM2_OTHER);
		for(i=0; i<browser.numEntries; i++){
			profile = new wt_profile(browserList[i].filename, DONT_SAVE);
			strcpy(p_data[i].name, profile->userName);
			p_data[i].NIP = profile->NIP;
			p_data[i].oauth = profile->oauth;
			p_data[i].img = profile->img;
			delete profile; profile = 0;
			RUnits[i] = new GuiRiverUnit(UNIT_PROFILE, &p_data[i]);
			profileRiver.Append(RUnits[i]);
		}

		HaltGui();
		profileRiver.Append(&newBtn);
		mainWindow->Append(&profileRiver);
		mainWindow->Append(&exitBtn);
		ResumeGui();
		CancelAction();

		while(menu == MENU_NONE  && !guiShutdown){
			for(i=0; i<browser.numEntries; i++){
				if(RUnits[i]->IsVisible()){
					if(RUnits[i]->buttons[P_BTN_FG].GetState() == STATE_CLICKED){
						char filename[256];
						sprintf(filename, "%s.wtp", p_data[i].name);
						profile = new wt_profile(filename, 1);

						if(profile->oauth == 1){
							std::string tsec( "" );
							profile->getTokenKey(tsec, 0);
							profile->twitterObj->getOAuth().setOAuthTokenKey(tsec);
							profile->getTokenSecret(tsec, 0);
							profile->twitterObj->getOAuth().setOAuthTokenSecret(tsec);
							profile->getScreenName(tsec);
							profile->twitterObj->getOAuth().setOAuthScreenName(tsec);

							std::string strPIN;
							char PIN[21]="";
							WindowPrompt("Enter your PIN", lostcreds_msg, ok_msg, 0);
							OnScreenKeypad(PIN,20);
							strPIN = PIN;
							profile->twitterObj->getOAuth().setOAuthPin(strPIN);
							if(profile->twitterObj->oAuthAccessToken()){
								profile->oauth = 2;
								setNIP(profile);
								menu = MENU_BROWSE_PROFILES;
							}else{
								profile->save = 0;
								ErrorPrompt("Something was wrong with your PIN (it could have expired)");
							}
						}else if(profile->oauth == 2){ //Profile ready to use
							char NIPstr[21]="";
							char *NIP = NULL;
							int cancel = 0;
							if(profile->NIP){
								WindowPrompt("Enter your WiiTweet password", lostcreds_msg, ok_msg, 0);
								cancel = !nonEmptyOSK(NIPstr, 20);
								NIP = &NIPstr[0];
							}
							if(!cancel){
								ShowAction("Verifying credentials...");
								std::string tmp( "" );
								profile->getTokenKey(tmp, NIP);
								profile->twitterObj->getOAuth().setOAuthTokenKey(tmp);
								profile->getTokenSecret(tmp, NIP);
								profile->twitterObj->getOAuth().setOAuthTokenSecret(tmp);
								if(profile->twitterObj->accountVerifyCredGet()){
									struct t_login login;
									std::string VerifyCredRes;
									profile->twitterObj->getLastWebResponse(VerifyCredRes);
									getUserInfo(VerifyCredRes, &login);

									if(!(profile->img)){
										u8 *imgdata = (u8 *)mem2_malloc(1024*300, MEM2_OTHER);
										if(imgdata){
											int size = http_request(login.imageurl, NULL, imgdata, 1024*300, 0, 0);

											if(size){
												char filename[256];
												sprintf(filename, "%s/profiles/%s_img", appPath, p_data[i].name);
												if(SaveFile((char *)imgdata, filename, size, 0)){
													profile->img = 1;
													profile->save = 1;
												}
											}
											mem2_free(imgdata, MEM2_OTHER);
										}
									}
									VerifyCredRes.clear();
									CancelAction();
									HaltGui();
									mainWindow->Remove(&profileRiver);
									BrowseTwitter(profile, &login);
									menu = MENU_BROWSE_PROFILES;
								}else{
									CancelAction();
									ErrorPrompt("Wrong password or connection problems");
									delete profile; profile = 0;
								}
							}
						}

						RUnits[i]->buttons[P_BTN_FG].ResetState();
					}else if(RUnits[i]->buttons[P_BTN_DEL].GetState() == STATE_CLICKED){
						if(WindowPrompt("Confirm deletion", "This is permanent. Continue?", "Delete", cancel_msg)){
							char removepath[256];
							sprintf(removepath, "%s/profiles/%s.wtp", appPath, p_data[i].name);
							if(remove(removepath)){
								ErrorPrompt("Could not delete file");
							}else{
								menu = MENU_BROWSE_PROFILES;
							}
						}
						RUnits[i]->buttons[P_BTN_DEL].ResetState();
					}
				}
			}
			if(exitBtn.GetState() == STATE_CLICKED){
				ExitRequested = true;
				exitBtn.ResetState();
			}

			if(newBtn.GetState() == STATE_CLICKED){
				createProfile( &profile );
				menu = MENU_BROWSE_PROFILES;
			}
		}

	}else{ // 0 profiles were found, is a device inserted?
		createProfile(&profile, profiles_num == 0);
		if(profile){
			if(profiles_num == -1 && profile->twitterObj->accountVerifyCredGet()){ //Go to Twitter in-spot as this profile won't be loaded...
				struct t_login login;
				std::string VerifyCredRes;
				profile->twitterObj->getLastWebResponse(VerifyCredRes);
				getUserInfo(VerifyCredRes, &login);
				VerifyCredRes.clear();
				HaltGui();
				mainWindow->Remove(&profileRiver);
				BrowseTwitter(profile, &login);
				profile->save = 0; //Don't save whatever it happened
				delete profile; profile = NULL;
				if(!WindowPrompt("Logged out", "Do you want to sign in again?", "Log in", exit_msg)){
					guiShutdown = true;
				}
			}
		}else{
			if(!WindowPrompt("Authorization interrumpted", "Do you want to restart the authorization process?", yes_msg, exit_msg)){
				guiShutdown = true;
			}
		}
		menu = MENU_BROWSE_PROFILES;
	}

	HaltGui();

	for(i=0; i < browser.numEntries; i++){
		delete	RUnits[i];
	}

	if(p_data) mem2_free(p_data, MEM2_OTHER);
	if(RUnits) mem2_free(RUnits, MEM2_OTHER);
	mainWindow->Remove(&profileRiver);
	mainWindow->Remove(&exitBtn);
	delete profile;

	return menu;
}


/*************************************************************************
 * First_Screen
 * Checks and network initialization happens here...
 *************************************************************************/
static int First_Screen()
{
	ResumeGui();

	//Network stuff: Initialization, update checking and installing. The time offset to twitter servers should have been calculated now
	InitializeNetwork(0);

	int device;
	if(UpdateCheck() && FindDevice(appPath, &device) && ChangeInterface(device, SILENT)){ //Check for updates even if no device is found (because time offset is calculated here) but don't prompt to update
		if(WindowPrompt("Update available", "You may be missing some features and/or fixes!", "Update now", "Update later")){
			if(DownloadUpdate()){
				ExitRequested = true;
			}
		}
	}

	//Custom keyboard layouts loading
	LoadKeyboardMaps();

	HaltGui();

return MENU_BROWSE_PROFILES;
}

void DefaultSettings(){
	Settings.ExitAction = 0;
	Settings.Rumble = 1; //Ignored as of now...
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void
MainMenu (int menu)
{
	int currentMenu = menu;
	guiShutdown = false;
	SetupGui();
	DefaultSettings();
	HaltGui();

	mainWindow = new GuiWindow(screenwidth, screenheight);
	mainWindow->SetFocus(1);

	GuiImageData bgimg_data(bground_png); 
	GuiImage bgroundimg(&bgimg_data);
	bgroundimg.SetEffect(EFFECT_FADE, 15); //Fade-in slowly so the splashscreen goes on screen first
	mainWindow->Append(&bgroundimg);

	while(!guiShutdown && !ExitRequested)
	{
		switch (currentMenu)
		{
			case MENU_FIRSTSCREEN:
				currentMenu = First_Screen();
			break;
			case MENU_BROWSE_PROFILES:
				currentMenu = Profiles_WiiTweet();
			break;
		}
		usleep(THREAD_SLEEP);
	}

	StopGuiThreads();
	ShutoffRumble();

	delete mainWindow;
	mainWindow = NULL;

	ExitApp();
}
