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

#include <stdlib.h>

#define LUAUQM_INTERNAL
#include "customfuncs.h"
#include "libs/scriptlib.h"
#include "libs/log.h"


// We use a wrapper, so that we can call the custom function with the
// arguments of our choice (i.e. no arguments).
static int
luaFunctionWrapper(lua_State *luaState) {
	int arg;
	int result;
	int isNum;
	int (*fun)(int) = lua_topointer(luaState, lua_upvalueindex(1));

	if (lua_gettop(luaState) == 0) {
		arg = 0;
	} else {
		arg = lua_tointegerx (luaState, -1, &isNum);
		if (!isNum) {
			log_add(log_Error, "[script] Warning: luaFunctionWrapper(): "
					"Invalid type of argument to custom function (%s).",
					lua_typename(luaState, lua_type(luaState, -1)));
			// arg will be 0
		}
	}
	
	result = (*fun)(arg);
	lua_pushinteger(luaState, result);
	return 1;
}

// [-1]  -> table t
// We keep the actual pointer as an upvalue in the closure.
static int
luaUqm_custom_addFunction(lua_State *luaState,
		const luaUqm_custom_Function *fun) {
	lua_pushlightuserdata (luaState, (void *) fun->fun);
	lua_pushcclosure(luaState, luaFunctionWrapper, 1);
	// [-2] -> table t
	// [-1] -> function fun
	lua_setfield(luaState, -2, fun->name);
	return 1;
}

// Returns with the new table on the stack.
int
luaUqm_custom_init(lua_State *luaState,
		const luaUqm_custom_Function *funs) {
	// Count the number of functions.
	size_t funCount = 0;
	const luaUqm_custom_Function *ptr;

	for (ptr = funs; ptr->name != NULL; ptr++)
		funCount++;

	lua_pushglobaltable(luaState);

	// Create a table for 'custom'.
	lua_createtable(luaState, 0, funCount);

	// Fill the 'custom' table.
	for (ptr = funs; ptr->name != NULL; ptr++)
		luaUqm_custom_addFunction(luaState, ptr);

	// Set 'custom' to the custom table.
	// [-2] -> table globalTable
	// [-1] -> table custom
	lua_setfield(luaState, -2, "custom");
	
	// [-1] -> table custom
	return 0;
}

