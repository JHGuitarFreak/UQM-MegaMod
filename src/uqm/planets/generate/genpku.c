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
#include "libs/mathlib.h"


static bool GeneratePkunk_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GeneratePkunk_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GeneratePkunk_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GeneratePkunk_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generatePkunkFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GeneratePkunk_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GeneratePkunk_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GeneratePkunk_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GeneratePkunk_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GeneratePkunk_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;
	int planetArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;

	if(!PrimeSeed){
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
	}

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	solarSys->PlanetDesc[0].data_index = WATER_WORLD;
	solarSys->PlanetDesc[0].alternate_colormap = NULL;
	solarSys->PlanetDesc[0].NumPlanets = 1;

	if(!PrimeSeed){
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2];
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
	} else { 
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 104L / 100;
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
GeneratePkunk_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (StartSphereTracking (PKUNK_SHIP))
		{
			NotifyOthers (PKUNK_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (PKUNK_SHIP,
					&GLOBAL (npc_built_ship_q), INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication (PKUNK_CONVERSATION);

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
					CaptureStringTable (LoadStringTable (PKUNK_RUINS_STRTAB));
			if (GET_GAME_STATE (CLEAR_SPINDLE))
			{	// Already picked up the Clear Spindle, skip the report
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
						SetAbsStringTableIndex (
						solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
			}
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static bool
GeneratePkunk_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		GenerateDefault_landerReportCycle (solarSys);

		// The artifact can be picked up from any ruin
		if (!GET_GAME_STATE (CLEAR_SPINDLE))
		{	// Just picked up the Clear Spindle from a ruin
			SetLanderTakeoff ();

			SET_GAME_STATE (CLEAR_SPINDLE, 1);
			SET_GAME_STATE (CLEAR_SPINDLE_ON_SHIP, 1);
		}

		return false; // do not remove the node
	}

	(void) whichNode;
	return false;
}

static COUNT
GeneratePkunk_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

