CC = gcc
CFLAGS = -Wall -Wextra -g -Iinclude -pthread -MMD -MP
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
SRC = main.c \
	  src/server.c \
	  src/fetch.c \
	  src/cache.c \
	  src/utils.c \
	  src/thread-pool.c \
	  src/client-queue.c \
	  src/cache-store.c \
	  src/blocked-sites.c \
	  src/http-parser.c \
	  src/html-rewriter.c \
	  src/socket-utils.c \
	  src/client-handler.c \
	  src/http-request-response.c

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
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(TARGET) -lssl -lcrypto -lm

# Include dependency files if they exist
-include $(DEP_FILES)

# Clean build files
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

# Default target
all: $(TARGET)
