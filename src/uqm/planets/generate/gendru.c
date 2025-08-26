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
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"

#include <string.h>


static bool GenerateDruuge_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateDruuge_generateName (const SOLARSYS_STATE *,
		const PLANET_DESC *world);
static bool GenerateDruuge_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateDruuge_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateDruuge_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateDruugeFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateDruuge_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDruuge_generateName,
	/* .generateOrbital  = */ GenerateDruuge_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDruuge_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDruuge_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateDruuge_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pPlanet;
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets (solarSys);

	if (PrimeSeed)
	{
		COUNT angle;

		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		memmove (&solarSys->PlanetDesc[1], &solarSys->PlanetDesc[0],
				sizeof (solarSys->PlanetDesc[0]) * pSunDesc->NumPlanets);
		++pSunDesc->NumPlanets;

		pPlanet->data_index = DUST_WORLD;
		pPlanet->radius = EARTH_RADIUS * 50L / 100;
		pPlanet->NumPlanets = 0;
		angle = HALF_CIRCLE - OCTANT;
		pPlanet->location.x = COSINE (angle, pPlanet->radius);
		pPlanet->location.y = SINE (angle, pPlanet->radius);
		pPlanet->rand_seed =
				MAKE_DWORD (pPlanet->location.x, pPlanet->location.y);
		ComputeSpeed (pPlanet, FALSE, 1);
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateDesolateWorld ();
	}

	return true;
}

static bool
GenerateDruuge_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	GenerateDefault_generateName (solarSys, world);

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& IsHomeworldKnown (DRUUGE_HOME))
	{
		BYTE PlanetByte = solarSys->SunDesc[0].PlanetByte;
		PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 41));

		SET_GAME_STATE (BATTLE_PLANET, pPlanetDesc.data_index);
	}

	return true;
}

static bool
GenerateDruuge_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		if (StartSphereTracking (DRUUGE_SHIP))
		{
			NotifyOthers (DRUUGE_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (DRUUGE_SHIP,
					&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication (DRUUGE_CONVERSATION);
			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
			}
			return true;
		}
		else
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (
							LoadStringTable (DRUUGE_RUINS_STRTAB));
			if (GET_GAME_STATE (ROSY_SPHERE))
			{	// Already picked up Rosy Sphere, skip the report
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
						SetAbsStringTableIndex (
						solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
			}
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& !DIF_HARD)
	{
		solarSys->SysInfo.PlanetInfo.Weather = 3;
		solarSys->SysInfo.PlanetInfo.Tectonics = 2;
	}

	return true;
}

static bool
GenerateDruuge_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		GenerateDefault_landerReportCycle (solarSys);

		// The artifact can be picked up from any ruin
		if (!GET_GAME_STATE (ROSY_SPHERE))
		{	// Just picked up the Rosy Sphere from a ruin
			SetLanderTakeoff ();

			SET_GAME_STATE (ROSY_SPHERE, 1);
			SET_GAME_STATE (ROSY_SPHERE_ON_SHIP, 1);
		}

		return false; // do not remove the node
	}

	(void) whichNode;
	return false;
}

static COUNT
GenerateDruuge_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}