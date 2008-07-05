CC = gcc
ifeq ($(shell uname),Darwin)
CC = gcc-mp-4.4
endif

bz2.so: lbz.c
	$(CC) -lbz2 -I/opt/local/include -O2 -fpic -shared -o bz2.so lbz.c -undefined dynamic_lookup

clean:
	-rm -f bz2.so

test: bz2.so
	lua test.lua
