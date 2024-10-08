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
#include "../../gendef.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateOrz_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateOrz_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateOrz_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateOrz_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateOrz_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateOrzFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateOrz_generatePlanets,
	/* .generateMoons    = */ GenerateOrz_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateOrz_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateOrz_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateOrz_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateOrz_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pPlanet;
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];

	GenerateDefault_generatePlanets (solarSys);


	if (CurStarDescPtr->Index == ORZ_DEFINED)
	{
		if (PrimeSeed)
		{
			COUNT angle;

			pSunDesc->PlanetByte = 0;
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = WATER_WORLD;
			pPlanet->radius = EARTH_RADIUS * 156L / 100;
			pPlanet->NumPlanets = 0;
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
	}

	if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED)
	{
		pSunDesc->PlanetByte = 1;
		pSunDesc->MoonByte = 2;

		if (!PrimeSeed)
		{
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateGasGiantWorld ();

			if (StarSeed)
				pSunDesc->MoonByte = PlanetByteGen (pPlanet);

			if (pPlanet->NumPlanets < (pSunDesc->MoonByte + 1))
				pPlanet->NumPlanets = pSunDesc->MoonByte + 1;
		}

	}

	return true;
}

static bool
GenerateOrz_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED
			&& matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		BYTE MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC *pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = GenerateCrystalWorld ();
	}

	return true;
}

static bool
GenerateOrz_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if ((CurStarDescPtr->Index == ORZ_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
			|| (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE)
			&& !GET_GAME_STATE (TAALO_PROTECTOR)))
	{
		COUNT i;

		if ((CurStarDescPtr->Index == ORZ_DEFINED
				|| !GET_GAME_STATE (TAALO_UNPROTECTED))
				&& StartSphereTracking (ORZ_SHIP))
		{
			NotifyOthers (ORZ_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			if (CurStarDescPtr->Index == ORZ_DEFINED)
			{
				CloneShipFragment (ORZ_SHIP,
						&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			}
			else
			{
				for (i = 0; i < 14; ++i)
				{
					CloneShipFragment (ORZ_SHIP,
							&GLOBAL (npc_built_ship_q), 0);
				}
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			}
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			InitCommunication (ORZ_CONVERSATION);

			if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				return true;

			{
				BOOLEAN OrzSurvivors;

				OrzSurvivors = GetHeadLink (&GLOBAL (npc_built_ship_q))
						&& (CurStarDescPtr->Index == ORZ_DEFINED
						|| !GET_GAME_STATE (TAALO_UNPROTECTED));

				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL (npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

				if (OrzSurvivors)
					return true;

				RepairSISBorder ();
			}
		}

		SET_GAME_STATE (TAALO_UNPROTECTED, 1);
		if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED)
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (
					LoadGraphic (TAALO_DEVICE_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (
					LoadStringTable (TAALO_DEVICE_STRTAB));

			if (false)
			{
				GenerateDefault_generateOrbital (solarSys, world);

				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -101;
				if (!DIF_HARD)
				{
					solarSys->SysInfo.PlanetInfo.Weather = 0;
					solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				}
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 200;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 27;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 54;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 199;
				solarSys->SysInfo.PlanetInfo.AxialTilt = -8;
				solarSys->SysInfo.PlanetInfo.LifeChance = -740;

				return true;
			}
		}
		else
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (RUINS_STRTAB));

			if (false)
			{
				GenerateDefault_generateOrbital (solarSys, world);

				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE * 160 / 100;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 44;
				if (!DIF_HARD)
				{
					solarSys->SysInfo.PlanetInfo.Weather = 3;
					solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				}
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 103;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 84;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 86;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 189;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 4;
				solarSys->SysInfo.PlanetInfo.LifeChance = 960;

				return true;
			}
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateOrz_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED
		&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (TAALO_PROTECTOR))
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (
				solarSys, whichNode, info);
	}

	if (CurStarDescPtr->Index == ORZ_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateOrz_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == TAALO_PROTECTOR_DEFINED
		&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		assert (!GET_GAME_STATE (TAALO_PROTECTOR) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (TAALO_PROTECTOR, 1);
		SET_GAME_STATE (TAALO_PROTECTOR_ON_SHIP, 1);

		return true; // picked up
	}

	if (CurStarDescPtr->Index == ORZ_DEFINED
		&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	(void) whichNode;
	return false;
}