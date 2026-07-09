# W1D4 训练一：简单 Shell

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = mini_shell

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

.PHONY: clean run
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
