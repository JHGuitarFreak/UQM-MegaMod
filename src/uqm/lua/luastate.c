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

/*
 * This file contains code for connecting storing the game state in Lua.
 */

#include "luastate.h"
#include "luainit.h"
#include "uqm/globdata.h"
#include "libs/log.h"
#include "libs/scriptlib.h"


// We store the game state in the global Lua context, in the Lua registry.

static void luaUqm_initStatePropertyTable(lua_State *luaState);
static void luaUqm_initEventTable(lua_State *luaState);

lua_State *luaUqm_globalState = NULL;
static const char statePropRegistryKey[] =
		"uqm_state_prop_registryKey";
static const char eventRegistryKey[] =
		"uqm_event_registryKey";


// Init the global Lua state. Called at the start of the main loop.
void
luaUqm_initState(void) {
	if (luaUqm_globalState != NULL) {
		log_add(log_Warning, "Lua state multiply uninitialized");
		luaUqm_uninitState ();
	}
	luaUqm_globalState = luaL_newstate();
	luaUqm_initStatePropertyTable(luaUqm_globalState);
	luaUqm_initEventTable(luaUqm_globalState);

	// XXX TODO: set up an alternative to the Lua 'require' function,
	// which makes use of uio.

	luaUqm_runInitScripts();
}

// Uninit the global Lua state.
void
luaUqm_uninitState(void) {
	if (luaUqm_globalState != NULL) {
		lua_close(luaUqm_globalState);
		luaUqm_globalState = NULL;
	} else {
		log_add(log_Warning, "Lua state multiply uninitialized");
	}
}

// Reinit the global Lua state. This does nothing that initState doesn't
// do, but unlike initState it warns only if you call it without having
// old data to dispose of. Called at the start of a new game, or when a
// game is loaded.

void
luaUqm_reinitState(void) {
	if (luaUqm_globalState == NULL) {
		log_add(log_Warning, "Lua state reinitialized while NULL");
	} else {
		luaUqm_uninitState ();
	}
	luaUqm_initState ();
}

/////////////////////////////////////////////////////////////////////////////
// Game state
/////////////////////////////////////////////////////////////////////////////

static void
luaUqm_initStatePropertyTable(lua_State *luaState)
{
	lua_pushstring(luaState, statePropRegistryKey);
	lua_newtable(luaState);
	lua_settable(luaState, LUA_REGISTRYINDEX);
}

// Check whether a lua value has a type acceptable as a property value.
int
luaUqm_checkPropValueType(lua_State *luaState, const char *funName,
		int nameIndex) {
	int type = lua_type(luaState, nameIndex);
	switch (type) {
		case LUA_TNUMBER:
		case LUA_TBOOLEAN:
		case LUA_TSTRING:
			// Ok
			return 0;
		default: {
			const char *typeName = lua_typename(luaState, nameIndex);
			return luaL_error(luaState, "Property value has an invalid "
					"type, in parameter to %s() (%s)", funName, typeName);
		}
	}
}

// Set the value of the property with the name on the stack on position
// 'nameIndex' to the value on the stack with index 'valueIndex'.
// Pre: nameIndex points to a string, and valueIndex points to a valid
// value.
void
luaUqm_setProp(lua_State *luaState, int nameIndex, int valueIndex) {
	nameIndex = lua_absindex(luaState, nameIndex);
	valueIndex = lua_absindex(luaState, valueIndex);

	lua_getfield(luaState, LUA_REGISTRYINDEX, statePropRegistryKey);
	lua_pushvalue(luaState, nameIndex);
	lua_pushvalue(luaState, valueIndex);
	// [-3] -> registry[statePropRegistrykey]
	// [-2] -> name
	// [-1] -> value
	lua_settable(luaState, -3);
	// [-1] -> registry[statePropRegistrykey]
	lua_pop(luaState, 1);
}

// Get the value of the property with the name on the stack on position
// 'nameIndex'.
// Pushes the property value on the stack.
// Pre: nameIndex points to a string.
void
luaUqm_getProp(lua_State *luaState, int nameIndex) {
	nameIndex = lua_absindex(luaState, nameIndex);

	lua_getfield(luaState, LUA_REGISTRYINDEX, statePropRegistryKey);
	// [-1] -> registry[statePropRegistrykey]
	lua_pushvalue(luaState, nameIndex);
	// [-2] -> registry[statePropRegistrykey]
	// [-1] -> name
	lua_gettable(luaState, -2);
	// [-2] -> registry[statePropRegistrykey]
	// [-1] -> registry[statePropRegistrykey][name]
	lua_replace(luaState, -2);
	// [-1] -> registry[statePropRegistrykey][name]
}

void
setGameStateUint(const char *name, DWORD val)
{
	lua_pushstring(luaUqm_globalState, name);
	lua_pushinteger(luaUqm_globalState, val);
	luaUqm_setProp(luaUqm_globalState, -2, -1);
	lua_pop(luaUqm_globalState, 2);

#ifdef STATE_DEBUG
	log_add(log_Debug, "State '%s' set to %u.", name, val);
#endif
}

DWORD
getGameStateUint(const char *name)
{
	DWORD result;
	int resultType;

	lua_pushstring(luaUqm_globalState, name);
	luaUqm_getProp(luaUqm_globalState, -1);
	// [-2] -> name
	// [-1] -> propValue

	resultType = lua_type(luaUqm_globalState, -1);
	switch (resultType) {
		case LUA_TNIL:
			// Unitialised properties are 0.
			lua_pop(luaUqm_globalState, 2);
			return 0;
		case LUA_TNUMBER:
			// Ok.
			break;
		default:
			log_add(log_Error, "Warning: getGameState(): property '%s' has "
					"a non-number value (%s).", name,
					lua_typename(luaUqm_globalState, -1));
			lua_pop(luaUqm_globalState, 2);
			return 0;
	}

	result = (DWORD) lua_tointeger(luaUqm_globalState, -1);
	lua_pop(luaUqm_globalState, 2);
	return result;
}

/////////////////////////////////////////////////////////////////////////////
// Game events
/////////////////////////////////////////////////////////////////////////////

static void
luaUqm_initEventTable(lua_State *luaState)
{
	lua_pushstring(luaState, eventRegistryKey);
	lua_newtable(luaState);
	lua_settable(luaState, LUA_REGISTRYINDEX);
}

void
luaUqm_getEventTable(lua_State *luaState)
{
	lua_getfield(luaState, LUA_REGISTRYINDEX, eventRegistryKey);
}

