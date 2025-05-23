# Makefile for server project

# raspberryPie info
HOSTNAME := veda
TARGET_IP := 192.168.0.105

# Directories
SRC_DIR := ./srcs
INC_DIR := ./include
BUILD_DIR := build
LIB_DIR := lib
BIN := Server

# Tools
CROSS := aarch64-linux-gnu
CC := gcc
CFLAGS := -Wall -fPIC -I/usr/aarch64-linux-gnu/include -lwiringPi -lcrypt \
					-I$(INC_DIR) 
LDFLAGS := -shared

# Source file
LIB_NAMES := led seg pr buzz
LIB_SRCS := $(foreach name, $(LIB_NAMES), $(SRC_DIR)/$(name).c)

LIB_OBJS := $(foreach name, $(LIB_NAMES), $(BUILD_DIR)/lib_$(name).o)
LIBS_SO := $(foreach name, $(LIB_NAMES), $(LIB_DIR)/lib$(name).so)

SERVER_SRC := $(SRC_DIR)/server.c \
			$(SRC_DIR)/buzz_thread.c \
			$(SRC_DIR)/led_thread.c \
			$(SRC_DIR)/pr_thread.c \
			$(SRC_DIR)/seg_thread.c  

SERVER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SERVER_SRC))

# Default target
all: $(BIN)

# Executable build
$(BIN): $(SERVER_OBJS) $(LIBS_SO)
	$(CROSS)-$(CC) -o $@ $(SERVER_OBJS) -ldl -lwiringPi -lcrypt -lwiringPiDev

# Shared library build
$(LIB_DIR)/lib%.so: $(BUILD_DIR)/%.o | $(LIB_DIR)
	$(CROSS)-$(CC) $(LDFLAGS) -o $@ $<

# build/lib_led.o 같은 것 생성
$(BUILD_DIR)/lib_%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CROSS)-$(CC) -c -fPIC $< -o $@ -I/usr/aarch64-linux-gnu/include -I$(INC_DIR) -Wall

# build/lib_led.o → lib/libled.so
$(LIB_DIR)/lib%.so: $(BUILD_DIR)/lib_%.o | $(LIB_DIR)
	$(CROSS)-$(CC) $(LDFLAGS) -o $@ $<

# Object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CROSS)-$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BIN)
	rm ./$(LIB_DIR)/*
	rm ./$(BUILD_DIR)/*

# re-make 
re: clean all

# Send command
send: 
	scp $(BIN) $(LIBS_SO) $(HOSTNAME)@$(TARGET_IP):~/
	scp startServer.sh $(HOSTNAME)@$(TARGET_IP):~/

.PHONY: all clean send re