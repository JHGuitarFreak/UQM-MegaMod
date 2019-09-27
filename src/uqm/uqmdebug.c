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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include "uqmdebug.h"

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "clock.h"
#include "starmap.h"
#include "intel.h"
#include "sis.h"
#include "status.h"
#include "gamestr.h"
#include "gameev.h"
#include "gendef.h"
#include "globdata.h"
#include "planets/lifeform.h"
#include "planets/scan.h"
#include "races.h"
#include "setup.h"
#include "state.h"
#include "libs/mathlib.h"
#include "lua/luadebug.h"

#include <stdio.h>
#include <errno.h>

void (* volatile debugHook) (void) = NULL;

// Move the Flagship to the destination of the autopilot.
// Should only be called from HyperSpace/QuasiSpace.
// It can be called from debugHook directly after entering HS/QS though.
void
doInstantMove (void)
{
	// Move to the new location:
	if ((GLOBAL (autopilot)).x == ~0 || (GLOBAL (autopilot)).y == ~0)
	{
		// If no destination has been selected, use the current location
		// as the destination.
		(GLOBAL (autopilot)).x = LOGX_TO_UNIVERSE(GLOBAL_SIS (log_x));
		(GLOBAL (autopilot)).y = LOGY_TO_UNIVERSE(GLOBAL_SIS (log_y));
	}
	else
	{
		// A new destination has been selected.
		GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX((GLOBAL (autopilot)).x);
		GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY((GLOBAL (autopilot)).y);
	}

	// Check for a solar systems at the destination.
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		// If there's a solar system at the destination, enter it.
		CurStarDescPtr = FindStar (0, &(GLOBAL (autopilot)), 0, 0);
		if (CurStarDescPtr)
		{
			// Leave HyperSpace/QuasiSpace if we're there:
			SET_GAME_STATE (USED_BROADCASTER, 0);
			GLOBAL (CurrentActivity) &= ~IN_BATTLE;

			// Enter IP:
			GLOBAL (ShipFacing) = 0;
			GLOBAL (ip_planet) = 0;
			GLOBAL (in_orbit) = 0;
					// This causes the ship position in IP to be reset.
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
		}
	}

	// Turn off the autopilot:
	(GLOBAL (autopilot)).x = ~0;
	(GLOBAL (autopilot)).y = ~0;
}

// playerNr should be 0 or 1
STARSHIP*
findPlayerShip (SIZE playerNr)
{
	HELEMENT hElement, hNextElement;

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);
					
		if ((ElementPtr->state_flags & PLAYER_SHIP)	&&
				ElementPtr->playerNr == playerNr)
		{
			STARSHIP *StarShipPtr;
			GetElementStarShip (ElementPtr, &StarShipPtr);
			UnlockElement (hElement);
			return StarShipPtr;
		}
		
		UnlockElement (hElement);
	}
	return NULL;
}

////////////////////////////////////////////////////////////////////////////

void
resetEnergyBattle (void)
{
	STARSHIP *StarShipPtr;
	COUNT delta;
	CONTEXT OldContext;
	
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE) ||
			inHQSpace())
		return;	

	if (PlayerControl[1] & HUMAN_CONTROL){
		StarShipPtr = findPlayerShip (NPC_PLAYER_NUM);
	} else if (PlayerControl[0] & HUMAN_CONTROL) {
		StarShipPtr = findPlayerShip (RPG_PLAYER_NUM);
	} else {
		StarShipPtr = NULL;
	}

	if (StarShipPtr == NULL || StarShipPtr->RaceDescPtr == NULL)
		return;

	delta = StarShipPtr->RaceDescPtr->ship_info.max_energy -
			StarShipPtr->RaceDescPtr->ship_info.energy_level;

	OldContext = SetContext (StatusContext);
	DeltaEnergy (StarShipPtr->hShip, delta);
	SetContext (OldContext);
}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)

static void dumpEventCallback (const EVENT *eventPtr, void *arg);

static void starRecurse (STAR_DESC *star, void *arg);
static void planetRecurse (STAR_DESC *star, SOLARSYS_STATE *system,
		PLANET_DESC *planet, void *arg);
static void moonRecurse (STAR_DESC *star, SOLARSYS_STATE *system,
		PLANET_DESC *planet, PLANET_DESC *moon, void *arg);

static void dumpSystemCallback (const STAR_DESC *star,
		const SOLARSYS_STATE *system, void *arg);
static void dumpPlanetCallback (const PLANET_DESC *planet, void *arg);
static void dumpMoonCallback (const PLANET_DESC *moon, void *arg);
static void dumpWorld (FILE *out, const PLANET_DESC *world);

typedef struct TallyResourcesArg TallyResourcesArg;
static void tallySystemPreCallback (const STAR_DESC *star, const
		SOLARSYS_STATE *system, void *arg);
static void tallySystemPostCallback (const STAR_DESC *star, const
		SOLARSYS_STATE *system, void *arg);
static void tallyPlanetCallback (const PLANET_DESC *planet, void *arg);
static void tallyMoonCallback (const PLANET_DESC *moon, void *arg);
static void tallyResourcesWorld (TallyResourcesArg *arg,
		const PLANET_DESC *world);

static void dumpPlanetTypeCallback (int index, const PlanetFrame *planet,
		void *arg);


BOOLEAN instantMove = FALSE;
BOOLEAN disableInteractivity = FALSE;


// Must be called on the Starcon2Main thread.
// This function is called synchronously wrt the game logic thread.
void
debugKeyPressedSynchronous (void)
{
	// State modifying:
//	equipShip ();
//	giveDevices ();

	// Give the player the ships you can't ally with under normal
	// conditions.
	/*clearEscorts ();
	AddEscortShips (ARILOU_SHIP, 1);
	AddEscortShips (PKUNK_SHIP, 1);
	AddEscortShips (VUX_SHIP, 1);
	AddEscortShips (YEHAT_SHIP, 1);
	AddEscortShips (MELNORME_SHIP, 1);
	AddEscortShips (DRUUGE_SHIP, 1);
	AddEscortShips (ILWRATH_SHIP, 1);
	AddEscortShips (MYCON_SHIP, 1);
	AddEscortShips (SLYLANDRO_SHIP, 1);
	AddEscortShips (UMGAH_SHIP, 1);
	AddEscortShips (URQUAN_SHIP, 1);
	AddEscortShips (BLACK_URQUAN_SHIP, 1);*/

//	resetCrewBattle ();
//	resetEnergyBattle ();
//	instantMove = !instantMove;
	showSpheres ();
//	activateAllShips ();
	forwardToNextEvent (TRUE);
//	SET_GAME_STATE (MELNORME_CREDIT1, 100);
//	GLOBAL_SIS (ResUnits) = 100000;

	// Informational:
//	dumpEvents (stderr);

	// Graphical and textual:
//	debugContexts();
}

// Can be called on any thread, but usually on main()
// This function is called asynchronously wrt the game logic thread,
// which means locking applies. Use carefully.
// TODO: Once game logic thread is purged of graphics and clock locks,
//   this function may not call graphics and game clock functions at all.
void
debugKeyPressed (void)
{
	// Tests
//	Scale_PerfTest ();

	// Informational:
//	dumpStrings (stdout);
//	dumpPlanetTypes(stderr);
//	debugHook = dumpUniverseToFile;
			// This will cause dumpUniverseToFile to be called from the
			// Starcon2Main loop. Calling it from here would give threading
			// problems.
//	debugHook = tallyResourcesToFile;
			// This will cause tallyResourcesToFile to be called from the
			// Starcon2Main loop. Calling it from here would give threading
			// problems.

	// Interactive:
//	uio_debugInteractive(stdin, stdout, stderr);
//	luaUqm_debug_run();
}

////////////////////////////////////////////////////////////////////////////

// Fast forwards to the next event.
// If skipHEE is set, HYPERSPACE_ENCOUNTER_EVENTs are skipped.
// Must be called from the Starcon2Main thread.
// TODO: LockGameClock may be removed since it is only
//   supposed to be called synchronously wrt the game logic thread.
void
forwardToNextEvent (BOOLEAN skipHEE)
{
	HEVENT hEvent;
	EVENT *EventPtr;
	COUNT year, month, day;
			// time of next event
	BOOLEAN done;

	if (!GameClockRunning ())
		return;

	LockGameClock ();

	done = !skipHEE;
	do {
		hEvent = GetHeadEvent ();
		if (hEvent == 0)
			return;
		LockEvent (hEvent, &EventPtr);
		if (EventPtr->func_index != HYPERSPACE_ENCOUNTER_EVENT)
			done = TRUE;
		year = EventPtr->year_index;
		month = EventPtr->month_index;
		day = EventPtr->day_index;
		UnlockEvent (hEvent);

		for (;;) {
			if (GLOBAL (GameClock.year_index) > year ||
					(GLOBAL (GameClock.year_index) == year &&
					(GLOBAL (GameClock.month_index) > month ||
					(GLOBAL (GameClock.month_index) == month &&
					GLOBAL (GameClock.day_index) >= day))))
				break;

			MoveGameClockDays (1);
		}
	} while (!done);

	UnlockGameClock ();
}

const char *
eventName (BYTE func_index)
{
	switch (func_index) {
	case ARILOU_ENTRANCE_EVENT:
		return "ARILOU_ENTRANCE_EVENT";
	case ARILOU_EXIT_EVENT:
		return "ARILOU_EXIT_EVENT";
	case HYPERSPACE_ENCOUNTER_EVENT:
		return "HYPERSPACE_ENCOUNTER_EVENT";
	case KOHR_AH_VICTORIOUS_EVENT:
		return "KOHR_AH_VICTORIOUS_EVENT";
	case ADVANCE_PKUNK_MISSION:
		return "ADVANCE_PKUNK_MISSION";
	case ADVANCE_THRADD_MISSION:
		return "ADVANCE_THRADD_MISSION";
	case ZOQFOT_DISTRESS_EVENT:
		return "ZOQFOT_DISTRESS";
	case ZOQFOT_DEATH_EVENT:
		return "ZOQFOT_DEATH_EVENT";
	case SHOFIXTI_RETURN_EVENT:
		return "SHOFIXTI_RETURN_EVENT";
	case ADVANCE_UTWIG_SUPOX_MISSION:
		return "ADVANCE_UTWIG_SUPOX_MISSION";
	case KOHR_AH_GENOCIDE_EVENT:
		return "KOHR_AH_GENOCIDE_EVENT";
	case SPATHI_SHIELD_EVENT:
		return "SPATHI_SHIELD_EVENT";
	case ADVANCE_ILWRATH_MISSION:
		return "ADVANCE_ILWRATH_MISSION";
	case ADVANCE_MYCON_MISSION:
		return "ADVANCE_MYCON_MISSION";
	case ARILOU_UMGAH_CHECK:
		return "ARILOU_UMGAH_CHECK";
	case YEHAT_REBEL_EVENT:
		return "YEHAT_REBEL_EVENT";
	case SLYLANDRO_RAMP_UP:
		return "SLYLANDRO_RAMP_UP";
	case SLYLANDRO_RAMP_DOWN:
		return "SLYLANDRO_RAMP_DOWN";
	default:
		// Should not happen
		return "???";
	}
}

static void
dumpEventCallback (const EVENT *eventPtr, void *arg)
{
	FILE *out = (FILE *) arg;
	dumpEvent (out, eventPtr);
}

void
dumpEvent (FILE *out, const EVENT *eventPtr)
{
	fprintf (out, "%4u/%02u/%02u: %s\n",
			eventPtr->year_index,
			eventPtr->month_index,
			eventPtr->day_index,
			eventName (eventPtr->func_index));
}

void
dumpEvents (FILE *out)
{
	LockGameClock ();
	ForAllEvents (dumpEventCallback, out);
	UnlockGameClock ();
}

////////////////////////////////////////////////////////////////////////////

// NB: Ship maximum speed and turning rate aren't updated in
// HyperSpace/QuasiSpace or in melee.
void
equipShip (void)
{
	int i;

	// Don't do anything unless in the full game.
	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
		return;

	// Thrusters:
	for (i = 0; i < NUM_DRIVE_SLOTS; i++)
		GLOBAL_SIS (DriveSlots[i]) = FUSION_THRUSTER;

	// Turning jets:
	for (i = 0; i < NUM_JET_SLOTS; i++)
		GLOBAL_SIS (JetSlots[i]) = TURNING_JETS;

	// Shields:
	SET_GAME_STATE (LANDER_SHIELDS,
			(1 << EARTHQUAKE_DISASTER) |
			(1 << BIOLOGICAL_DISASTER) |
			(1 << LIGHTNING_DISASTER) |
			(1 << LAVASPOT_DISASTER));
	// Lander upgrades:
	SET_GAME_STATE (IMPROVED_LANDER_SPEED, 1);
	SET_GAME_STATE (IMPROVED_LANDER_CARGO, 1);
	SET_GAME_STATE (IMPROVED_LANDER_SHOT, 1);

	// Modules:
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) < 2)
	{
		// The Precursor bomb has not been installed.
		// This is the original TFB testing layout.
		i = 0;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = STORAGE_BAY;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = DYNAMO_UNIT;
		GLOBAL_SIS (ModuleSlots[i++]) = TRACKING_SYSTEM;
		GLOBAL_SIS (ModuleSlots[i++]) = TRACKING_SYSTEM;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		
		// Landers:
		GLOBAL_SIS (NumLanders) = MAX_LANDERS;
	}
	else
	{
		// The Precursor bomb has been installed.
		i = NUM_BOMB_MODULES;
		GLOBAL_SIS (ModuleSlots[i++]) = HIGHEFF_FUELSYS;
		GLOBAL_SIS (ModuleSlots[i++]) = CREW_POD;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
		GLOBAL_SIS (ModuleSlots[i++]) = CANNON_WEAPON;
		GLOBAL_SIS (ModuleSlots[i++]) = SHIVA_FURNACE;
	}

	assert (i <= NUM_MODULE_SLOTS);

	// Fill the fuel and crew compartments to the maximum.
	GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;
	GLOBAL_SIS (CrewEnlisted) = 0;
	for (i = 0; i < NUM_MODULE_SLOTS; i++)
	{
		switch (GLOBAL_SIS (ModuleSlots[i])) {
			case CREW_POD:
				GLOBAL_SIS (CrewEnlisted) += CREW_POD_CAPACITY;
				break;
			case FUEL_TANK:
				GLOBAL_SIS (FuelOnBoard) += FUEL_TANK_CAPACITY;
				break;
			case HIGHEFF_FUELSYS:
				GLOBAL_SIS (FuelOnBoard) += HEFUEL_TANK_CAPACITY;
				break;
		}
	}

	// Update the maximum speed and turning rate when in interplanetary.
	if (pSolarSysState != NULL)
	{
		// Thrusters:
		pSolarSysState->max_ship_speed = 5 * IP_SHIP_THRUST_INCREMENT;
		for (i = 0; i < NUM_DRIVE_SLOTS; i++)
			if (GLOBAL_SIS (DriveSlots[i] == FUSION_THRUSTER))
				pSolarSysState->max_ship_speed += IP_SHIP_THRUST_INCREMENT;

		// Turning jets:
		pSolarSysState->turn_wait = IP_SHIP_TURN_WAIT;
		for (i = 0; i < NUM_JET_SLOTS; i++)
			if (GLOBAL_SIS (JetSlots[i]) == TURNING_JETS)
				pSolarSysState->turn_wait -= IP_SHIP_TURN_DECREMENT;
	}

	// Make sure everything is redrawn:
	if (inHQSpace () ||
			LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
	{
		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	}
}

////////////////////////////////////////////////////////////////////////////

void
giveDevices (void) {
	SET_GAME_STATE (ROSY_SPHERE_ON_SHIP, 1);
	SET_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP, 1);
	SET_GAME_STATE (GLOWING_ROD_ON_SHIP, 1);
	SET_GAME_STATE (SUN_DEVICE_ON_SHIP, 1);
	SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 1); 
	SET_GAME_STATE (UTWIG_BOMB, 1);
	SET_GAME_STATE (ULTRON_CONDITION, 1);
	//SET_GAME_STATE (ULTRON_CONDITION, 2);
	//SET_GAME_STATE (ULTRON_CONDITION, 3);
	//SET_GAME_STATE (ULTRON_CONDITION, 4);
	SET_GAME_STATE (MAIDENS_ON_SHIP, 1);
	SET_GAME_STATE (TALKING_PET_ON_SHIP, 1);
	SET_GAME_STATE (AQUA_HELIX_ON_SHIP, 1);
	SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 1);
	SET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP, 1);
	SET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP, 1);
	SET_GAME_STATE (EGG_CASE0_ON_SHIP, 1);
	SET_GAME_STATE (EGG_CASE1_ON_SHIP, 1);
	SET_GAME_STATE (EGG_CASE2_ON_SHIP, 1);
	SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 1);
	SET_GAME_STATE (VUX_BEAST_ON_SHIP, 1);
	SET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP, 1);
	SET_GAME_STATE (PORTAL_KEY_ON_SHIP, 1);
	SET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP, 1);
	SET_GAME_STATE (MOONBASE_ON_SHIP, 1);
	
	// Not strictly a device (although it originally was one).
	SET_GAME_STATE (DESTRUCT_CODE_ON_SHIP, 1);
}

////////////////////////////////////////////////////////////////////////////

void
clearEscorts (void)
{
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		RemoveQueue (&GLOBAL (built_ship_q), hStarShip);
		FreeShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}

	DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
}

////////////////////////////////////////////////////////////////////////////

#if 0
// Not needed anymore, but could be useful in the future.

// Find the HELEMENT belonging to the Flagship.
static HELEMENT
findFlagshipElement (void)
{
	HELEMENT hElement, hNextElement;
	ELEMENT *ElementPtr;

	// Find the ship element.
	for (hElement = GetTailElement (); hElement != 0;
			hElement = hNextElement)
	{
		LockElement (hElement, &ElementPtr);

		if ((ElementPtr->state_flags & PLAYER_SHIP) != 0)
		{
			UnlockElement (hElement);
			return hElement;
		}

		hNextElement = GetPredElement (ElementPtr);
		UnlockElement (hElement);
	}
	return 0;
}
#endif

////////////////////////////////////////////////////////////////////////////

void
showSpheres (void)
{
	HFLEETINFO hStarShip, hNextShip;
	
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip != NULL; hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		if ((FleetPtr->actual_strength != INFINITE_RADIUS) &&
				(FleetPtr->known_strength != FleetPtr->actual_strength))
		{
			FleetPtr->known_strength = FleetPtr->actual_strength;
			FleetPtr->known_loc = FleetPtr->loc;
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}
}

////////////////////////////////////////////////////////////////////////////

void
activateAllShips (void)
{
	HFLEETINFO hStarShip, hNextShip;
	
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip != NULL; hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		if (FleetPtr->icons != NULL)
				// Skip the Ur-Quan probe.
		{
			FleetPtr->allied_state = GOOD_GUY;
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}
}

////////////////////////////////////////////////////////////////////////////

void
forAllStars (void (*callback) (STAR_DESC *, void *), void *arg)
{
	int i;
	extern STAR_DESC starmap_array[];

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		callback (&starmap_array[i], arg);
}

void
forAllPlanets (STAR_DESC *star, SOLARSYS_STATE *system, void (*callback) (
		STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *, void *), void *arg)
{
	COUNT i;

	assert(CurStarDescPtr == star);
	assert(pSolarSysState == system);

	for (i = 0; i < system->SunDesc[0].NumPlanets; i++)
		callback (star, system, &system->PlanetDesc[i], arg);
}

void
forAllMoons (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		void (*callback) (STAR_DESC *, SOLARSYS_STATE *, PLANET_DESC *,
		PLANET_DESC *, void *), void *arg)
{
	COUNT i;

	assert(pSolarSysState == system);

	for (i = 0; i < planet->NumPlanets; i++)
		callback (star, system, planet, &system->MoonDesc[i], arg);
}

////////////////////////////////////////////////////////////////////////////

// Must be called from the Starcon2Main thread.
// TODO: LockGameClock may be removed
void
UniverseRecurse (UniverseRecurseArg *universeRecurseArg)
{
	ACTIVITY savedActivity;
	
	if (universeRecurseArg->systemFuncPre == NULL
			&& universeRecurseArg->systemFuncPost == NULL
			&& universeRecurseArg->planetFuncPre == NULL
			&& universeRecurseArg->planetFuncPost == NULL
			&& universeRecurseArg->moonFunc == NULL)
		return;
	
	LockGameClock ();
	//TFB_DEBUG_HALT = 1;
	savedActivity = GLOBAL (CurrentActivity);
	disableInteractivity = TRUE;

	forAllStars (starRecurse, (void *) universeRecurseArg);
	
	disableInteractivity = FALSE;
	GLOBAL (CurrentActivity) = savedActivity;
	UnlockGameClock ();
}

static void
starRecurse (STAR_DESC *star, void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;

	SOLARSYS_STATE SolarSysState;
	SOLARSYS_STATE *oldPSolarSysState = pSolarSysState;
	STAR_DESC *oldStarDescPtr = CurStarDescPtr;
	CurStarDescPtr = star;

	RandomContext_SeedRandom (SysGenRNG, GetRandomSeedForStar (star));

	memset (&SolarSysState, 0, sizeof (SolarSysState));
	SolarSysState.SunDesc[0].pPrevDesc = 0;
	SolarSysState.SunDesc[0].rand_seed = RandomContext_Random (SysGenRNG);
	SolarSysState.SunDesc[0].data_index = STAR_TYPE (star->Type);
	SolarSysState.SunDesc[0].location.x = 0;
	SolarSysState.SunDesc[0].location.y = 0;
	//SolarSysState.SunDesc[0].radius = MIN_ZOOM_RADIUS;
	SolarSysState.genFuncs = getGenerateFunctions (star->Index);

	pSolarSysState = &SolarSysState;
	(*SolarSysState.genFuncs->generatePlanets) (&SolarSysState);

	if (universeRecurseArg->systemFuncPre != NULL)
	{
		(*universeRecurseArg->systemFuncPre) (
				star, &SolarSysState, universeRecurseArg->arg);
	}
	
	if (universeRecurseArg->planetFuncPre != NULL
			|| universeRecurseArg->planetFuncPost != NULL
			|| universeRecurseArg->moonFunc != NULL)
	{
		forAllPlanets (star, &SolarSysState, planetRecurse,
				(void *) universeRecurseArg);
	}

	if (universeRecurseArg->systemFuncPost != NULL)
	{
		(*universeRecurseArg->systemFuncPost) (
				star, &SolarSysState, universeRecurseArg->arg);
	}
	
	pSolarSysState = oldPSolarSysState;
	CurStarDescPtr = oldStarDescPtr;
}

static void
planetRecurse (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;
	
	assert(CurStarDescPtr == star);
	assert(pSolarSysState == system);

	planet->pPrevDesc = &system->SunDesc[0];

	if (universeRecurseArg->planetFuncPre != NULL)
	{
		system->pOrbitalDesc = planet;
		DoPlanetaryAnalysis (&system->SysInfo, planet);
				// When GenerateDefaultFunctions is used as genFuncs,
				// generateOrbital will also call DoPlanetaryAnalysis,
				// but with other GenerateFunctions this is not guaranteed.
		(*system->genFuncs->generateOrbital) (system, planet);
		(*universeRecurseArg->planetFuncPre) (
				planet, universeRecurseArg->arg);
	}

	if (universeRecurseArg->moonFunc != NULL)
	{
		RandomContext_SeedRandom (SysGenRNG, planet->rand_seed);
		
		(*system->genFuncs->generateMoons) (system, planet);

		forAllMoons (star, system, planet, moonRecurse,
				(void *) universeRecurseArg);
	}
	
	if (universeRecurseArg->planetFuncPost != NULL)
	{
		system->pOrbitalDesc = planet;
		DoPlanetaryAnalysis (&system->SysInfo, planet);
				// When GenerateDefaultFunctions is used as genFuncs,
				// generateOrbital will also call DoPlanetaryAnalysis,
				// but with other GenerateFunctions this is not guaranteed.
		(*system->genFuncs->generateOrbital) (system, planet);
		(*universeRecurseArg->planetFuncPost) (
				planet, universeRecurseArg->arg);
	}
}

static void
moonRecurse (STAR_DESC *star, SOLARSYS_STATE *system, PLANET_DESC *planet,
		PLANET_DESC *moon, void *arg)
{
	UniverseRecurseArg *universeRecurseArg = (UniverseRecurseArg *) arg;
	
	assert(CurStarDescPtr == star);
	assert(pSolarSysState == system);
	
	moon->pPrevDesc = planet;

	if (universeRecurseArg->moonFunc != NULL)
	{
		system->pOrbitalDesc = moon;
		if (moon->data_index != HIERARCHY_STARBASE 
			&& moon->data_index != SA_MATRA 
			&& moon->data_index != DESTROYED_STARBASE
			&& moon->data_index != PRECURSOR_STARBASE)
		{
			DoPlanetaryAnalysis (&system->SysInfo, moon);
				// When GenerateDefaultFunctions is used as genFuncs,
				// generateOrbital will also call DoPlanetaryAnalysis,
				// but with other GenerateFunctions this is not guaranteed.
		}
		(*system->genFuncs->generateOrbital) (system, moon);
		(*universeRecurseArg->moonFunc) (
				moon, universeRecurseArg->arg);
	}
}

////////////////////////////////////////////////////////////////////////////

typedef struct
{
	FILE *out;
} DumpUniverseArg;

// Must be called from the Starcon2Main thread.
void
dumpUniverse (FILE *out)
{
	DumpUniverseArg dumpUniverseArg;
	UniverseRecurseArg universeRecurseArg;
	
	dumpUniverseArg.out = out;

	universeRecurseArg.systemFuncPre = dumpSystemCallback;
	universeRecurseArg.systemFuncPost = NULL;
	universeRecurseArg.planetFuncPre = dumpPlanetCallback;
	universeRecurseArg.planetFuncPost = NULL;
	universeRecurseArg.moonFunc = dumpMoonCallback;
	universeRecurseArg.arg = (void *) &dumpUniverseArg;

	UniverseRecurse (&universeRecurseArg);
}

// Must be called from the Starcon2Main thread.
void
dumpUniverseToFile (void)
{
	FILE *out;

#	define UNIVERSE_DUMP_FILE "PlanetInfo"
	out = fopen(UNIVERSE_DUMP_FILE, "w");
	if (out == NULL)
	{
		fprintf(stderr, "Error: Could not open file '%s' for "
				"writing: %s\n", UNIVERSE_DUMP_FILE, strerror(errno));
		return;
	}

	dumpUniverse (out);
	
	fclose(out);

	fprintf(stdout, "*** Star dump complete. The game may be in an "
			"undefined state.\n");
			// Data generation may have changed the game state,
			// in particular for special planet generation.
}

static void
dumpSystemCallback (const STAR_DESC *star, const SOLARSYS_STATE *system,
		void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpSystem (out, star, system);
}

void
dumpSystem (FILE *out, const STAR_DESC *star, const SOLARSYS_STATE *system)
{
	UNICODE name[256];
	UNICODE buf[40];

	GetClusterName (star, name);
	snprintf (buf, sizeof buf, "%s %s",
			bodyColorString (STAR_COLOR(star->Type)),
			starTypeString (STAR_TYPE(star->Type)));
	fprintf (out, "%-22s  (%3d.%1d, %3d.%1d) %-19s  %s\n",
			name,
			star->star_pt.x / 10, star->star_pt.x % 10,
			star->star_pt.y / 10, star->star_pt.y % 10,
			buf,
			starPresenceString (star->Index));

	(void) system;  /* satisfy compiler */
}

const char *
bodyColorString (BYTE col)
{
	switch (col) {
		case BLUE_BODY:
			return "blue";
		case GREEN_BODY:
			return "green";
		case ORANGE_BODY:
			return "orange";
		case RED_BODY:
			return "red";
		case WHITE_BODY:
			return "white";
		case YELLOW_BODY:
			return "yellow";
		case CYAN_BODY:
			return "cyan";
		case PURPLE_BODY:
			return "purple";
		case VIOLET_BODY:
			return "violet";
		default:
			// Should not happen
			return "???";
	}
}

const char *
starTypeString (BYTE type)
{
	switch (type) {
		case DWARF_STAR:
			return "dwarf";
		case GIANT_STAR:
			return "giant";
		case SUPER_GIANT_STAR:
			return "super giant";
		default:
			// Should not happen
			return "???";
	}
}

const char *
starPresenceString (BYTE index)
{
	switch (index) {
		case 0:
			// nothing
			return "";
		case SOL_DEFINED:
			return "Sol";
		case SHOFIXTI_DEFINED:
			return "Shofixti male";
		case MAIDENS_DEFINED:
			return "Shofixti maidens";
		case START_COLONY_DEFINED:
			return "Starting colony";
		case SPATHI_DEFINED:
			return "Spathi home";
		case ZOQFOT_DEFINED:
			return "Zoq-Fot-Pik home";
		case MELNORME0_DEFINED:
			return "Melnorme trader #0";
		case MELNORME1_DEFINED:
			return "Melnorme trader #1";
		case MELNORME2_DEFINED:
			return "Melnorme trader #2";
		case MELNORME3_DEFINED:
			return "Melnorme trader #3";
		case MELNORME4_DEFINED:
			return "Melnorme trader #4";
		case MELNORME5_DEFINED:
			return "Melnorme trader #5";
		case MELNORME6_DEFINED:
			return "Melnorme trader #6";
		case MELNORME7_DEFINED:
			return "Melnorme trader #7";
		case MELNORME8_DEFINED:
			return "Melnorme trader #8";
		case TALKING_PET_DEFINED:
			return "Talking Pet";
		case CHMMR_DEFINED:
			return "Chmmr home";
		case SYREEN_DEFINED:
			return "Syreen home";
		case BURVIXESE_DEFINED:
			return "Burvixese ruins";
		case SLYLANDRO_DEFINED:
			return "Slylandro home";
		case DRUUGE_DEFINED:
			return "Druuge home";
		case BOMB_DEFINED:
			return "Precursor bomb";
		case AQUA_HELIX_DEFINED:
			return "Aqua Helix";
		case SUN_DEVICE_DEFINED:
			return "Sun Device";
		case TAALO_PROTECTOR_DEFINED:
			return "Taalo Shield";
		case SHIP_VAULT_DEFINED:
			return "Syreen ship vault";
		case URQUAN_WRECK_DEFINED:
			return "Ur-Quan ship wreck";
		case VUX_BEAST_DEFINED:
			return "Zex' beauty";
		case SAMATRA_DEFINED:
			return "Sa-Matra";
		case ZOQ_SCOUT_DEFINED:
			return "Zoq-Fot-Pik scout";
		case MYCON_DEFINED:
			return "Mycon home";
		case EGG_CASE0_DEFINED:
			return "Mycon egg shell #0";
		case EGG_CASE1_DEFINED:
			return "Mycon egg shell #1";
		case EGG_CASE2_DEFINED:
			return "Mycon egg shell #2";
		case PKUNK_DEFINED:
			return "Pkunk home";
		case UTWIG_DEFINED:
			return "Utwig home";
		case SUPOX_DEFINED:
			return "Supox home";
		case YEHAT_DEFINED:
			return "Yehat home";
		case VUX_DEFINED:
			return "Vux home";
		case ORZ_DEFINED:
			return "Orz home";
		case THRADD_DEFINED:
			return "Thraddash home";
		case RAINBOW_DEFINED:
			return "Rainbow world";
		case ILWRATH_DEFINED:
			return "Ilwrath home";
		case ANDROSYNTH_DEFINED:
			return "Androsynth ruins";
		case MYCON_TRAP_DEFINED:
			return "Mycon trap";
		case URQUAN_DEFINED:
		case KOHRAH_DEFINED:
		case DESTROYED_STARBASE_DEFINED:
			return "Destroyed Starbase";
		case MOTHER_ARK_DEFINED:
			return "Mother-Ark";
		case ALGOLITES_DEFINED:
			return "Algolites";
		default:
			// Should not happen
			return "???";
	}
}

static void
dumpPlanetCallback (const PLANET_DESC *planet, void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpPlanet (out, planet);
}

void
dumpPlanet (FILE *out, const PLANET_DESC *planet)
{
	(*pSolarSysState->genFuncs->generateName) (pSolarSysState, planet);
	fprintf (out, "- %-37s  %s\n", GLOBAL_SIS (PlanetName),
			planetTypeString (planet->data_index & ~PLANET_SHIELDED));
	dumpWorld (out, planet);
}

static void
dumpMoonCallback (const PLANET_DESC *moon, void *arg)
{
	FILE *out = ((DumpUniverseArg *) arg)->out;
	dumpMoon (out, moon);
}

void
dumpMoon (FILE *out, const PLANET_DESC *moon)
{
	const char *typeStr;
	
	if (moon->data_index == HIERARCHY_STARBASE)
	{
		typeStr = "StarBase";
	}
	else if (moon->data_index == SA_MATRA)
	{
		typeStr = "Sa-Matra";
	}
	else if (moon->data_index == DESTROYED_STARBASE)
	{
		typeStr = "Destroyed StarBase";
	}
	else if (moon->data_index == PRECURSOR_STARBASE)
	{
		typeStr = "Precursor StarBase";
	}
	else
	{
		typeStr = planetTypeString (moon->data_index & ~PLANET_SHIELDED);
	}
	fprintf (out, "  - Moon %-30c  %s\n",
			'a' + (moon - &pSolarSysState->MoonDesc[0]), typeStr);

	dumpWorld (out, moon);
}

static void
dumpWorld (FILE *out, const PLANET_DESC *world)
{
	PLANET_INFO *info;
	
	if (world->data_index == HIERARCHY_STARBASE) {
		return;
	}
	
	if (world->data_index == SA_MATRA) {
		return;
	}

	if (world->data_index == DESTROYED_STARBASE) {
		return;
	}

	if (world->data_index == PRECURSOR_STARBASE) {
		return;
	}

	info = &pSolarSysState->SysInfo.PlanetInfo;
	fprintf(out, "          AxialTilt:  %d\n", info->AxialTilt);
	fprintf(out, "          Tectonics:  %d\n", info->Tectonics);
	fprintf(out, "          Weather:    %d\n", info->Weather);
	fprintf(out, "          Density:    %d\n", info->PlanetDensity);
	fprintf(out, "          Radius:     %d\n", info->PlanetRadius);
	fprintf(out, "          Gravity:    %d\n", info->SurfaceGravity);
	fprintf(out, "          Temp:       %d\n", info->SurfaceTemperature);
	fprintf(out, "          Day:        %d\n", info->RotationPeriod);
	fprintf(out, "          Atmosphere: %d\n", info->AtmoDensity);
	fprintf(out, "          LifeChance: %d\n", info->LifeChance);
	fprintf(out, "          DistToSun:  %d\n", info->PlanetToSunDist);

	if (world->data_index & PLANET_SHIELDED) {
		// Slave-shielded planet
		return;
	}

	fprintf (out, "          Bio: %4d    Min: %4d\n",
			calculateBioValue (pSolarSysState, world),
			calculateMineralValue (pSolarSysState, world));
}

COUNT
calculateBioValue (const SOLARSYS_STATE *system, const PLANET_DESC *world)
{
	COUNT result;
	COUNT numBio;
	COUNT i;

	assert (system->pOrbitalDesc == world);
	
	numBio = callGenerateForScanType (system, world, GENERATE_ALL,
			BIOLOGICAL_SCAN, NULL);

	result = 0;
	for (i = 0; i < numBio; i++)
	{
		NODE_INFO info;
		callGenerateForScanType (system, world, i, BIOLOGICAL_SCAN, &info);
		result += BIO_CREDIT_VALUE *
				LONIBBLE (CreatureData[info.type].ValueAndHitPoints);
	}
	return result;
}

void
generateBioIndex(const SOLARSYS_STATE *system, const PLANET_DESC *world,
		COUNT bio[])
{
	COUNT numBio;
	COUNT i;

	assert (system->pOrbitalDesc == world);
	
	numBio = callGenerateForScanType (system, world, GENERATE_ALL,
			BIOLOGICAL_SCAN, NULL);

	for (i = 0; i < NUM_CREATURE_TYPES + NUM_SPECIAL_CREATURE_TYPES; i++)
		bio[i] = 0;
	
	for (i = 0; i < numBio; i++)
	{
		NODE_INFO info;
		callGenerateForScanType (system, world, i, BIOLOGICAL_SCAN, &info);
		bio[info.type]++;
	}
}

COUNT
calculateMineralValue (const SOLARSYS_STATE *system, const PLANET_DESC *world)
{
	COUNT result;
	COUNT numDeposits;
	COUNT i;

	assert (system->pOrbitalDesc == world);
	
	numDeposits = callGenerateForScanType (system, world, GENERATE_ALL,
			MINERAL_SCAN, NULL);

	result = 0;
	for (i = 0; i < numDeposits; i++)
	{
		NODE_INFO info;
		callGenerateForScanType (system, world, i, MINERAL_SCAN, &info);
		result += HIBYTE (info.density) *
				GLOBAL (ElementWorth[ElementCategory (info.type)]);
	}
	return result;
}

void
generateMineralIndex(const SOLARSYS_STATE *system, const PLANET_DESC *world,
		COUNT minerals[])
{
	COUNT numDeposits;
	COUNT i;

	assert (system->pOrbitalDesc == world);
	
	numDeposits = callGenerateForScanType (system, world, GENERATE_ALL,
			MINERAL_SCAN, NULL);

	for (i = 0; i < NUM_ELEMENT_CATEGORIES; i++)
		minerals[i] = 0;
	
	for (i = 0; i < numDeposits; i++)
	{
		NODE_INFO info;
		callGenerateForScanType (system, world, i, MINERAL_SCAN, &info);
		minerals[ElementCategory (info.type)] += HIBYTE (info.density);
	}
}

////////////////////////////////////////////////////////////////////////////

struct TallyResourcesArg
{
	FILE *out;
	COUNT mineralCount;
	COUNT bioCount;
};

// Must be called from the Starcon2Main thread.
void
tallyResources (FILE *out)
{
	TallyResourcesArg tallyResourcesArg;
	UniverseRecurseArg universeRecurseArg;
	
	tallyResourcesArg.out = out;

	universeRecurseArg.systemFuncPre = tallySystemPreCallback;
	universeRecurseArg.systemFuncPost = tallySystemPostCallback;
	universeRecurseArg.planetFuncPre = tallyPlanetCallback;
	universeRecurseArg.planetFuncPost = NULL;
	universeRecurseArg.moonFunc = tallyMoonCallback;
	universeRecurseArg.arg = (void *) &tallyResourcesArg;

	UniverseRecurse (&universeRecurseArg);
}

// Must be called from the Starcon2Main thread.
void
tallyResourcesToFile (void)
{
	FILE *out;

#	define RESOURCE_TALLY_FILE "ResourceTally"
	out = fopen(RESOURCE_TALLY_FILE, "w");
	if (out == NULL)
	{
		fprintf(stderr, "Error: Could not open file '%s' for "
				"writing: %s\n", RESOURCE_TALLY_FILE, strerror(errno));
		return;
	}

	tallyResources (out);
	
	fclose(out);

	fprintf(stdout, "*** Resource tally complete. The game may be in an "
			"undefined state.\n");
			// Data generation may have changed the game state,
			// in particular for special planet generation.
}

static void
tallySystemPreCallback (const STAR_DESC *star, const SOLARSYS_STATE *system,
		void *arg)
{
	TallyResourcesArg *tallyResourcesArg = (TallyResourcesArg *) arg;
	tallyResourcesArg->mineralCount = 0;
	tallyResourcesArg->bioCount = 0;
	
	(void) star;  /* satisfy compiler */
	(void) system;  /* satisfy compiler */
}

static void
tallySystemPostCallback (const STAR_DESC *star, const SOLARSYS_STATE *system,
		void *arg)
{
	UNICODE name[256];
	TallyResourcesArg *tallyResourcesArg = (TallyResourcesArg *) arg;
	FILE *out = tallyResourcesArg->out;

	GetClusterName (star, name);
	fprintf (out, "%s\t%d\t%d\n", name, tallyResourcesArg->mineralCount,
			tallyResourcesArg->bioCount);

	(void) star;  /* satisfy compiler */
	(void) system;  /* satisfy compiler */
}

static void
tallyPlanetCallback (const PLANET_DESC *planet, void *arg)
{
	tallyResourcesWorld ((TallyResourcesArg *) arg, planet);
}

static void
tallyMoonCallback (const PLANET_DESC *moon, void *arg)
{
	tallyResourcesWorld ((TallyResourcesArg *) arg, moon);
}

static void
tallyResourcesWorld (TallyResourcesArg *arg, const PLANET_DESC *world)
{
	if (world->data_index == HIERARCHY_STARBASE) {
		return;
	}
	
	if (world->data_index == SA_MATRA) {
		return;
	}

	if (world->data_index == DESTROYED_STARBASE) {
		return;
	}

	if (world->data_index == PRECURSOR_STARBASE) {
		return;
	}

	arg->bioCount += calculateBioValue (pSolarSysState, world),
	arg->mineralCount += calculateMineralValue (pSolarSysState, world);
}

////////////////////////////////////////////////////////////////////////////

void
forAllPlanetTypes (void (*callback) (int, const PlanetFrame *, void *),
		void *arg)
{
	int i;
	extern const PlanetFrame planet_array[];

	for (i = 0; i < NUMBER_OF_PLANET_TYPES; i++)
		callback (i, &planet_array[i], arg);
}
	
typedef struct
{
	FILE *out;
} DumpPlanetTypesArg;

void
dumpPlanetTypes (FILE *out)
{
	DumpPlanetTypesArg dumpPlanetTypesArg;
	dumpPlanetTypesArg.out = out;

	forAllPlanetTypes (dumpPlanetTypeCallback, (void *) &dumpPlanetTypesArg);
}

static void
dumpPlanetTypeCallback (int index, const PlanetFrame *planetType, void *arg)
{
	DumpPlanetTypesArg *dumpPlanetTypesArg = (DumpPlanetTypesArg *) arg;

	dumpPlanetType(dumpPlanetTypesArg->out, index, planetType);
}

void
dumpPlanetType (FILE *out, int index, const PlanetFrame *planetType)
{
	int i;
	fprintf (out,
			"%s\n"
			"\tType: %s\n"
			"\tColor: %s\n"
			"\tSurface generation algoritm: %s\n"
			"\tTectonics: %s\n"
			"\tAtmosphere: %s\n"
			"\tDensity: %s\n"
			"\tElements:\n",
			planetTypeString (index),
			worldSizeString (PLANSIZE (planetType->Type)),
			bodyColorString (PLANCOLOR (planetType->Type)),
			worldGenAlgoString (PLANALGO (planetType->Type)),
			tectonicsString (planetType->BaseTectonics),
			atmosphereString (HINIBBLE (planetType->AtmoAndDensity)),
			densityString (LONIBBLE (planetType->AtmoAndDensity))
			);
	for (i = 0; i < NUM_USEFUL_ELEMENTS; i++)
	{
		const ELEMENT_ENTRY *entry;
		entry = &planetType->UsefulElements[i];
		if (entry->Density == 0)
			continue;
		fprintf(out, "\t\t0 to %d %s-quality (+%d) deposits of %s (%s)\n",
				DEPOSIT_QUANTITY (entry->Density),
				depositQualityString (DEPOSIT_QUALITY (entry->Density)),
				DEPOSIT_QUALITY (entry->Density) * 5,
				GAME_STRING (ELEMENTS_STRING_BASE + entry->ElementType),
				GAME_STRING (CARGO_STRING_BASE + 2 + ElementCategory (
				entry->ElementType))
			);
	}
	fprintf (out, "\n");
}

const char *
planetTypeString (int typeIndex)
{
	static UNICODE typeStr[40];

	if (typeIndex >= FIRST_GAS_GIANT)
	{
		// "Gas Giant"
		snprintf(typeStr, sizeof typeStr, "%s",
				GAME_STRING (SCAN_STRING_BASE + 4 + 51));
	}
	else
	{
		// "<type> World" (eg. "Water World")
		snprintf(typeStr, sizeof typeStr, "%s %s",
				GAME_STRING (SCAN_STRING_BASE + 4 + typeIndex),
				GAME_STRING (SCAN_STRING_BASE + 4 + 50));
	}
	return typeStr;
}

// size is what you get from PLANSIZE (planetFrame.Type)
const char *
worldSizeString (BYTE size)
{
	switch (size)
	{
		case SMALL_ROCKY_WORLD:
			return "small rocky world";
		case LARGE_ROCKY_WORLD:
			return "large rocky world";
		case GAS_GIANT:
			return "gas giant";
		default:
			// Should not happen
			return "???";
	}
}

// algo is what you get from PLANALGO (planetFrame.Type)
const char *
worldGenAlgoString (BYTE algo)
{
	switch (algo)
	{
		case TOPO_ALGO:
			return "TOPO_ALGO";
		case CRATERED_ALGO:
			return "CRATERED_ALGO";
		case GAS_GIANT_ALGO:
			return "GAS_GIANT_ALGO";
		default:
			// Should not happen
			return "???";
	}
}

// tectonics is what you get from planetFrame.BaseTechtonics
// not reentrant
const char *
tectonicsString (BYTE tectonics)
{
	static char buf[sizeof "-127"];
	switch (tectonics)
	{
		case NO_TECTONICS:
			return "none";
		case LOW_TECTONICS:
			return "low";
		case MED_TECTONICS:
			return "medium";
		case HIGH_TECTONICS:
			return "high";
		case SUPER_TECTONICS:
			return "super";
		default:
			snprintf (buf, sizeof buf, "%d", tectonics);
			return buf;
	}
}

// atmosphere is what you get from HINIBBLE (planetFrame.AtmoAndDensity)
const char *
atmosphereString (BYTE atmosphere)
{
	switch (atmosphere)
	{
		case LIGHT:
			return "thin";
		case MEDIUM:
			return "normal";
		case HEAVY:
			return "thick";
		default:
			return "super thick";
	}
}

// density is what you get from LONIBBLE (planetFrame.AtmoAndDensity)
const char *
densityString (BYTE density)
{
	switch (density)
	{
		case GAS_DENSITY:
			return "gaseous";
		case LIGHT_DENSITY:
			return "light";
		case LOW_DENSITY:
			return "low";
		case NORMAL_DENSITY:
			return "normal";
		case HIGH_DENSITY:
			return "high";
		case SUPER_DENSITY:
			return "super high";
		default:
			// Should not happen
			return "???";
	}
}

// quality is what you get from DEPOSIT_QUALITY (elementEntry.Density)
const char *
depositQualityString (BYTE quality)
{
	switch (quality)
	{
		case LIGHT:
			return "low";
		case MEDIUM:
			return "medium";
		case HEAVY:
			return "high";
		default:
			// Should not happen
			return "???";
	}
}

////////////////////////////////////////////////////////////////////////////

void
resetCrewBattle (void)
{
	STARSHIP *StarShipPtr;
	COUNT delta;
	CONTEXT OldContext;
	
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE) ||
			(inHQSpace ()))
		return;
	
	StarShipPtr = findPlayerShip (RPG_PLAYER_NUM);
	if (StarShipPtr == NULL || StarShipPtr->RaceDescPtr == NULL)
		return;

	delta = StarShipPtr->RaceDescPtr->ship_info.max_crew -
			StarShipPtr->RaceDescPtr->ship_info.crew_level;

	OldContext = SetContext (StatusContext);
	DeltaCrew (StarShipPtr->hShip, delta);
	SetContext (OldContext);
}

////////////////////////////////////////////////////////////////////////////

// This function should help in making sure that gamestr.h matches
// gamestrings.txt.
void
dumpStrings (FILE *out)
{
#define STRINGIZE(a) #a
#define MAKE_STRING_CATEGORY(prefix) \
		{ \
			/* .name  = */ STRINGIZE(prefix ## _BASE), \
			/* .base  = */ prefix ## _BASE, \
			/* .count = */ prefix ## _COUNT \
		}
	struct {
		const char *name;
		size_t base;
		size_t count;
	} categories[] = {
		{ "0", 0, 0 },
		MAKE_STRING_CATEGORY(STAR_STRING),
		MAKE_STRING_CATEGORY(DEVICE_STRING),
		MAKE_STRING_CATEGORY(CARGO_STRING),
		MAKE_STRING_CATEGORY(ELEMENTS_STRING),
		MAKE_STRING_CATEGORY(SCAN_STRING),
		MAKE_STRING_CATEGORY(STAR_NUMBER),
		MAKE_STRING_CATEGORY(PLANET_NUMBER),
		MAKE_STRING_CATEGORY(MONTHS_STRING),
		MAKE_STRING_CATEGORY(FEEDBACK_STRING),
		MAKE_STRING_CATEGORY(STARBASE_STRING),
		MAKE_STRING_CATEGORY(ENCOUNTER_STRING),
		MAKE_STRING_CATEGORY(NAVIGATION_STRING),
		MAKE_STRING_CATEGORY(NAMING_STRING),
		MAKE_STRING_CATEGORY(MELEE_STRING),
		MAKE_STRING_CATEGORY(SAVEGAME_STRING),
		MAKE_STRING_CATEGORY(OPTION_STRING),
		MAKE_STRING_CATEGORY(QUITMENU_STRING),
		MAKE_STRING_CATEGORY(STATUS_STRING),
		MAKE_STRING_CATEGORY(FLAGSHIP_STRING),
		MAKE_STRING_CATEGORY(ORBITSCAN_STRING),
		MAKE_STRING_CATEGORY(MAINMENU_STRING),
		MAKE_STRING_CATEGORY(NETMELEE_STRING),
		{ "GAMESTR_COUNT", GAMESTR_COUNT, (size_t) -1 }
	};
	size_t numCategories = sizeof categories / sizeof categories[0];
	size_t numStrings = GetStringTableCount (GameStrings);
	size_t stringI;
	size_t categoryI = 0;

	// Start with a sanity check to see if gamestr.h has been changed but
	// not this file.
	for (categoryI = 0; categoryI < numCategories - 1; categoryI++) {
		if (categories[categoryI].base + categories[categoryI].count !=
				categories[categoryI + 1].base) {
			fprintf(stderr, "Error: String category list in dumpStrings() is "
					"not up to date.\n");
			return;
		}
	}
	
	if (GAMESTR_COUNT != numStrings) {
		fprintf(stderr, "Warning: GAMESTR_COUNT is %d, but GameStrings "
				"contains %d strings.\n", GAMESTR_COUNT, numStrings);
	}

	categoryI = 0;
	for (stringI = 0; stringI < numStrings; stringI++) {
		while (categoryI < numCategories &&
				stringI >= categories[categoryI + 1].base)
			categoryI++;
		fprintf(out, "[ %s + %d ]  %s\n", categories[categoryI].name,
				stringI - categories[categoryI].base, GAME_STRING(stringI));
	}
}

////////////////////////////////////////////////////////////////////////////


static Color
hsvaToRgba (double hue, double sat, double val, BYTE alpha)
{
	unsigned int hi = (int) (hue / 60.0);
	double f = (hue / 60.0) - ((int) (hue / 60.0));
	double p = val * (1.0 - sat);
	double q = val * (1.0 - f * sat);
	double t = val * (1.0 - (1.0 - f * sat));

	// Convert p, q, t, and v from [0..1] to [0..255]
	BYTE pb = (BYTE) (p * 255.0 + 0.5);
	BYTE qb = (BYTE) (q * 255.0 + 0.5);
	BYTE tb = (BYTE) (t * 255.0 + 0.5);
	BYTE vb = (BYTE) (val * 255.0 + 0.5);

	assert (hue >= 0.0 && hue < 360.0);
	assert (sat >= 0 && sat <= 1.0);
	assert (val >= 0 && val <= 1.0);
	/*fprintf(stderr, "hsva = (%.1f, %.2f, %.2f, %.2d)\n",
			hue, sat, val, alpha);*/
	
	assert (hi < 6);
	switch (hi) {
		case 0: return BUILD_COLOR_RGBA (vb, tb, pb, alpha);
		case 1: return BUILD_COLOR_RGBA (qb, vb, pb, alpha);
		case 2: return BUILD_COLOR_RGBA (pb, vb, tb, alpha);
		case 3: return BUILD_COLOR_RGBA (pb, qb, vb, alpha);
		case 4: return BUILD_COLOR_RGBA (tb, pb, vb, alpha);
		case 5: return BUILD_COLOR_RGBA (vb, pb, qb, alpha);
	}

	// Should not happen.
	return BUILD_COLOR_RGBA (0, 0, 0, alpha);
}

// Returns true iff this context has a visible FRAME.
static bool
isContextVisible (CONTEXT context)
{
	FRAME contextFrame;

	// Save the original context.
	CONTEXT oldContext = SetContext (context);
	
	// Get the frame of the specified context.
	contextFrame = GetContextFGFrame ();

	// Restore the original context.
	SetContext (oldContext);

	return contextFrame == Screen;
}

static size_t
countVisibleContexts (void)
{
	size_t contextCount;
	CONTEXT context;

	contextCount = 0;
	for (context = GetFirstContext (); context != NULL;
			context = GetNextContext (context))
	{
		if (!isContextVisible (context))
			continue;
		
		contextCount++;
	}

	return contextCount;
}

static void
drawContext (CONTEXT context, double hue /* no pun intended */)
{
	FRAME drawFrame;
	CONTEXT oldContext;
	FONT oldFont;
	DrawMode oldMode;
	Color oldFgCol;
	Color rectCol;
	Color lineCol;
	Color textCol;
	bool haveClippingRect;
	RECT rect;
	LINE line;
	TEXT text;
	POINT p1, p2, p3, p4;

	drawFrame = GetContextFGFrame ();
	rectCol = hsvaToRgba (hue, 1.0, 0.5, 100);
	lineCol = hsvaToRgba (hue, 1.0, 1.0, 90);
	textCol = lineCol;

	// Save the original context.
	oldContext = SetContext (context);
	
	// Get the clipping rectangle of the specified context.
	haveClippingRect = GetContextClipRect (&rect);

	// Switch back the old context; we're going to draw in it.
	(void) SetContext (oldContext);

	p1 = rect.corner;
	p2.x = rect.corner.x + rect.extent.width - 1;
	p2.y = rect.corner.y;
	p3.x = rect.corner.x;
	p3.y = rect.corner.y + rect.extent.height - 1;
	p4.x = rect.corner.x + rect.extent.width - 1;
	p4.y = rect.corner.y + rect.extent.height - 1;

	oldFgCol = SetContextForeGroundColor (rectCol);
	DrawFilledRectangle (&rect);

	SetContextForeGroundColor (lineCol);
	line.first = p1; line.second = p2; DrawLine (&line);
	line.first = p2; line.second = p4; DrawLine (&line);
	line.first = p1; line.second = p3; DrawLine (&line);
	line.first = p3; line.second = p4; DrawLine (&line);
	line.first = p1; line.second = p4; DrawLine (&line);
	line.first = p2; line.second = p3; DrawLine (&line);
	// Gimme C'99! So I can do:
	//     DrawLine ((LINE) { .first = p1, .second = p2 })

	oldFont = SetContextFont (TinyFont);
	SetContextForeGroundColor (textCol);
	// Text prim does not yet support alpha via Color.a
	oldMode = SetContextDrawMode (MAKE_DRAW_MODE (DRAW_ALPHA, textCol.a));
	text.baseline.x = (p1.x + (p2.x + 1)) / 2;
	text.baseline.y = p1.y + 8;
	text.pStr = GetContextName (context);
	text.align = ALIGN_CENTER;
	text.CharCount = (COUNT) ~0;
	font_DrawText (&text);
	(void) SetContextDrawMode (oldMode);

	(void) SetContextForeGroundColor (oldFgCol);
	(void) SetContextFont (oldFont);
}

static void
describeContext (FILE *out, const CONTEXT context) {
	RECT rect;
	CONTEXT oldContext = SetContext (context);
	
	GetContextClipRect (&rect);
	fprintf(out, "Context '%s':\n"
			"\tClipRect = (%d, %d)-(%d, %d)  (%d x %d)\n",
		   GetContextName (context),
		   rect.corner.x, rect.corner.y,
		   rect.corner.x + rect.extent.width,
		   rect.corner.y + rect.extent.height,
		   rect.extent.width, rect.extent.height);
	
	SetContext (oldContext);
}


typedef struct wait_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct wait_state *self);
} WAIT_STATE;

		
// Maybe move to elsewhere, where it can be reused?
static BOOLEAN
waitForKey (struct wait_state *self) {
	if (PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
		return FALSE;

	SleepThread (ONE_SECOND / 20);
	
	(void) self;
	return TRUE;
}

// Maybe move to elsewhere, where it can be reused?
static FRAME
getScreen (void)
{
	CONTEXT oldContext = SetContext (ScreenContext);
	FRAME savedFrame;
	RECT screenRect;

	screenRect.corner.x = 0;
	screenRect.corner.y = 0;
	screenRect.extent.width = ScreenWidth;
	screenRect.extent.height = ScreenHeight;
	savedFrame = CaptureDrawable (LoadDisplayPixmap (&screenRect, (FRAME) 0));

	(void) SetContext (oldContext);
	return savedFrame;
}

static void
putScreen (FRAME savedFrame) {
	STAMP stamp;
	
	CONTEXT oldContext = SetContext (ScreenContext);

	stamp.origin.x = 0;
	stamp.origin.y = 0;
	stamp.frame = savedFrame;
	DrawStamp (&stamp);

	(void) SetContext (oldContext);
}

// Show the contexts on the screen.
// Must be called from the main thread.
void
debugContexts (void)
{
	static volatile bool inDebugContexts = false;
			// Prevent this function from being called from within itself.
	
	CONTEXT orgContext;
	CONTEXT debugDrawContext;
			// We're going to use this context to draw in.
	FRAME debugDrawFrame;
	double hueIncrement;
	size_t visibleContextI;
	CONTEXT context;
	size_t contextCount;
	FRAME savedScreen;

	// Prevent this function from being called from within itself.
	if (inDebugContexts)
		return;
	inDebugContexts = true;

	contextCount = countVisibleContexts ();
	if (contextCount == 0)
	{
		goto out;
	}
	
	savedScreen = getScreen ();
	FlushGraphics ();
			// Make sure that the screen has actually been captured,
			// before we use the frame.

	// Create a new frame to draw on.
	debugDrawContext = CreateContext ("debugDrawContext");
	// New work frame is a copy of the original.
	debugDrawFrame = CaptureDrawable (CloneFrame (savedScreen));
	orgContext = SetContext (debugDrawContext);
	SetContextFGFrame (debugDrawFrame);

	hueIncrement = 360.0 / contextCount;

	visibleContextI = 0;
	for (context = GetFirstContext (); context != NULL;
			context = GetNextContext (context))
	{
		if (context == debugDrawContext) {
			// Skip our own context.
			continue;
		}
	
		if (isContextVisible (context))
		{
			// Only draw the visible contexts.
			drawContext (context, visibleContextI * hueIncrement);
			visibleContextI++;
		}

		describeContext (stderr, context);
	}

	// Blit the final debugging frame to the screen.
	putScreen (debugDrawFrame);

	// Wait for a key:
	{
		WAIT_STATE state;
		state.InputFunc = waitForKey;
		DoInput(&state, TRUE);
	}

	SetContext (orgContext);

	// Destroy the debugging frame and context.
	DestroyContext (debugDrawContext);
			// This does nothing with the drawable set with
			// SetContextFGFrame().
	DestroyDrawable (ReleaseDrawable (debugDrawFrame));
	
	putScreen (savedScreen);

	DestroyDrawable (ReleaseDrawable (savedScreen));

out:
	inDebugContexts = false;
}

#endif  /* DEBUG */

