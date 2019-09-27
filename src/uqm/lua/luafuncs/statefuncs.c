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
#include "statefuncs.h"
#include "libs/scriptlib.h"
#include "libs/log.h"

#include "uqm/build.h"
#include "uqm/globdata.h"
#include "uqm/lua/luastate.h"


static int luaUqm_state_clock_getDate(lua_State *luaState);
static int luaUqm_state_escort_addShips(lua_State *luaState);
static int luaUqm_state_escort_canAddShips(lua_State *luaState);
static int luaUqm_state_escort_removeShips (lua_State *luaState);
static int luaUqm_state_escort_shipCount(lua_State *luaState);
static int luaUqm_state_escort_totalValue(lua_State *luaState);
static int luaUqm_state_race_isAlive(lua_State *luaState);
static int luaUqm_state_race_isAllied(lua_State *luaState);
static int luaUqm_state_race_isKnown(lua_State *luaState);
static int luaUqm_state_race_setAlive(lua_State *luaState);
static int luaUqm_state_race_setAllied(lua_State *luaState);
static int luaUqm_state_race_setKnown(lua_State *luaState);
static int luaUqm_state_sis_addCrew(lua_State *luaState);
static int luaUqm_state_sis_addFuel(lua_State *luaState);
static int luaUqm_state_sis_addLanders(lua_State *luaState);
static int luaUqm_state_sis_addResUnits(lua_State *luaState);
static int luaUqm_state_sis_getCaptainName(lua_State *luaState);
static int luaUqm_state_sis_getCrew(lua_State *luaState);
static int luaUqm_state_sis_getFuel(lua_State *luaState);
static int luaUqm_state_sis_getLanders(lua_State *luaState);
static int luaUqm_state_sis_getResUnits(lua_State *luaState);
static int luaUqm_state_sis_getShipName(lua_State *luaState);
static int luaUqm_state_prop_get(lua_State *luaState);
static int luaUqm_state_prop_set(lua_State *luaState);

static const luaL_Reg stateClockFuncs[] = {
	{ "getDate",         luaUqm_state_clock_getDate },
	{ NULL,              NULL },
};

static const luaL_Reg stateEscortFuncs[] = {
	{ "addShips",        luaUqm_state_escort_addShips },
	{ "canAddShips",     luaUqm_state_escort_canAddShips },
	{ "removeShips",     luaUqm_state_escort_removeShips },
	{ "shipCount",       luaUqm_state_escort_shipCount },
	{ "totalValue",      luaUqm_state_escort_totalValue },
	{ NULL,              NULL },
};

static const luaL_Reg statePropFuncs[] = {
	{ "get",             luaUqm_state_prop_get },
	{ "set",             luaUqm_state_prop_set },
	{ NULL,              NULL },
};

static const luaL_Reg stateRaceFuncs[] = {
	{ "isAlive",         luaUqm_state_race_isAlive },
	{ "isAllied",        luaUqm_state_race_isAllied },
	{ "isKnown",         luaUqm_state_race_isKnown },
	{ "setAlive",        luaUqm_state_race_setAlive },
	{ "setAllied",       luaUqm_state_race_setAllied },
	{ "setKnown",        luaUqm_state_race_setKnown },
	{ NULL,              NULL },
};

static const luaL_Reg stateSisFuncs[] = {
	{ "addCrew",         luaUqm_state_sis_addCrew },
	{ "addFuel",         luaUqm_state_sis_addFuel },
	{ "addLanders",      luaUqm_state_sis_addLanders },
	{ "addResUnits",     luaUqm_state_sis_addResUnits },
	{ "getCaptainName",  luaUqm_state_sis_getCaptainName },
	{ "getCrew",         luaUqm_state_sis_getCrew },
	{ "getFuel",         luaUqm_state_sis_getFuel },
	{ "getLanders",      luaUqm_state_sis_getLanders },
	{ "getResUnits",     luaUqm_state_sis_getResUnits },
	{ "getShipName",     luaUqm_state_sis_getShipName },
	{ NULL,              NULL },
};

int
luaUqm_state_open(lua_State *luaState) {
	// Create a table on the stack with space reserved for five fields.
	lua_createtable(luaState, 0, 5);

	luaL_newlib(luaState, stateClockFuncs);
	lua_setfield(luaState, -2, "clock");
	
	luaL_newlib(luaState, stateEscortFuncs);
	lua_setfield(luaState, -2, "escort");
	
	luaL_newlib(luaState, statePropFuncs);
	lua_setfield(luaState, -2, "prop");

	luaL_newlib(luaState, stateRaceFuncs);
	lua_setfield(luaState,  -2,"race");

	luaL_newlib(luaState, stateSisFuncs);
	lua_setfield(luaState, -2, "sis");

	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// Helper function. Returns an index for the race in the avail_race_q
// for the race given as a string on stack position [1].
// If it does not exist, -1 is returned and a warning is printed.
// [1] -> string raceIdStr
static COUNT
testRaceId(lua_State *luaState, int argn) {
	const char *raceIdStr = luaL_checkstring(luaState, argn);
	COUNT raceId = RaceIdStrToIndex(raceIdStr);
	if (raceId == (COUNT) -1) {
		// TODO: print script file name.
		log_add(log_Error, "[script] Warning: testRaceId(): No race exists "
				"with id '%s'.", raceIdStr);
		return (COUNT) -1;
	}

	return (COUNT) raceId;
}

// Helper function. Returns an index for the ship in the avail_race_q
// for the ship given as a string on stack position [1].
// If it does not exist, -1 is returned and a warning is printed.
// [1] -> string shipIdStr
static COUNT
testShipId(lua_State *luaState, int argn) {
	const char *shipIdStr = luaL_checkstring(luaState, argn);
	COUNT shipId = ShipIdStrToIndex(shipIdStr);
	if (shipId == (COUNT) -1) {
		// TODO: print script file name.
		log_add(log_Error, "[script] Warning: testShipId(): No ship exists "
				"with id '%s'.", shipIdStr);
		return (COUNT) -1;
	}

	return (COUNT) shipId;
}

#if 0
// Pushes the string, or nil if the string is not known.
static void
pushRaceId(lua_State *luaState, COUNT raceId) {
	const char *raceIdStr = raceIdNumToStr(raceId);
	if (raceIdStr != NULL) {
		lua_pushstring(luaState, raceIdStr);
	} else {
		lua_pushnil(luaState);
	}
}
#endif

/////////////////////////////////////////////////////////////////////////////

// Returns a table with the fields 'year', 'month', and 'day'.
static int
luaUqm_state_clock_getDate(lua_State *luaState) {
	// Create a table on the stack with space reserved for 3 fields.
	lua_createtable(luaState, 0, 3);

	lua_pushinteger(luaState, GLOBAL(GameClock.year_index));
	lua_setfield(luaState, -2, "year");
	
	lua_pushinteger(luaState, GLOBAL(GameClock.month_index));
	lua_setfield(luaState, -2, "month");

	lua_pushinteger(luaState, GLOBAL(GameClock.day_index));
	lua_setfield(luaState, -2, "day");

	return 1;
}

// [1] -> string shipIdStr
// [2] -> int count
static int
luaUqm_state_escort_addShips(lua_State *luaState) {
	COUNT shipId;
	int count;
	int numAdded;

	shipId = testShipId(luaState, 1);
	if (shipId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	count = luaL_checkint(luaState, 2);

	numAdded = AddEscortShips(shipId, count);
	lua_pushinteger(luaState, numAdded);
	return 1;
}

// [1] -> string shipIdStr
static int
luaUqm_state_escort_canAddShips(lua_State *luaState) {
	COUNT shipId;
	int result;

	shipId = testShipId(luaState, 1);
	if (shipId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = EscortFeasibilityStudy(shipId);
	lua_pushinteger(luaState, result);
	return 1;
}

// [1] -> string shipIdStr
// [2] -> int count
static int
luaUqm_state_escort_removeShips(lua_State *luaState) {
	COUNT shipId;
	int numRemoved;

	shipId = testShipId(luaState, 1);
	if (shipId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	if (lua_isnil(luaState, 2)) {
		numRemoved = RemoveEscortShips(shipId);
	} else {
		int count = luaL_checkint(luaState, 2);
		numRemoved = RemoveSomeEscortShips(shipId, count);
	}

	lua_pushinteger(luaState, numRemoved);
	return 1;
}

// [1] -> string shipIdStr
static int
luaUqm_state_escort_shipCount(lua_State *luaState) {
	COUNT shipId;
	int result;

	shipId = testShipId(luaState, 1);
	if (shipId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = CountEscortShips(shipId);
	lua_pushinteger(luaState, result);
	return 1;
}

// No arguments
static int
luaUqm_state_escort_totalValue(lua_State *luaState) {
	COUNT result = CalculateEscortsWorth();
	lua_pushinteger(luaState, result);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// [1] -> string raceIdStr
static int
luaUqm_state_race_isAlive(lua_State *luaState) {
	COUNT raceId;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = (CheckAlliance(raceId) != DEAD_GUY);
	lua_pushboolean(luaState, result);
	return 1;
}

// [1] -> string raceIdStr
static int
luaUqm_state_race_isAllied(lua_State *luaState) {
	COUNT raceId;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = (CheckAlliance(raceId) == GOOD_GUY);
	lua_pushboolean(luaState, result);
	return 1;
}

// [1] -> string raceIdStr
// Note that if a race has no SoI, this function will return false.
static int
luaUqm_state_race_isKnown(lua_State *luaState) {
	COUNT raceId;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = CheckSphereTracking(raceId);
	lua_pushboolean(luaState, result);
	return 1;
}

// [1] -> string raceIdStr
// [2] -> boolean flag
static int
luaUqm_state_race_setAlive(lua_State *luaState) {
	COUNT raceId;
	int flag;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	flag = lua_toboolean(luaState, 2);
	if (flag != 0) {
		log_add(log_Error, "[script] Warning: luaUqm_state_race_setAlive(): "
				"setAlive(true) is not implemented.");
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = KillRace(raceId);
	lua_pushboolean(luaState, result);
	return 1;
}

// [1] -> string raceIdStr
// [2] -> boolean flag
static int
luaUqm_state_race_setAllied(lua_State *luaState) {
	COUNT raceId;
	int flag;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	flag = lua_toboolean(luaState, 2);

	result = SetRaceAllied(raceId, flag);
	lua_pushboolean(luaState, result);
	return 1;
}

// [1] -> string raceIdStr
// [2] -> boolean flag
static int
luaUqm_state_race_setKnown(lua_State *luaState) {
	COUNT raceId;
	int flag;
	BOOLEAN result;

	raceId = testRaceId(luaState, 1);
	if (raceId == (COUNT) -1) {
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	flag = lua_toboolean(luaState, 2);
	if (flag == 0) {
		log_add(log_Error, "[script] Warning: luaUqm_state_race_setKnown(): "
				"setKnown(false) is not implemented.");
		lua_pushboolean(luaState, FALSE);
		return 1;
	}

	result = (StartSphereTracking(raceId) != 0);
	lua_pushboolean(luaState, result);
	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// [1] -> int delta
static int
luaUqm_state_sis_addCrew(lua_State *luaState) {
	int delta;
	COUNT oldCrew;
	COUNT newCrew;
	
	delta = luaL_checkint(luaState, 1);

	oldCrew = GLOBAL_SIS(CrewEnlisted);
	DeltaSISGauges(delta, 0, 0);
	newCrew = GLOBAL_SIS(CrewEnlisted);

	lua_pushinteger(luaState, newCrew - oldCrew);
	return 1;
}

// [1] -> int delta
static int
luaUqm_state_sis_addFuel(lua_State *luaState) {
	int delta;
	COUNT oldFuel;
	COUNT newFuel;
	
	delta = luaL_checkint(luaState, 1);

	oldFuel = GLOBAL_SIS(FuelOnBoard);
	DeltaSISGauges(0, delta, 0);
	newFuel = GLOBAL_SIS(FuelOnBoard);

	lua_pushinteger(luaState, newFuel - oldFuel);
	return 1;
}

// [1] -> int delta
static int
luaUqm_state_sis_addLanders(lua_State *luaState) {
	int delta;
	int oldCount;
	int newCount;
	
	delta = luaL_checkint(luaState, 1);

	oldCount = GLOBAL_SIS(NumLanders);
	newCount = oldCount + delta;
	if (newCount < 0) {
		newCount = 0;
	} else if (newCount > MAX_LANDERS) {
		newCount = MAX_LANDERS;
	}

	if (newCount != oldCount)
	{
		GLOBAL_SIS(NumLanders) = newCount;
		DrawLanders();
	}

	lua_pushinteger(luaState, newCount - oldCount);
	return 1;
}

// [1] -> int delta
static int
luaUqm_state_sis_addResUnits(lua_State *luaState) {
	int delta;
	COUNT oldResUnits;
	COUNT newResUnits;
	
	delta = luaL_checkint(luaState, 1);

	oldResUnits = GLOBAL_SIS(ResUnits);
	DeltaSISGauges(0, delta, 0);
	newResUnits = GLOBAL_SIS(ResUnits);

	lua_pushinteger(luaState, newResUnits - oldResUnits);
	return 1;
}

static int
luaUqm_state_sis_getCaptainName(lua_State *luaState) {
	lua_pushstring(luaState, GLOBAL_SIS (CommanderName));
	return 1;
}

static int
luaUqm_state_sis_getCrew(lua_State *luaState) {
	lua_pushinteger(luaState, GLOBAL_SIS (CrewEnlisted));
	return 1;
}

static int
luaUqm_state_sis_getFuel(lua_State *luaState) {
	lua_pushinteger(luaState, GLOBAL_SIS (FuelOnBoard));
	return 1;
}

static int
luaUqm_state_sis_getResUnits(lua_State *luaState) {
	lua_pushinteger(luaState, GLOBAL_SIS (ResUnits));
	return 1;
}

static int
luaUqm_state_sis_getLanders(lua_State *luaState) {
	lua_pushinteger(luaState, GLOBAL_SIS (NumLanders));
	return 1;
}

static int
luaUqm_state_sis_getShipName(lua_State *luaState) {
	lua_pushstring(luaState, GLOBAL_SIS (ShipName));
	return 1;
}

/////////////////////////////////////////////////////////////////////////////

// [1] -> int name
static int
luaUqm_state_prop_get(lua_State *luaState) {
	luaL_checktype(luaState, 1, LUA_TSTRING);

	luaUqm_getProp(luaState, 1);
	return 1;
}

// [1] -> int name
// [2] -> int|bool|string value
static int
luaUqm_state_prop_set(lua_State *luaState) {
	luaL_checktype(luaState, 1, LUA_TSTRING);
	luaUqm_checkPropValueType(luaState, "state.prop.set", 2);

	luaUqm_setProp(luaState, 1, 2);
	return 0;
}

