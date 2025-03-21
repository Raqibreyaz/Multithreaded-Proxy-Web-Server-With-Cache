# Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -g -MMD -MP

# Directories
OBJDIR = build
SRCDIR = .
UTILSDIR = utils

# Executable
TARGET = server

# Source & Object Files
SRCS = main.c utils/custom-utilities.c utils/http-parser.c utils/socket-library.c
OBJS = $(SRCS:%.c=$(OBJDIR)/%.o)
DEPS = $(OBJS:.o=.d)  # Dependency files for tracking changes

# Ensure build directory exists
$(shell mkdir -p $(OBJDIR))

# Build Target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Object File Rules
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	@mkdir -p $(dir $@)  # Create necessary subdirectories
	$(CC) $(CFLAGS) -c $< -o $@

# Ensure build directory exists before compiling
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Include dependency files
-include $(DEPS)

# Clean Build
clean:
	rm -rf $(TARGET) $(OBJDIR)
