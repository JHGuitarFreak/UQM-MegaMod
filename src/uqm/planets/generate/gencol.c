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
static bool GenerateColony_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateColony_generateName (const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateColony_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);


const GenerateFunctions generateColonyFunctions = {
	/* .initNpcs         = */ GenerateColony_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateColony_generatePlanets,
	/* .generateMoons    = */ GenerateColony_generateMoons,
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
		BYTE PlanetByte = solarSys->SunDesc[0].PlanetByte + 1;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		GroupPtr->task = IN_ORBIT;
		GroupPtr->sys_loc = PlanetByte; /* orbitting colony */
		GroupPtr->dest_loc = PlanetByte; /* orbitting colony */
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
	PLANET_DESC *pPlanet;
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];

	pSunDesc->PlanetByte = 0;
	pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

	FillOrbits (solarSys, (BYTE)~0, pPlanet, FALSE);

	if (PrimeSeed)
	{
		COUNT angle;

		pPlanet->radius = EARTH_RADIUS * 115L / 100;
		angle = ARCTAN (pPlanet->location.x, pPlanet->location.y);
		pPlanet->location.x = COSINE (angle, pPlanet->radius);
		pPlanet->location.y = SINE (angle, pPlanet->radius);
		pPlanet->data_index = WATER_WORLD | PLANET_SHIELDED;
		ComputeSpeed (pPlanet, FALSE, 1);
		if (EXTENDED)
			pPlanet->NumPlanets = 1;
	}
	else
	{
		DWORD rand_val = RandomContext_Random (SysGenRNG);

		pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld () | PLANET_SHIELDED;

		GeneratePlanets (solarSys);

		pPlanet->NumPlanets = (rand_val % 5 == 0 ? 2 : 1);
	}

	return true;
}

static bool
GenerateColony_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (!PrimeSeed)
		return true;

	if (EXTENDED
			&& matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET))
	{
		solarSys->MoonDesc[0].data_index = SELENIC_WORLD;
	}

	return true;
}

static bool
GenerateColony_generateName (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world)
{
	GenerateDefault_generateName (solarSys, world);

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)) 
	{
		BYTE PlanetByte = solarSys->SunDesc[0].PlanetByte;
		PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 33));

		SET_GAME_STATE (BATTLE_PLANET, pPlanetDesc.data_index);
	}

	return true;
}

static bool
GenerateColony_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		DoPlanetaryAnalysis (&solarSys->SysInfo, world);

		if (PrimeSeed)
		{
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