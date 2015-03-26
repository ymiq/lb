#ifndef __HTTP_CURL_H__
#define __HTTP_CURL_H__

#include <ctime>
#include <string>
#include "curl/curl.h"

class http
{
public:
	http(void);
	~http(void);

public:
	/**
	* @brief HTTP POST请求
	* @param str_url 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param str_post 输入参数,使用如下格式para1=val1?2=val2&…
	* @param str_resp 输出参数,返回的内容
	* @return 返回是否post成功
	*/
	int post(const std::string &str_url, const std::string &str_post, std::string &str_resp);

	/**
	* @brief HTTP GET请求
	* @param str_url 输入参数,请求的Url地址,如:http://www.baidu.com
	* @param str_resp 输出参数,返回的内容
	* @return 返回是否post成功
	*/
	int get(const std::string &str_url, std::string &str_resp);

	/**
	* @brief HTTPS POST请求,无证书版本
	* @param str_url 输入参数,请求的Url地址,如:https://www.alipay.com
	* @param str_post 输入参数,使用如下格式para1=val1?2=val2&…
	* @param str_resp 输出参数,返回的内容
	* @param ca_path 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性.
	* @return 返回是否post成功
	*/
	int posts(const std::string &str_url, const std::string &str_post, std::string &str_resp, const char *ca_path = NULL);

	/**
	* @brief HTTPS GET请求,无证书版本
	* @param str_url 输入参数,请求的Url地址,如:https://www.alipay.com
	* @param str_resp 输出参数,返回的内容
	* @param ca_path 输入参数,为CA证书的路径.如果输入为NULL,则不验证服务器端证书的有效性.
	* @return 返回是否post成功
	*/
	int gets(const std::string &str_url, std::string &str_resp, const char *ca_path = NULL);

public:
	void enable_debug(bool bDebug);

private:
	bool debug_flag;
	CURL* share_handle;
};

#endif
