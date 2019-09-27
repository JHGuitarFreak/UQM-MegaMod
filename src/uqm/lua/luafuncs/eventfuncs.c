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
#include "eventfuncs.h"
#include "uqm/clock.h"
#include "uqm/gameev.h"
#include "uqm/lua/luastate.h"
#include "libs/scriptlib.h"
#include "libs/log.h"


static int luaUqm_event_addAbsolute(lua_State *luaState);
static int luaUqm_event_addRelative(lua_State *luaState);
static int luaUqm_event_register(lua_State *luaState);
static int luaUqm_event_unregister(lua_State *luaState);

static const luaL_Reg eventFuncs[] = {
	{ "addAbsolute", luaUqm_event_addAbsolute },
	{ "addRelative", luaUqm_event_addRelative },
	{ "register",    luaUqm_event_register },
	{ "unregister",  luaUqm_event_unregister },
	{ NULL,          NULL },
};

int
luaUqm_event_open(lua_State *luaState) {
	luaL_newlib(luaState, eventFuncs);

	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// argn -> the relative index on the lua stack which contains the Lua
//         string identifying the event.
// Returns true if and only if an event is registered.
static BOOLEAN
isEventRegistered(lua_State *luaState, int argn)
{
	BOOLEAN result;
	argn = lua_absindex(luaState, argn);

	luaUqm_getEventTable(luaState);
	lua_pushvalue(luaState, argn);
	// [-2] -> table eventTable
	// [-1] -> string eventIdStr
	lua_gettable(luaState, -2);

	// [-2] -> table eventTable
	// [-1] -> function eventFun
	result = !lua_isnil(luaState, -1);

	// [-2] -> table eventTable
	// [-1] -> function eventFun
	lua_pop(luaState, 2);

	return result;
}

// [1] -> int year
// [2] -> int month
// [3] -> int day
// [4] -> string eventIdStr
// Returns -1 on error, and a different value otherwise (currently 0, but
// don't rely on this).
// TODO: make this function return an identifier for the event, so that it
// can be removed.
static int
addEvent(lua_State *luaState, EVENT_TYPE type) {
	int year = luaL_checkint(luaState, 1);
	int month = luaL_checkint(luaState, 2);
	int day = luaL_checkint(luaState, 3);
	const char *eventIdStr = luaL_checkstring(luaState, 4);
	int eventNum;
	HEVENT event;

	if (!isEventRegistered(luaState, 4)) {
		log_add(log_Warning, "[script] event.%s(): Event '%s' is "
				"not registered.",
				(type == RELATIVE_EVENT) ? "addRelative" : "addAbsolute",
				lua_tostring(luaState, 1));
		lua_pushinteger(luaState, -1);
		return 1;
	}
	
	eventNum = eventIdStrToNum(eventIdStr);
	if (eventNum == -1) {
		log_add(log_Warning, "[script] event.%s(): Event '%s' is "
				"not known. It must currently be one of the hard-coded "
				"strings.",
				(type == RELATIVE_EVENT) ? "addRelative" : "addAbsolute",
				lua_tostring(luaState, 1));
		lua_pushinteger(luaState, -1);
		return 1;
	}

	event = AddEvent(type, month, day, year, eventNum);
	lua_pushinteger(luaState, (event == NULL) ? -1 : 0);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// [1] -> int year
// [2] -> int month
// [3] -> int day
// [4] -> string eventIdStr
// TODO: make this function return an identifier for the event, so that it
// can be removed.
static int
luaUqm_event_addAbsolute(lua_State *luaState) {
	return addEvent(luaState, ABSOLUTE_EVENT);
}

// [1] -> int years
// [2] -> int months
// [3] -> int days
// [4] -> string eventIdStr
// Returns -1 on error, and 0 otherwise.
// TODO: make this function return an identifier for the event, so that it
// can be removed.
static int
luaUqm_event_addRelative(lua_State *luaState) {
	return addEvent(luaState, RELATIVE_EVENT);
}

// [1] -> string eventIdStr
// [2] -> function eventFun
static int
luaUqm_event_register(lua_State *luaState) {
	(void) luaL_checkstring(luaState, 1);
	luaL_checktype(luaState, 2, LUA_TFUNCTION);

	if (isEventRegistered(luaState, 1)) {
		log_add(log_Warning, "[script] event.register(): Event '%s' is "
				"already registered.", lua_tostring(luaState, 1));
		return 0;
	}
	
	luaUqm_getEventTable(luaState);
	lua_pushvalue(luaState, 1);
	lua_pushvalue(luaState, 2);
	// [-3] -> table eventTable
	// [-2] -> string eventIdStr
	// [-1] -> function eventFun
	lua_settable(luaState, -3);

	// [-1] -> table eventTable
	lua_pop(luaState, 1);

	return 0;
}

// [1] -> string eventIdStr
static int
luaUqm_event_unregister(lua_State *luaState) {
	(void) luaL_checkstring(luaState, 1);

	if (!isEventRegistered(luaState, 1)) {
		log_add(log_Warning, "[script] event.unregister(): Event '%s' was "
				"not registered.", lua_tostring(luaState, 1));
		return 0;
	}
	
	luaUqm_getEventTable(luaState);
	lua_pushvalue(luaState, 1);
	lua_pushnil(luaState);
	// [-3] -> table eventTable
	// [-2] -> string eventIdStr
	// [-1] -> nil
	lua_settable(luaState, -3);

	// [-1] -> table eventTable
	lua_pop(luaState, 1);

	return 0;
}

