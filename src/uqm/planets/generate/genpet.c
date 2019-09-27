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

#include "genall.h"
#include "../planets.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../encount.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "../../../options.h"


static bool GenerateTalkingPet_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateTalkingPet_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateTalkingPet_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateTalkingPet_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);

static void ZapToUrquanEncounter (void);


const GenerateFunctions generateTalkingPetFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateTalkingPet_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateTalkingPet_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateTalkingPet_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateTalkingPet_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateTalkingPet_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;
	int planetArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;

	if(!PrimeSeed){
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
		solarSys->SunDesc[0].PlanetByte = (RandomContext_Random (SysGenRNG) % solarSys->SunDesc[0].NumPlanets);
	}

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = TELLURIC_WORLD;

	if(!PrimeSeed){
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2];
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % MAX_GEN_MOONS);
	} else { 
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 204L / 100;
		angle = ARCTAN (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
				COSINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
				SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
		ComputeSpeed(&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
	}

	return true;
}

static bool
GenerateTalkingPet_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET)
			&& (GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES)
			|| !GET_GAME_STATE (TALKING_PET)
			|| StartSphereTracking (UMGAH_SHIP)))
	{
		NotifyOthers (UMGAH_SHIP, IPNL_ALL_CLEAR);
		PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
		ReinitQueue (&GLOBAL (ip_group_q));
		assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

		if (StartSphereTracking (UMGAH_SHIP))
		{
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			if (!GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES))
			{
				CloneShipFragment (UMGAH_SHIP,
						&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
				InitCommunication (UMGAH_CONVERSATION);
			}
			else
			{
				COUNT i;

				for (i = 0; i < 10; ++i)
				{
					CloneShipFragment (UMGAH_SHIP,
							&GLOBAL (npc_built_ship_q), 0);
				}
				InitCommunication (TALKING_PET_CONVERSATION);
			}
		}

		if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
		{
			BOOLEAN UmgahSurvivors;

			UmgahSurvivors = GetHeadLink (
					&GLOBAL (npc_built_ship_q)) != 0;
			GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;

			if (GET_GAME_STATE (PLAYER_HYPNOTIZED))
				ZapToUrquanEncounter ();
			else if (GET_GAME_STATE (UMGAH_ZOMBIE_BLOBBIES)
					&& !UmgahSurvivors)
			{
				// Defeated the zombie fleet.
				InitCommunication (TALKING_PET_CONVERSATION);
			}
			else if (!(StartSphereTracking (UMGAH_SHIP)))
			{
				// The Kohr-Ah have destroyed the Umgah, but the
				// talking pet survived.
				InitCommunication (TALKING_PET_CONVERSATION);
			}

			ReinitQueue (&GLOBAL (npc_built_ship_q));
			GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
		}

		return true;
	}

	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (RUINS_STRTAB));
	}

	GenerateDefault_generateOrbital (solarSys, world);

	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET) && PrimeSeed)
		solarSys->SysInfo.PlanetInfo.Weather = 0;

	return true;
}

static COUNT
GenerateTalkingPet_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateTalkingPet_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	(void) whichNode;
	return false;
}

static void
ZapToUrquanEncounter (void)
{
	HENCOUNTER hEncounter;

	if ((hEncounter = AllocEncounter ()) || (hEncounter = GetHeadEncounter ()))
	{
		SIZE dx, dy;
		ENCOUNTER *EncounterPtr;
		HFLEETINFO hStarShip;
		FLEET_INFO *TemplatePtr;
		BRIEF_SHIP_INFO *BSIPtr;

		LockEncounter (hEncounter, &EncounterPtr);

		if (hEncounter == GetHeadEncounter ())
			RemoveEncounter (hEncounter);
		memset (EncounterPtr, 0, sizeof (*EncounterPtr));

		InsertEncounter (hEncounter, GetHeadEncounter ());

		hStarShip = GetStarShipFromIndex (&GLOBAL (avail_race_q), URQUAN_SHIP);
		TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		EncounterPtr->origin = TemplatePtr->loc;
		EncounterPtr->radius = TemplatePtr->actual_strength;
		EncounterPtr->race_id = URQUAN_SHIP;
		EncounterPtr->num_ships = 1;
		EncounterPtr->flags = ONE_SHOT_ENCOUNTER;
		BSIPtr = &EncounterPtr->ShipList[0];
		BSIPtr->race_id = URQUAN_SHIP;
		BSIPtr->crew_level = TemplatePtr->crew_level;
		BSIPtr->max_crew = TemplatePtr->max_crew;
		BSIPtr->max_energy = TemplatePtr->max_energy;
		EncounterPtr->loc_pt.x = 5288;
		EncounterPtr->loc_pt.y = 4892;
		EncounterPtr->log_x = UNIVERSE_TO_LOGX (EncounterPtr->loc_pt.x);
		EncounterPtr->log_y = UNIVERSE_TO_LOGY (EncounterPtr->loc_pt.y);
		GLOBAL_SIS (log_x) = EncounterPtr->log_x;
		GLOBAL_SIS (log_y) = EncounterPtr->log_y;
		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);

		{
#define LOST_DAYS 15
			SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND * 2));
			MoveGameClockDays (LOST_DAYS);
		}

		GLOBAL (CurrentActivity) = MAKE_WORD (IN_HYPERSPACE, 0) | START_ENCOUNTER;

		dx = CurStarDescPtr->star_pt.x - EncounterPtr->loc_pt.x;
		dy = CurStarDescPtr->star_pt.y - EncounterPtr->loc_pt.y;
		dx = (SIZE)square_root ((long)dx * dx + (long)dy * dy)
				+ (FUEL_TANK_SCALE >> 1);

		if (!optInfiniteFuel)
			DeltaSISGauges (0, -dx, 0);

		if (GLOBAL_SIS (FuelOnBoard) < 5 * FUEL_TANK_SCALE)
		{
			dx = ((5 + ((COUNT)TFB_Random () % 5)) * FUEL_TANK_SCALE)
					- (SIZE)GLOBAL_SIS (FuelOnBoard);
			DeltaSISGauges (0, dx, 0);
		}
		DrawSISMessage (NULL);
		DrawHyperCoords (EncounterPtr->loc_pt);

		UnlockEncounter (hEncounter);
	}
}

