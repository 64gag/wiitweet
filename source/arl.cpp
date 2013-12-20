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

#include <list>
#include <gccore.h>
#include <stdlib.h>
#include <string.h>
#include "utils/mem2_manager.h"

#include "arl.h"

ARLs::ARLs(int max_e){
	selected_index = 0;
	maxelements = max_e;
}

struct t_arl * ARLs::GetSelected(){
	it = elements.begin();
	advance(it, selected_index);
 return &*it;
}

struct t_arl * ARLs::Next(){

	if(selected_index + 1 < elements.size()){
		selected_index++;
	}else{
		return NULL;
	}

 return GetSelected();
}

struct t_arl * ARLs::Previous(){

	if(selected_index > 0){
		selected_index--;
	}else{
		return NULL;
	}

 return GetSelected();
}

ARLs::~ARLs(){

}

void ARLs::Clear(){
	it = elements.begin();
	while(it != elements.end()){
		if((*it).text){
			mem2_free((*it).text, MEM2_OTHER);
		}
		++it;
	}
	elements.clear();
}

void *ARLs::operator new(size_t size)
{
	void *p = mem2_malloc(size, MEM2_OTHER);

	if (!p)
	{
		std::bad_alloc ba;
		throw ba;
	}
	return p;
}

// overloaded delete operator
void ARLs::operator delete(void *p)
{
	mem2_free(p, MEM2_OTHER);
}

struct t_arl * ARLs::Build(char ep, u64 arg1, u64 arg2, u64 arg3, const char * str){
 struct t_arl temp;
 memset(&temp, 0, sizeof(struct t_arl));

	if(elements.size() && (selected_index + 1 != elements.size())){
		it = elements.begin();
		advance(it, selected_index+1);
		while((++it) != elements.end()){
			if((*it).text) mem2_free((*it).text, MEM2_OTHER);
		}
		it = elements.begin();
		advance(it, selected_index+1);
		elements.erase(it, elements.end());
	}else if(elements.size() == maxelements){
		elements.erase(elements.begin());
		selected_index--;
	}

	temp.endpoint = ep;
	temp.user_id = arg1;
	temp.arg_id = arg2;
	temp.arg_id_2 = arg3;
	if(str)
		temp.text = mem2_strdup(str, MEM2_OTHER);

	if(elements.size() == maxelements){
		it = elements.begin();
		if((*it).text) mem2_free((*it).text, MEM2_OTHER);
		elements.erase(it);
	}

	elements.push_back(temp);

	selected_index = elements.size()-1;

 return GetSelected();
}
