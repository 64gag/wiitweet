/****************************************************************************
 * WiiTweet
 *
 * Pedro Aguiar 2012
 *
 * curl.h
 *
 * A class that emulates curl features used by twitcurl
 ***************************************************************************/

#include "utils/mem2_manager.h"
#include "curl.h"
#include "utils/http.h"
#include "fileop.h"

extern int split_res;
extern u32 http_status;
extern char * curl_request;
u8 * http_response = NULL;

int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);

CURL::CURL(){
	accept_encoding = 1;
	json = 0;
}

CURL::~CURL(){
	if(http_response){
		mem2_free(http_response, MEM2_OTHER);
		http_response = NULL;
	}
}

void CURL::setMethod(int option)
{
	switch(option)
	{
		case 3: method.assign("GET"); break;
		case 4: method.assign("POST"); break;
		case 5: method.assign("DELETE"); break;
	}
}

void CURL::URLsplit(){
	std::string httpless;
	size_t protocol = (this->url).find("//") + 2;
	httpless = (this->url).substr(protocol);
	host.assign(httpless.substr(0,httpless.find('/')));
	path.assign(httpless.substr(httpless.find('/')));
}

void InfoPrompt(const char *);
void CURL::buildHttpRequest(void){
	std::string request;
	std::string requestline;

	request = method+" "+path+" HTTP/1.1\r\n";
	request += "Host: "+host+"\r\n";
	request += "User-Agent: Nintendo Wii (WiiTweet)\r\n";
	
//	if(accept_encoding && path.find("/oauth/authorize") == std::string::npos){ //Don't accept encoding on requests to /oauth/authorize because the second decoding fails randomly (I do not know why)
	if(accept_encoding){
		request += "Accept-Encoding: gzip, deflate\r\n";
	}

	request += "Accept-Language: en-us,en;q=1\r\n";
	request += "Cache-Control: no-cache\r\n";
	if(headers.length()){ request += headers + "\r\n"; }

	if(!method.compare("POST")){
		if(json){
			request += "Content-Type: application/json\r\n";
		}else{
			request += "Content-Type: application/x-www-form-urlencoded\r\n";
		}
		request += "Content-Length: ";
		{
			char itoa[8];
			sprintf(itoa, "%d", postdata.length());
			request.append(itoa);
		}
		request += "\r\n\r\n";
		request += postdata;
	}else{
		request += "\r\n";
	}

	this->req.assign(request);
}

//These are the functions twitcurl attempts to use, the CURL class has to make twitcurl think it is actually "talking" to libcurl

CURL *curl_easy_init(void){
 	CURL * tmp = new CURL;

return tmp;
}

void curl_easy_cleanup(CURL *curl){
	delete curl;
}

//Should have used overloaded functions...
CURLcode curl_easy_setopt(CURL *curl, int option, ...)
{

void *argument;
cbptr fp=NULL;

	if(option <= 0){ return 0; }
	va_list arg;
	va_start (arg, option);

	if(option == CURLOPT_WRITEFUNCTION){
		fp = va_arg(arg, cbptr);
	}else{
		argument = va_arg(arg, void *);
	}

	switch(option){
		case CURLOPT_HTTPHEADER:
			(curl->headers).assign(((curl_slist *)argument)->data);
		break;
		case CURLOPT_URL: 
			(curl->url).assign((char *)argument);
		break;
		case CURLOPT_HTTPGET: 
			curl->setMethod(option);
		break;
		case CURLOPT_POST:
			curl->setMethod(option);
		break;
		case CURLOPT_COPYPOSTFIELDS:
			(curl->postdata).assign((char *)argument);
		break;
		case CURLOPT_CUSTOMREQUEST:
			if(argument){
				curl->setMethod(5);
			}else{
				curl->setMethod(3);
			}
		break;
		case CURLOPT_WRITEFUNCTION:
			curl->callback = fp;
		break;
		case CURLOPT_WRITEDATA:
			curl->userdata = argument;
		break;
		case CURLOPT_JSON:
			curl->json = option;
		break;
	}

	va_end(arg);

return 0;
}

CURLcode curl_easy_getinfo(CURL *curl, CURLINFO info, long unsigned int * stat){
	*stat = http_status;
	return http_status;
}

struct curl_slist * curl_slist_append(struct curl_slist * clist, const char * str){
	struct curl_slist * temp;

	temp = (curl_slist *)mem2_malloc(sizeof(curl_slist), MEM2_OTHER);
	if(temp == NULL){
		return NULL;
	}

	temp->data = (char *)mem2_malloc(strlen(str)+1, MEM2_OTHER);
	if(temp->data == NULL){
		mem2_free(temp, MEM2_OTHER);
		return NULL;
	}

	strcpy(temp->data, str);

	return temp;
}

void curl_slist_free_all(struct curl_slist * clist){
	if(clist->data){
		mem2_free(clist->data, MEM2_OTHER);
		clist->data = NULL;
	}

	if(clist){
		mem2_free(clist, MEM2_OTHER);
		clist = NULL;
	}
}

#define MAXTWITCURL (1024*1024*1.5)

CURLcode curl_easy_perform(CURL *curl){ 

	if(http_response == NULL){
		http_response = (u8 *) mem2_malloc(MAXTWITCURL, MEM2_OTHER);
	}

	curl->URLsplit();
	curl->buildHttpRequest();

	curl_request = mem2_strdup(curl->req.c_str(), MEM2_OTHER);
	if(!curl_request){
		return 1; // No memory
	}

	int ret = (http_request(curl->url.c_str(), NULL, http_response, MAXTWITCURL, 1, curl->accept_encoding));

	if(curl->callback)
		curl->callback((char *)http_response, ret, 1, curl->userdata);


	mem2_free(curl_request, MEM2_OTHER);
	curl_request = NULL;

	curl->postdata.clear();
	curl->req.clear();
	curl->headers.clear();

return !ret; //0 means ok to twitcurl
}
