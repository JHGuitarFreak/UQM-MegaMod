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
 * This file contains code for using Lua for game event scripts.
 */

#include <stdlib.h>
#include <string.h>

#define LUAUQM_INTERNAL
#include "port.h"
#include "luaevent.h"

#include "options.h"
		// For contentDir
#include "libs/scriptlib.h"
#include "uqm/lua/luastate.h"
#include "luafuncs/customfuncs.h"
#include "luafuncs/eventfuncs.h"
#include "luafuncs/logfuncs.h"
#include "luafuncs/statefuncs.h"
#include "libs/log.h"

lua_State *luaUqm_eventState = NULL;
	
static const luaL_Reg eventLibs[] = {
	{ "event", luaUqm_event_open },
	{ "log",   luaUqm_log_open },
	{ "state", luaUqm_state_open },
	{ NULL, NULL }
};

// Not reentrant.
BOOLEAN
luaUqm_event_init(const luaUqm_custom_Function *customFuncs,
		RESOURCE scriptRes) {
	char *scriptFileName;
	BOOLEAN loadOk;

	assert(luaUqm_eventState == NULL);

#ifdef EVENT_DEBUG
	log_add(log_Debug, "[script] Calling luaUqm_event_init()");
#endif

	luaUqm_eventState = luaUqm_globalState;

	// Prepare the global environment.
	luaUqm_prepareEnvironment(luaUqm_eventState);
	luaUqm_loadLibs(luaUqm_eventState, eventLibs);
	luaUqm_custom_init(luaUqm_eventState, customFuncs);
	lua_pop(luaUqm_eventState, 1);

	// Get the name of the script.
	scriptFileName = LoadScriptInstance(scriptRes);
	if (scriptFileName == NULL)
		return FALSE;

	// Load the script.
	loadOk = luaUqm_loadScript(luaUqm_eventState, contentDir, scriptFileName);
	ReleaseScriptResData(scriptFileName);
	if (!loadOk)
		return FALSE;

	// Call the script.
	luaUqm_callStackFunction(luaUqm_eventState);
	return TRUE;
}

void
luaUqm_event_uninit(void) {
	assert(luaUqm_eventState != NULL);

#ifdef EVENT_DEBUG
	log_add(log_Debug, "[script] Calling luaUqm_event_uninit()");
#endif

	luaUqm_eventState = NULL;
}

void
luaUqm_event_callEvent(const char *eventIdStr) {
	assert(luaUqm_eventState != NULL);

#ifdef EVENT_DEBUG
	log_add(log_Debug, "[script] Calling luaUqm_event_callEvent(\"%s\")",
			eventIdStr);
#endif

	luaUqm_getEventTable(luaUqm_eventState);
	lua_pushstring(luaUqm_eventState, eventIdStr);
	// [-2] -> table eventTable
	// [-1] -> string eventIdStr
	lua_gettable(luaUqm_eventState, -2);

	// [-2] -> table eventTable
	// [-1] -> function eventFun
	if (lua_isnil(luaUqm_eventState, -1)) {
		log_add(log_Warning, "[script] Warning: luaUqm_event_callEvent(): "
				"Event '%s' is not registered.", eventIdStr);
		lua_pop(luaUqm_eventState, 2);
		return;
	}

	// [-2] -> table eventTable
	// [-1] -> function eventFun
	lua_replace(luaUqm_eventState, -2);

	// [-1] -> function eventFun
	luaUqm_callStackFunction(luaUqm_eventState);
}

