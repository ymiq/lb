#ifndef __HUB_GLOBAL_H__
#define __HUB_GLOBAL_H__

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <string>
#include <evsock.h>
#include <hash_bind.h>
#include "hub_csrv.h"
#include "hub_ssrv.h"
#include "robot_clnt.h"

using namespace std;

#define CSRV_BIND		hash_bind<hub_csrv*, 1024, 16>
#define SSRV_BIND		hash_bind<hub_ssrv*, 1024, 16>


extern "C" {
	
extern CSRV_BIND *csrv_bind;
extern SSRV_BIND *ssrv_bind;
extern robot_clnt *robot_sock;

}

#endif /* __HUB_GLOBAL_H__ */
