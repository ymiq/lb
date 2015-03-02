
LBCGI_TARGET := bin/clb-fcgi
LBCGI_SRCS += examples/clb-fcgi/main.cpp
LBCGI_SRCS += examples/clb-fcgi/lbdb.cpp
LBCGI_SRCS += examples/clb-fcgi/cmdsrv.cpp
LBCGI_SRCS += src/logger.cpp
LBCGI_SRCS += src/evsock.cpp
LBCGI_SRCS += src/rcu_man.cpp
LBCGI_SRCS += src/lb_table.cpp
LBCGI_SRCS += src/stat_obj.cpp
LBCGI_SRCS += src/stat_table.cpp
LBCGI_SRCS += src/stat_man.cpp
LBCGI_CPPFLAGS = -O2 -Wall -Werror -I./include
LBCGI_LDFLAGS = -lstdc++ -lpthread -lfcgi -L/usr/lib64/mysql/ -lmysqlclient -lcrypto -levent

LBCMD_TARGET := bin/clb-cmd
LBCMD_SRCS = examples/clb-cmd/main.cpp
LBCMD_SRCS += examples/clb-cmd/lbdb.cpp
LBCMD_SRCS += examples/clb-cmd/command.cpp
LBCMD_LDFLAGS = -lstdc++ -lpthread -L/usr/lib64/mysql/ -lmysqlclient -lcrypto
LBCMD_CPPFLAGS = -O2 -Wall -Werror

LBSRV_TARGET := bin/hub
LBSRV_SRCS = examples/hub/main.cpp
LBSRV_SRCS += examples/hub/clbsrv.cpp
LBSRV_SRCS += src/evsock.cpp
LBSRV_SRCS += src/logger.cpp
LBSRV_LDFLAGS = -lstdc++ -lpthread -levent
LBSRV_CPPFLAGS = -O2 -Wall -Werror -I./include

LBPL_TARGET := bin/clb-payload
LBPL_SRCS = examples/clb-payload/main.cpp
LBPL_SRCS += examples/clb-payload/http.cpp
LBPL_LDFLAGS = -lstdc++ -lpthread -lcurl
LBPL_CPPFLAGS = -O2 -Wall -Werror -I examples/clb-payload/

.DEFAULT: all
all: clb_fcgi clb_cmd clb_pl hub
	@echo "All done."

.PHONY: clb_fcgi clb_cfgi_clean
clb_fcgi: clb_cfgi_clean
	@echo Building $@
	@gcc $(LBCGI_CPPFLAGS) $(LBCGI_LDFLAGS) $(LBCGI_SRCS) -o $(LBCGI_TARGET)

clb_cfgi_clean:
	@rm -f $(TARLBCGI_TARGETGET) 
	
.PHONY: lb_cmd lb_cmd_clean
clb_cmd: clb_cmd_clean
	@echo Building $@
	@gcc $(LBCMD_CPPFLAGS) $(LBCMD_LDFLAGS) $(LBCMD_SRCS) -o $(LBCMD_TARGET)

clb_cmd_clean:
	@rm -f $(LBCMD_TARGET)

.PHONY: clb_pl clb_pl_clean
clb_pl: clb_pl_clean
	@echo Building $@
	@gcc $(LBPL_CPPFLAGS) $(LBPL_LDFLAGS) $(LBPL_SRCS) -o $(LBPL_TARGET)

clb_pl_clean:
	@rm -f $(LBPL_TARGET)
	
	
.PHONY: hub hub_clean
hub: hub_clean
	@echo Building $@
	@gcc $(LBSRV_CPPFLAGS) $(LBSRV_LDFLAGS) $(LBSRV_SRCS) -o $(LBSRV_TARGET)

hub_clean:
	@rm -f $(LBSRV_TARGET)


clean:
	@rm -fr ./bin/*
