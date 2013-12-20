/****************************************************************************
 * WiiTweet
 *
 * Tantric 2008-2010
 *
 * networkop.cpp
 *
 * Network support routines
 *
 * Slightly modified by Pedro Aguiar
 ****************************************************************************/

#include <network.h>
#include <malloc.h>
#include <ogc/lwp_watchdog.h>
#include <smb.h>
#include <mxml.h>

#include "menu.h"
#include "fileop.h"
#include "utils/http.h"
#include "utils/unzip/unzip.h"
#include "utils/unzip/miniunz.h"

#include "wiitweet.h"
#include "twitcurl/twitcurl.h"
#include "twitcurl/SHA1.h"

static bool networkInit = false;
char wiiIP[16] = { 0 };

static int netHalt = 0;
static char updateURL[128]; // URL of app update
char updateHash[41]; //SHA1 of the update zip


/****************************************************************************
 * UpdateCheck
 * Checks for an update for the application
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);
bool UpdateCheck()
{
	bool ret = 0;

	// we only check for an update if we have internet
	if(!networkInit)
		return 0;

	ShowAction("Checking for updates...");
	class twitCurl twitterObj;
	std::string tmp("392315277"); // @WiiTwiity userid
	if( twitterObj.userGet(tmp, 1) ){
		tmp.clear();
		twitterObj.getLastWebResponse(tmp);
	}else{
		CancelAction();
		return 0;
	}
	mxml_node_t *xml;
	mxml_node_t *item;
	xml = mxmlLoadString(NULL, tmp.c_str(), MXML_OPAQUE_CALLBACK);

	if(!xml){
		CancelAction();
		return 0;
	}

	item = mxmlFindElement(xml, xml, "text", NULL, NULL, MXML_DESCEND);
	if(item) // Tweet found!
	{
		const char * tweet = item->child->value.opaque;
		int verMajor, verMinor, verPoint;
		if(sscanf(tweet, "WiiTweet %d.%d.%d released! SHA-1: %s", &verMajor, &verMinor, &verPoint, updateHash) == 4)
		{
			int curMajor = APPVERSION[0] - '0';
			int curMinor = APPVERSION[2] - '0';
			int curPoint = APPVERSION[4] - '0';

			// check that the versioning is valid and is a newer version
			if((verMajor >= 0 && verMajor <= 9 &&
				verMinor >= 0 && verMinor <= 9 &&
				verPoint >= 0 && verPoint <= 9) &&
				(verMajor > curMajor ||
				(verMajor == curMajor && verMinor > curMinor) ||
				(verMajor == curMajor && verMinor == curMinor && verPoint > curPoint)))
			{
				snprintf(updateURL, 128, "http://wiitweet.googlecode.com/files/wiitweet%%20%d.%d.%d.zip", verMajor, verMinor, verPoint);
				ret = 1;
			}
		}
	}

	mxmlDelete(xml);
	CancelAction();
	return ret;
}

static bool unzipArchive(char * zipfilepath, char * unzipfolderpath)
{
	unzFile uf = unzOpen(zipfilepath);
	if (uf==NULL)
		return false;

	if(chdir(unzipfolderpath)) // can't access dir
	{
		makedir(unzipfolderpath); // attempt to make dir
		if(chdir(unzipfolderpath)) // still can't access dir
			return false;
	}

	extractZip(uf,0,1,0);

	unzCloseCurrentFile(uf);
	return true;
}

bool DownloadUpdate()
{
	bool result = false;

	if(updateURL[0] == 0 || appPath[0] == 0 || !ChangeInterface(appPath, NOTSILENT))
	{
		ErrorPrompt("Did you unplug your SD/USB?");
		return false;
	}

	int device;
	FindDevice(appPath, &device);

	char updateFile[50];
	sprintf(updateFile, "%s%sUpdate.zip", pathPrefix[device], APPNAME);

	FILE * hfile = fopen (updateFile, "wb");

	if (hfile)
	{
		if(http_request(updateURL, hfile, NULL, (1024*1024*10), NOTSILENT, 0) > 0)
		{
			ShowAction("Extracting update...");
			fclose (hfile);
			CSHA1 sha1;
			if(sha1.HashFile(updateFile)){
				char fileHash[41]="";
				sha1.Final();
				sha1.ReportHash(fileHash, 0);
				if(!strcasecmp(fileHash, updateHash)){
					char destPath[256];
					sprintf(destPath, "%sapps", pathPrefix[device]);
					result = unzipArchive(updateFile, destPath);
				}else{
					ErrorPrompt("Corrupted file!");
				}
			}else{
				ErrorPrompt("Could not verify file!");
			}
		}
		else
		{
			ErrorPrompt("Unable to download!");
			fclose (hfile);
		}
		remove(updateFile); // delete update file
	}else{
		ErrorPrompt("Unable to write to SD/USB!");
	}

	if(result){
		InfoPrompt("Update successful!");
	}

	CancelAction();
	return result;
}

static lwp_t networkthread = LWP_THREAD_NULL;
static u8 netstack[32768] ATTRIBUTE_ALIGN (32);

static void * netcb (void *arg)
{
	s32 res=-1;
	int retry;
	int wait;
	static bool prevInit = false;

	while(netHalt != 2)
	{
		retry = 5;
		
		while (retry>0 && (netHalt != 2))
		{			
			if(prevInit) 
			{
				int i;
				net_deinit();
				for(i=0; i < 400 && (netHalt != 2); i++) // 10 seconds to try to reset
				{
					res = net_get_status();
					if(res != -EBUSY) // trying to init net so we can't kill the net
					{
						usleep(2000);
						net_wc24cleanup(); //kill the net 
						prevInit=false; // net_wc24cleanup is called only once
						usleep(20000);
						break;					
					}
					usleep(20000);
				}
			}

			usleep(2000);
			res = net_init_async(NULL, NULL);

			if(res != 0)
			{
				sleep(1);
				retry--;
				continue;
			}

			res = net_get_status();
			wait = 400; // only wait 8 sec
			while (res == -EBUSY && wait > 0  && (netHalt != 2))
			{
				usleep(20000);
				res = net_get_status();
				wait--;
			}

			if(res==0) break;
			retry--;
			usleep(2000);
		}
		if (res == 0)
		{
			struct in_addr hostip;
			hostip.s_addr = net_gethostip();
			if (hostip.s_addr)
			{
				strcpy(wiiIP, inet_ntoa(hostip));
				networkInit = true;	
				prevInit = true;
			}
		}
		if(netHalt != 2) LWP_SuspendThread(networkthread);
	}
	return NULL;
}

/****************************************************************************
 * StartNetworkThread
 *
 * Signals the network thread to resume, or creates a new thread
 ***************************************************************************/
void StartNetworkThread()
{
	netHalt = 0;

	if(networkthread == LWP_THREAD_NULL)
		LWP_CreateThread(&networkthread, netcb, NULL, netstack, 8192, 40);
	else
		LWP_ResumeThread(networkthread);
}

/****************************************************************************
 * StopNetworkThread
 *
 * Signals the network thread to stop
 ***************************************************************************/
void StopNetworkThread()
{
	if(networkthread == LWP_THREAD_NULL || !LWP_ThreadIsSuspended(networkthread))
		return;

	netHalt = 2;
	LWP_ResumeThread(networkthread);

	// wait for thread to finish
	LWP_JoinThread(networkthread, NULL);
	networkthread = LWP_THREAD_NULL;
}

/****************************************************************************
 * InitializeNetwork
 * Initializes the Wii/GameCube network interface
 ***************************************************************************/
bool InitializeNetwork(bool silent)
{

	StopNetworkThread();

	if(networkInit && net_gethostip() > 0)
		return true;

	networkInit = false;

	int retry = 1;

	while(retry)
	{
		ShowAction("Initializing network...");

		u64 start = gettime();
		StartNetworkThread();

		while (!LWP_ThreadIsSuspended(networkthread))
		{
			usleep(50 * 1000);

			if(diff_sec(start, gettime()) > 10) // wait for 10 seconds max for net init
				break;
		}

		CancelAction();

		if(networkInit || silent)
			break;

		retry = ErrorPromptRetry("Unable to initialize network!");
		
		if(networkInit && net_gethostip() > 0)

			return true;
	}
	return networkInit;
}

