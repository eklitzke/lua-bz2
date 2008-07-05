#include <bzlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define BUFSIZE 4096

typedef struct {
	BZFILE *bz_stream;
	FILE *f;
} lbz_state;

int lbz_read_open(lua_State *L) {
	size_t len;
	const char *fname = lua_tolstring(L, 1, &len);
	FILE *f = fopen(fname, "rb");
	if (f == NULL) {
		fprintf(stderr, "failed to fopen file\n");
		lua_pushnil(L);
		return 1;
	}

	int bzerror;
	lbz_state *state = (lbz_state *) lua_newuserdata(L, sizeof(lbz_state));
	state->bz_stream = BZ2_bzReadOpen(&bzerror, f, 0, 0, NULL, 0);
	state->f = f;

	if (bzerror != BZ_OK) {
		fprintf(stderr, "encountered error %d in BZ2_bzReadOpen\n", bzerror);
		lua_pushnil(L);
	}
	return 1;
}

int lbz_read(lua_State *L) {
	int bzerror;

	int len;
	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);
	len = luaL_checkint(L, 2);

	luaL_Buffer b;
	luaL_buffinit(L, &b);

	char *buf = malloc(len);
	int ret = BZ2_bzRead(&bzerror, state->bz_stream, buf, len);

	if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
		fprintf(stderr, "uh oh, encountered code %d in BZ2_bzRead\n", bzerror);
		lua_pushnil(L);
		return 1;
	}

	luaL_addlstring(&b, buf, ret);
	luaL_pushresult(&b);
	return 1;
}

static int lbz_read_close(lua_State *L) {
	int bzerror;

	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);

	BZ2_bzReadClose(&bzerror, state->bz_stream);
	fclose(state->f);
	lua_pushnil(L);
	return 1;
}

static const struct luaL_reg bz2lib [] = {
	{"read_open", lbz_read_open},
	{"read", lbz_read},
	{"read_close", lbz_read_close},
	{NULL, NULL} /* Sentinel */
};

int luaopen_bz2(lua_State *L) {
	luaL_openlib(L, "bz2", bz2lib, 0);
	return 1;
}
