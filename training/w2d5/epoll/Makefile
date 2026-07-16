# W2D5 训练：EpollServer + TCP Client

CC      = gcc
CFLAGS  = -g -Wall

.PHONY: all clean

all: EpollServer client

EpollServer: server.c
	$(CC) $(CFLAGS) server.c -o EpollServer

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f EpollServer client
