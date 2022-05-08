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
#include "../../state.h"
#include "../../build.h"
#include "libs/mathlib.h"
#include "../../comm.h"


static bool GenerateVault_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateVault_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateVault_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateVault_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateVaultFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateVault_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateVault_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateVault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateVault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static bool
GenerateVault_generatePlanets (SOLARSYS_STATE *solarSys)
{
	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;
	solarSys->SunDesc[0].PlanetByte = 0;
	solarSys->SunDesc[0].MoonByte = 0;

	if (!PrimeSeed)
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 1) + 1);

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if(!PrimeSeed)
	{
		if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == RAINBOW_WORLD)
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = RAINBOW_WORLD - 1;
		else if (solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index == SHATTERED_WORLD)
			solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].data_index = SHATTERED_WORLD + 1;

		solarSys->PlanetDesc[solarSys->SunDesc[0].PlanetByte].NumPlanets = 1;
	}

	return true;
}

static bool
GenerateVault_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (VAULT_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (VAULT_STRTAB));
		if (GET_GAME_STATE (SHIP_VAULT_UNLOCKED))
		{
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					SetAbsStringTableIndex (
					solarSys->SysInfo.PlanetInfo.DiscoveryString, 2);
		}
		else if (GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP))
		{
			if (DIF_HARD && !(GET_GAME_STATE (HM_ENCOUNTERS)
						& 1 << URQUAN_ENCOUNTER)
					&& !(GET_GAME_STATE(KOHR_AH_FRENZY)))
			{
				COUNT i;

				PutGroupInfo (GROUPS_RANDOM, GROUP_SAVE_IP);
				ReinitQueue (&GLOBAL (ip_group_q));
				assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);

				for (i = 0; i < 6; ++i)
					CloneShipFragment (URQUAN_SHIP,
						&GLOBAL (npc_built_ship_q), 0);

				SET_GAME_STATE (GLOBAL_FLAGS_AND_DATA, 1 << 6);
				GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				InitCommunication (URQUAN_CONVERSATION);

				if (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
					return true;

				{
					BOOLEAN Survivors =
							GetHeadLink (&GLOBAL(npc_built_ship_q)) != 0;

					GLOBAL (CurrentActivity) &= ~START_INTERPLANETARY;
					ReinitQueue (&GLOBAL (npc_built_ship_q));
					GetGroupInfo (GROUPS_RANDOM, GROUP_LOAD_IP);

					if (Survivors)
						return true;

					{
						UWORD state;

						state = GET_GAME_STATE (HM_ENCOUNTERS);

						state |= 1 << URQUAN_ENCOUNTER;

						SET_GAME_STATE (HM_ENCOUNTERS, state);
					}

					RepairSISBorder ();
				}
			}
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					SetAbsStringTableIndex (
					solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static COUNT
GenerateVault_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateVault_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, solarSys->SunDesc[0].PlanetByte, solarSys->SunDesc[0].MoonByte))
	{
		assert (whichNode == 0);

		if (GET_GAME_STATE (SHIP_VAULT_UNLOCKED))
		{	// Give the final report, "omg empty" and whatnot
			GenerateDefault_landerReportCycle (solarSys);
		}
		else if (GET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP))
		{
			GenerateDefault_landerReportCycle (solarSys);
			SetLanderTakeoff ();

			SET_GAME_STATE (SHIP_VAULT_UNLOCKED, 1);
			SET_GAME_STATE (SYREEN_SHUTTLE_ON_SHIP, 0);
			SET_GAME_STATE (SYREEN_HOME_VISITS, 0);
		}
		else
		{
			GenerateDefault_landerReport (solarSys);

			if (!GET_GAME_STATE (KNOW_SYREEN_VAULT))
			{
				SET_GAME_STATE (KNOW_SYREEN_VAULT, 1);
			}
		}

		// The Vault cannot be "picked up". It is always on the surface.
		return false;
	}

	(void) whichNode;
	return false;
}
