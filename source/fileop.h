/****************************************************************************
 * WiiTweet
 * Tantric 2009-2012
 *
 * fileop.cpp
 * File operations
 *
 * Stripped down by Pedro Aguiar from tantric's snes9xGX's fileop.cpp
 ***************************************************************************/
 
#ifndef _FILEOP_H_
#define _FILEOP_H_

#include <gccore.h>
#include <stdio.h>
#include <string.h>
#include <ogcsys.h>
#include <fat.h>
#include <unistd.h>

#define SAVEBUFFERSIZE (1024 * 512)

void MountAllFAT();
void UnmountAllFAT();
bool FindDevice(char * filepath, int * device);
char * StripDevice(char * path);
bool ChangeInterface(int device, bool silent);
bool ChangeInterface(char * filepath, bool silent);
void CreateAppPath(char * origpath);
void FindAppPath();
int GetFileSize(const char * path);
bool file_exists(char * filename);
void AllocSaveBuffer();
void FreeSaveBuffer();
size_t LoadFile(char * rbuffer, char *filepath, size_t length, bool silent);
size_t LoadFile(char * filepath, bool silent);
size_t SaveFile(char * buffer, char *filepath, size_t datasize, bool silent);
size_t SaveFile(char * filepath, size_t datasize, bool silent);

extern unsigned char *savebuffer;
extern FILE * file;
extern bool unmountRequired[];
extern bool isMounted[];
extern int selectLoadedFile;

#endif
