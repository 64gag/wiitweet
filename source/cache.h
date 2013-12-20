#ifndef CACHE_H
#define CACHE_H

#define SESSIONMAX 25
#define MAXIMGSIZE 300*1024

struct cached_imgs{
	u64 id;
	void * address;
};

void cache_init();
void cache_session_init(struct cached_imgs ** session);
void cache_session_destroy(struct cached_imgs ** session);
GuiImageData * cacheImage(const char *name, struct cached_imgs * session, int force, u64 id);
#endif
