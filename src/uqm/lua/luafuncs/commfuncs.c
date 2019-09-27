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
 * Right now, the functions to call as a callback for responses are stored
 * in the Lua registry. This could however also be handled in Lua code
 * itself: one generic Lua function which is called after a response, which
 * then calls the appropriate function which was registered for the response
 * in Lua code.
 */

#include <stdlib.h>

#define LUAUQM_INTERNAL
#include "commfuncs.h"
#include "libs/scriptlib.h"
#include "libs/log.h"

#include "uqm/lua/luacomm.h"
#include "uqm/commglue.h"
#include "uqm/battle.h"
		// For instantVictory


static const char npcPhraseCallbackRegistryKey[] =
		"uqm_comm_npcPhraseCallback";
		// Key in the registry storing the callback function for
		// after an NPC phrase is complete.
static const char responseCallbackRegistryKey[] =
		"uqm_comm_responseCallback";
		// Key in the registry storing a table callback function for
		// after an NPC phrase is complete.


static int luaUqm_comm_isPhraseEnabled(lua_State *luaState);
static int luaUqm_comm_disablePhrase(lua_State *luaState);
static int luaUqm_comm_doNpcPhrase(lua_State *luaState);
static int luaUqm_comm_addResponse(lua_State *luaState);
static int luaUqm_comm_getPhrase(lua_State *luaState);
static int luaUqm_comm_getSegue(lua_State *luaState);
static int luaUqm_comm_setSegue(lua_State *luaState);
static int luaUqm_comm_isInOuttakes(lua_State *luaState);

static const luaL_Reg commFuncs[] = {
	{ "addResponse",     luaUqm_comm_addResponse },
	{ "disablePhrase",   luaUqm_comm_disablePhrase },
	{ "doNpcPhrase",     luaUqm_comm_doNpcPhrase },
	{ "getPhrase",       luaUqm_comm_getPhrase },
	{ "getSegue",        luaUqm_comm_getSegue },
	{ "isInOuttakes",    luaUqm_comm_isInOuttakes },
	{ "isPhraseEnabled", luaUqm_comm_isPhraseEnabled },
	{ "setSegue",        luaUqm_comm_setSegue },
	{ NULL,              NULL },
};

static const luaUqm_EnumValue segueEnum[] = {
	{ /* .name = */ "peace",   /* .value = */ Segue_peace  },
	{ /* .name = */ "hostile", /* .value = */ Segue_hostile  },
	{ /* .name = */ "victory", /* .value = */ Segue_victory },
	{ /* .name = */ "defeat",  /* .value = */ Segue_defeat  },
	{ /* .name = */ NULL,      /* .value = */ 0             },
};

int
luaUqm_comm_open(lua_State *luaState) {
	luaL_newlib(luaState, commFuncs);

	luaUqm_makeEnum(luaState, segueEnum);
	// [-2] -> table commTable
	// [-1] -> table segueEnum
	lua_setfield(luaState, -2, "segue");
			// comm.segue = segueEnum
	
	// Prepare a table to store the callback functions for each response in.
	lua_newtable(luaState);
    lua_setfield(luaState, LUA_REGISTRYINDEX, responseCallbackRegistryKey);

	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// Helper function. Returns the value of the RESPONSE_REF for the
// phrase given as a string on stack position [1].
// If it does not exist, -1 is returned and a warning is printed.
// [1] -> string phraseIdStr
static int
testPhraseId(lua_State *luaState, int argn) {
	const char *phraseIdStr = luaL_checkstring(luaState, argn);
	RESPONSE_REF phraseId = phraseIdStrToNum(phraseIdStr);
	if (phraseId == (RESPONSE_REF) -1) {
		// TODO: print script file name.
		log_add(log_Error, "[script] Warning: testPhraseId(): No phrase "
				"exists with id '%s'.", phraseIdStr);
		return -1;
	}

	return (int) phraseId;
}

// Pushes the string, or nil if the string is not known.
static void
pushPhraseId(lua_State *luaState, RESPONSE_REF response) {
	const char *phraseIdStr = phraseIdNumToStr(response);
	if (phraseIdStr != NULL) {
		lua_pushstring(luaState, phraseIdStr);
	} else {
		lua_pushnil(luaState);
	}
}

// Store a Lua callback function to be called from npcPhraseCallback(),
// which is used as a callback for NPCPhrase_cb().
// [n] -> function callback
static void
setNpcPhraseCallback(lua_State *luaState, int argn) {
	lua_pushvalue(luaState, argn);
    lua_setfield(luaState, LUA_REGISTRYINDEX, npcPhraseCallbackRegistryKey);
}

// The callback function is pushed on the stack.
static void
pushNpcPhraseCallback(lua_State *luaState) {
    lua_getfield(luaState, LUA_REGISTRYINDEX, npcPhraseCallbackRegistryKey);
}

static void
pushResponseCallbackRegistry(lua_State *luaState) {
    lua_getfield(luaState, LUA_REGISTRYINDEX, responseCallbackRegistryKey);
}

// [n] -> function callback
// Store a Lua callback function to be called from responseCallback(),
// which is used as a callback for Response().
static void
setResponseCallback(lua_State *luaState, int responseArgN,
		int callbackArgN) {
	pushResponseCallbackRegistry(luaState);
	// [-1] -> table responseCallbackRegistry

	lua_pushvalue(luaState, responseArgN);
	lua_pushvalue(luaState, callbackArgN);
	// [-3] -> table responseCallbackRegistry
	// [-2] -> string response
	// [-1] -> function callback
    lua_settable(luaState, -3);
	
	// [-3] -> table responseCallbackRegistry
	lua_pop(luaState, 1);
}

// The callback function is pushed on the stack.
static void
pushResponseCallback(lua_State *luaState, RESPONSE_REF response) {
	pushResponseCallbackRegistry(luaState);
	pushPhraseId(luaState, response);
	// [-2] -> table responseCallbackRegistry
	// [-1] -> string response
	lua_gettable(luaState, -2);

	// [-2] -> table responseCallbackRegistry
	// [-1] -> function callback
	lua_replace(luaState, -2);
	// [-1] -> function callback
}

// Used as a callback function for NPCPhrase_cb().
// It in turn calls the registered Lua callback function.
static void
npcPhraseCallback(CallbackArg extra) {
	pushNpcPhraseCallback(luaUqm_commState);
	if (lua_pcall(luaUqm_commState, 0, 0, 0) != 0) {
		// An error occurred. We continue nonetheless.
		log_add(log_Error, "[script] An error occurred during a "
				"doNpcPhrase() callback: %s",
				lua_tostring(luaUqm_commState, -1));
		lua_pop(luaUqm_commState, 1);
	}
	(void) extra;
}

// Used as a callback function for Response().
// It in turn calls the registered Lua callback function.
static void
responseCallback(RESPONSE_REF response) {
	pushResponseCallback(luaUqm_commState, response);
	pushPhraseId(luaUqm_commState, response);
	if (lua_pcall(luaUqm_commState, 1, 0, 0) != 0) {
		// An error occurred. We continue nonetheless.
		log_add(log_Error, "[script] An error occurred during an "
				"addResponse() callback: %s",
				lua_tostring(luaUqm_commState, -1));
		lua_pop(luaUqm_commState, 1);
	}
}

/////////////////////////////////////////////////////////////////////////////

// [1] -> string phraseIdStr
static int
luaUqm_comm_isPhraseEnabled(lua_State *luaState) {
	int phraseId = testPhraseId(luaState, 1);
	if (phraseId == -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	lua_pushboolean(luaState, PHRASE_ENABLED(phraseId));
	return 1;
}

// [1] -> string phraseIdStr
static int
luaUqm_comm_disablePhrase(lua_State *luaState) {
	int phraseId = testPhraseId(luaState, 1);
	if (phraseId == -1)
		return 0;

	DISABLE_PHRASE(phraseId);
	return 0;
}

// [1] -> string phraseIdStr
static int
luaUqm_comm_doNpcPhrase(lua_State *luaState) {
	CallbackFunction callback;
	int phraseId = testPhraseId(luaState, 1);
	if (phraseId == -1)
		return 0;

	if (lua_gettop(luaState) >= 2) {
		// Callback function specified in second argument.
		setNpcPhraseCallback(luaState, 2);
		callback = npcPhraseCallback;
	} else {
		callback = NULL;
	}

	NPCPhrase_cb(phraseId, callback);
	return 0;
}

// [1] -> string phraseIdStr
// [2] -> function callback
static int
luaUqm_comm_addResponse(lua_State *luaState) {
	int phraseId = testPhraseId(luaState, 1);
	if (phraseId == -1)
		return 0;

	luaL_checktype(luaState, 2, LUA_TFUNCTION);

	setResponseCallback(luaState, 1, 2);
	Response(phraseId, responseCallback);
	return 0;
}

// [1] -> string phraseIdStr
static int
luaUqm_comm_getPhrase(lua_State *luaState) {
	int phraseId;
	STRING str;
	const char *strBuf;

	phraseId = testPhraseId(luaState, 1);
	if (phraseId == -1) {
		// A warning is already printed in testPhraseId().
		lua_pushnil(luaState);
		return 1;
	}

	// Find the string.
	str = SetAbsStringTableIndex(CommData.ConversationPhrases,
			phraseId - 1);
	strBuf = GetStringAddress(str);

	if (luaUqm_comm_stringNeedsInterpolate(strBuf))
	{
		char *interpolated = luaUqm_comm_stringInterpolate(strBuf);
		lua_pushstring(luaState, interpolated);  // This makes a copy.
		HFree(interpolated);
	}
	else
	{
		// No interpolation is necessary.
		lua_pushstring(luaState, strBuf);
	}

	// [1] -> string phrase
	return 1;
}

static int
luaUqm_comm_getSegue(lua_State *luaState) {
	int result = getSegue();
	lua_pushinteger(luaState, result);
	return 1;
}

// [1] -> string phraseIdStr
static int
luaUqm_comm_setSegue(lua_State *luaState) {
	int what = luaL_checkint(luaState, 1);
	switch ((Segue) what) {
		case Segue_peace:
		case Segue_hostile:
		case Segue_victory:
		case Segue_defeat:
			break;
		default:
			log_add(log_Error, "[script] Warning: setSegue(): Invalid "
					"parameter value (%d).", what);
			break;
	};
	setSegue((Segue) what);

	return 0;
}

static int
luaUqm_comm_isInOuttakes(lua_State *luaState) {
	BOOLEAN result = (LOBYTE(GLOBAL(CurrentActivity)) == WON_LAST_BATTLE);
	lua_pushboolean(luaState, result);
	return 1;
}

