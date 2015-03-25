#include "http.h"
#include "curl/curl.h"
#include <string>
#include <ctime>

using namespace std;

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
	string* str = dynamic_cast<string*>((string *)lpdata);
	if( NULL == str || NULL == buffer ) {
		return -1;
	}

    char* pdata = (char*)buffer;
    str->append(pdata, size * nmemb);
	return nmemb;
}


http::http(void):debug_flag(false) {
	share_handle = curl_share_init();  
	curl_share_setopt(share_handle, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);  

}

http::~http(void) {

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
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 3);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3);
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
	* ������̶߳�ʹ�ó�ʱ�����ʱ��ͬʱ���߳�����sleep����wait�Ȳ�����
	* ������������ѡ�libcurl���ᷢ�źŴ�����wait�Ӷ����³����˳���
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
		//ȱʡ�������PEM�������������ã�����֧��DER
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

