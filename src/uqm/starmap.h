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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef STARMAP_H_INCL_
#define STARMAP_H_INCL_

#include "libs/compiler.h"
#include "planets/planets.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Moving NUM_HYPER_VORTICES here for portal map; one fewer than the map
// because of the Arilou homeworld.
#define NUM_SOLAR_SYSTEMS 502
#define NUM_HYPER_VORTICES 15

extern STAR_DESC *CurStarDescPtr;
// star_array is now a static buffer we will copy the starmap_array into, and
// then manipulate from there, so that original starmap is never altered.
extern STAR_DESC star_array[];
extern POINT *constel_array;

// Global plot_map stores star_pt (coords) used while seeding into starmap
// Once seeded the plot_map will contain STAR_DESC pointers to the
// star locations on the starmap as well.
extern PLOT_LOCATION plot_map[NUM_PLOTS];

// Global portal_map stores "star_pt" coords to each portal's exit,
// as well as "quasi_pt" coords in quasispace, and the STAR_DESC of
// the "closest constellation" for naming purposes.
// Also Arilou homeworld but this is still mostly hardcoded.
extern PORTAL_LOCATION portal_map[NUM_HYPER_VORTICES + 1];

// A globally available seed for seeding the starmap and the plots it contains.
extern RandomContext *StarGenRNG;

extern STAR_DESC* FindStar (STAR_DESC *pLastStar, POINT *puniverse,
		SIZE xbounds, SIZE ybounds);

// Populates buf with the full name of the star at pSD
// May not be used much any more ***
extern void GetClusterName (const STAR_DESC *pSD, UNICODE buf[]);

// Returns the closest star to point p on the given starmap
STAR_DESC *FindNearestStar (STAR_DESC *starmap, POINT p);

// Returns the star in the closest constellation to point p on the starmap
// Constellation only returns stars with a Prefix > 0
STAR_DESC *FindNearestConstellation (STAR_DESC *starmap, POINT p);

// Resets the starmap given to the default values of the static starmap_array.
void DefaultStarmap (STAR_DESC *starmap);

// Seeds the provided starmap with no plot, random size and color.
// Super Giant Star does not generate randomly, these are MELNORME#_DEFINED
void SeedStarmap (STAR_DESC *starmap);

// Sets the plot to the default values of the provided starmap
void DefaultPlot (PLOT_LOCATION *plot, STAR_DESC *starmap);

// Sets the basic "push" framework of Melnormes and rainbow worlds
// and associated plot threads to other plot IDs.
void InitMelnormeRainbow (PLOT_LOCATION *plotmap);

// InitPlot sets the plotmap object up with the standard set of plot
// requirements based on the default UQM plot line.  This gives the required
// min/max distance of each plot laden star from related plot laden stars.
void InitPlot (PLOT_LOCATION *plotmap);

// SeedPlot seeds the initialized plot map into the starmap given
// It selects the next plot which needs to be... plotted... and places it into
// the starmap, setting the Index as appropriate, then iterates until all plots
// are placed.
COUNT SeedPlot (PLOT_LOCATION *plotmap, STAR_DESC *starmap);

// Sets the portal map to the default values of the static portalmap_array
void DefaultQuasispace (PORTAL_LOCATION *portalmap);

// Seeds the portal map, selecting valid locations referencing the
// provided plot map and star map.
BOOLEAN SeedQuasispace (PORTAL_LOCATION *portalmap, PLOT_LOCATION *plotmap,
		STAR_DESC *starmap);

// Converts the given plot string to the plot ID enum value,
// e.g. "pkunk" -> PKUNK_DEFINED
COUNT PlotIdStrToIndex (const char *plotIdStr);

// JSD end of changes to this file

#define INTERNAL_STAR_INDEX -1

extern BOOLEAN isStarMarked (const int star_index,
		const char *marker_state);
extern void setStarMarked (const int star_index, const char *marker_state);

#if defined(__cplusplus)
}
#endif

#endif  /* STARMAP_H_INCL_ */

