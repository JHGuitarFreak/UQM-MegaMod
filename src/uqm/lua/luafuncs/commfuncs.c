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

//#define DEBUG_STARSEED
#include <stdlib.h>

#define LUAUQM_INTERNAL
#include "commfuncs.h"
#include "libs/scriptlib.h"
#include "libs/log.h"

#include "uqm/lua/luacomm.h"
#include "uqm/commglue.h"
#include "uqm/battle.h"
		// For instantVictory
#include "uqm/comm.h"
#include "uqm/starmap.h" // for plot map, etc.
#include "uqm/gamestr.h" // for GAME_STRING
#include <ctype.h>		 // for islower, isupper, toupper


extern COUNT RoboTrack[];
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
static int luaUqm_comm_setCustomBaseline (lua_State *luaState);
static int luaUqm_comm_getPoint (lua_State *luaState);
static int luaUqm_comm_getStarName (lua_State *luaState);
static int luaUqm_comm_getConstellation (lua_State *luaState);
static int luaUqm_comm_getColor (lua_State *luaState);
static int luaUqm_comm_swapIfSeeded (lua_State *luaState);

static const luaL_Reg commFuncs[] = {
	{ "addResponse",       luaUqm_comm_addResponse },
	{ "disablePhrase",     luaUqm_comm_disablePhrase },
	{ "doNpcPhrase",       luaUqm_comm_doNpcPhrase },
	{ "getPhrase",         luaUqm_comm_getPhrase },
	{ "getSegue",          luaUqm_comm_getSegue },
	{ "isInOuttakes",      luaUqm_comm_isInOuttakes },
	{ "isPhraseEnabled",   luaUqm_comm_isPhraseEnabled },
	{ "setSegue",          luaUqm_comm_setSegue },
	{ "setCustomBaseline", luaUqm_comm_setCustomBaseline },
	{ "getPoint",          luaUqm_comm_getPoint },
	{ "getStarName",       luaUqm_comm_getStarName },
	{ "getConstellation",  luaUqm_comm_getConstellation },
	{ "getColor",          luaUqm_comm_getColor },
	{ "swapIfSeeded",      luaUqm_comm_swapIfSeeded },
	{ NULL,              NULL },
};

static const luaUqm_EnumValue segueEnum[] = {
	{ /* .name = */ "peace",   /* .value = */ Segue_peace   },
	{ /* .name = */ "hostile", /* .value = */ Segue_hostile },
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

// [1] -> int lineNumber
// [2] -> int baseLineX
// [3] -> int baseLineY
// [4] -> string alignment
static int
luaUqm_comm_setCustomBaseline (lua_State *luaState)
{
	static const char *const textAlign[] =
			{ "ALIGN_LEFT", "ALIGN_CENTER", "ALIGN_RIGHT", NULL };
	COUNT lineNumber = luaL_checkint (luaState, 1);
	COORD baselineX = luaL_checkint (luaState, 2);
	COORD baselineY = luaL_checkint (luaState, 3);
	int alignment =
			luaL_checkoption (luaState, 4, "ALIGN_LEFT", textAlign);

	SetCustomBaseLine (lineNumber, (POINT) { baselineX, baselineY },
			alignment);

	lua_pushstring (luaState, "");

	return 1;
}

// Prints out the coordinates "044.6 : 540.0" of the plot ID provided.
// [1] -> string default text
// [2] -> int plot_id (from plandata)
static int
luaUqm_comm_getPoint (lua_State *luaState)
{
	const char *prime_text = luaL_checkstring(luaState, 1);
	if (!StarSeed)
	{
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	const char *plot_name = luaL_checkstring(luaState, 2);
	COUNT plot_id = PlotIdStrToIndex (plot_name);
#ifdef DEBUG_STARSEED
	fprintf (stderr, "get Point called (%s %s) plot ID %d\n", prime_text,
			plot_name, plot_id);
#endif
	if (plot_id >= NUM_PLOTS)
	{
		fprintf (stderr, "Plot not found for Point (%s %s).\n", prime_text,
				plot_name);
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	char dialog[256];
	snprintf (dialog, sizeof (dialog), "%05.1f : %05.1f",
			(float) plot_map[plot_id].star_pt.x / 10,
			(float) plot_map[plot_id].star_pt.y / 10);
	lua_pushstring (luaState, dialog);
	RoboTrack[0] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.x / 1000;
	RoboTrack[1] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.x / 100 % 10;
	RoboTrack[2] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.x / 10 % 10;
	RoboTrack[3] = ROBOT_POINT;
	RoboTrack[4] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.x % 10;
	RoboTrack[5] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.y / 1000;
	RoboTrack[6] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.y / 100 % 10;
	RoboTrack[7] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.y / 10 % 10;
	RoboTrack[8] = ROBOT_POINT;
	RoboTrack[9] = ROBOT_DIGIT_0 + plot_map[plot_id].star_pt.y % 10;
	return 1;
}

// A helper function to upper case dialog if key is upper case.
// If the first two characters of key are upper case, upper the whole dialog.
// If the first character of key is upper, upper the first char of dialog.
void
CheckCase (const char *key, char *dialog)
{
	COUNT i = 0;
	if (isupper (key[0]) && isupper (key[1]))
		while (dialog[i] != '\0')
		{
			dialog[i] = toupper (dialog[i]);
			i++;
		}
	else if (isupper (key[0]) && islower (dialog[0]))
		dialog[0] = toupper (dialog[0]);
}

// Prints out the fully qualified star name, e.g. "Alpha Pavonis"
// [1] -> the default text for prime seed
// [2] -> the string name of the plot ID for seeding
static int
luaUqm_comm_getStarName (lua_State *luaState)
{
	const char *prime_text = luaL_checkstring(luaState, 1);
	if (!StarSeed)
	{
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	const char *plot_name = luaL_checkstring(luaState, 2);
	COUNT plot_id = PlotIdStrToIndex (plot_name);
#ifdef DEBUG_STARSEED
	fprintf (stderr, "get Star Name called (%s %s) plot ID %d\n",
			prime_text, plot_name, plot_id);
#endif
	if (plot_id >= NUM_PLOTS)
	{
		fprintf (stderr, "Plot not found for Star Name (%s %s).\n",
				prime_text, plot_name);
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	char dialog[256];
	GetClusterName (plot_map[plot_id].star, dialog);
	CheckCase (prime_text, dialog);
	lua_pushstring (luaState, dialog);
	if (plot_map[plot_id].star->Prefix > 0)
	{
		RoboTrack[0] = ROBOT_PREFIX_0 + plot_map[plot_id].star->Prefix;
		RoboTrack[1] = ROBOT_POSTFIX_0 + plot_map[plot_id].star->Postfix;
	}
	else
		RoboTrack[0] = ROBOT_POSTFIX_0 + plot_map[plot_id].star->Postfix;
	return 1;
}

// Prints out the nearest constellation name, e.g. "Pavonis"
// [1] -> the default text for prime seed
// [2] -> the string name of the plot ID for seeding
static int
luaUqm_comm_getConstellation (lua_State *luaState)
{
	const char *prime_text = luaL_checkstring(luaState, 1);
	if (!StarSeed)
	{
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	const char *plot_name = luaL_checkstring(luaState, 2);
	COUNT plot_id = PlotIdStrToIndex (plot_name);
#ifdef DEBUG_STARSEED
	fprintf (stderr, "get Constellation called (%s %s) plot ID %d\n",
			prime_text, plot_name, plot_id);
#endif
	if (plot_id >= NUM_PLOTS)
	{
		fprintf (stderr, "Plot not found for Constellation (%s %s).\n",
				prime_text, plot_name);
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	char dialog[256];
	STAR_DESC *SDPtr = FindNearestConstellation
			(star_array, plot_map[plot_id].star_pt);
	snprintf (dialog, sizeof (dialog), "%s",
			GAME_STRING (SDPtr->Postfix));
	CheckCase (prime_text, dialog);
	lua_pushstring (luaState, dialog);
	RoboTrack[0] = ROBOT_POSTFIX_0 + SDPtr->Postfix;
	return 1;
}

// Prints out the color of the star
// [1] -> the default text for prime seed
// [2] -> the string name of the plot ID for seeding
static int
luaUqm_comm_getColor (lua_State *luaState)
{
	const char *prime_text = luaL_checkstring(luaState, 1);
	if (!StarSeed)
	{
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	const char *plot_name = luaL_checkstring(luaState, 2);
	COUNT plot_id = PlotIdStrToIndex (plot_name);
#ifdef DEBUG_STARSEED
	fprintf (stderr, "get Color called (%s %s)\n", prime_text, plot_name);
#endif
	if (plot_id >= NUM_PLOTS)
	{
		fprintf (stderr, "Plot not found for getColor (%s %s).\n",
				prime_text, plot_name);
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	char dialog[256];
	switch (STAR_COLOR(plot_map[plot_id].star->Type))
	{
		case RED_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "red");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_RED : ROBOT_COLOR_RED;
			break;
		case ORANGE_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "orange");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_ORANGE : ROBOT_COLOR_ORANGE;
			break;
		case YELLOW_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "yellow");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_YELLOW : ROBOT_COLOR_YELLOW;
			break;
		case GREEN_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "green");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_GREEN : ROBOT_COLOR_GREEN;
			break;
		case BLUE_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "blue");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_BLUE : ROBOT_COLOR_BLUE;
			break;
		case WHITE_BODY:
			snprintf (dialog, sizeof (dialog), "%s", "white");
			RoboTrack[0] = (plot_id == ILWRATH_DEFINED) ?
					ILWRATH_COLOR_WHITE : ROBOT_COLOR_WHITE;
			break;
		default:
			snprintf (dialog, sizeof (dialog), "%s", "unknown");
			RoboTrack[0] = ROBOT_NULL_PHRASE;
			break;
	}
	CheckCase (prime_text, dialog);
	lua_pushstring (luaState, dialog);
	return 1;
}

// Prints out the second string instead of the first string.
// Used to curate text around plot-replacement lookups.
// [1] -> the default text for prime seed
// [2] -> the replacement text for starseed
static int
luaUqm_comm_swapIfSeeded (lua_State *luaState)
{
	const char *prime_text = luaL_checkstring(luaState, 1);
	if (!StarSeed)
	{
		lua_pushstring (luaState, prime_text);
		return 1;
	}
	const char *seed_text = luaL_checkstring(luaState, 2);
#ifdef DEBUG_STARSEED
	fprintf (stderr, "Swap If Seeded called (%s %s)\n", prime_text, seed_text);
#endif
	lua_pushstring (luaState, seed_text);
	RoboTrack[0] = (COUNT) ~0;
	return 1;
}
