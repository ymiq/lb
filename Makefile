
TARGET := bin/structs

SRCS += examples/main.cpp
SRCS += examples/lbdb.cpp
SRCS += src/rcu_man.cpp
SRCS += src/lb_table.cpp
SRCS += src/stat_obj.cpp
SRCS += src/stat_table.cpp
SRCS += src/stat_man.cpp

CPPFLAGS = -O2 -Werror -I./include
LDFLAGS = -l stdc++ -l pthread -L/usr/lib64/mysql/ -lmysqlclient

LBCMD_TARGET := bin/lb-cmd
LBCMD_SRCS = examples/lb-cmd/main.cpp
LBCMD_SRCS += examples/lb-cmd/lbdb.cpp
LBCMD_LDFLAGS = -l stdc++ -l pthread -L/usr/lib64/mysql/ -lmysqlclient
LBCMD_CPPFLAGS = -O2 -Werror

LBSRV_TARGET := bin/lb-server
LBSRV_SRCS = examples/lb-server/main.cpp
LBSRV_LDFLAGS = -l stdc++ -l pthread -l event
LBSRV_CPPFLAGS = -O2 -Werror

LBPL_TARGET := bin/lb-payload
LBPL_SRCS = examples/lb-payload/main.cpp
LBPL_LDFLAGS = -l stdc++ -l pthread 
LBPL_CPPFLAGS = -O2 -Werror

.DEFAULT: all
all: default lb_cmd
	@echo "All done."

.PHONY: default clean
default: clean
	gcc $(CPPFLAGS) $(LDFLAGS) $(SRCS) -o $(TARGET)

lb_cmd: lb_cmd_clean
	gcc $(LBCMD_CPPFLAGS) $(LBCMD_LDFLAGS) $(LBCMD_SRCS) -o $(LBCMD_TARGET)

lb_cmd_clean:
	rm -f $(LBCMD_TARGET)

lb_srv: lb_srv_clean
	gcc $(LBSRV_CPPFLAGS) $(LBSRV_LDFLAGS) $(LBSRV_SRCS) -o $(LBSRV_TARGET)

lb_srv_clean:
	rm -f $(LBSRV_TARGET)

lb_pl: lb_pl_clean
	gcc $(LBPL_CPPFLAGS) $(LBPL_LDFLAGS) $(LBPL_SRCS) -o $(LBPL_TARGET)

lb_pl_clean:
	rm -f $(LBPL_TARGET)

clean:
	rm -f $(TARGET) 
