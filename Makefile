CC = gcc
CFLAGS = -Os -g -Wall
LDFLAGS = -lbz2
SOFLAGS = -fpic -shared

ifeq ($(shell uname),Darwin)
CC = gcc-mp-4.4
CFLAGS += -undefined dynamic_lookup
endif


bz2.so: lbz.c
	$(CC) $(CFLAGS) $(SOFLAGS) $(LDFLAGS) -I/opt/local/include lbz.c -o bz2.so

clean:
	-rm -f bz2.so

test: bz2.so
	lua test.lua
