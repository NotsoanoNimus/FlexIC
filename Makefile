ARCH		:= x86_64
CC			:= clang
CFLAGS		:= -O2

VEHICLE		:= 2005_chevrolet_colorado
DBC_FILE	:= dbc/$(VEHICLE).dbc

BUILD_DIR	= build
GEN_DIR		= $(BUILD_DIR)/gen
VEHICLE_H	= $(GEN_DIR)/vehicle.h
VEHICLE_C	= $(GEN_DIR)/vehicle.c

TARGET		:= $(BUILD_DIR)/flexic

SRC_DIR		= src
INC_DIR		= $(SRC_DIR)/include

WIDGETS_DIR	= $(SRC_DIR)/widgets
WIDGETS		:= #needle_meter,stepped_bar
WIDGET_SRCS	= $(shell for x in $$(echo "$(WIDGETS)" | tr ',' '\n'); do echo -n "$(WIDGETS_DIR)/$${x}.c "; done)

RENDERER		:= raylib
RENDERER_SRC	= $(SRC_DIR)/renderers/$(RENDERER).c

IC_DEBUG	:= 0


all: $(BUILD_DIR) $(GEN_DIR) $(WIDGETS_DIR) $(VEHICLE_H) $(VEHICLE_C) $(RENDERER_SRC)
	$(CC) $(CFLAGS) -DIC_DEBUG=$(IC_DEBUG) -I$(INC_DIR) -I$(GEN_DIR) -o $(TARGET) \
		$(VEHICLE_C) $(RENDERER_SRC) $(WIDGET_SRCS) $(wildcard $(SRC_DIR)/*.c) \
		-lpthread -lraylib -lGL -ldl -lm

debug: IC_DEBUG=1
debug: all

$(BUILD_DIR):
	-@mkdir -p $(BUILD_DIR) &>/dev/null

$(GEN_DIR):
	-@mkdir -p $(GEN_DIR) &>/dev/null

$(WIDGETS_DIR):
	-@mkdir -p $(WIDGETS_DIR) &>/dev/null

$(VEHICLE_H):
$(VEHICLE_C):
	$(error "You need to compile a DBC file for your specific vehicle. Please refer to the FlexIC documentation.")