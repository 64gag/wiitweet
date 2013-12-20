/****************************************************************************
 * WiiTweet
 * Tantric 2009-2012
 *
 * fileop.cpp
 * File operations
 *
 * Stripped down by Pedro Aguiar from tantric's snes9xGX's fileop.cpp
 ***************************************************************************/

#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogcsys.h>
#include <dirent.h>
#include <sys/stat.h>
#include <malloc.h>
#include <sdcard/wiisd_io.h>
#include <sdcard/gcsd.h>
#include <ogc/usbstorage.h>
#include <ogc/lwp_watchdog.h>

#include "profilebrowser.h"
#include "wiitweet.h"
#include "fileop.h"
#include "networkop.h"
#include "menu.h"
#include "libwiigui/gui.h"

#define THREAD_SLEEP 100

unsigned char *savebuffer = NULL;
static mutex_t bufferLock = LWP_MUTEX_NULL;
FILE * file; // file pointer - the only one we should ever use!
bool unmountRequired[3] = { false, false, false };
bool isMounted[3] = { false, false, false };

const DISC_INTERFACE* sd = &__io_wiisd;
const DISC_INTERFACE* usb = &__io_usbstorage;

/****************************************************************************
 * UnmountAllFAT
 * Unmounts all FAT devices
 ***************************************************************************/
void UnmountAllFAT()
{
	fatUnmount("sd:");
	fatUnmount("usb:");
}

/****************************************************************************
 * MountFAT
 * Checks if the device needs to be (re)mounted
 * If so, unmounts the device
 * Attempts to mount the device specified
 * Sets libfat to use the device by default
 ***************************************************************************/

static bool MountFAT(int device, int silent)
{
	bool mounted = false;
	int retry = 1;
	char name[10], name2[10];
	const DISC_INTERFACE* disc = NULL;

	switch(device)
	{
		case DEVICE_SD:
			sprintf(name, "sd");
			sprintf(name2, "sd:");
			disc = sd;
			break;
		case DEVICE_USB:
			sprintf(name, "usb");
			sprintf(name2, "usb:");
			disc = usb;
			break;
		default:
			return false; // unknown device
	}

	if(unmountRequired[device])
	{
		unmountRequired[device] = false;
		fatUnmount(name2);
		disc->shutdown();
		isMounted[device] = false;
	}


	while(retry)
	{
		if(disc->startup() && fatMountSimple(name, disc))
			mounted = true;

		if(mounted || silent)
			break;

		if(device == DEVICE_SD)
			retry = ErrorPromptRetry("SD card not found!");
		else
			retry = ErrorPromptRetry("USB drive not found!");
	}

	isMounted[device] = mounted;
	return mounted;
}

void MountAllFAT()
{
	MountFAT(DEVICE_SD, SILENT);
	MountFAT(DEVICE_USB, SILENT);
}


bool FindDevice(char * filepath, int * device)
{
	if(!filepath || filepath[0] == 0)
		return false;

	if(strncmp(filepath, "sd:", 3) == 0)
	{
		*device = DEVICE_SD;
		return true;
	}
	else if(strncmp(filepath, "usb:", 4) == 0)
	{
		*device = DEVICE_USB;
		return true;
	}
	return false;
}

char * StripDevice(char * path)
{
	if(path == NULL)
		return NULL;

	char * newpath = strchr(path,'/');

	if(newpath != NULL)
		newpath++;

	return newpath;
}

/****************************************************************************
 * ChangeInterface
 * Attempts to mount/configure the device specified
 ***************************************************************************/
bool ChangeInterface(int device, bool silent)
{
	if(isMounted[device])
		return true;

	bool mounted = false;

	mounted = MountFAT(device, silent);

	return mounted;
}

bool ChangeInterface(char * filepath, bool silent)
{
	int device = -1;

	if(!FindDevice(filepath, &device))
		return false;

	return ChangeInterface(device, silent);
}

void CreateAppPath(char * origpath)
{
	if(!origpath || origpath[0] == 0 || origpath[0] == '/')
		return;

	char * path = strdup(origpath); // make a copy so we don't mess up original

	if(!path)
		return;

	char * loc = strrchr(path,'/');
	if (loc != NULL)
		*loc = 0; // strip file name

	int pos = 0;

	// replace fat:/ with sd:/
	if(strncmp(path, "fat:/", 5) == 0)
	{
		pos++;
		path[1] = 's';
		path[2] = 'd';
	}
	if(ChangeInterface(&path[pos], SILENT))
		snprintf(appPath, MAXPATHLEN-1, "%s", &path[pos]);

	free(path);
}

void FindAppPath()
{
	char filepath[MAXPATHLEN];
	DIR *dir;

	if(sd->startup() && sd->isInserted())
	{
		if(MountFAT(DEVICE_SD, SILENT)){
			sprintf(filepath, "sd:/apps/%s", APPFOLDER);
			dir = opendir(filepath);
			if(dir)
			{
				closedir(dir);
				strcpy(appPath, filepath);
				return;
			}
		}
	}

	u64 start = gettime();
	usleep(20000);

	while(diff_sec(start, gettime()) < 5) // 5 sec
	{
		if(usb->startup() && usb->isInserted())
		{
			break;
		}
		usleep(250000); // 1/4 sec
	}

	if(usb->isInserted())
	{
		if(MountFAT(DEVICE_USB, SILENT)){
			sprintf(filepath, "usb:/apps/%s", APPFOLDER);
			dir = opendir(filepath);
			if(dir)
			{
				closedir(dir);
				strcpy(appPath, filepath);
				return;
			}
		}
	}
}

int GetFileSize(const char * path)
{
	struct stat filestat;
	if(stat(path, &filestat) < 0){
		return 0;
	}else{
		return filestat.st_size;
	}
}

/****************************************************************************
 * AllocSaveBuffer ()
 * Clear and allocate the savebuffer
 ***************************************************************************/
void
AllocSaveBuffer ()
{
	if(bufferLock == LWP_MUTEX_NULL)
		LWP_MutexInit(&bufferLock, false);

	if(bufferLock != LWP_MUTEX_NULL)
		LWP_MutexLock(bufferLock);
	memset (savebuffer, 0, SAVEBUFFERSIZE);
}

bool file_exists(char * filename)
{
	char receiver;
	if (LoadFile(&receiver,filename, 1, SILENT))
	{
		return true;
	}
	return false;
}

/****************************************************************************
 * FreeSaveBuffer ()
 * Free the savebuffer memory
 ***************************************************************************/
void
FreeSaveBuffer ()
{
	if(bufferLock != LWP_MUTEX_NULL)
		LWP_MutexUnlock(bufferLock);
}

/****************************************************************************
 * LoadFile
 ***************************************************************************/
size_t
LoadFile (char * rbuffer, char *filepath, size_t length, bool silent)
{
	char zipbuffer[2048];
	size_t size = 0, offset = 0, readsize = 0;
	int retry = 1;
	int device;

	if(!FindDevice(filepath, &device))
		return 0;

	// open the file
	while(retry)
	{
		if(!ChangeInterface(device, silent))
			break;

		file = fopen (filepath, "rb");

		if(!file)
		{
			if(silent)
				break;

			retry = ErrorPromptRetry("Error opening file!");
			continue;
		}

		if(length > 0 && length <= 2048) // do a partial read (eg: to check file header)
		{
			size = fread (rbuffer, 1, length, file);
		}
		else // load whole file
		{
			readsize = fread (zipbuffer, 1, 32, file);

			if(!readsize)
			{
				unmountRequired[device] = true;
				retry = ErrorPromptRetry("Error reading file!");
				fclose (file);
				continue;
			}

			fseeko(file,0,SEEK_END);
			size = ftello(file);
			fseeko(file,0,SEEK_SET);

			while(!feof(file))
			{
				ShowProgress ("Loading...", offset, size);
				readsize = fread (rbuffer + offset, 1, 4096, file); // read in next chunk

				if(readsize <= 0)
					break; // reading finished (or failed)

				offset += readsize;
			}
			size = offset;
			CancelAction();

		}
		retry = 0;
		fclose (file);
	}

	CancelAction();
	return size;
}

size_t LoadFile(char * filepath, bool silent)
{
	return LoadFile((char *)savebuffer, filepath, 0, silent);
}

/****************************************************************************
 * SaveFile
 * Write buffer to file
 ***************************************************************************/
size_t
SaveFile (char * buffer, char *filepath, size_t datasize, bool silent)
{
	size_t written = 0;
	size_t writesize, nextwrite;
	int retry = 1;
	int device;

	if(!FindDevice(filepath, &device))
		return 0;

	if(datasize == 0)
		return 0;

	if(!silent)
		ShowAction("Saving...");

	while(!written && retry == 1)
	{
		if(!ChangeInterface(device, silent))
			break;

		file = fopen (filepath, "wb");

		if(!file)
		{
			if(silent)
				break;

			retry = ErrorPromptRetry("Error creating file!");
			continue;
		}

		while(written < datasize)
		{
			if(datasize - written > 4096) nextwrite=4096;
			else nextwrite = datasize-written;
			writesize = fwrite (buffer+written, 1, nextwrite, file);
			if(writesize != nextwrite) break; // write failure
			written += writesize;
		}
		fclose (file);

		if(written != datasize) written = 0;

		if(!written)
		{
			unmountRequired[device] = true;
			if(silent) break;
			retry = ErrorPromptRetry("Error saving file!");
		}
	}

	if(!silent)
		CancelAction();
	return written;
}

size_t SaveFile(char * filepath, size_t datasize, bool silent)
{
	return SaveFile((char *)savebuffer, filepath, datasize, silent);
}
