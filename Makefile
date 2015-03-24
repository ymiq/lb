
MAKEFLAGS         	   += --no-print-directory

export CC=gcc
export TOP_DIR=$(PWD)
export BIN_DIR=$(TOP_DIR)/bin
export BUILD_DIR=$(TOP_DIR)/build

.DEFAULT: all
all: clb_fcgi clb_srv clb_cmd clb_pl hub robot slb
	@echo "    All done."

clean:
	@rm -fr $(BIN_DIR) $(BUILD_DIR)

# #######################################################################################
.PHONY: clb_fcgi clb_fcgi_clean 
clb_fcgi: 
	@$(MAKE) -C $(TOP_DIR)/examples/clb-fcgi prepare
	@$(MAKE) -C $(TOP_DIR)/examples/clb-fcgi 

clb_fcgi_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-fcgi clean


# #######################################################################################
.PHONY: clb_srv clb_srv_clean 
clb_srv:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-srv prepare
	@$(MAKE) -C $(TOP_DIR)/examples/clb-srv 

clb_srv_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-srv clean
	

# #######################################################################################
.PHONY: lb_cmd lb_cmd_clean
clb_cmd: 
	@$(MAKE) -C $(TOP_DIR)/examples/clb-cmd prepare
	@$(MAKE) -C $(TOP_DIR)/examples/clb-cmd 
	
clb_cmd_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-cmd clean


# #######################################################################################
.PHONY: clb_pl clb_pl_clean
clb_pl:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-payload prepare
	@$(MAKE) -C $(TOP_DIR)/examples/clb-payload 

clb_pl_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/clb-payload clean

# #######################################################################################
.PHONY: hub hub_clean
hub:
	@$(MAKE) -C $(TOP_DIR)/examples/hub prepare
	@$(MAKE) -C $(TOP_DIR)/examples/hub
	
hub_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/hub clean

# #######################################################################################
.PHONY: robot robot_clean
robot:
	@$(MAKE) -C $(TOP_DIR)/examples/robot prepare
	@$(MAKE) -C $(TOP_DIR)/examples/robot 
	
robot_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/robot clean

# #######################################################################################
.PHONY: slb slb_clean
slb: 
	@$(MAKE) -C $(TOP_DIR)/examples/slb prepare
	@$(MAKE) -C $(TOP_DIR)/examples/slb

slb_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/slb clean

# #######################################################################################
.PHONY: test test_clean
test: 
	@$(MAKE) -C $(TOP_DIR)/examples/test prepare
	@$(MAKE) -C $(TOP_DIR)/examples/test

	
test_clean:
	@$(MAKE) -C $(TOP_DIR)/examples/test clean
