//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#include "build.h"
#include "options.h"
#include "races.h"
#include "master.h"
#include "sis.h"
#include "setup.h"
#include "libs/compiler.h"
#include "libs/mathlib.h"
#include "planets/planets.h"
#include <stdlib.h>


// Allocate a new STARSHIP or SHIP_FRAGMENT and put it in the queue
HLINK
Build (QUEUE *pQueue, SPECIES_ID SpeciesID)
{
	HLINK hNewShip;
	SHIP_BASE *ShipPtr;

	assert (GetLinkSize (pQueue) == sizeof (STARSHIP) ||
			GetLinkSize (pQueue) == sizeof (SHIP_FRAGMENT));

	hNewShip = AllocLink (pQueue);
	if (!hNewShip)
		return 0;

	ShipPtr = (SHIP_BASE *) LockLink (pQueue, hNewShip);
	memset (ShipPtr, 0, GetLinkSize (pQueue));
	ShipPtr->SpeciesID = SpeciesID;

	UnlockLink (pQueue, hNewShip);
	PutQueue (pQueue, hNewShip);

	return hNewShip;
}

HLINK
GetStarShipFromIndex (QUEUE *pShipQ, COUNT Index)
{
	HLINK hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (pShipQ);
			Index > 0 && hStarShip; hStarShip = hNextShip, --Index)
	{
		LINK *StarShipPtr;

		StarShipPtr = LockLink (pShipQ, hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockLink (pShipQ, hStarShip);
	}

	return (hStarShip);
}

HSHIPFRAG
GetEscortByStarShipIndex (COUNT index)
{
	HSHIPFRAG hStarShip;
	HSHIPFRAG hNextShip;
	SHIP_FRAGMENT *StarShipPtr;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		if (StarShipPtr->index == index)
		{
			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			break;
		}

		hNextShip = _GetSuccLink (StarShipPtr);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}

	return hStarShip;
}

SPECIES_ID
ShipIdStrToIndex (const char *shipIdStr)
{
	HMASTERSHIP hStarShip;
	HMASTERSHIP hNextShip;
	SPECIES_ID result = NO_ID;

	for (hStarShip = GetHeadLink (&master_q);
			hStarShip != 0; hStarShip = hNextShip)
	{
		MASTER_SHIP_INFO *MasterPtr;

		MasterPtr = LockMasterShip (&master_q, hStarShip);
		hNextShip = _GetSuccLink (MasterPtr);

		if (strcmp (shipIdStr, MasterPtr->ShipInfo.idStr) == 0)
		{
			result = MasterPtr->SpeciesID;
			UnlockMasterShip (&master_q, hStarShip);
			break;
		}

		UnlockMasterShip (&master_q, hStarShip);
	}

	return result;
}

typedef struct {
	const char *idStr;
	RACE_ID id;
} RaceIdMap;

// We would eventually want to unhardcode this.
static RaceIdMap raceIdMap[] = {
	// Sorted on the name, for the binary search.
	{ /* .idStr = */ "arilou",      /* .id = */ ARILOU_SHIP },
	{ /* .idStr = */ "chmmr",       /* .id = */ CHMMR_SHIP },
	{ /* .idStr = */ "druuge",      /* .id = */ DRUUGE_SHIP },
	{ /* .idStr = */ "human",       /* .id = */ HUMAN_SHIP },
	{ /* .idStr = */ "ilwrath",     /* .id = */ ILWRATH_SHIP },
	{ /* .idStr = */ "kohrah",      /* .id = */ BLACK_URQUAN_SHIP },
	{ /* .idStr = */ "melnorme",    /* .id = */ MELNORME_SHIP },
	{ /* .idStr = */ "mycon",       /* .id = */ MYCON_SHIP },
	{ /* .idStr = */ "orz",         /* .id = */ ORZ_SHIP },
	{ /* .idStr = */ "pkunk",       /* .id = */ PKUNK_SHIP },
	{ /* .idStr = */ "samatra",     /* .id = */ SAMATRA_SHIP },
	{ /* .idStr = */ "shofixti",    /* .id = */ SHOFIXTI_SHIP },
	{ /* .idStr = */ "slylandro",   /* .id = */ SLYLANDRO_SHIP },
	{ /* .idStr = */ "spathi",      /* .id = */ SPATHI_SHIP },
	{ /* .idStr = */ "supox",       /* .id = */ SUPOX_SHIP },
	{ /* .idStr = */ "syreen",      /* .id = */ SYREEN_SHIP },
	{ /* .idStr = */ "thraddash",   /* .id = */ THRADDASH_SHIP },
	{ /* .idStr = */ "umgah",       /* .id = */ UMGAH_SHIP },
	{ /* .idStr = */ "urquandrone", /* .id = */ URQUAN_DRONE_SHIP },
	{ /* .idStr = */ "urquan",      /* .id = */ URQUAN_SHIP },
	{ /* .idStr = */ "utwig",       /* .id = */ UTWIG_SHIP },
	{ /* .idStr = */ "vux",         /* .id = */ VUX_SHIP },
	{ /* .idStr = */ "yehat",       /* .id = */ YEHAT_SHIP },
	{ /* .idStr = */ "yehatrebel",  /* .id = */ YEHAT_REBEL_SHIP },
	{ /* .idStr = */ "zoqfotpik",   /* .id = */ ZOQFOTPIK_SHIP },
			// Same as URQUAN_DRONE_SHIP
};

static int
RaceIdCompare (const void *id1, const void *id2)
{
	return strcmp (((RaceIdMap *) id1)->idStr, ((RaceIdMap *) id2)->idStr);
}

RACE_ID
RaceIdStrToIndex (const char *raceIdStr)
{
	RaceIdMap key = { /* .idStr = */ raceIdStr, /* .id = */ -1 };
	RaceIdMap *found = bsearch (&key, raceIdMap,
			sizeof raceIdMap / sizeof raceIdMap[0],
			sizeof raceIdMap[0], RaceIdCompare);

	if (found == NULL)
		return (RACE_ID) -1;

	return found->id;
}

/*
 * Give the player 'count' ships of the specified race,
 * limited by the number of free slots.
 * Returns the number of ships added.
 */
COUNT
AddEscortShips (RACE_ID race, SIZE count)
{
	HFLEETINFO hFleet;
	BYTE which_window;
	COUNT i;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	assert (count > 0);

	which_window = 0;
	for (i = 0; i < (COUNT) count; i++)
	{
		HSHIPFRAG hStarShip;
		HSHIPFRAG hOldShip;
		SHIP_FRAGMENT *StarShipPtr;

		hStarShip = CloneShipFragment (race, &GLOBAL (built_ship_q), 0);
		if (!hStarShip)
			break;

		RemoveQueue (&GLOBAL (built_ship_q), hStarShip);

		/* Find first available escort window */
		while ((hOldShip = GetStarShipFromIndex (
				&GLOBAL (built_ship_q), which_window++)))
		{
			BYTE win_loc;

			StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hOldShip);
			win_loc = StarShipPtr->index;
			UnlockShipFrag (&GLOBAL (built_ship_q), hOldShip);
			if (which_window <= win_loc)
				break;
		}

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		StarShipPtr->index = which_window - 1;
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		InsertQueue (&GLOBAL (built_ship_q), hStarShip, hOldShip);
	}

	DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	return i;
}

/*
 * Returns the total value of all the ships escorting the SIS.
 */
COUNT
CalculateEscortsWorth (void)
{
	COUNT ShipCost[] =
	{
		RACE_SHIP_COST
	};
	COUNT total = 0;
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		total += ShipCost[StarShipPtr->race_id];
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	return total;
}

#if 0
/*
 * Returns the size of the fleet of the specified race when the starmap was
 * last checked. If the race has no SoI, 0 is returned.
 */
COUNT
GetRaceKnownSize (RACE_ID race)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;
	COUNT result;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	result = FleetPtr->known_strength;

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return result;
}
#endif

/*
 * Start or end an alliance with the specified race.
 * Being in an alliance with a race makes their ships available for building
 * in the shipyard.
 * flag == TRUE: start an alliance
 * flag == TRUE: end an alliance
 */
BOOLEAN
SetRaceAllied (RACE_ID race, BOOLEAN flag) {
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return FALSE;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	if (FleetPtr->allied_state == DEAD_GUY)
	{
		/* Strange request, silently ignore it */
	}
	else
	{
		FleetPtr->allied_state = (flag ? GOOD_GUY : BAD_GUY);
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return TRUE;
}

/*
 * Allows the building of ships regardless of alliance state
 * flag == TRUE: Allow to build ship
 * flag == FALSE: Normal, not allowed to build ships if not allied.
 */
BOOLEAN
SetRaceAllowBuild (RACE_ID race) {
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return FALSE;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	if (FleetPtr->allied_state != GOOD_GUY) {
		FleetPtr->allied_state = CAN_BUILD;
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return TRUE;
}

/*
 * 	Make the sphere of influence for the specified race shown on the starmap
 * 	in the future.
 * 	Does nothing for races without a SoI, or for races which have an
 * 	infinite SoI.
 * 	The value returned is 'race', unless the type of ship is only available
 * 	in SuperMelee, in which case 0 is returned.
 */
COUNT
StartSphereTracking (RACE_ID race)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	if (FleetPtr->actual_strength == 0)
	{
		// Race has no Sphere of Influence.
		if (FleetPtr->allied_state == DEAD_GUY)
		{
			// Race is extinct.
			UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
			return 0;
		}
	}
	else if (FleetPtr->known_strength == 0
			&& FleetPtr->actual_strength != INFINITE_RADIUS)
	{
		FleetPtr->known_strength = 1;
		FleetPtr->known_loc = FleetPtr->loc;
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return race;
}

/*
 * 	Check whether we are tracking the SoI of a race.
 * 	If a race has no SoI, this function will always return false.
 */
BOOLEAN
CheckSphereTracking (RACE_ID race)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;
	COUNT result;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return FALSE;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	if (FleetPtr->actual_strength == 0) {
		// Race has no Sphere of Influence.
		// Maybe it never had one, or maybe the race is extinct.
		result = FALSE;
	}
	else
	{
		result = (FleetPtr->known_strength > 0);
	}

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return result;
}

BOOLEAN
KillRace (RACE_ID race)
{
	HFLEETINFO hFleet;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return FALSE;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	FleetPtr->allied_state = DEAD_GUY;
	FleetPtr->actual_strength = 0;

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return TRUE;
}

/*
 * Returns the number of ships of the specified race among the
 * escort ships.
 */
COUNT
CountEscortShips (RACE_ID race)
{
	HFLEETINFO hFleet;
	HSHIPFRAG hStarShip, hNextShip;
	COUNT result = 0;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)); hStarShip;
			hStarShip = hNextShip)
	{
		BYTE ship_type;
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		ship_type = StarShipPtr->race_id;
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		if (ship_type == race)
			result++;
	}
	return result;
}

/*
 * Returns true if and only if a ship of the specified race is among the
 * escort ships.
 */
BOOLEAN
HaveEscortShip (RACE_ID race)
{
	return (CountEscortShips (race) > 0);
}

/*
 * Test if the SIS can have an escort of the specified race.
 * Returns 0 if 'race' is not available.
 * Otherwise, returns the number of ships that can be added.
 */
COUNT
EscortFeasibilityStudy (RACE_ID race)
{
	HFLEETINFO hFleet;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	return (MAX_BUILT_SHIPS - CountLinks (&GLOBAL (built_ship_q)));
}

/*
 * Test the alliance status of the specified race.
 * Either DEAD_GUY (extinct), GOOD_GUY (allied), or BAD_GUY (not allied) is
 * returned.
 */
COUNT
CheckAlliance (RACE_ID race)
{
	HFLEETINFO hFleet;
	UWORD flags;
	FLEET_INFO *FleetPtr;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), race);
	if (!hFleet)
		return 0;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	flags = FleetPtr->allied_state;
	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	return flags;
}

/*
 * Remove a number of escort ships of the specified race (if present).
 * Returns the number of escort ships removed.
 */
COUNT
RemoveSomeEscortShips (RACE_ID race, COUNT count)
{
	HSHIPFRAG hStarShip;
	HSHIPFRAG hNextShip;

	if (count == 0)
		return 0;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)); hStarShip;
			hStarShip = hNextShip)
	{
		BOOLEAN RemoveShip;
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		RemoveShip = (StarShipPtr->race_id == race);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		if (RemoveShip)
		{
			RemoveQueue (&GLOBAL (built_ship_q), hStarShip);
			FreeShipFrag (&GLOBAL (built_ship_q), hStarShip);
			count--;
			if (count == 0)
				break;
		}
	}
	
	if (count > 0)
	{
		// Update the display.
		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
	}

	return count;
}

/*
 * Remove all escort ships of the specified race.
 */
COUNT
RemoveEscortShips (RACE_ID race)
{
	return RemoveSomeEscortShips (race, (COUNT) -1);
}

COUNT
GetIndexFromStarShip (QUEUE *pShipQ, HLINK hStarShip)
{
	COUNT Index;

	Index = 0;
	while (hStarShip != GetHeadLink (pShipQ))
	{
		HLINK hNextShip;
		LINK *StarShipPtr;

		StarShipPtr = LockLink (pShipQ, hStarShip);
		hNextShip = _GetPredLink (StarShipPtr);
		UnlockLink (pShipQ, hStarShip);

		hStarShip = hNextShip;
		++Index;
	}

	return Index;
}

BYTE
NameCaptain (QUEUE *pQueue, SPECIES_ID SpeciesID)
{
	BYTE name_index;
	HLINK hStarShip;

	assert (GetLinkSize (pQueue) == sizeof (STARSHIP) ||
			GetLinkSize (pQueue) == sizeof (SHIP_FRAGMENT));

	do
	{
		HLINK hNextShip;

		name_index = PickCaptainName ();
		for (hStarShip = GetHeadLink (pQueue); hStarShip;
				hStarShip = hNextShip)
		{
			SHIP_BASE *ShipPtr;
			BYTE test_name_index = -1;

			ShipPtr = (SHIP_BASE *) LockLink (pQueue, hStarShip);
			hNextShip = _GetSuccLink (ShipPtr);
			if (ShipPtr->SpeciesID == SpeciesID)
				test_name_index = ShipPtr->captains_name_index;
			UnlockLink (pQueue, hStarShip);
			
			if (name_index == test_name_index)
				break;
		}
	} while (hStarShip /* name matched another ship */);

	return name_index;
}

// crew_level can be set to INFINITE_FLEET for a ship which is to
// represent an infinite number of ships.
HSHIPFRAG
CloneShipFragment (RACE_ID shipIndex, QUEUE *pDstQueue, COUNT crew_level)
{
	HFLEETINFO hFleet;
	HSHIPFRAG hBuiltShip;
	FLEET_INFO *TemplatePtr;
	BYTE captains_name_index;

	assert (GetLinkSize (pDstQueue) == sizeof (SHIP_FRAGMENT));

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), shipIndex);
	if (!hFleet)
		return 0;

	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	if (shipIndex == SAMATRA_SHIP)
		captains_name_index = 0;
	else
		captains_name_index = NameCaptain (pDstQueue,
				TemplatePtr->SpeciesID);
	hBuiltShip = Build (pDstQueue, TemplatePtr->SpeciesID);
	if (hBuiltShip)
	{
		SHIP_FRAGMENT *ShipFragPtr;

		ShipFragPtr = LockShipFrag (pDstQueue, hBuiltShip);
		ShipFragPtr->captains_name_index = captains_name_index;
		ShipFragPtr->race_strings = TemplatePtr->race_strings;
		ShipFragPtr->icons = TemplatePtr->icons;
		ShipFragPtr->melee_icon = TemplatePtr->melee_icon;
		if (crew_level)
			ShipFragPtr->crew_level = crew_level;
		else
			ShipFragPtr->crew_level = TemplatePtr->crew_level;
		ShipFragPtr->max_crew = TemplatePtr->max_crew;
		ShipFragPtr->energy_level = 0;
		ShipFragPtr->max_energy = TemplatePtr->max_energy;
		ShipFragPtr->race_id = (BYTE)shipIndex;
		ShipFragPtr->index = 0;
		UnlockShipFrag (pDstQueue, hBuiltShip);
	}
	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	return hBuiltShip;
}

/* Set the crew and captain's name on the first fully-crewed escort
 * ship of race 'which_ship' */
int
SetEscortCrewComplement (RACE_ID which_ship, COUNT crew_level, BYTE captain)
{
	HFLEETINFO hFleet;
	FLEET_INFO *TemplatePtr;
	HSHIPFRAG hStarShip, hNextShip;
	SHIP_FRAGMENT *StarShipPtr = 0;
	int Index;

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), which_ship);
	if (!hFleet)
		return -1;
	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);

	/* Find first ship of which_ship race */
	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q)), Index = 0;
			hStarShip; hStarShip = hNextShip, ++Index)
	{
		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		if (which_ship == StarShipPtr->race_id &&
				StarShipPtr->crew_level == TemplatePtr->crew_level)
			break; /* found one */
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	if (hStarShip)
	{
		StarShipPtr->crew_level = crew_level;
		StarShipPtr->captains_name_index = captain;
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	else
		Index = -1;

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return Index;
}

void
loadGameCheats (void){
	if(optInfiniteRU){
		oldRU = GlobData.SIS_state.ResUnits;
	} else {
		oldRU = 0;
	}
	if(optInfiniteFuel){
		loadFuel = GlobData.SIS_state.FuelOnBoard;
		GLOBAL_SIS (FuelOnBoard) = GetFuelTankCapacity();
	} else {
		loadFuel = 0;
	}
	if (optUnlockShips){
		BYTE i = 0;

		for (i = ARILOU_SHIP; i <= BLACK_URQUAN_SHIP; ++i) {
			SetRaceAllowBuild(i);
		}
	}
	if (optUnlockUpgrades){
		SET_GAME_STATE (IMPROVED_LANDER_SPEED, 1);
		SET_GAME_STATE (IMPROVED_LANDER_CARGO, 1);
		SET_GAME_STATE (IMPROVED_LANDER_SHOT, 1);
		SET_GAME_STATE (LANDER_SHIELDS, (1 << EARTHQUAKE_DISASTER) | (1 << BIOLOGICAL_DISASTER) |
			(1 << LIGHTNING_DISASTER) | (1 << LAVASPOT_DISASTER));				
		GLOBAL (ModuleCost[ANTIMISSILE_DEFENSE]) = 4000 / MODULE_COST_SCALE;				
		GLOBAL (ModuleCost[BLASTER_WEAPON]) = 4000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[HIGHEFF_FUELSYS]) = 1000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[TRACKING_SYSTEM]) = 5000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[CANNON_WEAPON]) = 6000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[SHIVA_FURNACE]) = 4000 / MODULE_COST_SCALE;
		//SET_GAME_STATE (MELNORME_TECH_STACK, 13);
	}
	if(optAddDevices){		
		SET_GAME_STATE (ROSY_SPHERE_ON_SHIP, 1);
		SET_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP, 1);
		SET_GAME_STATE (GLOWING_ROD_ON_SHIP, 1);
		SET_GAME_STATE (SUN_DEVICE_ON_SHIP, 1);
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 1);
		SET_GAME_STATE (ULTRON_CONDITION, 1);
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
		SET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP, 1);
		SET_GAME_STATE (DESTRUCT_CODE_ON_SHIP, 1);
	}
}

