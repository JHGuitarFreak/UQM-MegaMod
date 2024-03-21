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

#include "starmap.h"
#include "gamestr.h"
#include "globdata.h"
#include "libs/gfxlib.h"
//JSD include hyper.h for arilou homeworld
#include "hyper.h"

// JSD this is now a buffer for storing a copy of the starmap_array which we
// will modify from there
//STAR_DESC *star_array;
STAR_DESC star_array[NUM_SOLAR_SYSTEMS + NUM_HYPER_VORTICES + 3] =
		{[0 ... (NUM_SOLAR_SYSTEMS + NUM_HYPER_VORTICES + 2)] =
		{{~0, ~0}, 0, 0, 0, 0}};
STAR_DESC *CurStarDescPtr = 0;
POINT *constel_array;
// JSD Give my own starseed
RandomContext *StarGenRNG;


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

// JSD This is purely for debugging purposes
void print_plot_id (COUNT plot_id)
{
	static const char * const plot_name[] = {"ARILOU_DEFINED", "SOL_DEFINED", "SHOFIXTI_DEFINED", "MAIDENS_DEFINED", "START_COLONY_DEFINED", "SPATHI_DEFINED", "ZOQFOT_DEFINED", "MELNORME0_DEFINED", "MELNORME1_DEFINED", "MELNORME2_DEFINED", "MELNORME3_DEFINED", "MELNORME4_DEFINED", "MELNORME5_DEFINED", "MELNORME6_DEFINED", "MELNORME7_DEFINED", "MELNORME8_DEFINED", "TALKING_PET_DEFINED", "CHMMR_DEFINED", "SYREEN_DEFINED", "BURVIXESE_DEFINED", "SLYLANDRO_DEFINED", "DRUUGE_DEFINED", "BOMB_DEFINED", "AQUA_HELIX_DEFINED", "SUN_DEVICE_DEFINED", "TAALO_PROTECTOR_DEFINED", "SHIP_VAULT_DEFINED", "URQUAN_WRECK_DEFINED", "VUX_BEAST_DEFINED", "SAMATRA_DEFINED", "ZOQ_SCOUT_DEFINED", "MYCON_DEFINED", "EGG_CASE0_DEFINED", "EGG_CASE1_DEFINED", "EGG_CASE2_DEFINED", "PKUNK_DEFINED", "UTWIG_DEFINED", "SUPOX_DEFINED", "YEHAT_DEFINED", "VUX_DEFINED", "ORZ_DEFINED", "THRADD_DEFINED", "RAINBOW0_DEFINED", "RAINBOW1_DEFINED", "RAINBOW2_DEFINED", "RAINBOW3_DEFINED", "RAINBOW4_DEFINED", "RAINBOW5_DEFINED", "RAINBOW6_DEFINED", "RAINBOW7_DEFINED", "RAINBOW8_DEFINED", "RAINBOW9_DEFINED", "ILWRATH_DEFINED", "ANDROSYNTH_DEFINED", "MYCON_TRAP_DEFINED", "URQUAN_DEFINED", "KOHRAH_DEFINED", "DESTROYED_STARBASE_DEFINED", "MOTHER_ARK_DEFINED", "ZOQ_COLONY0_DEFINED", "ZOQ_COLONY1_DEFINED", "ALGOLITES_DEFINED", "SPATHI_MONUMENT_DEFINED", "EXCAVATION_SITE_DEFINED"};
	fprintf (stderr, "%s (%d)", plot_name[plot_id], plot_id);
}

// JSD - major changes in this area
// The plotmap is a global item (like starmap) that keeps track of the location on the starmap of each
// OG hard coded plot location, like SOL or the location of the SAMATRA.  It is populated first (default
// plot lengths from InitPlot, or write your own) and can be added to with further AddPlot calls.  Then
// it is used during SeedPlot which sets the starmap locations.  After this point it is kept globally to
// help with the creation/location of race fleets and groups, as well as for race dialogs when they need
// to issue system coodinates.
// JSD Plot location is a structure that contains the coordinates assigned to each plot (once chosen)
// And uses a dist_sq to store the min and max distance squared to other plots.  Keeps a running tally
// of the "plot weight" of each plot when the min/max distances are set.
// SET EACH PLOT AT MOST ONCE (or the weights will be wrong, not really broken)
// The COORDS are initialized with {~0, ~0}, the rest 0.
// Plot ID 0 is ARILOU_HYPERSPACE_PORTAL location.
PLOT_LOCATION plot_map[NUM_PLOTS] = {[0 ... (NUM_PLOTS - 1)] = {{~0, ~0}, NULL, {[0 ... (NUM_PLOTS - 1)] = 0}}};
#define VORTEX_SCALE 20
#define MIN_PORTAL_DISTANCE 2000
#define MIN_VORTEX_DISTANCE 150
// FYI these are called Hyperspace Vorticies (Vortex) when in Quasispace,
// and Quasispace Portals when in Hyperspace (all of these are exit only)
// The recurring 2-way Portal/Vortex is ARILOU_DEFINED plot loc and 5000x5000
// randomize the egress points using the plot ropes concept
// randomize the quasi side using random r (between 3-7 VORTEX_SCALE perhaps) and theta
// and maybe use plot ropes to force at least 100 apart?
// Then sort the quasi side based on Y value ascending
// Then shove the coords into the star_map (QUASI side) in the same order
PORTAL_LOCATION portal_map[NUM_HYPER_VORTICES+1] =
{
	{{4091, 7748}, {(-12* VORTEX_SCALE) + 5000, (-21 * VORTEX_SCALE) + 5000}, NULL},
	{{3184, 4906}, {(1* VORTEX_SCALE) + 5000, (-20 * VORTEX_SCALE) + 5000}, NULL},
	{{9211, 6104}, {(-16* VORTEX_SCALE) + 5000, (-18 * VORTEX_SCALE) + 5000}, NULL},
	{{5673, 1207}, {(8* VORTEX_SCALE) + 5000, (-17 * VORTEX_SCALE) + 5000}, NULL},
	{{1910,  926}, {(3* VORTEX_SCALE) + 5000, (-13 * VORTEX_SCALE) + 5000}, NULL},
	{{8607,  151}, {(-21* VORTEX_SCALE) + 5000, (-4 * VORTEX_SCALE) + 5000}, NULL},
	{{  50, 1647}, {(-4* VORTEX_SCALE) + 5000, (-4 * VORTEX_SCALE) + 5000}, NULL},
	{{6117, 4131}, {(-12* VORTEX_SCALE) + 5000, (-2 * VORTEX_SCALE) + 5000}, NULL},
	{{5658, 9712}, {(-26* VORTEX_SCALE) + 5000, (2 * VORTEX_SCALE) + 5000}, NULL},
	{{2302, 3988}, {(-17* VORTEX_SCALE) + 5000, (7 * VORTEX_SCALE) + 5000}, NULL},
	{{ 112, 9409}, {(10* VORTEX_SCALE) + 5000, (7 * VORTEX_SCALE) + 5000}, NULL},
	{{7752, 8906}, {(15* VORTEX_SCALE) + 5000, (14 * VORTEX_SCALE) + 5000}, NULL},
	{{ 368, 6332}, {(22* VORTEX_SCALE) + 5000, (16 * VORTEX_SCALE) + 5000}, NULL},
	{{9735, 3153}, {(-6* VORTEX_SCALE) + 5000, (19 * VORTEX_SCALE) + 5000}, NULL},
	{{5850, 6213}, {(10* VORTEX_SCALE) + 5000, (20 * VORTEX_SCALE) + 5000}, NULL},
	{{   0,    0}, {ARILOU_HOME_X, ARILOU_HOME_Y}, NULL}
};

// Functions which know how a plotmap works will call it "plot" otherwise plot_map or plotmap, who cares
// These functions have to have a "plot" variable to use the PLOT_ defines (functions) like PLOT_SET(id)
#define PLOT_SET(id) (plot[id].star_pt.x != ~0 && plot[id].star_pt.y != ~0)
#define PLOT_MAX(pi,pj) ((pi > pj) ? plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi])
#define PLOT_MAX_SET(pi,pj,value) if (pi > pj) plot[pi].dist_sq[pj] = value; else plot[pj].dist_sq[pi] = value
#define PLOT_MIN(pi,pj) ((pi < pj) ? plot[pi].dist_sq[pj] : plot[pj].dist_sq[pi])
#define PLOT_MIN_SET(pi,pj,value) if (pi < pj) plot[pi].dist_sq[pj] = value; else plot[pj].dist_sq[pi] = value;
#define PLOT_WEIGHT(id) (plot[id].dist_sq[id])

#define MIN_PLOT 100	// The minimum distance you can set a max plot length to, should probably be non-zero
#define MAX_PLOT 10000	// The maximum distance you can set a min plot length to - too large and it will push things off the map
			// We effectively won't care about the weight of a max plot length above this value either, it caps
#define MAX_PWEIGHT 100000000 // (10000 * 10000) or MAX_PLOT * MAX_PLOT
// Sets the min and max distances (squared internally for storage / comparison)
// of two plot IDs on given plotmap.  Now checks to see if already set,
// and remove the plot lengths from existing plotweights before changing.
// Also increments the plot weight totals of the plots.
// Min and max can't be the same value, should be at least 100 apart.
void SetPlotLength (PLOT_LOCATION *plot, COUNT plotA, COUNT plotB, COUNT p_min, COUNT p_max)
{
	COUNT min = p_min;
	COUNT max = p_max;
	// Reject bad data
	if (!(plot) || (plotA >= NUM_PLOTS) || (plotB >= NUM_PLOTS) || (p_min >= p_max))
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
		//fprintf (stderr, "SetPlotLength reducing plot min ");
		//print_plot_id (plotA);
		//fprintf (stderr, " (weight %d) to ", PLOT_WEIGHT (plotA));
		//print_plot_id (plotB);
		//fprintf (stderr, " (weight %d), min %d\n", PLOT_WEIGHT (plotB),
		//		(UWORD) sqrt (PLOT_MIN (plotA, plotB)));
		PLOT_WEIGHT (plotA) -= (UWORD) sqrt (PLOT_MIN (plotA, plotB));
		PLOT_WEIGHT (plotB) -= (UWORD) sqrt (PLOT_MIN (plotA, plotB));
	}
	if ((PLOT_MAX (plotA, plotB) != 0) && (PLOT_MAX (plotA, plotB) < MAX_PLOT * MAX_PLOT))
	{
		//fprintf (stderr, "SetPlotLength reducing plot max ");
		//print_plot_id (plotA);
		//fprintf (stderr, " (weight %d) to ", PLOT_WEIGHT (plotA));
		//print_plot_id (plotB);
		//fprintf (stderr, " (weight %d), max %d\n", PLOT_WEIGHT (plotB),
		//		(MAX_PLOT - (UWORD) sqrt (PLOT_MAX (plotA, plotB)))/2);
		PLOT_WEIGHT (plotA) += (UWORD) sqrt (PLOT_MAX (plotA, plotB)) - MAX_PLOT;
		PLOT_WEIGHT (plotB) += (UWORD) sqrt (PLOT_MAX (plotA, plotB)) - MAX_PLOT;
		//PLOT_WEIGHT (plotA) += ((UWORD) sqrt (PLOT_MAX (plotA, plotB)) - MAX_PLOT) / 2;
		//PLOT_WEIGHT (plotB) += ((UWORD) sqrt (PLOT_MAX (plotA, plotB)) - MAX_PLOT) / 2;
	}
	
	// Now set the plot lengths
	PLOT_MIN_SET (plotA, plotB, min * min);
	PLOT_MAX_SET (plotA, plotB, max * max);

	// Now we increase the weights
	// This isn't really the "right" answer but because of how many mutual pushes
	// can exist at once, pushes (MIN_PLOT) need to be "heavier" than pulls
	// (MAX_PLOT) so we divide MAX_PLOT by two.  A better solution might track
	// how many push/pulls and try and factor it in better.
	// In fact we may even want to judge weight based solely on pushes.
	PLOT_WEIGHT (plotA) += min + MAX_PLOT - max;
	PLOT_WEIGHT (plotB) += min + MAX_PLOT - max;
	//PLOT_WEIGHT (plotA) += min + ((MAX_PLOT - max) / 2);
	//PLOT_WEIGHT (plotB) += min + ((MAX_PLOT - max) / 2);
}

// If the min/max plot lengths are set and valid, gives relative weight of
// this plot pair, otherwise 0.  The relative weight is the min distance
// plus half the (MAX_PLOT - the max distance) of each plot thread connected.
DWORD ConnectedPlot (PLOT_LOCATION *plot, COUNT plotA, COUNT plotB)
{
	// report bad data
	if (!(plot && (plotA < NUM_PLOTS) && (plotB < NUM_PLOTS)))
	{
		fprintf (stderr, "%s called with bad data PTR %d %d\n",
				"ConnectedPlot (plotmap, plot_id, plot_id)",
				plotA, plotB);
		return 0;
	}
	return (((PLOT_MIN (plotA, plotB) > MAX_PWEIGHT)
			? MAX_PWEIGHT : PLOT_MIN (plotA, plotB)) +
		(((PLOT_MAX (plotA, plotB) == 0) || (PLOT_MAX (plotA, plotB) > MAX_PWEIGHT))
			? 0 : (MAX_PWEIGHT - PLOT_MAX (plotA, plotB))));
		// JSD Trust me
}

// This returns the plot_id (number) with the highest weight on the plot map which has
// yet to be assigned a home, or NUM_PLOTS if all plots are allocated
COUNT GetNextPlot (PLOT_LOCATION *plot)
{
	COUNT plot_id = 0;
	DWORD top_weight = 0;
	COUNT i;
	if (!(plot))
	{
		fprintf (stderr, "GetNextPlot (plotmap) called with bad data PTR\n");
		return 0;
	}
	for (i = 0; i < NUM_PLOTS; i++)
	{
		if ((PLOT_WEIGHT (i) > top_weight) && !(PLOT_SET(i)))
		{
			plot_id = i;
			top_weight = PLOT_WEIGHT (i);
		}
	}
	//fprintf(stderr, "Top Weight %d Plot ID %d", top_weight, plot_id);
	// After loop if top weight is 0 (plot_id will also be 0) there may be unassigned plots without weight, find the first one:
	if (top_weight == 0) while ((plot_id < NUM_PLOTS) && (PLOT_SET(plot_id))) plot_id++;
	// Now plot_id contains either the first unassigned plot with highest weight, OR NUM_PLOTS
	//fprintf(stderr, "Top Weight %d Plot ID %d", top_weight, plot_id);
	return plot_id;
}

// Sets all plot lengths and weights back to zero.  Does NOT clear COORDs, assumes you want to use existing COORDs.
void ResetPlotLengths (PLOT_LOCATION *plot)
{
	COUNT i, j;
	for (i = 0; i < NUM_PLOTS; i++) for (j = 0; j < NUM_PLOTS; j++) plot[i].dist_sq[j] = 0;
}
/*
		if (i > j)
		{
			plot[i].dist_sq[j] = 0;//MAX_PLOT * MAX_PLOT;
		}
		else plot[i].dist_sq[j] = 0;
}*/

// Sets all the plot locations back to zero zero.
void ResetPlotLocations (PLOT_LOCATION *plot)
{
	COUNT i;
	for (i = 0; i < NUM_PLOTS; i++)
		plot[i].star_pt = (POINT) {~0, ~0};
}

//COUNT GetNextConnectedPlot (PLOT_LOCATION *plot); // optional; returns next plot_id of highest weight plotline from allocated plots
//void SetPlotLocation (PLOT_LOCATION *pm, POINT); // sets the plot ID to a COORD on the map
// plot ropes:
// ARILOU_DEFINED = 0, SOL_DEFINED = 1, SHOFIXTI_DEFINED, MAIDENS_DEFINED, START_COLONY_DEFINED, SPATHI_DEFINED, ZOQFOT_DEFINED, MELNORME0_DEFINED, MELNORME1_DEFINED, MELNORME2_DEFINED, MELNORME3_DEFINED, MELNORME4_DEFINED, MELNORME5_DEFINED, MELNORME6_DEFINED, MELNORME7_DEFINED, MELNORME8_DEFINED, TALKING_PET_DEFINED, CHMMR_DEFINED, SYREEN_DEFINED, BURVIXESE_DEFINED, SLYLANDRO_DEFINED, DRUUGE_DEFINED, BOMB_DEFINED, AQUA_HELIX_DEFINED, SUN_DEVICE_DEFINED, TAALO_PROTECTOR_DEFINED, SHIP_VAULT_DEFINED, URQUAN_WRECK_DEFINED, VUX_BEAST_DEFINED, SAMATRA_DEFINED, ZOQ_SCOUT_DEFINED, MYCON_DEFINED, EGG_CASE0_DEFINED, EGG_CASE1_DEFINED, EGG_CASE2_DEFINED, PKUNK_DEFINED, UTWIG_DEFINED, SUPOX_DEFINED, YEHAT_DEFINED, VUX_DEFINED, ORZ_DEFINED, THRADD_DEFINED, RAINBOW0_DEFINED, RAINBOW1_DEFINED, RAINBOW2_DEFINED, RAINBOW3_DEFINED, RAINBOW4_DEFINED, RAINBOW5_DEFINED, RAINBOW6_DEFINED, RAINBOW7_DEFINED, RAINBOW8_DEFINED, RAINBOW9_DEFINED, ILWRATH_DEFINED, ANDROSYNTH_DEFINED, MYCON_TRAP_DEFINED, NUM_PLOTS
//
void InitPlot (PLOT_LOCATION *plotmap)
{
	COUNT i, j;
	ResetPlotLengths (plotmap);
	ResetPlotLocations (plotmap);

	// Set up buffers around anyone with a zone of influence during the war
	// or after the war where homeworlds inside the zone wouldn't make sense
	COUNT home_map[] =
	{ARILOU_DEFINED, 300,
	SOL_DEFINED, 700,
	YEHAT_DEFINED, 700,
	SHOFIXTI_DEFINED, 700,
	CHMMR_DEFINED, 700,		// Chenjesu
	SYREEN_DEFINED, 700,		// Bugsquirt
	EGG_CASE0_DEFINED, 700,		// Syra, when the walls fell
	MOTHER_ARK_DEFINED, 700,	// The Mmrnmhrm homeworld, I guess?

	DRUUGE_DEFINED, 1300,		// The Druuge have a huge AOI
	PKUNK_DEFINED, 700,
	UTWIG_DEFINED, 700,
	SUPOX_DEFINED, 300,
	ZOQFOT_DEFINED, 300,
	BURVIXESE_DEFINED, 300,

	SAMATRA_DEFINED, 3000,		// Represents the war zone
	ANDROSYNTH_DEFINED, 1000,
	ILWRATH_DEFINED, 1000,
	MYCON_DEFINED, 1000,
	SPATHI_DEFINED, 1000,
	THRADD_DEFINED, 1000,
	VUX_DEFINED, 1000,
	TALKING_PET_DEFINED, 1000};	// AKA the Umgah (8 + 6 + 8 currently 22 homeworlds)
// Set the homeworlds apart first (can be overwritten later)
// Stupid can't declare 2D array dynamically must fake it
	for (i = 0; i < 21 * 2; i += 2) for (j = i + 2; j < 22 * 2; j += 2)
	{
		fprintf(stderr, "Race ID %d %d Min Dist %d %d\n",
				home_map[i], home_map[j],
				home_map[i + 1], home_map[j + 1]);
		SetPlotLength (plotmap, home_map[i], home_map[j],
				(home_map[i + 1] > home_map[j + 1]) ? home_map[i + 1] : home_map[j + 1], MAX_PLOT);
	}
// Melnormes and Rainbow worlds all push each other pretty far away to keep
// them scattered nicely (may even need to crank these up). We then zero out
// their weights so that they are picked last.
	for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++) for (j = RAINBOW0_DEFINED; j <= RAINBOW9_DEFINED; j++)
		if (i != j) SetPlotLength (plotmap, i, j, 2500, MAX_PLOT);
	for (i = MELNORME0_DEFINED; i <= MELNORME8_DEFINED; i++) for (j = MELNORME0_DEFINED; j <= MELNORME8_DEFINED; j++)
		if (i != j) SetPlotLength (plotmap, i, j, 3000, MAX_PLOT);
	//for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++) plotmap[i].dist_sq[i] *= 5;
	//for (i = MELNORME0_DEFINED; i <= MELNORME8_DEFINED; i++) plotmap[i].dist_sq[i] *= 5;
	for (i = RAINBOW0_DEFINED; i <= RAINBOW9_DEFINED; i++) plotmap[i].dist_sq[i] = 0;
	for (i = MELNORME0_DEFINED; i <= MELNORME8_DEFINED; i++) plotmap[i].dist_sq[i] = 0;
// Arilou 250 and why not Human, Slylandro
	SetPlotLength (plotmap, ARILOU_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, SOL_DEFINED, SAMATRA_DEFINED, 5000, MAX_PLOT);	// I'm not a monster; start extra away from UQ/KA
	SetPlotLength (plotmap, ARILOU_DEFINED, ORZ_DEFINED, 4000, MAX_PLOT);	// The ORZ *smell*.
	SetPlotLength (plotmap, ARILOU_DEFINED, SOL_DEFINED, 3000, 7000);	// Not too close (hiding the humans) but not too far
	//SetPlotLength (plotmap, ARILOU_DEFINED, TALKING_PET_DEFINED, 1000, 3000);// The "sometimes a friend" zone
	SetPlotLength (plotmap, SLYLANDRO_DEFINED, SOL_DEFINED, 6000, MAX_PLOT);// Because I am a monster after all
	SetPlotLength (plotmap, SOL_DEFINED, START_COLONY_DEFINED, 1000, 3000);	// Not too close, but limited fuel (50 units = 5000)
// Druuge 1400
	SetPlotLength (plotmap, DRUUGE_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, DRUUGE_DEFINED, BURVIXESE_DEFINED, 2000, 4000);	 // Outside Druuge space but close enough to lure
// Ilwrath 1410 and Chmrr, Chenjesu technically
// The Ilwrath will start around 2/3 of the way from their homeworld to the Pkunk homeworld which should overlap
// the Pkunk home or very nearly.  They also need to be near the Chmmr so they can respond as long as they are fighting Pkunk.
	SetPlotLength (plotmap, CHMMR_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, ILWRATH_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, CHMMR_DEFINED, ILWRATH_DEFINED, 0, 3000);	 // If we put the Chmmr between Pkunk/Ilwrath
	SetPlotLength (plotmap, CHMMR_DEFINED, PKUNK_DEFINED, 0, 2500);		 // It should give a good result
	SetPlotLength (plotmap, ILWRATH_DEFINED, PKUNK_DEFINED, 3000, 5000);
// Mycon 1070
	SetPlotLength (plotmap, MYCON_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE0_DEFINED, 0, 1250);	 // Syra (no relation to bugsquirt)
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE1_DEFINED, 0, 1000);
	SetPlotLength (plotmap, MYCON_DEFINED, EGG_CASE2_DEFINED, 0, 750);
	SetPlotLength (plotmap, MYCON_DEFINED, MYCON_TRAP_DEFINED, 1000, 5000);	 // The trap should be outside Mycon space, not far
	SetPlotLength (plotmap, MYCON_DEFINED, SUN_DEVICE_DEFINED, 0, 750);
// Orz 333 and Androsynth
	SetPlotLength (plotmap, ORZ_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);	 // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED, ORZ_DEFINED, 0, 350);
	SetPlotLength (plotmap, ANDROSYNTH_DEFINED, SOL_DEFINED, 2500, MAX_PLOT);// Standard run away from home range
	SetPlotLength (plotmap, ORZ_DEFINED, TAALO_PROTECTOR_DEFINED, 0, 350);
// Pkunk 666
	SetPlotLength (plotmap, PKUNK_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, PKUNK_DEFINED, YEHAT_DEFINED, 2500, MAX_PLOT);	 // Standard run away from home range
// Spathi 1000 and Mmrnmhrm (eventually)
	SetPlotLength (plotmap, SPATHI_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
// Supox 333	
	SetPlotLength (plotmap, SUPOX_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, SUPOX_DEFINED, UTWIG_DEFINED, 0, 2000);		 // This is what we call the friend zone
// Thraddash 833
	SetPlotLength (plotmap, THRADD_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, THRADD_DEFINED, AQUA_HELIX_DEFINED, 0, 500);
	SetPlotLength (plotmap, ILWRATH_DEFINED, THRADD_DEFINED, 3000, MAX_PLOT);// They're away from the Ilwrath / Pkunk
	SetPlotLength (plotmap, PKUNK_DEFINED, THRADD_DEFINED, 3000, MAX_PLOT);	 // Conflict zone
// Umgah 833 (poor Umgah are defined by their suffering)
	SetPlotLength (plotmap, TALKING_PET_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
// Ur Quan and Kohr Ah both 2666, and Syreen, KA/UQ and use the SAMATRA_DEFINED as homeworld
	SetPlotLength (plotmap, SAMATRA_DEFINED, BURVIXESE_DEFINED, 0, 4000);	 // Used to lure the Kohr-Ah
	SetPlotLength (plotmap, SAMATRA_DEFINED, SHIP_VAULT_DEFINED, 0, 3000);	 // Storage is near Ur-Quan spce
	SetPlotLength (plotmap, SAMATRA_DEFINED, URQUAN_WRECK_DEFINED, 0, 3000); // Logically
	SetPlotLength (plotmap, SYREEN_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, SYREEN_DEFINED, SHIP_VAULT_DEFINED, 1850, 2100); // Syreen pilots are pretty accurate
// Utwig 666
	SetPlotLength (plotmap, UTWIG_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, UTWIG_DEFINED, BOMB_DEFINED, 0, 500);		 // Their bomb is inside their zone
// VUX 900
	SetPlotLength (plotmap, VUX_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);	 // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, VUX_DEFINED, MAIDENS_DEFINED, 0, 1000);		 // Maidens is Admiral ZEX
	SetPlotLength (plotmap, VUX_BEAST_DEFINED, MAIDENS_DEFINED, 5000, MAX_PLOT);// Tie to something... not too close to ZEX
// Yehat 750 and Shofixti
	SetPlotLength (plotmap, SHOFIXTI_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT);// Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, YEHAT_DEFINED, SAMATRA_DEFINED, 3000, MAX_PLOT); // Standard keep-away from the UQ/KA zone
	SetPlotLength (plotmap, SHOFIXTI_DEFINED, YEHAT_DEFINED, 0, 2000);	 // This is what we call the friend zone
// Zoq Fot 320
	SetPlotLength (plotmap, ZOQFOT_DEFINED, SAMATRA_DEFINED, 0, 3000);	 // C-C-Combo breaker ZFP start inside the conflict
	SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_SCOUT_DEFINED, 0, 3000);	 // Loosely bind ZFP - Scout - Sol (9000 total)
	SetPlotLength (plotmap, ZOQ_SCOUT_DEFINED, SOL_DEFINED, 0, 3000);	 // Keep in mind SOL pushes SAMATRA 5000
	SetPlotLength (plotmap, SHOFIXTI_DEFINED, RAINBOW0_DEFINED, 0, 1000);	 // Tanaka mentions a rainbow world
	SetPlotLength (plotmap, THRADD_DEFINED, RAINBOW1_DEFINED, 0, 1000);	 // Thraddash mentions a rainbow world
	SetPlotLength (plotmap, SLYLANDRO_DEFINED, RAINBOW2_DEFINED, 0, 5000);	 // The Slylandro mention two worlds but
	SetPlotLength (plotmap, SLYLANDRO_DEFINED, RAINBOW3_DEFINED, 0, 5000);	 // They're not necessarily close
	SetPlotLength (plotmap, MELNORME0_DEFINED, SOL_DEFINED, 0, 1000);	 // By some amazing coincidence Melnormes have an
										 // outpost near SOL.
	// if (EXTENDED) // JSD TODO: EXTENDED doesn't exist yet, but can it?
	{
		// ZFP colonies are inside the war zone and relatively near ZFP but not necessarily inside their zone
		SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY0_DEFINED, 0, 1250);
		SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY0_DEFINED, 0, 3000);
		SetPlotLength (plotmap, ZOQFOT_DEFINED, ZOQ_COLONY1_DEFINED, 0, 750);
		SetPlotLength (plotmap, SAMATRA_DEFINED, ZOQ_COLONY1_DEFINED, 0, 3000);
		// Excavation site is where the Androsynth were digging up their precursor artifacts
		SetPlotLength (plotmap, ANDROSYNTH_DEFINED, EXCAVATION_SITE_DEFINED, 0, 600);
		// Alas, the poor algolites (the naming of which is going to be a real chore)
		// I think I'm going to keep them in Druuge space for _reasons_ and weirdly not (required) 
		// to be super close to the Spathi
		SetPlotLength (plotmap, DRUUGE_DEFINED, ALGOLITES_DEFINED, 0, 1250);
		SetPlotLength (plotmap, SPATHI_DEFINED, ALGOLITES_DEFINED, 0, 4000);
		// Defines the Mmrnmhrm in relation to the Spathi
		SetPlotLength (plotmap, SPATHI_DEFINED, SPATHI_MONUMENT_DEFINED, 300, 1000);
		SetPlotLength (plotmap, MOTHER_ARK_DEFINED, SPATHI_MONUMENT_DEFINED, 0, 1000);
		SetPlotLength (plotmap, MOTHER_ARK_DEFINED, SPATHI_DEFINED, 1000, MAX_PLOT);
		// These two are casualties of the UQ war (need better text)
		SetPlotLength (plotmap, SAMATRA_DEFINED, URQUAN_DEFINED, 0, 3000);
		SetPlotLength (plotmap, SAMATRA_DEFINED, KOHRAH_DEFINED, 0, 3000);
		// This is an abandoned slave-shielded planet
		SetPlotLength (plotmap, SAMATRA_DEFINED, DESTROYED_STARBASE_DEFINED, 3000, MAX_PLOT);
	}	
}

// JSD Returns TRUE if all placed plots from plot_id's perspective are valid
// Don't need to worry about FindStar to verify the stars are correct, as they may temporarily
// go past validation check but eventually if the star is a false map, it will be replaced.
// Although it may also impose irrelevant restrictions in the interim.
// Do need to worry about undefined plot {~0,~0}.
BOOLEAN CheckValid (PLOT_LOCATION *plot, COUNT plot_id)
{
	COUNT i;
	DWORD distance_sq;
	if (!(plot && (plot_id < NUM_PLOTS)))
	{
		fprintf (stderr, "CheckValid (plotmap, plot_id) called with bad data or NULL: PTR %d\n", plot_id);
	}
	if (!PLOT_SET(plot_id))
	{
		fprintf(stderr, "CheckValid (plotmap, plot_id) called with un-set plot: %d\n", plot_id);
		return FALSE;
	} // WTF mate?
	for (i = 0; i < NUM_PLOTS; i++)
	{
		if (i == plot_id) continue; // Again, stop checking yourself!
		if (!PLOT_SET(i)) continue;
		if ((PLOT_MIN (plot_id, i) == 0) && (PLOT_MAX (plot_id, i) == MAX_PWEIGHT)) continue;
		distance_sq = ((plot[plot_id].star_pt.x - plot[i].star_pt.x) *
				(plot[plot_id].star_pt.x - plot[i].star_pt.x) +
				(plot[plot_id].star_pt.y - plot[i].star_pt.y) *
				(plot[plot_id].star_pt.y - plot[i].star_pt.y));
		//fprintf(stderr, "dsq %d mindsq %d maxdsq %d", distance_sq, PLOT_MIN (plot_id, i), PLOT_MAX (plot_id, i));
		if ((distance_sq < PLOT_MIN (plot_id, i)) ||
				((distance_sq > PLOT_MAX (plot_id, i)) && (PLOT_MAX (plot_id, i) > 0))) return FALSE;
	}
	return TRUE;
}

#if 0
Plotline based - Need to use constraints to BUILD the table, not to CHECK the table.
First make sure all plots have a max distance of 1500 to each other to make sure all are connected minimally?
Then start with any plot (or sort them based on their distance weights, the more min distance, or the less max distance, the heavier the weight also based on the 1500 number as "max distance".  I think actually this needs to be a lot lower like maybe ... pushes above 4000 are extreme.  Like maybe 1-2 per map can even exist (KA/UQ war zone e.g. pushes most races who arent ZFP or on a warpath out which is based on the sa matra)
these plots need to go first and then keep using constraints to pick the next plot NOT RANDOM, basically pick a star that fulfills that plot connection,
and then iterate to the next heaviest plotline.

Peelback (current) - if you get to the "no valid choice" you need to peel back to the first pick to which you are connected.  For example the mycon egg 
to the first any other mycon item - anything in between unrelated to the egg would get peeled back with it, the whole tree is bad.  This is another mathematically complete
solution, because the picks in between had no impact on the outcome so we dont have to explore them.

These two are mutually exlcusive, if you use the tethers to build the table instead of random, they are all related, but theres a lot less noise to filter out.  I also think
theyll run about the same speed.  We might try one if the other fails.

BOOLEAN CheckValid (PLOT_LOCATION *plotmap)
{
	COUNT i, j;
	DWORD distance_sq;
	for (i = 0; i < NUM_PLOTS; i++) for (j = 0; j < NUM_PLOTS; j++)
	{
		if (i == j) continue; // Dont check yourself, stupid.  Three hours well spent.
		if ((plotmap[i].star_pt.x == ~0) || (plotmap[i].star_pt.y == ~0) ||
				(plotmap[j].star_pt.x == ~0) || (plotmap[j].star_pt.y == ~0)) continue;
		distance_sq = ((plotmap[i].star_pt.x - plotmap[j].star_pt.x) *
				(plotmap[i].star_pt.x - plotmap[j].star_pt.x) +
				(plotmap[i].star_pt.y - plotmap[j].star_pt.y) *
				(plotmap[i].star_pt.y - plotmap[j].star_pt.y));
		if ((distance_sq < plotmap[i].min_dist[j] * plotmap[i].min_dist[j]) ||
				((distance_sq > plotmap[i].max_dist[j] * plotmap[i].max_dist[j]) && (plotmap[i].max_dist[j] > 0))) return FALSE;
	}
	return TRUE;
}
#endif

// JSD SeedPlot will create a seeded location for each remaining item on the plotmap which does not have an
// assigned location, within the bounds provided by the min/max range variables.  If a valid location can be found,
// recurse further until complete, otherwise return failed plot ID which is used by previous iteration 
// to retry with differnt locations.
#define STAR_FACTOR 89 // The amount we skip around in the starmap, must be coprime with NUM_SOLAR_SYSTEMS (502)
COUNT SeedPlot (STAR_DESC *starmap, PLOT_LOCATION *plotmap, DWORD seed)
{
	UWORD rand_val;
	COUNT plot_id, star_id, i;
	STAR_DESC *SDPtr;
	COUNT return_id;

        RandomContext_SeedRandom (StarGenRNG, seed);
	rand_val = RandomContext_Random (StarGenRNG);
	// choose a plot by weight
	if ((plot_id = GetNextPlot (plotmap)) == NUM_PLOTS)
	{
		// Congrats, all plots are assigned, just make sure the starmap knows
		for (i = 1; i < NUM_PLOTS; i++)
		{
			//print_plot_id (i);
			SDPtr = FindStar (NULL, &plotmap[i].star_pt, 1, 1);
			if (!(SDPtr))
			{
				fprintf(stderr, "NULL PTR ");
				print_plot_id (i);
				return (i);
			}
			SDPtr->Index=i;
		}
		return NUM_PLOTS;
	}
#if 0
	if ((plot_id < 10) || (plot_id > 15))
	{
		fprintf(stderr, "\nSelected ");
		//fprintf(stderr, "Plot ID %d RNG %d\n", plot_id, rand_val);
		print_plot_id (plot_id);
	}
#endif
	// ARILOU is a special case
	if (plot_id == ARILOU_DEFINED)
	{
		//return ( NUM_PLOTS);
		for (i = 0; i < 100; i++)
		{
			//fprintf(stderr, ":");
			// better ARILOU random placement - pick the sector randomly too
			star_id = (rand_val + i) % 100;
			plotmap[ARILOU_DEFINED].star_pt = (POINT) {(star_id / 10) * 1000 + LOBYTE (rand_val) * 100 / 256,
					(star_id % 10) * 1000 + HIBYTE (rand_val) * 100 / 256};
			//fprintf(stderr, ", %dx%d", plotmap[ARILOU_DEFINED].star_pt.x, plotmap[ARILOU_DEFINED].star_pt.y);
			// If this is valid, then try and move on to the next plot. If recursion succeeds, success.
			// Otherwise, if unrelated to us, pop this layer entirely, or if is related to us, requeue
			if (CheckValid (plotmap, plot_id))
			{
				//fprintf(stderr, "Seeding next plot");
				return_id = SeedPlot (starmap, plotmap, RandomContext_GetSeed(StarGenRNG));
				//fprintf(stderr, "returned %d", return_id);
				if (return_id == NUM_PLOTS) return NUM_PLOTS;
			 	// if ((PLOT_MIN (plot_id, return_id) == 0) && (PLOT_MAX (plot_id, return_id) == MAX_PWEIGHT))
				if (!(ConnectedPlot (plotmap, plot_id, return_id)))
				{
					//fprintf(stderr, "Popping ");
					//print_plot_id (plot_id);
					////fprintf(stderr, " from %dx%d, ", plotmap[plot_id].star_pt.x, plotmap[plot_id].star_pt.y);
					//fprintf(stderr, ", ");
					plotmap[ARILOU_DEFINED].star_pt = (POINT) {~0, ~0};
					return return_id;
				}
			}
			// admit defeat and move on to the next coordinates
			////fprintf(stderr, "Removing ");
			////print_plot_id (plot_id);
			////fprintf(stderr, " from %dx%d", plotmap[plot_id].star_pt.x, plotmap[plot_id].star_pt.y);
			plotmap[ARILOU_DEFINED].star_pt = (POINT) {~0, ~0};
		} // could not find any valid place for ARILOU, somehow
		return ARILOU_DEFINED;
	}
	// Now find this plot an empty star to call home, then recurse
	star_id = rand_val % NUM_SOLAR_SYSTEMS;
	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		// Not empty, requeue
		if (starmap[star_id].Index != 0)
		{
			star_id = (star_id + STAR_FACTOR) % NUM_SOLAR_SYSTEMS;
			//fprintf(stderr, ".");
			continue;
		}
		//print_plot_id (plot_id);
		//fprintf(stderr, "\n");
		//fprintf(stderr, "*");
		////fprintf(stderr, ", %dx%d", starmap[star_id].star_pt.x, starmap[star_id].star_pt.y);
		// Put the plot in the starsystem
		starmap[star_id].Index = plot_id;
		// Put the starsystem in the plot
		plotmap[plot_id].star_pt.x = starmap[star_id].star_pt.x;
		plotmap[plot_id].star_pt.y = starmap[star_id].star_pt.y;
		plotmap[plot_id].star = &(starmap[star_id]);
		// If this is valid, then try and move on to the next plot. If recursion succeeds, success.
		// Otherwise, if unrelated to us, pop this layer entirely, or if is related to us, requeue
		// This is not 100% mathematically complete - if the cause of failure is the star I selected
		// made it no longer possible for the other plots to complete.  But this represents a very
		// small area of the solution set.
		if (CheckValid (plotmap, plot_id))
		{
			return_id = SeedPlot (starmap, plotmap, RandomContext_GetSeed(StarGenRNG));
			//if (return_id == ARILOU_DEFINED) fprintf(stderr, "****ARILOU RETURN CODE****\n");
			if (return_id == NUM_PLOTS)
			{
				// set star size based on the plot's original star size.
				// For now we let the colors stay random but this is easily changed.
				extern const STAR_DESC starmap_array[];
				int j = 0;
				while (starmap[star_id].Index != starmap_array[j].Index)
					if (++j >= NUM_SOLAR_SYSTEMS)
						return NUM_PLOTS; // It doesn't have one
				// Found one, flip the type.
				fprintf(stderr, "Type %d Starmap ID %d ORIGINAL %d\n", starmap[star_id].Index,
						star_id, j);
				if (STAR_TYPE (starmap_array[j].Type) == SUPER_GIANT_STAR)
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR,
							STAR_COLOR (starmap[star_id].Type),
							STAR_OWNER (starmap[star_id].Type));
				if (STAR_TYPE (starmap_array[j].Type) == GIANT_STAR)
					starmap[star_id].Type = MAKE_STAR (GIANT_STAR,
							STAR_COLOR (starmap[star_id].Type),
							STAR_OWNER (starmap[star_id].Type));
				if (STAR_TYPE (starmap_array[j].Type) == DWARF_STAR)
					starmap[star_id].Type = MAKE_STAR (DWARF_STAR,
							STAR_COLOR (starmap[star_id].Type),
							STAR_OWNER (starmap[star_id].Type));


				// if ((plot_id >= MELNORME0_DEFINED) && (plot_id <= MELNORME8_DEFINED)) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, HIBYTE (rand_val) % NUM_STAR_COLORS, -1);
				/*
				// temporarily color code it for my convenience
				if ((plot_id >= MELNORME0_DEFINED) && (plot_id <= MELNORME8_DEFINED))
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, BLUE_BODY, -1);
				else if ((plot_id >= RAINBOW0_DEFINED) && (plot_id <= RAINBOW9_DEFINED))
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1);
				//else if (((plot_id >= MYCON_DEFINED) && (plot_id <= EGG_CASE2_DEFINED)) ||
				//		(plot_id == MYCON_TRAP_DEFINED) || (plot_id == SUN_DEVICE_DEFINED))
				//	starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, ORANGE_BODY, -1);
				else if ((plot_id == URQUAN_DEFINED) || (plot_id == KOHRAH_DEFINED) ||
						(plot_id == DESTROYED_STARBASE_DEFINED))
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, ORANGE_BODY, -1);
				else if ((plot_id == SAMATRA_DEFINED) || (plot_id == URQUAN_WRECK_DEFINED) ||
						(plot_id == ZOQFOT_DEFINED) || (plot_id == ZOQ_SCOUT_DEFINED) ||
						(plot_id == SHIP_VAULT_DEFINED) || (plot_id == SYREEN_DEFINED) ||
						(plot_id == BURVIXESE_DEFINED))
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, RED_BODY, -1);
				else if ((plot_id == SOL_DEFINED) || (plot_id == SUPOX_DEFINED) || (plot_id == ORZ_DEFINED) ||
						(plot_id == UTWIG_DEFINED) || (plot_id == BOMB_DEFINED) ||
						(plot_id == ANDROSYNTH_DEFINED) || (plot_id == TAALO_PROTECTOR_DEFINED) ||
						(plot_id == SLYLANDRO_DEFINED) || (plot_id == START_COLONY_DEFINED))
					starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, YELLOW_BODY, -1);
				else starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, WHITE_BODY, -1);
				*/
				//if (plot_id == ANDROSYNTH_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1);
				//if (plot_id == MYCON_TRAP_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, ORANGE_BODY, -1);
				//if (plot_id == EGG_CASE0_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, BLUE_BODY, -1);
				//if (plot_id == EGG_CASE1_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1);
				//if (plot_id == EGG_CASE2_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, YELLOW_BODY, -1);
				//if (plot_id == SUN_DEVICE_DEFINED) starmap[star_id].Type = MAKE_STAR (SUPER_GIANT_STAR, WHITE_BODY, -1);
				return NUM_PLOTS;
			}
			if (!(ConnectedPlot (plotmap, plot_id, return_id)))
			// if ((PLOT_MIN (plot_id, return_id) == 0) && (PLOT_MAX (plot_id, return_id) == MAX_PWEIGHT))
			{
				//fprintf(stderr, "Popping ");
				//print_plot_id (plot_id);
				//fprintf(stderr, ", ");
				////fprintf(stderr, " from %dx%d, ", plotmap[plot_id].star_pt.x, plotmap[plot_id].star_pt.y);
				starmap[star_id].Index = 0;
				plotmap[plot_id].star_pt = (POINT) {~0, ~0};
				return return_id;
			}
		}
		// admit defeat and move on to the next star system
		////fprintf(stderr, "Removing ");
		////print_plot_id (plot_id);
		////fprintf(stderr, " from %dx%d", plotmap[plot_id].star_pt.x, plotmap[plot_id].star_pt.y);
		starmap[star_id].Index = 0;
		plotmap[plot_id].star_pt = (POINT) {~0, ~0};
		star_id = (star_id + STAR_FACTOR) % NUM_SOLAR_SYSTEMS;
	}
	//fprintf(stderr, "\n\nRAN OUT OF PERMUTAITON\n\n");
	// Ran out of stars to try - this particular branch is impossible - return my own plot ID to pop above
	return (plot_id);
}

// JSD A function to randomize the type of each star, and the plot locations, using a locally defined plot framework
// Default: 9 Super-giant, 24 giant, 469 dwarf / 85 blue, 102 green, 53 white, 55 yellow, 102 orange, 105 red
// 80 blue dwarf 96 green dwarf 47 white dwarf 50 yellow dwarf 97 orange dwarf 99 red dwarf
// 4 blue giant 4 green giant 5 white giant 2 yellow giant 5 orange giant 4 red giant
// 1 blue super 2 green super 1 white super 3 yellow super 0 orange super 2 red super
//
void SeedStarmap (STAR_DESC *starmap, DWORD seed)
{
	COUNT i;
	UWORD rand_val;
	RandomContext_SeedRandom (StarGenRNG, seed);

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
	{
		rand_val = RandomContext_Random (StarGenRNG);
		// 469 is the default number of dwarf stars, and 24 the number of giant. Super giant is handled elsewhere.
		starmap[i].Type = MAKE_STAR (LOBYTE (rand_val) % (469 + 24) >= 24 ? DWARF_STAR : GIANT_STAR, HIBYTE (rand_val) % NUM_STAR_COLORS, -1);
		starmap[i].Index = 0;
	}
}

// Uses FindStar and therefore the default star_array (which is also what we should be seeding,
// but this is also problematic).
void SeedQuasispace (PORTAL_LOCATION *portalmap, PLOT_LOCATION *plotmap, DWORD seed)
{
	PORTAL_LOCATION swap;
	UWORD rand_val;
	COUNT i, j;
	BOOLEAN valid;
	RandomContext_SeedRandom (StarGenRNG, seed);
	// NUM_HYPER_VORTICES = Arilou Homeworld, but we don't actually desire to move it
	for (i = 0; i < NUM_HYPER_VORTICES; i++)
	{
		do
		{
			valid = TRUE;
			// Place a portal near SOL first.  Those perverted Arilou.  But not too close.
			if (i == 0)
			{
				rand_val = RandomContext_Random (StarGenRNG);
				// I know this seems arbitrary but rand(256) * 9 gives a value between
				// 0 and 2295.  Half of that is below 1147, the other half above.
				// We locate the portal by up to 1147 (114.7) in any direction.
				// Also note COORD is just SWORD and can be negative.
				portalmap[i].star_pt = (POINT) {
						(COORD) LOBYTE (rand_val) * 9 +
						plotmap[SOL_DEFINED].star_pt.x - 1147,
						(COORD) HIBYTE (rand_val) * 9 +
						plotmap[SOL_DEFINED].star_pt.y - 1147};
				// Take care of anything off the map or too close to SOL.
				// Here, 250000 (50.0 squared) is the min distance squared from SOL to portal.
				// And 1000000 (100.0 squared) is the max distance squared from SOL to portal.
				// Note: as long as MIN_PORTAL_DISTANCE > 1000 + 500 (1500) you don't have
				// to check for other portals too close to SOL (as long as 500 is that limit).
				if ((portalmap[i].star_pt.x < 189) ||
						(portalmap[i].star_pt.x > MAX_X_UNIVERSE - 189) ||
						(portalmap[i].star_pt.y < 189) ||
						(portalmap[i].star_pt.y > MAX_Y_UNIVERSE - 189) ||
						((portalmap[i].star_pt.x - plotmap[SOL_DEFINED].star_pt.x) *
						(portalmap[i].star_pt.x - plotmap[SOL_DEFINED].star_pt.x) +
						(portalmap[i].star_pt.y - plotmap[SOL_DEFINED].star_pt.y) *
						(portalmap[i].star_pt.y - plotmap[SOL_DEFINED].star_pt.y) < 250000) ||
						((portalmap[i].star_pt.x - plotmap[SOL_DEFINED].star_pt.x) *
						(portalmap[i].star_pt.x - plotmap[SOL_DEFINED].star_pt.x) +
						(portalmap[i].star_pt.y - plotmap[SOL_DEFINED].star_pt.y) *
						(portalmap[i].star_pt.y - plotmap[SOL_DEFINED].star_pt.y) > 1000000))
				{
					valid = FALSE;
					continue;
				}
			}
			// Place a normal portal, then make sure it is MIN_PORTAL_DISTANCE from the others
			else
			{
				rand_val = RandomContext_Random (StarGenRNG);
				// Squeeze gently at the sides to prevent being close to edges
				// 255*9999/265 = 9621 + 189 = 9810 (or 189 below the top)
				portalmap[i].star_pt = (POINT) {LOBYTE (rand_val) * MAX_X_UNIVERSE / 265 + 189,
						HIBYTE (rand_val) * MAX_Y_UNIVERSE / 265 + 189};
				//portalmap[i].star_pt = (POINT) {LOBYTE (rand_val) * MAX_X_UNIVERSE / 256 + 128,
				//		HIBYTE (rand_val) * MAX_Y_UNIVERSE / 256 + 128};
				for (j = 0; j < i; j++)
				{
					if ((portalmap[i].star_pt.x - portalmap[j].star_pt.x) *
							(portalmap[i].star_pt.x - portalmap[j].star_pt.x) +
							(portalmap[i].star_pt.y - portalmap[j].star_pt.y) *
							(portalmap[i].star_pt.y - portalmap[j].star_pt.y) <
							MIN_PORTAL_DISTANCE * MIN_PORTAL_DISTANCE)
						valid = FALSE;
				}
				if (!valid)
					continue;
			}
			if ((portalmap[i].star_pt.x <= 0) || (portalmap[i].star_pt.x >= MAX_X_UNIVERSE) ||
					(portalmap[i].star_pt.y <= 0) || (portalmap[i].star_pt.y >= MAX_Y_UNIVERSE))
			{
				fprintf(stderr, "Bad Quasi Portal %c at %dx%d.\n", 'A' + i, portalmap[i].star_pt.x, portalmap[i].star_pt.y);
				valid = FALSE;
				continue;
			}
			// Find *nearest* star, it should never get to 10000.  This is terribly brute force but only
			// done once per portal.
			j = 0;
			while (!(portalmap[i].nearest_star = FindStar (NULL, &portalmap[i].star_pt, j, j)))
			{
				if (++j > 10000)
				{
					fprintf(stderr, "Picked Quasi Portal %c at %3d.%d:%3d.%d, ", 'A' + i,
							portalmap[i].star_pt.x / 10, portalmap[i].star_pt.x % 10,
							portalmap[i].star_pt.y / 10, portalmap[i].star_pt.y % 10);
					fprintf(stderr, "but no star found within %d distance.\n", j / 10);
					valid = FALSE;
					break;
				}
			}
			if (!valid) continue;
			if ((portalmap[i].nearest_star->star_pt.x - portalmap[i].star_pt.x) *
					(portalmap[i].nearest_star->star_pt.x - portalmap[i].star_pt.x) +
					(portalmap[i].nearest_star->star_pt.y - portalmap[i].star_pt.y) *
					(portalmap[i].nearest_star->star_pt.y - portalmap[i].star_pt.y) < 400)
			{
				fprintf(stderr, "Picked Quasi Portal %c at %3d.%d : %3d.%d; ", 'A' + i,
						portalmap[i].star_pt.x / 10, portalmap[i].star_pt.x % 10,
						portalmap[i].star_pt.y / 10, portalmap[i].star_pt.y % 10);
				fprintf(stderr, "Nearest star found at %3d.%d : %3d.%d; ",
						portalmap[i].nearest_star->star_pt.x / 10,
						portalmap[i].nearest_star->star_pt.x % 10,
						portalmap[i].nearest_star->star_pt.y / 10,
						portalmap[i].nearest_star->star_pt.y % 10);
				fprintf(stderr, "TOO CLOSE to star.\n");
				valid = FALSE;
				continue;
			}
		} while (!valid);
		valid = FALSE;
		// Now we give a random vortex scaled position, keeping specific distances in mind.
		while (!valid)
		{
			valid = TRUE;
			rand_val = RandomContext_Random (StarGenRNG);
			portalmap[i].quasi_pt = (POINT) {(LOBYTE (rand_val) * 51 / 256 - 26) * VORTEX_SCALE + 5000,
					(HIBYTE (rand_val) * 51 / 256 - 26) * VORTEX_SCALE + 5000};
			for (j = 0; j < i; j++)
			{
				if ((portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) *
						(portalmap[i].quasi_pt.x - portalmap[j].quasi_pt.x) +
						(portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) *
						(portalmap[i].quasi_pt.y - portalmap[j].quasi_pt.y) <
						MIN_VORTEX_DISTANCE * MIN_VORTEX_DISTANCE)
					valid = FALSE;
			}
			if ((portalmap[i].quasi_pt.x - 5000) *
						(portalmap[i].quasi_pt.x - 5000) +
						(portalmap[i].quasi_pt.y - 5000) *
						(portalmap[i].quasi_pt.y - 5000) <
						MIN_VORTEX_DISTANCE * MIN_VORTEX_DISTANCE)
				valid = FALSE;
		}
	}
	// Now we must sort the quasi side and place in the starmap
	valid = FALSE;
	while (!valid)
	{
		valid = TRUE;
		// Yes I know bubble sorts are lazy but we really only do this once.
		for (i = 0; i < NUM_HYPER_VORTICES - 1; i++) for (j = i + 1; j < NUM_HYPER_VORTICES; j++)
			if ((portalmap[i].quasi_pt.y > portalmap[j].quasi_pt.y) ||
					((portalmap[i].quasi_pt.y == portalmap[j].quasi_pt.y) &&
					(portalmap[i].quasi_pt.x > portalmap[j].quasi_pt.x)))
			{
				valid = FALSE;
				swap = portalmap[i];
				portalmap[i] = portalmap[j];
				portalmap[j] = swap;
			}
	}
	for (i = 0, j = NUM_SOLAR_SYSTEMS + 1; i < NUM_HYPER_VORTICES; ++i, ++j)
	{
		star_array[j].star_pt = portalmap[i].quasi_pt;
	}
}

#if 0
void SeedFleet (FLEET_INFO *FleetPtr, PLOT_LOCATION *plotmap, DWORD seed)
{
	COUNT x, y, r, theta;
	(r = LOBYTE (rand_val) % 50) >= 10 ? r -= 10; : r = 0;
	distance = r * strength / 100; // +50 for ZFP

}
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
// Known location (center of SoI)	Map homeworld	offset		Jsq	Jitter	Str	J/S %
// androsyn      0, 0,
// arilou        438, 6372,		438, 6372	same		0	0	250	0
// chenjesu      0, 0,
// chmmr         0, 0,
// druuge        9500, 2792,		9469, 2806	31, -14		1157	34.015	1400	2.43
// human         1752, 1450,		same
// ilwrath       48, 1700,		229, 3666	229 - 48 - 522, 3666 - 1700 - 525 (1966 - 1175)
// kohrah        6000, 6250,		6200, 5935	-200, 315	139225	373.129	2666	14.00
// melnorme      MAX_X_UNIVERSE >> 1, MAX_Y_UNIVERSE >> 1,
// mmrnmhrm      0, 0,
// mycon         6392, 2200,		6291, 2208	101, -8		10265	101.316	1070	9.47
// orz           3608, 2637,		3713, 2537	-113, 100	21025	145.000	333	43.54
// pkunk         502, 401,		522, 525	-20, -124	15776	125.603	666	18.86
// shofixti      0, 0,
// slylandr      333, 9812,		same
// spathi        2549, 3600,		2416, 3687	133, -87	25258	158.928	1000	15.89
// supox         7468, 9246,		7414, 9124	54, 122		17800	133.417	333	40.07
// syreen        0, 0,
// thradd        2535, 8358,		2535, 8358	same??		0	0	833	0
// umgah         1798, 6000,		1978, 5968	-180, 32	33424	182.822	833	21.95
// urquan        5750, 6000,		6200, 5935	-450, 65	206725	454.670	2666	17.05
// utwig         8534, 8797,		8630, 8693	-96, 104	20032	141.534	666	21.25
// vux           4412, 1558,		4333, 1687	79, -129	22882	151.268	900	16.80
// yehat         4970, 40,		4923, 294	47, -254	66725	258.312	750	34.44
// zoqfot        3761, 5333,		4000, 5437	-239, -104	67937	260.647	320	81.45
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
// {{5708, 4604}, MAKE_STAR (DWARF_STAR, RED_BODY, -1), URQUAN_DEFINED, 0, 104},
// {{5006, 5011}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), URQUAN_DEFINED, 3, 11},
// {{3679, 5068}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), ZOQ_COLONY0_DEFINED, 3, 17},
// {{7416, 5083}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), RAINBOW2_DEFINED, 3, 68},
// {{3770, 5250}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ZOQ_COLONY1_DEFINED, 2, 17},
// {{7020, 5291}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), KOHRAH_DEFINED, 2, 68},
// {{3416, 5437}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), ZOQ_COLONY1_DEFINED, 2, 16},
// {{4000, 5437}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), ZOQFOT_DEFINED, 1, 18},
// {{3937, 5625}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), ZOQ_COLONY0_DEFINED, 2, 18},
// {{9645, 5791}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), BURVIXESE_DEFINED, 0, 130},
// {{6200, 5935}, MAKE_STAR (DWARF_STAR, YELLOW_BODY, -1), SAMATRA_DEFINED, 4, 21},
// {{1978, 5968}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), TALKING_PET_DEFINED, 2, 54},
// {{4625, 6000}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), URQUAN_DEFINED, 1, 131},
// {{1578, 6668}, MAKE_STAR (SUPER_GIANT_STAR, GREEN_BODY, -1), MELNORME5_DEFINED, 1, 56},
// {{5145, 6958}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), KOHRAH_DEFINED, 8, 28},
// {{8625, 7000}, MAKE_STAR (DWARF_STAR, GREEN_BODY, -1), RAINBOW3_DEFINED, 1, 41},
// {{ 395, 7458}, MAKE_STAR (DWARF_STAR, BLUE_BODY, -1), RAINBOW4_DEFINED, 2, 60},
// {{6479, 7541}, MAKE_STAR (DWARF_STAR, ORANGE_BODY, -1), KOHRAH_DEFINED, 0, 38},
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
