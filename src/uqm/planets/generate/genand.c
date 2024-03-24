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
#include "../../globdata.h"
#include "../../gendef.h"
#include "../../nameref.h"
#include "../../resinst.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "libs/mathlib.h"
#include "../../build.h"


static bool GenerateAndrosynth_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateAndrosynth_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateAndrosynth_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateAndrosynth_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateAndrosynthFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateAndrosynth_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateAndrosynth_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateAndrosynth_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateAndrosynth_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateAndrosynth_generatePlanets (SOLARSYS_STATE *solarSys)
{
	if (CurStarDescPtr->Index == ANDROSYNTH_DEFINED)
	{
		solarSys->SunDesc[0].PlanetByte = 1;
		if (PrimeSeed)
		{
			COUNT angle;

			GenerateDefault_generatePlanets (solarSys);

			solarSys->PlanetDesc[1].data_index = TELLURIC_WORLD;
			solarSys->PlanetDesc[1].radius = EARTH_RADIUS * 204L / 100;
			angle = ARCTAN (solarSys->PlanetDesc[1].location.x,
					solarSys->PlanetDesc[1].location.y);
			solarSys->PlanetDesc[1].location.x =
					COSINE (angle, solarSys->PlanetDesc[1].radius);
			solarSys->PlanetDesc[1].location.y =
					SINE (angle, solarSys->PlanetDesc[1].radius);
			// Recompute because we changed radius
			ComputeSpeed (&solarSys->PlanetDesc[1], FALSE, 1);
		}
		else
		{
			BYTE pArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };
			solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 2);

			FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
			solarSys->PlanetDesc[1].data_index =
					pArray[RandomContext_Random (SysGenRNG) % ARRAY_SIZE (pArray)];
			GeneratePlanets (solarSys);
			CheckForHabitable (solarSys);
		}
	}
	else if (CurStarDescPtr->Index == EXCAVATION_SITE_DEFINED)
	{
		solarSys->SunDesc[0].PlanetByte = 0;
		if (PrimeSeed)
			GenerateDefault_generatePlanets (solarSys);
		else
		{
			solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS) + 1);

			FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
			solarSys->PlanetDesc[0].data_index = GenerateRockyWorld (ALL_ROCKY);
			GeneratePlanets (solarSys);
		}
	}
	else // Just to be safe
		GenerateDefault_generatePlanets (solarSys);

	return true;
}

static bool
GenerateAndrosynth_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (CurStarDescPtr->Index == ANDROSYNTH_DEFINED)
	{
		if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
		{
			COUNT i;
			COUNT visits = 0;

			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (
					LoadStringTable (ANDROSYNTH_RUINS_STRTAB));
			// Androsynth ruins are a special case. The DiscoveryString contains
			// several lander reports which form a story. Each report is given
			// when the player collides with a new city ruin. Ruins previously
			// visited are marked in the upper 16 bits of ScanRetrieveMask, and
			// the lower bits are cleared to keep the ruin nodes on the map.
			for (i = 16; i < 32; ++i)
			{
				if (isNodeRetrieved (&solarSys->SysInfo.PlanetInfo, ENERGY_SCAN, i))
					++visits;
			}
			if (visits >= GetStringTableCount (
					solarSys->SysInfo.PlanetInfo.DiscoveryString))
			{	// All the reports were already given
				DestroyStringTable (ReleaseStringTable (
						solarSys->SysInfo.PlanetInfo.DiscoveryString));
				solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			}
			else
			{	// Advance the report sequence to the first unread
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
						SetRelStringTableIndex (
						solarSys->SysInfo.PlanetInfo.DiscoveryString, visits);
			}

			GenerateDefault_generateOrbital (solarSys, world);

			solarSys->SysInfo.PlanetInfo.AtmoDensity =
					EARTH_ATMOSPHERE * 144 / 100;
			solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;
			if (!DIF_HARD)
			{
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
			}
			if (!PrimeSeed)
			{
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 104;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 94;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 97;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 259;
				solarSys->SysInfo.PlanetInfo.AxialTilt = -22;
				solarSys->SysInfo.PlanetInfo.LifeChance = 560;
			}

			return true;
		}
	}

	if (CurStarDescPtr->Index == EXCAVATION_SITE_DEFINED)
	{
		if (EXTENDED &&
			matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (EXCAVATION_SITE_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (EXCAVATION_SITE_STRTAB));

			GenerateDefault_generateOrbital (solarSys, world);

			solarSys->SysInfo.PlanetInfo.AtmoDensity =
					EARTH_ATMOSPHERE * 126 / 100;
			solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 9;
			if (!DIF_HARD)
			{
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 2;
			}
			if (!PrimeSeed)
			{
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 146;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 85;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 82;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 191;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 25;
			}

			return true;
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static bool
GenerateAndrosynth_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == ANDROSYNTH_DEFINED &&
		matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		PLANET_INFO *planetInfo = &solarSys->SysInfo.PlanetInfo;

		// Ruins previously visited are marked in the upper 16 bits
		if (isNodeRetrieved (planetInfo, ENERGY_SCAN, whichNode + 16))
			return false; // already visited this ruin, do not remove

		setNodeRetrieved (planetInfo, ENERGY_SCAN, whichNode + 16);
		// We set the retrieved bit manually here and need to indicate
		// the change to the solar system state functions
		SET_GAME_STATE (PLANETARY_CHANGE, 1);

		// Androsynth ruins have several lander reports which form a story
		GenerateDefault_landerReportCycle (solarSys);
		
		// "Kill" the Androsynth once you learn they are gone
		if (CheckAlliance (ANDROSYNTH_SHIP) != DEAD_GUY)
			KillRace (ANDROSYNTH_SHIP);

		return false; // do not remove the node from the surface
	}

	if (EXTENDED
		&& CurStarDescPtr->Index == EXCAVATION_SITE_DEFINED
		&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		GenerateDefault_landerReportCycle (solarSys);

		return false; // do not remove the node
	}

	return false;
}

static COUNT
GenerateAndrosynth_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == ANDROSYNTH_DEFINED &&
		matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	if (EXTENDED
		&& CurStarDescPtr->Index == EXCAVATION_SITE_DEFINED
		&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateRandomNodes (&solarSys->SysInfo, ENERGY_SCAN, 1,
				0, whichNode, info);
	}

	return 0;
}
