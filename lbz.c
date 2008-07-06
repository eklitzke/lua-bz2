#include <bzlib.h>
#include <alloca.h>
#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define LBZ_EOS    0x01 /* end of stream */
#define LBZ_CLOSED 0x02 /* the file is closed */

typedef struct {
	BZFILE *bz_stream;
	FILE *f;
	int flags;
} lbz_state;

static int lbz_read_open(lua_State *L);
static int lbz_read(lua_State *L);
static int lbz_read_close(lua_State *L);

static const struct luaL_reg bzlib_f [] = {
	{"open", lbz_read_open},
	{NULL, NULL} /* Sentinel */
};

static const struct luaL_reg bzlib_m [] = {
	{"read", lbz_read},
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

	luaL_getmetatable(L, "LuaBook.bz2");
	lua_setmetatable(L, -2);

	if (bzerror != BZ_OK)
		lua_pushnil(L);

	return 1;
}

/* Binding to libbzip2's BZ2_bzReadOpen method */
static int lbz_read(lua_State *L) {
	int bzerror;
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
	int ret = BZ2_bzRead(&bzerror, state->bz_stream, buf, len);

	if (bzerror != BZ_OK && bzerror != BZ_STREAM_END) {
		lua_pushnil(L);
		return 1;
	}

	if (bzerror == BZ_STREAM_END)
		state->flags |= LBZ_EOS;

	luaL_addlstring(&b, buf, ret);
	luaL_pushresult(&b);
	return 1;
}

/* Binding to libbzip2's BZ2_bzReadClose method */
static int lbz_read_close(lua_State *L) {
	int bzerror;
	lbz_state *state = (lbz_state *) lua_touserdata(L, 1);
	BZ2_bzReadClose(&bzerror, state->bz_stream);
	fclose(state->f);
	state->flags |= LBZ_CLOSED;
	lua_pushnil(L);
	return 1;
}

int luaopen_bz2(lua_State *L) {
	luaL_newmetatable(L, "LuaBook.bz2");

	lua_pushstring(L, "__index");
	lua_pushvalue(L, -2); /* push the metatable */
	lua_settable(L, -3); /* metatable.__index = metatable */

	luaL_openlib(L, NULL, bzlib_m, 0);
	luaL_openlib(L, "bz2", bzlib_f, 0);
	return 1;
}
