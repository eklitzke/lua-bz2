/* This file implements the Lua binding to libbzip2.
 *
 * Copyright (c) 2008, Evan Klitzke <evan@eklitzke.org>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <bzlib.h>
#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define BUFSIZE 4096 /* how much data to read at a time */

#define LBZ_EOS    0x01 /* end of stream */
#define LBZ_CLOSED 0x02 /* the file is closed */

typedef struct {
	BZFILE *bz_stream;
	FILE *f;
	int flags;

	/* getline related stuff */
	char *getline_buf;
	char *read_buf;
	char *extra_buf;
	size_t getline_buf_size;
	size_t used;
} lbz_state;

/* Forward declarations */
static int lbz_read_open(lua_State *L);
static int lbz_read(lua_State *L);
static int lbz_read_close(lua_State *L);
static int lbz_getline(lua_State *L);
static int lbz_getline_read(lua_State *L, lbz_state *state, size_t offset);

static const struct luaL_reg bzlib_f [] = {
	{"open", lbz_read_open},
	{NULL, NULL} /* Sentinel */
};

static const struct luaL_reg bzlib_m [] = {
	{"read", lbz_read},
	{"getline", lbz_getline},
	{"close", lbz_read_close},
	{NULL, NULL} /* Sentinel */
};

/* Binding to libbzip2's BZ2_bzReadOpen method */
int lbz_read_open(lua_State *L) {
	size_t len;
	const char *fname = lua_tolstring(L, 1, &len);
	FILE *f = fopen(fname, "rb");

	if (f == NULL)
		return luaL_error(L, "Failed to fopen %s", fname);

	int bzerror;
	lbz_state *state = (lbz_state *) lua_newuserdata(L, sizeof(lbz_state));
	state->bz_stream = BZ2_bzReadOpen(&bzerror, f, 0, 0, NULL, 0);
	state->f = f;
	state->flags = 0;

	state->read_buf = malloc(BUFSIZE);
	state->extra_buf = state->read_buf;
	state->getline_buf_size = BUFSIZE;
	state->getline_buf = malloc(state->getline_buf_size);
	state->used = 0;

	luaL_getmetatable(L, "LuaBook.bz2");
	lua_setmetatable(L, -2);

	if (bzerror != BZ_OK)
		lua_pushnil(L);

	return 1;
}

/* Binding to libbzip2's BZ2_bzReadOpen method */
static int lbz_read(lua_State *L) {
	int bzerror = BZ_OK;
	int len;
	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);
	len = luaL_checkint(L, 2);

	if (state->flags & (LBZ_EOS | LBZ_CLOSED)) {
		/* The logical end of file has been reached -- there's no more data to
		 * return, and the user should call the read_close method. */
		lua_pushnil(L);
		return 1;
	}

	luaL_Buffer b;
	luaL_buffinit(L, &b);

	char *buf = alloca(len);
	int offset = 0;

	/* In case this function is being used alongsize the getline method, we
	 * should use the buffers that getline is using */
	if (state->used) {
		offset = (state->used < len) ? state->used : len;
		memcpy(buf, state->extra_buf, offset);
		state->extra_buf += offset;
		state->used -= offset;
	}

	int to_copy = len - offset;
	if (to_copy)
		offset += BZ2_bzRead(&bzerror, state->bz_stream, buf + offset, to_copy);

	if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
		lua_pushnil(L);
		return 1;
	}

	if (bzerror == BZ_STREAM_END)
		state->flags |= LBZ_EOS;

	luaL_addlstring(&b, buf, offset);
	luaL_pushresult(&b);
	return 1;
}

/* Binding to libbzip2's BZ2_bzReadClose method */
static int lbz_read_close(lua_State *L) {
	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);
	if (!(state->flags & LBZ_CLOSED)) {
		int bzerror;
		BZ2_bzReadClose(&bzerror, state->bz_stream);
		fclose(state->f);
		free(state->read_buf);
		free(state->getline_buf);
		state->flags |= LBZ_CLOSED;
	}
	lua_pushnil(L);
	return 1;
}

/*
 * GETLINE STUFF
 * This code is considerably more complicated... if you know of a simpler way
 * to do it that doesn't sacrifice speed, please let me know.
 */

/* Allocate space for the getline_buf if necessary. */
inline void realloc_double(lbz_state *state, size_t target) {
	if (state->getline_buf_size < target) {
		size_t newsize = (state->getline_buf_size) << 1;
		while (newsize < target)
			newsize <<= 1;
		state->getline_buf_size = newsize;
		state->getline_buf = realloc(state->getline_buf, newsize);
	}
}

/* This is an auxilliary function that lbz_getline calls when it needs to
 * actually use the BZ2_bzRead method to read more data from the bzipped file.
 **/
static int lbz_getline_read(lua_State *L, lbz_state *state, size_t offset) {
	int bzerror;
	int len = BZ2_bzRead(&bzerror, state->bz_stream, state->read_buf, BUFSIZE);

	if ((bzerror == BZ_OK) || (bzerror == BZ_STREAM_END)) {
		char *loc = memchr(state->read_buf, (int) '\n', len);

		/* If a newline hasn't been found, recursively call while building up
		 * the buffer */
		if (loc == NULL) {
			realloc_double(state, offset + len);
			memcpy(state->getline_buf + offset, state->read_buf, len);
			return lbz_getline_read(L, state, offset + len);
		}

		int distance = loc - state->read_buf + 1;
		realloc_double(state, offset + distance);
		memcpy(state->getline_buf + offset, state->read_buf, distance);
		state->getline_buf[offset+distance] = '\0';

		state->extra_buf = state->read_buf + distance;
		state->used = len - distance;

		/* Copy the data into a Lua buffer and return it */
		luaL_Buffer b;
		luaL_buffinit(L, &b);
		luaL_addlstring(&b, state->getline_buf, offset + distance);
		luaL_pushresult(&b);

		if (bzerror == BZ_STREAM_END)
			state->flags |= LBZ_EOS;
		return 1;
	}

	/* should not happen */
	lua_pushnil(L);
	return 1;
}

static int lbz_getline(lua_State *L) {
	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);

	if (state->flags & LBZ_CLOSED) {
		lua_pushnil(L);
		return 1;
	}

	if (state->used) {
		char *loc = memchr(state->extra_buf, (int) '\n', state->used);

		/* If we read extra data on the last pass and a newline character isn't
		 * in that extra data, copy the extra data into the line buffer and
		 * then call lbz_getline_read to read more data from the bz2 file until
		 * the newline is found. */
		if (loc == NULL) {
			realloc_double(state, state->used);
			memcpy(state->getline_buf, state->extra_buf, state->used);
			if (state->flags & LBZ_EOS) {
				luaL_Buffer b;
				luaL_buffinit(L, &b);
				luaL_addlstring(&b, state->getline_buf, state->used);
				luaL_pushresult(&b);
				return 1;
			}
			return lbz_getline_read(L, state, state->used);
		}

		/* This branch is executed if we had extra data on the last pass and a
		 * newline could be found in that extra data. This is pretty simple --
		 * just copy the appropriate data into the buffer and consume that data
		 * in the extra buffer. */
		else {
			int distance = loc - state->extra_buf + 1;
			realloc_double(state, distance);

			int move_amt = state->used - distance;
			memcpy(state->getline_buf, state->extra_buf, distance);
			state->getline_buf[distance] = '\0';

			state->extra_buf = loc + 1;
			state->used = move_amt;

			/* Copy the data into a Lua buffer and return it */
			luaL_Buffer b;
			luaL_buffinit(L, &b);
			luaL_addlstring(&b, state->getline_buf, distance);
			luaL_pushresult(&b);
			return 1;
		}
	}

	/* If there was no extra data from the last pass then we need to call
	 * lbz_getline_read directly to get more data and find the newline. */
	return lbz_getline_read(L, state, 0);
}

static int lbz_gc(lua_State *L) {
	lbz_read_close(L);
	return 0;
}

int luaopen_bz2(lua_State *L) {
	luaL_newmetatable(L, "LuaBook.bz2");
	int mt = lua_gettop(L); /* position of the metatable on the stack */
	lua_pushstring(L, "__index");
	lua_pushvalue(L, mt); /* push the metatable */
	lua_settable(L, mt); /* metatable.__index = metatable */

	lua_pushstring(L, "__gc");
	lua_pushcfunction(L, lbz_gc);
	lua_settable(L, mt);

	luaL_openlib(L, NULL, bzlib_m, 0);
	luaL_openlib(L, "bz2", bzlib_f, 0);
	return 1;
}
