CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
TARGET = server
SRC_DIR = src
INC_DIR = include
OBJ_DIR = exec
SRC = $(wildcard $(SRC_DIR)/*.c)

# Create object directory if it doesn't exist
$(shell mkdir -p $(OBJ_DIR))

# Define object files in the 'exec' directory
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Convert 'src/main.c' -> 'exec/main.o'
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build the final executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Clean compiled files
clean:
	rm -f $(TARGET) $(OBJ_DIR)/*.o