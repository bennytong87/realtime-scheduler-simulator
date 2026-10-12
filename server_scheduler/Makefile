CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = server_scheduler

all: $(TARGET)

$(TARGET): server_scheduler.c
	$(CC) $(CFLAGS) -o $(TARGET) server_scheduler.c -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean
