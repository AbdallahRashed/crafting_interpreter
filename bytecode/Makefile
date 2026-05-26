CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -O2
LDFLAGS =

BUILD_DIR = build
TARGET = $(BUILD_DIR)/clox

SRCS = main.c chunk.c compiler.c debug.c memory.c object.c scanner.c table.c value.c vm.c
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)

.PHONY: all clean run debug

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: $(TARGET)
	$(TARGET)

debug: CFLAGS += -g -DDEBUG_PRINT_CODE -DDEBUG_TRACE_EXECUTION
debug: clean $(TARGET)

# Dependencies (header includes)
$(BUILD_DIR)/main.o: main.c common.h vm.h
$(BUILD_DIR)/chunk.o: chunk.c chunk.h memory.h vm.h
$(BUILD_DIR)/compiler.o: compiler.c common.h compiler.h scanner.h memory.h debug.h
$(BUILD_DIR)/debug.o: debug.c debug.h chunk.h value.h object.h
$(BUILD_DIR)/memory.o: memory.c memory.h vm.h compiler.h
$(BUILD_DIR)/object.o: object.c object.h memory.h table.h value.h vm.h
$(BUILD_DIR)/scanner.o: scanner.c common.h scanner.h
$(BUILD_DIR)/table.o: table.c table.h memory.h object.h value.h
$(BUILD_DIR)/value.o: value.c value.h object.h memory.h
$(BUILD_DIR)/vm.o: vm.c common.h compiler.h debug.h object.h memory.h vm.h
