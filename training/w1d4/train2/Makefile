# W1D4 训练二：生产者-消费者

CC      = gcc
CFLAGS  = -g -Wall

.PHONY: all clean run

all: producer consumer

producer: producer.c ipc.c shared.h
	$(CC) $(CFLAGS) producer.c ipc.c -o producer

consumer: consumer.c ipc.c shared.h
	$(CC) $(CFLAGS) consumer.c ipc.c -o consumer

run:
	@echo "Run in separate terminals:"
	@echo "  ./producer 1 & ./producer 2 & ./consumer 1 & ./consumer 2"

clean:
	rm -f producer consumer
	rm -f consumer1.txt consumer2.txt
