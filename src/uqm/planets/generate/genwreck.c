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
#include "libs/mathlib.h"

static bool GenerateWreck_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateWreck_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateWreck_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateWreck_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);


const GenerateFunctions generateWreckFunctions = {
	/* .initNpcs         = */ GenerateDefault_initNpcs,
	/* .reinitNpcs       = */ GenerateDefault_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateWreck_generatePlanets,
	/* .generateMoons    = */ GenerateDefault_generateMoons,
	/* .generateName     = */ GenerateDefault_generateName,
	/* .generateOrbital  = */ GenerateWreck_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateWreck_generateEnergy,
	/* .generateLife     = */ GenerateDefault_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateWreck_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};

static bool
GenerateWreck_generatePlanets (SOLARSYS_STATE *solarSys)
{	
	solarSys->SunDesc[0].NumPlanets = (BYTE)~0;

	if (!PrimeSeed)
		solarSys->SunDesc[0].NumPlanets = (RandomContext_Random (SysGenRNG) % (MAX_GEN_PLANETS - 7) + 7);

	FillOrbits (solarSys, solarSys->SunDesc[0].NumPlanets, solarSys->PlanetDesc, FALSE);
	GeneratePlanets (solarSys);

	if(!PrimeSeed){
		solarSys->PlanetDesc[6].data_index = (RandomContext_Random (SysGenRNG) % LAST_SMALL_ROCKY_WORLD);
		solarSys->PlanetDesc[6].alternate_colormap = NULL;
	}

	return true;
}

static bool
GenerateWreck_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	if (matchWorld (solarSys, world, 6, MATCH_PLANET))
	{
		LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
		solarSys->PlanetSideFrame[1] =
				CaptureDrawable (LoadGraphic (WRECK_MASK_PMAP_ANIM));
		solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable (LoadStringTable (WRECK_STRTAB));
		if (GET_GAME_STATE (PORTAL_KEY))
		{	// Already picked it up, skip the first report
			solarSys->SysInfo.PlanetInfo.DiscoveryString =
					SetAbsStringTableIndex (
					solarSys->SysInfo.PlanetInfo.DiscoveryString, 1);
		}
	}

	GenerateDefault_generateOrbital (solarSys, world);
	return true;
}

static COUNT
GenerateWreck_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, 6, MATCH_PLANET))
	{
		return GenerateDefault_generateArtifact (solarSys, whichNode, info);
	}

	return 0;
}

static bool
GenerateWreck_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, 6, MATCH_PLANET))
	{
		assert (whichNode == 0);

		GenerateDefault_landerReportCycle (solarSys);

		if (!GET_GAME_STATE (PORTAL_KEY))
		{
			SetLanderTakeoff ();

			SET_GAME_STATE (PORTAL_KEY, 1);
			SET_GAME_STATE (PORTAL_KEY_ON_SHIP, 1);
		}

		// The Wreck cannot be "picked up". It is always on the surface.
		return false;
	}

	(void) whichNode;
	return false;
}