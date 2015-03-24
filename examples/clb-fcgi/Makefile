
TARGET    := $(BIN_DIR)/clb-fcgi
WORK_DIR  := $(BUILD_DIR)/clb-fcgi

SRCS  = $(TOP_DIR)/examples/clb-fcgi/main.cpp
SRCS += $(TOP_DIR)/examples/clb-fcgi/cfg_db.cpp
SRCS += $(TOP_DIR)/examples/clb-fcgi/cmd_srv.cpp
SRCS += $(TOP_DIR)/src/qao/qao_base.cpp
SRCS += $(TOP_DIR)/src/qao/cctl_req.cpp
SRCS += $(TOP_DIR)/src/qao/cctl_rep.cpp
SRCS += $(TOP_DIR)/src/qao/cdat_wx.cpp
SRCS += $(TOP_DIR)/src/qao/answer.cpp
SRCS += $(TOP_DIR)/src/utils/hash_alg.cpp
SRCS += $(TOP_DIR)/src/utils/logger.cpp
SRCS += $(TOP_DIR)/src/utils/lb_db.cpp
SRCS += $(TOP_DIR)/src/stat/clb_stat.cpp
SRCS += $(TOP_DIR)/src/stat/stat_man.cpp
SRCS += $(TOP_DIR)/src/clb/clb_tbl.cpp
SRCS += $(TOP_DIR)/src/clb/clb_grp.cpp
SRCS += $(TOP_DIR)/src/clb/clb_clnt.cpp
SRCS += $(TOP_DIR)/src/evsock.cpp
SRCS += $(TOP_DIR)/src/rcu_man.cpp

CPPFLAGS = -O2 -Wall -Werror -I$(TOP_DIR)/include

LDFLAGS = -lstdc++ -lpthread -lfcgi 
LDFLAGS += -L/usr/lib64/mysql/ -lmysqlclient
LDFLAGS += -lcrypto -levent -ljsoncpp -lexpat

OBJS := $(patsubst $(TOP_DIR)/%.cpp,$(WORK_DIR)/%.o,$(SRCS))
DEPS := $(patsubst $(TOP_DIR)/%.cpp,$(WORK_DIR)/%.d,$(SRCS))


.PHONY: all prepare clean $(TARGET) 
all: $(DEPS) $(TARGET)

$(TARGET): $(OBJS)
	@echo "    Linking $(notdir $@)"
	@$(CC) $(OBJS) $(LDFLAGS) -o $@

$(WORK_DIR)/%.d:$(TOP_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@set -e; \
	rm -f $@; \
	$(CC) $(CPPFLAGS) -M $< > $@.$$$$; \
	sed 's,$(notdir $*)\.o[ :]*,$(WORK_DIR)/$*\.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(WORK_DIR)/%.o:$(TOP_DIR)/%.cpp
	@echo "    Compiling $(notdir $(TARGET)) $*"
	@$(CC) $(CPPFLAGS) -c $< -o $@
	
-include $(DEPS)

prepare:
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(dir $(OBJS))
	
clean:
	@rm -fr $(TARGET) $(BUILD_DIR)


