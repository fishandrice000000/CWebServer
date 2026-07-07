# W1D2 训练二：有序链表

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = ordered_list

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

.PHONY: clean run
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
