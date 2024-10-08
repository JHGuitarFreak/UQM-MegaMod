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
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"

static bool GenerateSyreen_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSyreen_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateSyreen_generateName (const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateSyreen_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateSyreen_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateSyreen_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateSyreenFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSyreen_generatePlanets,
	/* .generateMoons    = */ GenerateSyreen_generateMoons,
	/* .generateName     = */ GenerateSyreen_generateName,
	/* .generateOrbital  = */ GenerateSyreen_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSyreen_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSyreen_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateSyreen_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC *pPlanet;

	pSunDesc->MoonByte = 0;

	GenerateDefault_generatePlanets (solarSys);

	if (PrimeSeed)
	{
		pSunDesc->PlanetByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = WATER_WORLD; // | PLANET_SHIELDED;
		pPlanet->NumPlanets = 1;
	}
	else
	{
		pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		pPlanet->data_index = GenerateHabitableWorld ();

		if (!pPlanet->NumPlanets)
			pPlanet->NumPlanets++;
	}

	if (!RaceDead (SYREEN_SHIP))
		pPlanet->data_index |= PLANET_SHIELDED;

	return true;
}

static bool
GenerateSyreen_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET))
	{
		BYTE MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC *pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (!RaceDead (SYREEN_SHIP))
			pMoonDesc->data_index = HIERARCHY_STARBASE;
		else
			pMoonDesc->data_index = DESTROYED_STARBASE;

		if (PrimeSeed || StarSeed)
		{
			pMoonDesc->radius = MIN_MOON_RADIUS;
			pMoonDesc->location.x = COSINE (QUADRANT, pMoonDesc->radius);
			pMoonDesc->location.y = SINE (QUADRANT, pMoonDesc->radius);
			ComputeSpeed (pMoonDesc, TRUE, 1);
		}
	}

	return true;
}

static bool
GenerateSyreen_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if ((GET_GAME_STATE (SYREEN_HOME_VISITS)
			|| GET_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON))
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 39));
		SET_GAME_STATE (BATTLE_PLANET,
			solarSys->PlanetDesc[
				solarSys->SunDesc[0].PlanetByte].data_index);
	}
	else
		GenerateDefault_generateName (solarSys, world);

	return true;
}

static bool
GenerateSyreen_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{

	if (matchWorld (solarSys, world,
			MATCH_PBYTE, MATCH_PLANET))
	{	/* Gaia */
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (RUINS_STRTAB));

		GenerateDefault_generateOrbital (solarSys, world);

		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 19;
		solarSys->SysInfo.PlanetInfo.AtmoDensity =
				EARTH_ATMOSPHERE * 9 / 10;

		if (!DIF_HARD)
		{
			solarSys->SysInfo.PlanetInfo.Tectonics = 0;
			solarSys->SysInfo.PlanetInfo.Weather = 0;
		}

		return true;
	}

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		if (!RaceDead (SYREEN_SHIP))
		{
			/* Starbase */
			InitCommunication (SYREEN_CONVERSATION);

			return true;
		}
		else
		{
			/* Starbase */
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (
						LoadStringTable (SYREEN_BASE_STRTAB)
					);

			DoDiscoveryReport (MenuSounds);

			DestroyStringTable (ReleaseStringTable (
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
			solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			FreeLanderFont (&solarSys->SysInfo.PlanetInfo);

			return true;
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateSyreen_generateEnergy (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateSyreen_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
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