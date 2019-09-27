/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define LUAUQM_INTERNAL
#include "logfuncs.h"
#include "libs/scriptlib.h"
#include "libs/log.h"

static int luaUqm_log_debug (lua_State *luaState);
static int luaUqm_log_error (lua_State *luaState);
static int luaUqm_log_fatal (lua_State *luaState);
static int luaUqm_log_info (lua_State *luaState);
static int luaUqm_log_warn (lua_State *luaState);

static const luaL_Reg logFuncs[] = {
	{ "debug", luaUqm_log_debug },
	{ "error", luaUqm_log_error },
	{ "fatal", luaUqm_log_fatal },
	{ "info",  luaUqm_log_info },
	{ "warn",  luaUqm_log_warn },
	{ NULL,    NULL },
};

int
luaUqm_log_open(lua_State *luaState) {
	luaL_newlib(luaState, logFuncs);
	return 1;
}

// [1] -> string logMessage
static int
logHelper(lua_State *luaState, log_Level level) {
	//const char *str = luaL_checkstring(luaState, 1);
	const char *str = lua_tostring(luaState, 1);
	// TODO: print the file name of the Lua script being executed.
	log_add(level, "Lua: %s", str);
	return 0;
}

// [1] -> string logMessage
static int
luaUqm_log_debug (lua_State *luaState) {
	return logHelper(luaState, log_Debug);
}

// [1] -> string logMessage
static int
luaUqm_log_error (lua_State *luaState) {
	return logHelper(luaState, log_Error);
}

// [1] -> string logMessage
static int
luaUqm_log_fatal (lua_State *luaState) {
	return logHelper(luaState, log_Fatal);
}

// [1] -> string logMessage
static int
luaUqm_log_info (lua_State *luaState) {
	return logHelper(luaState, log_Info);
}

// [1] -> string logMessage
static int
luaUqm_log_warn (lua_State *luaState) {
	return logHelper(luaState, log_Warning);
}

