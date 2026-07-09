# W1D4 训练二：生产者-消费者

CC      = gcc
CFLAGS  = -g -Wall

.PHONY: all clean ipcclean

all: producer consumer

producer: producer.c ipc.c shared.h
	$(CC) $(CFLAGS) producer.c ipc.c -o producer

consumer: consumer.c ipc.c shared.h
	$(CC) $(CFLAGS) consumer.c ipc.c -o consumer

# 多进程同时运行 (单终端, 后台+wait)
run: all
	@echo "=== Producer-Consumer Demo ==="
	./producer 1 & ./producer 2 & ./consumer 1 & ./consumer 2 & wait

clean:
	rm -f producer consumer
	rm -f consumer1.txt consumer2.txt

# 清理残留 IPC (如果进程异常退出后未清理)
ipcclean:
	ipcrm -S 0x1a0b 2>/dev/null; true
	ipcrm -M 0x1a0a 2>/dev/null; true
