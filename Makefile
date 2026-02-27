CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
LDFLAGS = -lpthread

TARGET = server
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all clean