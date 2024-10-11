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
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateYehat_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateYehat_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateYehat_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateYehat_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateYehatFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateYehat_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateYehat_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateYehat_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateYehat_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateYehat_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC *pPlanet;

	GenerateDefault_generatePlanets (solarSys);

	if (PrimeSeed)
	{
		COUNT angle;

		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = WATER_WORLD;
		pPlanet->NumPlanets = 1;
		pPlanet->radius = EARTH_RADIUS * 106L / 100;
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

	return true;
}

static bool
GenerateYehat_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		if (StartSphereTracking (YEHAT_SHIP))
		{
			NotifyOthers (YEHAT_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (YEHAT_SHIP, &GLOBAL (npc_built_ship_q),
					INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication (YEHAT_CONVERSATION);

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
				CaptureStringTable (LoadStringTable (RUINS_STRTAB));
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateYehat_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateYehat_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	(void) whichNode;
	return false;
}