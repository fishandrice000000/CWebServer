# CWebServer V1.0 Makefile

CC      = gcc
CFLAGS  = -g -Wall -Iinclude
LDFLAGS = -lpthread
TARGET  = mini_web_server
SRCDIR  = src
BUILDDIR = build

OBJS    = $(BUILDDIR)/main.o \
          $(BUILDDIR)/config.o \
          $(BUILDDIR)/log.o \
          $(BUILDDIR)/http_response.o \
          $(BUILDDIR)/user_store.o \
          $(BUILDDIR)/user_index.o \
          $(BUILDDIR)/request_handler.o \
          $(BUILDDIR)/process_server.o \
          $(BUILDDIR)/thread_server.o \
          $(BUILDDIR)/tcp_server.o \
          $(BUILDDIR)/tcp_fork_server.o \
          $(BUILDDIR)/thread_pool.o \
          $(BUILDDIR)/tcp_pool_server.o \
          $(BUILDDIR)/epoll_server.o \
          $(BUILDDIR)/cJSON.o

$(shell mkdir -p $(BUILDDIR))

$(BUILDDIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILDDIR)/main.o: $(SRCDIR)/main.c include/config.h include/http_response.h include/log.h include/user_store.h include/user_index.h include/process_server.h include/thread_server.h include/tcp_server.h include/tcp_fork_server.h include/tcp_pool_server.h include/epoll_server.h include/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/config.o: $(SRCDIR)/config.c include/config.h include/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/log.o: $(SRCDIR)/log.c include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/http_response.o: $(SRCDIR)/http_response.c include/http_response.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user_store.o: $(SRCDIR)/user_store.c include/user_store.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user_index.o: $(SRCDIR)/user_index.c include/user_index.h include/user_store.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/request_handler.o: $(SRCDIR)/request_handler.c include/request_handler.h include/user_store.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/process_server.o: $(SRCDIR)/process_server.c include/process_server.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/thread_server.o: $(SRCDIR)/thread_server.c include/thread_server.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/tcp_server.o: $(SRCDIR)/tcp_server.c include/tcp_server.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/tcp_fork_server.o: $(SRCDIR)/tcp_fork_server.c include/tcp_fork_server.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/thread_pool.o: $(SRCDIR)/thread_pool.c include/thread_pool.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/tcp_pool_server.o: $(SRCDIR)/tcp_pool_server.c include/tcp_pool_server.h include/thread_pool.h include/request_handler.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/epoll_server.o: $(SRCDIR)/epoll_server.c include/epoll_server.h include/request_handler.h include/log.h include/config.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/cJSON.o: $(SRCDIR)/cJSON.c include/cJSON.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run test01 test02 test03 test04 test05 test06 test07 test08 test10 clean test-clean

run: $(BUILDDIR)/$(TARGET)
	./$(BUILDDIR)/$(TARGET) config/server.conf

test01: $(BUILDDIR)/$(TARGET)
	bash test/test_day01.sh

test02: $(BUILDDIR)/$(TARGET)
	bash test/test_day02.sh

test03: $(BUILDDIR)/$(TARGET)
	bash test/test_day03.sh

test04: $(BUILDDIR)/$(TARGET)
	bash test/test_day04.sh

test05: $(BUILDDIR)/$(TARGET)
	bash test/test_day05.sh

test06: $(BUILDDIR)/$(TARGET)
	bash test/test_day06.sh

test07: $(BUILDDIR)/$(TARGET)
	bash test/test_day07.sh

test08: $(BUILDDIR)/$(TARGET)
	bash test/test_day08.sh

test10: $(BUILDDIR)/$(TARGET)
	bash test/test_day10.sh

clean:
	rm -rf $(BUILDDIR)
	rm -f day01_output.txt bad_log_output.txt
	rm -f logs/server.log
	rm -f outputs/*.out

test-clean:
	rm -f day01_output.txt bad_log_output.txt
	rm -f config/bad_log.conf
