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
#include "../scan.h"
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


static bool GenerateMycon_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateMycon_generateName(const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateMycon_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateMycon_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static COUNT GenerateMycon_generateLife (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateMycon_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateMyconFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateMycon_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateMycon_generateName,
	/* .generateOrbital  = */ GenerateMycon_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateMycon_generateEnergy,
	/* .generateLife     = */ GenerateMycon_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateMycon_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateMycon_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;

	if(!PrimeSeed){
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
		solarSys->SunDesc[0].PlanetByte = (RandomContext_Random (SysGenRNG) % solarSys->SunDesc[0].NumPlanets);
	} 	
	
	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if(PrimeSeed){
		if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets > 2)
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 2;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 80L / 100;
		angle = ARCTAN (
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
				COSINE (angle, solarSys->PlanetDesc[0].radius);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
				SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
		ComputeSpeed(&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
	}

	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = SHATTERED_WORLD;

	return true;
}

static bool
GenerateMycon_generateName(const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if (CurStarDescPtr->Index == EGG_CASE0_DEFINED && matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		utf8StringCopy(GLOBAL_SIS(PlanetName), sizeof(GLOBAL_SIS(PlanetName)),
			GAME_STRING(PLANET_NUMBER_BASE + 42));
		SET_GAME_STATE(BATTLE_PLANET, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index);
	}
	else
		GenerateDefault_generateName(solarSys, world);

	return true;
}

static bool
GenerateMycon_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if ((CurStarDescPtr->Index == MYCON_DEFINED
				|| CurStarDescPtr->Index == SUN_DEVICE_DEFINED)
				&& StartSphereTracking (MYCON_SHIP))
		{
			if (CurStarDescPtr->Index == MYCON_DEFINED
					|| !GET_GAME_STATE (SUN_DEVICE_UNGUARDED))
			{
				NotifyOthers (MYCON_SHIP, IPNL_ALL_CLEAR);
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (ip_group_q));
				assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

				if (CurStarDescPtr->Index == MYCON_DEFINED
						|| !GET_GAME_STATE (MYCON_FELL_FOR_AMBUSH))
				{
					CloneShipFragment (MYCON_SHIP,
							&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
				}
				else
				{
					COUNT i;

					for (i = 0; i < 5; ++i)
						CloneShipFragment (MYCON_SHIP,
								&GLOBAL (npc_built_ship_q), 0);
				}

				GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				if (CurStarDescPtr->Index == MYCON_DEFINED)
				{
					SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
				}
				else
				{
					SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
				}
				InitCommunication (MYCON_CONVERSATION);

				if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
					return true;

				{
					BOOLEAN MyconSurvivors;

					MyconSurvivors =
							GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0;

					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

					if (MyconSurvivors)
						return true;

					SET_GAME_STATE (SUN_DEVICE_UNGUARDED, 1);
					RepairSISBorder ();
				}
			}
		}

		switch (CurStarDescPtr->Index)
		{
			case SUN_DEVICE_DEFINED:
				if (!GET_GAME_STATE (SUN_DEVICE))
				{
					LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
							CaptureDrawable (
									LoadGraphic (SUN_DEVICE_MASK_PMAP_ANIM));
					solarSys->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (SUN_DEVICE_STRTAB));
				}
				break;
			case EGG_CASE0_DEFINED:
			case EGG_CASE1_DEFINED:
			case EGG_CASE2_DEFINED:
				if (GET_GAME_STATE (KNOW_ABOUT_SHATTERED) == 0)
					SET_GAME_STATE (KNOW_ABOUT_SHATTERED, 1);

				if (!isNodeRetrieved (&solarSys->SysInfo.PlanetInfo,
						ENERGY_SCAN, 0))
				{
					LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
							CaptureDrawable (
									LoadGraphic (EGG_CASE_MASK_PMAP_ANIM));
					solarSys->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
									LoadStringTable (EGG_CASE_STRTAB));
				}
				break;
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static COUNT
GenerateMycon_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == SUN_DEVICE_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (SUN_DEVICE))
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	if ((CurStarDescPtr->Index == EGG_CASE0_DEFINED
			|| CurStarDescPtr->Index == EGG_CASE1_DEFINED
			|| CurStarDescPtr->Index == EGG_CASE2_DEFINED)
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		// XXX: DiscoveryString is set by generateOrbital() only when the
		//   node has not been picked up yet
		if (!solarSys->SysInfo.PlanetInfo.DiscoveryString)
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateMycon_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == SUN_DEVICE_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		assert (!GET_GAME_STATE (SUN_DEVICE) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (SUN_DEVICE, 1);
		SET_GAME_STATE (SUN_DEVICE_ON_SHIP, 1);
		SET_GAME_STATE (MYCON_VISITS, 0);

		return true; // picked up
	}

	if ((CurStarDescPtr->Index == EGG_CASE0_DEFINED
			|| CurStarDescPtr->Index == EGG_CASE1_DEFINED
			|| CurStarDescPtr->Index == EGG_CASE2_DEFINED)
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		assert (whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		switch (CurStarDescPtr->Index)
		{
			case EGG_CASE0_DEFINED:
				SET_GAME_STATE (EGG_CASE0_ON_SHIP, 1);
				break;
			case EGG_CASE1_DEFINED:
				SET_GAME_STATE (EGG_CASE1_ON_SHIP, 1);
				break;
			case EGG_CASE2_DEFINED:
				SET_GAME_STATE (EGG_CASE2_ON_SHIP, 1);
				break;
		}

		return true; // picked up
	}

	(void) whichNode;
	return false;
}

static COUNT
GenerateMycon_generateLife (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	// Gee, I wonder why there isn't any life in Mycon systems...
	(void) whichNode;
	(void) solarSys;
	(void) world;
	(void) info;
	return 0;
}

