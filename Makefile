ARCH		:= x86_64
CC			:= clang
CFLAGS		:= -O2

VEHICLE		:= 2005_chevrolet_colorado
DBC_FILE	:= dbc/$(VEHICLE).dbc

BUILD_DIR	= build
GEN_DIR		= $(BUILD_DIR)/gen
GEN_PROJ_DIR= ./tools/fast_dbc_to_c
VEHICLE_H	= $(GEN_DIR)/vehicle.h
VEHICLE_C	= $(GEN_DIR)/vehicle.c
CONFIG_C	= $(GEN_DIR)/config.c

TARGET		:= $(BUILD_DIR)/flexic

SRC_DIR		= src
INC_DIR		= $(SRC_DIR)/include

WIDGETS_DIR	= $(SRC_DIR)/widgets
WIDGETS		:= needle_meter,stepped_bar
WIDGET_SRCS	= $(shell for x in $$(echo "$(WIDGETS)" | tr ',' '\n'); do echo -n "$(WIDGETS_DIR)/$${x}/$${x}.c "; done)
WIDGET_FACTORIES	= $(shell i=0; for x in $$(echo "$(WIDGETS)" | tr ',' '\n'); do echo -n "-DWIDGET_FACTORY_$${i}=$${x}_create "; i=$$((i+1)); done)

RENDERER		:= raylib
RENDERER_SRC	= $(SRC_DIR)/renderers/$(RENDERER).c
RENDERER_LIBS	:= -lraylib -lGL -ldl -lm

IC_DEBUG		:= 0

.PHONY: all debug dbc


all: $(BUILD_DIR) $(GEN_DIR) $(WIDGETS_DIR) $(VEHICLE_H) $(VEHICLE_C) $(CONFIG_C) $(RENDERER_SRC)
	$(CC) $(CFLAGS) \
		-DIC_WIDGETS=\"$(WIDGETS)\" -DIC_DEBUG=$(IC_DEBUG) $(WIDGET_FACTORIES) \
		-I$(INC_DIR) -I$(GEN_DIR) -o $(TARGET) \
		$(VEHICLE_C) $(CONFIG_C) $(RENDERER_SRC) $(WIDGET_SRCS) $(wildcard $(SRC_DIR)/*.c) \
		-lpthread $(RENDERER_LIBS)


debug: IC_DEBUG=1
debug: all


clean:
	-@rm -rf $(BUILD_DIR)


dbc:
ifeq ($(DBC),)
	$(error You need to specify a full path to a DBC file; e.g. "make all DBC=`pwd`/dbc/my_car.dbc")
endif
	$(shell ( T_PWD=$(shell pwd) && >&2 cd $(GEN_PROJ_DIR) && >&2 cargo run "$(DBC)" "$${T_PWD}/build/gen" yes ) \
		|| { echo >&2 ERROR: Failed to run cargo generation. && kill $$PPID; })


config: $(WIDGET_CONFIG)
ifeq ($(WIDGET_CONFIG),)
	$(error A widget configuration has not been generated. You must specify a path to your widget configuration; e.g. "make all WIDGET_CONFIG=/etc/widgets.ic_config")
endif
	$(shell ( >&2 ./tools/config_src_gen.py "$(WIDGET_CONFIG)" "`pwd`/$(CONFIG_C)" ) \
		|| { echo >&2 ERROR: Failed to run config source generator. && kill $$PPID; })


$(BUILD_DIR):
	-@mkdir -p $(BUILD_DIR) &>/dev/null

$(GEN_DIR):
	-@mkdir -p $(GEN_DIR) &>/dev/null

$(WIDGETS_DIR):
	-@mkdir -p $(WIDGETS_DIR) &>/dev/null

$(VEHICLE_H):
$(VEHICLE_C):
	$(info You need to use the source generator with a DBC file first. Trying that...)
	@$(MAKE) dbc


$(CONFIG_C):
	$(info You need to pre-compile a widgets configuration. Trying that...)
	@$(MAKE) config WIDGETS_CONFIG=$(WIDGETS_CONFIG)
