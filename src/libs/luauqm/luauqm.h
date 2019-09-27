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

#ifndef LIBS_LUAUQM_LUAUQM_H_
#define LIBS_LUAUQM_LUAUQM_H_

#ifdef USE_INTERNAL_LUA
#   include "libs/lua/lua.h"
#   include "libs/lua/lauxlib.h"
#else
#	include "lua.h"
#	include "lauxlib.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct luaUqm_EnumValue luaUqm_EnumValue;

#if defined(__cplusplus)
}
#endif

#include "libs/compiler.h"
#include "libs/uio.h"

#include "scriptres.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct luaUqm_EnumValue {
	const char *name;
	int value;
};

void luaUqm_init(void);
void luaUqm_uninit(void);

void luaUqm_prepareEnvironment(lua_State *luaState);
BOOLEAN luaUqm_loadScript(lua_State *luaState, uio_DirHandle *dir,
		const char *fileName);
BOOLEAN luaUqm_runScript(lua_State *luaState, uio_DirHandle *dir,
		const char *fileName);
void luaUqm_runLuaDir(lua_State *luaState, uio_DirHandle *dirHandle,
		const char *luaDirName);

void luaUqm_loadLib(lua_State *luaState, const luaL_Reg *lib);
void luaUqm_loadLibs(lua_State *luaState, const luaL_Reg *libs);
void luaUqm_loadSafeDefaultLibs(lua_State *luaState);
BOOLEAN luaUqm_callStackFunction(lua_State *luaState);
BOOLEAN luaUqm_callFunction(lua_State *luaState, const char *str);
void luaUqm_makeEnum(lua_State *luaState, const luaUqm_EnumValue *enumVals);

#ifdef DEBUG
void luaUqm_dumpStack(lua_State *luaState);
#endif

#if defined(__cplusplus)
}
#endif

#endif  /* LIBS_LUAUQM_LUAUQM_H_ */

