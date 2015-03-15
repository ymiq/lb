
CLBFCGI_TARGET := bin/clb-fcgi
CLBFCGI_SRCS += examples/clb-fcgi/main.cpp
CLBFCGI_SRCS += examples/clb-fcgi/cfg_db.cpp
CLBFCGI_SRCS += examples/clb-fcgi/cmd_srv.cpp
CLBFCGI_SRCS += src/qao/qao_base.cpp
CLBFCGI_SRCS += src/qao/cctl_req.cpp
CLBFCGI_SRCS += src/qao/cctl_rep.cpp
CLBFCGI_SRCS += src/qao/cdat_wx.cpp
CLBFCGI_SRCS += src/hash_alg.cpp
CLBFCGI_SRCS += src/logger.cpp
CLBFCGI_SRCS += src/evsock.cpp
CLBFCGI_SRCS += src/rcu_man.cpp
CLBFCGI_SRCS += src/clb_tbl.cpp
CLBFCGI_SRCS += src/lb_db.cpp
CLBFCGI_SRCS += src/stat/clb_stat.cpp
CLBFCGI_SRCS += src/stat/stat_man.cpp
CLBFCGI_SRCS += src/clb_grp.cpp
CLBFCGI_SRCS += src/clb_clnt.cpp
CLBFCGI_CPPFLAGS = -O2 -Wall -Werror -I./include
CLBFCGI_LDFLAGS = -lstdc++ -lpthread -lfcgi 
CLBFCGI_LDFLAGS += -L/usr/lib64/mysql/ -lmysqlclient
CLBFCGI_LDFLAGS += -lcrypto -levent -ljsoncpp -lexpat

CLBCMD_TARGET := bin/clb-cmd
CLBCMD_SRCS = examples/clb-cmd/main.cpp
CLBCMD_SRCS += examples/clb-cmd/cmd_clnt.cpp
CLBCMD_SRCS += src/qao/qao_base.cpp
CLBCMD_SRCS += src/qao/cctl_req.cpp
CLBCMD_SRCS += src/qao/cctl_rep.cpp
CLBCMD_SRCS += src/rcu_man.cpp
CLBCMD_SRCS += src/evsock.cpp
CLBCMD_SRCS += src/logger.cpp
CLBCMD_SRCS += src/lb_db.cpp
CLBCMD_CPPFLAGS = -O2 -Wall -Werror -I./include
CLBCMD_LDFLAGS = -lstdc++ -lpthread -L/usr/lib64/mysql/ -lmysqlclient -lcrypto -ljsoncpp -levent

CLBPL_TARGET := bin/clb-payload
CLBPL_SRCS = examples/clb-payload/main.cpp
CLBPL_SRCS += examples/clb-payload/http.cpp
CLBPL_CPPFLAGS = -O2 -Wall -Werror -I examples/clb-payload/
CLBPL_LDFLAGS = -lstdc++ -lpthread -lcurl

HUB_TARGET := bin/hub
HUB_SRCS = examples/hub/main.cpp
HUB_SRCS += examples/hub/hub_csrv.cpp
HUB_SRCS += src/qao/qao_base.cpp
HUB_SRCS += src/qao/question.cpp
HUB_SRCS += src/evsock.cpp
HUB_SRCS += src/logger.cpp
HUB_SRCS += src/rcu_man.cpp
HUB_CPPFLAGS = -O2 -Wall -Werror -I./include
HUB_LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp

TEST_TARGET := bin/test
TEST_SRCS = examples/test/main.cpp
TEST_CPPFLAGS = -O2 -Wall -Werror -I./include
TEST_LDFLAGS = -lstdc++ -lpthread -lexpat


.DEFAULT: all
all: clb_fcgi clb_cmd clb_pl hub
	@echo "All done."

.PHONY: clb_fcgi clb_cfgi_clean
clb_fcgi: prepare clb_cfgi_clean
	@echo Building $@
	@gcc $(CLBFCGI_CPPFLAGS) $(CLBFCGI_LDFLAGS) $(CLBFCGI_SRCS) -o $(CLBFCGI_TARGET)

clb_cfgi_clean:
	@rm -f $(TARCLBFCGI_TARGETGET) 
	
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
