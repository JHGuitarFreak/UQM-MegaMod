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
#include "coderes.h"
#include "corecode.h"
#include "globdata.h"
#include "nameref.h"
#include "races.h"
#include "init.h"

static RESOURCE code_resources[] = {
		NULL_RESOURCE,
		ARILOU_CODE,
		CHMMR_CODE,
		HUMAN_CODE,
		ORZ_CODE,
		PKUNK_CODE,
		SHOFIXTI_CODE,
		SPATHI_CODE,
		SUPOX_CODE,
		THRADDASH_CODE,
		UTWIG_CODE,
		VUX_CODE,
		YEHAT_CODE,
		MELNORME_CODE,
		DRUUGE_CODE,
		ILWRATH_CODE,
		MYCON_CODE,
		SLYLANDRO_CODE,
		UMGAH_CODE,
		URQUAN_CODE,
		ZOQFOTPIK_CODE,
		SYREEN_CODE,
		KOHR_AH_CODE,
		ANDROSYNTH_CODE,
		CHENJESU_CODE,
		MMRNMHRM_CODE,
		SIS_CODE,
		SAMATRA_CODE,
		URQUAN_DRONE_CODE };

SPECIES_ID ship[] = {CHMMR_ID, ORZ_ID, UTWIG_ID, YEHAT_ID,		// Capital
		MYCON_ID, UR_QUAN_ID, KOHR_AH_ID, CHENJESU_ID,			// Capital
		ARILOU_ID, PKUNK_ID, SPATHI_ID, SUPOX_ID, MELNORME_ID,	// Escort
		DRUUGE_ID, SLYLANDRO_ID, ANDROSYNTH_ID, MMRNMHRM_ID,	// Escort
		EARTHLING_ID, SHOFIXTI_ID, THRADDASH_ID, VUX_ID,		// Scout
		ILWRATH_ID, UMGAH_ID, ZOQFOTPIK_ID, SYREEN_ID};			// Scout

SPECIES_ID *capital = ship;
#define NUM_CAPITALS 8
SPECIES_ID *escort = &(ship[NUM_CAPITALS]);
#define NUM_ESCORTS 9
SPECIES_ID *scout = &(ship[NUM_CAPITALS + NUM_ESCORTS]);
#define NUM_SCOUTS 8
#define NUM_SHIPS (NUM_CAPITALS + NUM_ESCORTS + NUM_SCOUTS)

// Returns the array index in ship array for a specific SpeciesID.
static inline COUNT ShipIndex (SPECIES_ID SpeciesID)
{
	for (COUNT x = 0; x < NUM_SHIPS; x++)
		if (SpeciesID == ship[x])
			return x;
	return (NUM_SHIPS);
}

// Creates a ship map based on seed. We need up to two maps loaded
// at once to be able to handle the load/save game preview window.
static inline void SeedShipMap (SPECIES_ID *map, int seed)
{
	RandomContext *ShipGenRNG = RandomContext_New ();
	UWORD rand_val;
	SPECIES_ID *cMap = map;
	SPECIES_ID *eMap = &(map[NUM_CAPITALS]);
	SPECIES_ID *sMap = &(map[NUM_CAPITALS + NUM_ESCORTS]);
	COUNT x = 0;
	int saveSeedType = optSeedType;
	// Planet generation uses a different seeding math
	if (optSeedType == OPTVAL_PLANET)
		optSeedType = OPTVAL_MRQ;
	RandomContext_SeedRandom (ShipGenRNG, seed);
	for (x = 0; x < NUM_SHIPS; x++)
		map[x] = NUM_SPECIES_ID;
	for (x = 0; x < NUM_CAPITALS; x++)
	{
		rand_val = RandomContext_Random (ShipGenRNG) % NUM_CAPITALS;
		while (cMap[rand_val] != NUM_SPECIES_ID)
			rand_val = (rand_val + 1) % NUM_CAPITALS;
		cMap[rand_val] = capital[x];
	}
	for (x = 0; x < NUM_ESCORTS; x++)
	{
		rand_val = RandomContext_Random (ShipGenRNG) % NUM_ESCORTS;
		while (eMap[rand_val] != NUM_SPECIES_ID)
			rand_val = (rand_val + 1) % NUM_ESCORTS;
		eMap[rand_val] = escort[x];
	}
	for (x = 0; x < NUM_SCOUTS; x++)
	{
		rand_val = RandomContext_Random (ShipGenRNG) % NUM_SCOUTS;
		while (sMap[rand_val] != NUM_SPECIES_ID)
			rand_val = (rand_val + 1) % NUM_SCOUTS;
		sMap[rand_val] = scout[x];
	}
	if (ShipGenRNG)
	{
		RandomContext_Delete (ShipGenRNG);
		ShipGenRNG = NULL;
	}
	optSeedType = saveSeedType;
}

// Uses a ship map to map species ID to another species ID.
// When in load window, it uses two ship maps to fake a valid response.
SPECIES_ID
SeedShip (SPECIES_ID SpeciesID, BOOLEAN loadWindow)
{
	static int seedStamp = -1;
	static int sisStamp = -1;
	static SPECIES_ID shipMap[NUM_SHIPS];
	static SPECIES_ID shipWindowMap[NUM_SHIPS];
	SPECIES_ID target;
	COUNT index = NUM_SHIPS;

#ifdef DEBUG_SHIPSEED
	if (loadWindow)
		fprintf (stderr, "Calling SeedShip (load window) species %d, "
				"Seed %d, ShipSeed %s, SIS (Seed) %d, SIS (ShipSeed) %d\n",
				SpeciesID, optCustomSeed, optShipSeed ? "on" : "off",
				GLOBAL_SIS (Seed), GLOBAL_SIS (ShipSeed));
#endif
	if (seedStamp != optCustomSeed)
		SeedShipMap (shipMap, seedStamp = optCustomSeed);
	if (sisStamp != GLOBAL_SIS (Seed) && loadWindow)
		SeedShipMap (shipWindowMap, sisStamp = GLOBAL_SIS (Seed));

	target = SpeciesID;
	if ((index = ShipIndex (SpeciesID)) < NUM_SHIPS)
	{
		if (!loadWindow && optShipSeed)
			return shipMap[index];
		if (GLOBAL_SIS (ShipSeed) != 0)
			target = shipWindowMap[index];
		if (!optShipSeed)
			return target;
		for (index = 0; index < NUM_SHIPS; index++)
			if (shipMap[index] == target)
				break;
		if (index < NUM_SHIPS)
			return (ship[index]);
	}
	return target;
}

RACE_DESC *
load_ship (SPECIES_ID SpeciesID, BOOLEAN LoadBattleData)
{
	RACE_DESC *RDPtr = 0;
	void *CodeRef;
	RACE_DESC *RDPtrSwap = 0;
	void *CodeRefSwap;
	
	if (SpeciesID >= NUM_SPECIES_ID)
		return NULL;

#ifdef DEBUG_SHIPSEED
	fprintf (stderr, "Calling load_ship species %d, Seed %d, "
			"ShipSeed %s\n", SpeciesID, optCustomSeed,
			optShipSeed ? "on" : "off");
#endif
	CodeRef = CaptureCodeRes (LoadCodeRes (
			code_resources[optShipSeed ?
					SeedShip (SpeciesID, false) : SpeciesID]),
			&GlobData, (void **)(&RDPtr));
	CodeRefSwap = CaptureCodeRes (LoadCodeRes (
			code_resources[SpeciesID]),
			&GlobData, (void **)(&RDPtrSwap));
			
	if (!CodeRef || !CodeRefSwap)
		goto BadLoad;
	RDPtr->CodeRef = CodeRef;

	RDPtr->fleet = RDPtrSwap->fleet;
	if (RDPtr->ship_info.icons_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.icons = CaptureDrawable (LoadGraphic (
				RDPtr->ship_info.icons_rsc));
		if (!RDPtr->ship_info.icons)
		{
			/* goto BadLoad */
		}
	}
	
	if (RDPtr->ship_info.melee_icon_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.melee_icon = CaptureDrawable (LoadGraphic (
				RDPtr->ship_info.melee_icon_rsc));
		if (!RDPtr->ship_info.melee_icon)
		{
			/* goto BadLoad */
		}
	}

	if (RDPtrSwap->ship_info.race_strings_rsc != NULL_RESOURCE)
	{
		RDPtr->ship_info.race_strings = CaptureStringTable (LoadStringTable (
				RDPtrSwap->ship_info.race_strings_rsc));
		if (!RDPtr->ship_info.race_strings)
		{
			/* goto BadLoad */
		}
	}

	if (LoadBattleData)
	{
		DATA_STUFF *RawPtr = &RDPtr->ship_data;
		if (!load_animation (RawPtr->ship,
				RawPtr->ship_rsc[0],
				RawPtr->ship_rsc[1],
				RawPtr->ship_rsc[2]))
			goto BadLoad;

		if (RawPtr->weapon_rsc[0] != NULL_RESOURCE)
		{
			if (!load_animation (RawPtr->weapon,
					RawPtr->weapon_rsc[0],
					RawPtr->weapon_rsc[1],
					RawPtr->weapon_rsc[2]))
				goto BadLoad;
		}

		if (RawPtr->special_rsc[0] != NULL_RESOURCE)
		{
			if (!load_animation (RawPtr->special,
					RawPtr->special_rsc[0],
					RawPtr->special_rsc[1],
					RawPtr->special_rsc[2]))
				goto BadLoad;
		}

		if (RDPtrSwap->ship_data.captain_control.captain_rsc != NULL_RESOURCE)
		{
			RawPtr->captain_control.background = CaptureDrawable (LoadGraphic (
					RDPtrSwap->ship_data.captain_control.captain_rsc));
			if (!RawPtr->captain_control.background)
				goto BadLoad;
		}

		if (RDPtrSwap->ship_data.victory_ditty_rsc != NULL_RESOURCE)
		{
			RawPtr->victory_ditty =
					LoadMusic (RDPtrSwap->ship_data.victory_ditty_rsc);
			if (!RawPtr->victory_ditty)
				goto BadLoad;
		}

		if (RawPtr->ship_sounds_rsc != NULL_RESOURCE)
		{
			RawPtr->ship_sounds = CaptureSound (
					LoadSound (RawPtr->ship_sounds_rsc));
			if (!RawPtr->ship_sounds)
				goto BadLoad;
		}
	}

ExitFunc:
	return RDPtr;

	// TODO: We should really free the resources that did load here
BadLoad:
	if (CodeRef)
		DestroyCodeRes (ReleaseCodeRes (CodeRef));

	RDPtr = 0; /* failed */

	goto ExitFunc;
}

void
free_ship (RACE_DESC *raceDescPtr, BOOLEAN FreeIconData,
		BOOLEAN FreeBattleData)
{
	if (raceDescPtr->uninit_func != NULL)
		(*raceDescPtr->uninit_func) (raceDescPtr);

	if (FreeBattleData)
	{
		DATA_STUFF *shipData = &raceDescPtr->ship_data;

		free_image (shipData->special);
		free_image (shipData->weapon);
		free_image (shipData->ship);

		DestroyDrawable (
				ReleaseDrawable (shipData->captain_control.background));
		DestroyMusic (shipData->victory_ditty);
		DestroySound (ReleaseSound (shipData->ship_sounds));
	}

	if (FreeIconData)
	{
		SHIP_INFO *shipInfo = &raceDescPtr->ship_info;

		DestroyDrawable (ReleaseDrawable (shipInfo->melee_icon));
		DestroyDrawable (ReleaseDrawable (shipInfo->icons));
		DestroyStringTable (ReleaseStringTable (shipInfo->race_strings));
	}

	DestroyCodeRes (ReleaseCodeRes (raceDescPtr->CodeRef));
}
