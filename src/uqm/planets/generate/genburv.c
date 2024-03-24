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
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../resinst.h"
#include "libs/mathlib.h"


static bool GenerateBurvixese_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateBurvixese_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateBurvixese_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateBurvixese_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateBurvixese_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateBurvixeseFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateBurvixese_generatePlanets,
	/* .generateMoons    = */ GenerateBurvixese_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateBurvixese_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateBurvixese_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateBurvixese_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateBurvixese_generatePlanets (SOLARSYS_STATE *solarSys)
{
	solarSys->SunDesc[0].PlanetByte = 0;
	if (PrimeSeed)
	{
			COUNT angle;

		GenerateDefault_generatePlanets (solarSys);

		solarSys->PlanetDesc[0].data_index = REDUX_WORLD;
		solarSys->PlanetDesc[0].NumPlanets = 1;
		solarSys->PlanetDesc[0].radius = EARTH_RADIUS * 39L / 100;
		angle = ARCTAN (solarSys->PlanetDesc[0].location.x,
				solarSys->PlanetDesc[0].location.y);
		solarSys->PlanetDesc[0].location.x =
				COSINE (angle, solarSys->PlanetDesc[0].radius);
		solarSys->PlanetDesc[0].location.y =
				SINE (angle, solarSys->PlanetDesc[0].radius);
		ComputeSpeed (&solarSys->PlanetDesc[0], FALSE, 1);
	}
	else
	{
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS) + 1);
		FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index =
			GenerateRockyWorld (LARGE_ROCKY);
		GeneratePlanets (solarSys);
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;
		CheckForHabitable (solarSys);
	}

	return true;
}

static bool
GenerateBurvixese_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (matchWorld (solarSys, planet, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (PrimeSeed)
		{
			COUNT angle;
			DWORD rand_val;

			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = SELENIC_WORLD;
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius = MIN_MOON_RADIUS
					+ (MAX_GEN_MOONS - 1) * MOON_DELTA;
			rand_val = RandomContext_Random (SysGenRNG);
			angle = NORMALIZE_ANGLE (LOWORD (rand_val));
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].location.x =
					COSINE (angle, solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius);
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].location.y =
					SINE (angle, solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].radius);
			ComputeSpeed (&solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte], TRUE, 1);
		}
		else
		{			
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = 
				GenerateRockyWorld (SMALL_ROCKY);
		}
	}
	return true;
}

static bool
GenerateBurvixese_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	DWORD rand_val;

	DoPlanetaryAnalysis (&solarSys->SysInfo, world);
	rand_val = RandomContext_GetSeed (SysGenRNG);

	solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] = rand_val;
	GenerateLifeForms (&solarSys->SysInfo, GENERATE_ALL, NULL);
	rand_val = RandomContext_GetSeed (SysGenRNG);

	solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
	GenerateMineralDeposits (&solarSys->SysInfo, GENERATE_ALL, NULL);

	solarSys->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN] = rand_val;

	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (
				LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (
						LoadStringTable (BURV_RUINS_STRTAB));
		if (!DIF_HARD)
		{
			solarSys->SysInfo.PlanetInfo.Weather = 0;
			solarSys->SysInfo.PlanetInfo.Tectonics = 0;
		}
		if (!PrimeSeed)
		{
			solarSys->SysInfo.PlanetInfo.AtmoDensity =
				EARTH_ATMOSPHERE * 30 / 100;
			solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 18;
			solarSys->SysInfo.PlanetInfo.PlanetDensity = 96;
			solarSys->SysInfo.PlanetInfo.PlanetRadius = 103;
			solarSys->SysInfo.PlanetInfo.SurfaceGravity = 98;
			solarSys->SysInfo.PlanetInfo.RotationPeriod = 172;
			solarSys->SysInfo.PlanetInfo.AxialTilt = 6;
			solarSys->SysInfo.PlanetInfo.LifeChance = 860;
		}
	}
	else if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte)
			&& !GET_GAME_STATE (BURVIXESE_BROADCASTERS))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] = CaptureDrawable (
				LoadGraphic (BURV_BCS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (BURV_BCS_STRTAB));

		solarSys->SysInfo.PlanetInfo.AtmoDensity =
			EARTH_ATMOSPHERE * 30 / 100;
		solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 23;
		if (!DIF_HARD)
		{
			solarSys->SysInfo.PlanetInfo.Weather = 2;
			solarSys->SysInfo.PlanetInfo.Tectonics = 1;
		}
		solarSys->SysInfo.PlanetInfo.PlanetDensity = 61;
		solarSys->SysInfo.PlanetInfo.PlanetRadius = 30;
		solarSys->SysInfo.PlanetInfo.SurfaceGravity = 18;
		solarSys->SysInfo.PlanetInfo.RotationPeriod = 156;
		solarSys->SysInfo.PlanetInfo.AxialTilt = 20;
		solarSys->SysInfo.PlanetInfo.LifeChance = 860;
	}

	LoadPlanet (NULL);

	return true;
}

static COUNT
GenerateBurvixese_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (BURVIXESE_BROADCASTERS))
		{	// already picked up
			return 0;
		}
		
		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateBurvixese_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		assert (!GET_GAME_STATE (BURVIXESE_BROADCASTERS) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (BURVIXESE_BROADCASTERS, 1);
		SET_GAME_STATE (BURV_BROADCASTERS_ON_SHIP, 1);

		return true; // picked up
	}

	(void) whichNode;
	return false;
}
