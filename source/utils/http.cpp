/****************************************************************************
 * WiiTweet
 *
 * Tantric December 2008
 * Pedro Aguiar 2012
 *
 * http.cpp
 *
 * HTTP operations
 * Written by dhewg/bushing, modified by Tantric, trashed by Pedro Aguiar
 ***************************************************************************/

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ogcsys.h>
#include <network.h>
#include <ogc/lwp_watchdog.h>
#include <sys/types.h>
#include <sys/errno.h>
#include <fcntl.h>
#include <zlib.h>
#include <stdio.h>
#include "../menu.h"
#include "fileop.h"
#include "http.h"
#include "ssl.h"
#include "mem2_manager.h"

#define TCP_CONNECT_TIMEOUT 	4000  // 4 secs to make a connection
#define TCP_SEND_SIZE 		(32 * 1024)
#define TCP_RECV_SIZE 		(32 * 1024)
#define TCP_BLOCK_RECV_TIMEOUT 	4000 // 4 secs to receive
#define TCP_BLOCK_SEND_TIMEOUT 	4000 // 4 secs to send
#define TCP_BLOCK_SIZE 		1024
#define HTTP_TIMEOUT 		10000 // 10 secs to get an http response
#define IOS_O_NONBLOCK		0x04

#define MAXTWITCURL 1.5*1024*1024
#define MAX_SIZE (1024*1024*15)
//#define DEBUGHEADERS
//#define DEBUGERRORS

int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label);

int split_res;
u32 http_status, content_length;
char * curl_request = NULL;

//Pointer to read and write functions.
s32 (*writeFunc)(s32, const void *, s32);
s32 (*readFunc)(s32, void *, s32);
s32 *scktctx = NULL; //ugly-named pointer to the correct write/read function's first argument (socket/ssl_context)

//Time correction
time_t timeoffset = 0;
int get_timeoffset = 1;

static s32 tcp_socket(void)
{
	s32 s, res;

	s = net_socket(PF_INET, SOCK_STREAM, IPPROTO_IP);
	if (s < 0)
		return s;

	if(split_res == 2){
		return s;
	}

	res = net_fcntl(s, F_GETFL, 0);
	if (res < 0)
	{
		net_close(s);
		return res;
	}

	res = net_fcntl(s, F_SETFL, res | IOS_O_NONBLOCK);
	if (res < 0)
	{
		net_close(s);
		return res;
	}

	return s;
}

static s32 tcp_connect(char *host, const u16 port)
{
	struct hostent *hp;
	struct sockaddr_in sa;
	struct in_addr val;
	s32 s, res;
	u64 t1;

	s = tcp_socket();
	if (s < 0)
		return s;

	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family= PF_INET;
	sa.sin_len = sizeof(struct sockaddr_in);
	sa.sin_port= htons(port);

	if(strlen(host) < 16 && inet_aton(host, &val))
	{
		sa.sin_addr.s_addr = val.s_addr;
	}
	else
	{
		hp = net_gethostbyname (host);
		if (!hp || !(hp->h_addrtype == PF_INET))
			return errno;

		memcpy((char *) &sa.sin_addr, hp->h_addr_list[0], hp->h_length);
	}

	t1=ticks_to_secs(gettime());
	do 
	{
		res = net_connect(s,(struct sockaddr*) &sa, sizeof (sa));
		if(ticks_to_secs(gettime())-t1 > TCP_CONNECT_TIMEOUT*1000) break; 
		usleep(500);
	}while(res != -EISCONN);
	if(res != -EISCONN)
	{		
		net_close(s);
		return -1;
	}

	
	return s;
}

static int tcp_readln(const s32 s, char *buf, const u16 max_length)
{
	s32 res = -1;
	s32 ret;
	u64 start_time = gettime();
	u16 c = 0;

	while (c < max_length)
	{
		if (ticks_to_millisecs(diff_ticks(start_time, gettime())) > HTTP_TIMEOUT)
			break;

		ret = readFunc(s, &buf[c], 1);

		if (ret == -EAGAIN)
		{
			usleep(20 * 1000);
			continue;
		}

		if (ret <= 0)
			break;

		if (c > 0 && buf[c - 1] == '\r' && buf[c] == '\n')
		{
			res = 0;
			buf[c-1] = 0;
			break;
		}
		c++;
		start_time = gettime();
		usleep(100);
	}

 return res;
//return c;
}

static u32 tcp_read(const s32 s, u8 *buffer, const u32 length, int chunked)
{
	char *p;
	u32 left, block, received, step=0;
	u64 t;
	s32 res;

	p = (char *)buffer;
	left = length;
	received = 0;

	char line[64];
	int chunksize;

	t = gettime();

	while (left || chunked)
	{
		if(chunked == 1){
			tcp_readln(s, line, 64);
			sscanf(line,"%X",&chunksize);
			if(chunksize == 0){
				readFunc(s, line, 64); //Attempt to read a bit more so ssl_shutdown() does not complain...
				break;
			}
			left = chunksize + 2; //Read the \r\n
			chunked = 2;
			t = gettime();
		}

		if (ticks_to_millisecs(diff_ticks(t, gettime()))
				> TCP_BLOCK_RECV_TIMEOUT)
		{
			break;
		}

		block = left;
		if (block > TCP_RECV_SIZE)
			block = TCP_RECV_SIZE;

		res = readFunc(s, p, block);

		if (res == -EAGAIN)
		{
			usleep(20 * 1000);
			continue;
		}

		if(res<=0) break; 

		received += res;
		left -= res;
		if(left == 0 && chunked == 2){ chunked = 1; p -= 2; received -= 2; } //Overwrite \r\n
		p += res;
		usleep(1000);

		if ((received / TCP_BLOCK_SIZE) > step)
		{
			t = gettime ();
			step++;
		}
	}
	return received;
}

static u32 tcp_write(const s32 s, const u8 *buffer, const u32 length)
{
	const u8 *p;
	u32 left, block, sent, step=0;
	s64 t;
	s32 res;

	p = buffer;
	left = length;
	sent = 0;

	t = gettime();
	while (left)
	{
		if (ticks_to_millisecs(diff_ticks(t, gettime()))
				> TCP_BLOCK_SEND_TIMEOUT)
		{
			break;
		}

		block = left;
		if (block > TCP_SEND_SIZE)
			block = TCP_SEND_SIZE;

		res = writeFunc(s, p, block);

		if ((res == 0) || (res == -56))
		{
			usleep(20 * 1000);
			continue;
		}

		if (res < 0)
			break;

		sent += res;
		left -= res;
		p += res;
		usleep(100);

		if ((sent / TCP_BLOCK_SIZE) > step)
		{
			t = gettime ();
			step++;
		}
	}

	return left == 0;
}

unsigned int http_split_url(char *host, char *path, const char *url)
{
	const char *p;
	char *c;
	int http = 1;

	if (!strncasecmp(url, "https://", 8)){
		p = url+8;
		http = 2;
	}else if (!strncasecmp(url, "http://", 7)){
		p = url+7;
	}else{
		return 0;
	}

	c = strchr(p, '/');

	if (c == NULL || c[0] == 0)
		return 0;

	snprintf(host, c-p+1, "%s", p);
	strcpy(path, c);

	return http;
}

voidpf mem2_zalloc (voidpf opaque, uInt items, uInt size){
	void * block = mem2_malloc(size*items, MEM2_OTHER);
	if(block == NULL) return Z_NULL;

	return block;
}

void mem2_zfree(voidpf opaque, voidpf address){
	mem2_free(address, MEM2_OTHER);
}

int httpInflate(void * output, const void * input, unsigned int length, unsigned int type){
	z_stream * stream = (z_stream *)mem2_malloc(sizeof(z_stream), MEM2_OTHER);
	memset(stream, 0, sizeof(z_stream));

	stream->zalloc = mem2_zalloc;
	stream->zfree = mem2_zfree;
	stream->opaque = Z_NULL;
	stream->avail_in = length;
	stream->next_in = (Bytef *)input;
	stream->avail_out = MAXTWITCURL;
	stream->next_out = (Bytef *)output;

	int ret;

	if(type == 1){ //deflate
		ret = inflateInit(stream);
		if(ret != Z_OK) return ret;
	}else if(type == 2){ //gzip
		ret = inflateInit2(stream, 16+MAX_WBITS);
		if(ret != Z_OK) return ret;
	}else{ 
		return -10;
	}

	ret = inflate(stream, Z_NO_FLUSH);
	if(ret != Z_STREAM_END){
		inflateEnd(stream);
		return ret;
	}

	ret = inflateEnd(stream);
	if(ret != Z_OK) return ret;

	int size = (MAXTWITCURL - stream->avail_out);
	mem2_free(stream, MEM2_OTHER);
return size;
}

/****************************************************************************
 * http_request
 * Retrieves the specified URL, and stores it in the specified file or buffer
 ***************************************************************************/
int http_request(const char *url, FILE *hfile, u8 *buffer, u32 maxsize, bool silent, bool accept_encoding)
{
	int res = 0; int chunked = 0;
	char http_host[64];
	char http_path[256];
	char content_encoding[16] = "";
	u16 http_port;
	#ifdef DEBUGHEADERS
		int debugging = 0;
	#endif
	http_res result;
	u32 sizeread = 0;

	content_length = 0;

	int linecount;

	if(maxsize > MAX_SIZE){
		#ifdef DEBUGERRORS
			InfoPrompt("maxsize > MAX_SIZE");
		#endif
		return 0;
	}

	if (url == NULL || (hfile == NULL && buffer == NULL)){
		#ifdef DEBUGERRORS
			InfoPrompt("!url || (!hfile && !buffer)");
		#endif
		return 0;
	}

	if(!silent)
		ShowAction("Sending data...");
	
	split_res = http_split_url(http_host, http_path, url); // 2 : https ;  1 : http ;  0 : invalid url

	if (split_res == 2){
		http_port = 443;
		writeFunc = ssl_write;
		readFunc = ssl_read;		
	}else if( split_res == 1 ){
		http_port = 80;
		writeFunc = net_write;
		readFunc = net_read;
	}else{
		#ifdef DEBUGERRORS
			InfoPrompt("Invalid url");
		#endif
		return 0;
	}

	http_status = 404;

	int s = tcp_connect(http_host, http_port);

	if (s < 0)
	{
		result = HTTPR_ERR_CONNECT;
		#ifdef DEBUGERRORS
			InfoPrompt("Socket!");
		#endif
		return 0;
	}

	int ssl_context = 0;
	
	if(split_res == 2){  
		ssl_context = ssl_setup(http_host, s);
		#ifdef DEBUGERRORS
			if(ssl_context < 0){
				InfoPrompt("ssl_context() failed");
			}
		#endif
		if(ssl_context < 0){
			net_close(s);
			return 0;
		}
		scktctx = &ssl_context;
	} else{
		scktctx = &s;
	}

	if(curl_request){ //Request made by through the CURL class
		res = tcp_write(*scktctx, (u8 *) curl_request, strlen(curl_request));
	}else{
		char request[1024];
		char *r = request;
		r += sprintf(r, "GET %s HTTP/1.1\r\n", http_path);
		r += sprintf(r, "Host: %s\r\n", http_host);
		if(accept_encoding && hfile){
			r += sprintf(r, "Accept-Encoding: gzip, deflate\r\n");
		}
		r += sprintf(r, "Cache-Control: no-cache\r\n\r\n");
		res = tcp_write(*scktctx, (u8 *) request, strlen(request));
	}

	if(!silent)
		CancelAction();

	#ifdef DEBUGHEADERS
		InfoPrompt(http_path);
	#endif
	char line[1024]; //Twitter sends a long header

	for (linecount = 0; linecount < 45; linecount++)
	{
		if (tcp_readln(*scktctx, line, 1024) != 0)
		{
			#ifdef DEBUGERRORS
				InfoPrompt("tcp_readln != 0");
			#endif
			http_status = 404;
			result = HTTPR_ERR_REQUEST;
			break;
		}

		if (!line[0])
			break;

		#ifdef DEBUGHEADERS
				if(sscanf(line, "HTTP/1.%*u %u", &http_status)){
					if(http_status != 200)
						debugging = 1;
				}

				if(sscanf(line, "Content-Length: %u", &content_length) || sscanf(line, "Content-Encoding: %s", content_encoding)){
					if(!debugging){
						InfoPrompt(line);
					}
				}
				if(!strncmp(line, "Transfer-Encoding: chunked", 25)){ InfoPrompt("Transfer-Encoding: chunked"); chunked = 1; }
		#else
				sscanf(line, "HTTP/1.%*u %u", &http_status);
				sscanf(line, "Content-Length: %u", &content_length);
				sscanf(line, "Content-Encoding: %s", content_encoding);
				if(!strncmp(line, "Transfer-Encoding: chunked", 25)) chunked = 1;
		#endif

		u32 api_ratelimit=0;
		if(sscanf(line, "X-RateLimit-Remaining: %u", &api_ratelimit) && api_ratelimit <= 10 && api_ratelimit % 5 == 0){
			WindowPrompt("You are on fire!", "You are about to reach Twitter's requests limit. WiiTweet will not work correctly then.", "I'll take a break", 0);
		}
		
		if(get_timeoffset){
			if(!strncasecmp(line, "Date:", 5)){ //Case insensitiveness just in case...
				const char format[] = "%a, %d %b %Y %H:%M:%S %Z";
				const char *pointline = line;
				pointline += 6;
				struct tm tm;
				memset(&tm, 0, sizeof(tm));
				strptime(pointline, format, &tm);
				timeoffset = mktime(&tm) - time(NULL);
				get_timeoffset = 0;
			}
		}
		#ifdef DEBUGHEADERS
			if(debugging){
				InfoPrompt(line);
			}
		#endif
	}

	if (http_status != 200)
	{
		result = HTTPR_ERR_STATUS;
		#ifdef DEBUGERRORS
			if(ssl_context){
				if(ssl_shutdown(ssl_context)){
					InfoPrompt("ssl_shutdown() 1");
				}
			}
			net_close(s);
		#else
			if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
		#endif
		#ifdef DEBUGERRORS
			char status[64];
			sprintf(status, "HTTP Status = %d", http_status);
			InfoPrompt(status);
		#endif
//		return 0;
	}//Try to read anyways? ssl gets rude if it is not convinced there is no data

	//length unknown - just read as much as we can
	if(content_length == 0)
	{
		content_length = maxsize;
	}
	else if (content_length > maxsize) //ssl_shutdown() would fail in this case (?), but it is not likely for our purposes...
	{
		result = HTTPR_ERR_TOOBIG;
		#ifdef DEBUGERRORS
			if(ssl_context){
				if(ssl_shutdown(ssl_context)){
					InfoPrompt("ssl_shutdown() 2");
				}
			}
			net_close(s);
		#else
			if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
		#endif
		#ifdef DEBUGERRORS
			InfoPrompt("content_length > maxsize");
		#endif
		return 0;
	}

	unsigned int inflatetype = 0;
	if(!strncasecmp(content_encoding, "gzip", 4)){
		inflatetype = 2;
	}else if(!strncasecmp(content_encoding, "deflate", 7)){
		inflatetype = 1;
	}else if(content_encoding[0] != '\0'){//Unsupported encoding. This should never happen.
		#ifdef DEBUGERRORS
			if(ssl_context){
				if(ssl_shutdown(ssl_context)){
					InfoPrompt("ssl_shutdown() 3");
				}
			}
			net_close(s);
		#else
			if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
		#endif
		#ifdef DEBUGERRORS
			InfoPrompt("Unsupported encoding");
		#endif
		return 0;
	}

	if (buffer != NULL)
	{
		if(!silent)
			ShowAction("Downloading...");

		if(inflatetype){ //Compressed content
			u8 * inflate_me = (u8 *) mem2_malloc(content_length, MEM2_OTHER);
			if(!inflate_me){
				#ifdef DEBUGERRORS
					if(ssl_context){
						if(ssl_shutdown(ssl_context)){
							InfoPrompt("ssl_shutdown() 4");
						}
					}
					net_close(s);
				#else
					if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
				#endif
				#ifdef DEBUGERRORS
					InfoPrompt("!inflate_me");
				#endif
				return 0;
			}
			#ifdef DEBUGHEADERS
				int tcpread = tcp_read(*scktctx, inflate_me, content_length, chunked);
				char atoi[64];
				sprintf(atoi, "%d", tcpread);
				WindowPrompt("tcp_read()", atoi, "ok", 0);
			#else
				int tcpread = tcp_read(*scktctx, inflate_me, content_length, chunked);
			#endif

			sizeread = httpInflate(buffer, inflate_me, tcpread, inflatetype);
			if(sizeread < 0){
				mem2_free(inflate_me, MEM2_OTHER);
				#ifdef DEBUGERRORS
					InfoPrompt("httpInflate() < 0");
					if(ssl_context){
						if(ssl_shutdown(ssl_context)){
							InfoPrompt("ssl_shutdown() 5");
						}
					}
					net_close(s);
				#else
					if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
				#endif
				#ifdef DEBUGERRORS
					InfoPrompt("sizeread < 0");
				#endif
				return 0;
			}
			#ifdef DEBUGHEADERS
			else{
				InfoPrompt("Inflated OK!");
			}
			#endif

			mem2_free(inflate_me, MEM2_OTHER);
		}else{ //Uncomprpessed content
			sizeread = tcp_read(*scktctx, buffer, content_length, chunked);
		}

		if(!silent)
			CancelAction();
	}
	else // write into file
	{
		/* Uncompressed data. This may fail if the content is chunked and longer than 32KB+2B but chunked is not used in such scenarios */

		u32 bufSize = (1024 * 32);
		u32 bytesLeft = content_length;
		u32 readSize;

		if(!silent)
			ShowProgress("Downloading...", 0, content_length);
		u8 * fbuffer = (u8 *) malloc(bufSize);
		if(fbuffer)
		{
			while (bytesLeft > 0)
			{
				if (bytesLeft < bufSize)
					readSize = bytesLeft;
				else
					readSize = bufSize;

				res = tcp_read(*scktctx, fbuffer, readSize, chunked);
				if (!res)
					break;

				sizeread += res;
				bytesLeft -= res;

				res = fwrite(fbuffer, 1, res, hfile);
				if (!res)
					break;
 
				if(!silent)
					ShowProgress("Downloading...", (content_length - bytesLeft), content_length);
			}
			free(fbuffer);
		}
		if(!silent)
			CancelAction();
	}

	#ifdef DEBUGERRORS
		if(ssl_context){
			if(ssl_shutdown(ssl_context)){
				InfoPrompt("ssl_shutdown() 6");
			}
		}
		net_close(s);
	#else
		if(ssl_context){ssl_shutdown(ssl_context);} net_close(s);
	#endif

	if (content_length < maxsize && sizeread != content_length && !inflatetype)
	{
		#ifdef DEBUGERRORS
			InfoPrompt("ERR_RECEIVE");
		#endif
		result = HTTPR_ERR_RECEIVE;
		return 0;
	}

	if (http_status != 200){
		#ifdef DEBUGERRORS
			InfoPrompt("http_status != 200");
		#endif
		return 0;
	}

	if(result) //Avoid ugly compiler warning :p
		result = HTTPR_OK;

	return sizeread;
}
