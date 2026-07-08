# W1D3 训练二：BST 删除/修改/保存

CC      = gcc
CFLAGS  = -g -Wall
TARGET  = bst_train2

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

.PHONY: clean run
run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) contacts_result.csv
