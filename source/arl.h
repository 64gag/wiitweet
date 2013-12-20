/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar
 *
 * arl.cpp
 *
 * API resource locator class.
 * Stores the parameters passed to the API in previous calls.
 * Support for browsing backwards/forward between browsed API resources (history).
 ***************************************************************************/
#ifndef ARL_H
#define ARL_H

struct t_arl{
	char endpoint;
	u64 user_id;
	u64 arg_id;
	u64 arg_id_2;
	char * text;
};

class ARLs{
 public:
	ARLs(int max_e);
	~ARLs();
	void *operator new(size_t size);
	void operator delete(void *p);
	struct t_arl * Build(char ep, u64 arg1, u64 arg2, u64 arg3, const char * str);
	struct t_arl * Previous();
	struct t_arl * Next();
	struct t_arl * GetSelected();
	void Clear();
 private:
	unsigned int maxelements;
	unsigned int selected_index;
	std::list<struct t_arl> elements;
	std::list<struct t_arl>::iterator it;
};

#endif
