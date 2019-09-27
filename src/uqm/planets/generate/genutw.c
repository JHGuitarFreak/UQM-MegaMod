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
#include "../lander.h"
#include "../planets.h"
#include "../../build.h"
#include "../../comm.h"
#include "../../gendef.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"


static bool GenerateUtwig_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateUtwig_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateUtwig_generateName(const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateUtwig_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateUtwig_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateUtwig_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateUtwigFunctions = {
	/* .initNpcs         = */ GenerateUtwig_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateUtwig_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateUtwig_generateName,
	/* .generateOrbital  = */ GenerateUtwig_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateUtwig_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateUtwig_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateUtwig_initNpcs (SOLARSYS_STATE *solarSys)
{
	if (CurStarDescPtr->Index == BOMB_DEFINED
			&& !GET_GAME_STATE (UTWIG_BOMB))
	{
		ReinitQueue (&GLOBAL (ip_group_q));
		assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

		if (optSpaceMusic)
			findRaceSOI();
	}
	else
	{
		GenerateDefault_initNpcs (solarSys);
	}

	return true;
}

static bool
GenerateUtwig_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;
	int planetArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;

	if (CurStarDescPtr->Index == UTWIG_DEFINED) {
		solarSys->SunDesc[0].PlanetByte = 0;

		if(!PrimeSeed){
			solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
		}

		FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
		GeneratePlanets (solarSys);

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = WATER_WORLD;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].alternate_colormap = NULL;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;

		if(PrimeSeed){
			solarSys->PlanetDesc[0].radius = EARTH_RADIUS * 174L / 100;
			angle = ARCTAN (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
					solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
					COSINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
					SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			ComputeSpeed(&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
		} else {			
			solarSys->SunDesc[0].PlanetByte = (RandomContext_Random (SysGenRNG) % solarSys->SunDesc[0].NumPlanets);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2];
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
		}
	} else if (CurStarDescPtr->Index == BOMB_DEFINED) {		
		solarSys->SunDesc[0].PlanetByte = 5;		
		solarSys->SunDesc[0].MoonByte = 1;

		if(!PrimeSeed){
			solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 6) + 6);
		}

		FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
		GeneratePlanets (solarSys);

		if(!PrimeSeed){		
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = (RandomContext_Random (SysGenRNG) % (YEL_GAS_GIANT - BLU_GAS_GIANT) + BLU_GAS_GIANT);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % (4 - 2) + 2);
		}		
	}

	return true;
}

static bool
GenerateUtwig_generateName(const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if (CurStarDescPtr->Index == UTWIG_DEFINED && MET_AN_UTWIG
		&& matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		utf8StringCopy(GLOBAL_SIS(PlanetName), sizeof(GLOBAL_SIS(PlanetName)),
			GAME_STRING(PLANET_NUMBER_BASE + 40));
		SET_GAME_STATE(BATTLE_PLANET, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index);
	}
	else
		GenerateDefault_generateName(solarSys, world);

	return true;
}

static bool
GenerateUtwig_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if ((CurStarDescPtr->Index == UTWIG_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
			|| (CurStarDescPtr->Index == BOMB_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte)
			&& !GET_GAME_STATE (UTWIG_BOMB)))
	{
		if ((CurStarDescPtr->Index == UTWIG_DEFINED
				|| !GET_GAME_STATE (UTWIG_HAVE_ULTRON))
				&& StartSphereTracking (UTWIG_SHIP))
		{
			NotifyOthers (UTWIG_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (UTWIG_SHIP,
					&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			if (CurStarDescPtr->Index == UTWIG_DEFINED)
			{
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			}
			else
			{
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			}
			InitCommunication (UTWIG_CONVERSATION);

			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
			}
			return true;
		}

		if (CurStarDescPtr->Index == BOMB_DEFINED
				&& !GET_GAME_STATE (BOMB_UNPROTECTED)
				&& StartSphereTracking (DRUUGE_SHIP))
		{
			COUNT i;

			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			for (i = 0; i < 5; ++i)
			{
				CloneShipFragment (DRUUGE_SHIP,
						&GLOBAL (npc_built_ship_q), 0);
			}
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			InitCommunication (DRUUGE_CONVERSATION);

			if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				return true;

			{
				BOOLEAN DruugeSurvivors;

				DruugeSurvivors =
						GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0;

				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

				if (DruugeSurvivors)
					return true;

				RepairSISBorder ();
				SET_GAME_STATE (BOMB_UNPROTECTED, 1);
			}
		}

		if (CurStarDescPtr->Index == BOMB_DEFINED)
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (BOMB_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (BOMB_STRTAB));
		}
		else
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (RUINS_STRTAB));
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	if (CurStarDescPtr->Index == UTWIG_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET) && PrimeSeed)
	{
		solarSys->SysInfo.PlanetInfo.Weather = 1;
		solarSys->SysInfo.PlanetInfo.Tectonics = 1;
	}

	return true;
}

static COUNT
GenerateUtwig_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == UTWIG_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	if (CurStarDescPtr->Index == BOMB_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (UTWIG_BOMB))
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateUtwig_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == UTWIG_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	if (CurStarDescPtr->Index == BOMB_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		assert (!GET_GAME_STATE (UTWIG_BOMB) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (UTWIG_BOMB, 1);
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 1);
		SET_GAME_STATE (DRUUGE_MANNER, 1);
		SET_GAME_STATE (DRUUGE_VISITS, 0);
		SET_GAME_STATE (DRUUGE_HOME_VISITS, 0);

		return true; // picked up
	}

	(void) whichNode;
	return false;
}
