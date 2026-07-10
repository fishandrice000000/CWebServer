# W1D5 训练：哲学家进餐

CC      = gcc
CFLAGS  = -g -Wall
LDFLAGS = -lpthread
TARGET  = philosopher

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET) $(LDFLAGS)

.PHONY: clean run run-lock run-trylock

# 阻塞模式 (可能导致死锁)
run-lock: $(TARGET)
	./$(TARGET) 5 lock

# 非阻塞模式 (trylock 预防死锁)
run-trylock: $(TARGET)
	./$(TARGET) 5 trylock

clean:
	rm -f $(TARGET)
