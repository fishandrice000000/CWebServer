# W1D2 训练一：CSV + 链表 CRUD

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = user_mgr

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

.PHONY: clean run
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
