# Makefile for client project

TARGET := Client

# Directories
SRC_DIR := ./srcs
INC_DIR := ./include
BUILD_DIR := ./build
BIN := ../Client

# Tools
CC := gcc
CFLAGS := -Wall -I$(INC_DIR)

# Sources and objects
SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

# Default target
all: $(BIN)

# Executable build
$(BIN): $(OBJS) | $(BUILD_DIR)
	$(CC) -o $@ $^

# Object file compilation
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -rf $(BUILD_DIR)/*.o $(BIN)

# Rebuild
re: clean all

.PHONY: all clean re
