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
#include "../../comm.h"
#include "../../gamestr.h"


static bool GenerateSlylandro_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSlylandro_generateName (const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateSlylandro_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);


const GenerateFunctions generateSlylandroFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSlylandro_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateSlylandro_generateName,
	/* .generateOrbital  = */ GenerateSlylandro_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateSlylandro_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC *pPlanet;

	GenerateDefault_generatePlanets (solarSys);

	if (PrimeSeed)
	{
		pSunDesc->PlanetByte = 3;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = RED_GAS_GIANT;
		pPlanet->NumPlanets = 1;
	}
	else
	{
		BYTE i;
#define DWARF_GASG_DIST SCALE_RADIUS (12)

		for (i = 0; i < pSunDesc->NumPlanets; i++)
		{
			if (solarSys->PlanetDesc[i].radius >= DWARF_GASG_DIST)
				break;
		}

		pSunDesc->PlanetByte = i;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateWorlds (ONLY_GAS);

		if (solarSys->PlanetDesc[i].radius < DWARF_GASG_DIST)
		{
			COUNT angle;
			DWORD rand = RandomContext_GetSeed (SysGenRNG);

			pPlanet->radius =
					RangeMinMax (DWARF_GASG_DIST, MAX_PLANET_RADIUS, rand);
			angle = ARCTAN (pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = COSINE (angle, pPlanet->radius);
			pPlanet->location.y = SINE (angle, pPlanet->radius);
			ComputeSpeed (pPlanet, FALSE, 1);
		}
	}

	return true;
}

static bool
GenerateSlylandro_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if (GET_GAME_STATE (SLYLANDRO_HOME_VISITS)
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		utf8StringCopy (GLOBAL_SIS(PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 36));
		SET_GAME_STATE (BATTLE_PLANET,
				solarSys->PlanetDesc[
					solarSys->SunDesc[0].PlanetByte].data_index);
	}
	else
		GenerateDefault_generateName (solarSys, world);

	return true;
}

static bool
GenerateSlylandro_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		InitCommunication (SLYLANDRO_HOME_CONVERSATION);
		return true;
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}