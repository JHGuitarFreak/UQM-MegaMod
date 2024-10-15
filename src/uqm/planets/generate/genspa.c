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
#include "../../build.h"
#include "../../comm.h"
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../ipdisp.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "libs/mathlib.h"


static bool GenerateSpathi_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSpathi_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateSpathi_generateName (const SOLARSYS_STATE *,
	const PLANET_DESC *world);
static bool GenerateSpathi_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateSpathi_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static COUNT GenerateSpathi_generateLife (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateSpathi_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);
static bool GenerateSpathi_pickupLife (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateSpathiFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSpathi_generatePlanets,
	/* .generateMoons    = */ GenerateSpathi_generateMoons,
	/* .generateName     = */ GenerateSpathi_generateName,
	/* .generateOrbital  = */ GenerateSpathi_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSpathi_generateEnergy,
	/* .generateLife     = */ GenerateSpathi_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSpathi_pickupEnergy,
	/* .pickupLife       = */ GenerateSpathi_pickupLife,
};


static bool
GenerateSpathi_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	PLANET_DESC *pPlanet;

	if (CurStarDescPtr->Index == SPATHI_DEFINED)
	{
		pSunDesc->NumPlanets = 1;
		pSunDesc->PlanetByte = 0;
		pSunDesc->MoonByte = 0;
		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		FillOrbits (solarSys, NUMPLANETS_PDESC, NULL, FALSE);

		pPlanet->NumPlanets = 1;

		if (PrimeSeed)
		{
			COUNT angle;

			pPlanet->radius = EARTH_RADIUS * 1150L / 100;
			angle = ARCTAN (pPlanet->location.x, pPlanet->location.y);
			pPlanet->location.x = COSINE (angle, pPlanet->radius);
			pPlanet->location.y = SINE (angle, pPlanet->radius);
			pPlanet->data_index = WATER_WORLD;
			ComputeSpeed (pPlanet, FALSE, 1);
		}
		else
		{
			pPlanet->PlanetByte = PickClosestHabitable (solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateHabitableWorld ();
		}

		if (GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
				&& !(EXTENDED && GET_GAME_STATE (KOHR_AH_FRENZY)
				&& RaceDead (ORZ_SHIP)))
		{
			pPlanet->data_index |= PLANET_SHIELDED;
		}

		return true;
	}

	GenerateDefault_generatePlanets (solarSys);

	if (EXTENDED)
	{
		if (CurStarDescPtr->Index == ALGOLITES_DEFINED)
		{
			pSunDesc->PlanetByte = PickClosestHabitable (solarSys);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateHabitableWorld ();
		}

		if (CurStarDescPtr->Index == SPATHI_MONUMENT_DEFINED)
		{
			pSunDesc->PlanetByte = PlanetByteGen (pSunDesc);
			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = GenerateCrystalWorld ();
		}
	}

	return true;
}

static bool
GenerateSpathi_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (CurStarDescPtr->Index == SPATHI_DEFINED
			&& matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET))
	{
		BYTE MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC *pMoonDesc = &solarSys->MoonDesc[MoonByte];

		if (PrimeSeed)
		{
			COUNT angle;

			pMoonDesc->data_index = PELLUCID_WORLD;
			pMoonDesc->radius = MIN_MOON_RADIUS + MOON_DELTA;
			angle = NORMALIZE_ANGLE (
					LOWORD (RandomContext_Random (SysGenRNG)));
			pMoonDesc->location.x = COSINE (angle, pMoonDesc->radius);
			pMoonDesc->location.y = SINE (angle, pMoonDesc->radius);
			ComputeSpeed (pMoonDesc, TRUE, 1);
		}
		else
			pMoonDesc->data_index = GenerateWorlds (SMALL_ROCKY);
	}

	return true;
}

static bool
GenerateSpathi_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	GenerateDefault_generateName (solarSys, world);

	if (CurStarDescPtr->Index == SPATHI_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& IsHomeworldKnown (SPATHI_HOME))
	{
		BYTE PlanetByte = solarSys->SunDesc[0].PlanetByte;
		PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 37));

		SET_GAME_STATE (BATTLE_PLANET, pPlanetDesc.data_index);
	}

	return true;
}

static bool
GenerateSpathi_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	DWORD rand_val;

	if (CurStarDescPtr->Index == SPATHI_DEFINED)
	{
		if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
		{	/* Spathiwa's moon */
			if (!GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
					&& StartSphereTracking (SPATHI_SHIP))
			{
				NotifyOthers (SPATHI_SHIP, IPNL_ALL_CLEAR);
				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (ip_group_q));
				assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

				CloneShipFragment (SPATHI_SHIP, &GLOBAL (npc_built_ship_q),
						INFINITE_FLEET);

				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
				GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				InitCommunication (SPATHI_CONVERSATION);

				if (!(GLOBAL (CurrentActivity) &
						(CHECK_ABORT | CHECK_LOAD)))
				{
					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);
				}
				return true;
			}

			DoPlanetaryAnalysis (&solarSys->SysInfo, world);
			rand_val = RandomContext_GetSeed (SysGenRNG);

			solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] =
					rand_val;
			GenerateLifeForms (&solarSys->SysInfo, GENERATE_ALL, NULL);
			rand_val = RandomContext_GetSeed (SysGenRNG);

			solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
			GenerateMineralDeposits (
					&solarSys->SysInfo, GENERATE_ALL, NULL);

			solarSys->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN] = rand_val;


			if (!DIF_HARD)
			{
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 28;
			}

			if (!GET_GAME_STATE (UMGAH_BROADCASTERS))
			{
				LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] =
						CaptureDrawable (
							LoadGraphic (UMGAH_BCS_MASK_PMAP_ANIM));
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
						CaptureStringTable (
							LoadStringTable (UMGAH_BCS_STRTAB));
				if (!GET_GAME_STATE (SPATHI_SHIELDED_SELVES))
				{	// The first report talks extensively about Spathi
					// slave-shielding selves. If they never did so, the
					// report makes no sense, so use an alternate.
					solarSys->SysInfo.PlanetInfo.DiscoveryString =
							SetAbsStringTableIndex (
							solarSys->SysInfo.PlanetInfo.DiscoveryString,
								1);
				}
			}

			LoadPlanet (NULL);
			return true;
		}
		else if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
		{
			/* visiting Spathiwa */
			DoPlanetaryAnalysis (&solarSys->SysInfo, world);
			rand_val = RandomContext_GetSeed (SysGenRNG);

			solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
			GenerateMineralDeposits (
					&solarSys->SysInfo, GENERATE_ALL, NULL);
			rand_val = RandomContext_GetSeed (SysGenRNG);

			solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] =
					rand_val;

			if (GET_GAME_STATE (KOHR_AH_FRENZY)
					&& GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
					&& RaceDead (ORZ_SHIP)
					&& EXTENDED)
			{
				solarSys->SysInfo.PlanetInfo.ScanSeed[ENERGY_SCAN] =
						rand_val;

				LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);
				solarSys->PlanetSideFrame[1] =
					CaptureDrawable(LoadGraphic(RUINS_MASK_PMAP_ANIM));
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable(LoadStringTable(RUINS_STRTAB));
			}

			solarSys->SysInfo.PlanetInfo.PlanetRadius = 120;
			solarSys->SysInfo.PlanetInfo.SurfaceGravity = 23;
			if (!DIF_HARD)
			{
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 31;
			}

			LoadPlanet (NULL);

			return true;
		}
	}

	if (CurStarDescPtr->Index == ALGOLITES_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (RUINS_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (
					LoadStringTable (ALGOLITE_RUINS_STRTAB));
	}

	if (CurStarDescPtr->Index == SPATHI_MONUMENT_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (
					LoadGraphic (MONUMENT_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (
					LoadStringTable (SPATHI_MONUMENT_STRTAB));
	}

	GenerateDefault_generateOrbital (solarSys, world);

	if (CurStarDescPtr->Index == ALGOLITES_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
		solarSys->SysInfo.PlanetInfo.Weather = 0;
	}

	return true;
}

static COUNT
GenerateSpathi_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == SPATHI_DEFINED)
	{
		if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
		{
			// This check is redundant since the retrieval bit will keep
			// the node from showing up again
			if (GET_GAME_STATE (UMGAH_BROADCASTERS))
			{	// already picked up
				return 0;
			}

			return GenerateDefault_generateArtifact (
					solarSys, whichNode, info);
		}
		else if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
				&& GET_GAME_STATE (KOHR_AH_FRENZY) && RaceDead (ORZ_SHIP)
				&& GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
				&& EXTENDED)
		{
			return GenerateRandomNodes (&solarSys->SysInfo, ENERGY_SCAN, 4,
					0, whichNode, info);
		}
	}

	if (CurStarDescPtr->Index == ALGOLITES_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		return GenerateRandomNodes (&solarSys->SysInfo, ENERGY_SCAN, 6,
				0, whichNode, info);
	}

	if (CurStarDescPtr->Index == SPATHI_MONUMENT_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		return GenerateDefault_generateArtifact (
				solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateSpathi_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == SPATHI_DEFINED)
	{
		if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
		{
			assert (!GET_GAME_STATE (UMGAH_BROADCASTERS) && whichNode == 0);

			GenerateDefault_landerReport (solarSys);
			SetLanderTakeoff ();

			SET_GAME_STATE (UMGAH_BROADCASTERS, 1);
			SET_GAME_STATE (UMGAH_BROADCASTERS_ON_SHIP, 1);

			return true; // picked up
		}
		else if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
				&& GET_GAME_STATE (KOHR_AH_FRENZY) && RaceDead (ORZ_SHIP)
				&& GET_GAME_STATE (SPATHI_SHIELDED_SELVES)
				&& EXTENDED)
		{
			GenerateDefault_landerReportCycle (solarSys);

			return false; // do not remove the node
		}
	}

	if ((CurStarDescPtr->Index == ALGOLITES_DEFINED
			|| CurStarDescPtr->Index == SPATHI_MONUMENT_DEFINED)
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET)
			&& EXTENDED)
	{
		GenerateDefault_landerReportCycle (solarSys);

		return false; // do not remove the node
	}

	(void) whichNode;
	return false;
}

static COUNT
GenerateSpathi_generateLife (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (CurStarDescPtr->Index == SPATHI_DEFINED
			&& matchWorld(solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		#define NUM_EVIL_ONES  32
		return GenerateRandomNodes (&solarSys->SysInfo, BIOLOGICAL_SCAN,
				NUM_EVIL_ONES, EVIL_ONE, whichNode, info);
	}

	return 0;
}

static bool
GenerateSpathi_pickupLife (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (CurStarDescPtr->Index == SPATHI_DEFINED
		&& matchWorld(solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		assert (!GET_GAME_STATE (SPATHI_CREATURES_ELIMINATED) &&
				!GET_GAME_STATE (SPATHI_SHIELDED_SELVES));

		SET_GAME_STATE (SPATHI_CREATURES_EXAMINED, 1);
		if (countNodesRetrieved (
				&solarSys->SysInfo.PlanetInfo, BIOLOGICAL_SCAN)
				+ 1 == NUM_EVIL_ONES)
		{	// last creature picked up
			SET_GAME_STATE (SPATHI_CREATURES_ELIMINATED, 1);
		}

		return true; // picked up
	}

	return GenerateDefault_pickupLife (solarSys, world, whichNode);
}