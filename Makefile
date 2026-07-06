# CWebServer V0.1 Makefile

CC      = gcc
CFLAGS  = -g -Wall -Iinclude
TARGET  = mini_web_server
SRCDIR  = src
BUILDDIR = build

OBJS    = $(BUILDDIR)/main.o \
          $(BUILDDIR)/config.o \
          $(BUILDDIR)/log.o \
          $(BUILDDIR)/http_response.o

$(shell mkdir -p $(BUILDDIR))

$(BUILDDIR)/$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@

$(BUILDDIR)/main.o: $(SRCDIR)/main.c include/config.h include/http_response.h include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/config.o: $(SRCDIR)/config.c include/config.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/log.o: $(SRCDIR)/log.c include/log.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/http_response.o: $(SRCDIR)/http_response.c include/http_response.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: run test clean

run: $(BUILDDIR)/$(TARGET)
	./$(BUILDDIR)/$(TARGET) config/server.conf

test:
	bash test/test_day01.sh

clean:
	rm -rf $(BUILDDIR)
	rm -f day01_output.txt bad_log_output.txt
	rm -f logs/server.log
