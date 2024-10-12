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
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateZoqFotPik_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateZoqFotPik_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateZoqFotPik_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateZoqFotPik_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateZoqFotPik_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateZoqFotPik_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateZoqFotPikFunctions = {
	/* .initNpcs         = */ GenerateZoqFotPik_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateZoqFotPik_generatePlanets,
	/* .generateMoons    = */ GenerateZoqFotPik_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateZoqFotPik_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateZoqFotPik_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateZoqFotPik_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateZoqFotPik_initNpcs (SOLARSYS_STATE *solarSys)
{
	if (GET_GAME_STATE (ZOQFOT_DISTRESS) != 1)
		GenerateDefault_initNpcs (solarSys);

	if (SpaceMusicOK)
		findRaceSOI ();

	return true;
}

static bool
GenerateZoqFotPik_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC *pPlanet;

	GenerateDefault_generatePlanets (solarSys);

	if (CurStarDescPtr->Index == ZOQFOT_DEFINED)
	{
		if (PrimeSeed)
		{
			COUNT angle;

			pSunDesc->PlanetByte = 0;
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = WATER_WORLD;
			pPlanet->NumPlanets = 1;
			pPlanet->radius = EARTH_RADIUS * 138L / 100;
			angle = ARCTAN (pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = COSINE (angle, pPlanet->radius);
			pPlanet->location.y = SINE (angle, pPlanet->radius);
			ComputeSpeed (pPlanet, FALSE, 1);
		}
		else
		{
			pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateHabitableWorld ();
		}
	}
	else if (EXTENDED)
	{
		pSunDesc->PlanetByte = PickClosestHabitable (solarSys);

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED)
		{
			pPlanet->NumPlanets = 1;
			pSunDesc->MoonByte = 0;
			pPlanet->data_index = GenerateGasGiantWorld ();

			if (!PrimeSeed)
			{	// preserving the integrity of the universe
				while (pPlanet->NumPlanets == 0)
					GeneratePlanets (solarSys);
				pSunDesc->MoonByte = PlanetByteGen (pPlanet);
			}
		}
		else
			pPlanet->data_index = GenerateHabitableWorld ();
	}

	return true;
}

static bool
GenerateZoqFotPik_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED
			&& matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		BYTE MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC *pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = GenerateHabitableWorld ();
	}

	return true;
}

static bool
GenerateZoqFotPik_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (CurStarDescPtr->Index == ZOQFOT_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		if (StartSphereTracking (ZOQFOTPIK_SHIP))
		{
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			if (GET_GAME_STATE (ZOQFOT_DISTRESS))
			{
				CloneShipFragment (BLACK_URQUAN_SHIP,
					&GLOBAL (npc_built_ship_q), 0);

				GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
				InitCommunication (BLACKURQ_CONVERSATION);

				if (GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
					return true;

				if (GetHeadLink(&GLOBAL(npc_built_ship_q)))
				{
					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
					return true;
				}
			}

			CloneShipFragment (ZOQFOTPIK_SHIP,
					&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication (ZOQFOTPIK_CONVERSATION);

			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
			}

			return true;
		}

		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
			CaptureStringTable (LoadStringTable(RUINS_STRTAB));
	}

	if ((CurStarDescPtr->Index == ZOQ_COLONY0_DEFINED
			|| CurStarDescPtr->Index == ZOQ_COLONY1_DEFINED
			|| CurStarDescPtr->Index == ZOQ_COLONY3_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
			&& EXTENDED)
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (ZFPRUINS_STRTAB));
	}
	
	if (CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE)
			&& EXTENDED)
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (ZFPRUINS_STRTAB));
	}

	GenerateDefault_generateOrbital (solarSys, world);

	if ((CurStarDescPtr->Index == ZOQFOT_DEFINED || EXTENDED)
			&& CurStarDescPtr->Index != ZOQ_COLONY2_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& !DIF_HARD)
	{
		DWORD rand = RandomContext_GetSeed (SysGenRNG);
		PLANET_INFO *PlanetInfo = &solarSys->SysInfo.PlanetInfo;

		PlanetInfo->Weather = 1;
		PlanetInfo->Tectonics = 1;
		PlanetInfo->SurfaceTemperature = RangeMinMax (-10, 40, rand);
	}

	if (CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE)
			&& EXTENDED && !DIF_HARD)
	{
		DWORD rand = RandomContext_GetSeed (SysGenRNG);
		PLANET_INFO *PlanetInfo = &solarSys->SysInfo.PlanetInfo;

		PlanetInfo->Weather = 1;
		PlanetInfo->Tectonics = 1;
		PlanetInfo->SurfaceTemperature = RangeMinMax (-10, 40, rand);
	}

	return true;
}

static COUNT
GenerateZoqFotPik_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
#define NUM_RUINS 4

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		if (CurStarDescPtr->Index == ZOQFOT_DEFINED)
		{
			return GenerateDefault_generateRuins (
				solarSys, whichNode, info);
		}

		if ((CurStarDescPtr->Index == ZOQ_COLONY0_DEFINED
				|| CurStarDescPtr->Index == ZOQ_COLONY1_DEFINED
				|| CurStarDescPtr->Index == ZOQ_COLONY3_DEFINED)
				&& EXTENDED)
		{
			return GenerateRandomNodes (&solarSys->SysInfo, ENERGY_SCAN,
					NUM_RUINS, 0, whichNode, info);
		}
	}
	
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE)
			&& CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED
			&& EXTENDED)
	{
		return GenerateRandomNodes (&solarSys->SysInfo, ENERGY_SCAN,
				NUM_RUINS, 0, whichNode, info);
	}

	return 0;
}

static bool
GenerateZoqFotPik_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& CurStarDescPtr->Index != ZOQ_COLONY2_DEFINED)
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);

		return false;
	}
	
	if (CurStarDescPtr->Index == ZOQ_COLONY2_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		GenerateDefault_landerReportCycle (solarSys);

		return false;
	}

	(void) whichNode;
	return false;
}