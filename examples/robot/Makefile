
TARGET    := $(BIN_DIR)/robot
WORK_DIR  := $(BUILD_DIR)/robot


SRCS = $(TOP_DIR)/examples/robot/main.cpp
SRCS += $(TOP_DIR)/examples/robot/robot_hsrv.cpp
SRCS += $(TOP_DIR)/src/qao/qao_base.cpp
SRCS += $(TOP_DIR)/src/qao/question.cpp
SRCS += $(TOP_DIR)/src/qao/candidate.cpp
SRCS += $(TOP_DIR)/src/qao/answer.cpp
SRCS += $(TOP_DIR)/src/utils/logger.cpp
SRCS += $(TOP_DIR)/src/evsock.cpp
SRCS += $(TOP_DIR)/src/rcu_man.cpp

CPPFLAGS = -O2 -Wall -Werror -I$(TOP_DIR)/include

LDFLAGS = -lstdc++ -lpthread -levent -ljsoncpp

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
	@rm -fr $(TARGET) $(WORK_DIR)


