CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = periodic_scheduler

all: $(TARGET)

$(TARGET): periodic_scheduler.c
	$(CC) $(CFLAGS) -o $(TARGET) periodic_scheduler.c -lm

clean:
	rm -f $(TARGET)

.PHONY: all clean
