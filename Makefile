CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = server
SRC = server.c
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean