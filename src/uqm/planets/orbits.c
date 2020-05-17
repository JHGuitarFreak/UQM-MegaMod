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

#include "planets.h"
#include "../starmap.h"
#include "libs/compiler.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "../clock.h"
#include <math.h>

//#define DEBUG_ORBITS

enum
{
	PLANET_NEVER = 0,
	PLANET_RARE = 15,
	PLANET_FEW = 63,
	PLANET_COMMON = 127,
	PLANET_ALWAYS = 255
};

static BYTE
BlueDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_ALWAYS, /* OOLITE_WORLD */
		PLANET_ALWAYS, /* YTTRIC_WORLD */
		PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
		PLANET_ALWAYS, /* LANTHANIDE_WORLD */
		PLANET_ALWAYS, /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_ALWAYS, /* METAL_WORLD */
		PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
		PLANET_ALWAYS, /* OPALESCENT_WORLD */
		PLANET_ALWAYS, /* CYANIC_WORLD */
		PLANET_ALWAYS, /* ACID_WORLD */
		PLANET_ALWAYS, /* ALKALI_WORLD */
		PLANET_ALWAYS, /* HALIDE_WORLD */
		PLANET_ALWAYS, /* GREEN_WORLD */
		PLANET_ALWAYS, /* COPPER_WORLD */
		PLANET_ALWAYS, /* CARBIDE_WORLD */
		PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
		PLANET_ALWAYS, /* NOBLE_WORLD */
		PLANET_ALWAYS, /* AZURE_WORLD */
		PLANET_NEVER,  /* CHONDRITE_WORLD */
		PLANET_NEVER,  /* PURPLE_WORLD */
		PLANET_NEVER,  /* SUPER_DENSE_WORLD */
		PLANET_NEVER,  /* PELLUCID_WORLD */
		PLANET_NEVER,  /* DUST_WORLD */
		PLANET_NEVER,  /* CRIMSON_WORLD */
		PLANET_NEVER,  /* CIMMERIAN_WORLD */
		PLANET_NEVER,  /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_ALWAYS, /* AURIC_WORLD */

		PLANET_ALWAYS, /* FLUORESCENT_WORLD */
		PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
		PLANET_ALWAYS, /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_ALWAYS, /* SAPPHIRE_WORLD */
		PLANET_ALWAYS, /* ORGANIC_WORLD */
		PLANET_ALWAYS, /* XENOLITHIC_WORLD */
		PLANET_ALWAYS, /* REDUX_WORLD */
		PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
		PLANET_NEVER,  /* EMERALD_WORLD */
		PLANET_ALWAYS, /* CHLORINE_WORLD */
		PLANET_ALWAYS, /* MAGNETIC_WORLD */
		PLANET_ALWAYS, /* WATER_WORLD */
		PLANET_ALWAYS, /* TELLURIC_WORLD */
		PLANET_NEVER,  /* HYDROCARBON_WORLD */
		PLANET_NEVER,  /* IODINE_WORLD */
		PLANET_NEVER,  /* VINYLOGOUS_WORLD */
		PLANET_NEVER,  /* RUBY_WORLD */
		PLANET_NEVER,  /* MAGMA_WORLD */
		PLANET_NEVER,  /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

static BYTE
GreenDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_NEVER,  /* OOLITE_WORLD */
		PLANET_NEVER,  /* YTTRIC_WORLD */
		PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
		PLANET_ALWAYS, /* LANTHANIDE_WORLD */
		PLANET_ALWAYS, /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_ALWAYS, /* METAL_WORLD */
		PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
		PLANET_ALWAYS, /* OPALESCENT_WORLD */
		PLANET_ALWAYS, /* CYANIC_WORLD */
		PLANET_ALWAYS, /* ACID_WORLD */
		PLANET_ALWAYS, /* ALKALI_WORLD */
		PLANET_ALWAYS, /* HALIDE_WORLD */
		PLANET_ALWAYS, /* GREEN_WORLD */
		PLANET_ALWAYS, /* COPPER_WORLD */
		PLANET_ALWAYS, /* CARBIDE_WORLD */
		PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
		PLANET_ALWAYS, /* NOBLE_WORLD */
		PLANET_ALWAYS, /* AZURE_WORLD */
		PLANET_ALWAYS, /* CHONDRITE_WORLD */
		PLANET_ALWAYS, /* PURPLE_WORLD */
		PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
		PLANET_ALWAYS, /* PELLUCID_WORLD */
		PLANET_NEVER,  /* DUST_WORLD */
		PLANET_NEVER,  /* CRIMSON_WORLD */
		PLANET_NEVER,  /* CIMMERIAN_WORLD */
		PLANET_NEVER,  /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_ALWAYS, /* AURIC_WORLD */

		PLANET_ALWAYS, /* FLUORESCENT_WORLD */
		PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
		PLANET_ALWAYS, /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_NEVER,  /* SAPPHIRE_WORLD */
		PLANET_ALWAYS, /* ORGANIC_WORLD */
		PLANET_ALWAYS, /* XENOLITHIC_WORLD */
		PLANET_ALWAYS, /* REDUX_WORLD */
		PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
		PLANET_ALWAYS, /* EMERALD_WORLD */
		PLANET_ALWAYS, /* CHLORINE_WORLD */
		PLANET_ALWAYS, /* MAGNETIC_WORLD */
		PLANET_ALWAYS, /* WATER_WORLD */
		PLANET_ALWAYS, /* TELLURIC_WORLD */
		PLANET_ALWAYS, /* HYDROCARBON_WORLD */
		PLANET_ALWAYS, /* IODINE_WORLD */
		PLANET_NEVER,  /* VINYLOGOUS_WORLD */
		PLANET_NEVER,  /* RUBY_WORLD */
		PLANET_NEVER,  /* MAGMA_WORLD */
		PLANET_NEVER,  /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

static BYTE
OrangeDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_NEVER,  /* OOLITE_WORLD */
		PLANET_NEVER,  /* YTTRIC_WORLD */
		PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
		PLANET_NEVER,  /* LANTHANIDE_WORLD */
		PLANET_NEVER,  /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_NEVER,  /* METAL_WORLD */
		PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
		PLANET_NEVER,  /* OPALESCENT_WORLD */
		PLANET_NEVER,  /* CYANIC_WORLD */
		PLANET_ALWAYS, /* ACID_WORLD */
		PLANET_ALWAYS, /* ALKALI_WORLD */
		PLANET_ALWAYS, /* HALIDE_WORLD */
		PLANET_ALWAYS, /* GREEN_WORLD */
		PLANET_ALWAYS, /* COPPER_WORLD */
		PLANET_ALWAYS, /* CARBIDE_WORLD */
		PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
		PLANET_ALWAYS, /* NOBLE_WORLD */
		PLANET_ALWAYS, /* AZURE_WORLD */
		PLANET_ALWAYS, /* CHONDRITE_WORLD */
		PLANET_ALWAYS, /* PURPLE_WORLD */
		PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
		PLANET_ALWAYS, /* PELLUCID_WORLD */
		PLANET_ALWAYS, /* DUST_WORLD */
		PLANET_ALWAYS, /* CRIMSON_WORLD */
		PLANET_ALWAYS, /* CIMMERIAN_WORLD */
		PLANET_ALWAYS, /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_NEVER,  /* AURIC_WORLD */

		PLANET_NEVER,  /* FLUORESCENT_WORLD */
		PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
		PLANET_NEVER,  /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_NEVER,  /* SAPPHIRE_WORLD */
		PLANET_NEVER,  /* ORGANIC_WORLD */
		PLANET_NEVER,  /* XENOLITHIC_WORLD */
		PLANET_NEVER,  /* REDUX_WORLD */
		PLANET_NEVER,  /* PRIMORDIAL_WORLD */
		PLANET_NEVER,  /* EMERALD_WORLD */
		PLANET_ALWAYS, /* CHLORINE_WORLD */
		PLANET_ALWAYS, /* MAGNETIC_WORLD */
		PLANET_ALWAYS, /* WATER_WORLD */
		PLANET_ALWAYS, /* TELLURIC_WORLD */
		PLANET_ALWAYS, /* HYDROCARBON_WORLD */
		PLANET_ALWAYS, /* IODINE_WORLD */
		PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
		PLANET_NEVER,  /* RUBY_WORLD */
		PLANET_ALWAYS, /* MAGMA_WORLD */
		PLANET_ALWAYS, /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

static BYTE
RedDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_NEVER,  /* OOLITE_WORLD */
		PLANET_NEVER,  /* YTTRIC_WORLD */
		PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
		PLANET_NEVER,  /* LANTHANIDE_WORLD */
		PLANET_NEVER,  /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_ALWAYS, /* METAL_WORLD */
		PLANET_NEVER,  /* RADIOACTIVE_WORLD */
		PLANET_NEVER,  /* OPALESCENT_WORLD */
		PLANET_NEVER,  /* CYANIC_WORLD */
		PLANET_NEVER,  /* ACID_WORLD */
		PLANET_NEVER,  /* ALKALI_WORLD */
		PLANET_NEVER,  /* HALIDE_WORLD */
		PLANET_NEVER,  /* GREEN_WORLD */
		PLANET_NEVER,  /* COPPER_WORLD */
		PLANET_NEVER,  /* CARBIDE_WORLD */
		PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
		PLANET_ALWAYS, /* NOBLE_WORLD */
		PLANET_ALWAYS, /* AZURE_WORLD */
		PLANET_ALWAYS, /* CHONDRITE_WORLD */
		PLANET_ALWAYS, /* PURPLE_WORLD */
		PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
		PLANET_ALWAYS, /* PELLUCID_WORLD */
		PLANET_ALWAYS, /* DUST_WORLD */
		PLANET_ALWAYS, /* CRIMSON_WORLD */
		PLANET_ALWAYS, /* CIMMERIAN_WORLD */
		PLANET_ALWAYS, /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_NEVER,  /* AURIC_WORLD */

		PLANET_NEVER,  /* FLUORESCENT_WORLD */
		PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
		PLANET_NEVER,  /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_NEVER,  /* SAPPHIRE_WORLD */
		PLANET_NEVER,  /* ORGANIC_WORLD */
		PLANET_NEVER,  /* XENOLITHIC_WORLD */
		PLANET_NEVER,  /* REDUX_WORLD */
		PLANET_NEVER,  /* PRIMORDIAL_WORLD */
		PLANET_NEVER,  /* EMERALD_WORLD */
		PLANET_NEVER,  /* CHLORINE_WORLD */
		PLANET_ALWAYS, /* MAGNETIC_WORLD */
		PLANET_ALWAYS, /* WATER_WORLD */
		PLANET_ALWAYS, /* TELLURIC_WORLD */
		PLANET_ALWAYS, /* HYDROCARBON_WORLD */
		PLANET_ALWAYS, /* IODINE_WORLD */
		PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
		PLANET_ALWAYS, /* RUBY_WORLD */
		PLANET_ALWAYS, /* MAGMA_WORLD */
		PLANET_ALWAYS, /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

static BYTE
WhiteDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_ALWAYS, /* OOLITE_WORLD */
		PLANET_ALWAYS, /* YTTRIC_WORLD */
		PLANET_ALWAYS, /* QUASI_DEGENERATE_WORLD */
		PLANET_ALWAYS, /* LANTHANIDE_WORLD */
		PLANET_ALWAYS, /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_ALWAYS, /* METAL_WORLD */
		PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
		PLANET_ALWAYS, /* OPALESCENT_WORLD */
		PLANET_ALWAYS, /* CYANIC_WORLD */
		PLANET_ALWAYS, /* ACID_WORLD */
		PLANET_ALWAYS, /* ALKALI_WORLD */
		PLANET_ALWAYS, /* HALIDE_WORLD */
		PLANET_NEVER,  /* GREEN_WORLD */
		PLANET_NEVER,  /* COPPER_WORLD */
		PLANET_NEVER,  /* CARBIDE_WORLD */
		PLANET_NEVER,  /* ULTRAMARINE_WORLD */
		PLANET_NEVER,  /* NOBLE_WORLD */
		PLANET_NEVER,  /* AZURE_WORLD */
		PLANET_NEVER,  /* CHONDRITE_WORLD */
		PLANET_NEVER,  /* PURPLE_WORLD */
		PLANET_NEVER,  /* SUPER_DENSE_WORLD */
		PLANET_NEVER,  /* PELLUCID_WORLD */
		PLANET_NEVER,  /* DUST_WORLD */
		PLANET_NEVER,  /* CRIMSON_WORLD */
		PLANET_NEVER,  /* CIMMERIAN_WORLD */
		PLANET_NEVER,  /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_ALWAYS, /* AURIC_WORLD */

		PLANET_ALWAYS, /* FLUORESCENT_WORLD */
		PLANET_ALWAYS, /* ULTRAVIOLET_WORLD */
		PLANET_ALWAYS, /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_ALWAYS, /* SAPPHIRE_WORLD */
		PLANET_ALWAYS, /* ORGANIC_WORLD */
		PLANET_ALWAYS, /* XENOLITHIC_WORLD */
		PLANET_ALWAYS, /* REDUX_WORLD */
		PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
		PLANET_ALWAYS, /* EMERALD_WORLD */
		PLANET_NEVER,  /* CHLORINE_WORLD */
		PLANET_NEVER,  /* MAGNETIC_WORLD */
		PLANET_NEVER,  /* WATER_WORLD */
		PLANET_NEVER,  /* TELLURIC_WORLD */
		PLANET_NEVER,  /* HYDROCARBON_WORLD */
		PLANET_NEVER,  /* IODINE_WORLD */
		PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
		PLANET_ALWAYS, /* RUBY_WORLD */
		PLANET_NEVER,  /* MAGMA_WORLD */
		PLANET_NEVER,  /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

static BYTE
YellowDistribution (BYTE which_world)
{
	const BYTE PlanetDistribution[NUMBER_OF_PLANET_TYPES] =
	{
		PLANET_NEVER,  /* OOLITE_WORLD */
		PLANET_NEVER,  /* YTTRIC_WORLD */
		PLANET_NEVER,  /* QUASI_DEGENERATE_WORLD */
		PLANET_NEVER,  /* LANTHANIDE_WORLD */
		PLANET_ALWAYS, /* TREASURE_WORLD */
		PLANET_ALWAYS, /* UREA_WORLD */
		PLANET_ALWAYS, /* METAL_WORLD */
		PLANET_ALWAYS, /* RADIOACTIVE_WORLD */
		PLANET_ALWAYS, /* OPALESCENT_WORLD */
		PLANET_ALWAYS, /* CYANIC_WORLD */
		PLANET_ALWAYS, /* ACID_WORLD */
		PLANET_ALWAYS, /* ALKALI_WORLD */
		PLANET_ALWAYS, /* HALIDE_WORLD */
		PLANET_ALWAYS, /* GREEN_WORLD */
		PLANET_ALWAYS, /* COPPER_WORLD */
		PLANET_ALWAYS, /* CARBIDE_WORLD */
		PLANET_ALWAYS, /* ULTRAMARINE_WORLD */
		PLANET_ALWAYS, /* NOBLE_WORLD */
		PLANET_ALWAYS, /* AZURE_WORLD */
		PLANET_ALWAYS, /* CHONDRITE_WORLD */
		PLANET_ALWAYS, /* PURPLE_WORLD */
		PLANET_ALWAYS, /* SUPER_DENSE_WORLD */
		PLANET_ALWAYS, /* PELLUCID_WORLD */
		PLANET_ALWAYS, /* DUST_WORLD */
		PLANET_ALWAYS, /* CRIMSON_WORLD */
		PLANET_ALWAYS, /* CIMMERIAN_WORLD */
		PLANET_ALWAYS, /* INFRARED_WORLD */
		PLANET_ALWAYS, /* SELENIC_WORLD */
		PLANET_ALWAYS, /* AURIC_WORLD */

		PLANET_NEVER,  /* FLUORESCENT_WORLD */
		PLANET_NEVER,  /* ULTRAVIOLET_WORLD */
		PLANET_NEVER,  /* PLUTONIC_WORLD */
		PLANET_NEVER,  /* RAINBOW_WORLD */
		PLANET_NEVER,  /* SHATTERED_WORLD */
		PLANET_NEVER,  /* SAPPHIRE_WORLD */
		PLANET_ALWAYS, /* ORGANIC_WORLD */
		PLANET_ALWAYS, /* XENOLITHIC_WORLD */
		PLANET_ALWAYS, /* REDUX_WORLD */
		PLANET_ALWAYS, /* PRIMORDIAL_WORLD */
		PLANET_NEVER,  /* EMERALD_WORLD */
		PLANET_ALWAYS, /* CHLORINE_WORLD */
		PLANET_ALWAYS, /* MAGNETIC_WORLD */
		PLANET_ALWAYS, /* WATER_WORLD */
		PLANET_ALWAYS, /* TELLURIC_WORLD */
		PLANET_ALWAYS, /* HYDROCARBON_WORLD */
		PLANET_ALWAYS, /* IODINE_WORLD */
		PLANET_ALWAYS, /* VINYLOGOUS_WORLD */
		PLANET_NEVER,  /* RUBY_WORLD */
		PLANET_ALWAYS, /* MAGMA_WORLD */
		PLANET_ALWAYS, /* MAROON_WORLD */

		PLANET_ALWAYS, /* BLU_GAS_GIANT */
		PLANET_ALWAYS, /* CYA_GAS_GIANT */
		PLANET_ALWAYS, /* GRN_GAS_GIANT */
		PLANET_ALWAYS, /* GRY_GAS_GIANT */
		PLANET_ALWAYS, /* ORA_GAS_GIANT */
		PLANET_ALWAYS, /* PUR_GAS_GIANT */
		PLANET_ALWAYS, /* RED_GAS_GIANT */
		PLANET_ALWAYS, /* VIO_GAS_GIANT */
		PLANET_ALWAYS, /* YEL_GAS_GIANT */
	};

	return (PlanetDistribution[which_world]);
}

#define DWARF_ROCK_DIST MIN_PLANET_RADIUS
#define DWARF_GASG_DIST SCALE_RADIUS (12)

#define GIANT_ROCK_DIST SCALE_RADIUS (8)
#define GIANT_GASG_DIST SCALE_RADIUS (13)

#define SUPERGIANT_ROCK_DIST SCALE_RADIUS (16)
#define SUPERGIANT_GASG_DIST SCALE_RADIUS (33)

void ComputeSpeed(PLANET_DESC *planet, BOOLEAN GeneratingMoons, UWORD rand_val)
{
	//BW : empiric values, which would give roughly correct
	// rotation periods for most moons in the solar system
	if (GeneratingMoons) {
		planet->orb_speed = FULL_CIRCLE / (29 * pow((double)planet->radius / (MIN_MOON_RADIUS + (MAX_GEN_MOONS - 1) * MOON_DELTA), 1.5));
		if ((planet->pPrevDesc->data_index & ~PLANET_SHIELDED) >= FIRST_GAS_GIANT)
			planet->orb_speed *= 2;
		if (!(rand_val % 7))
			planet->orb_speed = - planet->orb_speed;
	} else {
		planet->orb_speed = FULL_CIRCLE / (ONE_YEAR * pow((double)planet->radius / EARTH_RADIUS, 1.5));
	}
}

void
FillOrbits (SOLARSYS_STATE *system, BYTE NumPlanets,
		PLANET_DESC *pBaseDesc, BOOLEAN TypesDefined)
{ /* Generate Planets in orbit around star */
	BYTE StarColor, PlanetCount, MaxPlanet;
	BOOLEAN GeneratingMoons;
	COUNT StarSize;
	PLANET_DESC *pPD;
	struct
	{
		COUNT MinRockyDist, MinGasGDist;
	} Suns[] =
	{
		{DWARF_ROCK_DIST, DWARF_GASG_DIST},
		{GIANT_ROCK_DIST, GIANT_GASG_DIST},
		{SUPERGIANT_ROCK_DIST, SUPERGIANT_GASG_DIST},
	};
#ifdef DEBUG_ORBITS
UNICODE buf[256];
char stype[] = {'D', 'G', 'S'};
char scolor[] = {'B', 'G', 'O', 'R', 'W', 'Y'};
#endif /* DEBUG_ORBITS */

	pPD = pBaseDesc;
	StarSize = system->SunDesc[0].data_index;
	StarColor = STAR_COLOR (CurStarDescPtr->Type);

	if (NumPlanets == (BYTE)~0)
	{
		// XXX: This is pretty funny. Instead of calling RNG once, like so:
		//     1 + Random % MAX_GENERATED_PLANETS
		//   we spin in a loop until the result > 0.
		//   Note that this behavior must be kept to preserve the universe.
		do
			NumPlanets = LOWORD (RandomContext_Random (SysGenRNG))
					% (MAX_GEN_PLANETS + 1);
		while (NumPlanets == 0);
		system->SunDesc[0].NumPlanets = NumPlanets;
	}

#ifdef DEBUG_ORBITS
	GetClusterName (CurStarDescPtr, buf);
	log_add (log_Debug, "cluster name = %s  color = %c type = %c", buf,
			scolor[STAR_COLOR (CurStarDescPtr->Type)],
			stype[STAR_TYPE (CurStarDescPtr->Type)]);
#endif /* DEBUG_ORBITS */
	GeneratingMoons = (BOOLEAN) (pBaseDesc == system->MoonDesc);
	if (GeneratingMoons)
		MaxPlanet = (PrimeSeed ? FIRST_LARGE_ROCKY_WORLD : LAST_LARGE_ROCKY_WORLD);
	else
		MaxPlanet = NUMBER_OF_PLANET_TYPES;
	PlanetCount = NumPlanets;
	while (NumPlanets--)
	{
		BYTE chance;
		DWORD rand_val, min_radius;
		SIZE delta_r;
		PLANET_DESC *pLocPD;

		do
		{
			rand_val = RandomContext_Random (SysGenRNG);
			if (TypesDefined)
				rand_val = 0;
			else
				pPD->data_index =
						(BYTE)(HIBYTE (LOWORD (rand_val)) % MaxPlanet);

			// JMS: This exists for special colormaps of Sol system planets.
			pPD->alternate_colormap = NULL;

			chance = PLANET_NEVER;
			switch (StarColor)
			{
				case BLUE_BODY:
					chance = BlueDistribution (pPD->data_index);
					break;
				case GREEN_BODY:
					chance = GreenDistribution (pPD->data_index);
					break;
				case ORANGE_BODY:
					chance = OrangeDistribution (pPD->data_index);
					break;
				case RED_BODY:
					chance = RedDistribution (pPD->data_index);
					break;
				case WHITE_BODY:
					chance = WhiteDistribution (pPD->data_index);
					break;
				case YELLOW_BODY:
					chance = YellowDistribution (pPD->data_index);
					break;
			}
		} while (LOBYTE (LOWORD (rand_val)) >= chance);

		if (pPD->data_index < FIRST_GAS_GIANT)
			min_radius = Suns[StarSize].MinRockyDist;
		else
			min_radius = Suns[StarSize].MinGasGDist;
RelocatePlanet:
		rand_val = RandomContext_Random (SysGenRNG);
		if (GeneratingMoons)
		{
			pPD->radius = MIN_MOON_RADIUS
				+ ((LOWORD(rand_val) % MAX_GEN_MOONS) * MOON_DELTA);
			for (pLocPD = pPD - 1; pLocPD >= pBaseDesc; --pLocPD)
			{
				if (pPD->radius == pLocPD->radius)
					goto RelocatePlanet;
			}
			pPD->NumPlanets = 0;
		}
		else
		{
			pPD->radius =
					(LOWORD (rand_val) % (MAX_PLANET_RADIUS - min_radius))
					+ min_radius;
			for (pLocPD = pPD - 1; pLocPD >= pBaseDesc; --pLocPD)
			{
				delta_r = UNSCALE_RADIUS (pLocPD->radius) / 5
						- UNSCALE_RADIUS (pPD->radius) / 5;
				if (delta_r < 0)
					delta_r = -delta_r;
				if (delta_r <= 1)
					goto RelocatePlanet;
			}
		}

		rand_val = RandomContext_Random (SysGenRNG);
		// Initial angle & coordinates as in Vanilla UQM
		// Still used to compute rand_seed and the position
		// of the planet at the start of the game
		pPD->angle = NORMALIZE_ANGLE (LOWORD (rand_val));
		pPD->location.x = COSINE (pPD->angle, pPD->radius);
		pPD->location.y = SINE (pPD->angle, pPD->radius);
		if (GeneratingMoons) {
			pPD->rand_seed = MAKE_DWORD (
				 COSINE (pPD->angle, RES_DESCALE(pPD->radius)),
				 SINE (pPD->angle, RES_DESCALE(pPD->radius)));
		} else {
			pPD->rand_seed = MAKE_DWORD (pPD->location.x, pPD->location.y);
		}
		// Angle is kept for reference but location will be adjusted
		// to take orbiting into account
		ComputeSpeed(pPD, GeneratingMoons, HIWORD (rand_val));

		++pPD;
	}

	{
		BYTE i;

		for (i = 0; i < PlanetCount; ++i)
		{
			BYTE j;

			for (j = (BYTE)(PlanetCount - 1); j > i; --j)
			{
				if (pBaseDesc[i].radius > pBaseDesc[j].radius)
				{
					PLANET_DESC temp;

					temp = pBaseDesc[i];
					pBaseDesc[i] = pBaseDesc[j];
					pBaseDesc[j] = temp;
				}
			}
		}
	}
}

