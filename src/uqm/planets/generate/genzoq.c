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
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateZoqFotPik_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateZoqFotPik_generatePlanets (SOLARSYS_STATE *solarSys);
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
	/* .generateMoons    = */ GenerateDefault_generateMoons,
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

	if (optSpaceMusic)
		findRaceSOI();

	return true;
}

static bool
GenerateZoqFotPik_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;
	int planetArray[] = { REDUX_WORLD, PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD, };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;

	if(!PrimeSeed){
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
	}

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);
	
	if (CurStarDescPtr->Index == ZOQFOT_DEFINED || (EXTENDED && CurStarDescPtr->Index == ZOQ_COLONY_DEFINED)) {

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = REDUX_WORLD;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;
		solarSys->PlanetDesc[0].alternate_colormap = NULL;

		if (!PrimeSeed) {
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random(SysGenRNG) % 3];
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
		}
		else if (CurStarDescPtr->Index == ZOQFOT_DEFINED) {
			solarSys->PlanetDesc[0].radius = EARTH_RADIUS * 138L / 100;
			angle = ARCTAN(
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
				COSINE(angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
				SINE(angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			ComputeSpeed(&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
		}
	}

	return true;
}

static bool
GenerateZoqFotPik_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (CurStarDescPtr->Index == ZOQFOT_DEFINED) {
			if (StartSphereTracking(ZOQFOTPIK_SHIP))
			{
				PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue(&GLOBAL(ip_group_q));
				assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

				if (GET_GAME_STATE(ZOQFOT_DISTRESS))
				{
					CloneShipFragment(BLACK_URQUAN_SHIP,
						&GLOBAL(npc_built_ship_q), 0);

					GLOBAL(CurrentActivity) |= START_INTERPLANETARY;
					SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
					InitCommunication(BLACKURQ_CONVERSATION);

					if (GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
						return true;

					if (GetHeadLink(&GLOBAL(npc_built_ship_q)))
					{
						GLOBAL(CurrentActivity) &= ~START_INTERPLANETARY;
						ReinitQueue(&GLOBAL(npc_built_ship_q));
						GetGroupInfo(GROUPS_RANDOM, GROUP_LOAD_IP);
						return true;
					}
				}

				CloneShipFragment(ZOQFOTPIK_SHIP, &GLOBAL(npc_built_ship_q),
					INFINITE_FLEET);

				GLOBAL(CurrentActivity) |= START_INTERPLANETARY;
				SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
				InitCommunication(ZOQFOTPIK_CONVERSATION);

				if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
				{
					GLOBAL(CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue(&GLOBAL(npc_built_ship_q));
					GetGroupInfo(GROUPS_RANDOM, GROUP_LOAD_IP);
				}

				return true;
			}

			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable(LoadStringTable(RUINS_STRTAB));
		} 
		else if (EXTENDED 
			&& CurStarDescPtr->Index == ZOQ_COLONY_DEFINED
			&& matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
		{
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable(LoadGraphic(RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable(LoadStringTable(ZFPRUINS_STRTAB));
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static COUNT
GenerateZoqFotPik_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	
	if (matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (CurStarDescPtr->Index == ZOQFOT_DEFINED)
			return GenerateDefault_generateRuins(solarSys, whichNode, info);
		else if (EXTENDED && CurStarDescPtr->Index == ZOQ_COLONY_DEFINED)
			return GenerateRandomNodes(&solarSys->SysInfo, ENERGY_SCAN, 4,
				0, whichNode, info);
	}

	return 0;
}

static bool
GenerateZoqFotPik_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (CurStarDescPtr->Index == ZOQFOT_DEFINED 
			|| (EXTENDED && CurStarDescPtr->Index == ZOQ_COLONY_DEFINED))
		{
			// Standard ruins report
			GenerateDefault_landerReportCycle(solarSys);
			return false;
		}
	}

	(void) whichNode;
	return false;
}
