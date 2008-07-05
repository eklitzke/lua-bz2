CC = gcc
CFLAGS = -Os -g -Wall
LDFLAGS = -lbz2 -llua
SOFLAGS = -fpic -shared

ifeq ($(shell uname),Darwin)
CC = gcc-mp-4.4
CPPFLAGS = -I/opt/local/include
LDFLAGS += -L/opt/local/lib
endif


bz2.so: lbz.c
	$(CC) $(CFLAGS) $(SOFLAGS) $(CPPFLAGS) $(LDFLAGS) lbz.c -o bz2.so

clean:
	-rm -f bz2.so

test: bz2.so
	lua test.lua
