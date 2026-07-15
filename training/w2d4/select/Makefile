# W2D4 训练：SelectServer + TCP Client

CC      = gcc
CFLAGS  = -g -Wall

.PHONY: all clean

all: SelectServer client

SelectServer: server.c
	$(CC) $(CFLAGS) server.c -o SelectServer

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f SelectServer client
