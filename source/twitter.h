#ifndef TWITTER_H
#define TWITTER_H

#include <string>
#include <mxml.h>

#define RETWEETED 1
#define FAVORITED 2
#define FOLLOWMASK 12
#define FOLLOWING 4
#define NFOLLOWING 8
#define SEARCHTWEET 16

enum {
	TIMELINE_HOME = 0,
	TIMELINE_USER,
	USER_MENTIONS,
	SHOW_FAVOURITES,
	SHOW_FOLLOWING,
	SHOW_FOLLOWERS,
	SEARCH_TWEET,
	SEARCH_USER,
	CREDITS,
	OTHER
};

struct t_login{
	u64 userid;
	char screenname[20];
	char imageurl[256];
};

struct t_hashtag{
	char text[64];
};

struct t_mention{
	char screenname[20];
	u64 id;
};

struct t_user{
	u64 id;
	char name[81]; //These can be unicode characters
	char screenname[20]; 
	char location[160];
	char description[641];
	char imageurl[256];
	char url[160];
	u64 followers;
	u64 following;
	u64 favourites;
	u64 tweets;
	u32 flags;
};

struct t_tweet{
	time_t created;
	u64 id;
	char text[768];
	char retweetby[96];
	u64 retweet_id;
	u32 flags;
	struct t_user user;
	char hashtags_count;
	char mentions_count;
	struct t_hashtag * hashtags;
	struct t_mention * mentions;
};

char *parseIDs(const char * array_ids, unsigned int page);
void parseEntities(mxml_node_t *entities, struct t_tweet *dest);
void parseUser(mxml_node_t *user, struct t_user *dest, struct t_tweet *desttweet = NULL);
void parseTweet(mxml_node_t *tweet, struct t_tweet *dest, int parseuser = 1);
void parseSearchEntry(mxml_node_t * entry, struct t_tweet *dest);
void getUserInfo(std::string& xmlstr, struct t_login * dest);
int parseUsers(std::string &xmlstr, struct t_user ** dest);
int parseTweets(std::string &xmlstr, struct t_tweet ** dest, int usertop = 0);
int parseSearch(std::string &xmlstr, struct t_tweet ** dest);

#endif
