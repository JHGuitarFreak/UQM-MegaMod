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

#include "solarsys.h"
#include "lander.h"
#include "../build.h"
#include "../colors.h"
#include "../controls.h"
#include "../menustat.h"
		// for DrawMenuStateStrings()
#include "../starmap.h"
#include "../races.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../globdata.h"
#include "../sis.h"
#include "../init.h"
#include "../shipcont.h"
#include "../gameopt.h"
#include "../nameref.h"
#include "../resinst.h"
#include "../settings.h"
#include "../ipdisp.h"
#include "../grpinfo.h"
#include "../process.h"
#include "../setup.h"
#include "../sounds.h"
#include "../state.h"
#include "../uqmdebug.h"
#include "../save.h"
#include "options.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "libs/misc.h"
#include "scan.h"
#include "libs/graphics/cmap.h"

#include "../hyper.h"
		// for SOL_X/Y

#include <math.h>
#include <time.h>

#include SDL_INCLUDE(SDL_version.h)

//#define DEBUG_SOLARSYS
//#define SMOOTH_SYSTEM_ZOOM  1

#define IP_FRAME_RATE  (ONE_SECOND / 30)

// BW: those do not depend on the resolution because numbers too small
// cause crashes in the generation and rendering
#define MOON_DIAMETER (7 << 2)
#define LARGE_MOON_DIAMETER (11 << 2)
#define PLANET_DIAMETER (29 << 2)
#define GENERATE_PERIMETER(a) \
		(a * ORIGINAL_MAP_WIDTH / ORIGINAL_MAP_HEIGHT)

static void AnimateSun (SIZE radius);
static BOOLEAN DoIpFlight (SOLARSYS_STATE *pSS);
static void DrawInnerPlanets (PLANET_DESC* planet);
static void DrawOuterPlanets(SIZE radius);
static void DrawSystem (SIZE radius, BOOLEAN IsInnerSystem);
static void DrawInnerSystem (void);
static void DrawOuterSystem (void);
static void SetPlanetColorMap (PLANET_DESC *planet);
static void ValidateInnerOrbits (void);
static void ValidateOrbits (void);
static COORD scaleSISDimensions (BOOLEAN is_width, COORD value);
static int widthHeightPicker (BOOLEAN width);

// SolarSysMenu() items
enum SolarSysMenuMenuItems
{
	// XXX: Must match the enum in menustat.h
	STARMAP = 1,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION,
};

SOLARSYS_STATE *pSolarSysState;
FRAME SISIPFrame;
FRAME SunFrame;
FRAME OrbitalFrame;
FRAME OrbitalShield;
FRAME SpaceJunkFrame;
COLORMAP OrbitalCMap;
COLORMAP SunCMap;
MUSIC_REF SpaceMusic;
static DWORD SpaceMusicPos[21];

SIZE EncounterRace;
BYTE EncounterGroup;
		// last encountered group info

static FRAME StarsFrame;
		// prepared star-field graphic
static FRAME SolarSysFrame;
		// saved solar system view graphic

static RECT scaleRect;
		// system zooms in when the flagship enters this rect

static COUNT PBodySize[7] = { 0, 3, 4, 7, 11, 15, 29 };
		// Planet sizes primary used for textured planets
		// There are actually only 6 sizes, but 0 is reached
		// only for WORLD_TYPE_SPECIAL and '!= 0' check prevents
		// from nuking SpaceJunkFrame in SetPlanetOldFrame()
#define UNDEFINED_OFFSET ((BYTE)~0)

RandomContext *SysGenRNG;
RandomContext* SysGenRNGDebug;

#define DISPLAY_TO_LOC  (DISPLAY_FACTOR >> 1)
#define DISPLAY_TO_LOC_US  (DISPLAY_FACTOR_US >> 1)

POINT
locationToDisplay (POINT pt, SIZE scaleRadius)
{
	POINT out;

	out.x = (RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1)
			+ (long)pt.x * DISPLAY_TO_LOC / scaleRadius);
	out.y = (RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1)
			+ (long)pt.y * DISPLAY_TO_LOC / scaleRadius);

	return out;
}

POINT
displayToLocation (POINT pt, SIZE scaleRadius)
{
	POINT out;

	out.x = (((long)pt.x - RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1))
		* scaleRadius / DISPLAY_TO_LOC);
	out.y = (((long)pt.y - RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1))
		* scaleRadius / DISPLAY_TO_LOC);

	return out;
}

POINT
planetOuterLocation (COUNT planetI)
{
	SIZE scaleRadius = pSolarSysState->SunDesc[0].radius;
	return displayToLocation (
			pSolarSysState->PlanetDesc[planetI].image.origin,
			scaleRadius);
}

bool
worldIsPlanet (const SOLARSYS_STATE *solarSys, const PLANET_DESC *world)
{
	return world->pPrevDesc == solarSys->SunDesc;
}

bool
worldIsMoon (const SOLARSYS_STATE *solarSys, const PLANET_DESC *world)
{
	return world->pPrevDesc != solarSys->SunDesc;
}

// Returns the planet index of the world. If the world is a moon, then
// this is the index of the planet it is orbiting.
COUNT
planetIndex (const SOLARSYS_STATE *solarSys, const PLANET_DESC *world)
{
	const PLANET_DESC *planet = worldIsPlanet (solarSys, world) ?
			world : world->pPrevDesc;
	return planet - solarSys->PlanetDesc;
}

COUNT
moonIndex (const SOLARSYS_STATE *solarSys, const PLANET_DESC *moon)
{
	assert (!worldIsPlanet (solarSys, moon));
	return moon - solarSys->MoonDesc;
}

// Test whether 'world' is the planetI-th planet, and if moonI is not
// set to MATCH_PLANET, also whether 'world' is the moonI-th moon.
bool
matchWorld (const SOLARSYS_STATE *solarSys, const PLANET_DESC *world,
		BYTE planetI, BYTE moonI)
{
	// Check whether we have the right planet.
	if (planetIndex (solarSys, world) != planetI)
		return false;

	if (moonI == MATCH_PLANET)
	{
		// Only test whether we are at the planet.
		if (!worldIsPlanet (solarSys, world))
			return false;
	}
	else
	{
		// Test whether the moon matches too
		if (!worldIsMoon (solarSys, world))
			return false;

		if (moonIndex (solarSys, world) != moonI)
			return false;
	}

	return true;
}

bool
playerInSolarSystem (void)
{
	return pSolarSysState != NULL;
}

bool
playerInPlanetOrbit (void)
{
	return playerInSolarSystem () && pSolarSysState->InOrbit;
}

bool
playerInInnerSystem (void)
{
	assert (playerInSolarSystem ());
	assert (pSolarSysState->pBaseDesc == pSolarSysState->PlanetDesc
			|| pSolarSysState->pBaseDesc == pSolarSysState->MoonDesc);
	return pSolarSysState->pBaseDesc != pSolarSysState->PlanetDesc;
}

static void
GenerateTexturedMoons (SOLARSYS_STATE *system, PLANET_DESC *planet)
{
	COUNT i;
	SIZE MoonDiameter;
	PLANET_DESC *pMoonDesc;
	PLANET_DESC *previousOrbitalDesc = pSolarSysState->pOrbitalDesc;
	PLANET_INFO *planetInfo = &pSolarSysState->SysInfo.PlanetInfo;

	for (i = 0, pMoonDesc = &system->MoonDesc[0];
			i < planet->NumPlanets; ++i, ++pMoonDesc)
	{
		RESOURCE maskAnim = NULL;
	
		// BW : precompute the generated texture to display it in IP
		if (!(pMoonDesc->data_index & WORLD_TYPE_SPECIAL))
		{
			DoPlanetaryAnalysis (&pSolarSysState->SysInfo, pMoonDesc);
			
			if (CurStarDescPtr->Index == SOL_DEFINED)
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 0;

			pSolarSysState->pOrbitalDesc = pMoonDesc;

			if (CurStarDescPtr->Index == SOL_DEFINED)
			{	// png defined moons in Sol
				COUNT curr_planet_index =
						planetIndex (pSolarSysState, planet);

				switch (curr_planet_index)
				{
					case 2:	// EARTH
						if (i == 1)
						{	// LUNA
							maskAnim = IP_LUNA_MASK_ANIM;
							planetInfo->RotationPeriod = 240 * 29;
						}
						break;
					case 4:	// JUPITER
						switch (i)
						{
							case 0: // IO
								maskAnim = IP_IO_MASK_ANIM;
								planetInfo->RotationPeriod = 390;
								break;
							case 1: // EUROPA
								maskAnim = IP_EUROPA_MASK_ANIM;
								planetInfo->RotationPeriod = 840;
								break;
							case 2: // GANYMEDE
								maskAnim = IP_GANYMEDE_MASK_ANIM;
								planetInfo->RotationPeriod = 1728;
								break;
							case 3: // CALLISTO
								maskAnim = IP_CALLISTO_MASK_ANIM;
								planetInfo->RotationPeriod = 4008;
								break;
						}
						break;
					case 5:	// SATURN
						// TITAN
						maskAnim = IP_TITAN_MASK_ANIM;
						planetInfo->RotationPeriod = 3816;
						break;
					case 7:	// NEPTUNE
						// TRITON
						maskAnim = IP_TRITON_MASK_ANIM;
						planetInfo->RotationPeriod = 4300;
						break;
					case 8:	// PLUTO
						// CHARON
						maskAnim = IP_CHARON_MASK_ANIM;
						planetInfo->RotationPeriod = 6.4 * EARTH_HOURS;
						break;
				}
			}

			MoonDiameter = pMoonDesc->data_index > LAST_SMALL_ROCKY_WORLD ?
					LARGE_MOON_DIAMETER : MOON_DIAMETER;
			GeneratePlanetSurface (pMoonDesc,
					CaptureDrawable (LoadGraphic (maskAnim)),
					GENERATE_PERIMETER (MoonDiameter), MoonDiameter
				);
			pMoonDesc->orbit = pSolarSysState->Orbit;
			PrepareNextRotationFrameForIP (pMoonDesc, 0);

			DestroyStringTable (ReleaseStringTable (
					pSolarSysState->XlatRef));
			pSolarSysState->XlatRef = 0;
			DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
			pSolarSysState->TopoFrame = 0;
			DestroyColorMap (ReleaseColorMap (
					pSolarSysState->OrbitalCMap));
			pSolarSysState->OrbitalCMap = 0;
		}
	}
	pSolarSysState->pOrbitalDesc = previousOrbitalDesc;
}

// Sets the SysGenRNG to the required state first.
static void
GenerateMoons (SOLARSYS_STATE *system, PLANET_DESC *planet)
{
	COUNT i;
	COUNT facing;
	PLANET_DESC *pMoonDesc;

	RandomContext_SeedRandom (SysGenRNG, planet->rand_seed);
	
	facing = NORMALIZE_FACING (ANGLE_TO_FACING (
			ARCTAN (planet->location.x, planet->location.y)));
	for (i = 0, pMoonDesc = &system->MoonDesc[0];
			i < MAX_GEN_MOONS; ++i, ++pMoonDesc)
	{
		pMoonDesc->pPrevDesc = planet;
		if (i >= planet->NumPlanets)
			continue;
		
		pMoonDesc->temp_color = planet->temp_color;
		pMoonDesc->frame_offset = UNDEFINED_OFFSET;
	}

	// These are moved down here to satisfy the ComputeSpeed function
	(*system->genFuncs->generateName) (system, planet);
	(*system->genFuncs->generateMoons) (system, planet);
}

static void
LoadPixelatedSun (void)
{
	if (optNebulae)
	{
		COUNT i;
		BYTE *pix, *pIter;
		Color *map, *mIter;
		SIZE width, height;
		RECT r;
		Color AColor;

		FRAME idxFrame =
				CaptureDrawable (LoadGraphic (SUN_MASK_PMAP_ANIM));
		SunFrame = CaptureDrawable (LoadGraphic (SUN_MASK_RGB_PMAP_ANIM));

		for (i = 0; i < 5; i++)
		{		
			SIZE x, y;
			GetFrameRect (idxFrame, &r);
			width = r.extent.width;
			height = r.extent.height;

			pix = HMalloc (sizeof (BYTE) * width * height);
			map = HMalloc (sizeof (Color) * width * height);

			pIter = pix;
			mIter = map;

			ReadFramePixelIndexes (idxFrame, pix, width, height, TRUE);
			ReadFramePixelColors (SunFrame, map, width, height);

			AColor = GetColorMapColor (56, 239);


			for (y = 0; y < height; ++y)
				for (x = 0; x < width; ++x, ++pIter, ++mIter)
				{
					if (mIter->a == 0)// Fully transparent color
						continue;

					if (mIter->a != 0xFF)// partially transparent
					{
						mIter->r = AColor.r;
						mIter->g = AColor.g;
						mIter->b = AColor.b;
					}

					if (mIter->a == 0xFF)// opaque
					{
						*mIter = GetColorMapColor (56, *pIter);
					}
				}

			WriteFramePixelColors (SunFrame, map, width, height);

			HFree (map);
			HFree (pix);

			idxFrame = IncFrameIndex (idxFrame);
			SunFrame = IncFrameIndex (SunFrame);
		}

		DestroyDrawable (ReleaseDrawable (idxFrame));
		idxFrame = 0;
	}
	else
		SunFrame = CaptureDrawable (LoadGraphic (SUN_MASK_PMAP_ANIM));
}

void
FreeIPData (void)
{
	DestroyDrawable (ReleaseDrawable (SISIPFrame));
	SISIPFrame = 0;
	DestroyDrawable (ReleaseDrawable (SunFrame));
	SunFrame = 0;
	DestroyColorMap (ReleaseColorMap (SunCMap));
	SunCMap = 0;
	DestroyColorMap (ReleaseColorMap (OrbitalCMap));
	OrbitalCMap = 0;
	DestroyDrawable (ReleaseDrawable (OrbitalFrame));
	OrbitalFrame = 0;
	DestroyDrawable (ReleaseDrawable (OrbitalShield));
	OrbitalShield = 0;
	DestroyDrawable (ReleaseDrawable (SpaceJunkFrame));
	SpaceJunkFrame = 0;
	DestroyMusic (SpaceMusic);
	SpaceMusic = 0;

	RandomContext_Delete (SysGenRNG);
	SysGenRNG = NULL;
}

void
LoadIPData (void)
{
	if (SpaceJunkFrame == 0)
	{
		SpaceJunkFrame = CaptureDrawable (
				LoadGraphic (IPBKGND_MASK_PMAP_ANIM));

		if (optFlagshipColor == OPT_3DO)
			SISIPFrame =
				CaptureDrawable (LoadGraphic (SISIP_MASK_PMAP_ANIM_RED));
		else
			SISIPFrame =
				CaptureDrawable (LoadGraphic (SISIP_MASK_PMAP_ANIM));

		OrbitalCMap = CaptureColorMap (LoadColorMap (ORBPLAN_COLOR_MAP));
		OrbitalFrame = CaptureDrawable (
				LoadGraphic ((!optTexturedPlanets && isPC(optPlanetStyle))
					? DOS_ORBPLAN_MASK_PMAP_ANIM
						: ORBPLAN_MASK_PMAP_ANIM));
		OrbitalShield = CaptureDrawable (
				LoadGraphic (ORBSHLD_MASK_PMAP_ANIM));
		SunCMap = CaptureColorMap (LoadColorMap (IPSUN_COLOR_MAP));

		SetColorMap (GetColorMapAddress (SetAbsColorMapIndex(
			SunCMap, STAR_COLOR (CurStarDescPtr->Type))));

		if (!IS_HD)
			LoadPixelatedSun ();
		else
		{
			RESOURCE maskAnim = SUNYELLOW_MASK_PMAP_ANIM;

			switch (STAR_COLOR (CurStarDescPtr->Type))
			{
				case BLUE_BODY:
					maskAnim = SUNBLUE_MASK_PMAP_ANIM;
					break;
				case GREEN_BODY:
					maskAnim = SUNGREEN_MASK_PMAP_ANIM;
					break;
				case ORANGE_BODY:
					maskAnim = SUNORANGE_MASK_PMAP_ANIM;
					break;
				case RED_BODY:
					maskAnim = SUNRED_MASK_PMAP_ANIM;
					break;
				case WHITE_BODY:
					maskAnim = SUNWHITE_MASK_PMAP_ANIM;
					break;
			}

			SunFrame = CaptureDrawable (LoadGraphic (maskAnim));
		}

		SpaceMusic = 0;
	}

	if (!SysGenRNG)
	{
		SysGenRNG = RandomContext_New ();
		SysGenRNGDebug = SysGenRNG;
	}
}
	

static void
sortPlanetPositions (void)
{
	COUNT i;
	SIZE sort_array[MAX_PLANETS + 1];

	// When this part is done, sort_array will contain the indices to
	// all planets, sorted on their y position.
	// The sun itself, which has its data located at
	// pSolarSysState->PlanetDesc[-1], is included in this array.
	// Very ugly stuff, but it's correct.

	// Initialise sort_array.
	memset (sort_array, 0, sizeof (sort_array));

	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i)
		sort_array[i] = i - 1;

	// Sort sort_array, based on the positions of the planets/sun.
	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i)
	{
		COUNT j;

		for (j = pSolarSysState->SunDesc[0].NumPlanets; j > i; --j)
		{
			SIZE real_i, real_j, temp;

			real_i = sort_array[i];
			real_j = sort_array[j];
			if ((pSolarSysState->PlanetDesc[real_i].image.origin.y >
					pSolarSysState->PlanetDesc[real_j].image.origin.y) 
					|| (pSolarSysState->PlanetDesc[real_i].image.origin.y 
					== pSolarSysState->PlanetDesc[real_j].image.origin.y 
					&& real_i == -1))
			{
				temp = sort_array[i];
				sort_array[i] = sort_array[j];
				sort_array[j] = temp;
			}
		}
	}

	// Put the results of the sorting in the solar system structure.
	pSolarSysState->FirstPlanetIndex = sort_array[0];
	pSolarSysState->LastPlanetIndex =
			sort_array[pSolarSysState->SunDesc[0].NumPlanets];
	for (i = 0; i <= pSolarSysState->SunDesc[0].NumPlanets; ++i) {
		PLANET_DESC *planet = &pSolarSysState->PlanetDesc[sort_array[i]];
		planet->NextIndex = sort_array[i + 1];
	}
}

static void
initSolarSysSISCharacteristics (void)
{
	BYTE i;
	BYTE num_thrusters;

	num_thrusters = 0;
	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		if (GLOBAL_SIS (DriveSlots[i]) == FUSION_THRUSTER)
			++num_thrusters;
	}
	pSolarSysState->max_ship_speed = (BYTE)(
			(num_thrusters + 5) * IP_SHIP_THRUST_INCREMENT);

	pSolarSysState->turn_wait = IP_SHIP_TURN_WAIT;
	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		if (GLOBAL_SIS (JetSlots[i]) == TURNING_JETS)
			pSolarSysState->turn_wait -= IP_SHIP_TURN_DECREMENT;
	}
}

DWORD
GetRandomSeedForStar (const STAR_DESC *star)
{
	return MAKE_DWORD (star->star_pt.x, star->star_pt.y);
}

DWORD
GetRandomSeedForVar (const POINT point)
{
	return MAKE_DWORD (point.x, point.y);
}

void GenerateTexturedPlanets (void)
{
	COUNT i;
	PLANET_DESC *pCurDesc;
	PLANET_DESC *previousOrbitalDesc;
	previousOrbitalDesc = pSolarSysState->pOrbitalDesc;
	
	for (i = 0, pCurDesc = pSolarSysState->PlanetDesc;
			i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
	{
		RESOURCE maskanim = NULL;

		DoPlanetaryAnalysis (&pSolarSysState->SysInfo, pCurDesc);
		
		// BW : precompute the generated texture to display it in IP
		pSolarSysState->pOrbitalDesc = pCurDesc;
		if (CurStarDescPtr->Index == SOL_DEFINED)
		{
			switch (i)
			{
			case 0: /* MERCURY */
				maskanim = IP_MERCURY_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod =
						59 * 240;
				break;
			case 1: /* VENUS */
				maskanim = IP_VENUS_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 177;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod =
						243 * 240;
				break;
			case 2: // EARTH
				maskanim = EARTH_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 23;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 240;
				break;
			case 3: // MARS
				maskanim = IP_MARS_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 24;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 246;
				break;
			case 4: /* JUPITER */
				maskanim = IP_JUPITER_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 3;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 98;
				break;
			case 5: /* SATURN */
				maskanim = IP_SATURN_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 27;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 102;
				break;
			case 6: /* URANUS */
				maskanim = IP_URANUS_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 98;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 172;
				break;
			case 7: /* NEPTUNE */
				maskanim = IP_NEPTUNE_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 30;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 182;
				break;
			case 8: /* PLUTO */
				maskanim = IP_PLUTO_MASK_ANIM;
				pSolarSysState->SysInfo.PlanetInfo.AxialTilt = 119;
				pSolarSysState->SysInfo.PlanetInfo.RotationPeriod = 1533;
				break;
			}
		}
		
		GeneratePlanetSurface (pCurDesc,
				CaptureDrawable (LoadGraphic (maskanim)),
				GENERATE_PERIMETER (PLANET_DIAMETER), PLANET_DIAMETER
			);
		pCurDesc->orbit = pSolarSysState->Orbit;
		PrepareNextRotationFrameForIP (pCurDesc, 0);
		
		// Clean up some parasitic use of pSolarSysState
		DestroyStringTable (ReleaseStringTable (pSolarSysState->XlatRef));
		pSolarSysState->XlatRef = 0;
		DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
		pSolarSysState->TopoFrame = 0;
		DestroyColorMap (ReleaseColorMap (pSolarSysState->OrbitalCMap));
		pSolarSysState->OrbitalCMap = 0;
		// End clean up
	}
	pSolarSysState->pOrbitalDesc = previousOrbitalDesc;
}

// Returns an orbital PLANET_DESC when player is in orbit
static PLANET_DESC *
LoadSolarSys (void)
{
	COUNT i;
	PLANET_DESC *orbital = NULL;
	PLANET_DESC *pCurDesc;
#define NUM_TEMP_RANGES 5
	Color temp_color_array[NUM_TEMP_RANGES] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0E), 0x54),
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x08), 0x62),
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x0B, 0x00), 0x6D),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x08, 0x00), 0x75),
	};

	if (optUnscaledStarSystem && IS_HD)
		temp_color_array[0] =
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 19), 0x54);

	RandomContext_SeedRandom (SysGenRNG,
			GetRandomSeedForStar (CurStarDescPtr));

	// JMS: Animating IP sun in hi-res...
	if (!IS_HD)
		SunFrame = SetAbsFrameIndex (SunFrame,
				STAR_TYPE (CurStarDescPtr->Type));
	else
		SunFrame = SetAbsFrameIndex (SunFrame,
				STAR_TYPE (CurStarDescPtr->Type) * 32);

	pCurDesc = &pSolarSysState->SunDesc[0];
	pCurDesc->pPrevDesc = 0;
	pCurDesc->rand_seed = RandomContext_Random (SysGenRNG);

	pCurDesc->data_index = STAR_TYPE (CurStarDescPtr->Type);
	pCurDesc->location.x = 0;
	pCurDesc->location.y = 0;
	pCurDesc->image.origin = pCurDesc->location;
	pCurDesc->image.frame = SunFrame;

	(*pSolarSysState->genFuncs->generatePlanets) (pSolarSysState);
	if (GET_GAME_STATE (PLANETARY_CHANGE))
	{
		PutPlanetInfo ();
		SET_GAME_STATE (PLANETARY_CHANGE, 0);
	}

	for (i = 0, pCurDesc = pSolarSysState->PlanetDesc;
			i < MAX_PLANETS; ++i, ++pCurDesc)
	{
		pCurDesc->pPrevDesc = &pSolarSysState->SunDesc[0];
		pCurDesc->image.origin = pCurDesc->location;
		if (i >= pSolarSysState->SunDesc[0].NumPlanets)
		{
			pCurDesc->image.frame = 0;
		}
		else
		{
			COUNT index;
			SYSTEM_INFO SysInfo;

			DoPlanetaryAnalysis (&SysInfo, pCurDesc);
			index = (SysInfo.PlanetInfo.SurfaceTemperature + 250) / 100;
			if (index >= NUM_TEMP_RANGES)
				index = NUM_TEMP_RANGES - 1;
			pCurDesc->temp_color = temp_color_array[index];
		}
		pCurDesc->frame_offset = UNDEFINED_OFFSET;
	}

	sortPlanetPositions ();

	if (!GLOBAL (ip_planet))
	{	// Outer system
		pSolarSysState->pBaseDesc = pSolarSysState->PlanetDesc;
		pSolarSysState->pOrbitalDesc = NULL;
	}
	else
	{	// Inner system
		pSolarSysState->SunDesc[0].location = GLOBAL (ip_location);
		GLOBAL (ip_location) = displayToLocation (
				GLOBAL (ShipStamp.origin), MAX_ZOOM_RADIUS);

		i = GLOBAL (ip_planet) - 1;
		pSolarSysState->pOrbitalDesc = &pSolarSysState->PlanetDesc[i];
		GenerateMoons (pSolarSysState, pSolarSysState->pOrbitalDesc);
		pSolarSysState->pBaseDesc = pSolarSysState->MoonDesc;

		SET_GAME_STATE (PLANETARY_LANDING, 0);
	}

	initSolarSysSISCharacteristics ();

	if (GLOBAL (in_orbit))
	{	// Only when loading a game into orbital
		i = GLOBAL (in_orbit) - 1;
		if (i == 0)
		{	// Orbiting the planet itself
			orbital = pSolarSysState->pBaseDesc->pPrevDesc;
		}
		else
		{	// Orbiting a moon
			// -1 because planet itself is 1, and moons have to be 1-based
			i -= 1;
			orbital = &pSolarSysState->MoonDesc[i];
		}
		GLOBAL (ip_location) = pSolarSysState->SunDesc[0].location;
		GLOBAL (in_orbit) = 0;
	}
	else
	{
		i = GLOBAL (ShipFacing);
		// XXX: Solar system reentry test depends on ShipFacing != 0
		if (i == 0)
			++i;

		GLOBAL (ShipStamp.frame) = SetAbsFrameIndex (SISIPFrame, i - 1);
	}

	return orbital;
}

static void
saveNonOrbitalLocation (void)
{
	// XXX: Solar system reentry test depends on ShipFacing != 0
	GLOBAL (ShipFacing) = GetFrameIndex (GLOBAL (ShipStamp.frame)) + 1;
	GLOBAL (in_orbit) = 0;
	if (!playerInInnerSystem ())
	{
		GLOBAL (ip_planet) = 0;
	}
	else
	{
		// ip_planet is 1-based because code tests for ip_planet!=0
		GLOBAL (ip_planet) = 1 + planetIndex (pSolarSysState,
				pSolarSysState->pOrbitalDesc);
		GLOBAL (ip_location) = pSolarSysState->SunDesc[0].location;
	}
}

static void
FreeSolarSys (void)
{
	COUNT i;
	PLANET_DESC *pCurDesc;

	if (pSolarSysState->InIpFlight)
	{
		pSolarSysState->InIpFlight = FALSE;
			
		if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
			saveNonOrbitalLocation ();
	}
	
	DestroyDrawable (ReleaseDrawable (SolarSysFrame));
	SolarSysFrame = NULL;
	
	StopMusic ();

	if (optTexturedPlanets)
	{
		// BW: clean up data generated for textured IP planets
		for (i = 0, pCurDesc = pSolarSysState->PlanetDesc;
				i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
		{
			DestroyOrbitStruct (&pCurDesc->orbit, PLANET_DIAMETER);
			// JMS: Not sure if these do any good...
			DestroyStringTable (
					ReleaseStringTable (pSolarSysState->XlatRef));
			pSolarSysState->XlatRef = 0;
			DestroyDrawable (ReleaseDrawable (pSolarSysState->TopoFrame));
			pSolarSysState->TopoFrame = 0;
			DestroyColorMap (
					ReleaseColorMap (pSolarSysState->OrbitalCMap));
			pSolarSysState->OrbitalCMap = 0;
			// JMS ends.
		}

		// BW: if we were in Inner System, clean up data for textured IP
		// moons
		if (playerInInnerSystem ())
		{
			COUNT numMoons;
			if (worldIsMoon (pSolarSysState, pSolarSysState->pOrbitalDesc))
				numMoons =
					pSolarSysState->pOrbitalDesc->pPrevDesc->NumPlanets;
			else
				numMoons = pSolarSysState->pOrbitalDesc->NumPlanets;
		
			for (i = 0, pCurDesc = pSolarSysState->MoonDesc;
					i < numMoons; ++i, ++pCurDesc)
			{
				if (!(pCurDesc->data_index & WORLD_TYPE_SPECIAL))
				{
					SIZE diameterPick =
							pCurDesc->data_index > LAST_SMALL_ROCKY_WORLD ?
								LARGE_MOON_DIAMETER : MOON_DIAMETER;

					DestroyOrbitStruct (&pCurDesc->orbit, diameterPick);
					pCurDesc->frame_offset = UNDEFINED_OFFSET;
				}
			}
		}
	}
	else
	{	// we need to destroy DOS style planets because they are cut from
		// the main frame and stored separately therefore they aren't
		// destroyed with OrbitFrame
		// also reset all planet offsets
		for (i = 0, pCurDesc = pSolarSysState->PlanetDesc;
				i < pSolarSysState->SunDesc[0].NumPlanets; ++i, ++pCurDesc)
		{
			if (isPC (optPlanetStyle))
			{
				DestroyDrawable (ReleaseDrawable (pCurDesc->image.frame));
				pCurDesc->image.frame = 0;
			}
			pCurDesc->frame_offset = UNDEFINED_OFFSET;
		}

		if (playerInInnerSystem ())
		{
			COUNT numMoons;
			if (worldIsMoon (pSolarSysState, pSolarSysState->pOrbitalDesc))
				numMoons =
						pSolarSysState->pOrbitalDesc->pPrevDesc->
							NumPlanets;
			else
				numMoons = pSolarSysState->pOrbitalDesc->NumPlanets;

			for (i = 0, pCurDesc = pSolarSysState->MoonDesc;
					i < numMoons; ++i, ++pCurDesc)
			{
				if (!(pCurDesc->data_index & WORLD_TYPE_SPECIAL) &&
					isPC (optPlanetStyle))
				{
					DestroyDrawable (ReleaseDrawable (
							pCurDesc->image.frame));
					pCurDesc->image.frame = 0;
				}
				pCurDesc->frame_offset = UNDEFINED_OFFSET;
			}
		}
	}
	// FreeIPData ();
}

static FRAME
getCollisionFrame (PLANET_DESC *planet, COUNT WaitPlanet)
{
	if (pSolarSysState->WaitIntersect != (COUNT)~0
			&& pSolarSysState->WaitIntersect != WaitPlanet)
	{
		if (!IS_HD)
			return DecFrameIndex (stars_in_space);
		else
			return SetAbsFrameIndex (SpaceJunkFrame, 24);

	}
	else
	{	// Existing collisions are cleared only once the ship does not
		// intersect anymore with a full planet image
		return planet->image.frame;
	}
}

// Returns the planet with which the flagship is colliding
static PLANET_DESC *
CheckIntersect (void)
{
	COUNT i;
	PLANET_DESC *pCurDesc;
	INTERSECT_CONTROL ShipIntersect, PlanetIntersect;
	COUNT NewWaitPlanet;
	BYTE PlanetOffset, MoonOffset;

	// Check collisions with the system center object
	// This may be the planet in inner view, or the sun
	pCurDesc = pSolarSysState->pBaseDesc->pPrevDesc;
	PlanetOffset = pCurDesc - pSolarSysState->PlanetDesc + 1;
	MoonOffset = 1; // the planet itself

	ShipIntersect.IntersectStamp.origin = GLOBAL (ShipStamp.origin);
	ShipIntersect.EndPoint = ShipIntersect.IntersectStamp.origin;
	ShipIntersect.IntersectStamp.frame = GLOBAL (ShipStamp.frame);

	PlanetIntersect.IntersectStamp.origin.x =
			RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
	PlanetIntersect.IntersectStamp.origin.y =
			RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1);
	PlanetIntersect.EndPoint = PlanetIntersect.IntersectStamp.origin;
	
	PlanetIntersect.IntersectStamp.frame = getCollisionFrame (pCurDesc,
			MAKE_WORD (PlanetOffset, MoonOffset));

	/*log_add (log_Debug,"Nyt: x:%d, y:%d",
			PlanetIntersect.IntersectStamp.origin.x,
			PlanetIntersect.IntersectStamp.origin.y);*/
	
	// Start with no collisions
	NewWaitPlanet = 0;

	if (pCurDesc != pSolarSysState->SunDesc /* can't intersect with sun */
			&& DrawablesIntersect (&ShipIntersect,
			&PlanetIntersect, MAX_TIME_VALUE))
	{
#ifdef DEBUG_SOLARSYS
		log_add (log_Debug, "0: Planet %d, Moon %d", PlanetOffset,
				MoonOffset);
#endif /* DEBUG_SOLARSYS */
		NewWaitPlanet = MAKE_WORD (PlanetOffset, MoonOffset);
		if (pSolarSysState->WaitIntersect != (COUNT)~0
				&& pSolarSysState->WaitIntersect != NewWaitPlanet)
		{
			pSolarSysState->WaitIntersect = NewWaitPlanet;
#ifdef DEBUG_SOLARSYS
			log_add (log_Debug,
					"Star index = %d, Planet index = %d, <%d, %d>",
					CurStarDescPtr - star_array,
					pCurDesc - pSolarSysState->PlanetDesc,
					pSolarSysState->SunDesc[0].location.x,
					pSolarSysState->SunDesc[0].location.y);
#endif /* DEBUG_SOLARSYS */
			return pCurDesc;
		}
	}

	for (i = pCurDesc->NumPlanets,
			pCurDesc = pSolarSysState->pBaseDesc; i; --i, ++pCurDesc)
	{
		PlanetIntersect.IntersectStamp.origin = pCurDesc->image.origin;
		PlanetIntersect.EndPoint = PlanetIntersect.IntersectStamp.origin;
		if (playerInInnerSystem ())
		{
			PlanetOffset = pCurDesc->pPrevDesc -
					pSolarSysState->PlanetDesc;
			MoonOffset = pCurDesc - pSolarSysState->MoonDesc + 2;
		}
		else
		{
			PlanetOffset = pCurDesc - pSolarSysState->PlanetDesc;
			MoonOffset = 0;
		}
		++PlanetOffset;
		PlanetIntersect.IntersectStamp.frame = getCollisionFrame (pCurDesc,
				MAKE_WORD (PlanetOffset, MoonOffset));

		/*log_add (log_Debug, "Ship x:%d y:%d. Planet x:%d, y:%d",
				ShipIntersect.IntersectStamp.origin.x,
				ShipIntersect.IntersectStamp.origin.y,
				PlanetIntersect.IntersectStamp.origin.x,
				PlanetIntersect.IntersectStamp.origin.y);*/
		
		if (DrawablesIntersect (&ShipIntersect,
				&PlanetIntersect, MAX_TIME_VALUE))
		{
#ifdef DEBUG_SOLARSYS
			log_add (log_Debug, "1: Planet %d, Moon %d", PlanetOffset,
					MoonOffset);
#endif /* DEBUG_SOLARSYS */
			NewWaitPlanet = MAKE_WORD (PlanetOffset, MoonOffset);
			
			if (pSolarSysState->WaitIntersect == (COUNT)~0)
			{	// All collisions disallowed, but the ship is still
				// colliding with something. Collisions will remain
				// disabled.
				break;
			}
			else if (pSolarSysState->WaitIntersect == NewWaitPlanet)
			{	// Existing and continued collision -- ignore
				continue;
			}
			
			// Collision with a new planet/moon. This may cause a
			// transition to an inner system or start an orbital view.
			pSolarSysState->WaitIntersect = NewWaitPlanet;
			return pCurDesc;
		}
	}

	// This records the planet/moon with which the ship just collided
	// It may be a previously existing collision also (the value won't
	// change) If all collisions were disabled, this will reenable then
	// once the ship stops colliding with any planets
	if (pSolarSysState->WaitIntersect != (COUNT)~0 || NewWaitPlanet == 0)
		pSolarSysState->WaitIntersect = NewWaitPlanet;

	return NULL;
}

static void
GetOrbitRect (RECT *pRect, COORD dx, COORD dy, SIZE radius,
		int xnumer, int ynumer, int denom)
{
	pRect->corner.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1)
			+ (long)-dx * xnumer / denom;
	pRect->corner.y = RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1)
			+ (long)-dy * ynumer / denom;
	pRect->extent.width = (long)radius * (xnumer << 1) / denom;
	pRect->extent.height = pRect->extent.width >> 1;
}

static void
GetPlanetOrbitRect (RECT *r, PLANET_DESC *planet, int sizeNumer,
		int dyNumer, int denom)
{
	COORD dx, dy;

	dx = planet->radius;
	dy = planet->radius;
	if (sizeNumer > DISPLAY_FACTOR)
	{
		dx = dx + planet->location.x;
		dy = (dy + planet->location.y) << 1;
	}
	GetOrbitRect (r, dx, dy, planet->radius, sizeNumer, dyNumer, denom);
}

static void
SetPlanetOldFrame (PLANET_DESC *planet, COUNT index, COUNT color)
{
	FRAME oldFrame;
	RECT r;

	oldFrame = SetAbsFrameIndex (OrbitalFrame, index);
	GetFrameRect (oldFrame, &r);

	r.corner.y = 0; // messed because of sprite hotspot
	r.corner.x = (r.extent.width / NUM_PLANET_COLORS) * color;
	r.extent.width /= NUM_PLANET_COLORS;

	// If previous frame wasn't SpaceJunkFrame
	if (planet->image.frame && planet->size != 0)
	{
		DestroyDrawable (ReleaseDrawable (planet->image.frame));
		planet->image.frame = 0;
	}

	planet->image.frame = CaptureDrawable (CopyFrameRect (oldFrame, &r));
	SetFrameHot (planet->image.frame, GetFrameHot (oldFrame));
}

static void
ValidateOrbit (PLANET_DESC *planet, int sizeNumer, int dyNumer, int denom)
{
	COUNT index;
	
	if (optOrbitingPlanets)
	{
		// BW: recompute planet position to account for orbiting
		// COUNT newAngle;
		// newAngle = NORMALIZE_ANGLE(planet->angle
		// 		+ (COUNT)(daysElapsed() * planet->orb_speed));
		// planet->location.x = COSINE (newAngle, planet->radius);
		// planet->location.y = SINE (newAngle, planet->radius);
		double newAngle;
		newAngle = (planet->angle + daysElapsed() * planet->orb_speed)
				* M_PI / 32 - M_PI/2;
		planet->location.x = (COORD)(cos(newAngle) * planet->radius);
		planet->location.y = (COORD)(sin(newAngle) * planet->radius);
	}

	if (sizeNumer <= DISPLAY_FACTOR)
	{	// All planets in outer view, and moons in inner
		RECT r;

		GetPlanetOrbitRect (&r, planet, sizeNumer, dyNumer, denom);

		// Calculate the location of the planet's image
		r.corner.x += (r.extent.width >> 1);
		r.corner.y += (r.extent.height >> 1);
		r.corner.x += (long)planet->location.x * sizeNumer / denom;
		// Ellipse function always has coefficients a^2 = 2 * b^2
		r.corner.y += (long)planet->location.y * (sizeNumer / 2) / denom;

		planet->image.origin = r.corner;
	}

	// Calculate the size and lighting angle of planet's image and
	// set the image that will be drawn
	index = planet->data_index & ~WORLD_TYPE_SPECIAL;
	if (index < NUMBER_OF_PLANET_TYPES)
	{	// The world is a normal planetary body (planet or moon)
		BYTE Type;
		COUNT Size;
		COUNT angle;
		BYTE offset = 0;

		Type = PlanData[index].Type;
		Size = PLANSIZE (Type);
		if (sizeNumer > DISPLAY_FACTOR)
		{
			Size += 3;
		}
		else if (worldIsMoon (pSolarSysState, planet))
		{
			Size += 2;
		}
		else if (denom <= (MAX_ZOOM_RADIUS >> 2))
		{
			++Size;
			if (denom == MIN_ZOOM_RADIUS)
				++Size;
		}
		
		if (worldIsPlanet (pSolarSysState, planet))
		{	// Planet
			angle = ARCTAN (planet->location.x, planet->location.y);
		}
		else
		{	// Moon
			angle = ARCTAN (planet->pPrevDesc->location.x,
					planet->pPrevDesc->location.y);
		}

		offset = (Size << FACING_SHIFT) + NORMALIZE_FACING (
				ANGLE_TO_FACING (angle));

		if (planet->frame_offset != offset)
		{
			planet->frame_offset = offset;

			if (!optTexturedPlanets && isPC (optPlanetStyle))
				SetPlanetOldFrame (planet, offset, PLANCOLOR (Type));
			else
				planet->image.frame = SetAbsFrameIndex (OrbitalFrame,
						offset);

			// Normal planets have sizes start from 1st array element
			planet->size = Size + 1;
		}
	}
	else if (planet->data_index > WORLD_TYPE_SPECIAL &&
			planet->frame_offset == UNDEFINED_OFFSET)
	{
		BYTE frameNum = 0;

		// these have no animations - so set the frame once
		// it'll reset on leave
		planet->frame_offset = 0;
		planet->size = 0;

		switch (planet->data_index)
		{
			case PRECURSOR_STARBASE:
				frameNum = 23;
				break;
			case DESTROYED_STARBASE:
				frameNum = 22;
				break;
			case SA_MATRA:
				frameNum = 19;
				break;
			case HIERARCHY_STARBASE:
			default:
				frameNum = 16;
				break;
		}
		planet->image.frame = SetAbsFrameIndex (SpaceJunkFrame, frameNum);
	}
}

static void
DrawOrbit (PLANET_DESC *planet, int sizeNumer, int dyNumer, int denom)
{
	RECT r;

	GetPlanetOrbitRect (&r, planet, sizeNumer, dyNumer, denom);

	SetContextForeGroundColor (planet->temp_color);
	if (!optUnscaledStarSystem)
		DrawOval (&r, RES_BOOL (1, 6), FALSE);
	else
		DrawOval (&r, 1, FALSE);
}

static SIZE
FindRadius (POINT shipLoc, SIZE fromRadius)
{
	SIZE nextRadius;
	POINT displayLoc;

	do
	{
		fromRadius >>= 1;
		if (fromRadius > MIN_ZOOM_RADIUS)
			nextRadius = fromRadius >> 1;
		else
			nextRadius = 0; // scaleRect will be nul

		GetOrbitRect (&scaleRect, nextRadius, nextRadius, nextRadius,
				DISPLAY_FACTOR, RES_SCALE (DISPLAY_FACTOR_US >> 2),
				fromRadius);
		displayLoc = locationToDisplay (shipLoc, fromRadius);
	
	} while (pointWithinRect (scaleRect, displayLoc));

	return fromRadius;
}

static UWORD
flagship_inertial_thrust (COUNT CurrentAngle)
{
	SIZE max_ship_speed;
	SIZE cur_delta_x, cur_delta_y;
	COUNT TravelAngle, thrust_increment;
	VELOCITY_DESC *VelocityPtr;

	max_ship_speed = pSolarSysState->max_ship_speed << 1;
	thrust_increment = IP_SHIP_THRUST_INCREMENT << 1;
	VelocityPtr = &GLOBAL(velocity);
	GetCurrentVelocityComponents (VelocityPtr, &cur_delta_x, &cur_delta_y);
	TravelAngle = GetVelocityTravelAngle (VelocityPtr);
	if (TravelAngle == CurrentAngle
			&& cur_delta_x == COSINE (CurrentAngle, max_ship_speed)
			&& cur_delta_y == SINE (CurrentAngle, max_ship_speed))
	{	// already maxed-out acceleration
		return (SHIP_AT_MAX_SPEED);
	}
	else
	{
		SIZE delta_x, delta_y;
		DWORD desired_speed, max_speed;

		delta_x = cur_delta_x + COSINE (CurrentAngle, thrust_increment);
		delta_y = cur_delta_y + SINE (CurrentAngle, thrust_increment);
		desired_speed = VelocitySquared (delta_x, delta_y);
		max_speed = pow (max_ship_speed, 2);

		if (desired_speed <= max_speed)
		{	// normal acceleration
			SetVelocityComponents (VelocityPtr, delta_x, delta_y);
		}
		else if (TravelAngle == CurrentAngle)
		{	// normal max acceleration, same vector
			SetVelocityComponents (VelocityPtr,
					COSINE (CurrentAngle, max_ship_speed),
					SINE (CurrentAngle, max_ship_speed));
			return (SHIP_AT_MAX_SPEED);
		}
		else
		{	// maxed-out acceleration at an angle to current travel vector
			// thrusting at an angle while at max velocity only changes
			// the travel vector, but does not really change the velocity

			VELOCITY_DESC v = *VelocityPtr;

			DeltaVelocityComponents (&v,
					COSINE (CurrentAngle, thrust_increment >> 1)
					- COSINE (TravelAngle, thrust_increment),
					SINE (CurrentAngle, thrust_increment >> 1)
					- SINE (TravelAngle, thrust_increment));
			GetCurrentVelocityComponents (&v, &cur_delta_x, &cur_delta_y);
			desired_speed = VelocitySquared (cur_delta_x, cur_delta_y);

			if (desired_speed > max_speed)
			{
				SetVelocityComponents (VelocityPtr,
						COSINE (CurrentAngle, max_ship_speed),
						SINE (CurrentAngle, max_ship_speed));
				return (SHIP_AT_MAX_SPEED);
			}

			*VelocityPtr = v;
		}

		return 0;
	}
}

static SIZE
ScreenCompass (COUNT index)
{
	SIZE facing;
	POINT scrLoc = GLOBAL (ShipStamp.origin);
	EXTENT sisScr = { SIS_SCREEN_WIDTH, SIS_SCREEN_HEIGHT };
	BOOLEAN westOfCenter = scrLoc.x < (sisScr.width >> 1);
	BOOLEAN northOfCenter = scrLoc.y < (sisScr.height >> 1);

#define NORTH  0
#define EAST   4
#define SOUTH  8
#define WEST  12

	if (westOfCenter && northOfCenter)
	{	// NorthWest Quadrant
		if (scrLoc.x < scrLoc.y)
			facing = WEST;
		else
			facing = NORTH;
	}
	else if (!westOfCenter && northOfCenter)
	{	// NorthEast Quadrant
		if ((sisScr.width - scrLoc.x) < scrLoc.y)
			facing = EAST;
		else
			facing = NORTH;
	}
	else if (!westOfCenter && !northOfCenter)
	{	// SouthEast Quadrant
		if ((sisScr.width - scrLoc.x) < (sisScr.height - scrLoc.y))
			facing = EAST;
		else
			facing = SOUTH;
	}
	else if (westOfCenter && !northOfCenter)
	{	// SouthWest Quadrant
		if (scrLoc.x < (sisScr.height - scrLoc.y))
			facing = WEST;
		else
			facing = SOUTH;
	}
	else
		facing = index;

	return facing;
}

static POINT
TurnAbout (SIZE delta_x, SIZE delta_y)
{
	SIZE facing;
	COUNT frame_index;
	POINT delta = { delta_x, delta_y };

	if (!optSmartAutoPilot)
	{
		delta.y = -1;
		return delta;
	}

	frame_index = GetFrameIndex (GLOBAL (ShipStamp.frame));

	facing = ScreenCompass (frame_index);

	if ((int)facing != frame_index)
	{
		if (NORMALIZE_FACING (frame_index - facing)
				>= ANGLE_TO_FACING (HALF_CIRCLE))
		{
			facing = NORMALIZE_FACING (facing - 1);
			delta.x++;
		}
		else if ((int)frame_index != (int)facing)
		{
			facing = NORMALIZE_FACING (facing + 1);
			delta.x--;
		}
	}
	else
		delta.y = -1;

	return delta;
}

static void
ProcessShipControls (void)
{
	COUNT index;
	SIZE delta_x, delta_y;

#if defined(ANDROID) || defined(__ANDROID__)
	BATTLE_INPUT_STATE InputState = GetDirectionalJoystickInput(index, 0);

	if (InputState & BATTLE_THRUST_ALT)
#else
	if (CurrentInputState.key[PlayerControls[0]][KEY_UP]
			|| CurrentInputState.key[PlayerControls[0]][KEY_THRUST])
#endif
		delta_y = -1;
	else
		delta_y = 0;

	delta_x = 0;

#if defined(ANDROID) || defined(__ANDROID__)
	if (InputState & BATTLE_LEFT)
		delta_x -= 1;
	if (InputState & BATTLE_RIGHT)
		delta_x += 1;
#else
	if (CurrentInputState.key[PlayerControls[0]][KEY_LEFT])
		delta_x -= 1;
	if (CurrentInputState.key[PlayerControls[0]][KEY_RIGHT])
		delta_x += 1;
#endif
		
	if (delta_x || delta_y < 0)
	{
		GLOBAL (autopilot.x) = ~0;
		GLOBAL (autopilot.y) = ~0;
	}
	else if (GLOBAL (autopilot.x) != ~0 && GLOBAL (autopilot.y) != ~0)
	{
		POINT delta;
		delta = TurnAbout (delta_x, delta_y);

		delta_x = delta.x;
		delta_y = delta.y;
	}
	else
		delta_y = 0;

	index = GetFrameIndex (GLOBAL (ShipStamp.frame));
	if (pSolarSysState->turn_counter)
		--pSolarSysState->turn_counter;
	else if (delta_x)
	{
		if (delta_x < 0)
			index = NORMALIZE_FACING (index - 1);
		else
			index = NORMALIZE_FACING (index + 1);

		GLOBAL (ShipStamp.frame) =
				SetAbsFrameIndex (GLOBAL (ShipStamp.frame), index);

		pSolarSysState->turn_counter = pSolarSysState->turn_wait;
	}
	if (pSolarSysState->thrust_counter)
		--pSolarSysState->thrust_counter;
	else if (delta_y < 0)
	{
#define THRUST_WAIT 1
		flagship_inertial_thrust (FACING_TO_ANGLE (index));

		pSolarSysState->thrust_counter = THRUST_WAIT;
	}
}

static void
enterInnerSystem (PLANET_DESC *planet)
{
#define INNER_ENTRY_DISTANCE  (MIN_MOON_RADIUS + ((MAX_GEN_MOONS - 1) \
		* MOON_DELTA) + (MOON_DELTA / 4)) + RES_SCALE (5)
	COUNT angle;

	// Calculate the inner system entry location and facing
	angle = FACING_TO_ANGLE (GetFrameIndex (GLOBAL (ShipStamp.frame)))
			+ HALF_CIRCLE;
	GLOBAL (ShipStamp.origin.x) = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1)
			+ COSINE (angle, INNER_ENTRY_DISTANCE);
	GLOBAL (ShipStamp.origin.y) = RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1)
			+ SINE (angle, INNER_ENTRY_DISTANCE);
	if (GLOBAL (ShipStamp.origin.y) < 0)
		GLOBAL (ShipStamp.origin.y) = RES_SCALE (1);
	else if (GLOBAL (ShipStamp.origin.y) >= SIS_SCREEN_HEIGHT)
		GLOBAL (ShipStamp.origin.y) =
				(SIS_SCREEN_HEIGHT - RES_SCALE (1)) - RES_SCALE (1);

	GLOBAL (ip_location) = displayToLocation (
			GLOBAL (ShipStamp.origin), MAX_ZOOM_RADIUS);
	
	pSolarSysState->SunDesc[0].location =
			planetOuterLocation (planetIndex (pSolarSysState, planet));
	ZeroVelocityComponents (&GLOBAL (velocity));

	GenerateMoons (pSolarSysState, planet);
	if (optTexturedPlanets)
		GenerateTexturedMoons (pSolarSysState, planet);

	pSolarSysState->pBaseDesc = pSolarSysState->MoonDesc;
	pSolarSysState->pOrbitalDesc = planet;
}

static void
leaveInnerSystem (PLANET_DESC *planet)
{
	COUNT outerPlanetWait;
	COUNT i;
	PLANET_DESC *pMoonDesc;

	pSolarSysState->pBaseDesc = pSolarSysState->PlanetDesc;
	pSolarSysState->pOrbitalDesc = NULL;

	outerPlanetWait = MAKE_WORD (
			planet - pSolarSysState->PlanetDesc + 1, 0);
	// BW: planet may have moved while we were into Inner System
	ValidateOrbit (planet, DISPLAY_FACTOR,
			RES_SCALE (DISPLAY_FACTOR_US / 4),
			pSolarSysState->SunDesc[0].radius);
	pSolarSysState->SunDesc[0].location =
			planetOuterLocation (planetIndex (pSolarSysState, planet));
	GLOBAL (ip_location) = pSolarSysState->SunDesc[0].location;
	XFormIPLoc (&GLOBAL (ip_location), &GLOBAL (ShipStamp.origin), TRUE);
	ZeroVelocityComponents (&GLOBAL (velocity));

	// Now the ship is in outer system (as per game logic)

	if (optTexturedPlanets) 
	{	// BW: clean up data generated for textured IP moons
		for (i = 0, pMoonDesc = pSolarSysState->MoonDesc;
			 i < planet->NumPlanets; ++i, ++pMoonDesc)
		{
			if (!(pMoonDesc->data_index & WORLD_TYPE_SPECIAL))
			{
				SIZE diameterPick =
					pMoonDesc->data_index > LAST_SMALL_ROCKY_WORLD ?
					LARGE_MOON_DIAMETER : MOON_DIAMETER;

				DestroyOrbitStruct (&pMoonDesc->orbit, diameterPick);
			}
			pMoonDesc->frame_offset = UNDEFINED_OFFSET;
		}
	}	// End clean up
	else if (isPC (optPlanetStyle))
	{
		for (i = 0, pMoonDesc = pSolarSysState->MoonDesc;
			i < planet->NumPlanets; ++i, ++pMoonDesc)
		{
			if (!(pMoonDesc->data_index & WORLD_TYPE_SPECIAL))
			{
				DestroyDrawable (ReleaseDrawable (pMoonDesc->image.frame));
				pMoonDesc->image.frame = 0;
			}
			pMoonDesc->frame_offset = UNDEFINED_OFFSET;
		}
	}

	pSolarSysState->WaitIntersect = outerPlanetWait;
	// See if we also intersect with another planet, and if we do,
	// disable collisions comletely until we stop intersecting
	// with any planet at all.
	CheckIntersect ();
	if (pSolarSysState->WaitIntersect != outerPlanetWait)
		pSolarSysState->WaitIntersect = (COUNT)~0;
}

static void
enterOrbital (PLANET_DESC *planet)
{
	ZeroVelocityComponents (&GLOBAL (velocity));
	pSolarSysState->pOrbitalDesc = planet;
	pSolarSysState->InOrbit = TRUE;
}

static BOOLEAN
CheckShipLocation (SIZE *newRadius)
{
	SIZE radius;
	BOOLEAN SISonScreen;

	radius = pSolarSysState->SunDesc[0].radius;
	*newRadius = pSolarSysState->SunDesc[0].radius;

	SISonScreen = (GLOBAL (ShipStamp.origin.x) < 0
			|| GLOBAL (ShipStamp.origin.x) >= SIS_SCREEN_WIDTH
			|| GLOBAL (ShipStamp.origin.y) < 0
			|| GLOBAL (ShipStamp.origin.y) >= SIS_SCREEN_HEIGHT);
	
	if (SISonScreen)
	{
		// The ship leaves the screen.
		if (!playerInInnerSystem ())
		{	// Outer zoom-out transition
			if (radius == MAX_ZOOM_RADIUS)
			{
				// The ship leaves IP.
				GLOBAL (CurrentActivity) |= END_INTERPLANETARY;
				return FALSE; // no location change
			}

			*newRadius = FindRadius (GLOBAL (ip_location),
					MAX_ZOOM_RADIUS << 1);
		}
		else
		{
			leaveInnerSystem (pSolarSysState->pOrbitalDesc);

			if (pointWithinRect (scaleRect, GLOBAL (ShipStamp.origin)))
			{
				*newRadius = FindRadius (GLOBAL (ip_location), radius);
				pSolarSysState->SunDesc[0].radius = *newRadius;
			}
			else if (SISonScreen)
			{
				*newRadius = FindRadius (GLOBAL (ip_location),
						MAX_ZOOM_RADIUS << 1);
				pSolarSysState->SunDesc[0].radius = *newRadius;
			}

		}
		
		return TRUE;
	}

	if (!playerInInnerSystem ()
			&& pointWithinRect (scaleRect, GLOBAL (ShipStamp.origin)))
	{	// Outer zoom-in transition
		*newRadius = FindRadius (GLOBAL (ip_location), radius);
		return TRUE;
	}

	if (GLOBAL (autopilot.x) == ~0 && GLOBAL (autopilot.y) == ~0)
	{	// Not on autopilot -- may collide with a planet
		PLANET_DESC *planet = CheckIntersect ();
		if (planet)
		{	// Collision with a planet
			if (playerInInnerSystem ())
			{	// Entering planet orbit (scans, etc.)
				enterOrbital (planet);
				return FALSE; // no location change
			}
			else
			{	// Transition to inner system
				enterInnerSystem (planet);
				return TRUE;
			}
		}
	}

	return FALSE; // no location change
}

static void
DrawSystemTransition (BOOLEAN inner)
{
	SetTransitionSource (NULL);
	BatchGraphics ();
	if (inner)
		DrawInnerSystem ();
	else
		DrawOuterSystem ();
	RedrawQueue (FALSE);
	ScreenTransition (optScrTrans, NULL);
	UnbatchGraphics ();
}

static void
TransitionSystemIn (void)
{
	SetContext (SpaceContext);
	DrawSystemTransition (playerInInnerSystem ());
}

static void
ScaleSystem (SIZE new_radius)
{
#ifdef SMOOTH_SYSTEM_ZOOM
	// XXX: This appears to have been an attempt to zoom the system view
	//   in a different way. This code zooms gradually instead of
	//   doing a crossfade from one zoom level to the other.
	// TODO: Do not loop here, and instead increment the zoom level
	//   in IP_frame() with a function drawing the new zoom. The ship
	//   controls are not handled in the loop, and the flagship
	//   can collide with a group while zooming, and that is not handled
	//   100% correctly.
#define NUM_STEPS 10
	COUNT i;
	SIZE old_radius;
	SIZE d, step;

	old_radius = pSolarSysState->SunDesc[0].radius;

	assert (old_radius != 0);
	assert (old_radius != new_radius);

	d = new_radius - old_radius;
	step = d / NUM_STEPS;

	for (i = 0; i < NUM_STEPS - 1; ++i)
	{
		pSolarSysState->SunDesc[0].radius += step;
		XFormIPLoc (&GLOBAL (ip_location), &GLOBAL (ShipStamp.origin),
				TRUE);

		BatchGraphics ();
		DrawOuterSystem ();
		RedrawQueue (FALSE);
		UnbatchGraphics ();

		SleepThread (ONE_SECOND / 30);
	}
	
	// Final zoom step
	pSolarSysState->SunDesc[0].radius = new_radius;
	XFormIPLoc (&GLOBAL (ip_location), &GLOBAL (ShipStamp.origin), TRUE);
	
	BatchGraphics ();
	DrawOuterSystem ();
	RedrawQueue (FALSE);
	UnbatchGraphics ();
	
#else // !SMOOTH_SYSTEM_ZOOM
	RECT r;

	pSolarSysState->SunDesc[0].radius = new_radius;
	XFormIPLoc (&GLOBAL (ip_location), &GLOBAL (ShipStamp.origin), TRUE);

	GetContextClipRect (&r);
	SetTransitionSource (&r);
	BatchGraphics ();
	DrawOuterSystem ();
	RedrawQueue (FALSE);
	ScreenTransition (optScrTrans, &r);
	UnbatchGraphics ();
#endif // SMOOTH_SYSTEM_ZOOM
}

static void
RestoreSystemView (void)
{
	STAMP s;

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SolarSysFrame;
	DrawStamp (&s);
}

// JMS: This animates the truespace suns!
#define SUN_ANIMFRAMES_NUM 32
static void
AnimateSun (SIZE radius)
{
	PLANET_DESC *pSunDesc = &pSolarSysState->SunDesc[0];
	PLANET_DESC *pNearestPlanetDesc = &pSolarSysState->PlanetDesc[0];
	static COUNT sunAnimIndex = 0;
	COUNT zoomLevelIndex = 0;
	
	// Advance to the next frame.
	sunAnimIndex++;
	
	// Go back to start of the anim after advancing past the last frame.
	if (sunAnimIndex % SUN_ANIMFRAMES_NUM == 0)
		sunAnimIndex = 0;
	
	// Zoom according to how close we are to the sun.
	if (radius <= (MAX_ZOOM_RADIUS >> 1))
	{
		zoomLevelIndex += SUN_ANIMFRAMES_NUM;
		if (radius <= (MAX_ZOOM_RADIUS >> 2))
			zoomLevelIndex += SUN_ANIMFRAMES_NUM;
	}
	
	// Tell the imageset which frame it should use.
	pSunDesc->image.frame = SetRelFrameIndex (
			SunFrame, zoomLevelIndex + sunAnimIndex);
	
	// Draw the image.
	DrawStamp (&pSunDesc->image);
}

static void
DrawTexturedBody (PLANET_DESC* planet, STAMP s)
{
	int oldScale;
	int oldMode;
	SIZE moonDiameter;
	COUNT size = PBodySize[planet->size];
	
	BatchGraphics ();
	oldMode = SetGraphicScaleMode (TFB_SCALE_BILINEAR);

	if (worldIsMoon (pSolarSysState, planet))
	{
		moonDiameter = planet->data_index > LAST_SMALL_ROCKY_WORLD ?
				LARGE_MOON_DIAMETER : MOON_DIAMETER;

		oldScale = SetGraphicScale (
				GSCALE_IDENTITY * RES_SCALE (size) / moonDiameter);
	}
	else
		oldScale = SetGraphicScale (
				GSCALE_IDENTITY * RES_SCALE (size)
					/ PLANET_DIAMETER);

	s.frame = planet->orbit.SphereFrame;
	DrawStamp (&s);

	if (planet->orbit.ObjectFrame)
	{
		s.frame = planet->orbit.ObjectFrame;
		DrawStamp (&s);
	}

	SetGraphicScale (oldScale);
	SetGraphicScaleMode (oldMode);
	
	UnbatchGraphics ();
}

void
RotatePlanets (BOOLEAN IsInnerSystem)
{
	PLANET_DESC *planet;
	PLANET_DESC *moon;
	static SIZE frameCounter;
	COUNT i;

	// Do not try to rotate planets that haven't been generated yet.
	if (!pSolarSysState->PlanetDesc->orbit.lpTopoData)
		return;

	++frameCounter;
	
	if (IsInnerSystem)
	{
		planet = pSolarSysState->pOrbitalDesc;
		PrepareNextRotationFrameForIP (planet, frameCounter);
		for (i = 0; i < planet->NumPlanets; ++i)
		{
			moon = &pSolarSysState->MoonDesc[i];
			if (!(moon->data_index & WORLD_TYPE_SPECIAL))
				PrepareNextRotationFrameForIP (moon, frameCounter);
		}
	}
	else
	{
		for (i = pSolarSysState->SunDesc[0].NumPlanets,
				planet = &pSolarSysState->PlanetDesc[0]; i; --i, ++planet)
			PrepareNextRotationFrameForIP (planet, frameCounter);
	}
}

// Normally called by DoIpFlight() to process a frame
static void
IP_frame (void)
{
	BOOLEAN locChange;
	SIZE newRadius;

	SetContext (SpaceContext);
	ProcessShipControls ();
	
	locChange = CheckShipLocation (&newRadius);
	if (locChange)
	{
		if (playerInInnerSystem ())
		{	// Entering inner system
			DrawSystemTransition (TRUE);
		}
		else if (pSolarSysState->SunDesc[0].radius == newRadius)
		{	// Leaving inner system to outer
			DrawSystemTransition (FALSE);
		}
		else
		{	// Zooming outer system
			ScaleSystem (newRadius);
		}
	}
	else if (!pSolarSysState->InOrbit)
	{	// Just flying around, minding own business..
		BatchGraphics ();
		RestoreSystemView ();
		if (IS_HD || optOrbitingPlanets || optTexturedPlanets)
		{
			// BW: recompute planet position to account for orbiting
			if (playerInInnerSystem ())
			{
				// Draw the inner system view
				ValidateInnerOrbits ();
				DrawInnerPlanets (pSolarSysState->pOrbitalDesc);
			}
			else
			{
				// Draw the outer system view
				ValidateOrbits ();
				DrawOuterPlanets (pSolarSysState->SunDesc[0].radius);
			}
		}
		RedrawQueue (FALSE);
		DrawAutoPilotMessage (FALSE);
		UnbatchGraphics ();
	}
}

static BOOLEAN
CheckZoomLevel (void)
{
	BOOLEAN InnerSystem;
	POINT shipLoc;

	InnerSystem = playerInInnerSystem ();
	if (InnerSystem)
		shipLoc = pSolarSysState->SunDesc[0].location;
	else
		shipLoc = GLOBAL (ip_location);

	pSolarSysState->SunDesc[0].radius = FindRadius (shipLoc,
			MAX_ZOOM_RADIUS << 1);
	if (!InnerSystem)
	{	// Update ship stamp since the radius probably changed
		XFormIPLoc (&shipLoc, &GLOBAL (ShipStamp.origin), TRUE);
	}

	return InnerSystem;
}

static void
ValidateOrbits (void)
{
	COUNT i;
	PLANET_DESC *planet;

	for (i = pSolarSysState->SunDesc[0].NumPlanets,
			planet = &pSolarSysState->PlanetDesc[0]; i; --i, ++planet)
	{
		ValidateOrbit (planet, DISPLAY_FACTOR,
				RES_SCALE (DISPLAY_FACTOR_US / 4),
				pSolarSysState->SunDesc[0].radius);
	}
}

static void
ValidateInnerOrbits (void)
{
	COUNT i;
	PLANET_DESC *planet;

	assert (playerInInnerSystem ());

	planet = pSolarSysState->pOrbitalDesc;
	ValidateOrbit (planet, RES_SCALE (DISPLAY_FACTOR_US * 4),
			DISPLAY_FACTOR, planet->radius);

	for (i = 0; i < planet->NumPlanets; ++i)
	{
		PLANET_DESC *moon = &pSolarSysState->MoonDesc[i];
		ValidateOrbit (moon, 2, 1, 2);
	}
}

static void
DrawInnerSystem (void)
{
	ValidateInnerOrbits ();
	DrawSystem (pSolarSysState->pOrbitalDesc->radius, TRUE);
	if (IS_HD || optOrbitingPlanets || optTexturedPlanets)
		DrawInnerPlanets (pSolarSysState->pOrbitalDesc);
	DrawSISTitle (GLOBAL_SIS (PlanetName));
}

static void
DrawOuterSystem (void)
{
	ValidateOrbits ();
	DrawSystem (pSolarSysState->SunDesc[0].radius, FALSE);
	if (IS_HD || optOrbitingPlanets || optTexturedPlanets)
		DrawOuterPlanets (pSolarSysState->SunDesc[0].radius);
	DrawHyperCoords (CurStarDescPtr->star_pt);
}

RESOURCE
spaceMusicSwitch (BYTE SpeciesID)
{
	switch (SpeciesID)
	{
		case ARILOU_ID:
			return ARILOU_SPACE_MUSIC;
		case CHMMR_ID:
			return CHMMR_SPACE_MUSIC;
		case ORZ_ID:
			return ORZ_SPACE_MUSIC;
		case PKUNK_ID:
			return PKUNK_SPACE_MUSIC;
		case SPATHI_ID:
			return SPATHI_SPACE_MUSIC;
		case SUPOX_ID:
			return SUPOX_SPACE_MUSIC;
		case THRADDASH_ID:
			return THRADDASH_SPACE_MUSIC;
		case UTWIG_ID:
			return UTWIG_SPACE_MUSIC;
		case VUX_ID:
			return VUX_SPACE_MUSIC;
		case YEHAT_ID:
			return YEHAT_SPACE_MUSIC;
		case DRUUGE_ID:
			return DRUUGE_SPACE_MUSIC;
		case ILWRATH_ID:
			return ILWRATH_SPACE_MUSIC;
		case MYCON_ID:
			return MYCON_SPACE_MUSIC;
		case UMGAH_ID:
			return UMGAH_SPACE_MUSIC;
		case UR_QUAN_ID:
		case KOHR_AH_ID:
			return URQUAN_SPACE_MUSIC;
		case ZOQFOTPIK_ID:
			return ZOQFOTPIK_SPACE_MUSIC;
		case SYREEN_ID:
			return SYREEN_SPACE_MUSIC;
		case SA_MATRA_ID:
			return KOHRAH_SPACE_MUSIC;
		default:
			return IP_MUSIC;
	}
}

static void
playSpaceMusic (void)
{

	if (!SpaceMusic)
	{
		if (SpaceMusicOK)
		{
			findRaceSOI ();
			SpaceMusic = LoadMusic (spaceMusicSwitch (spaceMusicBySOI));
		}
		else
		{
			SpaceMusic = LoadMusic (IP_MUSIC);
		}
	}

	// Do not start playing the music if we entered the solarsys only
	// to load a game (load invoked from Main menu)
	// XXX: This is quite hacky
	if (!PLRPlaying((MUSIC_REF)~0) &&
		(LastActivity != CHECK_LOAD || NextActivity))
	{
		PlayMusic (SpaceMusic, TRUE, 1);

		// Commented out for use in future version.
		//if (SpaceMusicPos[spaceMusicBySOI] > 0)
		//{
		//	FadeMusic (0, 0);
		//	FadeMusic (NORMAL_VOLUME, ONE_SECOND);
		//	SeekMusic (SpaceMusicPos[spaceMusicBySOI]);
		//}
	}
}

void
ResetSolarSys (void)
{
	// Originally there was a flash_task test here, however, I found no
	// cases where flash_task could be set at the time of call. The test
	// was probably needed on 3DO when IP_frame() was a task.
	assert (!pSolarSysState->InIpFlight);

	DrawMenuStateStrings (PM_STARMAP, -(PM_NAVIGATE - PM_SCAN));

	InitDisplayList ();
	// This also spawns the flagship element
	DoMissions ();

	// Figure out and note which planet/moon we just left, if any
	// This records any existing collision and prevents the ship
	// from entering planets until a new collision occurs.
	// TODO: this may need logic similar to one in leaveInnerSystem()
	//   for when the ship collides with more than one planet at
	//   the same time. While quite rare, it's still possible.
	CheckIntersect ();
	
	pSolarSysState->InIpFlight = TRUE;

	playSpaceMusic ();
}

static void
ReloadSolarSys (void)
{
	// START_ENCOUNTER could be set by Devices menu a number of ways:
	// Talking Pet, Sun Device or a Caster over Chmmr, or
	// a Caster for Ilwrath
	// Could also have blown self up with Utwig Bomb
	if (!(GLOBAL(CurrentActivity) & (START_ENCOUNTER |
		CHECK_ABORT | CHECK_LOAD))
		&& GLOBAL_SIS(CrewEnlisted) != (COUNT)~0)
	{	// Reload the system and return to the inner view
		PLANET_DESC* orbital = LoadSolarSys();
		assert(!orbital);
		CheckZoomLevel();
		ValidateOrbits();
		ValidateInnerOrbits();
		ResetSolarSys();

		if (optTexturedPlanets)
		{
			if (worldIsMoon(pSolarSysState, pSolarSysState->pOrbitalDesc))
				GenerateTexturedMoons(pSolarSysState,
					pSolarSysState->pOrbitalDesc->pPrevDesc);
			else
				GenerateTexturedMoons(pSolarSysState,
					pSolarSysState->pOrbitalDesc);
		}

		RepairSISBorder();
		TransitionSystemIn();
	}
}

static void
EnterPlanetOrbit (void)
{
	if (pSolarSysState->InIpFlight)
	{	// This means we hit a planet in IP flight; not a Load into orbit
		FreeSolarSys ();

		if (worldIsMoon (pSolarSysState, pSolarSysState->pOrbitalDesc))
		{	// Moon -- use its origin
			// XXX: The conversion functions do not error-correct, so the
			//   point we set here will change once flag_ship_preprocess()
			//   in ipdisp.c starts over again.
			GLOBAL (ShipStamp.origin) =
					pSolarSysState->pOrbitalDesc->image.origin;

			// JMS_GFX: Draw the moon letter when orbiting a moon
			if (!(GetNamedPlanetaryBody ()) && isPC (optWhichFonts)
					&& (pSolarSysState->pOrbitalDesc->data_index
						!= HIERARCHY_STARBASE
					&& pSolarSysState->pOrbitalDesc->data_index
						!= DESTROYED_STARBASE
					&& pSolarSysState->pOrbitalDesc->data_index
						!= PRECURSOR_STARBASE))
			{
				snprintf (GLOBAL_SIS (PlanetName)
						+ strlen (GLOBAL_SIS (PlanetName)), 3, "-%c%c",
						'A' + moonIndex (
							pSolarSysState, pSolarSysState->pOrbitalDesc),
						'\0');
				DrawSISTitle (GLOBAL_SIS (PlanetName));
			}
		}
		else
		{	// Planet -- its origin is for the outer view, so use mid-screen
			GLOBAL (ShipStamp.origin.x) =
					RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
			GLOBAL (ShipStamp.origin.y) =
					RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1);
		}
	}

	GetPlanetInfo ();
	(*pSolarSysState->genFuncs->generateOrbital) (pSolarSysState,
			pSolarSysState->pOrbitalDesc);
	LastActivity &= ~(CHECK_LOAD | CHECK_RESTART);
	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD |
			START_ENCOUNTER)) || GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
			|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
			return;

	// Implement a to-do in generate.h for a better test
	if (pSolarSysState->TopoFrame)
	{	// We've entered orbit; LoadPlanet() called planet surface-gen code
		PlanetOrbitMenu ();
		FreePlanet ();
	}
	// Otherwise, generateOrbital function started a homeworld
	// conversation, and we did not get to the planet no matter what.
}

static void
InitSolarSys (void)
{
	BOOLEAN InnerSystem;
	BOOLEAN Reentry;
	PLANET_DESC *orbital;


	LoadIPData ();
	LoadLanderData ();

	Reentry = (GLOBAL (ShipFacing) != 0);
	if (!Reentry)
	{
		GLOBAL (autopilot.x) = ~0;
		GLOBAL (autopilot.y) = ~0;

		GLOBAL (ShipStamp.origin.x) =
				RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
		GLOBAL (ShipStamp.origin.y) = SIS_SCREEN_HEIGHT - RES_SCALE (2);
		
		GLOBAL (ip_location) = displayToLocation (
				GLOBAL (ShipStamp.origin),
				MAX_ZOOM_RADIUS);
	}


	StarsFrame = CreateStarBackGround (FALSE);
	
	SetContext (SpaceContext);
	SetContextFGFrame (Screen);
	SetContextBackGroundColor (BLACK_COLOR);
	

	orbital = LoadSolarSys ();
	InnerSystem = CheckZoomLevel ();
	ValidateOrbits ();

	if (InnerSystem)
		ValidateInnerOrbits ();

	if (Reentry)
	{
		(*pSolarSysState->genFuncs->reinitNpcs) (pSolarSysState);
	}
	else
	{
		EncounterRace = -1;
		EncounterGroup = 0;
		GLOBAL (BattleGroupRef) = 0;
		ReinitQueue (&GLOBAL (ip_group_q));
		ReinitQueue (&GLOBAL (npc_built_ship_q));
		(*pSolarSysState->genFuncs->initNpcs) (pSolarSysState);
	}

	if (orbital)
	{
		enterOrbital (orbital);
	}
	else
	{	// Draw the borders, the system (inner or outer) and
		// fade/transition
		SetContext (SpaceContext);

		SetTransitionSource (NULL);
		BatchGraphics ();

		DrawSISFrame ();
		DrawSISMessage (NULL);

		ResetSolarSys ();

		if (!isStarMarked (INTERNAL_STAR_INDEX, "VISITED"))
			setStarMarked (INTERNAL_STAR_INDEX, "VISITED");

		if (isStarMarked (INTERNAL_STAR_INDEX, "PLYR_MARKER"))
			setStarMarked (INTERNAL_STAR_INDEX, "PLYR_MARKER");

		// JMS: This is to prevent flashing the 3do "navigate"
		// unnecessarily whilst starting a new game.
		// SetFlashRect (NULL);

		if (LastActivity == (CHECK_LOAD | CHECK_RESTART))
		{	// Starting a new game, NOT from load!
			// We have to fade the screen in from intro or menu
			DrawOuterSystem ();
			RedrawQueue (FALSE);
			UnbatchGraphics ();
			FadeScreen (FadeAllToColor, ONE_SECOND / 2);
			NewGameInit = TRUE;

			LastActivity = 0;
		}
		else if (LastActivity == CHECK_LOAD && !NextActivity)
		{	// Called just to load a game; invoked from Main menu
			// No point in drawing anything
			UnbatchGraphics ();
		}
		else
		{	// Entered a new system, or loaded into inner or outer
			if (InnerSystem)
			{
 				if (optTexturedPlanets)
					GenerateTexturedMoons (pSolarSysState,
							pSolarSysState->pOrbitalDesc);
 				DrawInnerSystem ();
			}
			else
			{
				DrawOuterSystem ();
			}
			RedrawQueue (FALSE);
			ScreenTransition (optScrTrans, NULL);
			UnbatchGraphics ();

			LastActivity &= ~CHECK_LOAD;
		}
	}
}

static void
endInterPlanetary (void)
{
	GLOBAL (CurrentActivity) &= ~END_INTERPLANETARY;
	
	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		// These are game state changing ops and so cannot be
		// called once another game has been loaded!
		(*pSolarSysState->genFuncs->uninitNpcs) (pSolarSysState);
		SET_GAME_STATE (USED_BROADCASTER, 0);
	}
}

// Find the closest planet to a point, in interplanetary.
static PLANET_DESC *
closestPlanetInterPlanetary (const POINT *point)
{
	BYTE i;
	BYTE numPlanets;
	DWORD bestDistSquared;
	PLANET_DESC *bestPlanet = NULL;

	assert(pSolarSysState != NULL);

	numPlanets = pSolarSysState->SunDesc[0].NumPlanets;

	bestDistSquared = (DWORD) -1;  // Maximum value of DWORD.
	for (i = 0; i < numPlanets; i++)
	{
		PLANET_DESC *planet = &pSolarSysState->PlanetDesc[i];

		SIZE dx = point->x - planet->image.origin.x;
		SIZE dy = point->y - planet->image.origin.y;

		DWORD distSquared = (DWORD) ((long) dx * dx + (long) dy * dy);
		if (distSquared < bestDistSquared)
		{
			bestDistSquared = distSquared;
			bestPlanet = planet;
		}
	}

	return bestPlanet;
}

static void
UninitSolarSys (void)
{
	FreeSolarSys ();

	// FreeLanderData (); // JMS: This is not needed since the landerframes
	// won't reload if they're already loaded once.
	FreeIPData (); // JMS This IS necessary.

	DestroyDrawable (ReleaseDrawable (StarsFrame));
	StarsFrame = NULL;

	if (GLOBAL (CurrentActivity) & END_INTERPLANETARY)
	{
		endInterPlanetary ();
		return;
	}

	if ((GLOBAL (CurrentActivity) & START_ENCOUNTER) && EncounterGroup)
	{
		GetGroupInfo (GLOBAL (BattleGroupRef), EncounterGroup);
		// Generate the encounter location name based on the closest planet

		if (GLOBAL (ip_planet) == 0)
		{
			PLANET_DESC *planet =
					closestPlanetInterPlanetary (
						&GLOBAL (ShipStamp.origin));

			(*pSolarSysState->genFuncs->generateName) (
					pSolarSysState, planet);
		}
	}
}

static void
CalcSunSize (PLANET_DESC *pSunDesc, SIZE radius)
{
	SIZE index = 0;

	if (radius <= (MAX_ZOOM_RADIUS >> 1))
	{
		++index;
		if (radius <= (MAX_ZOOM_RADIUS >> 2))
			++index;
	}

	pSunDesc->image.origin.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
	pSunDesc->image.origin.y = RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1);
	
	// JMS: Animating IP sun in hi-res modes...
	if (!IS_HD)
		pSunDesc->image.frame = SetRelFrameIndex (SunFrame, index);
	else
		pSunDesc->image.frame =
				SetRelFrameIndex (SunFrame, index * SUN_ANIMFRAMES_NUM);
}

static void
SetPlanetColorMap (PLANET_DESC *planet)
{
	COUNT index = planet->data_index & ~WORLD_TYPE_SPECIAL;
	assert (index < NUMBER_OF_PLANET_TYPES);
	SetColorMap (GetColorMapAddress (SetAbsColorMapIndex (OrbitalCMap,
			PLANCOLOR (PlanData[index].Type))));
}

static void
DrawInnerPlanets (PLANET_DESC *planet)
{
	STAMP s;
	COUNT i;
	PLANET_DESC *moon;

	s.origin.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
	s.origin.y = RES_SCALE (ORIG_SIS_SCREEN_HEIGHT >> 1);

	if (optTexturedPlanets)
	{	// Draw the planet image
		RotatePlanets (TRUE);
		DrawTexturedBody (planet, s);

		// Draw the moon images
		for (i = planet->NumPlanets, moon = pSolarSysState->MoonDesc;
				i; --i, ++moon)
		{
			if (moon->data_index & WORLD_TYPE_SPECIAL)
				DrawStamp (&moon->image);
			else
				DrawTexturedBody (moon, moon->image);
		}
	} 
	else 
	{	// Draw the planet image
		SetPlanetColorMap (planet);
		s.frame = planet->image.frame;

		i = planet->data_index & ~WORLD_TYPE_SPECIAL;
		if (i < NUMBER_OF_PLANET_TYPES
			&& (planet->data_index & PLANET_SHIELDED))
		{	// Shielded world looks "shielded" in inner view
			// s.frame = SetAbsFrameIndex (SpaceJunkFrame, 17);
			COUNT angle = ARCTAN (planet->location.x, planet->location.y);

			s.frame = SetAbsFrameIndex (OrbitalShield, NORMALIZE_FACING (
				ANGLE_TO_FACING (angle)));
		}
		DrawStamp (&s);

		// Draw the moon images
		for (i = planet->NumPlanets, moon = pSolarSysState->MoonDesc;
				i; --i, ++moon)
		{
			if (!(moon->data_index & WORLD_TYPE_SPECIAL))
				SetPlanetColorMap (moon);
			DrawStamp (&moon->image);
		}
	}
}

static void
DrawOuterPlanets (SIZE radius)
{
	SIZE index;
	PLANET_DESC *pCurDesc;
	
	CalcSunSize (&pSolarSysState->SunDesc[0], radius);
	if (optOrbitingPlanets)
		sortPlanetPositions ();
	
	index = pSolarSysState->FirstPlanetIndex;
	for (;;)
	{
		pCurDesc = &pSolarSysState->PlanetDesc[index];

		if (pCurDesc == &pSolarSysState->SunDesc[0])
		{	// It's a sun
			// Core part that animates sun
			if (IS_HD)
				AnimateSun (radius);
			else
				DrawStamp (&pCurDesc->image);
		}
		else
		{	// It's a planet
			if (optTexturedPlanets)
			{
				RotatePlanets (FALSE);
				DrawTexturedBody (pCurDesc, pCurDesc->image);
			}
			else
			{
				SetPlanetColorMap (pCurDesc);
				DrawStamp (&pCurDesc->image);
			}
		}
		if (index == pSolarSysState->LastPlanetIndex)
			break;
		index = pCurDesc->NextIndex;
	}
}

static void
DrawSystem (SIZE radius, BOOLEAN IsInnerSystem)
{
	BYTE i;
	PLANET_DESC *pCurDesc;
	PLANET_DESC *pBaseDesc;
	CONTEXT oldContext;
	STAMP s;

	if (optTexturedPlanets)
	{
		// BW: This to test if we have already rendered 
		if (!pSolarSysState->PlanetDesc->orbit.lpTopoData)
			GenerateTexturedPlanets ();
	}

	if (!SolarSysFrame)
	{	// Create the saved view graphic
		RECT clipRect;

		GetContextClipRect (&clipRect);
		SolarSysFrame = CaptureDrawable (CreateDrawable (WANT_PIXMAP,
				clipRect.extent.width, clipRect.extent.height, 1));
	}

	oldContext = SetContext (OffScreenContext);
	SetContextFGFrame (SolarSysFrame);
	SetContextClipRect (NULL);

	DrawStarBackGround ();

	pBaseDesc = pSolarSysState->pBaseDesc;
	if (IsInnerSystem)
	{	// Draw the inner system view *planet's* orbit segment
		pCurDesc = pSolarSysState->pOrbitalDesc;
		DrawOrbit (pCurDesc, RES_SCALE (DISPLAY_FACTOR_US * 4),
				DISPLAY_FACTOR, radius);
	}

	// Draw the planet orbits or moon orbits
	for (i = pBaseDesc->pPrevDesc->NumPlanets, pCurDesc = pBaseDesc;
			i; --i, ++pCurDesc)
	{
		if (IsInnerSystem)
			DrawOrbit (pCurDesc, 2, 1, 2);
		else
			DrawOrbit (pCurDesc, DISPLAY_FACTOR,
					RES_SCALE (DISPLAY_FACTOR_US / 4), radius);
	}

	if (!optOrbitingPlanets && !optTexturedPlanets)
	{
		if (IsInnerSystem) // Draw the inner system view
			DrawInnerPlanets (pSolarSysState->pOrbitalDesc);
		else // Draw the outer system view
			DrawOuterPlanets (radius);
	}

	SetContext (oldContext);

	// Draw the now-saved view graphic
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SolarSysFrame;
	DrawStamp (&s);
}

void
DrawStarBackGround (void)
{
	STAMP s;
	
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = StarsFrame;
	DrawStamp (&s);
}

FRAME
GetStarBackFround (void)
{
	if (StarsFrame)
		return StarsFrame;
	else
		return NULL;
}

void
BrightenNebula (FRAME nebula, BYTE factor)
{
	COUNT x, y;
	Color *pix;
	Color *map;
	RECT r;
	COUNT width, height;
	float f;

	GetFrameRect (nebula, &r);
	width = r.extent.width;
	height = r.extent.height;

	map = HMalloc (sizeof (Color) * width * height);
	ReadFramePixelColors (nebula, map, width, height);

	pix = map;

	for (y = 0; y < height; ++y)
	{
		for (x = 0; x < width; ++x, ++pix)
		{
			f = (pix->a << 2) * ((float)factor / 100);

			if (f > 0xC0)
				pix->a = 0xC0;
			else
				pix->a = (BYTE)f;
		}
	}

	WriteFramePixelColors (nebula, map, width, height);

	HFree (map);
}

void
DrawNebula (POINT star_point)
{
	const POINT solPoint = { SOL_X, SOL_Y };
	
	if (!pointsEqual (star_point, solPoint) || (classicPackPresent &&
			(LastActivity != CHECK_LOAD || NextActivity)))
	{	// To avoid loading nebulae in loading menu (Yay optimization)
		FRAME NebulaeFrame =
				CaptureDrawable (LoadGraphic (NEBULAE_PMAP_ANIM));
		const BYTE numNebulae = GetFrameCount (NebulaeFrame);

		if ((star_point.y % (numNebulae + 6)) < numNebulae
			|| classicPackPresent)
		{
			STAMP s;
			s.origin = MAKE_POINT (0, 0);
			s.frame = SetAbsFrameIndex (NebulaeFrame,
					star_point.x % numNebulae);
			if (optNebulaeVolume != 24)
				BrightenNebula (s.frame, optNebulaeVolume);

			DrawStamp (&s);
		}
		DestroyDrawable (ReleaseDrawable (NebulaeFrame));
		NebulaeFrame = 0;
	}
}

FRAME
CreateStarBackGround (BOOLEAN encounter)
{
	COUNT i, j;
	DWORD rand_val;
	STAMP s;
	CONTEXT oldContext;
	RECT clipRect;
	FRAME frame;
	POINT starPoint;
	RandomContext *OldSysGenRNG = SysGenRNG;
	BOOLEAN hdScaled = (!optUnscaledStarSystem || !IS_HD);

	if (encounter && !playerInSolarSystem ())
	{
		SpaceJunkFrame =
				CaptureDrawable (LoadGraphic (IPBKGND_MASK_PMAP_ANIM));
		SysGenRNG = RandomContext_New ();
	}

	if (CurStarDescPtr)
		starPoint = CurStarDescPtr->star_pt;
	else
	{
		starPoint = MAKE_POINT (
				LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
				LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y))
			);
	}

	// Use SpaceContext to find out the dimensions of the background
	oldContext = SetContext (SpaceContext);
	GetContextClipRect (&clipRect);

	// Prepare a pre-drawn stars frame for this system
	frame = CaptureDrawable (CreateDrawable (WANT_PIXMAP,
			clipRect.extent.width, clipRect.extent.height, 1));
	SetContext (OffScreenContext);
	SetContextFGFrame (frame);
	SetContextClipRect (NULL);
	SetContextBackGroundColor (BLACK_COLOR);

	ClearDrawable ();

	RandomContext_SeedRandom (SysGenRNG, GetRandomSeedForVar (starPoint));

#define NUM_DIM_PIECES 8
	s.frame = SetAbsFrameIndex (SpaceJunkFrame, hdScaled ? 0 : 26);
	for (i = 0; i < NUM_DIM_PIECES; ++i)
	{
#define NUM_DIM_DRAWN 5
		for (j = 0; j < NUM_DIM_DRAWN; ++j)
		{
			rand_val = RandomContext_Random (SysGenRNG);
			s.origin.x = RES_SCALE (
					scaleSISDimensions (TRUE,
					LOWORD (rand_val) % widthHeightPicker (TRUE)
				));
			s.origin.y = RES_SCALE (
					scaleSISDimensions (FALSE,
					HIWORD (rand_val) % widthHeightPicker (FALSE)
				));

			DrawStamp (&s);
		}
		s.frame = IncFrameIndex (s.frame);
	}
#define NUM_BRT_PIECES 8
	for (i = 0; i < NUM_BRT_PIECES; ++i)
	{
#define NUM_BRT_DRAWN 30
		for (j = 0; j < (hdScaled ? NUM_BRT_DRAWN : 90); ++j)
		{
			rand_val = RandomContext_Random (SysGenRNG);
			s.origin.x = RES_SCALE (
					scaleSISDimensions (TRUE,
					LOWORD (rand_val) % widthHeightPicker (TRUE)
				));
			s.origin.y = RES_SCALE (
					scaleSISDimensions (FALSE,
					HIWORD (rand_val) % widthHeightPicker (FALSE)
				));

			DrawStamp (&s);
		}
		s.frame = IncFrameIndex (s.frame);
	}

	if (optNebulae)
		DrawNebula (starPoint);

	SetContext (oldContext);

	if (encounter && !playerInSolarSystem ())
	{
		DestroyDrawable (ReleaseDrawable (SpaceJunkFrame));
		SpaceJunkFrame = 0;
		RandomContext_Delete (SysGenRNG);
		SysGenRNG = OldSysGenRNG;
	}

	return frame;
}

void
XFormIPLoc (POINT *pIn, POINT *pOut, BOOLEAN ToDisplay)
{
	if (ToDisplay)
		*pOut = locationToDisplay (*pIn,
				pSolarSysState->SunDesc[0].radius);
	else
		*pOut = displayToLocation (*pIn,
				pSolarSysState->SunDesc[0].radius);
}

void
ExploreSolarSys (void)
{
	SOLARSYS_STATE SolarSysState;
	
	if (CurStarDescPtr == 0)
	{
		POINT universe;

		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		CurStarDescPtr = FindStar (0, &universe, 1, 1);
		if (!CurStarDescPtr)
		{
			log_add (log_Fatal,
					"ExploreSolarSys(): do not know where you are!");
			explode ();
		}
	}
	GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (CurStarDescPtr->star_pt.x);
	GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (CurStarDescPtr->star_pt.y);

	pSolarSysState = &SolarSysState;

	memset (pSolarSysState, 0, sizeof (*pSolarSysState));

	SolarSysState.genFuncs = getGenerateFunctions (CurStarDescPtr->Index);

	InitSolarSys ();
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	SolarSysState.InputFunc = DoIpFlight;
#if defined(ANDROID) || defined(__ANDROID__)
	TFB_SetOnScreenKeyboard_Melee();
	DoInput(&SolarSysState, FALSE);
	TFB_SetOnScreenKeyboard_Menu();
#else
	DoInput (&SolarSysState, FALSE);
#endif

	// Commented out for use in future version.
	/*if (LastActivity != CHECK_LOAD)
	{
		if (!SpaceMusicOK)
			spaceMusicBySOI = 0;

		SpaceMusicPos[spaceMusicBySOI] = PLRGetPos ();
	}*/

	UninitSolarSys ();
	pSolarSysState = 0;
}

UNICODE *
GetNamedPlanetaryBody (void)
{
	if (!CurStarDescPtr || !playerInSolarSystem ()
			|| !playerInInnerSystem ())
		return NULL; // Not inside an inner system, so no name

	assert (pSolarSysState->pOrbitalDesc != NULL);

	if (CurStarDescPtr->Index == SOL_DEFINED)
	{	// Planets and moons in Sol
		int planet;
		int moon;

		planet =
				planetIndex (pSolarSysState, pSolarSysState->pOrbitalDesc);

		if (worldIsPlanet (pSolarSysState, pSolarSysState->pOrbitalDesc))
		{	// A planet
			return GAME_STRING (PLANET_NUMBER_BASE + planet);
		}

		// Moons
		moon = moonIndex (pSolarSysState, pSolarSysState->pOrbitalDesc);
		switch (planet)
		{
			case 2: // Earth
				switch (moon)
				{
					case 0: // Starbase
						return GAME_STRING (STARBASE_STRING_BASE + 0);
					case 1: // Luna
						return GAME_STRING (PLANET_NUMBER_BASE + 9);
				}
				break;
			case 4: // Jupiter
				switch (moon)
				{
					case 0: // Io
						return GAME_STRING (PLANET_NUMBER_BASE + 10);
					case 1: // Europa
						return GAME_STRING (PLANET_NUMBER_BASE + 11);
					case 2: // Ganymede
						return GAME_STRING (PLANET_NUMBER_BASE + 12);
					case 3: // Callisto
						return GAME_STRING (PLANET_NUMBER_BASE + 13);
				}
				break;
			case 5: // Saturn
				if (moon == 0) // Titan
					return GAME_STRING (PLANET_NUMBER_BASE + 14);
				break;
			case 7: // Neptune
				if (moon == 0) // Triton
					return GAME_STRING (PLANET_NUMBER_BASE + 15);
				break;
			case 8: // Pluto
				if (moon == 0) // Charon
					return GAME_STRING (PLANET_NUMBER_BASE + 34);
				break;
		}
	}
	else if (CurStarDescPtr->Index == SAMATRA_DEFINED 
			&& matchWorld (pSolarSysState, pSolarSysState->pOrbitalDesc,
				pSolarSysState->SunDesc[0].PlanetByte,
				pSolarSysState->SunDesc[0].MoonByte))
	{	// Sa-Matra
		return GAME_STRING (PLANET_NUMBER_BASE + 32);
	}
	else if (CurStarDescPtr->Index > 0
			&& matchWorld (pSolarSysState, pSolarSysState->pOrbitalDesc,
				pSolarSysState->SunDesc[0].PlanetByte, MATCH_PLANET))
	{
		switch (CurStarDescPtr->Index)
		{
			case SHOFIXTI_DEFINED:     // Kyabetsu
				if (GET_GAME_STATE (KNOW_SHOFIXTI_HOMEWORLD))
					return GAME_STRING (PLANET_NUMBER_BASE + 35);
				break;
			case START_COLONY_DEFINED: // Unzervalt
				return GAME_STRING (PLANET_NUMBER_BASE + 33);
			case SPATHI_DEFINED:       // Spathiwa
				if (GET_GAME_STATE (KNOW_SPATHI_HOMEWORLD))
					return GAME_STRING (PLANET_NUMBER_BASE + 37);
				break;
			case SYREEN_DEFINED:       // Gaia
				if ((GET_GAME_STATE (SYREEN_HOME_VISITS)
						|| GET_GAME_STATE (SYREEN_KNOW_ABOUT_MYCON)))
					return GAME_STRING (PLANET_NUMBER_BASE + 39);
				break;
			case SLYLANDRO_DEFINED:    // Source
				if (GET_GAME_STATE (SLYLANDRO_HOME_VISITS))
					return GAME_STRING (PLANET_NUMBER_BASE + 36);
				break;
			case DRUUGE_DEFINED:       // Trade HQ
				if (GET_GAME_STATE (KNOW_DRUUGE_HOMEWORLD))
					return GAME_STRING (PLANET_NUMBER_BASE + 41);
				break;
			case EGG_CASE0_DEFINED:    // Syra
				return GAME_STRING (PLANET_NUMBER_BASE + 42);
			case UTWIG_DEFINED:        // Fahz
				if (GET_GAME_STATE (KNOW_UTWIG_HOMEWORLD))
					return GAME_STRING (PLANET_NUMBER_BASE + 40);
				break;
			case SUPOX_DEFINED:        // Vlik
				if (GET_GAME_STATE (SUPOX_STACK1) > 2)
					return GAME_STRING (PLANET_NUMBER_BASE + 38);
				break;
			default:
				return NULL;
		}
	}
	return NULL;
}

void
GetPlanetOrMoonName (UNICODE *buf, COUNT bufsize)
{
	UNICODE *named;
	UNICODE *tempbuf;
	int moon, i;
	BOOLEAN name_has_suffix = FALSE;

	named = GetNamedPlanetaryBody ();
	if (named)
	{
		utf8StringCopy (buf, bufsize, named);
		return;
	}

	// Either not named or we already have a name
	utf8StringCopy (buf, bufsize, GLOBAL_SIS (PlanetName));

	if (!playerInSolarSystem () || !playerInInnerSystem ()
			|| worldIsPlanet (pSolarSysState, pSolarSysState->pOrbitalDesc)
			|| is3DO (optWhichFonts))
	{	// Outer or inner system or orbiting a planet
		return;
	}

	// Orbiting an unnamed moon
	i = strlen (buf);
	tempbuf = buf;
	buf += i;
	bufsize -= i;
	moon = moonIndex (pSolarSysState, pSolarSysState->pOrbitalDesc);

	// log_add (log_Debug,"last %02d, i %d", tempbuf[i-1], i);
	// JMS: Prevent printing something like 'planet II-A-A' in summary
	// screen.
	if (i > 0)
	{
		if(tempbuf[i-1] == 'A' || tempbuf[i-1] == 'B'
				|| tempbuf[i-1] == 'C' || tempbuf[i-1] == 'D')
			name_has_suffix = TRUE;
	}

	if (bufsize >= 3 && !name_has_suffix)
	{
		snprintf (buf, bufsize, "-%c", 'A' + moon);
		buf[bufsize - 1] = '\0';
	}
}

void
SaveSolarSysLocation (void)
{
	assert (playerInSolarSystem ());

	// This is a two-stage saving procedure
	// Stage 1: called when saving from inner/outer view
	// Stage 2: called when saving from orbital

	if (!playerInPlanetOrbit ())
	{
		saveNonOrbitalLocation ();
	}
	else
	{	// In orbit around a planet.
		BYTE moon;

		// Update the starinfo.dat file if necessary.
		if (GET_GAME_STATE (PLANETARY_CHANGE))
		{
			PutPlanetInfo ();
			SET_GAME_STATE (PLANETARY_CHANGE, 0);
		}

		// GLOBAL (ip_planet) is already set
		assert (GLOBAL (ip_planet) != 0);

		// has to be at least 1 because code tests for in_orbit!=0
		moon = 1; /* the planet itself */
		if (worldIsMoon (pSolarSysState, pSolarSysState->pOrbitalDesc))
		{
			moon += moonIndex (pSolarSysState,
					pSolarSysState->pOrbitalDesc);
			// +1 because moons have to be 1-based
			moon += 1;
		}
		GLOBAL (in_orbit) = moon;
	}
}

static BOOLEAN
DoSolarSysMenu (MENU_STATE *pMS)
{
	BOOLEAN select = PulsedInputState.menu[KEY_MENU_SELECT];
	BOOLEAN handled;

	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
		return FALSE;

	handled = DoMenuChooser (pMS, PM_STARMAP);
	if (handled)
		return TRUE;

	if (LastActivity == CHECK_LOAD)
		select = TRUE; // Selected LOAD from main menu

	if (!select)
		return TRUE;

	SetFlashRect (NULL, FALSE);

	switch (pMS->CurState)
	{
		case EQUIP_DEVICE:
			select = DevicesMenu ();
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
			{	// Invoked Talking Pet or a Caster for Ilwrath
				// Going into conversation
				return FALSE;
			}
			break;
		case CARGO:
			CargoMenu ();
			break;
		case ROSTER:
			select = RosterMenu ();
			break;
		case GAME_MENU:
			if (!GameOptions ())
				return FALSE; // abort or load
			break;
		case STARMAP:
			StarMap ();
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return FALSE;

			TransitionSystemIn ();
			// Fall through !!!
		case NAVIGATION:
			return FALSE;
	}

	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (select)
		{	// 3DO menu jumps to NAVIGATE after a successful submenu run
			if (optWhichMenu != OPT_PC)
				pMS->CurState = NAVIGATION;
			DrawMenuStateStrings (PM_STARMAP, pMS->CurState);
		}
		SetFlashRect (SFR_MENU_3DO, FALSE);
	}

	return TRUE;
}

static void
SolarSysMenu (void)
{
	MENU_STATE MenuState;

	memset (&MenuState, 0, sizeof MenuState);

	if (LastActivity == CHECK_LOAD)
	{	// Selected LOAD from main menu
		MenuState.CurState = GAME_MENU;
	}
	else
	{
		DrawMenuStateStrings (PM_STARMAP, STARMAP);
		MenuState.CurState = STARMAP;
	}

	DrawStatusMessage (NULL);
	SetFlashRect (SFR_MENU_3DO, FALSE);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	MenuState.InputFunc = DoSolarSysMenu;
	DoInput (&MenuState, TRUE);

	if (!(GLOBAL (CurrentActivity) & CHECK_LOAD))
		DrawMenuStateStrings (PM_STARMAP, -NAVIGATION);
}

static BOOLEAN
DoIpFlight (SOLARSYS_STATE *pSS)
{
	static TimeCount NextTime;
	BOOLEAN cancel = PulsedInputState.menu[KEY_MENU_CANCEL];

	if (pSS->InOrbit)
	{	// CheckShipLocation() or InitSolarSys() sent us to orbital
#if defined(ANDROID) || defined(__ANDROID__)
		TFB_SetOnScreenKeyboard_Menu ();
		EnterPlanetOrbit ();
		TFB_SetOnScreenKeyboard_Melee ();
#else
		EnterPlanetOrbit ();
#endif
		SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		pSS->InOrbit = FALSE;
		ReloadSolarSys ();
	}
	else if (!NewGameInit && (cancel || LastActivity == CHECK_LOAD))
	{
#if defined(ANDROID) || defined(__ANDROID__)
		TFB_SetOnScreenKeyboard_Menu ();
		SolarSysMenu ();
		TFB_SetOnScreenKeyboard_Melee ();
#else
		SolarSysMenu ();
#endif
		SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	}
	else if (!(GLOBAL(CurrentActivity) & CHECK_ABORT))
	{
		static TimeCount TimeOutIP, TimeOutClock;
		TimeCount Now = GetTimeCounter ();

		assert (pSS->InIpFlight);

		if (Now >= TimeOutClock)
		{
			GameClockTick ();
			TimeOutClock = Now + CLOCK_FRAME_RATE;
		}

		if (Now >= TimeOutIP)
		{
			IP_frame ();
			TimeOutIP = Now + IP_FRAME_RATE;
		}

		if (NewGameInit)
		{
			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			SettingsMenu (FALSE);
			SolarSysMenu ();
			SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		}
	}

	return (!(GLOBAL (CurrentActivity)
			& (START_ENCOUNTER | END_INTERPLANETARY
			| CHECK_ABORT | CHECK_LOAD))
			&& GLOBAL_SIS (CrewEnlisted) != (COUNT)~0);
}

static int
widthHeightPicker (BOOLEAN is_width)
{
	switch (optStarBackground)
	{
	case 0:
		return (is_width ? ORIG_SIS_SCREEN_WIDTH : PC_SIS_SCREEN_HEIGHT);
	case 1:
		return (is_width ? THREEDO_SIS_SCREEN_WIDTH
				: THREEDO_SIS_SCREEN_HEIGHT);
	case 2:
		return (is_width ? (ORIG_SIS_SCREEN_WIDTH - 1)
				: ORIG_SIS_SCREEN_HEIGHT);
	case 3:
	default:
		return (is_width ? HDMOD_SIS_SCREEN_WIDTH
				: HDMOD_SIS_SCREEN_HEIGHT);
	}
}

static COORD
scaleSISDimensions (BOOLEAN is_width, COORD value)
{
	float percentage;
	int widthOrHeight = is_width ?
			ORIG_SIS_SCREEN_WIDTH : ORIG_SIS_SCREEN_HEIGHT;

	if (widthOrHeight == widthHeightPicker (is_width))
		percentage = 1;
	else
		percentage = scaleThing (widthOrHeight,
				widthHeightPicker (is_width));

	return (COORD)(value * percentage);
}

void
InitialIntersect (void)
{	// need to check collision with moons on load
	// rare but annoying
	CheckIntersect ();
}