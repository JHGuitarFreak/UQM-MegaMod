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
#include "../../state.h"


static bool GenerateZoqFotPikScout_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateZoqFotPikScout_reinitNpcs (SOLARSYS_STATE *solarSys);


const GenerateFunctions generateZoqFotPikScoutFunctions = {
	/* .initNpcs         = */ GenerateZoqFotPikScout_initNpcs,
	/* .reinitNpcs       = */ GenerateZoqFotPikScout_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateDefault_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateDefault_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateDefault_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateDefault_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateZoqFotPikScout_initNpcs (SOLARSYS_STATE *solarSys)
{
	if (!GET_GAME_STATE (MET_ZOQFOT))
	{
		GLOBAL (BattleGroupRef) = GET_GAME_STATE (ZOQFOT_GRPOFFS);
		if (GLOBAL (BattleGroupRef) == 0)
		{
			CloneShipFragment (ZOQFOTPIK_SHIP,
					&GLOBAL (npc_built_ship_q), 0);
			GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
			ReinitQueue (&GLOBAL (npc_built_ship_q));
			SET_GAME_STATE (ZOQFOT_GRPOFFS, GLOBAL (BattleGroupRef));
		}
	}

	GenerateDefault_initNpcs (solarSys);

	return true;
}

static bool
GenerateZoqFotPikScout_reinitNpcs (SOLARSYS_STATE *solarSys)
{
	HIPGROUP hGroup;
	IP_GROUP *GroupPtr;

	GenerateDefault_reinitNpcs (solarSys);

	if (!GLOBAL (BattleGroupRef))
		return true; // nothing to check

	hGroup = GetHeadLink (&GLOBAL (ip_group_q));
	if (!hGroup)
		return true; // still nothing to check

	GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
	// REFORM_GROUP was set in ipdisp.c:ip_group_collision()
	// during a collision with the flagship.
	if (GroupPtr->race_id == ZOQFOTPIK_SHIP
			&& (GroupPtr->task & REFORM_GROUP))
	{
		GroupPtr->task = FLEE | IGNORE_FLAGSHIP | REFORM_GROUP;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);

	return true;
}

