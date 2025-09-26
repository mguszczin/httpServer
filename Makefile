# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iincludes -g

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDES = includes

# Source files (all .c files in src subfolders)
SRCS := $(wildcard $(SRC_DIR)/**/*.c) $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))

# Executable name
TARGET = server

# Create obj directories automatically
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Clean
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean