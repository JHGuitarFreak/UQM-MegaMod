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
#include "../../build.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../istrtab.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../state.h"
#include "../../gamestr.h"
#include "../lander.h"
#include "../planets.h"


static bool GenerateShofixti_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateShofixti_reinitNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateShofixti_uninitNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateShofixti_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateShofixti_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateShofixti_generateName (const SOLARSYS_STATE *,
		const PLANET_DESC *world);
static bool GenerateShofixti_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);

static void check_old_shofixti (void);


const GenerateFunctions generateShofixtiFunctions = {
	/* .initNpcs         = */ GenerateShofixti_initNpcs,
	/* .reinitNpcs       = */ GenerateShofixti_reinitNpcs,
	/* .uninitNpcs       = */ GenerateShofixti_uninitNpcs,
	/* .generatePlanets  = */ GenerateShofixti_generatePlanets,
	/* .generateMoons    = */ GenerateShofixti_generateMoons,
	/* .generateName     = */ GenerateShofixti_generateName,
	/* .generateOrbital  = */ GenerateShofixti_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateShofixti_initNpcs (SOLARSYS_STATE *solarSys)
{
	if (!GET_GAME_STATE (SHOFIXTI_RECRUITED)
			&& (!GET_GAME_STATE (SHOFIXTI_KIA)
			|| (!GET_GAME_STATE (SHOFIXTI_BRO_KIA)
			&& GET_GAME_STATE (MAIDENS_ON_SHIP))))
	{
		GLOBAL (BattleGroupRef) = GET_GAME_STATE (SHOFIXTI_GRPOFFS);
		if (GLOBAL (BattleGroupRef) == 0
				|| !GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP))
		{
			HSHIPFRAG hStarShip;

			if (GLOBAL (BattleGroupRef) == 0)
				GLOBAL (BattleGroupRef) = ~0L;

			hStarShip = CloneShipFragment (SHOFIXTI_SHIP,
					&GLOBAL (npc_built_ship_q), 1);
			if (hStarShip)
			{	/* Set old Shofixti name; his brother if Tanaka died */
				SHIP_FRAGMENT *FragPtr = LockShipFrag (
						&GLOBAL (npc_built_ship_q), hStarShip);
				/* Name Tanaka or Katana (+1) */
				FragPtr->captains_name_index =
						NAME_OFFSET + NUM_CAPTAINS_NAMES +
						(GET_GAME_STATE (SHOFIXTI_KIA) & 1);
				UnlockShipFrag (&GLOBAL (npc_built_ship_q), hStarShip);
			}

			GLOBAL (BattleGroupRef) = PutGroupInfo (
					GLOBAL (BattleGroupRef), 1);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
			SET_GAME_STATE (SHOFIXTI_GRPOFFS, GLOBAL (BattleGroupRef));
		}
	}

	// This was originally a fallthrough to REINIT_NPCS.
	// XXX: is the call to check_old_shofixti() needed?
	GenerateDefault_initNpcs (solarSys);
	check_old_shofixti ();

	return true;
}

static bool
GenerateShofixti_reinitNpcs (SOLARSYS_STATE *solarSys)
{
	GenerateDefault_reinitNpcs (solarSys);
	check_old_shofixti ();

	(void) solarSys;
	return true;
}

static bool
GenerateShofixti_uninitNpcs (SOLARSYS_STATE *solarSys)
{
	if (GLOBAL (BattleGroupRef)
			&& !GET_GAME_STATE (SHOFIXTI_RECRUITED)
			&& GetHeadLink (&GLOBAL (ip_group_q)) == 0)
	{
		if (!GET_GAME_STATE (SHOFIXTI_KIA))
		{
			SET_GAME_STATE (SHOFIXTI_KIA, 1);
			SET_GAME_STATE (SHOFIXTI_VISITS, 0);
			if (DIF_HARD)
				SET_GAME_STATE(SHOFIXTI_BRO_KIA, 1);
		}
		else if (GET_GAME_STATE (MAIDENS_ON_SHIP))
		{
			SET_GAME_STATE (SHOFIXTI_BRO_KIA, 1);
		}
	}

	GenerateDefault_uninitNpcs (solarSys);
	return true;
}

static bool
GenerateShofixti_generatePlanets (SOLARSYS_STATE *solarSys)
{
	PLANET_DESC *pSunDesc = &solarSys->SunDesc[0];
	COUNT i;

	pSunDesc->PlanetByte = 0;
	pSunDesc->MoonByte = 0;

	if (PrimeSeed)
	{
#define NUM_PLANETS 6
		pSunDesc->NumPlanets = NUM_PLANETS;
		for (i = 0; i < NUM_PLANETS; ++i)
		{
			PLANET_DESC *pCurDesc = &solarSys->PlanetDesc[i];

			pCurDesc->NumPlanets = 0;
			if (i < (NUM_PLANETS >> 1))
				pCurDesc->data_index = SELENIC_WORLD;
			else
				pCurDesc->data_index = METAL_WORLD;
		}

		FillOrbits (solarSys, NUM_PLANETS, solarSys->PlanetDesc, TRUE);
	}
	else
	{
		DWORD rand_val = RandomContext_Random (SysGenRNG);
		BYTE planet = 0;

		pSunDesc->NumPlanets = GenerateMinPlanets (2);
		if (pSunDesc->NumPlanets > 9)
			pSunDesc->NumPlanets = 9;

		FillOrbits (solarSys, NUMPLANETS_PDESC, NULL, FALSE);

		if (StarSeed)
			pSunDesc->PlanetByte = PickClosestHabitable (solarSys);

		// The only benefit to pre-stamping is that it shuffles them.
		// But this causes problems when the star is orange, due to metal
		// type. Easier to seed them then stamp them in a shuffled loop so
		// that all types work. This relies on MAX_PLANETS <= 9 for
		// randomness to work, otherwise you'll have to count the bitshifts
		// and re-random every 4th time.
		for (i = 0; i < pSunDesc->NumPlanets; ++i)
			solarSys->PlanetDesc[i].data_index = METAL_WORLD;
		for (i = 0; i < pSunDesc->NumPlanets / 2; ++i)
		{
			BYTE offset = LOBYTE (rand_val) % (pSunDesc->NumPlanets - i);

			while (solarSys->PlanetDesc[planet].data_index ==
					SELENIC_WORLD || offset > 0)
			{
				if (solarSys->PlanetDesc[planet].data_index == METAL_WORLD)
					offset--;
				planet = (planet + 1) % pSunDesc->NumPlanets;
			}

			solarSys->PlanetDesc[planet].data_index = SELENIC_WORLD;
			rand_val = rand_val >> 8;
		}
	}

	if (NOMAD && CheckAlliance (SHOFIXTI_SHIP) == GOOD_GUY)
		solarSys->PlanetDesc[pSunDesc->PlanetByte].NumPlanets = 1;

	return true;
}

static bool
GenerateShofixti_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (NOMAD && matchWorld (solarSys, planet, MATCH_PBYTE, MATCH_PLANET))
	{
		BYTE MoonByte = solarSys->SunDesc[0].MoonByte;
		PLANET_DESC *pMoonDesc = &solarSys->MoonDesc[MoonByte];

		pMoonDesc->data_index = HIERARCHY_STARBASE;
	}

	return true;
}

static bool
GenerateShofixti_generateName (const SOLARSYS_STATE *solarSys,
	const PLANET_DESC *world)
{
	GenerateDefault_generateName (solarSys, world);

	if (matchWorld (solarSys, world, MATCH_PBYTE, MATCH_PLANET))
	{
		BYTE PlanetByte = solarSys->SunDesc[0].PlanetByte;
		PLANET_DESC pPlanetDesc = solarSys->PlanetDesc[PlanetByte];

		utf8StringCopy (GLOBAL_SIS (PlanetName),
				sizeof (GLOBAL_SIS (PlanetName)),
				GAME_STRING (PLANET_NUMBER_BASE + 35));

		SET_GAME_STATE (BATTLE_PLANET, pPlanetDesc.data_index);
	}

	return true;
}

static bool
GenerateShofixti_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world)
{
	if (NOMAD && matchWorld (solarSys, world, MATCH_PBYTE, MATCH_MBYTE))
	{
		if (CheckAlliance (SHOFIXTI_SHIP) == GOOD_GUY)
		{
			BOOLEAN MaxShips = (CountEscortShips (SHOFIXTI_SHIP) <
					IF_HARD (2, 1) ? TRUE : FALSE);
			BOOLEAN RoomInFleet = EscortFeasibilityStudy (SHOFIXTI_SHIP);
			BYTE Index = !MaxShips ? 0 : (!RoomInFleet ? 1 : 2);

			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				SetRelStringTableIndex (
					CaptureStringTable (
						LoadStringTable (SHOFIXTI_BASE_STRTAB)), Index);

			DoDiscoveryReport (MenuSounds);

			if (Index == 2)
			{
				AddEscortShips (SHOFIXTI_SHIP, (
						CountEscortShips (SHOFIXTI_SHIP) == 1 ? 1 : 2));
			}

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

static void
check_old_shofixti (void)
{
	HIPGROUP hGroup;
	IP_GROUP *GroupPtr;

	if (!GLOBAL (BattleGroupRef))
		return; // nothing to check

	hGroup = GetHeadLink (&GLOBAL (ip_group_q));
	if (!hGroup)
		return; // still nothing to check

	GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == SHOFIXTI_SHIP
			&& (GroupPtr->task & REFORM_GROUP)
			&& GET_GAME_STATE (SHOFIXTI_RECRUITED))
	{
		GroupPtr->task = FLEE | IGNORE_FLAGSHIP | REFORM_GROUP;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
}