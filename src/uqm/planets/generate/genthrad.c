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
#include "../../../options.h"


static bool GenerateThraddash_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateThraddash_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateThraddash_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateThraddash_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateThraddashFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateThraddash_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateThraddash_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateThraddash_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateThraddash_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateThraddash_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT angle;
	int planetArray[] = { PRIMORDIAL_WORLD, WATER_WORLD, TELLURIC_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;

	if (!PrimeSeed)
	{
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);
		solarSys->SunDesc[0].PlanetByte = (RandomContext_Random (SysGenRNG) % solarSys->SunDesc[0].NumPlanets);
	}
	
	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED)
	{
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = PRIMORDIAL_WORLD;

		if (PrimeSeed)
		{
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 65L / 100;
			angle = ARCTAN (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
					solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
					COSINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
					SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			ComputeSpeed (&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
		}
	}
	else if	(CurStarDescPtr->Index == THRADD_DEFINED)
	{
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = WATER_WORLD;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 0;

		if (PrimeSeed)
		{
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius = EARTH_RADIUS * 98L / 100;
			angle = ARCTAN (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x,
					solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.x =
					COSINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].location.y =
					SINE (angle, solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].radius);
			ComputeSpeed (&solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte], FALSE, 1);
		}
	}

	if (!PrimeSeed)
	{
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = planetArray[RandomContext_Random (SysGenRNG) % 2];
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random (SysGenRNG) % MAX_GEN_MOONS);
	}

	return true;
}

static bool
GenerateThraddash_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		if (StartSphereTracking (THRADDASH_SHIP)
				&& (CurStarDescPtr->Index == THRADD_DEFINED
				|| (!GET_GAME_STATE (HELIX_UNPROTECTED)
				&& (BYTE)(GET_GAME_STATE (THRADD_MISSION) - 1) >= 3)))
		{
			NotifyOthers (THRADDASH_SHIP, IPNL_ALL_CLEAR);
			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			CloneShipFragment (THRADDASH_SHIP, &GLOBAL (npc_built_ship_q),
					INFINITE_FLEET);

			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			if (CurStarDescPtr->Index == THRADD_DEFINED)
			{
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			}
			else
			{
				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			}
			InitCommunication (THRADD_CONVERSATION);

			if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				return true;

			GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
			ReinitQueue (&GLOBAL (npc_built_ship_q));
			GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

			if (CurStarDescPtr->Index == THRADD_DEFINED
					|| (!GET_GAME_STATE (HELIX_UNPROTECTED)
					&& (BYTE)(GET_GAME_STATE (THRADD_MISSION) - 1) >= 3))
				return true;

			RepairSISBorder (); // reachable if you're frendly with thraddsh and talk to them at helix world
		}
		else if (DIF_HARD 
					&& CurStarDescPtr->Index == AQUA_HELIX_DEFINED
					&& !(GET_GAME_STATE (HM_ENCOUNTERS) & 1 << THRADDASH_ENCOUNTER) 
					&& (StartSphereTracking (THRADDASH_SHIP) || !(GET_GAME_STATE(KOHR_AH_FRENZY))))
		{
			COUNT sum, i;

			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL(npc_built_ship_q)) == 0);

			if (!StartSphereTracking (THRADDASH_SHIP)
					|| GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH))
				sum = 6;
			else
				sum = 12;

			for (i = 0; i < sum; ++i)
				CloneShipFragment (THRADDASH_SHIP,
					&GLOBAL (npc_built_ship_q), 0);

			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			InitCommunication (THRADD_CONVERSATION);

			if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				return true;

			{
				BOOLEAN Survivors = GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0;

				GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue (&GLOBAL(npc_built_ship_q));
				GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

				if (Survivors)
					return true;

				{
					UWORD state;

					state = GET_GAME_STATE (HM_ENCOUNTERS);

					state |= 1 << THRADDASH_ENCOUNTER;

					SET_GAME_STATE (HM_ENCOUNTERS, state);
				}

				RepairSISBorder ();
			}
		}

		if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED
				&& !GET_GAME_STATE (AQUA_HELIX))
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (AQUA_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (AQUA_STRTAB));

			if (!PrimeSeed)
			{
				GenerateDefault_generateOrbital (solarSys, world);

				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE * 160 / 100;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -66;
				if (!DIF_HARD)
				{
					solarSys->SysInfo.PlanetInfo.Weather = 3;
					solarSys->SysInfo.PlanetInfo.Tectonics = 6;
				}
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 105;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 97;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 101;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 205;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 10;
				solarSys->SysInfo.PlanetInfo.LifeChance = 560;

				return true;
			}
		}
		else if (CurStarDescPtr->Index == THRADD_DEFINED)
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] =
					CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (LoadStringTable (RUINS_STRTAB));

			if (!PrimeSeed)
			{
				GenerateDefault_generateOrbital (solarSys, world);

				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE * 194 / 100;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 38;
				if (!DIF_HARD)
				{
					solarSys->SysInfo.PlanetInfo.Weather = 3;
					solarSys->SysInfo.PlanetInfo.Tectonics = 2;
				}
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 103;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 84;
				solarSys->SysInfo.PlanetInfo.SurfaceGravity = 86;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 252;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 21;
				solarSys->SysInfo.PlanetInfo.LifeChance = 960;

				return true;
			}
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static COUNT
GenerateThraddash_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == THRADD_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (AQUA_HELIX))
		{	// already picked up
			return 0;
		}

		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateThraddash_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	HFLEETINFO hThradd = GetStarShipFromIndex (&GLOBAL (avail_race_q), THRADDASH_SHIP);
	FLEET_INFO *ThraddPtr = LockFleetInfo (&GLOBAL (avail_race_q), hThradd);
	SIZE strength_loss;

	if (CurStarDescPtr->Index == THRADD_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	if (CurStarDescPtr->Index == AQUA_HELIX_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		assert (!GET_GAME_STATE (AQUA_HELIX) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		SET_GAME_STATE (HELIX_VISITS, 0);
		SET_GAME_STATE (AQUA_HELIX, 1);
		SET_GAME_STATE (AQUA_HELIX_ON_SHIP, 1);
		SET_GAME_STATE (HELIX_UNPROTECTED, 1);
		if (EXTENDED && ThraddPtr->allied_state == GOOD_GUY && GET_GAME_STATE (ILWRATH_FIGHT_THRADDASH)) 
		{
			SetRaceAllied (THRADDASH_SHIP, FALSE);
			RemoveEscortShips (THRADDASH_SHIP);
			strength_loss = (SIZE)(ThraddPtr->actual_strength);
			ThraddPtr->growth = (BYTE)(-strength_loss / ThraddPtr->days_left);
			ThraddPtr->growth_fract = (BYTE)(((strength_loss % ThraddPtr->days_left) << 8) / ThraddPtr->days_left);
			SET_GAME_STATE (THRADD_VISITS, 0);
		}

		return true; // picked up
	}

	(void) whichNode;
	return false;
}
