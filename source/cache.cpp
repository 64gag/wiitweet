/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar
 *
 * cache.cpp
 *
 * Image caching/reusing system
 ***************************************************************************/
#include <sys/stat.h>
#include "libwiigui/gui.h"
#include "fileop.h"
#include "utils/http.h"
#include "wiitweet.h"
#include "cache.h"

void escapeString( const char *src, char *dest){

 strcpy(dest, src);
 
	for(u32 i = 0; i < strlen(src); i++){
		if(src[i] == ' ' || src[i] == '\\' || src[i] == '/' || src[i] == ':'){
			dest[i] = '_';
		}
	}
}

void cache_session_init(struct cached_imgs ** session){
struct cached_imgs * temp = NULL;
	if(*session){ //Previous session was not destroyed! (the user clicked somewhere before downloading all the images)
		memset(*session, 0, SESSIONMAX*sizeof(struct cached_imgs));
		return;
	}

	temp = (struct cached_imgs *)mem2_calloc(SESSIONMAX, sizeof(struct cached_imgs), MEM2_OTHER);
	if(temp){
		*session = temp;
	}else{
		*session = NULL;
	}
}

void cache_session_destroy(struct cached_imgs ** session){
	if(*session) mem2_free(*session, MEM2_OTHER);
	*session = NULL;
}

void cache_init(){
	char cachedir[512];
	sprintf(cachedir,"%s/cache", appPath);
	if(chdir(cachedir) == -1)
		mkdir(cachedir, 0777);
}

//#define FATCACHING
#define CACHESESSION
GuiImageData * cacheImage(const char *name, struct cached_imgs * session, int force, u64 id){

#ifdef FATCACHING
	char escaped[256];
	char fullpath[512];
	int fsize = 0;

 	struct stat filestat;
 	int cached = 0;
#endif
	GuiImageData * temp = NULL;
	u8 * imgdata = NULL;

#ifdef CACHESESSION
	int i = 0;

	if(session){
		for(i = 0; i < SESSIONMAX; i++){ //If the image was loaded in this session we have its data
			if(!session[i].id){
				break;
			}
			if(session[i].id == id){
				((GuiImageData *)session[i].address)->shared++;
				return ((GuiImageData *)session[i].address);
			}
		}
	}
#endif
#ifdef FATCACHING
	escapeString(name, escaped);
	sprintf(fullpath, "%s/cache/%s",appPath, escaped);

	if(!force){
		cached = !stat(fullpath, &filestat);
		fsize = filestat.st_size;
	}

	if(cached){
		imgdata = (u8 *)gui_malloc(fsize);
	}else{
		imgdata = (u8 *)gui_malloc(MAXIMGSIZE);
	}
	
	if(imgdata){
		if(cached){
			LoadFile((char *)imgdata, fullpath, fsize, SILENT);
			temp = new GuiImageData(imgdata, fsize, GX_TF_RGBA8);
		}else{
			int size = http_request(name, NULL, imgdata, MAXIMGSIZE, SILENT, 0);
			if(size){
				SaveFile((char *)imgdata, fullpath, size, 0);
				temp = new GuiImageData(imgdata, size, GX_TF_RGBA8);
			}
		}
		gui_free(imgdata);		
	}
#else
	imgdata = (u8 *)gui_malloc(MAXIMGSIZE);
	int size = http_request(name, NULL, imgdata, MAXIMGSIZE, SILENT, 0);
	if(size){
		temp = new GuiImageData(imgdata, size, GX_TF_RGBA8);
	}
	gui_free(imgdata);
#endif

#ifdef CACHESESSION	
	if(session && temp && i < SESSIONMAX){
		session[i].id = id;
		session[i].address = temp;
	}
#endif
return temp;
}
