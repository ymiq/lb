
LBCGI_TARGET := bin/lb-fcgi
LBCGI_SRCS += examples/lb-fcgi/main.cpp
LBCGI_SRCS += examples/lb-fcgi/lbdb.cpp
LBCGI_SRCS += src/logger.cpp
LBCGI_SRCS += src/rcu_man.cpp
LBCGI_SRCS += src/lb_table.cpp
LBCGI_SRCS += src/stat_obj.cpp
LBCGI_SRCS += src/stat_table.cpp
LBCGI_SRCS += src/stat_man.cpp
LBCGI_CPPFLAGS = -O2 -Werror -I./include
LBCGI_LDFLAGS = -l stdc++ -l pthread -l fcgi -L/usr/lib64/mysql/ -l mysqlclient -l crypto

LBCMD_TARGET := bin/lb-cmd
LBCMD_SRCS = examples/lb-cmd/main.cpp
LBCMD_SRCS += examples/lb-cmd/lbdb.cpp
LBCMD_LDFLAGS = -l stdc++ -l pthread -L/usr/lib64/mysql/ -l mysqlclient -l crypto
LBCMD_CPPFLAGS = -O2 -Werror

LBSRV_TARGET := bin/lb-server
LBSRV_SRCS = examples/lb-server/main.cpp
LBSRV_LDFLAGS = -l stdc++ -l pthread -l event
LBSRV_CPPFLAGS = -O2 -Werror

LBPL_TARGET := bin/lb-payload
LBPL_SRCS = examples/lb-payload/main.cpp
LBPL_SRCS += examples/lb-payload/http.cpp
LBPL_LDFLAGS = -l stdc++ -l pthread -l curl
LBPL_CPPFLAGS = -O2 -Werror -I examples/lb-payload/

.DEFAULT: all
all: lb_fcgi lb_cmd lb_srv lb_pl
	@echo "All done."

.PHONY: lb_fcgi lb_cfgi_clean
lb_fcgi: lb_cfgi_clean
	@echo Building $@
	@gcc $(LBCGI_CPPFLAGS) $(LBCGI_LDFLAGS) $(LBCGI_SRCS) -o $(LBCGI_TARGET)

lb_cfgi_clean:
	@rm -f $(TARLBCGI_TARGETGET) 
	
.PHONY: lb_cmd lb_cmd_clean
lb_cmd: lb_cmd_clean
	@echo Building $@
	@gcc $(LBCMD_CPPFLAGS) $(LBCMD_LDFLAGS) $(LBCMD_SRCS) -o $(LBCMD_TARGET)

lb_cmd_clean:
	@rm -f $(LBCMD_TARGET)

.PHONY: lb_srv lb_srv_clean
lb_srv: lb_srv_clean
	@echo Building $@
	@gcc $(LBSRV_CPPFLAGS) $(LBSRV_LDFLAGS) $(LBSRV_SRCS) -o $(LBSRV_TARGET)

lb_srv_clean:
	@rm -f $(LBSRV_TARGET)

.PHONY: lb_pl lb_pl_clean
lb_pl: lb_pl_clean
	@echo Building $@
	@gcc $(LBPL_CPPFLAGS) $(LBPL_LDFLAGS) $(LBPL_SRCS) -o $(LBPL_TARGET)

lb_pl_clean:
	@rm -f $(LBPL_TARGET)
clean:
	@rm -fr ./bin/*
