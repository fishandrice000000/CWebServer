# W2D2 训练：多客户端 TCP 文本聊天

CC      = gcc
CFLAGS  = -g -Wall

.PHONY: all clean

all: server client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

clean:
	rm -f server client
