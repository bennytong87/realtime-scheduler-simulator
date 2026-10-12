CC = gcc
CFLAGS = -Wall -Wextra -g

all: periodic_scheduler server_scheduler

periodic_scheduler:
	$(MAKE) -C periodic_scheduler

server_scheduler:
	$(MAKE) -C server_scheduler

clean:
	$(MAKE) -C periodic_scheduler clean
	$(MAKE) -C server_scheduler clean

.PHONY: all periodic_scheduler server_scheduler clean
