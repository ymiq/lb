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
	* @brief HTTP POST����
	* @param str_url �������,�����Url��ַ,��:http://www.baidu.com
	* @param str_post �������,ʹ�����¸�ʽpara1=val1?2=val2&��
	* @param str_resp �������,���ص�����
	* @return �����Ƿ�post�ɹ�
	*/
	int post(const std::string &str_url, const std::string &str_post, std::string &str_resp);

	/**
	* @brief HTTP GET����
	* @param str_url �������,�����Url��ַ,��:http://www.baidu.com
	* @param str_resp �������,���ص�����
	* @return �����Ƿ�post�ɹ�
	*/
	int get(const std::string &str_url, std::string &str_resp);

	/**
	* @brief HTTPS POST����,��֤��汾
	* @param str_url �������,�����Url��ַ,��:https://www.alipay.com
	* @param str_post �������,ʹ�����¸�ʽpara1=val1?2=val2&��
	* @param str_resp �������,���ص�����
	* @param ca_path �������,ΪCA֤���·��.�������ΪNULL,����֤��������֤�����Ч��.
	* @return �����Ƿ�post�ɹ�
	*/
	int posts(const std::string &str_url, const std::string &str_post, std::string &str_resp, const char *ca_path = NULL);

	/**
	* @brief HTTPS GET����,��֤��汾
	* @param str_url �������,�����Url��ַ,��:https://www.alipay.com
	* @param str_resp �������,���ص�����
	* @param ca_path �������,ΪCA֤���·��.�������ΪNULL,����֤��������֤�����Ч��.
	* @return �����Ƿ�post�ɹ�
	*/
	int gets(const std::string &str_url, std::string &str_resp, const char *ca_path = NULL);

public:
	void enable_debug(bool bDebug);

private:
	bool debug_flag;
	CURL* share_handle;
};

#endif
