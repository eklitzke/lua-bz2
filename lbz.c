#include <bzlib.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define BUFSIZE 4096

int lbz_read_open(lua_State *L) {
	size_t len;
	const char *fname = lua_tolstring(L, 1, &len);
	FILE *f = fopen(fname, "rb");
	if (f == NULL) {
		fprintf(stderr, "fuck me\n");
		lua_pushnil(L);
		return 1;
	}


	int bzerror;
	BZFILE **b = (BZFILE *) lua_newuserdata(L, sizeof(BZFILE));
	BZFILE *t = BZ2_bzReadOpen(&bzerror, f, 0, 0, NULL, 0);

	*b = t;

	if (bzerror != BZ_OK) {
		fprintf(stderr, "encountered error %d in BZ2_bzReadOpen\n", bzerror);
		lua_pushnil(L);
	}
	return 1;
}

int lbz_read(lua_State *L) {
	int bzerror;

	int len;
	BZFILE **bzf = (BZFILE **) lua_touserdata(L, 1);
	len = luaL_checkint(L, 2);

	luaL_Buffer b;
	luaL_buffinit(L, &b);

	char *buf = malloc(len);
	int ret = BZ2_bzRead(&bzerror, *bzf, buf, len);

	if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
		fprintf(stderr, "uh oh, encountered code %d in BZ2_bzRead\n", bzerror);
		lua_pushnil(L);
	}

	luaL_addlstring(&b, buf, ret);
	luaL_pushresult(&b);
	return 1;
}

static const struct luaL_reg bz2lib [] = {
	{"read_open", lbz_read_open},
	{"read", lbz_read},
	{NULL, NULL} /* Sentinel */
};

int luaopen_bz2(lua_State *L) {
	luaL_openlib(L, "bz2", bz2lib, 0);
	return 1;
}
