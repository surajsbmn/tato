CC = gcc
CPPFLAGS = -Iinclude
CFLAGS = -Wall -Wextra -std=c11 -O2 -D_POSIX_C_SOURCE=200809L
LDFLAGS = 
LDLIBS = -lpthread

TARGET = build/server
SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJ) | build
	$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS)

build/%.o: src/%.c | build
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

debug: CFLAGS += -g -O0
debug: clean all

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean