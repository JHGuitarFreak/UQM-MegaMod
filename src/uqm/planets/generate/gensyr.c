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
	int planetArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;
	solarSys->SunDesc[0].MoonByte = 0;

	if (!PrimeSeed)
	{
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
		solarSys->SunDesc[0].PlanetByte = (RandomContext_Random (SysGenRNG) % solarSys->SunDesc[0].NumPlanets);
	}

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);	

	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = WATER_WORLD;
	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].alternate_colormap = NULL;
	solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;

	if (!PrimeSeed)
	{
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2];
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
		solarSys->SunDesc[0].MoonByte = (RandomContext_Random (SysGenRNG) % solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets);
	}

	if (CheckAlliance (SYREEN_SHIP) != DEAD_GUY)
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index |= PLANET_SHIELDED;

	return true;
}

static bool
GenerateSyreen_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (matchWorld (solarSys, planet, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (CheckAlliance (SYREEN_SHIP) != DEAD_GUY)
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = HIERARCHY_STARBASE;
		else
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = DESTROYED_STARBASE;
		solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].alternate_colormap = NULL;

		if (PrimeSeed)
		{
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius = MIN_MOON_RADIUS;
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].location.x =
					COSINE (QUADRANT, solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius);
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].location.y =
					SINE (QUADRANT, solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius);
			ComputeSpeed (&solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte], TRUE, 1);
		}
	}

	return true;
}

static bool
GenerateSyreen_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	if ((GET_GAME_STATE (SYREEN_HOME_VISITS) || GET_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON))
		&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		utf8StringCopy (GLOBAL_SIS (PlanetName), sizeof (GLOBAL_SIS (PlanetName)),
			GAME_STRING (PLANET_NUMBER_BASE + 39));
		SET_GAME_STATE (BATTLE_PLANET, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index);
	}
	else
		GenerateDefault_generateName (solarSys, world);

	return true;
}

static bool
GenerateSyreen_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{

	if (matchWorld (solarSys, world,
			solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (RUINS_STRTAB));

		GenerateDefault_generateOrbital (solarSys, world);

		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 19;
		solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		solarSys->SysInfo.PlanetInfo.Weather = 0;
		solarSys->SysInfo.PlanetInfo.AtmoDensity = EARTH_ATMOSPHERE * 9 / 10;

		return true;
	}

	if (matchWorld (solarSys, world,
			solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		if (CheckAlliance (SYREEN_SHIP) != DEAD_GUY)
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
				SetRelStringTableIndex (
					CaptureStringTable (
						LoadStringTable (URQUAN_BASE_STRTAB)), 0);

			DoDiscoveryReport (MenuSounds);

			DestroyStringTable (ReleaseStringTable (
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
			solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			FreeLanderFont (&solarSys->SysInfo.PlanetInfo);
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateSyreen_generateEnergy (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateSyreen_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
	COUNT whichNode)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	(void) whichNode;
	return false;
}