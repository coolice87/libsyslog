CC = gcc
CFLAGS = -Wall -Wextra -Werror -O2 -fPIC
DEBUG = -g -DDEBUG
LDFLAGS = -lpthread

SRC_DIR = src
INCLUDE_DIR = include
LIB_DIR = lib
BIN_DIR = bin
EXAMPLE_DIR = examples
OBJ_DIR = build

LIB_SOURCES = $(SRC_DIR)/libsyslog.c $(SRC_DIR)/buffer.c $(SRC_DIR)/file_manager.c
LIB_OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LIB_SOURCES))

STATIC_LIB = $(LIB_DIR)/libsyslog.a
SHARED_LIB = $(LIB_DIR)/libsyslog.so

SIMPLE_EXAMPLE = $(BIN_DIR)/simple_example
MULTI_EXAMPLE = $(BIN_DIR)/multi_thread_example

.PHONY: all static shared examples clean run-simple run-multithread logs help

all: static shared examples

help:
	@echo "Available targets:"
	@echo "  make static              - Build static library (.a)"
	@echo "  make shared              - Build shared library (.so)"
	@echo "  make examples            - Build example programs"
	@echo "  make all                 - Build all (default)"
	@echo "  make run-simple          - Run simple example"
	@echo "  make run-multithread     - Run multi-thread example"
	@echo "  make clean               - Clean build files"
	@echo "  make logs                - Show generated log files"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

static: $(LIB_DIR) $(STATIC_LIB)

$(STATIC_LIB): $(LIB_OBJECTS)
	@echo "Linking static library: $@"
	ar rcs $@ $^

shared: $(LIB_DIR) $(SHARED_LIB)

$(SHARED_LIB): $(LIB_OBJECTS)
	@echo "Linking shared library: $@"
	$(CC) -shared -o $@ $^ $(LDFLAGS)

$(BIN_DIR)/%: $(EXAMPLE_DIR)/%.c static | $(BIN_DIR)
	@echo "Compiling example: $@"
	$(CC) $(CFLAGS) -I$(INCLUDE_DIR) -o $@ $< -L$(LIB_DIR) -lsyslog $(LDFLAGS)

examples: $(SIMPLE_EXAMPLE) $(MULTI_EXAMPLE)

run-simple: $(SIMPLE_EXAMPLE)
	@echo "Running simple example..."
	LD_LIBRARY_PATH=$(LIB_DIR) $(SIMPLE_EXAMPLE)

run-multithread: $(MULTI_EXAMPLE)
	@echo "Running multi-thread example..."
	LD_LIBRARY_PATH=$(LIB_DIR) $(MULTI_EXAMPLE)

logs:
	@echo "Log files:"
	@ls -lh logs/ 2>/dev/null || echo "No logs directory"

$(OBJ_DIR) $(LIB_DIR) $(BIN_DIR):
	mkdir -p $@

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)
	@rm -rf logs/*.log 2>/dev/null
	@echo "Clean complete"
