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
#include "../../globdata.h"
#include "../../nameref.h"
#include "../../setup.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"

static bool GenerateChmmr_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateChmmr_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateChmmr_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateChmmr_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateChmmr_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateChmmrFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateChmmr_generatePlanets,
	/* .generateMoons    = */ GenerateChmmr_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateChmmr_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateChmmr_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateChmmr_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateChmmr_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pPlanet;
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];

	if (CurStarDescPtr->Index == CHMMR_DEFINED)
	{
		pSunDesc->PlanetByte = 1;
		pSunDesc->MoonByte = 0;

		if (PrimeSeed)
		{
			GenerateDefault_generatePlanets (solarSys);

			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			pPlanet->data_index = SAPPHIRE_WORLD;
			pPlanet->NumPlanets = 1;
		}
		else
		{
			pSunDesc->NumPlanets = GenerateMinPlanets (2);

			if (StarSeed)
				pSunDesc->PlanetByte = PlanetByteGen (pSunDesc);

			pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

			FillOrbits (solarSys, NUMPLANETS_PDESC, NULL, FALSE);

			pPlanet->data_index = GenerateCrystalWorld ();

			GeneratePlanets (solarSys);

			pPlanet->NumPlanets += 1;
		}

		if (!GET_GAME_STATE (CHMMR_UNLEASHED))
			pPlanet->data_index |= PLANET_SHIELDED;
	}

	if (CurStarDescPtr->Index == MOTHER_ARK_DEFINED)
	{
		GenerateDefault_generatePlanets (solarSys);

		pSunDesc->PlanetByte = 3;
		if (!PrimeSeed)
			pSunDesc->PlanetByte = PlanetByteGen (pSunDesc);

		pPlanet = &solarSys->PlanetDesc[pSunDesc->PlanetByte];

		if (EXTENDED)
			pPlanet->data_index = GenerateCrystalWorld ();
	}

	return true;
}

static bool
GenerateChmmr_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (CurStarDescPtr->Index == CHMMR_DEFINED
			&& matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET))
	{
		COUNT angle;
		DWORD rand_val;
		PLANET_DESC *pMoonDesc =
				&solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte];

		if (!RaceDead (CHMMR_SHIP))
			pMoonDesc->data_index = HIERARCHY_STARBASE;
		else
			pMoonDesc->data_index = DESTROYED_STARBASE;

		if (PrimeSeed || StarSeed)
		{
			pMoonDesc->radius = MIN_MOON_RADIUS;
			rand_val = RandomContext_Random (SysGenRNG);
			angle = NORMALIZE_ANGLE (LOWORD (rand_val));
			pMoonDesc->location.x = COSINE (angle, pMoonDesc->radius);
			pMoonDesc->location.y = SINE (angle, pMoonDesc->radius);
			ComputeSpeed (pMoonDesc, TRUE, 1);
		}
	}

	return true;
}

static bool
GenerateChmmr_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (CurStarDescPtr->Index == CHMMR_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		BOOLEAN HardModeBS = DIF_HARD && !GET_GAME_STATE (KOHR_AH_FRENZY)
			&& !(GET_GAME_STATE (HM_ENCOUNTERS) & 1 << ILWRATH_ENCOUNTER);

		if (RaceDead (CHMMR_SHIP))
		{
			LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
			solarSys->PlanetSideFrame[1] = CaptureDrawable (
					LoadGraphic (RUINS_MASK_PMAP_ANIM));
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					CaptureStringTable (
						LoadStringTable (CHMMR_HOME_STRTAB));

			GenerateDefault_generateOrbital (solarSys, world);

			return true;
		}

		if (GET_GAME_STATE (CHMMR_UNLEASHED))
		{
			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 7);
			InitCommunication (CHMMR_CONVERSATION);

			if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
			{
				GLOBAL (CurrentActivity) |= END_INTERPLANETARY;
			}

			return true;
		}
		else if (GET_GAME_STATE (SUN_DEVICE_ON_SHIP)
				&& ((!GET_GAME_STATE (ILWRATH_DECEIVED)
				&& StartSphereTracking (ILWRATH_SHIP))
				|| HardModeBS ))
		{
			BOOLEAN Survivors;
			UWORD state;

			PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue (&GLOBAL (ip_group_q));
			assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

			if (GET_GAME_STATE (ILWRATH_DECEIVED) && DIF_HARD)
			{
				COUNT lim, i;

				if (StartSphereTracking (ILWRATH_SHIP))
					lim = 14;
				else
					lim = 6;

				for (i = 0; i < lim; ++i)
				{
					CloneShipFragment (ILWRATH_SHIP,
							&GLOBAL (npc_built_ship_q), 0);
				}
			}
			else
			{
				CloneShipFragment (ILWRATH_SHIP,
						&GLOBAL (npc_built_ship_q), INFINITE_FLEET);
			}

			SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
			GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
			InitCommunication (ILWRATH_CONVERSATION);

			if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
				return true;

			Survivors = GetHeadLink (&GLOBAL (npc_built_ship_q)) != 0;

			GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
			ReinitQueue (&GLOBAL (npc_built_ship_q));
			GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

			if (Survivors)
				return true;

			state = GET_GAME_STATE (HM_ENCOUNTERS);
			state |= 1 << ILWRATH_ENCOUNTER;
			SET_GAME_STATE (HM_ENCOUNTERS, state);

			RepairSISBorder ();

			return true;
		}
	}

	if (CurStarDescPtr->Index == CHMMR_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		STRING str;

		/* Starbase */
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);

		if (RaceDead (CHMMR_SHIP))
		{
			str = SetRelStringTableIndex (CaptureStringTable (
					LoadStringTable (URQUAN_BASE_STRTAB)), 0);
		}
		else
			str = CaptureStringTable (LoadStringTable (CHMMR_BASE_STRTAB));

		solarSys->SysInfo.PlanetInfo.DiscoveryString = str;

		DoDiscoveryReport (MenuSounds);

		DestroyStringTable (ReleaseStringTable (
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
		solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
		FreeLanderFont (&solarSys->SysInfo.PlanetInfo);

		return true;
	}

	if (EXTENDED && CurStarDescPtr->Index == MOTHER_ARK_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		BOOLEAN MelnormeInfo =
				GET_GAME_STATE (MELNORME_ALIEN_INFO_STACK) >= 8;
		BOOLEAN ChmmrStack = GET_GAME_STATE (CHMMR_HOME_VISITS);

		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable (LoadGraphic (MOTHER_ARK_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
			CaptureStringTable (LoadStringTable (MOTHER_ARK_STRTAB));

		if (MelnormeInfo || ChmmrStack)
		{
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					SetRelStringTableIndex (
						solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateChmmr_generateEnergy (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{

	if (CurStarDescPtr->Index == CHMMR_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		// Standard ruins report
		return GenerateDefault_generateRuins (solarSys, whichNode, info);
	}

	if (EXTENDED && CurStarDescPtr->Index == MOTHER_ARK_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateChmmr_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
	COUNT whichNode)
{

	if (CurStarDescPtr->Index == CHMMR_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		// Standard ruins report
		GenerateDefault_landerReportCycle (solarSys);
		return false;
	}

	if (EXTENDED && CurStarDescPtr->Index == MOTHER_ARK_DEFINED
			&& matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		assert (whichNode == 0);

		GenerateDefault_landerReport (solarSys);

		// The Ark cannot be "picked up". It is always on the surface.
		return false;
	}

	(void)whichNode;
	return false;
}