# CWebServer V0.2 Makefile

CC      = gcc
CFLAGS  = -g -Wall -Iinclude
TARGET  = mini_web_server
SRCDIR  = src
BUILDDIR = build

OBJS    = $(BUILDDIR)/main.o \
          $(BUILDDIR)/config.o \
          $(BUILDDIR)/log.o \
          $(BUILDDIR)/http_response.o \
          $(BUILDDIR)/user_store.o

$(shell mkdir -p $(BUILDDIR))

$(BUILDDIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(BUILDDIR)/main.o: $(SRCDIR)/main.c include/config.h include/http_response.h include/log.h include/user_store.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/config.o: $(SRCDIR)/config.c include/config.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/log.o: $(SRCDIR)/log.c include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/http_response.o: $(SRCDIR)/http_response.c include/http_response.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/user_store.o: $(SRCDIR)/user_store.c include/user_store.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run test test02 clean test-clean

run: $(BUILDDIR)/$(TARGET)
	./$(BUILDDIR)/$(TARGET) config/server.conf

test01: $(BUILDDIR)/$(TARGET)
	bash test/test_day01.sh

test02: $(BUILDDIR)/$(TARGET)
	bash test/test_day02.sh

clean:
	rm -rf $(BUILDDIR)
	rm -f day01_output.txt bad_log_output.txt
	rm -f logs/server.log

test-clean:
	rm -f day01_output.txt bad_log_output.txt
	rm -f config/bad_log.conf
