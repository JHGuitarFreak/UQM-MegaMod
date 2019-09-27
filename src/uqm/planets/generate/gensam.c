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
#include "../../encount.h"
#include "../../gamestr.h"
#include "../../gendef.h"
#include "../../globdata.h"
#include "../../grpinfo.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../state.h"
#include "libs/mathlib.h"


static bool GenerateSaMatra_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateSaMatra_reinitNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateSaMatra_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSaMatra_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateSaMatra_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);

static void BuildUrquanGuard (SOLARSYS_STATE *solarSys);


const GenerateFunctions generateSaMatraFunctions = {
	/* .initNpcs         = */ GenerateSaMatra_initNpcs,
	/* .reinitNpcs       = */ GenerateSaMatra_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSaMatra_generatePlanets,
	/* .generateMoons    = */ GenerateSaMatra_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateSaMatra_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateSaMatra_initNpcs (SOLARSYS_STATE *solarSys)
{
	if (CurStarDescPtr->Index == SAMATRA_DEFINED) {
		if (!GET_GAME_STATE(URQUAN_MESSED_UP))
		{
			BuildUrquanGuard(solarSys);
		}
		else
		{	// Exorcise Ur-Quan ghosts upon system reentry
			PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
			// wipe out the group
		}
	}

	if (optSpaceMusic)
		findRaceSOI();

	(void) solarSys;
	return true;
}

static bool
GenerateSaMatra_reinitNpcs (SOLARSYS_STATE *solarSys)
{
	BOOLEAN GuardEngaged;
	HIPGROUP hGroup;
	HIPGROUP hNextGroup;

	if (CurStarDescPtr->Index == SAMATRA_DEFINED) {
		GetGroupInfo(GROUPS_RANDOM, GROUP_LOAD_IP);
		EncounterGroup = 0;
		EncounterRace = -1;
		// Do not want guards to chase the player

		GuardEngaged = FALSE;
		for (hGroup = GetHeadLink(&GLOBAL(ip_group_q));
			hGroup; hGroup = hNextGroup)
		{
			IP_GROUP *GroupPtr;

			GroupPtr = LockIpGroup(&GLOBAL(ip_group_q), hGroup);
			hNextGroup = _GetSuccLink(GroupPtr);

			if (GET_GAME_STATE(URQUAN_MESSED_UP))
			{
				GroupPtr->task &= REFORM_GROUP;
				GroupPtr->task |= FLEE | IGNORE_FLAGSHIP;
				GroupPtr->dest_loc = 0;
			}
			else if (GroupPtr->task & REFORM_GROUP)
			{
				// REFORM_GROUP was set in ipdisp.c:ip_group_collision
				// during a collision with the flagship.
				GroupPtr->task &= ~REFORM_GROUP;
				GroupPtr->group_counter = 0;

				GuardEngaged = TRUE;
			}

			UnlockIpGroup(&GLOBAL(ip_group_q), hGroup);
		}

		if (GuardEngaged)
		{
			COUNT angle;
			POINT org;

			org = planetOuterLocation(solarSys->SunDesc[0].PlanetByte);
			angle = ARCTAN(GLOBAL(ip_location.x) - org.x,
				GLOBAL(ip_location.y) - org.y);
			GLOBAL(ip_location.x) = org.x + COSINE(angle, 3000);
			GLOBAL(ip_location.y) = org.y + SINE(angle, 3000);
			XFormIPLoc(&GLOBAL(ip_location),
				&GLOBAL(ShipStamp.origin), TRUE);
		}
	}

	(void) solarSys;
	return true;
}

static bool
GenerateSaMatra_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT p;

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;


	if(!PrimeSeed){
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);

		if (EXTENDED && CurStarDescPtr->Index >= URQUAN_DEFINED)
			solarSys->SunDesc[0].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_PLANETS - 9) + 9);
	}

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if (CurStarDescPtr->Index == SAMATRA_DEFINED) {
		solarSys->SunDesc[0].PlanetByte = 4;
		solarSys->SunDesc[0].MoonByte = 0;

		if (!PrimeSeed)
			solarSys->SunDesc[0].PlanetByte = (RandomContext_Random(SysGenRNG) % solarSys->SunDesc[0].NumPlanets);

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;

		if (!PrimeSeed) {
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = (RandomContext_Random(SysGenRNG) % MAROON_WORLD);

			if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == RAINBOW_WORLD)
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = RAINBOW_WORLD - 1;
			else if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == SHATTERED_WORLD)
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = SHATTERED_WORLD + 1;

			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
			solarSys->SunDesc[0].MoonByte = (RandomContext_Random(SysGenRNG) % solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets);
		}
	}
	
	if (EXTENDED && CurStarDescPtr->Index == DESTROYED_STARBASE_DEFINED) {
		solarSys->SunDesc[0].PlanetByte = 0;
		solarSys->SunDesc[0].MoonByte = 0;

		if (!PrimeSeed)
			solarSys->SunDesc[0].PlanetByte = (RandomContext_Random(SysGenRNG) % solarSys->SunDesc[0].NumPlanets);

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = PLANET_SHIELDED;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].alternate_colormap = NULL;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;

		if (!PrimeSeed) {
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = PLANET_SHIELDED;
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
			solarSys->SunDesc[0].MoonByte = (RandomContext_Random(SysGenRNG) % solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets);
		}
	}

	if (CurStarDescPtr->Index == URQUAN_DEFINED || CurStarDescPtr->Index == KOHRAH_DEFINED) {
		for (p = 0; p < solarSys->SunDesc[0].NumPlanets; p++) {
			if (solarSys->PlanetDesc[p].NumPlanets <= 1)
				break;
		}
		solarSys->SunDesc[0].PlanetByte = p;
		solarSys->SunDesc[0].MoonByte = 0;

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].alternate_colormap = NULL;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;

		if (!PrimeSeed) {
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
			solarSys->SunDesc[0].MoonByte = (RandomContext_Random(SysGenRNG) % solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets);
		}
	}

	return true;
}

static bool
GenerateSaMatra_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (matchWorld (solarSys, planet, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		COUNT angle;
		DWORD rand_val;

		if (CurStarDescPtr->Index == SAMATRA_DEFINED) {
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = SA_MATRA;

			if (PrimeSeed) {
				solarSys->MoonDesc[0].radius = MIN_MOON_RADIUS + (2 * MOON_DELTA);
				rand_val = RandomContext_Random(SysGenRNG);
				angle = NORMALIZE_ANGLE(LOWORD(rand_val));
				solarSys->MoonDesc[0].location.x =
					COSINE(angle, solarSys->MoonDesc[0].radius);
				solarSys->MoonDesc[0].location.y =
					SINE(angle, solarSys->MoonDesc[0].radius);
				ComputeSpeed(&solarSys->MoonDesc[0], TRUE, 1);
			}
		}


		if (EXTENDED && (CurStarDescPtr->Index == DESTROYED_STARBASE_DEFINED 
				|| CurStarDescPtr->Index == URQUAN_DEFINED 
				|| CurStarDescPtr->Index == KOHRAH_DEFINED)) 
		{
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = DESTROYED_STARBASE;
			solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].alternate_colormap = NULL;
		}
		
	}

	return true;
}

static bool
GenerateSaMatra_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	/* Samatra */
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		if (CurStarDescPtr->Index == SAMATRA_DEFINED) {
			PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue(&GLOBAL(ip_group_q));
			assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

			if (!GET_GAME_STATE(URQUAN_MESSED_UP))
			{
				CloneShipFragment(!GET_GAME_STATE(KOHR_AH_FRENZY) ?
					URQUAN_SHIP : BLACK_URQUAN_SHIP,
					&GLOBAL(npc_built_ship_q), INFINITE_FLEET);
			}
			else
			{
#define URQUAN_REMNANTS 3
				BYTE i;

				for (i = 0; i < URQUAN_REMNANTS; ++i)
				{
					CloneShipFragment(URQUAN_SHIP,
						&GLOBAL(npc_built_ship_q), 0);
					CloneShipFragment(BLACK_URQUAN_SHIP,
						&GLOBAL(npc_built_ship_q), 0);
				}
			}

			GLOBAL(CurrentActivity) |= START_INTERPLANETARY;
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, 1 << 7);
			SET_GAME_STATE(URQUAN_PROTECTING_SAMATRA, 1);
			InitCommunication(URQUAN_CONVERSATION);

			if (!(GLOBAL(CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			{
				BOOLEAN UrquanSurvivors;

				UrquanSurvivors = GetHeadLink(&GLOBAL(npc_built_ship_q)) != 0;

				GLOBAL(CurrentActivity) &= ~START_INTERPLANETARY;
				ReinitQueue(&GLOBAL(npc_built_ship_q));
				GetGroupInfo(GROUPS_RANDOM, GROUP_LOAD_IP);
				if (UrquanSurvivors)
				{
					SET_GAME_STATE(URQUAN_PROTECTING_SAMATRA, 0);
				}
				else
				{
					EncounterGroup = 0;
					EncounterRace = -1;
					GLOBAL(CurrentActivity) = IN_LAST_BATTLE | START_ENCOUNTER;
					if (GET_GAME_STATE(YEHAT_CIVIL_WAR)
						&& StartSphereTracking(YEHAT_SHIP)
						&& EscortFeasibilityStudy(YEHAT_REBEL_SHIP))
						InitCommunication(YEHAT_REBEL_CONVERSATION);
				}
			}

			return true;
		}

		if (EXTENDED && CurStarDescPtr->Index == DESTROYED_STARBASE_DEFINED) {
			/* Starbase */
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable(
					LoadStringTable(DESTROYED_BASE_STRTAB));

			// use alternate text if the player
			// hasn't freed the Earth starbase yet
			if (!GET_GAME_STATE(STARBASE_AVAILABLE))
				solarSys->SysInfo.PlanetInfo.DiscoveryString =
				SetRelStringTableIndex(
					solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);


			DoDiscoveryReport(MenuSounds);

			DestroyStringTable(ReleaseStringTable(
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
			solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);
			return true;
		}

		if (CurStarDescPtr->Index == URQUAN_DEFINED
			|| CurStarDescPtr->Index == KOHRAH_DEFINED) 
		{
			BYTE Index = CurStarDescPtr->Index == URQUAN_DEFINED ? 0 : 1;

			/* Starbase */
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				SetRelStringTableIndex(
					CaptureStringTable(
						LoadStringTable(URQUAN_BASE_STRTAB)), Index);


			DoDiscoveryReport(MenuSounds);

			DestroyStringTable(ReleaseStringTable(
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
			solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);
			return true;
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static void
BuildUrquanGuard (SOLARSYS_STATE *solarSys)
{
	BYTE ship1, ship2;
	BYTE b0, b1;
	POINT org;
	HIPGROUP hGroup, hNextGroup;

	GLOBAL (BattleGroupRef) = GET_GAME_STATE (SAMATRA_GRPOFFS);

	if (!GET_GAME_STATE (KOHR_AH_FRENZY))
	{
		ship1 = URQUAN_SHIP;
		ship2 = BLACK_URQUAN_SHIP;
	}
	else
	{
		ship1 = BLACK_URQUAN_SHIP;
		ship2 = URQUAN_SHIP;
	}

	assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

	for (b0 = 0; b0 < MAX_SHIPS_PER_SIDE; ++b0)
		CloneShipFragment (ship1, &GLOBAL (npc_built_ship_q), 0);

	if (GLOBAL (BattleGroupRef) == 0)
	{
		GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
		SET_GAME_STATE (SAMATRA_GRPOFFS, GLOBAL (BattleGroupRef));
	}

#define NUM_URQUAN_GUARDS0 12
	for (b0 = 1; b0 <= NUM_URQUAN_GUARDS0; ++b0)
		PutGroupInfo (GLOBAL (BattleGroupRef), b0);

	ReinitQueue (&GLOBAL (npc_built_ship_q));
	for (b0 = 0; b0 < MAX_SHIPS_PER_SIDE; ++b0)
		CloneShipFragment (ship2, &GLOBAL (npc_built_ship_q), 0);

#define NUM_URQUAN_GUARDS1 4
	for (b0 = 1; b0 <= NUM_URQUAN_GUARDS1; ++b0)
		PutGroupInfo (GLOBAL (BattleGroupRef),
				(BYTE)(NUM_URQUAN_GUARDS0 + b0));

	ReinitQueue (&GLOBAL (npc_built_ship_q));

	GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP);

	org = planetOuterLocation (solarSys->SunDesc[0].PlanetByte);
	hGroup = GetHeadLink (&GLOBAL (ip_group_q));
	for (b0 = 0, b1 = 0;
			b0 < NUM_URQUAN_GUARDS0;
			++b0, b1 += FULL_CIRCLE / (NUM_URQUAN_GUARDS0 + NUM_URQUAN_GUARDS1))
	{
		IP_GROUP *GroupPtr;

		if (b1 % (FULL_CIRCLE / NUM_URQUAN_GUARDS1) == 0)
			b1 += FULL_CIRCLE / (NUM_URQUAN_GUARDS0 + NUM_URQUAN_GUARDS1);

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);
		GroupPtr->task = ON_STATION | IGNORE_FLAGSHIP;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = solarSys->SunDesc[0].PlanetByte + 1;
		GroupPtr->orbit_pos = NORMALIZE_FACING (ANGLE_TO_FACING (b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + COSINE (b1, STATION_RADIUS);
		GroupPtr->loc.y = org.y + SINE (b1, STATION_RADIUS);
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hGroup = hNextGroup;
	}

	for (b0 = 0, b1 = 0;
			b0 < NUM_URQUAN_GUARDS1;
			++b0, b1 += FULL_CIRCLE / NUM_URQUAN_GUARDS1)
	{
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hNextGroup = _GetSuccLink (GroupPtr);
		GroupPtr->task = ON_STATION | IGNORE_FLAGSHIP;
		GroupPtr->sys_loc = 0;
		GroupPtr->dest_loc = solarSys->SunDesc[0].PlanetByte + 1;
		GroupPtr->orbit_pos = NORMALIZE_FACING (ANGLE_TO_FACING (b1));
		GroupPtr->group_counter = 0;
		GroupPtr->loc.x = org.x + COSINE (b1, STATION_RADIUS);
		GroupPtr->loc.y = org.y + SINE (b1, STATION_RADIUS);
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
		hGroup = hNextGroup;
	}

	(void) solarSys;
}

