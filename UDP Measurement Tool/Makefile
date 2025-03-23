CC = gcc
CFLAGS = -Wall -Wextra -g

all: udp_server udp_client

udp_server: udp_server.c include/common.h
	$(CC) $(CFLAGS) -o udp_server udp_server.c

udp_client: udp_client.c include/common.h
	$(CC) $(CFLAGS) -o udp_client udp_client.c

clean:
	rm -f udp_server udp_client *.o

.PHONY: all clean 