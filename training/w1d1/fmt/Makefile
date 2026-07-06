# W1D1 训练二：文本格式化 Makefile

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = fmt
OBJS    = fmt.o word.o line.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

fmt.o: fmt.c word.h line.h
	$(CC) $(CFLAGS) -c fmt.c

word.o: word.c word.h
	$(CC) $(CFLAGS) -c word.c

line.o: line.c line.h
	$(CC) $(CFLAGS) -c line.c

.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
