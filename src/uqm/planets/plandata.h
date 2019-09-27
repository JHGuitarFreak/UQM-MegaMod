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

#ifndef UQM_PLANETS_PLANDATA_H_
#define UQM_PLANETS_PLANDATA_H_

#include "libs/reslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*------------------------------ Type Defines ----------------------------- */
#define NUMBER_OF_ORBITS 16
#define VACANT 0xFF

enum
{
	SMALL_ROCKY_WORLD = 0,
	LARGE_ROCKY_WORLD,
	GAS_GIANT,

	NUM_PLANET_TYPES
};

enum
{
	DWARF_STAR = 0,
	GIANT_STAR,
	SUPER_GIANT_STAR,

	NUM_STAR_TYPES
};

enum
{
	BLUE_BODY = 0,
	GREEN_BODY,
	ORANGE_BODY,
	RED_BODY,
	WHITE_BODY,
	GRAY_BODY = WHITE_BODY,
	YELLOW_BODY,

	NUM_STAR_COLORS,

	CYAN_BODY = NUM_STAR_COLORS,
	PURPLE_BODY,
	VIOLET_BODY
};

enum
{
	OWNER_NOBODY = 0,
	OWNER_NEUTRAL,
	OWNER_HIERARCHY,

	OWNER_PLAYER = (1 << 2)
};

#define STAR_OWNER_SHIFT 0
#define STAR_TYPE_SHIFT 3 /* STAR_OWNER_SHIFT + 3 */
#define STAR_COLOR_SHIFT 5 /* STAR_TYPE_SHIFT + 2 */
#define STAR_COLOR_MASK (BYTE)(0xFF << STAR_COLOR_SHIFT)
#define STAR_TYPE_MASK (BYTE)((0xFF << STAR_TYPE_SHIFT) \
										& ~STAR_COLOR_MASK)
#define STAR_OWNER_MASK (BYTE)((0xFF << STAR_OWNER_SHIFT) \
										& ~(STAR_COLOR_MASK \
										| STAR_TYPE_MASK))
#define STAR_UNKNOWN_MASK (STAR_OWNER_MASK & ~OWNER_PLAYER)

#define MAKE_STAR(t,c,o) \
		(BYTE)((((BYTE)(t) << STAR_TYPE_SHIFT) & STAR_TYPE_MASK) \
		| (((BYTE)(c) << STAR_COLOR_SHIFT) & STAR_COLOR_MASK) \
		| (((BYTE)(o) << STAR_OWNER_SHIFT) & STAR_OWNER_MASK))
#define STAR_TYPE(f) (BYTE)(((f) & STAR_TYPE_MASK) >> STAR_TYPE_SHIFT)
#define STAR_COLOR(f) (BYTE)(((f) & STAR_COLOR_MASK) >> STAR_COLOR_SHIFT)
#define STAR_OWNER(f) (BYTE)(((f) & STAR_OWNER_MASK) >> STAR_OWNER_SHIFT)
#define STAR_UNKNOWN(f) (BOOLEAN)((STAR_OWNER(f) \
								& STAR_UNKNOWN_MASK) == STAR_UNKNOWN_MASK)

#define PLAN_SIZE_MASK 0x03

#define TOPO_ALGO (0 << 2)
#define CRATERED_ALGO (1 << 2)
#define GAS_GIANT_ALGO (2 << 2)
#define PLAN_ALGO_MASK 0x0C

#define PLANSIZE(type)  ((BYTE)((type) & PLAN_SIZE_MASK))
#define PLANALGO(type)  ((BYTE)((type) & PLAN_ALGO_MASK))
#define PLANCOLOR(type) HINIBBLE (type)

#define THIN_ATMOSPHERE 10
#define NORMAL_ATMOSPHERE 75
#define THICK_ATMOSPHERE 200
#define SUPER_THICK_ATMOSPHERE 2500
#define GAS_GIANT_ATMOSPHERE 0xFFFF

enum
{
	FIRST_ROCKY_WORLD = 0,
	FIRST_SMALL_ROCKY_WORLD = FIRST_ROCKY_WORLD,

	OOLITE_WORLD = FIRST_SMALL_ROCKY_WORLD,
	YTTRIC_WORLD,
	QUASI_DEGENERATE_WORLD,
	LANTHANIDE_WORLD,
	TREASURE_WORLD,
	UREA_WORLD,
	METAL_WORLD,
	RADIOACTIVE_WORLD,
	OPALESCENT_WORLD,
	CYANIC_WORLD,
	ACID_WORLD,
	ALKALI_WORLD,
	HALIDE_WORLD,
	GREEN_WORLD,
	COPPER_WORLD,
	CARBIDE_WORLD,
	ULTRAMARINE_WORLD,
	NOBLE_WORLD,
	AZURE_WORLD,
	CHONDRITE_WORLD,
	PURPLE_WORLD,
	SUPER_DENSE_WORLD,
	PELLUCID_WORLD,
	DUST_WORLD,
	CRIMSON_WORLD,
	CIMMERIAN_WORLD,
	INFRARED_WORLD,
	SELENIC_WORLD,
	AURIC_WORLD,
	LAST_SMALL_ROCKY_WORLD = AURIC_WORLD,

	FIRST_LARGE_ROCKY_WORLD,
	FLUORESCENT_WORLD = FIRST_LARGE_ROCKY_WORLD,
	ULTRAVIOLET_WORLD,
	PLUTONIC_WORLD,
	RAINBOW_WORLD,
	SHATTERED_WORLD,
	SAPPHIRE_WORLD,
	ORGANIC_WORLD,
	XENOLITHIC_WORLD,
	REDUX_WORLD,
	PRIMORDIAL_WORLD,
	EMERALD_WORLD,
	CHLORINE_WORLD,
	MAGNETIC_WORLD,
	WATER_WORLD,
	TELLURIC_WORLD,
	HYDROCARBON_WORLD,
	IODINE_WORLD,
	VINYLOGOUS_WORLD,
	RUBY_WORLD,
	MAGMA_WORLD,
	MAROON_WORLD,
	LAST_LARGE_ROCKY_WORLD = MAROON_WORLD,

	FIRST_GAS_GIANT,
	BLU_GAS_GIANT = FIRST_GAS_GIANT, /* Gas Giants */
	CYA_GAS_GIANT,
	GRN_GAS_GIANT,
	GRY_GAS_GIANT,
	ORA_GAS_GIANT,
	PUR_GAS_GIANT,
	RED_GAS_GIANT,
	VIO_GAS_GIANT,
	YEL_GAS_GIANT,
	LAST_GAS_GIANT = YEL_GAS_GIANT,

	NUMBER_OF_PLANET_TYPES,

	WORLD_TYPE_SPECIAL = 0x80,
	PLANET_SHIELDED    = WORLD_TYPE_SPECIAL,

	HIERARCHY_STARBASE = 127 | WORLD_TYPE_SPECIAL,
	SA_MATRA           = 126 | WORLD_TYPE_SPECIAL,
	DESTROYED_STARBASE = 125 | WORLD_TYPE_SPECIAL,
	PRECURSOR_STARBASE = 124 | WORLD_TYPE_SPECIAL,
};

#define NUMBER_OF_SMALL_ROCKY_WORLDS (LAST_SMALL_ROCKY_WORLD - FIRST_SMALL_ROCKY_WORLD + 1)
#define NUMBER_OF_LARGE_ROCKY_WORLDS (LAST_LARGE_ROCKY_WORLD - FIRST_LARGE_ROCKY_WORLD + 1)
#define NUMBER_OF_ROCKY_WORLDS (NUMBER_OF_SMALL_ROCKY_WORLDS + NUMBER_OF_LARGE_ROCKY_WORLDS)
#define NUMBER_OF_GAS_GIANTS (LAST_GAS_GIANT - FIRST_GAS_GIANT + 1)

// TODO: This struct is highly alignment and padding dependent and
//   should not be used! The data is loaded as binary from files and
//   cast to this struct.
typedef struct
{
	const SIZE level_tab[3];
	const BYTE xlat_tab[256];
} XLAT_DESC;

typedef struct
{
	BYTE ElementType;
			/* Index of this element in element_array */
	BYTE Density;
			/* bits 0-3: quantity of the deposits (maximum number of
			 *           deposits), one of FEW, MODERATE, or NUMEROUS
			 * bits 4-7: quality of the deposit, one of LOW, MEDIUM, or HEAVY
			 */
} ELEMENT_ENTRY;

// PlanetFrame describes a type of planet. It is not used to describe
// individual planets.
typedef struct
{
	BYTE Type;
			/* bits 0-1: size, one of SMALL_ROCKY_WORLD, LARGE_ROCKY_WORLD, or
			 *           GAS_GIANT
			 * bits 2-3: map creation algoritm, one of TOPO_ALGO,
			 *           CRATERED_ALGO, or GAS_GIANT_ALGO
			 * bits 4-7: interplanetary color, one of BLUE_BODY, GREEN_BODY,
			 *           ORANGE_BODY, RED_BODY, WHITE_BODY (same as
			 *           GRAY_BODY), YELLOW_BODY, CYAN_BODY, PURPLE_BODY,
			 *           VIOLET_BODY)
             */
	BYTE BaseTectonics;
			/* Base constant for calculation of tectonic activity,
			 * relative to Earth at 100.
			 * One of: NO_TECTONICS, LOW_TECTONICS, MED_TECTONICS,
			 *         HIGH_TECTONICS, or SUPER_TECTONICS
			 */
	BYTE AtmoAndDensity;
			/* bits 0-3: planet density, one of GAS_DENSITY, LIGHT_DENSITY,
			 *           LOW_DENSITY, NORMAL_DENSITY, HIGH_DENSITY,
			 *           SUPER_DENSITY
			 * bits 4-7: atmosphere, one of LIGHT, MEDIUM, HEAVY, or
			 *           (no define for this) super thick.
			 */
#define NUM_USEFUL_ELEMENTS 8
	ELEMENT_ENTRY UsefulElements[NUM_USEFUL_ELEMENTS];
			/* Minerals on the planet */

	RESOURCE CMapInstance;
			/* Color map */
	RESOURCE XlatTabInstance;
			/* Color translation map */
	
	// Parameters for map-generation algoritms:
	COUNT num_faults;
	SIZE fault_depth;
	COUNT num_blemishes;
	SIZE base_elevation;
} PlanetFrame;

typedef struct
{
	SIZE AxialTilt;
	UWORD Tectonics;
	UWORD Weather;
	UWORD PlanetDensity;
	UWORD PlanetRadius;
	UWORD SurfaceGravity;
	SIZE SurfaceTemperature;
	UWORD RotationPeriod;
	UWORD AtmoDensity;
	SIZE LifeChance;
	DWORD PlanetToSunDist;

	const PlanetFrame *PlanDataPtr;

	DWORD ScanSeed[NUM_SCAN_TYPES];
	DWORD ScanRetrieveMask[NUM_SCAN_TYPES];

	// JMS: This stores the amount of partially scavenged minerals.
	//
	// How it works: When the mineral deposits are generated in scan.c, first the deposit
	// is given its normal kiloton-size (decided by pseudo-random number). Then, the value
	// stored in this list is subtracted from the initial value.
	// There can be max DWORD = 32 mineral deposits on the planet, thus the 32.
	//
	// This can be later made to support also partially scavenged biodata.
	BYTE  PartiallyScavengedList[NUM_SCAN_TYPES][32];

	STRING DiscoveryString;
	FONT LanderFont;
	FRAME LanderFontEff;
} PLANET_INFO;

enum
{
	GAS_DENSITY,
	LIGHT_DENSITY,
	LOW_DENSITY,
	NORMAL_DENSITY,
	HIGH_DENSITY,
	SUPER_DENSITY
};

extern UWORD CalcGravity (const PLANET_INFO*);

#define EARTH_ATMOSPHERE 50

#define COLD_THRESHOLD -40
#define HOT_THRESHOLD 100

#define SOL_COLOR_TABS \
	MERCURY_COLOR_TAB, \
	VENUS_COLOR_TAB, \
	EARTH_COLOR_TAB, \
	MARS_COLOR_TAB, \
	JUPITER_COLOR_TAB, \
	SATURN_COLOR_TAB, \
	URANUS_COLOR_TAB, \
	NEPTUNE_COLOR_TAB, \
	PLUTO_COLOR_TAB

/*------------------------------ Global Data ------------------------------ */

#define NO_TECTONICS 0
#define LOW_TECTONICS 40
#define MED_TECTONICS 80
#define HIGH_TECTONICS 140
#define SUPER_TECTONICS 200

extern const PlanetFrame *PlanData;

#if defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_PLANDATA_H_ */

