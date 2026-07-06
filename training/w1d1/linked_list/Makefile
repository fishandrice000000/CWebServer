# W1D1 训练一：单向链表 Makefile

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = linked_list
OBJS    = main.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
