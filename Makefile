CC = gcc
CFLAGS = -Wall -Wextra -g -pthread -MMD -MP
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
SRC = main.c \
      cache-list/cache-list.c \
      http-parser/http-parser.c \
      socket-library/socket-library.c \
      utils/custom-utilities.c

# Generate object file paths
OBJ_FILES = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC))

# Generate dependency file paths
DEP_FILES = $(OBJ_FILES:.o=.d)

# Target executable
TARGET = server

# Create build directories (if they don't exist)
$(shell mkdir -p $(OBJ_DIR) $(BIN_DIR))

# Rule to compile object files and generate dependency files
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)  # Ensure directory exists
	$(CC) $(CFLAGS) -c $< -o $@

# Link the final executable
$(TARGET): $(OBJ_FILES)
	@echo "Linking Target: $(Target)"
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET)

# Include dependency files if they exist
-include $(DEP_FILES)

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Default target
all: $(TARGET)
