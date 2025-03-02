CC = gcc
CFLAGS = -Wall -Wextra -Werror -Iinclude
TARGET = server
SRC_DIR = src
INC_DIR = include
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:.c=.o)

# Convert 'src/main.c' -> 'src/main.o' correctly
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(INC_DIR)/*.h
	$(CC) $(CFLAGS) -c $< -o $@

# Build the final executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Clean compiled files
clean:
	rm -f $(TARGET) $(OBJ)
