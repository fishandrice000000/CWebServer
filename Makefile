# CWebServer V0.1 Makefile

CC      = gcc
CFLAGS  = -g -Wall -Iinclude
TARGET  = mini_web_server
SRCDIR  = src
OBJS    = $(SRCDIR)/main.o \
          $(SRCDIR)/config.o \
          $(SRCDIR)/log.o \
          $(SRCDIR)/http_response.o

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

$(SRCDIR)/main.o: $(SRCDIR)/main.c include/config.h include/http_response.h include/log.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/main.c -o $(SRCDIR)/main.o

$(SRCDIR)/config.o: $(SRCDIR)/config.c include/config.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/config.c -o $(SRCDIR)/config.o

$(SRCDIR)/log.o: $(SRCDIR)/log.c include/log.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/log.c -o $(SRCDIR)/log.o

$(SRCDIR)/http_response.o: $(SRCDIR)/http_response.c include/http_response.h
	$(CC) $(CFLAGS) -c $(SRCDIR)/http_response.c -o $(SRCDIR)/http_response.o

.PHONY: run test clean

run: $(TARGET)
	./$(TARGET) config/server.conf

test:
	bash test/test_day01.sh

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f day01_output.txt bad_log_output.txt
	rm -f logs/server.log
