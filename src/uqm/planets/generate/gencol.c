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
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"


static bool GenerateColony_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateColony_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateColony_generateName(const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateColony_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);


const GenerateFunctions generateColonyFunctions = {
	/* .initNpcs         = */ GenerateColony_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateColony_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateColony_generateName,
	/* .generateOrbital  = */ GenerateColony_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateColony_initNpcs (SOLARSYS_STATE *solarSys)
{
	HIPGROUP hGroup;

	GLOBAL (BattleGroupRef) = GET_GAME_STATE (COLONY_GRPOFFS);
	if (GLOBAL (BattleGroupRef) == 0)
	{
		CloneShipFragment (URQUAN_SHIP,
				&GLOBAL (npc_built_ship_q), 0);
		GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		SET_GAME_STATE (COLONY_GRPOFFS, GLOBAL (BattleGroupRef));
	}

	GenerateDefault_initNpcs (solarSys);

	if (GLOBAL (BattleGroupRef)
			&& (hGroup = GetHeadLink (&GLOBAL (ip_group_q))))
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		GroupPtr->task = IN_ORBIT;
		GroupPtr->sys_loc = solarSys->SunDesc[0].PlanetByte + 1; /* orbitting colony */
		GroupPtr->dest_loc = solarSys->SunDesc[0].PlanetByte + 1; /* orbitting colony */
		GroupPtr->loc.x = 0;
		GroupPtr->loc.y = 0;
		GroupPtr->group_counter = 0;
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
	}

	return true;
}

static bool
GenerateColony_generatePlanets (SOLARSYS_STATE *solarSys)
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

	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = WATER_WORLD | PLANET_SHIELDED;
	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].alternate_colormap = NULL;

	if(!PrimeSeed){
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2] | PLANET_SHIELDED;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % MAX_GEN_MOONS);
	} else {
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 115L / 100;
		angle = ARCTAN (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x = COSINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y = SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
		ComputeSpeed(&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
	}

	return true;
}

static bool
GenerateColony_generateName(const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if (matchWorld(solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		utf8StringCopy(GLOBAL_SIS(PlanetName), sizeof(GLOBAL_SIS(PlanetName)),
			GAME_STRING(PLANET_NUMBER_BASE + 33));
		SET_GAME_STATE(BATTLE_PLANET, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index);
	} else
		GenerateDefault_generateName(solarSys, world);

	return true;
}

static bool
GenerateColony_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		DoPlanetaryAnalysis (&solarSys->SysInfo, world);

		if(PrimeSeed){
			solarSys->SysInfo.PlanetInfo.AtmoDensity =
					EARTH_ATMOSPHERE * 98 / 100;
			solarSys->SysInfo.PlanetInfo.Weather = 0;
			solarSys->SysInfo.PlanetInfo.Tectonics = 0;
			solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;
		}

		LoadPlanet (NULL);

		return true;
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

