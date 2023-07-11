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

#ifndef UQM_PLANETS_PLANETS_H_
#define UQM_PLANETS_PLANETS_H_

#include "libs/mathlib.h"
#include "options.h"

#define END_INTERPLANETARY START_INTERPLANETARY

#define ONE_YEAR 365.25
#ifndef M_PI
#define M_PI 3.141592653589
#endif

enum PlanetScanTypes
{
	MINERAL_SCAN = 0,
	ENERGY_SCAN,
	BIOLOGICAL_SCAN,

	NUM_SCAN_TYPES,
};

#define SC2_MAP_WIDTH 210
#define SC2_MAP_HEIGHT 67
#define UQM_MAP_WIDTH 243
#define UQM_MAP_HEIGHT 75
#define ORIGINAL_MAP_WIDTH \
		(optPlanetTexture ? UQM_MAP_WIDTH-1 : SC2_MAP_WIDTH)
#define ORIGINAL_MAP_HEIGHT \
		(optPlanetTexture ? UQM_MAP_HEIGHT : SC2_MAP_HEIGHT)
#define MAP_WIDTH RES_SCALE (UQM_MAP_WIDTH)
#define MAP_HEIGHT RES_SCALE (SC2_MAP_HEIGHT)
#define SCALED_MAP_WIDTH RES_SCALE \
		(is3DO (optSuperPC) ? UQM_MAP_WIDTH : SC2_MAP_WIDTH)

enum
{
	BIOLOGICAL_DISASTER = 0,
	EARTHQUAKE_DISASTER,
	LIGHTNING_DISASTER,
	LAVASPOT_DISASTER,

	/* additional lander sounds */
	LANDER_INJURED,
	LANDER_SHOOTS,
	LANDER_HITS,
	LIFEFORM_CANNED,
	LANDER_PICKUP,
	LANDER_FULL,
	LANDER_DEPARTS,
	LANDER_RETURNS,
	LANDER_DESTROYED
};

#define MAX_SCROUNGED (isPC (optSuperPC) ? 64 : 50)
#define MAX_HOLD_BARS (isPC (optSuperPC) ? 64 : 50)

#define SCALE_RADIUS(r) ((r) << 6)
#define UNSCALE_RADIUS(r) ((r) >> 6)
#define MAX_ZOOM_RADIUS SCALE_RADIUS(128)
#define MIN_ZOOM_RADIUS (MAX_ZOOM_RADIUS>>3)
#define EARTH_RADIUS SCALE_RADIUS(8)
#define EARTH_HOURS 240

#define MIN_PLANET_RADIUS SCALE_RADIUS (4)
#define MAX_PLANET_RADIUS SCALE_RADIUS (124)

#define DISPLAY_FACTOR RES_SCALE ((ORIG_SIS_SCREEN_WIDTH >> 1) - 8)
#define DISPLAY_FACTOR_US ((ORIG_SIS_SCREEN_WIDTH >> 1) - 8)

#define NUM_SCANDOT_TRANSITIONS 4

#define MIN_MOON_RADIUS RES_SCALE (35)
#define MOON_DELTA RES_SCALE (20)

#define MAX_SUNS 1
#define MAX_PLANETS 16
#define MAX_MOONS 4
#define MAX_GEN_MOONS MAX_MOONS
#define MAX_GEN_PLANETS 9

#define MAP_BORDER_HEIGHT RES_SCALE (5)
#define SCAN_SCREEN_HEIGHT \
		(SIS_SCREEN_HEIGHT - MAP_HEIGHT - MAP_BORDER_HEIGHT)

#define PC_ROTATION_RATE (ONE_SECOND / RES_SCALE (12))
#define THREEDO_ROTATION_RATE (ONE_SECOND * 6 / RES_SCALE (SC2_MAP_WIDTH))
#define UQM_ROTATION_RATE (ONE_SECOND * 12 / MAP_WIDTH)

#define PLANET_ROTATION_RATE(a) (!a ? PC_ROTATION_RATE : \
		(a == 1 ? THREEDO_ROTATION_RATE : UQM_ROTATION_RATE))
#define PLANET_ORG_Y ((SCAN_SCREEN_HEIGHT - SIS_ORG_Y) / 2)

#define NUM_RACE_RUINS 16

#define MIN_BRIGHTNESS(a) ((a == ENERGY_SCAN) ? (50) : (55))
#define MAX_BRIGHTNESS(a) ((a == ENERGY_SCAN) ? (90) : (135))

typedef struct planet_desc PLANET_DESC;
typedef struct star_desc STAR_DESC;
typedef struct node_info NODE_INFO;
typedef struct planet_orbit PLANET_ORBIT;
typedef struct solarsys_state SOLARSYS_STATE;

#include "generate.h"
#include "../units.h"
#include "lifeform.h"
#include "plandata.h"
#include "sundata.h"
 
typedef struct
{
	POINT p[4];
	DWORD m[4];
} MAP3D_POINT;

struct planet_orbit
{
	FRAME TopoZoomFrame;
			// 4x scaled topo image for planet-side
	SBYTE  *lpTopoData;
			// normal topo data; expressed in elevation levels
			// data is signed for planets other than gas giants
			// transformed to light variance map for 3d planet
	FRAME SphereFrame;
			// rotating 3d planet frames (current and next)
	FRAME ObjectFrame;
			// any extra planetary object (shield, atmo, rings)
			// automatically drawn if present
	FRAME TintFrame;
			// tinted topo images for current scan type (dynamic)
	Color TintColor;
			// the color of the last used tint
	Color *TopoColors;
			// RGBA version of topo image; for 3d planet
	Color **ScanColors;
			// RGBA version of scan colors; for 3d planet
	Color *ScratchArray;
			// temp RGBA data for whatever transforms (nuked often)
	FRAME WorkFrame;
			// any extra frame workspace (for dynamic objects)
	FRAME BackFrame;
			// background frame to make shields transparent with nebulae on
	// BW: extra stuff for animated IP
	DWORD **light_diff;
	MAP3D_POINT **map_rotate;
	// doubly dynamically allocated depending on map size

	// stuff to draw DOS spheres
	FRAME TopoMask;
	BYTE *sphereBytes;
	COLORMAP sphereMap;
	COUNT scanType;

	FRAME Shade;
	Color *ShadeColors;
};

#if defined(__cplusplus)
extern "C" {
#endif

struct planet_desc
{
	DWORD rand_seed;    // seed for topography and node generation

	BYTE data_index;    // what planet is this
	BYTE NumPlanets;    // number of moons
	SIZE radius;        // radius of planet orbit
	POINT location;     // coords on screen

	Color temp_color;   // color of planet orbit
	COUNT NextIndex;    // index to a next planet
	STAMP image;        // image of a planet in IP view

	PLANET_DESC *pPrevDesc;
			// The Sun or planet that this world is orbiting around.
	
	// BW : new stuff for animated solar systems
	PLANET_ORBIT orbit; // Link to moon(s)
	COUNT size;         // size of a planet
	
	COUNT angle;
	int rotFrameIndex, rotPointIndex, rotwidth, rotheight;
	double orb_speed;
	double rot_speed;
			// Handles rotation and orbiting

	BYTE PlanetByte;
	BYTE MoonByte;
			// Handles hard coded planets/moons for Custom Seed
	BYTE frame_offset;
};

struct star_desc
{
	POINT star_pt;
	BYTE Type;
	BYTE Index;
	BYTE Prefix;
	BYTE Postfix;
};

struct node_info
{
	// This structire is filled in when a generateMinerals, generateEnergy,
	// or generateLife call is made.
	POINT loc_pt;
			// Position of the mineral/bio/energy node on the planet.
	COUNT density;
			// For bio and energy: undefined
			// For minerals the low byte is the gross size of the
			// deposit (this determines the image), and the high
			// byte is the fine size (the actual quantity).
	COUNT type;
			// For minerals: the type of element
			// For bio: the type of the creature.
			//       0 through NUM_CREATURE_TYPES - 1 are normal creatures,
			//       NUM_CREATURE_TYPES     is an Evil One
			//       NUM_CREATURE_TYPES + 1 is a Brainbox Bulldozer
			//       NUM_CREATURE_TYPES + 2 is Zex' Beauty
			// For energy: undefined
};

// See doc/devel/generate for information on how this structure is filled.
struct solarsys_state
{
	// Standard field required by DoInput()
	BOOLEAN (*InputFunc) (struct solarsys_state *);

	BOOLEAN InIpFlight;
			// Set to TRUE when player is flying around in interplanetary
			// Reset to FALSE when going into orbit or encounter

	COUNT WaitIntersect;
			// Planet/moon number with which the flagship should not
			// collide. For example, if the player just left the planet or
			// inner system. If set to (COUNT)~0, all planet collisions are
			// disabled until the flagship stops intersecting with all
			// planets.
	PLANET_DESC SunDesc[MAX_SUNS];
	PLANET_DESC PlanetDesc[MAX_PLANETS];
			// Description of the planets in the system.
			// Only defined after a call to (*genFuncs)->generatePlanets()
			// and overwritten by subsequent calls.
	PLANET_DESC MoonDesc[MAX_GEN_MOONS];
			// Description of the moons orbiting the planet pointed to
			// by pBaseDesc.
			// Only defined after a call to (*genFuncs)->generateMoons()
			// as its argument, and overwritten by subsequent calls.
	PLANET_DESC *pBaseDesc;
			// In outer system: points to PlanetDesc[]
			// In inner system: points to MoonDesc[]
	PLANET_DESC *pOrbitalDesc;
			// In orbit: points into PlanetDesc or MoonDesc to the planet
			// currently orbiting.
			// In inner system: points into PlanetDesc to the planet whose
			// inner system the ship is inside
	SIZE FirstPlanetIndex, LastPlanetIndex;
			// The planets get sorted on their image.origin.y value.
			// PlanetDesc[FirstPlanetIndex] is the planet with the lowest
			// image.origin.y, and PlanetDesc[LastPlanetIndex] has the
			// highest image.origin.y.
			// PlanetDesc[PlanetDesc[i].NextIndex] is the next planet
			// after PlanetDesc[i] in the ordering.

	BYTE turn_counter;
	BYTE turn_wait;
	BYTE thrust_counter;
	BYTE max_ship_speed;

	STRING XlatRef;
	const void *XlatPtr;
	COLORMAP OrbitalCMap;

	SYSTEM_INFO SysInfo;

	const GenerateFunctions *genFuncs;
			// Functions to call to fill in various parts of this
			// structure. See generate.h, doc/devel/generate

	FRAME PlanetSideFrame[3 + MAX_LIFE_VARIATION];
			/* Frames for planet-side elements.
			 * [0] = bio cannister
			 * [1] = energy node (world-specific)
			 * [2] = unused (formerly static slave shield, presumed)
			 * [3] = bio 1 (world-specific)
			 * [4] = bio 2 (world-specific)
			 * [5] = bio 3 (world-specific)
			 */
	FRAME TopoFrame;
	PLANET_ORBIT Orbit;
	BOOLEAN InOrbit;
			// Set to TRUE when player hits a world in an inner system
			// Homeworld encounters count as 'in orbit'
	FRAME ScanFrame[NUM_SCAN_TYPES];
			// For PC scan, generated from TopoFrame
};

extern SOLARSYS_STATE *pSolarSysState;
extern MUSIC_REF SpaceMusic;
extern CONTEXT PlanetContext;
extern BOOLEAN useDosSpheres;
extern BOOLEAN use3DOSpheres;

// Random context used for all solar system, planets, and surfaces
// generation
extern RandomContext *SysGenRNG;
extern RandomContext* SysGenRNGDebug;

bool playerInSolarSystem (void);
bool playerInPlanetOrbit (void);
bool playerInInnerSystem (void);
bool worldIsPlanet (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world);
bool worldIsMoon (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world);
COUNT planetIndex (const SOLARSYS_STATE *solarSys,
		const PLANET_DESC *world);
COUNT moonIndex (const SOLARSYS_STATE *solarSys, const PLANET_DESC *moon);
#define MATCH_PLANET ((BYTE) -1)
bool matchWorld (const SOLARSYS_STATE *solarSys, const PLANET_DESC *world,
		BYTE planetI, BYTE moonI);

DWORD GetRandomSeedForStar (const STAR_DESC *star);

POINT locationToDisplay (POINT pt, SIZE scaleRadius);
POINT displayToLocation (POINT pt, SIZE scaleRadius);
POINT planetOuterLocation (COUNT planetI);

extern void DestroyOrbitStruct (PLANET_ORBIT *Orbit, SIZE height);
extern void LoadPlanet (FRAME SurfDefFrame);
extern void DrawPlanet (int dy, Color tintColor);
extern void DrawPCScanTint (COUNT cur_scan);
extern void FreePlanet (void);
extern void LoadStdLanderFont (PLANET_INFO *info);
extern void FreeLanderFont (PLANET_INFO *info);

extern void ExploreSolarSys (void);
extern void DrawStarBackGround (void);
extern FRAME GetStarBackFround (void);
extern void XFormIPLoc (POINT *pIn, POINT *pOut, BOOLEAN ToDisplay);
extern void DrawOval (RECT *pRect, BYTE num_off_pixels, BOOLEAN scaled);
extern void DrawFilledOval (RECT *pRect);
extern void DrawEllipse (int cx, int cy, int rx, int ry, int shear,
		int filled, int dotted);
extern void DrawRotatedEllipse (int cx, int cy, int rx, int ry,
		int angle_deg, int filled, int dotted);
extern void ComputeSpeed(PLANET_DESC *planet, BOOLEAN GeneratingMoons,
		UWORD rand_val);
extern void FillOrbits (SOLARSYS_STATE *system, BYTE NumPlanets,
		PLANET_DESC *pBaseDesc, BOOLEAN TypesDefined);
extern void InitLander (BYTE LanderFlags);
extern void InitPCLander (BOOLEAN Loading);
extern void DestroyPCLanderContext (void);

extern void InitSphereRotation (int direction, BOOLEAN shielded,
		COUNT width, COUNT height);
extern void UninitSphereRotation (void);
extern void PrepareNextRotationFrame (void);
extern void PrepareNextRotationFrameForIP (PLANET_DESC *pPlanetDesc,
		SIZE frameCounter);
extern void DrawPlanetSphere (int x, int y, bool back);
extern void DrawDefaultPlanetSphere (void);
extern void RerenderPlanetSphere (void);
extern void RenderDOSPlanetSphere (PLANET_ORBIT* Orbit, FRAME MaskFrame,
		int offset);
extern void Render3DOPlanetSphere (PLANET_ORBIT* Orbit, FRAME MaskFrame,
		int offset, COUNT rotwidth, COUNT height);
extern void RenderPlanetSphere (PLANET_ORBIT *Orbit, FRAME Frame,
		int offset, BOOLEAN shielded, BOOLEAN doThrob, COUNT width,
		COUNT height, COUNT radius);
extern void SetShieldThrobEffect (FRAME FromFrame, int offset,
		FRAME ToFrame);
extern void Draw3DOShield (STAMP ShieldFrame);

extern void ZoomInPlanetSphere (void);
extern void RotatePlanetSphere (BOOLEAN keepRate, STAMP *onTop);

extern void DrawScannedObjects (BOOLEAN Reversed);
extern void GeneratePlanetSurface (PLANET_DESC *pPlanetDesc,
		FRAME SurfDefFrame, COUNT width, COUNT height);
extern void DeltaTopography (COUNT num_iterations, SBYTE *DepthArray,
		RECT *pRect, SIZE depth_delta);

extern void TransformColor (Color *c, COUNT scan);

extern void DrawPlanetSurfaceBorder (void);

extern FRAME CreateStarBackGround (BOOLEAN encounter);
extern UNICODE* GetNamedPlanetaryBody (void);
extern void GetPlanetOrMoonName (UNICODE *buf, COUNT bufsize);

extern void PlanetOrbitMenu (void);
extern void SaveSolarSysLocation (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_PLANETS_H_ */
