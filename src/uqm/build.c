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
#include "starbase.h"
#include "starmap.h"
#include "gendef.h"
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
	{ /* .idStr = */ "androsynth",  /* .id = */ ANDROSYNTH_SHIP },
	{ /* .idStr = */ "arilou",      /* .id = */ ARILOU_SHIP },
	{ /* .idStr = */ "chenjesu",    /* .id = */ CHENJESU_SHIP },
	{ /* .idStr = */ "chmmr",       /* .id = */ CHMMR_SHIP },
	{ /* .idStr = */ "druuge",      /* .id = */ DRUUGE_SHIP },
	{ /* .idStr = */ "human",       /* .id = */ HUMAN_SHIP },
	{ /* .idStr = */ "ilwrath",     /* .id = */ ILWRATH_SHIP },
	{ /* .idStr = */ "kohrah",      /* .id = */ BLACK_URQUAN_SHIP },
	{ /* .idStr = */ "melnorme",    /* .id = */ MELNORME_SHIP },
	{ /* .idStr = */ "mmrnmhrm",    /* .id = */ MMRNMHRM_SHIP },
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
			ARRAY_SIZE (raceIdMap),
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
	if (!hFleet || count <= 0)
		return 0;

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
	COUNT total = 0;
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		total += ShipCost (StarShipPtr->race_id);
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
 * flag == FALSE: end an alliance
 */
BOOLEAN
SetRaceAllied (RACE_ID race, BOOLEAN flag)
{
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
loadGameCheats (void)
{
	// JSD TODO: look at the following for plotmap
	if (EXTENDED && star_array[63].Index == MELNORME0_DEFINED)
	{
		star_array[63].Type =
				MAKE_STAR (SUPER_GIANT_STAR, ORANGE_BODY, -1);
	}

	if(optInfiniteRU)
		oldRU = GlobData.SIS_state.ResUnits;
	else
		oldRU = 0;

	/*for (BYTE i = ARILOU_SHIP; i <= MMRNMHRM_SHIP; ++i)
	{
		StartSphereTracking (i);
		KillRace (i);
	}*/

	// SET_GAME_STATE (KNOW_HOMEWORLD, ~0);
		
	if(optInfiniteFuel)
	{
		loadFuel = GlobData.SIS_state.FuelOnBoard;
		GLOBAL_SIS (FuelOnBoard) = GetFuelTankCapacity();
	} 
	else
		loadFuel = 0;
	
	if (optUnlockUpgrades)
	{
		SET_GAME_STATE (IMPROVED_LANDER_SPEED, 1);
		SET_GAME_STATE (IMPROVED_LANDER_CARGO, 1);
		SET_GAME_STATE (IMPROVED_LANDER_SHOT, 1);
		SET_GAME_STATE (LANDER_SHIELDS,
				(1 << EARTHQUAKE_DISASTER) | (1 << BIOLOGICAL_DISASTER)
				| (1 << LIGHTNING_DISASTER) | (1 << LAVASPOT_DISASTER));
		GLOBAL (ModuleCost[ANTIMISSILE_DEFENSE]) = 4000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[BLASTER_WEAPON]) = 4000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[HIGHEFF_FUELSYS]) = 1000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[TRACKING_SYSTEM]) = 5000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[CANNON_WEAPON]) = 6000 / MODULE_COST_SCALE;
		GLOBAL (ModuleCost[SHIVA_FURNACE]) = 4000 / MODULE_COST_SCALE;
		//SET_GAME_STATE (MELNORME_TECH_STACK, 13);
	}
	
	if(optAddDevices)
	{
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

		if (GET_GAME_STATE(KNOW_ABOUT_SHATTERED) < 2)
			SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 2);
		SET_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED, 1);

		SET_GAME_STATE (SHIP_VAULT_UNLOCKED, 1);
		SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 0);
		SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
	}
}

// Jitter returns a distance between 0..66.6% of the fleet's actual strength,
// weighted towards 20% median value.  We can adjust the jitter by changing
// the fraction at the end.
// (COUNT) sqrt (rand_val) gives a value 0..255 which leans heavy towards 255
// so subtract from 255 to receive a weighted towards zero jitter.
COUNT Jitter (FLEET_INFO *FleetPtr, UWORD rand_val)
{
	return (FleetPtr->actual_strength * (SPHERE_RADIUS_INCREMENT / 2) *
			(255 - (COUNT) sqrt (rand_val)) / 256) * 2 / 3;
}

void JitDebug (FLEET_INFO *FleetPtr, UWORD rand_val_x, UWORD rand_val_y, char race[])
{
	fprintf(stderr, "Fleet %d (%s); Actual Str %d; Rand x %d (%d); Rand y %d (%d) \n",
			FleetPtr->SpeciesID, race, FleetPtr->actual_strength,
			rand_val_x, (255 - (COUNT) sqrt (rand_val_x)),
			rand_val_y, (255 - (COUNT) sqrt (rand_val_y)));
	fprintf(stderr, "        Jitter X %d%% Jitter Y %d%% Jitter X %d (%d), Jitter Y %d (%d)\n",
			(255 - (COUNT) sqrt (rand_val_x)) * 100 / 256,
			(255 - (COUNT) sqrt (rand_val_y)) * 100 / 256,
			FleetPtr->actual_strength * (SPHERE_RADIUS_INCREMENT / 2) *
				(COUNT) (255 - (COUNT) sqrt (rand_val_x)) / 256,
			Jitter (FleetPtr, rand_val_x),
			FleetPtr->actual_strength * (SPHERE_RADIUS_INCREMENT / 2) *
				(COUNT) (255 - (COUNT) sqrt (rand_val_y)) / 256,
			Jitter (FleetPtr, rand_val_y));
}

// JSD SeedFleet sets the fleet referenced in the FLEET_INFO pointer passed
// to the coordinates of that fleet's plot on the PLOT_LOCATION array passed.
// Essentially this function is the map between a fleet/race ID and the plot IDs.
void SeedFleet (FLEET_INFO *FleetPtr, PLOT_LOCATION *plotmap)
{
	if (!(FleetPtr) || !(plotmap))
	{
		fprintf(stderr, "SeedFleet called with NULL PTR(s).\n");
		return;
	}
	POINT location = SeedFleetLocation (FleetPtr, plotmap, 0);
	FleetPtr->known_loc = location;
	return;
}

// JSD SeedFleetLocation return the center for the SOI of the race listed,
// or if visit plot ID nonzero another value as dictated by the
// interaction of the two (usually jitter baesd on visit plot ID's location).
// Yes this means no arilou visit fleet movements.
POINT SeedFleetLocation (FLEET_INFO *FleetPtr, PLOT_LOCATION *plotmap, COUNT visit)
{
	UWORD rand_val_x, rand_val_y;
	COUNT home;		// Plot ID of the homeworld for the fleet
	POINT destination;
	if (!StarGenRNG)
	{
		fprintf(stderr, "****Creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New ();
		RandomContext_SeedRandom (StarGenRNG, 123456);
	}

	switch (FleetPtr->SpeciesID)
	{
		case ARILOU_ID:
			// Arilou don't have either a homeworld or jitter.  Set and go home.
			home = ARILOU_DEFINED;
			return plotmap[ARILOU_DEFINED].star_pt;
		case CHMMR_ID:
			home = CHMMR_DEFINED;
			break;
		case EARTHLING_ID:
			home = SOL_DEFINED;
			break;
		case ORZ_ID:
			home = ORZ_DEFINED;
			break;
		case PKUNK_ID:
			home = PKUNK_DEFINED;
			break;
		case SHOFIXTI_ID:
			home = SHOFIXTI_DEFINED;
			break;
		case SPATHI_ID:
			home = SPATHI_DEFINED;
			break;
		case SUPOX_ID:
			home = SUPOX_DEFINED;
			break;
		case THRADDASH_ID:
			home = THRADD_DEFINED;
			break;
		case UTWIG_ID:
			home = UTWIG_DEFINED;
			break;
		case VUX_ID:
			home = VUX_DEFINED;
			break;
		case YEHAT_ID:
			home = YEHAT_DEFINED;
			break;
		case MELNORME_ID:
			// There's more than one of these but it doesn't matter since they
			// don'y seed any fleets.
			home = MELNORME0_DEFINED;
			break;
		case DRUUGE_ID:
			home = DRUUGE_DEFINED;
			break;
		case ILWRATH_ID:
			home = ILWRATH_DEFINED;
			break;
		case MYCON_ID:
			home = MYCON_DEFINED;
			break;
		case SLYLANDRO_ID:
			home = SLYLANDRO_DEFINED;
			break;
		case UMGAH_ID:
			home = TALKING_PET_DEFINED;
			break;
		case UR_QUAN_ID:
			home = SAMATRA_DEFINED;
			break;
		case ZOQFOTPIK_ID:
			home = ZOQFOT_DEFINED;
			break;
		case SYREEN_ID:
			// For the historical map, however, we will need to swap
			// out the plot ID to EGG_CASE0_DEFINED
			home = SYREEN_DEFINED;
			break;
		case KOHR_AH_ID:
			home = SAMATRA_DEFINED;
			break;
		case ANDROSYNTH_ID:
			home = ANDROSYNTH_DEFINED;
			break;
		case CHENJESU_ID:
			home = CHMMR_DEFINED;
			break;
		case MMRNMHRM_ID:
			home = MOTHER_ARK_DEFINED;
			break;
		default:
			fprintf(stderr, "SeedFleet called with bad species ID %d.\n", FleetPtr->SpeciesID);
		   	return (POINT) {0, 0};
	}
	// Arilou have to be resolved before here
	if (!(plotmap[home].star))
	{
		fprintf(stderr, "SeedFleet called, but home plot ID %d has a NULL star pointer.\n", home);
		return (POINT) {0, 0};
	}
	if (visit >= NUM_PLOTS)
	{
		fprintf(stderr, "SeedFleet called with invalid away plot ID %d\n", visit);
		return (POINT) {0, 0};
	}
	if (!(plotmap[home].star))
	{
		fprintf(stderr, "SeedFleet called, but away plot ID %d has a NULL star pointer.\n", visit);
		return (POINT) {0, 0};
	}
	RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar (plotmap[home].star));
	rand_val_x = RandomContext_Random (StarGenRNG);
	rand_val_y = RandomContext_Random (StarGenRNG);
	if ((visit > 0) && (visit != SAMATRA_DEFINED))
	{
		RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar (plotmap[visit].star));
		rand_val_x += RandomContext_Random (StarGenRNG) % sizeof (DWORD);
		rand_val_y += RandomContext_Random (StarGenRNG) % sizeof (DWORD);
	}

	switch (FleetPtr->SpeciesID)
	{
		case ARILOU_ID: // This shouldn't be possible
			return plotmap[ARILOU_DEFINED].star_pt;
			break;
		case CHMMR_ID:
			return plotmap[CHMMR_DEFINED].star_pt;
			break;
		case EARTHLING_ID:
			return plotmap[SOL_DEFINED].star_pt;
			break;
		case ORZ_ID: // Jitter towards < the playground
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Orz");
			destination = (POINT) {plotmap[home].star_pt.x +
					((plotmap[home].star_pt.x - plotmap[TAALO_PROTECTOR_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[home].star_pt.y +
					((plotmap[home].star_pt.y - plotmap[TAALO_PROTECTOR_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case PKUNK_ID: // Jitter away > from Ilwrath
			if (visit == YEHAT_DEFINED)
			{
				// Jitter from Yehat space towards Pkunk
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "Pkunk at Yehat space");
				destination = (POINT) {plotmap[visit].star_pt.x +
						((plotmap[visit].star_pt.x - plotmap[home].star_pt.x < 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						plotmap[visit].star_pt.y +
						((plotmap[visit].star_pt.y - plotmap[home].star_pt.y < 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Pkunk");
			destination = (POINT) {plotmap[home].star_pt.x +
					((plotmap[home].star_pt.x - plotmap[ILWRATH_DEFINED].star_pt.x > 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[home].star_pt.y +
					((plotmap[home].star_pt.y - plotmap[ILWRATH_DEFINED].star_pt.y > 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			//FleetPtr->known_loc.x = plotmap[PKUNK_DEFINED].star_pt.x +
			//		((plotmap[PKUNK_DEFINED].star_pt.x - plotmap[ILWRATH_DEFINED].star_pt.x > 0) ? 1 : -1) *
			//		Jitter (FleetPtr, rand_val_x);
			//FleetPtr->known_loc.y = plotmap[PKUNK_DEFINED].star_pt.y +
			//		((plotmap[PKUNK_DEFINED].star_pt.y - plotmap[ILWRATH_DEFINED].star_pt.y > 0) ? 1 : -1) *
			//		Jitter (FleetPtr, rand_val_y);
			break;
		case SHOFIXTI_ID:
			return plotmap[SHOFIXTI_DEFINED].star_pt;
			break;
		case SPATHI_ID: // Jitter at random (use %2)
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Spathi");
			destination = (POINT) {plotmap[SPATHI_DEFINED].star_pt.x +
					(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[SPATHI_DEFINED].star_pt.y +
					(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			//FleetPtr->known_loc.x = plotmap[SPATHI_DEFINED].star_pt.x +
			//		(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
			//		Jitter (FleetPtr, rand_val_x);
			//FleetPtr->known_loc.y = plotmap[SPATHI_DEFINED].star_pt.y +
			//		(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
			//		Jitter (FleetPtr, rand_val_y);
			break;
		case SUPOX_ID: // Jitter towards Utwig
			if (visit == SAMATRA_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "Supox coming at the UQ/KA zone");
				// Contrary to most other fleet movements, I don't want them jittered from the
				// SAMATRA based on their own radii.  Pick a spot nearby and normal jitter that.
				// First, imagine a coord near SAMATRA that Utwig will also use.
				RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar (plotmap[visit].star));
				UWORD rand_val_s = RandomContext_Random (StarGenRNG);
				POINT samatra = {plotmap[visit].star_pt.x + HIBYTE (rand_val_s) - 128,
						plotmap[visit].star_pt.y + LOBYTE (rand_val_s) - 128};
				COUNT warpath = sqrt ((plotmap[home].star_pt.x - samatra.x) *
						(plotmap[home].star_pt.x - samatra.x) +
						(plotmap[home].star_pt.y - samatra.y) *
						(plotmap[home].star_pt.y - samatra.y));
				fprintf(stderr, "***** WARPATH center %d.%d : %d.%d\n",
						samatra.x / 10, samatra.x % 10, samatra.y / 10, samatra.y % 10);
				// Then scale the new x and y coordinates based on being 1000 units
				// away from this space, scaled based on total distance away from home
				// to apply it in a line towards home.
				// To do this multiply delta x, y by 1000 / warpath
				// Then jitter fleet away from Utwig (for reasons)
				destination = (POINT) {samatra.x + 1000 * (plotmap[home].star_pt.x - samatra.x) / warpath +
						((plotmap[home].star_pt.x - plotmap[UTWIG_DEFINED].star_pt.x > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						samatra.y + 1000 * (plotmap[home].star_pt.y - samatra.y) / warpath +
						((plotmap[home].star_pt.y - plotmap[UTWIG_DEFINED].star_pt.y > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "+++++ Supox center %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Supox");
			destination = (POINT) {plotmap[SUPOX_DEFINED].star_pt.x +
					((plotmap[SUPOX_DEFINED].star_pt.x - plotmap[UTWIG_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[SUPOX_DEFINED].star_pt.y +
					((plotmap[SUPOX_DEFINED].star_pt.y - plotmap[UTWIG_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case THRADDASH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "Thraddash ATTACK");
				// Contrary to most other fleet movements, I don't want them jittered from the
				// SAMATRA based on their own radii.  Pick a spot nearby and normal jitter that.
				// First, imagine a coord near SAMATRA.  But do it backwards because.
				RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar (plotmap[visit].star));
				UWORD rand_val_s = RandomContext_Random (StarGenRNG);
				POINT samatra = {plotmap[visit].star_pt.x + LOBYTE (rand_val_s) - 128,
						plotmap[visit].star_pt.y + 128 - HIBYTE (rand_val_s)};
				COUNT warpath = sqrt ((plotmap[home].star_pt.x - samatra.x) *
						(plotmap[home].star_pt.x - samatra.x) +
						(plotmap[home].star_pt.y - samatra.y) *
						(plotmap[home].star_pt.y - samatra.y));
				fprintf(stderr, "***** WARPATH center %d.%d : %d.%d\n",
						samatra.x / 10, samatra.x % 10, samatra.y / 10, samatra.y % 10);
				// Then scale the new x and y coordinates based on being 1000 units
				// away from this space, scaled based on total distance away from home
				// to apply it in a line towards home.
				// To do this multiply delta x, y by 1000 / warpath
				// Bias away from the aqua helix because thraddash.
				destination = (POINT) {samatra.x + 1000 * (plotmap[home].star_pt.x - samatra.x) / warpath +
						((plotmap[home].star_pt.x - plotmap[AQUA_HELIX_DEFINED].star_pt.x > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						samatra.y + 1000 * (plotmap[home].star_pt.y - samatra.y) / warpath +
						((plotmap[home].star_pt.y - plotmap[AQUA_HELIX_DEFINED].star_pt.y > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "+++++ Thraddash center %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Thraddash");
			destination = (POINT) {plotmap[THRADD_DEFINED].star_pt.x +
					//((rand_val_x % 4) ? 1 : -1) *
					((plotmap[home].star_pt.x - plotmap[AQUA_HELIX_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[home].star_pt.y +
					//((rand_val_y % 4) ? 1 : -1) *
					((plotmap[home].star_pt.y - plotmap[AQUA_HELIX_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case UTWIG_ID: // Jitter towards Supox
			if (visit == SAMATRA_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "Utwig coming at the UQ/KA zone");
				// Contrary to most other fleet movements, I don't want them jittered from the
				// SAMATRA based on their own radii.  Pick a spot nearby and normal jitter that.
				// First, imagine a coord near SAMATRA that Supox used.
				RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar (plotmap[visit].star));
				UWORD rand_val_s = RandomContext_Random (StarGenRNG);
				POINT samatra = {plotmap[visit].star_pt.x + HIBYTE (rand_val_s) - 128,
						plotmap[visit].star_pt.y + LOBYTE (rand_val_s) - 128};
				COUNT warpath = sqrt ((plotmap[home].star_pt.x - samatra.x) *
						(plotmap[home].star_pt.x - samatra.x) +
						(plotmap[home].star_pt.y - samatra.y) *
						(plotmap[home].star_pt.y - samatra.y));
				fprintf(stderr, "***** WARPATH center %d.%d : %d.%d\n",
						samatra.x / 10, samatra.x % 10, samatra.y / 10, samatra.y % 10);
				// Then scale the new x and y coordinates based on being 1000 units
				// away from this space, scaled based on total distance away from home
				// to apply it in a line towards home.
				// To do this multiply delta x, y by 1000 / warpath
				// Then jitter fleet away from Supox, for reasons
				destination = (POINT) {samatra.x + 1000 * (plotmap[home].star_pt.x - samatra.x) / warpath +
						((plotmap[home].star_pt.x - plotmap[SUPOX_DEFINED].star_pt.x > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						samatra.y + 1000 * (plotmap[home].star_pt.y - samatra.y) / warpath +
						((plotmap[home].star_pt.y - plotmap[SUPOX_DEFINED].star_pt.y > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "+++++ Utwig center %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Utwig");
			destination = (POINT) {plotmap[UTWIG_DEFINED].star_pt.x +
					((plotmap[UTWIG_DEFINED].star_pt.x - plotmap[SUPOX_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[UTWIG_DEFINED].star_pt.y +
					((plotmap[UTWIG_DEFINED].star_pt.y - plotmap[SUPOX_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case VUX_ID:
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "VUX");
			destination = (POINT) {plotmap[VUX_DEFINED].star_pt.x +
					(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[VUX_DEFINED].star_pt.y +
					(((rand_val_x + rand_val_y) % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case YEHAT_ID: // Jitter towards Shofixti.  Except rebels who go the other way.
			if (visit == YEHAT_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "REBEL TIME\n");
				destination = (POINT) {plotmap[home].star_pt.x +
						((plotmap[home].star_pt.x - plotmap[SHOFIXTI_DEFINED].star_pt.x > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						plotmap[home].star_pt.y +
						((plotmap[home].star_pt.x - plotmap[SHOFIXTI_DEFINED].star_pt.x > 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "New rebel base at %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Yehat");
			destination = (POINT) {plotmap[YEHAT_DEFINED].star_pt.x +
					((plotmap[YEHAT_DEFINED].star_pt.x - plotmap[SHOFIXTI_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[YEHAT_DEFINED].star_pt.y +
					((plotmap[YEHAT_DEFINED].star_pt.y - plotmap[SHOFIXTI_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case MELNORME_ID:
			break;
		case DRUUGE_ID:
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Druuge");
			destination = (POINT) {plotmap[DRUUGE_DEFINED].star_pt.x +
					((rand_val_y % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[DRUUGE_DEFINED].star_pt.y +
					((rand_val_x % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case ILWRATH_ID: // Jitter towards pkunk
			if (visit == THRADD_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "ILWRATH ATTACK");
				COUNT warpath = sqrt ((plotmap[home].star_pt.x - plotmap[visit].star_pt.x) *
						(plotmap[home].star_pt.x - plotmap[visit].star_pt.x) +
						(plotmap[home].star_pt.y - plotmap[visit].star_pt.y) *
						(plotmap[home].star_pt.y - plotmap[visit].star_pt.y));
				// Scale the new x and y coordinates based on being 300 units
				// away from this space, scaled based on total distance away from home
				// to apply it in a line towards home.
				// To do this multiply delta x, y by 300 / warpath
				destination = (POINT) {plotmap[visit].star_pt.x + 300 *
						(plotmap[home].star_pt.x - plotmap[visit].star_pt.x) / warpath +
						Jitter (FleetPtr, rand_val_x),
						plotmap[visit].star_pt.y + 300 *
						(plotmap[home].star_pt.y - plotmap[visit].star_pt.y) / warpath +
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "+++++ Conflict center THRADDASHxILWRATH %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Ilwrath");
			destination = (POINT) {(plotmap[PKUNK_DEFINED].star_pt.x * 2 / 3) +
					(plotmap[ILWRATH_DEFINED].star_pt.x / 3) + 
					((plotmap[ILWRATH_DEFINED].star_pt.x - plotmap[PKUNK_DEFINED].star_pt.x < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					(plotmap[PKUNK_DEFINED].star_pt.y * 2 / 3) +
					(plotmap[ILWRATH_DEFINED].star_pt.y / 3) +
					((plotmap[ILWRATH_DEFINED].star_pt.y - plotmap[PKUNK_DEFINED].star_pt.y < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case MYCON_ID: // Jitter towards sun device and egg0 egg1 egg2
			if (visit == MYCON_TRAP_DEFINED)
			{
				JitDebug (FleetPtr, rand_val_x, rand_val_y, "*+*+* Juffo-Wup is the hot light in the darkness +*+*+\n");
				destination = (POINT) {plotmap[visit].star_pt.x +
						((plotmap[visit].star_pt.x - plotmap[home].star_pt.x < 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_x),
						plotmap[visit].star_pt.y +
						((plotmap[visit].star_pt.x - plotmap[home].star_pt.x < 0) ? 1 : -1) *
						Jitter (FleetPtr, rand_val_y)};
				fprintf(stderr, "The NON will be VOID at %d.%d : %d.%d\n",
						destination.x / 10, destination.x % 10, destination.y / 10, destination.y % 10);
				break;
			}
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Mycon");
			destination = (POINT) {plotmap[MYCON_DEFINED].star_pt.x +
					(((plotmap[MYCON_DEFINED].star_pt.x - plotmap[SUN_DEVICE_DEFINED].star_pt.x) +
					((plotmap[MYCON_DEFINED].star_pt.x - plotmap[EGG_CASE2_DEFINED].star_pt.x) / 2) +
					((plotmap[MYCON_DEFINED].star_pt.x - plotmap[EGG_CASE1_DEFINED].star_pt.x) / 3) +
					((plotmap[MYCON_DEFINED].star_pt.x - plotmap[EGG_CASE0_DEFINED].star_pt.x) / 4) < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[MYCON_DEFINED].star_pt.y +
					(((plotmap[MYCON_DEFINED].star_pt.y - plotmap[SUN_DEVICE_DEFINED].star_pt.y) +
					((plotmap[MYCON_DEFINED].star_pt.y - plotmap[EGG_CASE2_DEFINED].star_pt.y) / 2) +
					((plotmap[MYCON_DEFINED].star_pt.y - plotmap[EGG_CASE1_DEFINED].star_pt.y) / 3) +
					((plotmap[MYCON_DEFINED].star_pt.y - plotmap[EGG_CASE0_DEFINED].star_pt.y) / 4) < 0) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
 		case SLYLANDRO_ID:
			break;
		case UMGAH_ID:
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Umgah");
			destination = (POINT) {plotmap[TALKING_PET_DEFINED].star_pt.x +
					((rand_val_y % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x),
					plotmap[TALKING_PET_DEFINED].star_pt.y +
					((rand_val_x % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y)};
			break;
		case UR_QUAN_ID: // Halved jitter
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Ur Quan");
			destination = (POINT) {plotmap[SAMATRA_DEFINED].star_pt.x +
					((rand_val_y % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x) / 2,
					plotmap[SAMATRA_DEFINED].star_pt.y +
					((rand_val_x % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y) / 2};
			break;
		case KOHR_AH_ID: // Halved jitter, swap x and y from UQ (same seed) and ONE sign.
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "Kohr Ah");
			destination = (POINT) {plotmap[SAMATRA_DEFINED].star_pt.x -
					((rand_val_x % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_y) / 2,
					plotmap[SAMATRA_DEFINED].star_pt.y +
					((rand_val_y % 2) ? 1 : -1) *
					Jitter (FleetPtr, rand_val_x) / 2};
			break;
 		case ZOQFOTPIK_ID:
			// ZoqFot jitter is inverted (strength - jitter) away from the
			// KA/UQ conflict zone (sa-matra) and then /2, gives 16% - 50% jitter
			JitDebug (FleetPtr, rand_val_x, rand_val_y, "ZoqFot");
			destination = (POINT) {plotmap[home].star_pt.x +
					((plotmap[home].star_pt.x - plotmap[SAMATRA_DEFINED].star_pt.x > 0) ? 1 : -1) *
					((FleetPtr->actual_strength * (SPHERE_RADIUS_INCREMENT / 2)) -
						Jitter (FleetPtr, rand_val_x)) / 2,
					plotmap[home].star_pt.y +
					((plotmap[home].star_pt.y - plotmap[SAMATRA_DEFINED].star_pt.y > 0) ? 1 : -1) *
					((FleetPtr->actual_strength * (SPHERE_RADIUS_INCREMENT / 2)) -
						Jitter (FleetPtr, rand_val_y)) / 2};
			break;
 		case SYREEN_ID:
			return plotmap[SYREEN_DEFINED].star_pt;
			break;
		case ANDROSYNTH_ID:
	 	case CHENJESU_ID:
 		case MMRNMHRM_ID:
		default: break;
	}
	if (destination.x < 0)
			destination.x = 0;
	if (destination.x >= MAX_X_UNIVERSE)
			destination.x = MAX_X_UNIVERSE - 1;
	if (destination.y < 0)
			destination.y = 0;
	if (destination.y >= MAX_Y_UNIVERSE)
			destination.y = MAX_Y_UNIVERSE - 1;
	return (destination);
}
/*
ARILOU_ID:
 CHMMR_ID:
 EARTHLING_ID:
 ORZ_ID:
 PKUNK_ID:
 SHOFIXTI_ID:
 SPATHI_ID:
 SUPOX_ID:
 THRADDASH_ID:
 UTWIG_ID:
 VUX_ID:
 YEHAT_ID:
 MELNORME_ID:
 DRUUGE_ID:
 ILWRATH_ID:
 MYCON_ID:
 SLYLANDRO_ID:
 UMGAH_ID:
 UR_QUAN_ID:
 ZOQFOTPIK_ID:
 SYREEN_ID:
 KOHR_AH_ID:
 ANDROSYNTH_ID:
 CHENJESU_ID:
 MMRNMHRM_ID:
 */
