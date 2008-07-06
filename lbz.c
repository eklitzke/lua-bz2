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

	if (bzerror != BZ_OK)
		lua_pushnil(L);
	return 1;
}

/* Binding to libbzip2's BZ2_bzReadOpen method */
int lbz_read(lua_State *L) {
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
