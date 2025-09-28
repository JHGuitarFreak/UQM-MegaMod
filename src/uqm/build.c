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
#include "save.h"
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

// Gives the first fleet in avail_race_q that builds specified ship.
HLINK
GetFleetFromSpecies (SPECIES_ID id)
{
	HLINK hFleet, hNextFleet;

	for (hFleet = GetHeadLink (&GLOBAL (avail_race_q));
			hFleet; hFleet = hNextFleet)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
		if (FleetPtr->SpeciesID == id)
		{
			UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
			return hFleet;
		}
		hNextFleet = _GetSuccLink (FleetPtr);
		UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	}

	return hFleet;
}

// Returns the handle for the first fleet that normally builds the ships
// the fleet in Index builds after seeding.
// e.g. if Index is a fleet building Cruisers, returns the Earthling fleet.
// This is for every part of the game that assumes a RACE_ID is a SPECIES_ID
// If shipseed is not in use, it will do GetStarShipFromIndex on avail_race_q
// If this seems like overkill just remember the Yehat rebels.
HFLEETINFO
GetSeededFleetFromIndex (COUNT Index)
{
	FLEET_INFO *TemplatePtr = NULL;
	HFLEETINFO hFleet;
	SPECIES_ID ship;
	BOOLEAN loading = GLOBAL (CurrentActivity) & CHECK_PAUSE;
	BOOLEAN loadWindow = ((optShipSeed && GLOBAL_SIS (ShipSeed) == 0) ||
			(!optShipSeed && GLOBAL_SIS (ShipSeed) != 0) ||
			(optCustomSeed != GLOBAL_SIS (Seed)));

	hFleet = GetStarShipFromIndex (&GLOBAL (avail_race_q), Index);
	if (!hFleet)
		return hFleet;
	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	if (!TemplatePtr)
		return NULL;
	ship = SeedShip (TemplatePtr->SpeciesID, loadWindow);
	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	hFleet = GetFleetFromSpecies (ship);
	return hFleet;
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

/*
 * Returns the total point value of all the ships escorting the SIS.
 */
COUNT
CalculateEscortsPoints (void)
{
	COUNT total = 0;
	HSHIPFRAG hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			hStarShip; hStarShip = hNextShip)
	{
		SHIP_FRAGMENT *StarShipPtr;

		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		hNextShip = _GetSuccLink (StarShipPtr);
		total += ShipPoints (StarShipPtr->race_id);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	return total;
}

BOOLEAN
ShipsReady (RACE_ID race)
{
	SIZE i;
	COUNT year, month, day = 0;
	switch (race)
	{
		case PKUNK_SHIP:
			year = GET_GAME_STATE (PKUNK_SHIP_YEAR);
			month = GET_GAME_STATE (PKUNK_SHIP_MONTH);
			day = GET_GAME_STATE (PKUNK_SHIP_DAY);
			break;
		case SUPOX_SHIP:
			year = GET_GAME_STATE (SUPOX_SHIP_YEAR);
			month = GET_GAME_STATE (SUPOX_SHIP_MONTH);
			day = GET_GAME_STATE (SUPOX_SHIP_DAY);
			break;
		case UTWIG_SHIP:
			year = GET_GAME_STATE (UTWIG_SHIP_YEAR);
			month = GET_GAME_STATE (UTWIG_SHIP_MONTH);
			day = GET_GAME_STATE (UTWIG_SHIP_DAY);
			break;
		case YEHAT_SHIP:
		case YEHAT_REBEL_SHIP:
			year = GET_GAME_STATE (YEHAT_SHIP_YEAR);
			month = GET_GAME_STATE (YEHAT_SHIP_MONTH);
			day = GET_GAME_STATE (YEHAT_SHIP_DAY);
			break;
		default:
			return false;
	}
	return ((i = (GLOBAL (GameClock.year_index) - START_YEAR) - year) > 0) ||
			(i == 0 && (((i = GLOBAL (GameClock.month_index) - month) > 0) ||
			(i == 0 && GLOBAL (GameClock.day_index) > day)));
}

void
PrepareShip (RACE_ID race)
{
	BYTE mi, di, yi;

	mi = GLOBAL (GameClock.month_index);
	if ((di = GLOBAL (GameClock.day_index)) > 28)
		di = 28;
	yi = (BYTE)(GLOBAL (GameClock.year_index) - START_YEAR) + 1;
	switch (race)
	{
		case PKUNK_SHIP:
			SET_GAME_STATE (PKUNK_SHIP_YEAR, yi);
			SET_GAME_STATE (PKUNK_SHIP_MONTH, mi);
			SET_GAME_STATE (PKUNK_SHIP_DAY, di);
			break;
		case SUPOX_SHIP:
			SET_GAME_STATE (SUPOX_SHIP_YEAR, yi);
			SET_GAME_STATE (SUPOX_SHIP_MONTH, mi);
			SET_GAME_STATE (SUPOX_SHIP_DAY, di);
			break;
		case UTWIG_SHIP:
			SET_GAME_STATE (UTWIG_SHIP_YEAR, yi);
			SET_GAME_STATE (UTWIG_SHIP_MONTH, mi);
			SET_GAME_STATE (UTWIG_SHIP_DAY, di);
			break;
		case YEHAT_SHIP:
		case YEHAT_REBEL_SHIP:
			SET_GAME_STATE (YEHAT_SHIP_YEAR, yi);
			SET_GAME_STATE (YEHAT_SHIP_MONTH, mi);
			SET_GAME_STATE (YEHAT_SHIP_DAY, di);
			break;
		default:
			break;
	}
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

BOOLEAN
RaceDead (RACE_ID race)
{
	return CheckAlliance (race) == DEAD_GUY;
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

	// If options mismatch with SIS, it means we're in a load window.
	// In that case we want to find the correct fleet ID for that ship.
	if ((optShipSeed && GLOBAL_SIS (ShipSeed) == 0)
			|| (!optShipSeed && GLOBAL_SIS (ShipSeed) != 0)
			|| (optCustomSeed != GLOBAL_SIS (Seed)))
		hFleet = GetSeededFleetFromIndex (shipIndex);
	else
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
		StarShipPtr->crew_level = (crew_level > StarShipPtr->max_crew ?
				StarShipPtr->max_crew : crew_level);
		StarShipPtr->captains_name_index = captain;
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	else
		Index = -1;

	UnlockFleetInfo (&GLOBAL (avail_race_q), hFleet);
	return Index;
}

static void
deviceSwitch (int device, int val)
{
	int var = 0;

	if (val)
		var = val - 1;

	switch (device)
	{
		case 0:
			SET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP, var);
			break;
		case 1:
			SET_GAME_STATE (TALKING_PET_ON_SHIP, var);
			break;
		case 2:
			SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, var);
			break;
		case 3:
			SET_GAME_STATE (SUN_DEVICE_ON_SHIP, var);
			break;
		case 4:
			SET_GAME_STATE (ROSY_SPHERE_ON_SHIP, var);
			break;
		case 5:
			SET_GAME_STATE (AQUA_HELIX_ON_SHIP, var);
			break;
		case 6:
			SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, var);
			break;
		case 7:
			SET_GAME_STATE (ULTRON_CONDITION, var);
			break;
		case 8:
			SET_GAME_STATE (ULTRON_CONDITION, var ? 2 : 0);
			break;
		case 9:
			SET_GAME_STATE (ULTRON_CONDITION, var ? 3 : 0);
			break;
		case 10:
			SET_GAME_STATE (ULTRON_CONDITION, var ? 4 : 0);
			break;
		case 11:
			SET_GAME_STATE (MAIDENS_ON_SHIP, var);
			break;
		case 12:
			SET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP, var);
			break;
		case 13:
			SET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP, var);
			break;
		case 14:
			SET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP, var);
			break;
		case 15:
		case 16:
		case 17:
			if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) < 2 && var)
				SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 2);
			SET_GAME_STATE (KNOW_SYREEN_WORLD_SHATTERED, var);

			if (device == 15)
				SET_GAME_STATE (EGG_CASE0_ON_SHIP, var);
			else if (device == 16)
				SET_GAME_STATE (EGG_CASE1_ON_SHIP, var);
			else
				SET_GAME_STATE (EGG_CASE2_ON_SHIP, var);
			break;
		case 18:
			SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, var);
			break;
		case 19:
			SET_GAME_STATE (VUX_BEAST_ON_SHIP, var);
			break;
		case 20:
			SET_GAME_STATE (DESTRUCT_CODE_ON_SHIP, var);
			break;
		case 21:
			SET_GAME_STATE (PORTAL_KEY_ON_SHIP, var);
			break;
		case 22:
			SET_GAME_STATE (WIMBLIS_TRIDENT_ON_SHIP, var);
			break;
		case 23:
			SET_GAME_STATE (GLOWING_ROD_ON_SHIP, var);
			break;
		case 24:
			SET_GAME_STATE (MOONBASE_ON_SHIP, var);
			break;
		default: // Shouldn't happen, do nothing
			break;
	}

	//SET_GAME_STATE (SHIP_VAULT_UNLOCKED, 1);
	//SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 0);
	//SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
}

static void
upgradeSwitch (int upgrade, int val)
{
	int var = 0;
	BYTE LanderShields;
	int ModuleCost;

	if (val)
		var = val - 1;

	if (upgrade > 2 || upgrade < 7)
	{
		LanderShields = GET_GAME_STATE (LANDER_SHIELDS);
	}

	switch (upgrade)
	{
		case 0:
			SET_GAME_STATE (IMPROVED_LANDER_SPEED, var);
			break;
		case 1:
			SET_GAME_STATE (IMPROVED_LANDER_CARGO, var);
			break;
		case 2:
			SET_GAME_STATE (IMPROVED_LANDER_SHOT, var);
			break;
		case 3:
			if (var)
				LanderShields |= (1 << BIOLOGICAL_DISASTER);
			else
				LanderShields &= ~(1 << BIOLOGICAL_DISASTER);
			SET_GAME_STATE (LANDER_SHIELDS, LanderShields);
			break;
		case 4:
			if (var)
				LanderShields |= (1 << EARTHQUAKE_DISASTER);
			else
				LanderShields &= ~(1 << EARTHQUAKE_DISASTER);
			SET_GAME_STATE (LANDER_SHIELDS, LanderShields);
			break;
		case 5:
			if (var)
				LanderShields |= (1 << LIGHTNING_DISASTER);
			else
				LanderShields &= ~(1 << LIGHTNING_DISASTER);
			SET_GAME_STATE (LANDER_SHIELDS, LanderShields);
			break;
		case 6:
			if (var)
				LanderShields |= (1 << LAVASPOT_DISASTER);
			else
				LanderShields &= ~(1 << LAVASPOT_DISASTER);
			SET_GAME_STATE (LANDER_SHIELDS, LanderShields);
			break;
		case 7:
			ModuleCost = var ? 4000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[ANTIMISSILE_DEFENSE]) = ModuleCost;
			break;
		case 8:
			ModuleCost = var ? 4000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[BLASTER_WEAPON]) = ModuleCost;
			break;
		case 9:
			ModuleCost = var ? 1000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[HIGHEFF_FUELSYS]) = ModuleCost;
			break;
		case 10:
			ModuleCost = var ? 5000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[TRACKING_SYSTEM]) = ModuleCost;
			break;
		case 11:
			ModuleCost = var ? 6000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[CANNON_WEAPON]) = ModuleCost;
			break;
		case 12:
			ModuleCost = var ? 4000 / MODULE_COST_SCALE : 0;
			GLOBAL (ModuleCost[SHIVA_FURNACE]) = ModuleCost;
			break;
		default: // Shouldn't happen, do nothing
			break;
	}
}

static void
cheatAddRemoveDevices (void)
{
	BYTE i;

	for (i = 0; i < ARRAY_SIZE (optDeviceArray); i++)
	{
		if (!optDeviceArray[i])
			continue;
		else
			deviceSwitch (i, optDeviceArray[i]);
	}
}

static void
cheatAddRemoveUpgrades (void)
{
	BYTE i;

	for (i = 0; i < NUM_UPGRADES; i++)
	{
		if (!optUpgradeArray[i])
			continue;
		else
			upgradeSwitch (i, optUpgradeArray[i]);
	}
}

void
loadGameCheats (void)
{
	if (EXTENDED && !StarSeed)
	{
		plot_map[MELNORME0_DEFINED].star->Type =
				MAKE_STAR (SUPER_GIANT_STAR, ORANGE_BODY, -1);
	}

	if(optInfiniteRU)
		oldRU = GlobData.SIS_state.ResUnits;
	else
		oldRU = 0;

	//for (BYTE i = ARILOU_SHIP; i <= MMRNMHRM_SHIP; ++i)
	//{
	//	StartSphereTracking (i);
	//	KillRace (i);
	//}

	// SET_GAME_STATE (CHMMR_UNLEASHED, 1);

	// SET_GAME_STATE (KNOW_HOMEWORLD, ~0);
		
	if (optInfiniteFuel)
	{
		loadFuel = GlobData.SIS_state.FuelOnBoard;
		GLOBAL_SIS (FuelOnBoard) = GetFuelTankCapacity();
	} 
	else
		loadFuel = 0;

	cheatAddRemoveDevices ();
	cheatAddRemoveUpgrades ();
}

// Jitter returns a distance between 0..66.6% of the fleet's actual strength,
// weighted towards 20% median value.  We can adjust the jitter by changing
// the fraction at the end.
// (COUNT) sqrt (rand_val) gives a value 0..255 which leans heavy towards 255
// so subtract from 255 to receive a weighted towards zero jitter.
COUNT
Jitter (COUNT str, UWORD rand_val)
{
	return (str * (SPHERE_RADIUS_INCREMENT / 2) *
			(255 - (COUNT) sqrt (rand_val)) / 256) * 2 / 3;
}

void
JitDebug (FLEET_INFO *FleetPtr, UWORD rand_val_x, UWORD rand_val_y)
{
	static const char * const fleet_name[] =
			{"ERROR", "Arilou", "CHMMR", "Earthling", "Orz", "Pkunk",
			"Shofixti", "Spathi", "Supox", "Thraddash", "Utwig", "VUX",
			"Yehat", "Melnorme", "Druuge", "Ilwrath", "Mycon", "Slylandro",
			"Umgah", "Ur Quan", "ZoqFotPik", "Syreen", "Kohr Ah",
			"Androsynth", "Chenjesu", "Mmrnmhrm"};
	COUNT str = (FleetPtr->actual_strength > 0 ?
			FleetPtr->actual_strength :
			WarEraStrength (FleetPtr->SpeciesID));
	fprintf (stderr, "%s Fleet %d: Actual Str %d; WarEra Str %d; ",
			fleet_name[FleetPtr->SpeciesID > 25 ? 0 : FleetPtr->SpeciesID],
			FleetPtr->SpeciesID,
			FleetPtr->actual_strength * SPHERE_RADIUS_INCREMENT / 2,
			WarEraStrength (FleetPtr->SpeciesID) * SPHERE_RADIUS_INCREMENT / 2);
	fprintf (stderr, "Rand X %d (%d); Rand Y %d (%d)\n",
			rand_val_x, (255 - (COUNT) sqrt (rand_val_x)),
			rand_val_y, (255 - (COUNT) sqrt (rand_val_y)));
	fprintf (stderr, "		Jit X %d%% Jit Y %d%%; (Jit X %d Jit Y %d)  ",
			(255 - (COUNT) sqrt (rand_val_x)) * 67 / 256,
			(255 - (COUNT) sqrt (rand_val_y)) * 67 / 256,
			Jitter (str, rand_val_x),
			Jitter (str, rand_val_y));
	fprintf (stderr, "%05.1f : %05.1f  units.\n",
			(float) Jitter (str, rand_val_x) / 10,
			(float) Jitter (str, rand_val_y) / 10);
}

// Provide the default fleet movement or war era position for the
// given fleet and plot ID being visited
POINT
DefaultFleetLocation (SPECIES_ID SpeciesID, COUNT visit)
{
#ifdef DEBUG_STARSEED
	fprintf (stderr, "Seeding hard coded location for fleet %d (plot %d).\n",
			SpeciesID, visit);
#endif
	// We need to return plot specific coordinates here
	// We also assume we only care about fleet movements and war map,
	// as initial fleet positions are loaded from the ship files.
	switch (SpeciesID)
	{
		case CHMMR_ID:
				return (POINT) { 577, 2509}; // Homeworld
		case EARTHLING_ID:
				return (POINT) {1806, 1476}; // War era map
		case PKUNK_ID:
			if (visit == YEHAT_DEFINED)
				return (POINT) {4970,  400}; // Rejoin Yehat
			else if (visit == WAR_ERA)
				return (POINT) { 577,  463}; // War Era 'Unknown'
			else
				return (POINT) { 502,  401}; // Return homeworld
		case SHOFIXTI_ID:
				return (POINT) {2852,  242}; // War era map
		case SPATHI_ID:
				return (POINT) {2416, 3687}; // War era map
		case SUPOX_ID:
			if (visit == SAMATRA_DEFINED)
				return (POINT) {6479, 7541}; // Attack Kohr-Ah
			else
				return (POINT) {7468, 9246}; // Return homeworld
		case THRADDASH_ID:
			if (visit == SAMATRA_DEFINED)
				return (POINT) {4879, 7201}; // Attack Kohr-Ah
			else if (visit == WAR_ERA)
				return (POINT) {2808, 8522}; // War Era 'Unknown'
			else
				return (POINT) {2535, 8358}; // Return homeworld
		case UTWIG_ID:
			if (visit == SAMATRA_DEFINED)
				return (POINT) {7208, 7000}; // Attack Kohr-Ah
			else
				return (POINT) {8534, 8797}; // Return homeworld
		case VUX_ID:
				return (POINT) {4333, 1520}; // War era map
		case YEHAT_ID:
			if (visit == YEHAT_DEFINED)
				return (POINT) {5150,    0}; // Here there be rebels
			else if (visit == WAR_ERA)
				return (POINT) {4969,   75}; // War era map
			else
				return (POINT) {4970,   40}; // Filthy Royalists
		case DRUUGE_ID:
				return (POINT) {9421, 2754}; // War Era 'Unknown'
		case ILWRATH_ID:
			if (visit == THRADD_DEFINED)
				return (POINT) {2500, 8070}; // Thraddash/Ilwrath zone
			else if (visit == PKUNK_DEFINED)
				return (POINT) {  48, 1700}; // Attacking Pkunk (unused?)
			else if (visit == WAR_ERA)
				return (POINT) {   0, 3589}; // War era map
			else
				return (POINT) { 215, 3630}; // Homeworld (extended lore)
		case MYCON_ID:
			if (visit == MYCON_TRAP_DEFINED)
				return (POINT) {6858,  577}; // Goin to the party
			else if (visit == WAR_ERA)
				return (POINT) {6278, 2399}; // War era map
			else
				return (POINT) {6392, 2200}; // Going back home
		case UMGAH_ID:
			if (visit == SAMATRA_DEFINED)
				return (POINT) {5288, 4892}; // Compelled to seek death
			else
				return (POINT) {1860, 6099}; // War era map
		case SYREEN_ID:
			if (visit == MYCON_TRAP_DEFINED)
				return (POINT) {6858,  577}; // Goin to the party
			else
				return (POINT) {4125, 3770}; // Going back home
		case ANDROSYNTH_ID:
				return (POINT) {3676, 2619}; // War era map
		case CHENJESU_ID:
				return (POINT) { 701, 2137}; // War era map
		case MMRNMHRM_ID:
				return (POINT) { 672, 2930}; // War era map
		default:
			fprintf (stderr, "%s %d / plot %d combo.\n",
					"DefaultFleetLocation called with bad fleet",
					SpeciesID, visit);
			return (POINT) {0, 0};
	}
}

// Provide the war era strength for the given species ID (from fleet).
COUNT
WarEraStrength (SPECIES_ID SpeciesID)
{
	switch (SpeciesID)
	{
		case EARTHLING_ID:
			return 460 / SPHERE_RADIUS_INCREMENT * 2;
		case PKUNK_ID:
			return 350 / SPHERE_RADIUS_INCREMENT * 2;
		case SHOFIXTI_ID:
			return 329 / SPHERE_RADIUS_INCREMENT * 2;
		case SPATHI_ID:
			return 1007 / SPHERE_RADIUS_INCREMENT * 2;
		case THRADDASH_ID:
			return 723 / SPHERE_RADIUS_INCREMENT * 2;
		case VUX_ID:
			return 572 / SPHERE_RADIUS_INCREMENT * 2;
		case YEHAT_ID:
			return 1029 / SPHERE_RADIUS_INCREMENT * 2;
		case DRUUGE_ID:
			return 745 / SPHERE_RADIUS_INCREMENT * 2;
		case ILWRATH_ID:
			return 470 / SPHERE_RADIUS_INCREMENT * 2;
		case MYCON_ID:
			return 591 / SPHERE_RADIUS_INCREMENT * 2;
		case UMGAH_ID:
			return 506 / SPHERE_RADIUS_INCREMENT * 2;
		case ANDROSYNTH_ID:
			return 504 / SPHERE_RADIUS_INCREMENT * 2;
		case CHENJESU_ID:
			return 657 / SPHERE_RADIUS_INCREMENT * 2;
		case MMRNMHRM_ID:
			return 482 / SPHERE_RADIUS_INCREMENT * 2;
		default:
			return 0;
	}
	return 0;
}

// Provide the homeworld plot ID for the given species ID (from fleet).
COUNT
Homeworld (SPECIES_ID SpeciesID)
{
	switch (SpeciesID)
	{
		case ARILOU_ID:
			return ARILOU_DEFINED;
		case CHMMR_ID:
			return CHMMR_DEFINED;
		case EARTHLING_ID:
			return SOL_DEFINED;
		case ORZ_ID:
			return ORZ_DEFINED;
		case PKUNK_ID:
			return PKUNK_DEFINED;
		case SHOFIXTI_ID:
			return SHOFIXTI_DEFINED;
		case SPATHI_ID:
			return SPATHI_DEFINED;
		case SUPOX_ID:
			return SUPOX_DEFINED;
		case THRADDASH_ID:
			return THRADD_DEFINED;
		case UTWIG_ID:
			return UTWIG_DEFINED;
		case VUX_ID:
			return VUX_DEFINED;
		case YEHAT_ID:
			return YEHAT_DEFINED;
		case MELNORME_ID:
			return MELNORME1_DEFINED; // I have my reasons
		case DRUUGE_ID:
			return DRUUGE_DEFINED;
		case ILWRATH_ID:
			return ILWRATH_DEFINED;
		case MYCON_ID:
			return MYCON_DEFINED;
		case SLYLANDRO_ID:
			return SLYLANDRO_DEFINED;
		case UMGAH_ID:
			return TALKING_PET_DEFINED;
		case UR_QUAN_ID:
			return SAMATRA_DEFINED;
		case ZOQFOTPIK_ID:
			return ZOQFOT_DEFINED;
		case SYREEN_ID:
			return SYREEN_DEFINED;
		case KOHR_AH_ID:
			return SAMATRA_DEFINED;
		case ANDROSYNTH_ID:
			return ANDROSYNTH_DEFINED;
		case CHENJESU_ID:
			return CHMMR_DEFINED;
		case MMRNMHRM_ID:
			return MOTHER_ARK_DEFINED;
		default:
			fprintf(stderr, "Homeworld %s %d.\n",
					"(Fleet ID) called with bad fleet ID", SpeciesID);
		   	return NUM_PLOTS;
	}

}

// SeedFleet is called during initial setup of a map.  Sets the fleet
// referenced in the FLEET_INFO pointer passed to the coordinates of that
// fleet's plot on the PLOT_LOCATION array passed.
// For Prime seed games this just returns, as the fleet does not relocate.
void
SeedFleet (FLEET_INFO *FleetPtr, PLOT_LOCATION *plotmap)
{
	if ((!StarSeed) || (optSeedType == OPTVAL_MRQ))
		return;
	if (!FleetPtr || !plotmap)
	{
		fprintf(stderr, "SeedFleet called with NULL PTR(s).\n");
		return;
	}
	FleetPtr->known_loc = SeedFleetLocation (FleetPtr, plotmap,
			(FleetPtr->SpeciesID == ILWRATH_ID) ? PKUNK_DEFINED : HOME);
	return;
}

// Helper define functions for SeedFleetLocation
//
// WARPATH gives an offset to the "warpoint" destination, to be multiplied
// by dist, in the direction of the warpather's home star.  This will scale
// the arguments based on total distance to provide a new center.
// {WARPATH_X (visit, dist) + jitter, WARPATH_Y (visit, dist) + jitter}
#define WARPATH_X(a, d) \
	(warpoint.x + (d) * (plotmap[home].star_pt.x - (a).x) / sqrt \
	((plotmap[home].star_pt.x - (a).x) * (plotmap[home].star_pt.x - (a).x) + \
	(plotmap[home].star_pt.y - (a).y) * (plotmap[home].star_pt.y - (a).y)))
#define WARPATH_Y(a, d) \
	(warpoint.y + (d) * (plotmap[home].star_pt.y - (a).y) / sqrt \
	((plotmap[home].star_pt.x - (a).x) * (plotmap[home].star_pt.x - (a).x) + \
	(plotmap[home].star_pt.y - (a).y) * (plotmap[home].star_pt.y - (a).y)))

// This points your jitter towards or away from "a" in X, Y directions
#define TOWARD_X(a) \
	((plotmap[home].star_pt.x < plotmap[(a)].star_pt.x) ? 1 : -1)
#define TOWARD_Y(a) \
	((plotmap[home].star_pt.y < plotmap[(a)].star_pt.y) ? 1 : -1)
#define AWAY_X(a) \
	((plotmap[home].star_pt.x > plotmap[(a)].star_pt.x) ? 1 : -1)
#define AWAY_Y(a) \
	((plotmap[home].star_pt.y > plotmap[(a)].star_pt.y) ? 1 : -1)

// For those with no toward/away you can randomly jitter +/-
#define HALF_X (rand_val_x % 2 ? 1 : -1)
#define HALF_Y (rand_val_y % 2 ? 1 : -1)

// 1/5 of the time we flip X, or Y, or both for some races.  Why?  Why not!
#define FIFTH_X ((rand_val_x * rand_val_y) % 10 ? 1 : -1)
#define FIFTH_Y ((rand_val_x + rand_val_y) % 5 ? 1 : -1)

// SeedFleetLocation is called when a fleet needs to be located or relocated.
// It will return the center for the SOI of the race listed, or if visit
// plot ID != HOME, another value as dictated by the interaction of the two.
// Then the location is "jittered".  Essentially this function maps between a
// fleet/race ID and the plot IDs.
// For PrimeSeed games this will need to look up the hard coded fleet movements
// or resets which were part of gameev.c, or the war era map.
POINT
SeedFleetLocation (FLEET_INFO *FleetPtr, PLOT_LOCATION *plotmap, COUNT visit)
{
	UWORD rand_val_x, rand_val_y;
	COUNT home;		// Plot ID of the homeworld for the fleet
	COUNT strength; // Strength of the fleet
	POINT warpoint;	// location being visited, or offset for samatra
	POINT location = {0, 0}; // The results of the seeding
	BOOLEAN myRNG = false; // If you create RNG, clean up RNG
	UNICODE buf[256] = ""; // For debug string

	if (!FleetPtr || !plotmap)
	{
		fprintf (stderr, "SeedFleetLocation called with NULL PTR(s).\n");
		return (POINT) {~0, ~0};
	}

	// Firstly, handle hard coded data from prime seed
	if (!StarSeed || optSeedType == OPTVAL_MRQ)
		return DefaultFleetLocation (FleetPtr->SpeciesID, visit);

	home = Homeworld (FleetPtr->SpeciesID);
	if (home >= NUM_PLOTS)
		return (POINT) {~0, ~0}; // Error already messaged in Homeworld ().
	if (visit >= NUM_PLOTS && visit != WAR_ERA && visit != HOME)
	{
		fprintf (stderr, "%s %d\n",
				"SeedFleet called with invalid away plot ID", visit);
		return (POINT) {~0, ~0};
	}
	if (visit < NUM_PLOTS && !plotmap[visit].star)
	{
		fprintf (stderr, "%s %d has a NULL star pointer.\n",
				"SeedFleet called, but away plot ID", visit);
		return (POINT) {~0, ~0};
	}
	if (!plotmap[home].star)
	{
		fprintf (stderr, "%s %d has a NULL star pointer.\n",
				"SeedFleet called, but home plot ID", home);
		return (POINT) {~0, ~0};
	}

	if (!StarGenRNG)
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr, "SeedFleet creating a STAR GEN RNG\n");
#endif
		StarGenRNG = RandomContext_New ();
		myRNG = true;
	}
	RandomContext_SeedRandom (StarGenRNG,
			GetRandomSeedForStar (plotmap[home].star));
	rand_val_x = RandomContext_Random (StarGenRNG);
	rand_val_y = RandomContext_Random (StarGenRNG);
	if (visit > 0 && visit != SAMATRA_DEFINED && visit < NUM_PLOTS)
	{
		RandomContext_SeedRandom (StarGenRNG,
				GetRandomSeedForStar (plotmap[visit].star));
		rand_val_x += RandomContext_Random (StarGenRNG) % sizeof (UWORD);
		rand_val_y += RandomContext_Random (StarGenRNG) % sizeof (UWORD);
		// If there's a conflict, this will assist in WARPATH
		warpoint = plotmap[visit].star_pt;
	}
	else if (visit == SAMATRA_DEFINED)
	{
	// Contrary to other fleet movements, I don't want to jitter from
	// the SAMATRA center.  Pick a spot nearby and then jitter from there.
	// First, imagine a coord near SAMATRA that Utwig/Supox/Thraddash can
	// sort of share (it will bias in the general direction of home).
		RandomContext_SeedRandom (StarGenRNG, GetRandomSeedForStar
				(plotmap[visit].star));
		UWORD rand_val_s = RandomContext_Random (StarGenRNG);
		warpoint = (POINT) {
				plotmap[visit].star_pt.x +
				LOBYTE (rand_val_s) * 2 * AWAY_X (visit),
				plotmap[visit].star_pt.y +
				HIBYTE (rand_val_s) * 2 * AWAY_Y (visit)};

		snprintf (buf, sizeof (buf), "'Sa-Matra' center %05.1f : %05.1f\n",
				(float) warpoint.x / 10, (float) warpoint.y / 10);
	}
	else
		warpoint = plotmap[home].star_pt; // just in case

	strength = (visit == WAR_ERA ? WarEraStrength (FleetPtr->SpeciesID) :
			FleetPtr->actual_strength);

	switch (FleetPtr->SpeciesID)
	{
		case ARILOU_ID: // Arilou don't jitter
			location = plotmap[home].star_pt;
			break;
		case CHMMR_ID: // Jitter towards Mmrnmhrm home
			location = (POINT) {
					plotmap[home].star_pt.x + TOWARD_X (MOTHER_ARK_DEFINED) *
					FIFTH_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y + TOWARD_Y (MOTHER_ARK_DEFINED) *
					FIFTH_Y * Jitter (strength, rand_val_y)};
			break;
		case EARTHLING_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case ORZ_ID: // Jitter towards the playground
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (TAALO_PROTECTOR_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (TAALO_PROTECTOR_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case PKUNK_ID: // Jitter away from Ilwrath / home from Yehat
			if (visit == YEHAT_DEFINED)
			{
				location = (POINT) {
						plotmap[visit].star_pt.x +
						AWAY_X (visit) * FIFTH_X *
						Jitter (strength, rand_val_x),
						plotmap[visit].star_pt.y +
						AWAY_Y (visit) * FIFTH_Y *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					AWAY_X (ILWRATH_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					AWAY_Y (ILWRATH_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case SHOFIXTI_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (YEHAT_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (YEHAT_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case SPATHI_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case SUPOX_ID: // Jitter towards Utwig
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Utwig, for reasons
				location = (POINT) {
						WARPATH_X (warpoint, 1200) +
						AWAY_X (UTWIG_DEFINED) *
						Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 1200) +
						AWAY_Y (UTWIG_DEFINED) *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (UTWIG_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (UTWIG_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case THRADDASH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Bias away from the aqua helix because thraddash.
				location = (POINT) {
						WARPATH_X (warpoint, 1200) +
						AWAY_X (AQUA_HELIX_DEFINED) *
						Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 1200) +
						AWAY_Y (AQUA_HELIX_DEFINED) *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (AQUA_HELIX_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (AQUA_HELIX_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case UTWIG_ID: // Jitter towards Supox
			if (visit == SAMATRA_DEFINED)
			{
				// 1200 distance times warpath
				// Then jitter fleet away from Supox, for reasons
				location = (POINT) {
						WARPATH_X (warpoint, 1200) +
						AWAY_X (SUPOX_DEFINED) *
						Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 1200) +
						AWAY_Y (SUPOX_DEFINED) *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (SUPOX_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (SUPOX_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case VUX_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case YEHAT_ID: // Jitter towards Shofixti.  Except rebels.
			if (visit == YEHAT_DEFINED)
			{
				location = (POINT) {
						plotmap[home].star_pt.x +
						AWAY_X (SHOFIXTI_DEFINED) * FIFTH_X *
						Jitter (strength, rand_val_x),
						plotmap[home].star_pt.y +
						AWAY_Y (SHOFIXTI_DEFINED) * FIFTH_Y *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					TOWARD_X (SHOFIXTI_DEFINED) * FIFTH_X *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					TOWARD_Y (SHOFIXTI_DEFINED) * FIFTH_Y *
					Jitter (strength, rand_val_y)};
			break;
		case MELNORME_ID:
			break;
		case DRUUGE_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case ILWRATH_ID: // Jitter towards victims
			if (visit == THRADD_DEFINED) // default 290 distance
			{
				// Set the X and Y coords based on being 500 units away
				// from the target along the warpath.
				location = (POINT) {
						WARPATH_X (warpoint, 500) +
						TOWARD_X (visit) * Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 500) +
						TOWARD_Y (visit) * Jitter (strength, rand_val_y)};
				snprintf (buf, sizeof (buf), "%s %05.1f : %05.1f\n",
						"ILWRATH x THRADDASH",
						(float) location.x / 10, (float) location.y / 10);
				break;
			}
			if (visit == PKUNK_DEFINED) // default 1270 distance
			{
				warpoint = plotmap[visit].star_pt;
				location = (POINT) {
						WARPATH_X (warpoint, 1400) +
						TOWARD_X (visit) * Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 1400) +
						TOWARD_Y (visit) * Jitter (strength, rand_val_y)};
				snprintf (buf, sizeof (buf), "%s %05.1f : %05.1f\n",
						"ILWRATH x PKUNK",
						(float) location.x / 10, (float) location.y / 10);
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case MYCON_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				location = (POINT) {
						plotmap[visit].star_pt.x +
						AWAY_X (visit) * Jitter (strength, rand_val_x),
						plotmap[visit].star_pt.y +
						AWAY_Y (visit) * Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) { // Jitter towards sun device and eggs
					plotmap[home].star_pt.x +
					(plotmap[home].star_pt.x * 2 < 
					plotmap[SUN_DEVICE_DEFINED].star_pt.x + 
					plotmap[EGG_CASE2_DEFINED].star_pt.x / 2 +
					plotmap[EGG_CASE1_DEFINED].star_pt.x / 3 +
					plotmap[EGG_CASE0_DEFINED].star_pt.x / 6 ? 1 : -1) *
					Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					(plotmap[home].star_pt.y * 2 < 
					plotmap[SUN_DEVICE_DEFINED].star_pt.y + 
					plotmap[EGG_CASE2_DEFINED].star_pt.y / 2 +
					plotmap[EGG_CASE1_DEFINED].star_pt.y / 3 +
					plotmap[EGG_CASE0_DEFINED].star_pt.y / 6 ? 1 : -1) *
					Jitter (strength, rand_val_y)};
			break;
 		case SLYLANDRO_ID:
			break;
		case UMGAH_ID:
			if (visit == SAMATRA_DEFINED)
			{
				// 2000 distance times warpath
				// Bias away from the umgah
				location = (POINT) {
						WARPATH_X (warpoint, 2000) +
						AWAY_X (TALKING_PET_DEFINED) *
						Jitter (strength, rand_val_x),
						WARPATH_Y (warpoint, 2000) +
						AWAY_Y (TALKING_PET_DEFINED) *
						Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case UR_QUAN_ID: // Halved jitter due to size
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_Y * Jitter (strength, rand_val_x) / 2,
					plotmap[home].star_pt.y +
					HALF_X + Jitter (strength, rand_val_y) / 2};
			break;
		case KOHR_AH_ID: // Halved jitter, swap x and y from UQ (same seed)
			location = (POINT) { // and ONE sign for a 90 degree turn
					plotmap[home].star_pt.x -
					HALF_X * Jitter (strength, rand_val_y) / 2,
					plotmap[home].star_pt.y +
					HALF_Y + Jitter (strength, rand_val_x) / 2};
			break;
 		case ZOQFOTPIK_ID: // ZoqFot jitter is inverted (str - jit) away from
			location = (POINT) { // KA/UQ zone, then /2, gives 16% - 50% jit
					plotmap[home].star_pt.x + AWAY_X (SAMATRA_DEFINED) *
					(FleetPtr->actual_strength * SPHERE_RADIUS_INCREMENT / 2 -
						Jitter (strength, rand_val_x)) / 2,
					plotmap[home].star_pt.y + AWAY_Y (SAMATRA_DEFINED) *
					(FleetPtr->actual_strength * SPHERE_RADIUS_INCREMENT / 2 -
						Jitter (strength, rand_val_y)) / 2};
			break;
 		case SYREEN_ID:
			if (visit == MYCON_TRAP_DEFINED)
			{
				location = (POINT) {
						plotmap[visit].star_pt.x +
						HALF_X * Jitter (strength, rand_val_x),
						plotmap[visit].star_pt.y +
						HALF_Y * Jitter (strength, rand_val_y)};
				break;
			}
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
		case ANDROSYNTH_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_y),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_x)};
			break;
	 	case CHENJESU_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_x),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_y)};
			break;
 		case MMRNMHRM_ID:
			location = (POINT) {
					plotmap[home].star_pt.x +
					HALF_X * Jitter (strength, rand_val_y),
					plotmap[home].star_pt.y +
					HALF_Y * Jitter (strength, rand_val_x)};
			break;
		default: break;
	}
	if (location.x < 0)
			location.x = 0;
	if (location.x >= MAX_X_UNIVERSE)
			location.x = MAX_X_UNIVERSE - 1;
	if (location.y < 0)
			location.y = 0;
	if (location.y >= MAX_Y_UNIVERSE)
			location.y = MAX_Y_UNIVERSE - 1;

#ifdef DEBUG_STARSEED
	JitDebug (FleetPtr, rand_val_x, rand_val_y);
	if (buf[0])
		fprintf (stderr, "%s", buf);
#endif
	if (StarGenRNG && myRNG)
	{
		RandomContext_Delete (StarGenRNG);
		StarGenRNG = NULL;
	}
	return (location);
}
