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
//#define DEBUG_STARSEED
//#define DEBUG_STARSEED_TRACE
//#define DEBUG_STARSEED_TRACE_W
//#define DEBUG_STARSEED_TRACE_X
//#define DEBUG_STARSEED_TRACE_Y
//#define DEBUG_STARSEED_TRACE_Z

#include "starmap.h"
#include "gamestr.h"
#include "globdata.h"
#include "libs/gfxlib.h"
#include "hyper.h"	// JSD: For arilou_home in the portal map
#include <stdlib.h>	// bsearch needs this or it cores!
#include <time.h>	// For the clock.

// The "starmap_array" variable (from plandata) is only used to intialize
// the "star_array" global variable.  This is now intentionally a copy
// of the data so that it may be manipulated or placed back to default from
// there.  "starmap_array" should never be altered and referenced as little
// as possible.
//STAR_DESC *star_array;
STAR_DESC star_array[NUM_SOLAR_SYSTEMS + NUM_HYPER_VORTICES + 3] =
		{[0 ... (NUM_SOLAR_SYSTEMS + NUM_HYPER_VORTICES + 2)] =
		{{~0, ~0}, 0, 0, 0, 0}};
STAR_DESC *CurStarDescPtr = 0;
POINT *constel_array;
// JSD Give my own starseed
RandomContext *StarGenRNG;
PORTAL_LOCATION portal_map[NUM_HYPER_VORTICES+1] =
		{[0 ... (NUM_HYPER_VORTICES)] = {{0, 0}, {0, 0}, NULL}};


STAR_DESC*
FindStar (STAR_DESC *LastSDPtr, POINT *puniverse, SIZE xbounds,
		SIZE ybounds)
{
	COORD min_y, max_y;
	SIZE lo, hi;
	STAR_DESC *BaseSDPtr;

	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		BaseSDPtr = star_array;
		hi = NUM_SOLAR_SYSTEMS - 1;
	}
	else
	{
//#define NUM_HYPER_VORTICES 15 //JSD moved to .h
		BaseSDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		hi = (NUM_HYPER_VORTICES + 1) - 1;
	}

	if (LastSDPtr == NULL)
		lo = 0;
	else if ((lo = LastSDPtr - BaseSDPtr + 1) > hi)
		return (0);
	else
		hi = lo;

	if (ybounds <= 0)
		min_y = max_y = puniverse->y;
	else
	{
		min_y = puniverse->y - ybounds;
		max_y = puniverse->y + ybounds;
	}

	while (lo < hi)
	{
		SIZE mid;

		mid = (lo + hi) >> 1;
		if (BaseSDPtr[mid].star_pt.y >= min_y)
			hi = mid - 1;
		else
			lo = mid + 1;
	}

	LastSDPtr = &BaseSDPtr[lo];
	if (ybounds < 0 || LastSDPtr->star_pt.y <= max_y)
	{
		COORD min_x, max_x;

		if (xbounds <= 0)
			min_x = max_x = puniverse->x;
		else
		{
			min_x = puniverse->x - xbounds;
			max_x = puniverse->x + xbounds;
		}

		do
		{
			if ((ybounds < 0 || LastSDPtr->star_pt.y >= min_y)
					&& (xbounds < 0
					|| (LastSDPtr->star_pt.x >= min_x
					&& LastSDPtr->star_pt.x <= max_x))
					)
				return (LastSDPtr);
		} while ((++LastSDPtr)->star_pt.y <= max_y);
	}

	return (0);
}

void
GetClusterName (const STAR_DESC *pSD, UNICODE buf[])
{
	UNICODE *pBuf, *pStr;

	pBuf = buf;
	if (pSD->Prefix)
	{
		pStr = GAME_STRING (STAR_NUMBER_BASE + pSD->Prefix - 1);
		if (pStr)
		{
			while ((*pBuf++ = *pStr++))
				;
			pBuf[-1] = ' ';
		}
	}
	if ((pStr = GAME_STRING (pSD->Postfix)) == 0)
		*pBuf = '\0';
	else
	{
		while ((*pBuf++ = *pStr++))
			;
	}
}

// Finds the nearest (constellation = TRUE / FALSE = star) to the
// point P provided on the starmap, returns pointer to that star.
STAR_DESC*
FindNearest (STAR_DESC *starmap, POINT p, BOOLEAN constellation)
{
	if (!starmap)
		return NULL;
	COUNT index, star_id = 0;
	DWORD dist, min_dist = MAX_X_UNIVERSE * MAX_Y_UNIVERSE;
	for (index = 0; index < NUM_SOLAR_SYSTEMS; index++)
	{
		dist = (starmap[index].star_pt.x - p.x) *
				(starmap[index].star_pt.x - p.x) +
				(starmap[index].star_pt.y - p.y) *
				(starmap[index].star_pt.y - p.y);
		if (dist < min_dist && (starmap[index].Prefix > 0 || !constellation))
		{
			min_dist = dist;
			star_id = index;
		}
	}
	return (&(starmap[star_id]));
}

// Returns a pointer to the closest star to point p on the starmap
STAR_DESC*
FindNearestStar (STAR_DESC *starmap, POINT p)
{
	return (starmap ? FindNearest (starmap, p, FALSE) : NULL);
}

// Returns a pointer to the closest constellation to point p on the starmap
STAR_DESC*
FindNearestConstellation (STAR_DESC *starmap, POINT p)
{
	return (starmap ? FindNearest (starmap, p, TRUE) : NULL);
}

// plot_map is a global item (like star_map) that keeps track of the location
// on the star map of each plot item's location, like SOL or the SAMATRA.
// It is populated first (default plot lengths from InitPlot, or write your own)
// and can be added to with further AddPlot calls.  Then it is used during
// SeedPlot which sets the starmap locations.  After this point it is kept
// globally to help with the creation/location fleets, dialog generation, and
// any time the game needs to reference these "set" locations.
//
// Plot location is a structure that contains the coordinates assigned to each
// plot (once chosen), as well as a pointer to the located star in the star map.
// Uses dist_sq to store the min and max distance squared to other plots.
// Keeps a running tally of the "plot weight" of each plot when the min/max
// distances are set.
// The COORDS are initialized with {~0, ~0}, the rest 0.
// Plot ID 0 (ARILOU_DEFINED) is ARILOU_HYPERSPACE_PORTAL location.
PLOT_LOCATION plot_map[NUM_PLOTS] = {[0 ... (NUM_PLOTS - 1)] =
		{{~0, ~0}, NULL, {[0 ... (NUM_PLOTS - 1)] = 0}}};

#define VORTEX_SCALE 20		// From the starmap; scales vortex distances
#define MIN_PORTAL 2000		// The min distance between portals (exits)
#define MIN_VORTEX 150		// The min distance between vorticies in QS
#define MIN_PORTAL_SOL 500	// The min distance between SOL and portal 0
#define MAX_PORTAL_SOL 1000	// The max distance between SOL and portal 0

// portal_map and portalmap_array work exactly like star_map and starmap_array
// but for Quasispace.  We use this static one here for reference to initialize
// the global portal_map to the default Prime Seed values.
// These are called Hyperspace Vortices (Vortex) when in Quasispace,
// and Quasispace Portals when in Hyperspace (all of these are exit only)
// The recurring 2-way Portal/Vortex is ARILOU_DEFINED plot loc and 5000x5000
PORTAL_LOCATION portalmap_array[NUM_HYPER_VORTICES + 1] =
{
	{{4091, 7748},
			{(-12* VORTEX_SCALE) + 5000, (-21 * VORTEX_SCALE) + 5000}, NULL},
	{{3184, 4906},
			{(1* VORTEX_SCALE) + 5000, (-20 * VORTEX_SCALE) + 5000}, NULL},
	{{9211, 6104},
			{(-16* VORTEX_SCALE) + 5000, (-18 * VORTEX_SCALE) + 5000}, NULL},
	{{5673, 1207},
			{(8* VORTEX_SCALE) + 5000, (-17 * VORTEX_SCALE) + 5000}, NULL},
	{{1910,  926},
			{(3* VORTEX_SCALE) + 5000, (-13 * VORTEX_SCALE) + 5000}, NULL},
	{{8607,  151},
			{(-21* VORTEX_SCALE) + 5000, (-4 * VORTEX_SCALE) + 5000}, NULL},
	{{  50, 1647},
			{(-4* VORTEX_SCALE) + 5000, (-4 * VORTEX_SCALE) + 5000}, NULL},
	{{6117, 4131},
			{(-12* VORTEX_SCALE) + 5000, (-2 * VORTEX_SCALE) + 5000}, NULL},
	{{5658, 9712},
			{(-26* VORTEX_SCALE) + 5000, (2 * VORTEX_SCALE) + 5000}, NULL},
	{{2302, 3988},
			{(-17* VORTEX_SCALE) + 5000, (7 * VORTEX_SCALE) + 5000}, NULL},
	{{ 112, 9409},
			{(10* VORTEX_SCALE) + 5000, (7 * VORTEX_SCALE) + 5000}, NULL},
	{{7752, 8906},
			{(15* VORTEX_SCALE) + 5000, (14 * VORTEX_SCALE) + 5000}, NULL},
	{{ 368, 6332},
			{(22* VORTEX_SCALE) + 5000, (16 * VORTEX_SCALE) + 5000}, NULL},
	{{9735, 3153},
			{(-6* VORTEX_SCALE) + 5000, (19 * VORTEX_SCALE) + 5000}, NULL},
	{{5850, 6213},
			{(10* VORTEX_SCALE) + 5000, (20 * VORTEX_SCALE) + 5000}, NULL},
	{{   0,    0},
			{ARILOU_HOME_X, ARILOU_HOME_Y}, NULL}
};

// Reset the given starmap to the default statics starmap array
void
DefaultStarmap (STAR_DESC *starmap)
{
	if (!starmap)
	{
		fprintf (stderr, "DefaaultStarmap called with NULL starmap PTR.\n");
		return;
	}
#ifdef DEBUG_STARSEED
	fprintf (stderr, "DefaultStarmap setting star map to original values.\n");
#endif
	COUNT i;
	extern const STAR_DESC starmap_array[];
	for (i = 0; i < NUM_SOLAR_SYSTEMS + 1 + NUM_HYPER_VORTICES + 1 + 1; i++)
			starmap[i] = starmap_array[i];
}

// Seed the type of each star, randomly selecting a color and
// either dwarf or giant star (super giant is handled by plot).
// Default: 9 Super-giant, 24 giant, 469 dwarf 
// 53 white   85 blue   102 green   55 yellow 102 orange 105 red
// 47 w dwarf 80 b dwarf 96 g dwarf 50 y dwarf 97 o dwarf 99 r dwarf
//  5 w giant  4 b giant  4 g giant  2 y giant  5 o giant  4 r giant
//  1 w super  1 b super  2 g super  3 y super  0 o super  2 r super
void
SeedStarmap (STAR_DESC *starmap)
{
	if (!starmap)
	{
		fprintf (stderr, "SeedStarmap called with NULL starmap PTR.\n");
		return;
	}
	COUNT i;
	UWORD rand_val;
	if (!StarGenRNG)
	{
		fprintf(stderr, "****SeedStarmap creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New ();
	}
	RandomContext_SeedRandom (StarGenRNG, optCustomSeed);

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		rand_val = RandomContext_Random (StarGenRNG);
		// 469 is the default number of dwarf stars, and 24 the number of
		// giant. Super giant is handled elsewhere.
		starmap[i].Type = MAKE_STAR
				(LOBYTE (rand_val) % (469 + 24) >= 24 ? DWARF_STAR : GIANT_STAR,
				HIBYTE (rand_val) % NUM_STAR_COLORS,
				-1);
		starmap[i].Index = 0;
	}
}

// Functions which know how a plotmap works call it "plot" so they can access
// these internal define functions.
#define PLOT_SET(id) (plot[id].star && plot[id].star_pt.x != ~0 && \
		plot[id].star_pt.y != ~0)
#define PLOT_MAX(pi,pj) ((pi > pj) ? \
		plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi])
#define PLOT_MIN(pi,pj) ((pi < pj) ? \
		plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi])
#define PLOT_MAX_SET(pi,pj,value) \
		if (pi > pj) plot[pi].dist_sq[pj] = value; \
		else plot[pj].dist_sq[pi] = value;
#define PLOT_MIN_SET(pi,pj,value) \
		if (pi < pj) plot[pi].dist_sq[pj] = value; \
		else plot[pj].dist_sq[pi] = value;
#define PLOT_WEIGHT(id) (plot[id].dist_sq[id])

// This is purely for debugging purposes - print out the plot name by ID
void
print_plot_id (COUNT plot_id)
{
	static const char * const plot_name[] = {
			"ARILOU_DEFINED", "SOL_DEFINED", "SHOFIXTI_DEFINED",
			"MAIDENS_DEFINED", "START_COLONY_DEFINED", "SPATHI_DEFINED",
			"ZOQFOT_DEFINED", "MELNORME0_DEFINED", "MELNORME1_DEFINED", 
			"MELNORME2_DEFINED", "MELNORME3_DEFINED", "MELNORME4_DEFINED",
			"MELNORME5_DEFINED", "MELNORME6_DEFINED", "MELNORME7_DEFINED", 
			"MELNORME8_DEFINED", "TALKING_PET_DEFINED", "CHMMR_DEFINED",
			"SYREEN_DEFINED", "BURVIXESE_DEFINED", "SLYLANDRO_DEFINED",
			"DRUUGE_DEFINED", "BOMB_DEFINED", "AQUA_HELIX_DEFINED",
			"SUN_DEVICE_DEFINED", "TAALO_PROTECTOR_DEFINED",
			"SHIP_VAULT_DEFINED", "URQUAN_WRECK_DEFINED", "VUX_BEAST_DEFINED",
			"SAMATRA_DEFINED", "ZOQ_SCOUT_DEFINED", "MYCON_DEFINED",
			"EGG_CASE0_DEFINED", "EGG_CASE1_DEFINED", "EGG_CASE2_DEFINED",
			"PKUNK_DEFINED", "UTWIG_DEFINED", "SUPOX_DEFINED", "YEHAT_DEFINED",
			"VUX_DEFINED", "ORZ_DEFINED", "THRADD_DEFINED", "RAINBOW0_DEFINED",
			"RAINBOW1_DEFINED", "RAINBOW2_DEFINED", "RAINBOW3_DEFINED",
			"RAINBOW4_DEFINED", "RAINBOW5_DEFINED", "RAINBOW6_DEFINED",
			"RAINBOW7_DEFINED", "RAINBOW8_DEFINED", "RAINBOW9_DEFINED",
			"ILWRATH_DEFINED", "ANDROSYNTH_DEFINED", "MYCON_TRAP_DEFINED",
			"URQUAN0_DEFINED", "URQUAN1_DEFINED", "URQUAN2_DEFINED",
			"KOHRAH0_DEFINED", "KOHRAH1_DEFINED", "KOHRAH2_DEFINED",
			"DESTROYED_STARBASE_DEFINED", "MOTHER_ARK_DEFINED",
			"ZOQ_COLONY0_DEFINED", "ZOQ_COLONY1_DEFINED",
			"ZOQ_COLONY2_DEFINED", "ZOQ_COLONY3_DEFINED", "ALGOLITES_DEFINED",
			"SPATHI_MONUMENT_DEFINED", "EXCAVATION_SITE_DEFINED"};
	fprintf (stderr, "%s (%d)", plot_name[plot_id], plot_id);
}

// The minimum distance you can set a max plot length to; needs to be
// non-zero, LARGER values have more weight.  The maximum distance you
// can set a min plot length to; too large and it will push off the map.
#define MIN_PLOT 100
#define MAX_PLOT 10000
// We effectively won't care about the weight of a max plot length above this
// value either, it caps
#define MAX_PWEIGHT 100000000 // (10000 * 10000) or MAX_PLOT * MAX_PLOT

// Internal (although it could be shared)
// Sets the min and max distances (squared internally)
// of two plot IDs on given plotmap.  Also checks to see if already set,
// and remove the plot lengths from existing plotweights before changing.
// Also increments the plot weight totals of the plots.
// Min and max can't be the same value, must be at least 100 apart.
void
SetPlotLength (PLOT_LOCATION *plot, COUNT plotA, COUNT plotB,
		COUNT p_min, COUNT p_max)
{
	COUNT min = p_min;
	COUNT max = p_max;
	// Reject bad data
	if (!plot || plotA >= NUM_PLOTS || plotB >= NUM_PLOTS ||
			p_min > p_max - 100)
	{
		fprintf (stderr, "%s called with bad data (PTR %d %d %d %d)\n",
				"SetPlotLength (plotmap, plot, plot, min, max)",
				plotA, plotB, p_min, p_max);
		return;
	}
	// If you zero out max, it will treat it as MAX_PLOT (any length)
	// If your plot max/min is too small/large, push it up/down to the cap
	if ((max > MAX_PLOT) || (max == 0)) max = MAX_PLOT;
	if (max < MIN_PLOT) max = MIN_PLOT;
	if (min > MAX_PLOT) min = MAX_PLOT;

	// If the plots already have a min or max, remove it from the plot_weight
	if (PLOT_MIN (plotA, plotB) > 0)
	{
#ifdef DEBUG_STARSEED_TRACE
		fprintf (stderr, "SetPlotLength reducing plot min ");
		print_plot_id (plotA);
		fprintf (stderr, " (weight %d) to ", PLOT_WEIGHT (plotA));
		print_plot_id (plotB);
		fprintf (stderr, " (weight %d), min %d\n", PLOT_WEIGHT (plotB),
				(UWORD) sqrt (PLOT_MIN (plotA, plotB)));
#endif
		PLOT_WEIGHT (plotA) -= (UWORD) sqrt (PLOT_MIN (plotA, plotB));
		PLOT_WEIGHT (plotB) -= (UWORD) sqrt (PLOT_MIN (plotA, plotB));
	}
	if (PLOT_MAX (plotA, plotB) != 0 &&
			PLOT_MAX (plotA, plotB) < MAX_PLOT * MAX_PLOT)
	{
#ifdef DEBUG_STARSEED_TRACE
		fprintf (stderr, "SetPlotLength reducing plot max ");
		print_plot_id (plotA);
		fprintf (stderr, " (weight %d) to ", PLOT_WEIGHT (plotA));
		print_plot_id (plotB);
		fprintf (stderr, " (weight %d), max %d\n", PLOT_WEIGHT (plotB),
			MAX_PLOT - (UWORD) sqrt (PLOT_MAX (plotA, plotB)));
#endif
		PLOT_WEIGHT (plotA) += (UWORD) sqrt
				(PLOT_MAX (plotA, plotB)) - MAX_PLOT;
		PLOT_WEIGHT (plotB) += (UWORD) sqrt
				(PLOT_MAX (plotA, plotB)) - MAX_PLOT;
	}
	
	// Now set the plot lengths
	PLOT_MIN_SET (plotA, plotB, min * min);
	PLOT_MAX_SET (plotA, plotB, max * max);

	// Now we increase the weights
	PLOT_WEIGHT (plotA) += min + MAX_PLOT - max;
	PLOT_WEIGHT (plotB) += min + MAX_PLOT - max;
}

// Internal. (although it could be shared)
// If the min/max plot lengths are set and valid, gives relative weight of
// this plot pair, otherwise 0.  The relative weight is the min distance
// plus the (MAX_PLOT - the max distance) of each plot thread connected.
DWORD
ConnectedPlot (PLOT_LOCATION *plot, COUNT plotA, COUNT plotB)
{
	// report bad data
	if (!(plot && (plotA < NUM_PLOTS) && (plotB < NUM_PLOTS)))
	{
		fprintf (stderr, "%s called with bad data PTR %d %d\n",
				"ConnectedPlot (plotmap, plot_id, plot_id)",
				plotA, plotB);
		return 0;
	}
	// Plot MIN (capped at max pweight) + MAX weight - PLOT MAX (if valid)
	return (PLOT_MIN (plotA, plotB) > MAX_PWEIGHT ?
				MAX_PWEIGHT : PLOT_MIN (plotA, plotB)) +
			((PLOT_MAX (plotA, plotB) == 0 ||
			PLOT_MAX (plotA, plotB) > MAX_PWEIGHT) ?
				0 : MAX_PWEIGHT - PLOT_MAX (plotA, plotB));
}

// Used by GetNextPlot to re-iterate on a failed plot after pop-back.
// Needs SeedPlot to reset it as well (when it starts the clock).
static COUNT next_plot = ~0;

// Internal.
// This returns the plot_id (number) with the highest weight on the
// plot map which has yet to be assigned a home, or NUM_PLOTS if all
// plots are allocated For a plot to be set, both COORD and STAR_DESC
// ptr need data.  If both are set, we'll just stamp the starmap.
COUNT
GetNextPlot (PLOT_LOCATION *plot)
{
	COUNT plot_id = 0;
	DWORD top_weight = 0;
	COUNT i;
	//DWORD connect;
	if (!plot)
	{
		fprintf (stderr, "GetNextPlot (plotmap) called with bad data PTR\n");
		return 0;
	}
	// If we're already working on one, continue to try that one.
	if (next_plot < NUM_PLOTS && !PLOT_SET(next_plot))
		return next_plot;
	
#if 0
	// Leaving this in for now as a stern reminder:
	// This makes things worse, even if you turn off the retry-last-plot above.
	// [Otherwise try and find one that's connected to the last one.]
	for (i = 0; i < NUM_PLOTS && next_plot < NUM_PLOTS; i++)
	{
		if (ConnectedPlot (plot, next_plot, i) > top_weight && !PLOT_SET(i))
		{
			plot_id = i;
			top_weight = ConnectedPlot (plot, next_plot, i);
		}
	}
#endif

	// Otherwise find the heaviest plot available.
	if (top_weight == 0)
		for (i = 0; i < NUM_PLOTS; i++)
			if ((PLOT_WEIGHT (i) > top_weight) && !(PLOT_SET(i)))
			{
				plot_id = i;
				top_weight = PLOT_WEIGHT (i);
			}

	// After loop if top weight is 0 (plot_id will also be 0) there may be
	// unassigned plots without weight, find the first one:
	if (top_weight == 0)
		while ((plot_id < NUM_PLOTS) && (PLOT_SET(plot_id)))
			plot_id++;

	// Now plot_id contains either the first unassigned plot with highest
	// weight, OR NUM_PLOTS
#ifdef DEBUG_STARSEED_TRACE_W
	fprintf (stderr, "Top Weight %d Plot ID %d", top_weight, plot_id);
#endif
	return next_plot = plot_id;
	//return plot_id;
}

// Internal but could be shared
// Sets all plot lengths and weights, locations and pointers to zero.
void
ResetPlot (PLOT_LOCATION *plot)
{
	if (!plot)
	{
		fprintf (stderr, "ResetPlot (plotmap) called with bad data PTR.\n");
		return;
	}
	COUNT i, j;
	for (i = 0; i < NUM_PLOTS; i++)
	{
		for (j = 0; j < NUM_PLOTS; j++)
			plot[i].dist_sq[j] = 0;
		plot[i].star_pt = (POINT) {~0, ~0};
		plot[i].star = NULL;
	}
}

// Sets all plot locations to their position on the provided starmap.
void
DefaultPlot (PLOT_LOCATION *plot, STAR_DESC *starmap)
{
#define ARILOU_SPACE_X  438 // We don't seed arilou from the map, they
#define ARILOU_SPACE_Y 6372 // are hard code all the way.
	if (!plot || !starmap)
	{
		fprintf (stderr, "DefaultPlot (plotmap, starmap) called %s\n",
				"with bad data PTR.");
		return;
	}
	COUNT i;
	plot[0].star_pt = (POINT) {ARILOU_SPACE_X, ARILOU_SPACE_Y};
	plot[0].star = &(starmap[NUM_SOLAR_SYSTEMS + 1 + NUM_HYPER_VORTICES +1]);
	for (i = 1; i < NUM_SOLAR_SYSTEMS; i++)
		if (starmap[i].Index > 0)
		{
			if (starmap[i].Index >= NUM_PLOTS)
					continue;
			plot[starmap[i].Index].star_pt = starmap[i].star_pt;
			plot[starmap[i].Index].star = &(starmap[i]);
		}
}

// InitPlot will set the plotmap up with weighted connections to help
// the seeding engine determine correctness.
// Extended lore is taken into account regardless so that changing modes
// doesn't alter the seed.
void
InitPlot (PLOT_LOCATION *plotmap)
{
	if (!plotmap)
	{
		fprintf (stderr, "InitPlot (plotmap) called with bad data PTR\n");
		return;
	}
	COUNT i, j;
	ResetPlot (plotmap);

	// Set up buffers around anyone with a zone of influence during the war
	// or after the war where homeworlds inside the zone wouldn't make sense
	// Orz are skipped, Androsynth takes care of them.
	COUNT home_map[] =
			{ARILOU_DEFINED, 300,
			SOL_DEFINED, 700,
			YEHAT_DEFINED, 700,
			SHOFIXTI_DEFINED, 700,
			CHMMR_DEFINED, 700,
			SYREEN_DEFINED, 700,
			EGG_CASE0_DEFINED, 700,	
			MOTHER_ARK_DEFINED, 700,

			DRUUGE_DEFINED, 1300,
			PKUNK_DEFINED, 700,
			UTWIG_DEFINED, 700,
			SUPOX_DEFINED, 300,
			ZOQFOT_DEFINED, 700,
			BURVIXESE_DEFINED, 300,
			START_COLONY_DEFINED, 300,

			SAMATRA_DEFINED, 3000,
			ANDROSYNTH_DEFINED, 1000,
			ILWRATH_DEFINED, 1000,
			MYCON_DEFINED, 1000,
			SPATHI_DEFINED, 1000,
			THRADD_DEFINED, 1000,
			VUX_DEFINED, 1000,
			TALKING_PET_DEFINED, 1000};
	// Total 8 + 7 + 8 currently 23 homeworlds
	// Set the homeworlds apart first (can be overwritten later)
	// Because you can't declare 2D array dynamically, must fake it
	for (i = 0; i < 22 * 2; i += 2)
		for (j = i + 2; j < 23 * 2; j += 2)
			SetPlotLength (plotmap, home_map[i], home_map[j],
					(home_map[i + 1] > home_map[j + 1]) ?
					home_map[i + 1] : home_map[j + 1], MAX_PLOT);

	// Sets up Melnormes and Rainbow Worlds with minimal plot weight
	// also their connections to other races.
	InitMelnormeRainbow (plotmap);

	// The numbers next to a race are their strength.  The numbers at the start
	// of a comment are the standard distance in Prime seed between the plots.

	// Arilou 250 and Earthling, Slylandro
	// 6240 Keep humans far from the conflict at start
	// 1660 Vela is not too close to Sol, but limited by fuel
	// 5030 Arilou are not too close or far, so they can "hide" the humans
	// 5090 Arilou and Orz are not in the same *space*
	// 8480 The Slylandro are far away and annoying to find
	SetPlotLength (plotmap, SOL_DEFINED, SAMATRA_DEFINED, 5000, MAX_PLOT);
	SetPlotLength (plotmap, SOL_DEFINED, START_COLONY_DEFINED, 1500, 5000);
	SetPlotLength (plotmap, ARILOU_DEFINED, ORZ_DEFINED, 4000, MAX_PLOT);
	SetPlotLength (plotmap, ARILOU_DEFINED, SOL_DEFINED, 3000, 7000);
	SetPlotLength (plotmap, SLYLANDRO_DEFINED, SOL_DEFINED, 6000, MAX_PLOT);

	// Druuge 1400 and Melnorme
	// 2990 Burvix are outside Druuge space but close enough to lure
	// 650 Alas, the poor algolites.  Keep them in Druuge space for _reasons_
	// 6600 They're not very close to Spathi at all for some reason.
	SetPlotLength (plotmap, DRUUGE_DEFINED, BURVIXESE_DEFINED, 2000, 4000);
	SetPlotLength (plotmap, DRUUGE_DEFINED, ALGOLITES_DEFINED, 0, 1250);
	SetPlotLength (plotmap, SPATHI_DEFINED, ALGOLITES_DEFINED, 5000, MAX_PLOT);

	// Ilwrath 1410 and Chmrr, Chenjesu
	// 1490 The Chenjesu homeworld is between Ilwrath homeworld (guarding)
	// 1760 [Chenjesu] and the Pkunk howeworld
	// 3150 [Pkunk] Which the Ilwrath are attacking
	SetPlotLength (plotmap, CHMMR_DEFINED, ILWRATH_DEFINED, 1000, 3000);
	SetPlotLength (plotmap, CHMMR_DEFINED, PKUNK_DEFINED, 700, 3500);
	SetPlotLength (plotmap, ILWRATH_DEFINED, PKUNK_DEFINED, 3000, 6000);

	// Mycon 1070
	// 150 The Mycon start with the Sun Device very close
	// 510, 240, 520 The Egg Cases (Syra is Egg #0) are all very close as well
	// 1739 The trap should be outside Mycon SoI but not far
	SetPlotLength (plotmap, MYCON_DEFINED, SUN_DEVICE_DEFINED, 0, 500);
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE0_DEFINED, 300, 1250);
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE1_DEFINED, 0, 500);
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE2_DEFINED, 0, 800);
	SetPlotLength (plotmap, MYCON_DEFINED, MYCON_TRAP_DEFINED, 1200, 3500);

	// Orz 333 and Androsynth
	// Give Orz the standard UQ/KA push (they don't have other HW pushes)
	// 130 The Orz are very close / around to the Androsynth home
	// 80 The Orz are also around the Playground
	// 120 The Androsynth were near the Dig Site
	// 2150 The Androysnth fled Sol (but not too far because of colony)
	// 680 The Start Colony is near Androsynth space
	SetPlotLength (plotmap, ORZ_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED, ORZ_DEFINED, 0, 350);
	SetPlotLength (plotmap, ORZ_DEFINED, TAALO_PROTECTOR_DEFINED, 0, 350);
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED,
			EXCAVATION_SITE_DEFINED, 0, 350);
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED, SOL_DEFINED, 2000, 6000);
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED,
			START_COLONY_DEFINED, 350, 1000);

	// Pkunk 666
	// 4410 The Pkunk went a pretty good distance from home
	SetPlotLength (plotmap, PKUNK_DEFINED, YEHAT_DEFINED, 3000, 6000);

	// Spathi 1000 and Mmrnmhrm (eventually)
	// 710 The Spathi were close to the contested world
	// 660 The Mmrnmhrm were close to the contested world
	// 1360 The Spathi and Mmrnmhrm were this far apart
	SetPlotLength
			(plotmap, SPATHI_DEFINED, SPATHI_MONUMENT_DEFINED, 300, 1000);
	SetPlotLength
			(plotmap, MOTHER_ARK_DEFINED, SPATHI_MONUMENT_DEFINED, 300, 1000);
	SetPlotLength (plotmap, MOTHER_ARK_DEFINED, SPATHI_DEFINED, 1000, 2000);

	// Supox 333
	// 1290 This is the Supox Utwig Super Friends Zone
	SetPlotLength (plotmap, SUPOX_DEFINED, UTWIG_DEFINED, 300, 3000);

	// Thraddash 833
	// 400 The Thraddash keep the Aqua Helix sacred
	// 6350 Thraddash are far from Chmmr for Ilwrath distraction
	// 5230 Have them start pretty far from Ilwrath space
	// 8090 And also have them start far from the Pkunk space
	SetPlotLength (plotmap, THRADD_DEFINED, AQUA_HELIX_DEFINED, 0, 700);
	SetPlotLength (plotmap, CHMMR_DEFINED, THRADD_DEFINED, 3000, MAX_PLOT);
	SetPlotLength (plotmap, ILWRATH_DEFINED, THRADD_DEFINED, 3000, MAX_PLOT);
	SetPlotLength (plotmap, PKUNK_DEFINED, THRADD_DEFINED, 3000, MAX_PLOT);

	// Umgah 833 (defined by their suffering: TALKING_PET_DEFINED)
	// Alas, no plot threads for them, just standard homeworld stuff

	// Ur Quan and Kohr Ah both 2666, and Syreen,
	// KA/UQ and use the SAMATRA_DEFINED as homeworld
	// The Ship Vault is in the battle zone
	// The UQ Wreck is outside the battle zone
	// 1820 The Ship Vault is specifically close to the Syreen
	SetPlotLength (plotmap, SAMATRA_DEFINED, SHIP_VAULT_DEFINED, 0, 3000);
	SetPlotLength
			(plotmap, SAMATRA_DEFINED, URQUAN_WRECK_DEFINED, 4000, MAX_PLOT);
	SetPlotLength (plotmap, SYREEN_DEFINED, SHIP_VAULT_DEFINED, 1700, 2100);
	// The UQ/KA bases are all in or near the battle zone
	SetPlotLength (plotmap, SAMATRA_DEFINED, URQUAN0_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, URQUAN1_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, URQUAN2_DEFINED, 0, 3000);
	SetPlotLength (plotmap, SAMATRA_DEFINED, KOHRAH0_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, KOHRAH1_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, KOHRAH2_DEFINED, 0, 3000);
	// This is an abandoned slave-shielded planet
	// (also used as random planet name by ZEX)
	SetPlotLength (plotmap,
			SAMATRA_DEFINED, DESTROYED_STARBASE_DEFINED, 3000, MAX_PLOT);

	// Utwig 666
	// 690 The Utwig keep their Bomb handy
	// 3070 The Burvixese were close enough to the Utwig to be targeted
	SetPlotLength (plotmap, UTWIG_DEFINED, BOMB_DEFINED, 0, 750);
	SetPlotLength (plotmap, UTWIG_DEFINED, BURVIXESE_DEFINED, 1000, 4000);

	// VUX 900
	// 320 Admiral ZEX (the maidens) is within VUX territory
	// 7950 the VUX beast is far away from ZEX
	SetPlotLength (plotmap, VUX_DEFINED, MAIDENS_DEFINED, 0, 1000);
	SetPlotLength
			(plotmap, VUX_BEAST_DEFINED, MAIDENS_DEFINED, 5000, MAX_PLOT);

	// Yehat 750 and Shofixti
	// 2020 The Shofixti and Yehat - brothers for life
	SetPlotLength (plotmap, SHOFIXTI_DEFINED, YEHAT_DEFINED, 700, 3000);

	// Zoq Fot 320
	// The ZFP start inside the conflict zone
	// 3850 Loosely bind the ZFP scout - ZFP homeworld
	// 720 and ZFP scout - SOL ; keep in mind Sol - Samatra > 5000
	// 200, 490, 580, 300 ZFP colonies are inside the war zone and
	// relatively near ZFP but not necessarily inside their zone
	SetPlotLength (plotmap, ZOQFOT_DEFINED, SAMATRA_DEFINED, 1500, 2500);
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_SCOUT_DEFINED, 2000, 5000);
	SetPlotLength (plotmap, ZOQ_SCOUT_DEFINED, SOL_DEFINED, 0, 1500);
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY0_DEFINED, 0, 1250);
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY1_DEFINED, 0, 1000);
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY2_DEFINED, 0, 1000);
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY3_DEFINED, 0, 750);
	SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY0_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY1_DEFINED, 0, 2500);
	SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY2_DEFINED, 0, 3000);
	SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY3_DEFINED, 0, 3000);
}

// Sets up the plot threads for rainbow worlds and Melnormes, including
// the Melnorme 1 near Sol, and Melnorme 7 near Thraddash
void
InitMelnormeRainbow (PLOT_LOCATION *plotmap)
{
	COUNT i, j;
	// Melnormes and Rainbow worlds all push each other pretty far away
	// to keep them scattered nicely. We then zero out their weights so
	// that they are picked last.  In practice 2500 (rainbow) and
	// 2800 (melnorme) are about as high as you can go before the
	// seeding algorithm slows way down.
	for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++)
	{
		plotmap[i].star = NULL;
		plotmap[i].star_pt = (POINT) {~0, ~0};
		for (j = RAINBOW0_DEFINED; j <= RAINBOW9_DEFINED; j++)
			if (i != j)
				SetPlotLength (plotmap, i, j, 2500, MAX_PLOT);
	}
	for (i = MELNORME0_DEFINED; i <= MELNORME8_DEFINED; i++)
	{
		plotmap[i].star = NULL;
		plotmap[i].star_pt = (POINT) {~0, ~0};
		for (j = MELNORME0_DEFINED; j <= MELNORME8_DEFINED; j++)
			if (i != j)
				SetPlotLength (plotmap, i, j, 2750, MAX_PLOT);
	}

	// Zero out their weights - just seeding them mostly last removes a
	// lot of the issues.
	for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++)
		plotmap[i].dist_sq[i] *= 0;
	for (i = MELNORME0_DEFINED; i <= MELNORME8_DEFINED; i++)
		plotmap[i].dist_sq[i] *= 0;

	// 670 Tanaka mentions a rainbow world, in Yehat space
	// 580 Thraddash mention a rainbow world,
	// 2350 Slylandro mention a rainbow world "not far"
	// 520 Supox mention a rainbow world "gross star color"
	// 500 By some strange coincidence, Melnormes have outpost #1 near SOL.
	// 160 Thraddash also have the stele on Melnorme #7
	SetPlotLength (plotmap, YEHAT_DEFINED, RAINBOW0_DEFINED, 0, 1000);
	SetPlotLength (plotmap, THRADD_DEFINED, RAINBOW5_DEFINED, 0, 1000);
	SetPlotLength (plotmap, SLYLANDRO_DEFINED, RAINBOW4_DEFINED, 1500, 3500);
	SetPlotLength (plotmap, SUPOX_DEFINED, RAINBOW7_DEFINED, 0, 1000);
	SetPlotLength (plotmap, SOL_DEFINED, MELNORME1_DEFINED, 0, 1000);
	SetPlotLength (plotmap, THRADD_DEFINED, MELNORME7_DEFINED, 0, 1000);
}

// Internal
// Returns TRUE if all placed plots from plot_id's perspective ONLY are valid
// Don't need to worry about if stars are correct, as they may temporarily
// go past validation check but eventually if the star is in a bad spot,
// it will be replaced.  Although it may also impose irrelevant restrictions
// in the interim.  Do need to worry about undefined plot {~0,~0}.
BOOLEAN
CheckValid (PLOT_LOCATION *plot, COUNT plot_id)
{
	COUNT i;
	DWORD distance_sq;
	if (!plot || plot_id >= NUM_PLOTS)
	{
		fprintf (stderr, "CheckValid (plotmap, plot_id) called %d.\n"
				"with bad data or NULL: PTR", plot_id);
		return FALSE;
	}
	if (!PLOT_SET(plot_id))
	{
		fprintf(stderr, "CheckValid (plotmap, plot_id) called %d.\n"
				"with un-set plot:", plot_id);
		return FALSE;
	}
	for (i = 0; i < NUM_PLOTS; i++)
	{
		if (i == plot_id)
			continue; // Don't check yourself
		if (!PLOT_SET(i))
			continue;
		if (PLOT_MIN (plot_id, i) == 0 && PLOT_MAX (plot_id, i) == MAX_PWEIGHT)
			continue;
		distance_sq = ((plot[plot_id].star_pt.x - plot[i].star_pt.x) *
				(plot[plot_id].star_pt.x - plot[i].star_pt.x) +
				(plot[plot_id].star_pt.y - plot[i].star_pt.y) *
				(plot[plot_id].star_pt.y - plot[i].star_pt.y));
#ifdef DEBUG_STARSEED_TRACE_X
		fprintf (stderr, "__dsq %d mindsq %d maxdsq %d__", distance_sq,
				PLOT_MIN (plot_id, i), PLOT_MAX (plot_id, i));
#endif
		if (distance_sq < PLOT_MIN (plot_id, i) ||
				(distance_sq > PLOT_MAX (plot_id, i) &&
				PLOT_MAX (plot_id, i) > 0))
			return FALSE;
	}
	return TRUE;
}

// Plotify (internal) takes
// starmap - map you are plotting
// star - star you are adjusting to the plot
// Using the provided starmap, set the passed star to proper plot
// specifications, using other plots on the starmap, and/or using
// the global starmap_array constants.
// We let the colors mostly stay random but this is easily changed.
void
Plotify (STAR_DESC *starmap, STAR_DESC *star)
{
	if (!starmap || !star)
	{
		fprintf (stderr, "Plotify (starmap, star) called with NULL PTR.\n");
		return;
	}
	extern const STAR_DESC starmap_array[];
	COUNT i = 0;

	while (star->Index != starmap_array[i].Index)
		if (++i >= NUM_SOLAR_SYSTEMS)
			return; // It doesn't have one

	// Section 1: change the star to the right size for the plot
	if (STAR_TYPE (starmap_array[i].Type) == SUPER_GIANT_STAR)
		star->Type = MAKE_STAR (
				SUPER_GIANT_STAR,
				STAR_COLOR (star->Type),
				STAR_OWNER (star->Type));
	if (STAR_TYPE (starmap_array[i].Type) == GIANT_STAR)
		star->Type = MAKE_STAR (
				GIANT_STAR,
				STAR_COLOR (star->Type),
				STAR_OWNER (star->Type));
	if (STAR_TYPE (starmap_array[i].Type) == DWARF_STAR)
		star->Type = MAKE_STAR (
				DWARF_STAR,
				STAR_COLOR (star->Type),
				STAR_OWNER (star->Type));

	// Section 2: change colors of stars based on plot
	// Specific homeworlds that work best with their correct color star (SOL)
	if (star->Index == SOL_DEFINED)
		star->Type = starmap_array[i].Type;
	// On world 44s, the Eye Of Dogar is Green, of course
	// All other channels are false gods
	if (star->Index == ILWRATH_DEFINED)
	{
		if (optCustomSeed % 44 == 0)
				star->Type = starmap_array[i].Type;
		else star->Type = MAKE_STAR (
				STAR_TYPE (star->Type),
				STAR_COLOR (starmap[i].Type) +
					(optCustomSeed % 5 >= GREEN_BODY ?
					(optCustomSeed % 5) + 1 :
					optCustomSeed % 5),
				STAR_OWNER (star->Type));
	}
	// Supox will NOT be the same color as rainbow world
	// This swaps yellow/blue, red/white, and green/orange
	if (star->Index == SUPOX_DEFINED)
	{
		int j = 0;
		while (RAINBOW0_DEFINED != starmap[j].Index)
			if (++j >= NUM_SOLAR_SYSTEMS)
				return; // It doesn't have one
		star->Type = MAKE_STAR
				(DWARF_STAR,
				(STAR_COLOR (starmap[j].Type + 6) + 
					STAR_COLOR (starmap[j].Type) % 2 -
					(STAR_COLOR (starmap[j].Type) + 1) % 2) % 6,
				STAR_OWNER (star->Type));
	}
	// Rainbow worlds all get RAINBOW0's color, whatever it is.
	// We will use that later in the alien dialogs.
	if ((star->Index > RAINBOW0_DEFINED) &&
			(star->Index <= RAINBOW9_DEFINED))
	{
		int j = 0;
		while (RAINBOW0_DEFINED != starmap[j].Index)
			if (++j >= NUM_SOLAR_SYSTEMS)
				return; // It doesn't have one
		star->Type = MAKE_STAR
				(STAR_TYPE (star->Type),
				STAR_COLOR (starmap[j].Type),
				STAR_OWNER (star->Type));
	}

	// Section 3: Mess with names of star systems
	// Melnormes live in super giants, and super giants take the Alpha
	// of their system.
	if ((star->Index >= MELNORME0_DEFINED) &&
			(star->Index <= MELNORME8_DEFINED) &&
			(star->Prefix > 1))
	{
		int j = 0;
		while ((starmap[j].Postfix != star->Postfix) ||
				(starmap[j].Prefix != 1))
			if (++j >= NUM_SOLAR_SYSTEMS)
				return; // It doesn't have one
#ifdef DEBUG_STARSEED
		fprintf (stderr, "Melnorme %d swapping %d %d at %d.%d : %d.%d for "
				"%d %d at %d.%d : %d.%d\n",
				star->Index - MELNORME0_DEFINED, star->Prefix, star->Postfix,
				star->star_pt.x / 10, star->star_pt.x % 10,
				star->star_pt.y / 10, star->star_pt.y % 10,
				starmap[j].Prefix, starmap[j].Postfix,
				starmap[j].star_pt.x / 10, starmap[j].star_pt.x % 10,
				starmap[j].star_pt.y / 10, starmap[j].star_pt.y % 10);
#endif
		starmap[j].Prefix = star->Prefix;
		star->Prefix = 1;
	}
	// Alas, the poor algolites... wherever they are, the stars
	// carry their name (Postfix 113).
	if (star->Index == ALGOLITES_DEFINED)
	{
		int j = 0;
		for (j = 0; j < NUM_SOLAR_SYSTEMS; j++)
		{
			if (&starmap[j] == star)
				continue; // Not yet...
			if (starmap[j].Postfix == star->Postfix)
				starmap[j].Postfix = 113;
			else if (starmap[j].Postfix == 113)
				starmap[j].Postfix = star->Postfix;
		}
		star->Postfix = 113;
	}
}

#ifdef DEBUG_STARSEED_TRACE
COUNT last_err = NUM_PLOTS;
void
DebugPlotTicker (COUNT plot_id)
{
	if (last_err == plot_id)
	{
		fprintf(stderr, ".");
	}
	else
	{
		fprintf(stderr, "\n");
		print_plot_id (plot_id);
		last_err = plot_id;
	}
}
#endif

// The amount we skip around in the starmap, must be coprime with
// NUM_SOLAR_SYSTEMS (502)
#define STAR_FACTOR 89

// SeedPlot will create a seeded location for each remaining item on the
// plotmap which does not have an assigned location, within the bounds
// provided by the min/max range variables.  If a valid location can be found,
// recurse further until complete, otherwise return failed plot ID which is
// used by previous iteration to retry with differnt locations.
// NUM_PLOTS = success; NUM_PLOTS + 1 = timed out
COUNT
SeedPlot (PLOT_LOCATION *plotmap, STAR_DESC *starmap)
{
	static BOOLEAN timer_running = FALSE;
	static clock_t timer;
	if (!plotmap || !starmap)
	{
		fprintf (stderr, "SeedPlot (plotmap, starmap) called with NULL PTR.\n");
		return 0;
	}
	// The clock.  The first time this is called the clock is not running.
	// It will start the clock, and set timer_running TRUE globally, and
	// my_clock TRUE *locally*, which is how you detect the top layer of the
	// recursion and thus stop the clock on failure.
	BOOLEAN my_clock = FALSE;
#ifdef DEBUG_STARSEED_TRACE_Z
	BOOLEAN tried[NUM_SOLAR_SYSTEMS] = {[0 ... NUM_SOLAR_SYSTEMS - 1] = FALSE};
#endif
	UWORD rand_val;
	COUNT plot_id, star_id, i;
	COUNT return_id;
	// timelimit is in deciseconds, if loading 60 seconds, if new 2 seconds
	COUNT timelimit = (GLOBAL (CurrentActivity) ? 600 : 20);
#ifdef DEBUG_STARSEED_TRACE
	timelimit = (GLOBAL (CurrentActivity) ? 3600 : 3600);
#endif

	if (!StarGenRNG)
	{
		fprintf (stderr, "****SeedPlot creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New ();
	}
	if (!timer_running)
	{
#ifdef DEBUG_STARSEED
		fprintf (stderr,
				"%s %d | optCustomSeed %d | NUM_PLOTS %d.\n",
				"Starting a timer; global activity", GLOBAL (CurrentActivity),
				optCustomSeed, NUM_PLOTS);
#endif
		// Basically a wrapper around the recursion.
		timer_running = TRUE;
		next_plot = ~0;
		my_clock = TRUE;
		timer = clock();
		RandomContext_SeedRandom (StarGenRNG, optCustomSeed);
	}
	else if ((clock() - timer) / 100000 > timelimit)
	{
		fprintf (stderr, "TIME'S UP!  Giving up on seed %d.\n", optCustomSeed);
		timer_running = FALSE;
		return NUM_PLOTS + 1;
	}

	rand_val = RandomContext_Random (StarGenRNG);
	// choose a plot by weight
	if ((plot_id = GetNextPlot (plotmap)) == NUM_PLOTS)
	{
		// All plots are assigned, but any without location allocated
		// were passed in and need Plotify().
		//next_plot = ~0;
		timer_running = FALSE;
		for (i = 1; i < NUM_PLOTS; i++)
		{
			// Sanity check the placements - they all pass PLOT_SET(id)
			// but may be missing from starmap.
			if (!plotmap[i].star || plotmap[i].star != FindNearestStar
					(starmap, plotmap[i].star_pt))
			{
				print_plot_id (i);
				fprintf (stderr, " star ptr was unassigned, corrected.\n");
				plotmap[i].star = FindNearestStar (starmap, plotmap[i].star_pt);
			}
			if ((plotmap[i].star_pt.x != plotmap[i].star->star_pt.x) ||
					(plotmap[i].star_pt.y != plotmap[i].star->star_pt.y))
			{
				print_plot_id (i);
				fprintf (stderr, " coords don't match star ptr, corrected.\n");
				plotmap[i].star_pt = plotmap[i].star->star_pt;
			}
			if (plotmap[i].star->Index != i)
			{
				print_plot_id (i);
				fprintf (stderr, " plot location was missing from star, "
						"corrected.\n");
				plotmap[i].star->Index = i;
				Plotify (starmap, plotmap[i].star);
			}
#ifdef DEBUG_STARSEED
			print_plot_id (i);
			fprintf (stderr, " at %05.1f : %05.1f.\n",
					(float) plotmap[i].star_pt.x / 10,
					(float) plotmap[i].star_pt.y / 10);
#endif
		}
		return NUM_PLOTS;
	}
#ifdef DEBUG_STARSEED_TRACE
	fprintf (stderr, "\nSelected ");
	print_plot_id (plot_id);
#endif

	// If it has coords, it was selected because no pointer; try and locate
	// those coords and if valid, use that pointer to seed the plot
	if (plotmap[plot_id].star_pt.x != ~0 &&
			plotmap[plot_id].star_pt.y != ~0 &&
			(plotmap[plot_id].star =
			FindNearestStar (starmap, plotmap[plot_id].star_pt)))
	{
		// If destination is not empty, prior seeding is a failure.
		if (plot_id > 0 && plotmap[plot_id].star->Index != 0 &&
				plotmap[plot_id].star->Index != plot_id)
		{
			if (my_clock)
			{
				fprintf(stderr, "Complete failure (starmap full?), "
						"stopping clock.\n");
				timer_running = FALSE;
			}
			return plot_id;
		}
		if (plot_id > 0)
		{
			plotmap[plot_id].star->Index = plot_id;
#ifdef DEBUG_STARSEED
			print_plot_id (plot_id);
			fprintf(stderr, " static location %05.1f : %05.1f "
					"being stamped to starmap.\n",
					(float) plotmap[plot_id].star_pt.x / 10,
					(float) plotmap[plot_id].star_pt.y / 10);
#endif
		}
		return_id = SeedPlot (plotmap, starmap);
		if (return_id == NUM_PLOTS)
		{
			timer_running = FALSE;
			if (plot_id != ARILOU_DEFINED)
				Plotify (starmap, plotmap[plot_id].star);
			return return_id;
		}
		// Theoretically by leaving this assigned, we don't have to iterate
		// through it further.  Alas, this fouls the map if we have to reseed.
		if (plot_id != ARILOU_DEFINED)
			plotmap[plot_id].star->Index = 0;
		plotmap[plot_id].star = NULL;
		if (my_clock)
		{
			fprintf(stderr, "Complete failure, stopping clock.\n");
			timer_running = FALSE;
		}
		return return_id;
	}
	// Otherwise find this plot an empty star to call home, then recurse
	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
#ifdef DEBUG_STARSEED_TRACE
		DebugPlotTicker (plot_id);
#endif
		// ARILOU is a special case, we pick a point not a star
		if (plot_id == ARILOU_DEFINED)
		{
			star_id = (rand_val + i * STAR_FACTOR) % 100;
			plotmap[ARILOU_DEFINED].star_pt = (POINT)
					{(star_id / 10) * 1000 +
					LOBYTE (rand_val) * 100 / 256,
					(star_id % 10) * 1000 +
					HIBYTE (rand_val) * 100 / 256};
			plotmap[ARILOU_DEFINED].star = FindNearestStar (starmap,
					plotmap[ARILOU_DEFINED].star_pt);
			// Anything less than 2.0 units from a star is too close
			if ((plotmap[ARILOU_DEFINED].star) &&
					((plotmap[ARILOU_DEFINED].star_pt.x -
					plotmap[ARILOU_DEFINED].star->star_pt.x) *
					(plotmap[ARILOU_DEFINED].star_pt.x -
					plotmap[ARILOU_DEFINED].star->star_pt.x) +
					(plotmap[ARILOU_DEFINED].star_pt.y -
					plotmap[ARILOU_DEFINED].star->star_pt.y) *
					(plotmap[ARILOU_DEFINED].star_pt.y -
					plotmap[ARILOU_DEFINED].star->star_pt.y) < 400))
				continue;
			// Arilou homeworld pointer for reasons
			star_id = NUM_SOLAR_SYSTEMS + 1 + NUM_HYPER_VORTICES + 1;
			plotmap[plot_id].star = &(starmap[star_id]);
		}
		else
		{
			star_id = (rand_val + i * STAR_FACTOR) % NUM_SOLAR_SYSTEMS;
			// Not empty, requeue
			if (starmap[star_id].Index != 0)
				continue;
			// Put the plot in the starsystem
			starmap[star_id].Index = plot_id;
			// Put the starsystem in the plot
			plotmap[plot_id].star_pt = starmap[star_id].star_pt;
			plotmap[plot_id].star = &(starmap[star_id]);
		}
		// If this is valid, then try and move on to the next plot.
		// If recursion succeeds, success.  Otherwise, if unrelated to us,
		// pop this layer entirely, or if is related to us, requeue this.
		// This is not 100% mathematically complete; if the cause of failure
		// is because the star I selected made it no longer possible for the
		// other plots to complete.  Additionally, it should attempt to
		// repick the failed plot, altering the seeding tree going forward
		// and losing significant areas of the solution set.  This is
		// preferable to a high failure %.
		if (CheckValid (plotmap, plot_id))
		{
#ifdef DEBUG_STARSEED_TRACE
			print_plot_id (plot_id);
			fprintf (stderr, " trying %05.1f : %05.1f ",
					(float) plotmap[plot_id].star_pt.x / 10,
					(float) plotmap[plot_id].star_pt.y / 10);
#endif
			return_id = SeedPlot (plotmap, starmap);
			if (return_id == NUM_PLOTS)
			{
				timer_running = FALSE;
				if (plot_id != ARILOU_DEFINED)
					Plotify (starmap, plotmap[plot_id].star);
				return return_id;
			}
			// If we ran out of time or the plot is not heavily connected,
			// pop this layer
			if (return_id > NUM_PLOTS ||
					ConnectedPlot (plotmap, plot_id, return_id) <= 2000000)
			{
#ifdef DEBUG_STARSEED_TRACE_Y
				fprintf (stderr, " [%d] Popping ", return_id);
				print_plot_id (plot_id);
				fprintf (stderr, ", from %05.1f : %05.1f [C %d]\n",
						(float) plotmap[plot_id].star_pt.x / 10,
						(float) plotmap[plot_id].star_pt.y / 10,
						(return_id > NUM_PLOTS) ? -1 :
						(DWORD) sqrt (ConnectedPlot
						(plotmap, plot_id, return_id)));
#endif
				if (plot_id != ARILOU_DEFINED)
					starmap[star_id].Index = 0;
				plotmap[plot_id].star_pt = (POINT) {~0, ~0};
				plotmap[plot_id].star = NULL;
				if (my_clock)
				{
					fprintf(stderr, "Complete failure, stopping clock.\n");
					timer_running = FALSE;
				}
				return return_id;
			}
		}
		// admit defeat and move on to the next star system
#ifdef DEBUG_STARSEED_TRACE_Y
		fprintf(stderr, "Removing ");
		print_plot_id (plot_id);
		fprintf (stderr, ", from %05.1f : %05.1f, ",
				(float) plotmap[plot_id].star_pt.x / 10,
				(float) plotmap[plot_id].star_pt.y / 10);
#endif
		if (plot_id != ARILOU_DEFINED)
			starmap[star_id].Index = 0;
		plotmap[plot_id].star_pt = (POINT) {~0, ~0};
		plotmap[plot_id].star = NULL;
	}
#ifdef DEBUG_STARSEED_TRACE_Z
	fprintf (stderr, "RAN OUT OF PERMUTATIONS.\n");
#endif
	// Ran out of stars to try - this particular branch is impossible 
	// return my own plot ID to pop above
	if (my_clock)
	{
		fprintf(stderr, "Complete failure, stopping clock.\n");
		timer_running = FALSE;
	}
	return (plot_id);
}

// Reset the quasispace portal map to the static default portalmap_array
void
DefaultQuasispace (PORTAL_LOCATION *portalmap)
{
	if (!portalmap)
	{
		fprintf (stderr, "DefaultQuasispace (portalmap) called"
				"with NULL PTR.\n");
		return;
	}
	extern const STAR_DESC starmap_array[];
	COUNT i;
	for (i = 0; i < NUM_HYPER_VORTICES + 1; i++)
	{
		portalmap[i].star_pt = portalmap_array[i].star_pt;
		portalmap[i].quasi_pt = portalmap_array[i].quasi_pt;
		portalmap[i].nearest_star = FindNearestConstellation
				((void *) starmap_array, portalmap[i].star_pt);
		if (!portalmap[i].nearest_star)
			fprintf (stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, %s",
					'A' + i, (float) portalmap[i].star_pt.x / 10,
					(float) portalmap[i].star_pt.y / 10,
					"but no star found with FindNearestStar.\n");
	}
}

// Seeds the quasispace to hyperspace portals in hyperspace, ensuring a minimum
// distance between each.  Then randomizes the quasispace vortices using a
// random 0..25 * VORTEX_SCALE (similar to how they appear to be created),
// and keep them a minimum distance apart as well.
// Then sort the quasi side based on Y value ascending (starmap requirement)
// Then shove the coords into the star_map (QUASI side) in the same order
BOOLEAN
SeedQuasispace (PORTAL_LOCATION *portalmap, PLOT_LOCATION *plotmap,
		STAR_DESC *starmap)
{
	PORTAL_LOCATION swap;
	UWORD rand_val;
	COUNT i, j;
	BOOLEAN valid;
	if (!portalmap || !plotmap || !starmap)
	{
		fprintf (stderr, "Seed Quasispace called with NULL pointer(s).\n");
		return FALSE;
	}
	if (!StarGenRNG)
	{
		fprintf(stderr, "****SeedQuasispace creating a STAR GEN RNG****\n");
		StarGenRNG = RandomContext_New ();
	}
	RandomContext_SeedRandom (StarGenRNG, optCustomSeed);
	// NUM_HYPER_VORTICES is the Arilou Homeworld, but no reason to move it
	for (i = 0; i < NUM_HYPER_VORTICES; i++)
	{
		do
		{
			valid = TRUE;
			// Place a portal near, but not too near, SOL first.
			if (i == 0)
			{
				rand_val = RandomContext_Random (StarGenRNG);
				// I know this seems arbitrary but rand(256) * 9 gives a value
				// between 0 and 2295.  Half of that is below 1147, the other
				// half above.
				// We locate the portal by up to 1147 (114.7) in any direction.
				// Also note COORD is just SWORD and can be negative.
				portalmap[i].star_pt = (POINT) {
						(COORD) LOBYTE (rand_val) * 9 +
						plotmap[SOL_DEFINED].star_pt.x - 1147,
						(COORD) HIBYTE (rand_val) * 9 +
						plotmap[SOL_DEFINED].star_pt.y - 1147};
				// Take care of anything off the map (w/in 18.9) or too close
				// to SOL.  Here, 250000 (50.0 squared) is the min distance
				// squared from SOL to portal and 1000000 (100.0 squared) is
				// the max distance squared from SOL to portal.
				// Note: as long as MIN_PORTAL > 1000 + 500 (1500) you don't
				// have to check for other portals too close to SOL
				// (as long as 500 is that limit).
				if ((portalmap[i].star_pt.x < 189) ||
						(portalmap[i].star_pt.x > MAX_X_UNIVERSE - 189) ||
						(portalmap[i].star_pt.y < 189) ||
						(portalmap[i].star_pt.y > MAX_Y_UNIVERSE - 189) ||
						((portalmap[i].star_pt.x -
						plotmap[SOL_DEFINED].star_pt.x) *
						(portalmap[i].star_pt.x -
						plotmap[SOL_DEFINED].star_pt.x) +
						(portalmap[i].star_pt.y -
						plotmap[SOL_DEFINED].star_pt.y) *
						(portalmap[i].star_pt.y -
						plotmap[SOL_DEFINED].star_pt.y) < 250000) ||
						((portalmap[i].star_pt.x -
						plotmap[SOL_DEFINED].star_pt.x) *
						(portalmap[i].star_pt.x -
						plotmap[SOL_DEFINED].star_pt.x) +
						(portalmap[i].star_pt.y -
						plotmap[SOL_DEFINED].star_pt.y) *
						(portalmap[i].star_pt.y -
						plotmap[SOL_DEFINED].star_pt.y) > 1000000))
				{
					valid = FALSE;
					continue;
				}
			}
			// Place a portal, make sure it is MIN_PORTAL from the others
			else
			{
				rand_val = RandomContext_Random (StarGenRNG);
				// Squeeze gently at the sides to prevent being close to
				// map edges.  This should give a value between
				// 189 [0] and 9810 [255].
				// ([rnd] 255 * 9999 / 265 = 9621) + 189 = 9810
				// (or 189 below the top)
				portalmap[i].star_pt = (POINT) {LOBYTE (rand_val) *
						MAX_X_UNIVERSE / 265 + 189,
						HIBYTE (rand_val) * MAX_Y_UNIVERSE / 265 + 189};
				for (j = 0; j < i; j++)
				{
					if ((portalmap[i].star_pt.x - portalmap[j].star_pt.x) *
							(portalmap[i].star_pt.x - portalmap[j].star_pt.x) +
							(portalmap[i].star_pt.y - portalmap[j].star_pt.y) *
							(portalmap[i].star_pt.y - portalmap[j].star_pt.y) <
							MIN_PORTAL * MIN_PORTAL)
						valid = FALSE;
				}
				if (!valid)
					continue;
			}
			if (portalmap[i].star_pt.x <= 0 ||
					portalmap[i].star_pt.x >= MAX_X_UNIVERSE ||
					portalmap[i].star_pt.y <= 0 ||
					portalmap[i].star_pt.y >= MAX_Y_UNIVERSE)
			{
				fprintf (stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, %s",
					'A' + i, (float) portalmap[i].star_pt.x / 10,
					(float) portalmap[i].star_pt.y / 10,
					"but no star found with FindNearestStar.\n");
				valid = FALSE;
				continue;
			}
			// Find the nearest constellation name to assign to this portal
			portalmap[i].nearest_star = FindNearestConstellation
					(star_array, portalmap[i].star_pt);
			if (!portalmap[i].nearest_star)
			{
				fprintf (stderr, "BAD Quasi Portal %c at %05.1f : %05.1f, %s",
					'A' + i, (float) portalmap[i].star_pt.x / 10,
					(float) portalmap[i].star_pt.y / 10,
					"but no star found with FindNearestStar.\n");
				valid = FALSE;
				continue;
			}
			if ((portalmap[i].nearest_star->star_pt.x -
					portalmap[i].star_pt.x) *
					(portalmap[i].nearest_star->star_pt.x -
					portalmap[i].star_pt.x) +
					(portalmap[i].nearest_star->star_pt.y -
					portalmap[i].star_pt.y) *
					(portalmap[i].nearest_star->star_pt.y -
					portalmap[i].star_pt.y) < 400)
			{
#ifdef DEBUG_STARSEED
				fprintf(stderr, "Picked Quasi Portal %c at %05.1f : %05.1f, ",
						'A' + i, portalmap[i].star_pt.x / 10
						portalmap[i].star_pt.y / 10);
				fprintf(stderr, "nearest star found at %05.1f : %05.1f, "
						(float) portalmap[i].nearest_star->star_pt.x / 10,
						(float) portalmap[i].nearest_star->star_pt.y / 10);
				fprintf(stderr, "portal TOO CLOSE to star.\n");
#endif
				valid = FALSE;
				continue;
			}
		} while (!valid);
		valid = FALSE;
		// Now we give a random vortex scaled position,
		// keeping specific distances in mind.
		while (!valid)
		{
			valid = TRUE;
			rand_val = RandomContext_Random (StarGenRNG);
			portalmap[i].quasi_pt = (POINT)
					{(LOBYTE (rand_val) * 51 / 256 - 25) * VORTEX_SCALE + 5000,
					(HIBYTE (rand_val) * 51 / 256 - 25) * VORTEX_SCALE + 5000};
			for (j = 0; j < i; j++)
			{
				if ((portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) *
						(portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) +
						(portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) *
						(portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) <
						MIN_VORTEX * MIN_VORTEX)
					valid = FALSE;
			}
			if ((portalmap[i].quasi_pt.x - 5000) *
					(portalmap[i].quasi_pt.x - 5000) +
					(portalmap[i].quasi_pt.y - 5000) *
					(portalmap[i].quasi_pt.y - 5000) <
					MIN_VORTEX * MIN_VORTEX)
				valid = FALSE;
		}
	}
	// Now we must sort the quasi side and place in the starmap
	valid = FALSE;
	while (!valid)
	{
		valid = TRUE;
		// Yes I know bubble sorts are lazy but we really only do this once.
		for (i = 0; i < NUM_HYPER_VORTICES - 1; i++)
			for (j = i + 1; j < NUM_HYPER_VORTICES; j++)
				if (portalmap[i].quasi_pt.y > portalmap[j].quasi_pt.y ||
						(portalmap[i].quasi_pt.y == portalmap[j].quasi_pt.y &&
						portalmap[i].quasi_pt.x > portalmap[j].quasi_pt.x))
				{
					valid = FALSE;
					swap = portalmap[i];
					portalmap[i] = portalmap[j];
					portalmap[j] = swap;
				}
	}
	for (i = 0, j = NUM_SOLAR_SYSTEMS + 1; i < NUM_HYPER_VORTICES; ++i, ++j)
	{
		starmap[j].star_pt = portalmap[i].quasi_pt;
	}
	return TRUE;
}

typedef struct {
	const char *idStr;
	COUNT id;
} PlotIdMap;

// The PlotIdMap is sorted **by name** for the binary search function.
static PlotIdMap plotIdMap[] = {
	{"algolites",			ALGOLITES_DEFINED},		// Algol [IV]
	{"androsynth",			ANDROSYNTH_DEFINED},	// Eta Vulpeculae
	{"aqua helix",			AQUA_HELIX_DEFINED},	// Zeta Draconis
	{"arilou",				ARILOU_DEFINED},		// 43.8 : 637.2
	{"bomb",				BOMB_DEFINED},			// Zeta Hyades [VI-B]
	{"burvixese",			BURVIXESE_DEFINED},		// Arcturus [I]
	{"chmmr",				CHMMR_DEFINED},			// Procyon
	{"destroyed starbase",	DESTROYED_STARBASE_DEFINED},
	{"druuge",				DRUUGE_DEFINED},		// Zeta Persei [I]
	{"egg case 0",			EGG_CASE0_DEFINED},		// Beta Copernicus [I]
	{"egg case 1",			EGG_CASE1_DEFINED},
	{"egg case 2",			EGG_CASE2_DEFINED},
	{"excavation site",		EXCAVATION_SITE_DEFINED},	// ALPHA LALANDE I
	{"ilwrath",				ILWRATH_DEFINED},		// .. / .. / 022.9, 366.6
	{"kohrah 0",			KOHRAH0_DEFINED},
	{"kohrah 1",			KOHRAH1_DEFINED},
	{"kohrah 2",			KOHRAH2_DEFINED},
	{"maidens",				MAIDENS_DEFINED},		// Alpha Cerenkov [I]
	{"melnorme 0",			MELNORME0_DEFINED},
	{"melnorme 1",			MELNORME1_DEFINED},		// Alpha Centauri (SOL)
	{"melnorme 2",			MELNORME2_DEFINED},
	{"melnorme 3",			MELNORME3_DEFINED},
	{"melnorme 4",			MELNORME4_DEFINED},
	{"melnorme 5",			MELNORME5_DEFINED},
	{"melnorme 6",			MELNORME6_DEFINED},
	{"melnorme 7",			MELNORME7_DEFINED},		// Alpha Apodis (Thraddash)
	{"melnorme 8",			MELNORME8_DEFINED},
	{"mother ark",			MOTHER_ARK_DEFINED},	//Delta Virginis (not said)
	{"mycon",				MYCON_DEFINED},			// Brahe / 629.1, 220.8
	{"mycon trap",			MYCON_TRAP_DEFINED},	// Organon [1] [I]
	{"orz",					ORZ_DEFINED},			// Gamma Vulpeculae
	{"pkunk",				PKUNK_DEFINED},			// Gamma Krueger [I]
	{"rainbow 0",			RAINBOW0_DEFINED},		// Zeta Sextantis (Yehat)
	{"rainbow 1",			RAINBOW1_DEFINED},		//
	{"rainbow 2",			RAINBOW2_DEFINED},		//
	{"rainbow 3",			RAINBOW3_DEFINED},		//
	{"rainbow 4",			RAINBOW4_DEFINED},		// Slylandro (blue pair)
	{"rainbow 5",			RAINBOW5_DEFINED},		// Epsilon Draconis (Thradd)
	{"rainbow 6",			RAINBOW6_DEFINED},
	{"rainbow 7",			RAINBOW7_DEFINED},		// Beta Leporis (Supox)
	{"rainbow 8",			RAINBOW8_DEFINED},
	{"rainbow 9",			RAINBOW9_DEFINED},		// GIANT rbw (Groombridge)
													// Thraddash / Slylandro.
	{"samatra",				SAMATRA_DEFINED},		// Delta Crateris
	{"ship vault",			SHIP_VAULT_DEFINED},	// Epsilon Camelopardalis
	{"shofixti",			SHOFIXTI_DEFINED},		// Delta Gorno
	{"slylandro",			SLYLANDRO_DEFINED},		// Beta Corvi
	{"sol",					SOL_DEFINED},
	{"spathi",				SPATHI_DEFINED},		// Epsilon Gruis
	{"spathi monument",		SPATHI_MONUMENT_DEFINED}, // Beta Herculis
	{"start colony",		START_COLONY_DEFINED},  // Vela
	{"sun device",			SUN_DEVICE_DEFINED},	// Beta Brahe
	{"supox",				SUPOX_DEFINED},
	{"syreen",				SYREEN_DEFINED},		// Betelgeuse
	{"taalo protector",		TAALO_PROTECTOR_DEFINED}, // Delta Vulpeculae
	{"talking pet",			TALKING_PET_DEFINED},	// Beta Orionis
	{"thraddash",			THRADD_DEFINED},		// Delta Draconis
	{"urquan 0",			URQUAN0_DEFINED},
	{"urquan 1",			URQUAN1_DEFINED},
	{"urquan 2",			URQUAN2_DEFINED},
	{"urquan wreck",		URQUAN_WRECK_DEFINED},	// Alpha Pavonis
	{"utwig",				UTWIG_DEFINED},			// Aquarii [constellation]
	{"vux",					VUX_DEFINED},			// Luyten [star group]
	{"vux beast",			VUX_BEAST_DEFINED},		// Delta Lyncis [I]
	{"yehat",				YEHAT_DEFINED},			// .. / Serpentis
	{"zoq colony 0",		ZOQ_COLONY0_DEFINED},
	{"zoq colony 1",		ZOQ_COLONY1_DEFINED},
	{"zoq colony 2",		ZOQ_COLONY2_DEFINED},
	{"zoq colony 3",		ZOQ_COLONY3_DEFINED},
	{"zoq scout",			ZOQ_SCOUT_DEFINED},		// Rigel
	{"zoqfot",				ZOQFOT_DEFINED}			// Alpha Tucanae
}; // Currently 70 total (correct number)

static int
PlotIdCompare (const void *id1, const void *id2)
{
#ifdef DEBUG_STARSEED
	fprintf (stderr, "compare %s and %s.\n",
			((PlotIdMap *) id1)->idStr, ((PlotIdMap *) id2)->idStr);
#endif
	return strcmp (((PlotIdMap *) id1)->idStr, ((PlotIdMap *) id2)->idStr);
}

COUNT
PlotIdStrToIndex (const char *plotIdStr)
{
#ifdef DEBUG_STARSEED
	fprintf (stderr, "START PlotIdStrToIndex %s.\n", plotIdStr);
#endif
	PlotIdMap key = { /* .idStr = */ plotIdStr, /* .id = */ ~0 };
	PlotIdMap *found = bsearch (&key, plotIdMap, ARRAY_SIZE (plotIdMap),
			sizeof plotIdMap[0], PlotIdCompare);
	if (found == NULL)
		return NUM_PLOTS + 1;
#ifdef DEBUG_STARSEED
	fprintf (stderr, "END PlotIdStrToIndex %s %d.\n", plotIdStr, found->id);
#endif
	return found->id;
}
#if 0
// Arilou 250 and why not Human, Slylandro
// Druuge 1400
// Ilwrath 1410 and Chmrr, Chenjesu technically
// Mycon 1070
// Orz 333 and Androsynth
// Pkunk 666
// Spathi 1000 and Mmrnmhrm (eventually)
// Supox 333	
// Thraddash 833
// Umgah 833 (poor Umgah are defined by their suffering)
// Ur Quan and Kohr Ah both 2666, and Syreen, KA/UQ and use the SAMATRA_DEFINED as homeworld
// Utwig 666
// VUX 900
// Yehat 750 and Shofixti
// Zoq Fot 320
/*
ARILOU_ID:
CHMMR_ID:
EARTHLING_ID:
ORZ_ID:
PKUNK_ID:
SHOFIXTI_ID:
SPATHI_ID:
SUPOX_ID:
THRADDASH_ID:
UTWIG_ID:
VUX_ID:
YEHAT_ID:
MELNORME_ID:
DRUUGE_ID:
ILWRATH_ID:
MYCON_ID:
SLYLANDRO_ID:
UMGAH_ID:
UR_QUAN_ID:
ZOQFOTPIK_ID:
SYREEN_ID:
KOHR_AH_ID:
ANDROSYNTH_ID:
CHENJESU_ID:
MMRNMHRM_ID:
*/

//
// Known location (SoI)		Map homeworld	offset		Jitter	Str		J/S %
// androsyn      0, 0,
// arilou        438, 6372,		438, 6372	same		0		250		0
// chenjesu      0, 0,
// chmmr         0, 0,
// druuge        9500, 2792,	9469, 2806	31, -14		34.015	1400	2.43
// human         1752, 1450,		same
// ilwrath       48, 1700,		229, 3666	Not using this one
// kohrah        6000, 6250,	6200, 5935	-200, 315	373.129	2666	14.00
// melnorme      MAX_X_UNIVERSE >> 1, MAX_Y_UNIVERSE >> 1,
// mmrnmhrm      0, 0,
// mycon         6392, 2200,	6291, 2208	101, -8		101.316	1070	9.47
// orz           3608, 2637,	3713, 2537	-113, 100	145.000	333		43.54
// pkunk         502, 401,		522, 525	-20, -124	125.603	666		18.86
// shofixti      0, 0,
// slylandr      333, 9812,		same
// spathi        2549, 3600,	2416, 3687	133, -87	158.928	1000	15.89
// supox         7468, 9246,	7414, 9124	54, 122		133.417	333		40.07
// syreen        0, 0,
// thradd        2535, 8358,	2535, 8358	same??		0		833		0
// umgah         1798, 6000,	1978, 5968	-180, 32	182.822	833		21.95
// urquan        5750, 6000,	6200, 5935	-450, 65	454.670	2666	17.05
// utwig         8534, 8797,	8630, 8693	-96, 104	141.534	666		21.25
// vux           4412, 1558,	4333, 1687	79, -129	151.268	900		16.80
// yehat         4970, 40,		4923, 294	47, -254	258.312	750		34.44
// zoqfot        3761, 5333,	4000, 5437	-239, -104	260.647	320		81.45
//
// Default starmaps for races and plots
// {{2908, 269}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), SHOFIXTI_DEFINED, 4, 82},
// {{4923, 294}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), YEHAT_DEFINED, 3, 74},
// {{ 522, 525}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), PKUNK_DEFINED, 3, 92},
// {{6858, 577}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), MYCON_TRAP_DEFINED, 0, 123},
// {{4681, 916}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), RAINBOW0_DEFINED, 6, 75},
// {{9333, 937}, MAKE_STAR (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME0_DEFINED, 2, 5},
// {{1559, 993}, MAKE_STAR (SUPER_GIANT_STAR, RED_BODY, -1), MELNORME1_DEFINED, 1, 80},
// {{1752, 1450}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), SOL_DEFINED, 0, 129},
// {{4333, 1687}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), VUX_DEFINED, 2, 88},
// {{3345, 1931}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), START_COLONY_DEFINED, 0, 98},
// {{3352, 1940}, MAKE_STAR (SUPER_GIANT_STAR, WHITE_BODY, -1), MELNORME2_DEFINED, 0, 97},
// {{4221, 1986}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), MAIDENS_DEFINED, 1, 100},
// {{6479, 2062}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), EGG_CASE1_DEFINED, 3, 71},
// {{2104, 2083}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), ZOQ_SCOUT_DEFINED, 0, 118},
// {{6291, 2208}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), MYCON_DEFINED, 5, 71},
// {{ 742, 2268}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), CHMMR_DEFINED, 0, 117},
// {{6395, 2312}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), SUN_DEVICE_DEFINED, 2, 12},
// {{3713, 2537}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ORZ_DEFINED, 3, 86},
// {{3587, 2566}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ANDROSYNTH_DEFINED, 7, 86},
// {{3654, 2587}, MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME3_DEFINED, 1, 86},
// {{3721, 2619}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), TAALO_PROTECTOR_DEFINED, 4, 86},
// {{6008, 2631}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), EGG_CASE0_DEFINED, 2, 14},
// {{3499, 2648}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), EXCAVATION_SITE_DEFINED, 1, 87},
// {{6354, 2729}, MAKE_STAR (DWARF_STAR, WHITE_BODY, -1), EGG_CASE2_DEFINED, 3, 12},
// {{9469, 2806}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), DRUUGE_DEFINED, 6, 61},
// {{6020, 2979}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), RAINBOW1_DEFINED, 3, 13},
// {{9000, 3250}, MAKE_STAR (DWARF_STAR, RED_BODY, -1), ALGOLITES_DEFINED, 0, 113},
// {{2354, 3291}, MAKE_STAR (SUPER_GIANT_STAR, RED_BODY, -1), MELNORME4_DEFINED, 1, 106},
// {{1104, 3333}, MAKE_STAR (DWARF_STAR, WHITE_BODY, -1), MOTHER_ARK_DEFINED, 4, 84},
// {{1758, 3418}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), SPATHI_MONUMENT_DEFINED, 2, 108},
// {{ 229, 3666}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ILWRATH_DEFINED, 1, 76},
// {{2416, 3687}, MAKE_STAR (GIANT_STAR, ORANGE_BODY, -1), SPATHI_DEFINED, 5, 109},
// {{4125, 3770}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), SYREEN_DEFINED, 0, 114},
// {{5937, 3937}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), SHIP_VAULT_DEFINED, 5, 10},
// {{5708, 4604}, MAKE_STAR (DWARF_STAR, RED_BODY, -1), URQUAN0_DEFINED, 0, 104},
// {{5006, 5011}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), URQUAN1_DEFINED, 3, 11},
// {{3679, 5068}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), ZOQ_COLONY0_DEFINED, 3, 17},
// {{7416, 5083}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), RAINBOW2_DEFINED, 3, 68},
// {{3770, 5250}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ZOQ_COLONY1_DEFINED, 2, 17},
// {{7020, 5291}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), KOHRAH0_DEFINED, 2, 68},
// {{3416, 5437}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), ZOQ_COLONY3_DEFINED, 2, 16},
// {{4000, 5437}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ZOQFOT_DEFINED, 1, 18},
// {{3937, 5625}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), ZOQ_COLONY2_DEFINED, 2, 18},
// {{9645, 5791}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), BURVIXESE_DEFINED, 0, 130},
// {{6200, 5935}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), SAMATRA_DEFINED, 4, 21},
// {{1978, 5968}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), TALKING_PET_DEFINED, 2, 54},
// {{4625, 6000}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), URQUAN2_DEFINED, 1, 131},
// {{1578, 6668}, MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME5_DEFINED, 1, 56},
// {{5145, 6958}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), KOHRAH1_DEFINED, 8, 28},
// {{8625, 7000}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), RAINBOW3_DEFINED, 1, 41},
// {{ 395, 7458}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), RAINBOW4_DEFINED, 2, 60},
// {{6479, 7541}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), KOHRAH2_DEFINED, 0, 38},
// {{5875, 7729}, MAKE_STAR (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME6_DEFINED, 1, 34},
// {{2836, 7857}, MAKE_STAR (DWARF_STAR, WHITE_BODY, -1), RAINBOW5_DEFINED, 5, 53},
// {{ 562, 8000}, MAKE_STAR (GIANT_STAR, GREEN_BODY, -1), URQUAN_WRECK_DEFINED, 1, 59},
// {{5437, 8270}, MAKE_STAR (DWARF_STAR, RED_BODY, -1), RAINBOW6_DEFINED, 5, 48},
// {{2535, 8358}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), THRADD_DEFINED, 4, 53},
// {{2582, 8507}, MAKE_STAR (SUPER_GIANT_STAR, YELLOW_BODY, -1), MELNORME7_DEFINED, 1, 2},
// {{7666, 8666}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), RAINBOW7_DEFINED, 2, 46},
// {{2776, 8673}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), AQUA_HELIX_DEFINED, 6, 53},
// {{8630, 8693}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), UTWIG_DEFINED, 2, 3},
// {{8534, 8797}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), RAINBOW8_DEFINED, 3, 3},
// {{ 333, 8916}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), DESTROYED_STARBASE_DEFINED, 3, 57}, // here
// {{9960, 9042}, MAKE_STAR (GIANT_STAR, WHITE_BODY, -1), RAINBOW9_DEFINED, 0, 42},
// {{7414, 9124}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), SUPOX_DEFINED, 2, 47},
// {{8500, 9372}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), BOMB_DEFINED, 6, 45},
// {{9159, 9745}, MAKE_STAR (SUPER_GIANT_STAR, BLUE_BODY, -1), MELNORME8_DEFINED, 1, 4},
// {{5704, 9795}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), VUX_BEAST_DEFINED, 4, 49},
// {{ 333, 9812}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), SLYLANDRO_DEFINED, 2, 27},
#endif
