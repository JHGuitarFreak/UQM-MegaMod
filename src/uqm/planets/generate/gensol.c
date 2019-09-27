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
#include "../lifeform.h"
#include "../planets.h"
#include "../../build.h"
#include "../../encount.h"
#include "../../globdata.h"
#include "../../gamestr.h"
#include "../../grpinfo.h"
#include "../../nameref.h"
#include "../../sounds.h"
#include "../../state.h"
#include "libs/mathlib.h"
#include "options.h"
#include "../../setup.h"
#include <math.h>

static bool GenerateSol_initNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateSol_reinitNpcs (SOLARSYS_STATE *solarSys);
static bool GenerateSol_generatePlanets (SOLARSYS_STATE *solarSys);
static bool GenerateSol_generateMoons (SOLARSYS_STATE *solarSys,
		PLANET_DESC *planet);
static bool GenerateSol_generateName (const SOLARSYS_STATE *,
		const PLANET_DESC *world);
static bool GenerateSol_generateOrbital (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world);
static COUNT GenerateSol_generateEnergy (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static COUNT GenerateSol_generateLife (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *);
static bool GenerateSol_pickupEnergy (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT whichNode);

static int init_probe (void);
static void check_probe (void);


const GenerateFunctions generateSolFunctions = {
	/* .initNpcs         = */ GenerateSol_initNpcs,
	/* .reinitNpcs       = */ GenerateSol_reinitNpcs,
	/* .uninitNpcs       = */ GenerateDefault_uninitNpcs,
	/* .generatePlanets  = */ GenerateSol_generatePlanets,
	/* .generateMoons    = */ GenerateSol_generateMoons,
	/* .generateName     = */ GenerateSol_generateName,
	/* .generateOrbital  = */ GenerateSol_generateOrbital,
	/* .generateMinerals = */ GenerateDefault_generateMinerals,
	/* .generateEnergy   = */ GenerateSol_generateEnergy,
	/* .generateLife     = */ GenerateSol_generateLife,
	/* .pickupMinerals   = */ GenerateDefault_pickupMinerals,
	/* .pickupEnergy     = */ GenerateSol_pickupEnergy,
	/* .pickupLife       = */ GenerateDefault_pickupLife,
};


static bool
GenerateSol_initNpcs (SOLARSYS_STATE *solarSys)
{
	GLOBAL (BattleGroupRef) = GET_GAME_STATE (URQUAN_PROBE_GRPOFFS);
	if (GLOBAL (BattleGroupRef) == 0)
	{
		if (!optHeadStart)
			CloneShipFragment (URQUAN_DRONE_SHIP, &GLOBAL (npc_built_ship_q), 0);
		GLOBAL (BattleGroupRef) = PutGroupInfo (GROUPS_ADD_NEW, 1);
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		SET_GAME_STATE (URQUAN_PROBE_GRPOFFS, GLOBAL (BattleGroupRef));
	}

	if (!init_probe ())
		GenerateDefault_initNpcs (solarSys);
	else if (optSpaceMusic)
		findRaceSOI();

	return true;
}

static bool
GenerateSol_reinitNpcs (SOLARSYS_STATE *solarSys)
{
	if (GET_GAME_STATE (CHMMR_BOMB_STATE) != 3)
	{
		GenerateDefault_reinitNpcs (solarSys);
		check_probe ();
	}
	else
	{
		GLOBAL (BattleGroupRef) = 0;
		ReinitQueue (&GLOBAL (ip_group_q));
		assert (CountLinks (&GLOBAL (npc_built_ship_q)) == 0);
	}
	return true;
}

static bool
GenerateSol_generatePlanets (SOLARSYS_STATE *solarSys)
{
	COUNT planetI;

#define SOL_SEED 334241042L
	RandomContext_SeedRandom (SysGenRNG, SOL_SEED);

	solarSys->SunDesc[0].NumPlanets = 9;
	for (planetI = 0; planetI < 9; ++planetI)
	{
		//COUNT angle;
		DWORD rand_val;
		UWORD word_val;
		PLANET_DESC *pCurDesc = &solarSys->PlanetDesc[planetI];

		pCurDesc->rand_seed = RandomContext_Random (SysGenRNG);
		rand_val = pCurDesc->rand_seed;
		word_val = LOWORD (rand_val);
		//angle = NORMALIZE_ANGLE ((COUNT)HIBYTE (word_val));
		pCurDesc->angle = NORMALIZE_ANGLE ((COUNT)HIBYTE (word_val));

		switch (planetI)
		{
			case 0: /* MERCURY */
				pCurDesc->data_index = METAL_WORLD;
				if (solTexturesPresent)
					pCurDesc->alternate_colormap = MERCURY_COLOR_TAB;
				pCurDesc->radius = EARTH_RADIUS * 39L / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 1: /* VENUS */
				pCurDesc->data_index = PRIMORDIAL_WORLD;
				if (solTexturesPresent)
					pCurDesc->alternate_colormap = VENUS_COLOR_TAB;
				pCurDesc->radius = EARTH_RADIUS * 72L / 100;
				pCurDesc->NumPlanets = 0;
				pCurDesc->angle = NORMALIZE_ANGLE (FULL_CIRCLE - pCurDesc->angle);
				break;
			case 2: /* EARTH */
				pCurDesc->data_index = WATER_WORLD | PLANET_SHIELDED;
				pCurDesc->alternate_colormap = NULL;
				pCurDesc->radius = EARTH_RADIUS;
				pCurDesc->NumPlanets = 2;
				break;
			case 3: /* MARS */
				pCurDesc->data_index = DUST_WORLD;
				if (solTexturesPresent)
					pCurDesc->alternate_colormap = MARS_COLOR_TAB;
				pCurDesc->radius = EARTH_RADIUS * 152L / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 4: /* JUPITER */
				pCurDesc->data_index = RED_GAS_GIANT;
				pCurDesc->radius = EARTH_RADIUS * 500L /* 520L */ / 100;
				pCurDesc->NumPlanets = 4;
				break;
			case 5: /* SATURN */
				pCurDesc->data_index = ORA_GAS_GIANT;
				pCurDesc->radius = EARTH_RADIUS * 750L /* 952L */ / 100;
				pCurDesc->NumPlanets = 1;
				break;
			case 6: /* URANUS */
				pCurDesc->data_index = GRN_GAS_GIANT;
				pCurDesc->radius = EARTH_RADIUS * 1000L /* 1916L */ / 100;
				pCurDesc->NumPlanets = 0;
				break;
			case 7: /* NEPTUNE */
				pCurDesc->data_index = BLU_GAS_GIANT;
				pCurDesc->radius = EARTH_RADIUS * 1250L /* 2999L */ / 100;
				pCurDesc->NumPlanets = 1;
				break;
			case 8: /* PLUTO */
				pCurDesc->data_index = PELLUCID_WORLD;
				if (solTexturesPresent)
					pCurDesc->alternate_colormap = PLUTO_COLOR_TAB;
				pCurDesc->radius = EARTH_RADIUS * 1550L /* 3937L */ / 100;
				pCurDesc->NumPlanets = 0;
				if(PrimeSeed)
					pCurDesc->angle = FULL_CIRCLE - OCTANT;
				break;
		}

		pCurDesc->orb_speed = FULL_CIRCLE / (365.25 * pow((float)pCurDesc->radius / EARTH_RADIUS, 1.5));
		pCurDesc->location.x = COSINE (pCurDesc->angle, pCurDesc->radius);
		pCurDesc->location.y = SINE (pCurDesc->angle, pCurDesc->radius);
	}

	return true;
}

static bool
GenerateSol_generateMoons (SOLARSYS_STATE *solarSys, PLANET_DESC *planet)
{
	COUNT planetNr;
	DWORD rand_val;

	GenerateDefault_generateMoons (solarSys, planet);

	planetNr = planetIndex (solarSys, planet);
	switch (planetNr)
	{
		case 2: /* moons of EARTH */
		{
			COUNT angle;

			/* Starbase: */
			solarSys->MoonDesc[0].data_index = HIERARCHY_STARBASE;
			solarSys->MoonDesc[0].radius = MIN_MOON_RADIUS;
			angle = HALF_CIRCLE + QUADRANT;
			solarSys->MoonDesc[0].location.x =
					COSINE (angle, solarSys->MoonDesc[0].radius);
			solarSys->MoonDesc[0].location.y =
					SINE (angle, solarSys->MoonDesc[0].radius);
			solarSys->MoonDesc[0].orb_speed = FULL_CIRCLE / 11.46;

			/* Luna: */
			solarSys->MoonDesc[1].data_index = SELENIC_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[1].alternate_colormap = LUNA_COLOR_TAB;
			solarSys->MoonDesc[1].radius = MIN_MOON_RADIUS
					+ (4 - 1) * MOON_DELTA;
			rand_val = RandomContext_Random (SysGenRNG);
			angle = NORMALIZE_ANGLE (LOWORD (rand_val));
			solarSys->MoonDesc[1].location.x =
					COSINE (angle, solarSys->MoonDesc[1].radius);
			solarSys->MoonDesc[1].location.y =
					SINE (angle, solarSys->MoonDesc[1].radius);
			solarSys->MoonDesc[1].orb_speed = FULL_CIRCLE / 29;
			break;
		}
		case 4: /* moons of JUPITER */
			solarSys->MoonDesc[0].data_index = RADIOACTIVE_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[0].alternate_colormap = IO_COLOR_TAB;
			solarSys->MoonDesc[0].orb_speed = FULL_CIRCLE / 1.77;
					/* Io */
			solarSys->MoonDesc[1].data_index = HALIDE_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[1].alternate_colormap = EUROPA_COLOR_TAB;
			solarSys->MoonDesc[1].orb_speed = FULL_CIRCLE / 3.55;
					/* Europa */
			solarSys->MoonDesc[2].data_index = CYANIC_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[2].alternate_colormap = GANYMEDE_COLOR_TAB;
			solarSys->MoonDesc[2].orb_speed = FULL_CIRCLE / 7.16;
					/* Ganymede */
			solarSys->MoonDesc[3].data_index = PELLUCID_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[3].alternate_colormap = CALLISTO_COLOR_TAB;
			solarSys->MoonDesc[3].orb_speed = FULL_CIRCLE / 16.69;
					/* Callisto */
			break;
		case 5: /* moons of SATURN */
			solarSys->MoonDesc[0].data_index = ALKALI_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[0].alternate_colormap = TITAN_COLOR_TAB;
			/*solarSys->MoonDesc[0].radius = MIN_MOON_RADIUS
					+ (MAX_MOONS - 1) * MOON_DELTA;*/
			solarSys->MoonDesc[0].orb_speed = FULL_CIRCLE / 15.95;
					/* Titan */
			break;
		case 7: /* moons of NEPTUNE */
			solarSys->MoonDesc[0].data_index = VINYLOGOUS_WORLD;
			if (solTexturesPresent)
				solarSys->MoonDesc[0].alternate_colormap = TRITON_COLOR_TAB;
			solarSys->MoonDesc[0].orb_speed = FULL_CIRCLE / -5.88;
					/* Triton */
			break;
	}

	return true;
}

static bool
GenerateSol_generateName (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world)
{
	COUNT planetNr = planetIndex (solarSys, world);
	utf8StringCopy (GLOBAL_SIS (PlanetName), sizeof (GLOBAL_SIS (PlanetName)),
			GAME_STRING (PLANET_NUMBER_BASE + planetNr));
	SET_GAME_STATE (BATTLE_PLANET, solarSys->PlanetDesc[planetNr].data_index);

	return true;
}

static bool
GenerateSol_generateOrbital (SOLARSYS_STATE *solarSys, PLANET_DESC *world)
{
	DWORD rand_val;
	COUNT planetNr;

	if (matchWorld (solarSys, world, 2, 0))
	{
		if (!NOMAD) {
			/* Starbase */
			PutGroupInfo(GROUPS_RANDOM, GROUP_SAVE_IP);
			ReinitQueue(&GLOBAL(ip_group_q));
			assert(CountLinks(&GLOBAL(npc_built_ship_q)) == 0);

			EncounterGroup = 0;
			GLOBAL(CurrentActivity) |= START_ENCOUNTER;
			SET_GAME_STATE(GLOBAL_FLAGS_AND_DATA, (BYTE)~0);
			return true;
		} else {
			LoadStdLanderFont(&solarSys->SysInfo.PlanetInfo);

			solarSys->SysInfo.PlanetInfo.DiscoveryString =
				CaptureStringTable(
					LoadStringTable(NOMAD_BASE_STRTAB));

			DoDiscoveryReport(MenuSounds);

			DestroyStringTable(ReleaseStringTable(
				solarSys->SysInfo.PlanetInfo.DiscoveryString));
			solarSys->SysInfo.PlanetInfo.DiscoveryString = 0;
			FreeLanderFont(&solarSys->SysInfo.PlanetInfo);

			return true;
		}
	}

	DoPlanetaryAnalysis (&solarSys->SysInfo, world);
	rand_val = RandomContext_GetSeed (SysGenRNG);

	solarSys->SysInfo.PlanetInfo.ScanSeed[MINERAL_SCAN] = rand_val;
	GenerateMineralDeposits (&solarSys->SysInfo, GENERATE_ALL, NULL);
	rand_val = RandomContext_GetSeed (SysGenRNG);

	planetNr = planetIndex (solarSys, world);
	if (worldIsPlanet (solarSys, world))
	{
		switch (planetNr)
		{
			case 0: /* MERCURY */
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 98;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 38;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 3;
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 2;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 59 * 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 165;
				break;
			case 1: /* VENUS */
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 90 *
						EARTH_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 95;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 95;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 177;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 243 * 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 457;
				break;
			case 2: /* EARTH */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						EARTH_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 100;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 100;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 23;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 240;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = 22;
				break;
			case 3: /* MARS */
				// XXX: Mars atmo should actually be 1/2 in current units
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 1;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 72;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 53;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 24;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 246;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -53;
				break;
			case 4: /* JUPITER */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 24;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 1120;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 3;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 98;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -143;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 520L / 100;
				break;
			case 5: /* SATURN */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 13;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 945;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 27;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 102;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -197;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 952L / 100;
				break;
			case 6: /* URANUS */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 21;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 411;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 98;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 172;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -217;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 1916L / 100;
				break;
			case 7: /* NEPTUNE */
				solarSys->SysInfo.PlanetInfo.AtmoDensity =
						GAS_GIANT_ATMOSPHERE;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 28;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 396;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 30;
				solarSys->SysInfo.PlanetInfo.Weather = 7;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 182;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -229;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 2999L / 100;
				break;
			case 8: /* PLUTO */
				if (!GET_GAME_STATE (FOUND_PLUTO_SPATHI))
				{
					LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
							CaptureDrawable (
							LoadGraphic (SPAPLUTO_MASK_PMAP_ANIM));
					solarSys->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
							LoadStringTable (SPAPLUTO_STRTAB));
				}

				solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 33;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 18;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 119;
				solarSys->SysInfo.PlanetInfo.Weather = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 1533;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -235;
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 3937L / 100;
				break;
		}
		
		if (solTexturesPresent){
			switch (planetNr) {
				case 0: /* MERCURY */
					LoadPlanet (CaptureDrawable (LoadGraphic (MERCURY_MASK_ANIM)));
					break;
				case 1: /* VENUS */
					LoadPlanet (CaptureDrawable (LoadGraphic (VENUS_MASK_ANIM)));
					break;
				case 2: /* EARTH */
					LoadPlanet (CaptureDrawable (LoadGraphic (EARTH_MASK_ANIM)));
					break;
				case 3: /* MARS */
					LoadPlanet (CaptureDrawable (LoadGraphic (MARS_MASK_ANIM)));
					break;
				case 4: /* JUPITER*/
					LoadPlanet (CaptureDrawable (LoadGraphic (JUPITER_MASK_ANIM)));
					break;
				case 5: /* SATURN*/
					LoadPlanet (CaptureDrawable (LoadGraphic (SATURN_MASK_ANIM)));
					break;
				case 6: /* URANUS */
					LoadPlanet (CaptureDrawable (LoadGraphic (URANUS_MASK_ANIM)));
					break;
				case 7: /* NEPTUNE */
					LoadPlanet (CaptureDrawable (LoadGraphic (NEPTUNE_MASK_ANIM)));
					break;
				case 8: /* PLUTO */
					LoadPlanet (CaptureDrawable (LoadGraphic (PLUTO_MASK_ANIM)));
					break;
				default:
					LoadPlanet (NULL);
					break;
			}
		} else {
			LoadPlanet (planetNr == 2 ?
					CaptureDrawable (LoadGraphic (EARTH_MASK_ANIM)) : NULL);			
		}
	}
	else
	{
		// World is a moon.
		COUNT moonNr = moonIndex (solarSys, world);

		solarSys->SysInfo.PlanetInfo.AxialTilt = 0;
		solarSys->SysInfo.PlanetInfo.AtmoDensity = 0;
		solarSys->SysInfo.PlanetInfo.Weather = 0;

		switch (planetNr)
		{
			case 2: /* moons of EARTH */
				// NOTE: Even though we save the seed here, it is irrelevant.
				//   The seed will be used to randomly place the tractors, but
				//   since they are mobile, they will be moved to different
				//   locations not governed by this seed.
				solarSys->SysInfo.PlanetInfo.ScanSeed[BIOLOGICAL_SCAN] =
						rand_val;

				if (!GET_GAME_STATE (MOONBASE_DESTROYED))
				{
					LoadStdLanderFont (&solarSys->SysInfo.PlanetInfo);
					solarSys->PlanetSideFrame[1] =
							CaptureDrawable (
							LoadGraphic (MOONBASE_MASK_PMAP_ANIM));
					solarSys->SysInfo.PlanetInfo.DiscoveryString =
							CaptureStringTable (
							LoadStringTable (MOONBASE_STRTAB));
				}

				solarSys->SysInfo.PlanetInfo.PlanetDensity = 60;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
				solarSys->SysInfo.PlanetInfo.AxialTilt = 0;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 240 * 29;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -18;
				break;

			case 4: /* moons of JUPITER */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 520L / 100;
				switch (moonNr)
				{
					case 0: /* Io */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 69;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
						solarSys->SysInfo.PlanetInfo.Tectonics = 3;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 390;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -163;
						break;
					case 1: /* Europa */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 54;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 25;
						solarSys->SysInfo.PlanetInfo.Tectonics = 1;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 840;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -161;
						break;
					case 2: /* Ganymede */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 35;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 41;
						solarSys->SysInfo.PlanetInfo.Tectonics = 0;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 1728;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -164;
						break;
					case 3: /* Callisto */
						solarSys->SysInfo.PlanetInfo.PlanetDensity = 35;
						solarSys->SysInfo.PlanetInfo.PlanetRadius = 38;
						solarSys->SysInfo.PlanetInfo.Tectonics = 1;
						solarSys->SysInfo.PlanetInfo.RotationPeriod = 4008;
						solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -167;
						break;
				}
				break;

			case 5: /* moon of SATURN: Titan */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 952L / 100;
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 160;
				solarSys->SysInfo.PlanetInfo.Weather = 2;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 34;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 40;
				solarSys->SysInfo.PlanetInfo.Tectonics = 1;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 3816;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -178;
				break;

			case 7: /* moon of NEPTUNE: Triton */
				solarSys->SysInfo.PlanetInfo.PlanetToSunDist =
						EARTH_RADIUS * 2999L / 100;
				solarSys->SysInfo.PlanetInfo.AtmoDensity = 10;
				solarSys->SysInfo.PlanetInfo.Weather = 1;
				solarSys->SysInfo.PlanetInfo.PlanetDensity = 95;
				solarSys->SysInfo.PlanetInfo.PlanetRadius = 27;
				solarSys->SysInfo.PlanetInfo.Tectonics = 0;
				solarSys->SysInfo.PlanetInfo.RotationPeriod = 4300;
				solarSys->SysInfo.PlanetInfo.SurfaceTemperature = -216;
				break;
		}

		solarSys->SysInfo.PlanetInfo.SurfaceGravity =
				CalcGravity (&solarSys->SysInfo.PlanetInfo);
		
		if (solTexturesPresent){
			switch (planetNr) {
				case 2: /* moons of EARTH */
					if (moonNr == 1)
						LoadPlanet (CaptureDrawable (LoadGraphic (LUNA_MASK_ANIM)));
					else
						LoadPlanet (NULL);
					break;
				case 4: /* moons of JUPITER */
					switch (moonNr) {
						case 0: /* Io */
							LoadPlanet (CaptureDrawable (LoadGraphic (IO_MASK_ANIM)));
							break;
						case 1: /* Europa */
							LoadPlanet (CaptureDrawable (LoadGraphic (EUROPA_MASK_ANIM)));
							break;
						case 2: /* Ganymede */
							LoadPlanet (CaptureDrawable (LoadGraphic (GANYMEDE_MASK_ANIM)));
							break;
						case 3: /* Callisto */
							LoadPlanet (CaptureDrawable (LoadGraphic (CALLISTO_MASK_ANIM)));
							break;
					}
					break;
				case 5: /* moon of Saturn: Titan */
					LoadPlanet (CaptureDrawable (LoadGraphic (TITAN_MASK_ANIM)));
					break;
				case 7: /* moon of NEPTUNE: Triton */
				default:
					LoadPlanet (CaptureDrawable (LoadGraphic (TRITON_MASK_ANIM)));
					break;
			}
		} else			
			LoadPlanet (NULL);
	}

	return true;
}

static COUNT
GenerateSol_generateEnergy (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, 8, MATCH_PLANET))
	{
		/* Pluto */
		// This check is needed because the retrieval bit is not set for
		// this node to keep it on the surface while the lander is taking off
		if (GET_GAME_STATE (FOUND_PLUTO_SPATHI))
		{	// already picked up
			return 0;
		}

		if (info)
		{
			info->loc_pt.x = RES_SCALE(20);
			info->loc_pt.y = MAP_HEIGHT - RES_SCALE(8);
		}

		return 1; // only matters when count is requested
	}
	
	if (matchWorld (solarSys, world, 2, 1))
	{
		/* Earth Moon */
		// This check is redundant since the retrieval bit will keep the
		// node from showing up again
		if (GET_GAME_STATE (MOONBASE_DESTROYED))
		{	// already picked up
			return 0;
		}

		if (info)
		{
			info->loc_pt.x = MAP_WIDTH * 3 / 4;
			info->loc_pt.y = MAP_HEIGHT * 1 / 4;
		}

		return 1; // only matters when count is requested
	}

	(void) whichNode;
	return 0;
}

static bool
GenerateSol_pickupEnergy (SOLARSYS_STATE *solarSys, PLANET_DESC *world,
		COUNT whichNode)
{
	if (matchWorld (solarSys, world, 8, MATCH_PLANET))
	{	// Pluto
		assert (!GET_GAME_STATE (FOUND_PLUTO_SPATHI) && whichNode == 0);
	
		// Ran into Fwiffo on Pluto
		#define FWIFFO_FRAGS  8
		if (!KillLanderCrewSeq (FWIFFO_FRAGS, ONE_SECOND / 20))
			return false; // lander probably died

		SET_GAME_STATE (FOUND_PLUTO_SPATHI, 1);

		GenerateDefault_landerReport (solarSys);
		SetLanderTakeoff ();

		// Do not remove the node from the surface while the lander is
		// taking off. FOUND_PLUTO_SPATHI bit will keep the node from
		// showing up on subsequent visits.
		return false;
	}
	
	if (matchWorld (solarSys, world, 2, 1))
	{	// Earth Moon
		// assert (!GET_GAME_STATE (MOONBASE_DESTROYED) && whichNode == 0);

		GenerateDefault_landerReport (solarSys);

		if (!NOMAD) {
			SetLanderTakeoff();

			SET_GAME_STATE(MOONBASE_DESTROYED, 1);
			SET_GAME_STATE(MOONBASE_ON_SHIP, 1);

			return true; // picked up
		}
		else
			return false;
	}

	(void) whichNode;
	return false;
}

static COUNT
GenerateSol_generateLife (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world, COUNT whichNode, NODE_INFO *info)
{
	if (matchWorld (solarSys, world, 2, 1))
	{
		/* Earth Moon */
		return GenerateRandomNodes (&solarSys->SysInfo, BIOLOGICAL_SCAN, 10,
				BRAINBOX_BULLDOZER, whichNode, info);
	}

	return 0;
}


static int
init_probe (void)
{
	HIPGROUP hGroup;

	if (!GET_GAME_STATE (PROBE_MESSAGE_DELIVERED)
			&& GetGroupInfo (GLOBAL (BattleGroupRef), GROUP_INIT_IP)
			&& (hGroup = GetHeadLink (&GLOBAL (ip_group_q)))) {
		IP_GROUP *GroupPtr;

		GroupPtr = LockIpGroup (&GLOBAL (ip_group_q), hGroup);
		GroupPtr->task = IN_ORBIT;
		GroupPtr->sys_loc = 2 + 1; /* orbitting earth */
		GroupPtr->dest_loc = 2 + 1; /* orbitting earth */
		GroupPtr->loc.x = 0;
		GroupPtr->loc.y = 0;
		GroupPtr->group_counter = 0;
		UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);

		return 1;
	} else {
		return 0;
	}
}

static void
check_probe (void)
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
	if (GroupPtr->race_id == URQUAN_DRONE_SHIP
			&& (GroupPtr->task & REFORM_GROUP))
	{
		// We just want the probe to take off as fast as possible,
		// so clear out REFORM_GROUP
		GroupPtr->task = FLEE | IGNORE_FLAGSHIP;
		GroupPtr->dest_loc = 0;
	}
	UnlockIpGroup (&GLOBAL (ip_group_q), hGroup);
}

