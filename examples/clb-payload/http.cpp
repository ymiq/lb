#include "http.h"
#include "curl/curl.h"
#include <string>
#include <ctime>

using namespace std;

static unsigned long total_recv = 0;
static timeval start_tv;
static bool binit = false;

static int on_debug(CURL *curl, 
			curl_infotype itype, char *pdata, size_t size, void *pvoid) {
	if(itype == CURLINFO_TEXT) 	{
		//printf("[TEXT]%s\n", pdata);
	}
	else if(itype == CURLINFO_HEADER_IN) {
		printf("[HEADER_IN]%s\n", pdata);
	}
	else if(itype == CURLINFO_HEADER_OUT) {
		printf("[HEADER_OUT]%s\n", pdata);
	}
	else if(itype == CURLINFO_DATA_IN) 	{
		printf("[DATA_IN]%s\n", pdata);
	}
	else if(itype == CURLINFO_DATA_OUT) {
		printf("[DATA_OUT]%s\n", pdata);
	}
	return 0;
}

static size_t on_write_data(void* buffer, 
			size_t size, size_t nmemb, void* lpdata) {
#if 0				
	string* str = dynamic_cast<string*>((string *)lpdata);
	if( NULL == str || NULL == buffer ) {
		return -1;
	}

    char* pdata = (char*)buffer;
    str->append(pdata, size * nmemb);
#else
	if (!binit) {
		binit = true;
		gettimeofday(&start_tv, NULL);
	}
	
	unsigned long reply = __sync_add_and_fetch(&total_recv, 1);	
	if ((reply % 1000) == 0) {
		struct timeval tv;
		
		gettimeofday(&tv, NULL);
		unsigned long diff;
		if (tv.tv_usec >= start_tv.tv_usec) {
			diff = (tv.tv_sec - start_tv.tv_sec) * 10 + 
				(tv.tv_usec - start_tv.tv_usec) / 100000;
		} else {
			diff = (tv.tv_sec - start_tv.tv_sec - 1) * 10 + 
				(tv.tv_usec + 1000000 - start_tv.tv_usec) / 100000;
		}
		if (!diff) {
			diff = 10;
		}

		printf("REPLAY: %ld, speed: %ld qps\n", reply, (reply * 10) / diff);
	}
#endif
	return nmemb;
}


http::http(void):debug_flag(false) {
	share_handle = curl_share_init();  
	curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);  
	curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_COOKIE);   
}


http::~http(void) {
	curl_share_cleanup(share_handle);
}

int http::post(const string &str_url, 
	const string &str_post, string &str_resp) {
	CURLcode res;
	CURL* curl = curl_easy_init();
	if(NULL == curl) {
		return CURLE_FAILED_INIT;
	}
	if(debug_flag) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, on_debug);
	}
    curl_easy_setopt(curl, CURLOPT_SHARE, share_handle);  
    curl_easy_setopt(curl, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);  
	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_post.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str_resp);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 1);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}


int http::get(const string &str_url, string &str_resp)
{
	CURLcode res;
	CURL* curl = curl_easy_init();
	if(NULL == curl) {
		return CURLE_FAILED_INIT;
	}
	if(debug_flag) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, on_debug);
	}
    curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str_resp);
	/**
	* 当多个线程都使用超时处理的时候，同时主线程中有sleep或是wait等操作。
	* 如果不设置这个选项，libcurl将会发信号打断这个wait从而导致程序退出。
	*/
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}


int http::posts(const string &str_url, const string &str_post, 
							string &str_resp, const char *ca_path) {
	CURLcode res;
	CURL* curl = curl_easy_init();
	if(NULL == curl) {
		return CURLE_FAILED_INIT;
	}
	if(debug_flag) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, on_debug);
	}
	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_POST, 1);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, str_post.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str_resp);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	if(NULL == ca_path) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
	} else {
		//缺省情况就是PEM，所以无需设置，另外支持DER
		//curl_easy_setopt(curl,CURLOPT_SSLCERTTYPE,"PEM");
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
		curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
	}
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}


int http::gets(const string &str_url, string &str_resp, const char *ca_path) {
	CURLcode res;
	CURL* curl = curl_easy_init();
	if(NULL == curl) {
		return CURLE_FAILED_INIT;
	}
	if(debug_flag) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
		curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, on_debug);
	}
	curl_easy_setopt(curl, CURLOPT_URL, str_url.c_str());
	curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&str_resp);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	if(NULL == ca_path) {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, false);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, false);
	} else {
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, true);
		curl_easy_setopt(curl, CURLOPT_CAINFO, ca_path);
	}
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
	res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	return res;
}


void http::enable_debug(bool dbg)
{
	debug_flag = dbg;
}

