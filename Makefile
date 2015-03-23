
CLBFCGI_TARGET := bin/clb-fcgi
CLBFCGI_SRCS += examples/clb-fcgi/main.cpp
CLBFCGI_SRCS += examples/clb-fcgi/cfg_db.cpp
CLBFCGI_SRCS += examples/clb-fcgi/cmd_srv.cpp
CLBFCGI_SRCS += src/qao/qao_base.cpp
CLBFCGI_SRCS += src/qao/cctl_req.cpp
CLBFCGI_SRCS += src/qao/cctl_rep.cpp
CLBFCGI_SRCS += src/qao/cdat_wx.cpp
CLBFCGI_SRCS += src/qao/answer.cpp
CLBFCGI_SRCS += src/utils/hash_alg.cpp
CLBFCGI_SRCS += src/utils/logger.cpp
CLBFCGI_SRCS += src/utils/lb_db.cpp
CLBFCGI_SRCS += src/evsock.cpp
CLBFCGI_SRCS += src/rcu_man.cpp
CLBFCGI_SRCS += src/clb_tbl.cpp
CLBFCGI_SRCS += src/stat/clb_stat.cpp
CLBFCGI_SRCS += src/stat/stat_man.cpp
CLBFCGI_SRCS += src/clb_grp.cpp
CLBFCGI_SRCS += src/clb_clnt.cpp
CLBFCGI_CPPFLAGS = -O2 -Wall -Werror -I./include
CLBFCGI_LDFLAGS = -lstdc++ -lpthread -lfcgi 
CLBFCGI_LDFLAGS += -L/usr/lib64/mysql/ -lmysqlclient
CLBFCGI_LDFLAGS += -lcrypto -levent -ljsoncpp -lexpat

CLBSRV_TARGET := bin/clb-srv
CLBSRV_SRCS += examples/clb-srv/main.cpp
CLBSRV_SRCS += examples/clb-srv/clb_srv.cpp
CLBSRV_SRCS += examples/clb-fcgi/cfg_db.cpp
CLBSRV_SRCS += examples/clb-fcgi/cmd_srv.cpp
CLBSRV_SRCS += src/qao/qao_base.cpp
CLBSRV_SRCS += src/qao/cctl_req.cpp
CLBSRV_SRCS += src/qao/cctl_rep.cpp
CLBSRV_SRCS += src/qao/cdat_wx.cpp
CLBSRV_SRCS += src/qao/answer.cpp
CLBSRV_SRCS += src/utils/hash_alg.cpp
CLBSRV_SRCS += src/utils/logger.cpp
CLBSRV_SRCS += src/utils/lb_db.cpp
CLBSRV_SRCS += src/evsock.cpp
CLBSRV_SRCS += src/rcu_man.cpp
CLBSRV_SRCS += src/clb_tbl.cpp
CLBSRV_SRCS += src/stat/clb_stat.cpp
CLBSRV_SRCS += src/stat/stat_man.cpp
CLBSRV_SRCS += src/clb_grp.cpp
CLBSRV_SRCS += src/clb_clnt.cpp
CLBSRV_CPPFLAGS = -ggdb -Wall -Werror -I./include  -I./examples/clb-fcgi
CLBSRV_LDFLAGS = -lstdc++ -lpthread -lfcgi 
CLBSRV_LDFLAGS += -L/usr/lib64/mysql/ -lmysqlclient
CLBSRV_LDFLAGS += -lcrypto -levent -ljsoncpp -lexpat

CLBCMD_TARGET := bin/clb-cmd
CLBCMD_SRCS = examples/clb-cmd/main.cpp
CLBCMD_SRCS += examples/clb-cmd/cmd_clnt.cpp
CLBCMD_SRCS += src/qao/qao_base.cpp
CLBCMD_SRCS += src/qao/cctl_req.cpp
CLBCMD_SRCS += src/qao/cctl_rep.cpp
CLBCMD_SRCS += src/utils/logger.cpp
CLBCMD_SRCS += src/utils/lb_db.cpp
CLBCMD_SRCS += src/rcu_man.cpp
CLBCMD_SRCS += src/evsock.cpp
CLBCMD_CPPFLAGS = -O2 -Wall -Werror -I./include
CLBCMD_LDFLAGS = -lstdc++ -lpthread -L/usr/lib64/mysql/ -lmysqlclient -lcrypto -ljsoncpp -levent

CLBPL_TARGET := bin/clb-payload
CLBPL_SRCS = examples/clb-payload/main.cpp
CLBPL_SRCS += examples/clb-payload/http.cpp
CLBPL_SRCS += examples/clb-payload/pl_clnt.cpp
CLBPL_SRCS += src/qao/qao_base.cpp
CLBPL_SRCS += src/qao/question.cpp
CLBPL_SRCS += src/qao/candidate.cpp
CLBPL_SRCS += src/qao/answer.cpp
CLBPL_SRCS += src/utils/logger.cpp
CLBPL_SRCS += src/utils/hash_alg.cpp
CLBPL_SRCS += src/rcu_man.cpp
CLBPL_SRCS += src/evsock.cpp
CLBPL_CPPFLAGS = -O2 -Wall -Werror -I./include
CLBPL_LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp -lcrypto -lcurl

HUB_TARGET := bin/hub
HUB_SRCS = examples/hub/main.cpp
HUB_SRCS += examples/hub/hub_csrv.cpp
HUB_SRCS += examples/hub/hub_ssrv.cpp
HUB_SRCS += examples/hub/robot_clnt.cpp
HUB_SRCS += src/qao/qao_base.cpp
HUB_SRCS += src/qao/question.cpp
HUB_SRCS += src/qao/candidate.cpp
HUB_SRCS += src/qao/answer.cpp
HUB_SRCS += src/qao/sclnt_decl.cpp
HUB_SRCS += src/qao/ssrv_factory.cpp
HUB_SRCS += src/utils/logger.cpp
HUB_SRCS += src/utils/hash_alg.cpp
HUB_SRCS += src/evsock.cpp
HUB_SRCS += src/rcu_man.cpp
HUB_CPPFLAGS = -O2 -Wall -Werror -I./include
HUB_LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp -lcrypto

ROBOT_TARGET := bin/robot
ROBOT_SRCS = examples/robot/main.cpp
ROBOT_SRCS += examples/robot/robot_hsrv.cpp
ROBOT_SRCS += src/qao/qao_base.cpp
ROBOT_SRCS += src/qao/question.cpp
ROBOT_SRCS += src/qao/candidate.cpp
ROBOT_SRCS += src/qao/answer.cpp
ROBOT_SRCS += src/utils/logger.cpp
ROBOT_SRCS += src/evsock.cpp
ROBOT_SRCS += src/rcu_man.cpp
ROBOT_CPPFLAGS = -O2 -Wall -Werror -I./include
ROBOT_LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp

SLB_TARGET := bin/slb
SLB_SRCS = examples/slb/main.cpp
SLB_SRCS += examples/slb/slb_clnt.cpp
SLB_SRCS += src/qao/qao_base.cpp
SLB_SRCS += src/qao/question.cpp
SLB_SRCS += src/qao/candidate.cpp
SLB_SRCS += src/qao/answer.cpp
SLB_SRCS += src/qao/sclnt_decl.cpp
SLB_SRCS += src/utils/logger.cpp
SLB_SRCS += src/utils/hash_alg.cpp
SLB_SRCS += src/evsock.cpp
SLB_SRCS += src/rcu_man.cpp
SLB_CPPFLAGS = -O2 -Wall -Werror -I./include
SLB_LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp -lcrypto

TEST_TARGET := bin/test
TEST_SRCS = examples/test/main.cpp
TEST_CPPFLAGS = -O2 -Wall -Werror -I./include
TEST_LDFLAGS = -lstdc++ -lpthread -lexpat


.DEFAULT: all
all: clb_fcgi clb_srv clb_cmd clb_pl hub robot slb
	@echo "All done."

.PHONY: clb_fcgi clb_cfgi_clean
clb_fcgi: prepare clb_cfgi_clean
	@echo Building $@
	@gcc $(CLBFCGI_CPPFLAGS) $(CLBFCGI_LDFLAGS) $(CLBFCGI_SRCS) -o $(CLBFCGI_TARGET)

clb_cfgi_clean:
	@rm -f $(CLBFCGI_TARGET) 
	
.PHONY: clb_srv clb_srv_clean
clb_srv: prepare clb_srv_clean
	@echo Building $@
	@gcc $(CLBSRV_CPPFLAGS) $(CLBSRV_LDFLAGS) $(CLBSRV_SRCS) -o $(CLBSRV_TARGET)

clb_srv_clean:
	@rm -f $(CLBSRV_TARGET) 
	
.PHONY: lb_cmd lb_cmd_clean
clb_cmd: prepare clb_cmd_clean
	@echo Building $@
	@gcc $(CLBCMD_CPPFLAGS) $(CLBCMD_LDFLAGS) $(CLBCMD_SRCS) -o $(CLBCMD_TARGET)

clb_cmd_clean:
	@rm -f $(CLBCMD_TARGET)

.PHONY: clb_pl clb_pl_clean
clb_pl: prepare clb_pl_clean
	@echo Building $@
	@gcc $(CLBPL_CPPFLAGS) $(CLBPL_LDFLAGS) $(CLBPL_SRCS) -o $(CLBPL_TARGET)

clb_pl_clean:
	@rm -f $(CLBPL_TARGET)
	
	
.PHONY: hub hub_clean
hub: prepare hub_clean
	@echo Building $@
	@gcc $(HUB_CPPFLAGS) $(HUB_LDFLAGS) $(HUB_SRCS) -o $(HUB_TARGET)

hub_clean:
	@rm -f $(HUB_TARGET)

.PHONY: robot robot_clean
robot: prepare robot_clean
	@echo Building $@
	@gcc $(ROBOT_CPPFLAGS) $(ROBOT_LDFLAGS) $(ROBOT_SRCS) -o $(ROBOT_TARGET)

robot_clean:
	@rm -f $(ROBOT_TARGET)

.PHONY: slb slb_clean
slb: prepare slb_clean
	@echo Building $@
	@gcc $(SLB_CPPFLAGS) $(SLB_LDFLAGS) $(SLB_SRCS) -o $(SLB_TARGET)

slb_clean:
	@rm -f $(SLB_TARGET)


.PHONY:prepare
prepare:
	@if [ ! -d ./bin ]; then mkdir -p ./bin; fi

clean:
	@rm -fr ./bin

.PHONY: test test_clean
test: prepare test_clean
	@echo Building $@
	@gcc $(TEST_CPPFLAGS) $(TEST_LDFLAGS) $(TEST_SRCS) -o $(TEST_TARGET)

test_clean:
	@rm -f $(TEST_TARGET)
