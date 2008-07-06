CC = gcc
CFLAGS = -Os -Wall
LDFLAGS = -lbz2
SOFLAGS = -fpic -shared

# FIXME: Assumes you have GCC 4.4 installed for building on OS X
ifeq ($(shell uname),Darwin)
CC = gcc-mp-4.4
endif

ifeq ($(shell pkg-config --exists lua5.1; echo $$?),0)
PFLAGS = $(shell pkg-config --cflags --libs lua5.1)
else
PFLAGS = $(shell pkg-config --cflags --libs lua)
endif

bz2.so: lbz.c
	$(CC) $(SOFLAGS) $(PFLAGS) $(CFLAGS) $(LDFLAGS) lbz.c -o bz2.so

clean:
	-rm -f bz2.so

test: bz2.so
	lua test.lua
