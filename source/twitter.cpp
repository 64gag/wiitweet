/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar
 *
 * twitter.cpp
 *
 * Parsing and processing twitter data
 ***************************************************************************/
#include "utils/mem2_manager.h"
#include "twitter.h"

#define MAXUNITS 20

extern time_t timeoffset;
const char tweet_format[] = "%a %b %d %H:%M:%S +0000 %Y";
const char search_format[] = "%Y-%m-%dT%H:%M:%SZ";

//strings
const char id_str[] = "id";
const char text_str[] = "text";
const char screenname_str[] = "screen_name";
const char name_str[] = "name";
const char status_str[] = "status";
const char user_str[] = "user";
const char hashtag_str[] = "hashtag";
const char ampamp_str[] = "&amp;amp;";
const char usermention_str[] = "user_mention";

void wt_strncpy(char * dest, const char * src, int max){

	int j = 1;
	char * to = dest;
	const char * from = src;
	while( *from != '\0' && j < max ){
		if(*from > 31){
			if(*from != '&'){
				*to++ = *from++;
			}else{
				if(!strncmp(from, "&quot;", 6)){
					*to++ = '"'; from += 6;
				}else if(!strncmp(from, "&amp;", 5)){
					*to++ = '&'; from += 5;
				}else if(!strncmp(from, "&apos;", 6)){
					*to++ = '\''; from += 6;
				}else if(!strncmp(from, "&lt;", 4)){
					*to++ = '<'; from += 4;
				}else if(!strncmp(from, "&gt;", 4)){
					*to++ = '>'; from += 4;
				}else{
					*to++ = *from++;
				}
			}
		}else if(*from == '\n' || *from == '\r'){
			*to++ = ' '; from++;
		}else{
			*to++ = *from++;
		}

		j++;
	}

	*to = '\0';
}

void pre_mxml_processing(std::string &xmlstr){
	size_t pos = 0;
	while(1){
		pos = xmlstr.find(ampamp_str, pos);
		if(pos == std::string::npos) break;
		xmlstr.replace(pos, 9, "&amp;");
	}
}

char *parseIDs(const char * array_ids, unsigned int page){
 unsigned int line = 0, read = 0;
 const char * src = array_ids;

	while(line < 3 + page * 20){
		while(*src != '\n' && *src != '\0')
			src++;

		if(*src++ == '\0') return NULL;
		line++;
	}

	if(src[1] == '/') return NULL; //empty list

	char * tag = NULL;
	char * ret = (char *)mem2_malloc(17*20, MEM2_OTHER);
	if(ret == NULL) return NULL;
	char * dest = ret;

	src += 4; 						//skip the <id> tag
	while((tag = strchr(src, '<')) && read < 20){
		if(strncmp(tag, "</id>", 5)) break;	//IDs are over
		line = tag - src;
		memcpy(dest, src, line);
		dest += line; *dest++ = ',';
		src = tag + 10; 				// Skip </id>\n<id>
		read++;
	}
	*--dest = '\0';

	return ret;
}

void parseEntities(mxml_node_t *entities, struct t_tweet *dest){
	mxml_node_t *group = NULL;
	mxml_node_t *unit = NULL;
	mxml_node_t *value = NULL;

	dest->hashtags = NULL;
	dest->mentions = NULL;

	group = mxmlFindElement(entities, entities, "user_mentions", NULL, NULL, MXML_DESCEND);
	if(group && group->child){
		for(unit = mxmlFindElement(group, group, usermention_str, NULL, NULL, MXML_DESCEND); unit && unit->child; unit = mxmlFindElement(unit, group, usermention_str, NULL, NULL, MXML_DESCEND)){
			dest->mentions = (struct t_mention *)mem2_realloc(dest->mentions, (++(dest->mentions_count))*sizeof(struct t_mention), MEM2_OTHER);
			value = mxmlFindElement(unit, unit, screenname_str, NULL, NULL, MXML_DESCEND);
			snprintf(dest->mentions[dest->mentions_count - 1].screenname, 20, "@%s", value->child->value.opaque);
			value = mxmlFindElement(unit, unit, id_str, NULL, NULL, MXML_DESCEND);
			sscanf(value->child->value.opaque, "%llu", &(dest->mentions[dest->mentions_count - 1].id));
		}
	}

	group = mxmlFindElement(entities, entities, "hashtags", NULL, NULL, MXML_DESCEND);
	if(group && group->child){
		for(unit = mxmlFindElement(group, group, hashtag_str, NULL, NULL, MXML_DESCEND); unit && unit->child; unit = mxmlFindElement(unit, group, hashtag_str, NULL, NULL, MXML_DESCEND)){
			dest->hashtags = (struct t_hashtag *)mem2_realloc(dest->hashtags, (++(dest->hashtags_count))*sizeof(struct t_hashtag), MEM2_OTHER);
			value = mxmlFindElement(unit, unit, text_str, NULL, NULL, MXML_DESCEND);
			snprintf(dest->hashtags[dest->hashtags_count - 1].text, 64, "#%s", value->child->value.opaque);
		}
	}

	mxmlDelete(value);
	mxmlDelete(unit);
	mxmlDelete(group);
}

void parseSearchEntry(mxml_node_t * entry, struct t_tweet *dest){
	mxml_node_t * value = NULL;
 
	value = mxmlFindElement(entry, entry, "id", NULL, NULL, MXML_DESCEND);
	if(value && value->child){
		char * scan = value->child->value.opaque;
		scan += strlen(value->child->value.opaque) - 5;
		while(*--scan >= '0' && *scan <= '9');
		sscanf(++scan, "%llu", &(dest->id));
	}

	value = mxmlFindElement(value, entry, "published", NULL, NULL, MXML_DESCEND);
	{
		struct tm tm;
		strptime(value->child->value.opaque, search_format, &tm);
		tm.tm_isdst = -1;
		dest->created = mktime(&tm) - timeoffset;
	}

	value = mxmlFindElement(value, entry, "title", NULL, NULL, MXML_DESCEND);
	if(value && value->child) wt_strncpy(dest->text, value->child->value.opaque, 768);

	value = mxmlFindElement(value, entry, "link", "rel", "image", MXML_DESCEND);
	if(value) wt_strncpy(dest->user.imageurl, mxmlElementGetAttr(value, "href"), 256);

	value = mxmlFindElement(value, entry, "name", NULL, NULL, MXML_DESCEND);
	if(value && value->child){
		dest->user.screenname[0] = '@';
		sscanf(value->child->value.opaque, "%s", &(dest->user.screenname[1]));
		char * scan = value->child->value.opaque;
		scan += strlen(dest->user.screenname) + 1;
		wt_strncpy(dest->user.name, scan, 81);
		dest->user.name[strlen(dest->user.name)-1] = '\0';
	}

	dest->user.flags |= SEARCHTWEET;
	mxmlDelete(value);
}

int parseSearch(std::string &xmlstr, struct t_tweet ** dest){
	mxml_node_t *xml;
	mxml_node_t *unit;

	struct t_tweet * tweets = (struct t_tweet *)mem2_calloc(MAXUNITS, sizeof(struct t_tweet), MEM2_OTHER);
	if(!tweets){
		return 0;
	}

	pre_mxml_processing(xmlstr);

	xml = mxmlLoadString(NULL, xmlstr.c_str(), MXML_OPAQUE_CALLBACK);

	if(!xml){
		mem2_free(tweets, MEM2_OTHER);
		return 0;
	}

	int t=0;
	for(unit = mxmlFindElement(xml, xml, "entry", NULL, NULL, MXML_DESCEND); unit && t < MAXUNITS; unit = mxmlFindElement(unit, xml, "entry", NULL, NULL, MXML_DESCEND)){
		parseSearchEntry(unit, &tweets[t++]);
	}

	mxmlDelete(xml);
	if(t < MAXUNITS){ //Free unused memory
		tweets = (struct t_tweet *)mem2_realloc(tweets, t*sizeof(struct t_tweet), MEM2_OTHER);
	}

	*dest = tweets;
return t;
}

void parseUser(mxml_node_t *user, struct t_user *dest, struct t_tweet * dest_tweet){
	mxml_node_t * value = NULL;
 
 	if(dest_tweet){
		value = mxmlFindElement(user, user, status_str, NULL, NULL, MXML_DESCEND);
		if(value){ parseTweet(value, dest_tweet, 0); }
	}

	value = mxmlFindElement(user, user, id_str, NULL, NULL, MXML_DESCEND);
	if(value && value->child) sscanf(value->child->value.opaque, "%llu", &(dest->id));
	value = mxmlFindElement(user, user, name_str, NULL, NULL, MXML_DESCEND);
	if(value && value->child) wt_strncpy(dest->name, value->child->value.opaque, 81);
	value = mxmlFindElement(user, user, screenname_str, NULL, NULL, MXML_DESCEND);
	if(value && value->child) snprintf(dest->screenname, 20, "@%s", value->child->value.opaque);
	value = mxmlFindElement(user, user, "location", NULL, NULL, MXML_DESCEND);
	if(value && value->child) wt_strncpy(dest->location, value->child->value.opaque, 160);
	value = mxmlFindElement(user, user, "description", NULL, NULL, MXML_DESCEND);
	if(value && value->child) wt_strncpy(dest->description, value->child->value.opaque, 641);
	value = mxmlFindElement(user, user, "profile_image_url", NULL, NULL, MXML_DESCEND);
	if(value && value->child) strncpy(dest->imageurl, value->child->value.opaque, 255);
	value = mxmlFindElement(user, user, "url", NULL, NULL, MXML_DESCEND);
	if(value && value->child) strncpy(dest->url, value->child->value.opaque, 160);
	value = mxmlFindElement(user, user, "followers_count", NULL, NULL, MXML_DESCEND);
	if(value && value->child) sscanf(value->child->value.opaque, "%llu", &(dest->followers));
	value = mxmlFindElement(user, user, "friends_count", NULL, NULL, MXML_DESCEND);
	if(value && value->child) sscanf(value->child->value.opaque, "%llu", &(dest->following));
	value = mxmlFindElement(user, user, "favourites_count", NULL, NULL, MXML_DESCEND);
	if(value && value->child) sscanf(value->child->value.opaque, "%llu", &(dest->favourites));
	value = mxmlFindElement(user, user, "statuses_count", NULL, NULL, MXML_DESCEND);
	if(value && value->child) sscanf(value->child->value.opaque, "%llu", &(dest->tweets));
	value = mxmlFindElement(user, user, "following", NULL, NULL, MXML_DESCEND);
	if(value && value->child){
		if(value->child->value.opaque[0] == 't'){
			dest->flags |= FOLLOWING;
		}else{
			dest->flags |= NFOLLOWING;
		}
	}

	mxmlDelete(value);
}

int parseUsers(std::string &xmlstr, struct t_user ** dest){
	mxml_node_t *xml;
	mxml_node_t *unit;

	struct t_user * users = (struct t_user *)mem2_calloc(MAXUNITS, sizeof(struct t_user), MEM2_OTHER);
	if(!users){
		return 0;
	}

	pre_mxml_processing(xmlstr);

	xml = mxmlLoadString(NULL, xmlstr.c_str(), MXML_OPAQUE_CALLBACK);

	if(!xml){
		mem2_free(users, MEM2_OTHER);
		return 0;
	}

	int t=0;
	for(unit = mxmlFindElement(xml, xml, user_str, NULL, NULL, MXML_DESCEND); unit; unit = mxmlFindElement(unit, xml, user_str, NULL, NULL, MXML_DESCEND)){
		parseUser(unit, &users[t++]);
	}

	mxmlDelete(xml);
	if(t < MAXUNITS){ //Free unused memory
		users = (struct t_user *)mem2_realloc(users, t*sizeof(struct t_user), MEM2_OTHER);
	}

	*dest = users;
return t;
}

void parseTweet(mxml_node_t *tweet, struct t_tweet *dest, int parseuser){
	mxml_node_t *value = NULL;

	if(parseuser){
		value = mxmlFindElement(tweet, tweet, name_str, NULL, NULL, MXML_DESCEND);
		snprintf(dest->retweetby, 96, "Retweeted by %s", value->child->value.opaque);
	}

	value = mxmlFindElement(tweet, tweet, "current_user_retweet", NULL, NULL, MXML_DESCEND);
	if(value && value->child){
		value = mxmlFindElement(tweet, tweet, id_str, NULL, NULL, MXML_DESCEND);
		sscanf(value->child->value.opaque, "%llu", &(dest->retweet_id));
		dest->flags |= RETWEETED;
	}

	value = mxmlFindElement(tweet, tweet, "retweeted_status", NULL, NULL, MXML_DESCEND);
	if(value && value->child){ //The retweeted status becomes the target unit
		tweet = value;
	}else{
		dest->retweetby[0] = 0;
	}

	value = mxmlFindElement(tweet, tweet, "created_at", NULL, NULL, MXML_DESCEND);
	{
		struct tm tm;
		strptime(value->child->value.opaque, tweet_format, &tm);
		tm.tm_isdst = -1;
		dest->created = mktime(&tm) - timeoffset;
	}

	value = mxmlFindElement(tweet, tweet, id_str, NULL, NULL, MXML_DESCEND);
	sscanf(value->child->value.opaque, "%llu", &(dest->id));
	value = mxmlFindElement(tweet, tweet, text_str, NULL, NULL, MXML_DESCEND);
	wt_strncpy(dest->text, value->child->value.opaque, 765);

	if(parseuser){
		value = mxmlFindElement(tweet, tweet, user_str, NULL, NULL, MXML_DESCEND);
		if(value && value->child) parseUser(value, &(dest->user));
	}

	value = mxmlFindElement(tweet, tweet, "favorited", NULL, NULL, MXML_DESCEND);
	if(value && value->child && value->child->value.opaque[0] == 't') dest->flags |= FAVORITED;
	value = mxmlFindElement(tweet, tweet, "retweeted", NULL, NULL, MXML_DESCEND);
	if(value && value->child && value->child->value.opaque[0] == 't') dest->flags |= RETWEETED;

	value = mxmlFindElement(tweet, tweet, "entities", NULL, NULL, MXML_DESCEND);
	if(value && value->child) parseEntities(value, dest);

	mxmlDelete(value);
}

int parseTweets(std::string &xmlstr, struct t_tweet ** dest, int usertop)
{
	mxml_node_t *xml;
	mxml_node_t *unit;

	struct t_tweet * tweets = (struct t_tweet *)mem2_calloc(MAXUNITS, sizeof(struct t_tweet), MEM2_OTHER);
	if(!tweets){
		return 0;
	}

	pre_mxml_processing(xmlstr);

	xml = mxmlLoadString(NULL, xmlstr.c_str(), MXML_OPAQUE_CALLBACK);

	if(!xml){
		mem2_free(tweets, MEM2_OTHER);
		return 0;
	}

	int t=0;
	if(usertop){
		for(unit = mxmlFindElement(xml, xml, user_str, NULL, NULL, MXML_DESCEND); unit && t < MAXUNITS; unit = mxmlFindElement(unit, xml, user_str, NULL, NULL, MXML_DESCEND)){
			parseUser(unit, &(tweets[t].user), &(tweets[t]));
			t++;
		}
	}else{
		for(unit = mxmlFindElement(xml, xml, status_str, NULL, NULL, MXML_DESCEND); unit && t < MAXUNITS; unit = mxmlFindElement(unit, xml, status_str, NULL, NULL, MXML_DESCEND)){
			parseTweet(unit, &tweets[t++]);
		}
	}

	mxmlDelete(xml);
	if(t < MAXUNITS){ //Free unused memory
		tweets = (struct t_tweet *)mem2_realloc(tweets, t*sizeof(struct t_tweet), MEM2_OTHER);
	}

	*dest = tweets;
return t;
}

void getUserInfo (std::string &xmlstr, struct t_login * dest){
	mxml_node_t *xml;
	mxml_node_t *value;

	pre_mxml_processing(xmlstr);

	xml = mxmlLoadString(NULL, xmlstr.c_str(), MXML_OPAQUE_CALLBACK);

	if(!xml){
		return;
	}

	value = mxmlFindElement(xml, xml, id_str, NULL, NULL, MXML_DESCEND);
	sscanf(value->child->value.opaque, "%llu", &(dest->userid));
	value = mxmlFindElement(xml, xml, "profile_image_url", NULL, NULL, MXML_DESCEND);
	strncpy(dest->imageurl, value->child->value.opaque, 255);
	value = mxmlFindElement(xml, xml, screenname_str, NULL, NULL, MXML_DESCEND);
	snprintf(dest->screenname, 20, "@%s", value->child->value.opaque);

	mxmlDelete(xml);
}
