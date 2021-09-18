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
#include "../../build.h"
#include "../../comm.h"
#include "../../gendef.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../starmap.h"
#include "../../globdata.h"
#include "../../state.h"
#include "libs/log.h"


static bool GenerateMelnorme_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateMelnorme_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateMelnorme_generateMoons (SOLARSYS_STATE *solarSys,
	PLANET_DESC *planet);
static bool GenerateMelnorme_generateOrbital(SOLARSYS_STATE *solarSys,
	PLANET_DESC *world);
static COUNT GenerateMelnorme_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateMelnorme_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);

static DWORD GetMelnormeRef (void);
static void SetMelnormeRef (DWORD Ref);


const GenerateFunctions generateMelnormeFunctions = {
	/* .initNpcs         = */ GenerateMelnorme_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateMelnorme_generatePlanets,
	/* .generateMoons    = */ GenerateMelnorme_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateMelnorme_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateMelnorme_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateMelnorme_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateMelnorme_initNpcs (SOLARSYS_STATE *solarSys)
{
	if ((EXTENDED && !GET_GAME_STATE (KOHR_AH_FRENZY)) 
		|| !EXTENDED)
	{
		GLOBAL (BattleGroupRef) = GetMelnormeRef ();
		if (GLOBAL (BattleGroupRef) == 0)
		{
			CloneShipFragment (MELNORME_SHIP, &GLOBAL (npc_built_ship_q), 0);
			GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
			SetMelnormeRef (GLOBAL (BattleGroupRef));
		}
	}

	GenerateDefault_initNpcs (solarSys);

	return true;
}

static bool
GenerateMelnorme_generatePlanets (SOLARSYS_STATE *solarSys)
{
	int jewelArray[] = { SAPPHIRE_WORLD, EMERALD_WORLD, RUBY_WORLD };

	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;

	if (EXTENDED && !PrimeSeed && CurStarDescPtr->Index == MELNORME1_DEFINED)
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 2) + 2);
	if (EXTENDED && !PrimeSeed && CurStarDescPtr->Index == MELNORME7_DEFINED)
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 4) + 4);

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if (EXTENDED && CurStarDescPtr->Index == MELNORME1_DEFINED)
	{
		solarSys->SunDesc[0].PlanetByte = 2;
		solarSys->SunDesc[0].MoonByte = 0;
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;
	}

	if (EXTENDED && !PrimeSeed && CurStarDescPtr->Index == MELNORME1_DEFINED)
	{
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = jewelArray[RandomContext_Random(SysGenRNG) % 2];
		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = (RandomContext_Random(SysGenRNG) % (MAX_GEN_MOONS - 1) + 1);
		solarSys->SunDesc[0].MoonByte = (RandomContext_Random (SysGenRNG) % solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets);
	}

	if (EXTENDED && CurStarDescPtr->Index == MELNORME7_DEFINED)
	{
		solarSys->SunDesc[0].PlanetByte = 3;

		if (!PrimeSeed)
		{
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = 
					RandomContext_Random (SysGenRNG) % LAST_LARGE_ROCKY_WORLD;

			if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == RAINBOW_WORLD)
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = RAINBOW_WORLD - 1;
			else if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == SHATTERED_WORLD)
				solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = SHATTERED_WORLD + 1;
		}
	}

	return true;
}

static bool
GenerateMelnorme_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	GenerateDefault_generateMoons (solarSys, planet);

	if (EXTENDED && CurStarDescPtr->Index == MELNORME1_DEFINED
		&& matchWorld (solarSys, planet, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		solarSys->MoonDesc[solarSys->SunDesc[0].MoonByte].data_index = PRECURSOR_STARBASE;
	}

	return true;
}

static bool
GenerateMelnorme_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (EXTENDED && CurStarDescPtr->Index == MELNORME1_DEFINED &&
			matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		/* Starbase */
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);

		solarSys->SysInfo.PlanetInfo.DiscoveryString =
			CaptureStringTable (
				LoadStringTable (PRECURSOR_BASE_STRTAB));

		DoDiscoveryReport (MenuSounds);

		DestroyStringTable(ReleaseStringTable (
			solarSys->SysInfo.PlanetInfo.DiscoveryString));
		solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
		FreeLanderFont (&solarSys->SysInfo.PlanetInfo);
	}

	if (EXTENDED && CurStarDescPtr->Index == MELNORME7_DEFINED &&
			matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
			CaptureDrawable (LoadGraphic (STELE_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
			CaptureStringTable (LoadStringTable (STELE_STRTAB));
	}

	GenerateDefault_generateOrbital (solarSys, world);

	return true;
}

static COUNT
GenerateMelnorme_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (EXTENDED && CurStarDescPtr->Index == MELNORME7_DEFINED
			&& matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateMelnorme_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (EXTENDED && CurStarDescPtr->Index == MELNORME7_DEFINED &&
			matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		assert (whichNode == 0);

		GenerateDefault_landerReport (solarSys);

		// The cheese can not be "picked up". It is always on the surface.
		return false;
	}

	(void) whichNode;
	return false;
}

static DWORD
GetMelnormeRef (void)
{
	switch (CurStarDescPtr->Index)
	{
		case MELNORME0_DEFINED: return GET_GAME_STATE (MELNORME0_GRPOFFS);
		case MELNORME1_DEFINED: return GET_GAME_STATE (MELNORME1_GRPOFFS);
		case MELNORME2_DEFINED: return GET_GAME_STATE (MELNORME2_GRPOFFS);
		case MELNORME3_DEFINED: return GET_GAME_STATE (MELNORME3_GRPOFFS);
		case MELNORME4_DEFINED: return GET_GAME_STATE (MELNORME4_GRPOFFS);
		case MELNORME5_DEFINED: return GET_GAME_STATE (MELNORME5_GRPOFFS);
		case MELNORME6_DEFINED: return GET_GAME_STATE (MELNORME6_GRPOFFS);
		case MELNORME7_DEFINED: return GET_GAME_STATE (MELNORME7_GRPOFFS);
		case MELNORME8_DEFINED: return GET_GAME_STATE (MELNORME8_GRPOFFS);
		default:
			log_add (log_Warning, "GetMelnormeRef(): reference unknown");
			return 0;
	}
}

static void
SetMelnormeRef (DWORD Ref)
{
	switch (CurStarDescPtr->Index)
	{
		case MELNORME0_DEFINED: SET_GAME_STATE (MELNORME0_GRPOFFS, Ref); break;
		case MELNORME1_DEFINED: SET_GAME_STATE (MELNORME1_GRPOFFS, Ref); break;
		case MELNORME2_DEFINED: SET_GAME_STATE (MELNORME2_GRPOFFS, Ref); break;
		case MELNORME3_DEFINED: SET_GAME_STATE (MELNORME3_GRPOFFS, Ref); break;
		case MELNORME4_DEFINED: SET_GAME_STATE (MELNORME4_GRPOFFS, Ref); break;
		case MELNORME5_DEFINED: SET_GAME_STATE (MELNORME5_GRPOFFS, Ref); break;
		case MELNORME6_DEFINED: SET_GAME_STATE (MELNORME6_GRPOFFS, Ref); break;
		case MELNORME7_DEFINED: SET_GAME_STATE (MELNORME7_GRPOFFS, Ref); break;
		case MELNORME8_DEFINED: SET_GAME_STATE (MELNORME8_GRPOFFS, Ref); break;
		default:
			log_add (log_Warning, "SetMelnormeRef(): reference unknown");
			return;
	}
}

