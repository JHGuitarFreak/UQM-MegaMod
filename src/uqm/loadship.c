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

SPECIES_ID capital[] = {CHMMR_ID, ORZ_ID, UTWIG_ID, YEHAT_ID, MYCON_ID,
		UR_QUAN_ID, KOHR_AH_ID, CHENJESU_ID};
#define NUM_CAPITALS 8
SPECIES_ID escort[] = {ARILOU_ID, PKUNK_ID, SPATHI_ID, SUPOX_ID, MELNORME_ID,
		DRUUGE_ID, SLYLANDRO_ID, ANDROSYNTH_ID, MMRNMHRM_ID};
#define NUM_ESCORTS 9
SPECIES_ID scout[] = {EARTHLING_ID, SHOFIXTI_ID, THRADDASH_ID, VUX_ID,
		ILWRATH_ID, UMGAH_ID, ZOQFOTPIK_ID, SYREEN_ID};
#define NUM_SCOUTS 8

int seedStamp = -1;

RandomContext *ShipGenRNG;

// these functions return MAX+1 if no match, or index if matched
static inline COUNT CapitalID (SPECIES_ID SpeciesID)
{
	for (COUNT x = 0; x < NUM_CAPITALS; x++)
		if (SpeciesID == capital[x])
			return x;
	return NUM_CAPITALS;
}
static inline COUNT EscortID (SPECIES_ID SpeciesID)
{
	for (COUNT x = 0; x < NUM_ESCORTS; x++)
		if (SpeciesID == escort[x])
			return x;
	return NUM_ESCORTS;
}
static inline COUNT ScoutID (SPECIES_ID SpeciesID)
{
	for (COUNT x = 0; x < NUM_SCOUTS; x++)
		if (SpeciesID == scout[x])
			return x;
	return NUM_SCOUTS;
}

// Uses a shipMap to map species ID to another species ID
SPECIES_ID
SeedShip (SPECIES_ID SpeciesID)
{
	static SPECIES_ID capitalMap[NUM_CAPITALS];
	static SPECIES_ID escortMap[NUM_ESCORTS];
	static SPECIES_ID scoutMap[NUM_SCOUTS];
	COUNT x = 0;
	UWORD rand_val;

	if (seedStamp != optCustomSeed)
	{
		if (!ShipGenRNG)
			ShipGenRNG = RandomContext_New ();
		RandomContext_SeedRandom (ShipGenRNG, optCustomSeed);
		for (x = 0; x < NUM_CAPITALS; x++)
			capitalMap[x] = NUM_CAPITALS;
		for (x = 0; x < NUM_ESCORTS; x++)
			escortMap[x] = NUM_ESCORTS;
		for (x = 0; x < NUM_SCOUTS; x++)
			scoutMap[x] = NUM_SCOUTS;
		for (x = 0; x < NUM_CAPITALS; x++)
		{
			rand_val = RandomContext_Random (ShipGenRNG) % NUM_CAPITALS;
			while (capitalMap[rand_val] != NUM_CAPITALS)
				rand_val = (rand_val + 1) % NUM_CAPITALS;
			capitalMap[rand_val] = x;
		}
		for (x = 0; x < NUM_ESCORTS; x++)
		{
			rand_val = RandomContext_Random (ShipGenRNG) % NUM_ESCORTS;
			while (escortMap[rand_val] != NUM_ESCORTS)
				rand_val = (rand_val + 1) % NUM_ESCORTS;
			escortMap[rand_val] = x;
		}
		for (x = 0; x < NUM_SCOUTS; x++)
		{
			rand_val = RandomContext_Random (ShipGenRNG) % NUM_SCOUTS;
			while (scoutMap[rand_val] != NUM_SCOUTS)
				rand_val = (rand_val + 1) % NUM_SCOUTS;
			scoutMap[rand_val] = x;
		}
		if (ShipGenRNG)
		{
			RandomContext_Delete (ShipGenRNG);
			ShipGenRNG = NULL;
		}
		seedStamp = optCustomSeed;
	}
	if ((x = CapitalID (SpeciesID)) < NUM_CAPITALS)
		return (capital[capitalMap[x]]);
	if ((x = EscortID (SpeciesID)) < NUM_ESCORTS)
		return (escort[escortMap[x]]);
	if ((x = ScoutID (SpeciesID)) < NUM_SCOUTS)
		return (scout[scoutMap[x]]);
	return SpeciesID;
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
			code_resources[optShipSeed ? SeedShip (SpeciesID) : SpeciesID]),
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
