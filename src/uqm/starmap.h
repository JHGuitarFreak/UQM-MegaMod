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

extern STAR_DESC *CurStarDescPtr;
// star_array is now a static buffer we will copy the starmap_array into, and
// then manipulate from there, so that original starmap is never altered.
//extern STAR_DESC *star_array;
extern STAR_DESC star_array[];
extern POINT *constel_array;
// JSD adding a global plot_map variable, portal_map variable, and moving the
// NUM_HYPER_VORTICES here. NUM_HYPER_VORTICIES is one fewer than the map
// because of the Arilou homeworld.
#define NUM_SOLAR_SYSTEMS 502
#define NUM_HYPER_VORTICES 15
extern PLOT_LOCATION plot_map[NUM_PLOTS];
extern PORTAL_LOCATION portal_map[NUM_HYPER_VORTICES+1];

extern STAR_DESC* FindStar (STAR_DESC *pLastStar, POINT *puniverse,
		SIZE xbounds, SIZE ybounds);

extern void GetClusterName (const STAR_DESC *pSD, UNICODE buf[]);

// JSD Adding InitPlot, SeedPlot, SeedStarmap which will implement the seeding
// of plot across the starmap

// InitPlot sets the plotmap object up with the standard set of plot requirements
// based on the default UQM plot line.  This gives the required min/max distance
// of each plot laden star from related plot laden stars.
void InitPlot (PLOT_LOCATION *plotmap);

// SeedPlot takes the starmap and plotmap given, along with the starseed (long)
// It selects the next plot which needs to be... plotted... and places it into
// the starmap, setting the Index as appropriate, then iterates until all plots
// are placed.
COUNT SeedPlot (STAR_DESC *starmap, PLOT_LOCATION *plotmap, DWORD seed);

// SeedPortalmap takes an array of star_pt and quasi_pt elements and shuffles
// them (at both sides) according to seed.
void SeedQuasispace (PORTAL_LOCATION *portalmap, PLOT_LOCATION *plotmap, DWORD seed);

// SeedStarmap resets the starmap given to have no plots, and randomizes the size
// and colors of the stars on the map.  SUPER_GIANT_STAR does not generate randomly
// these will be allocated by the plotmap as MELNORME#_DEFINED.
void SeedStarmap (STAR_DESC *starmap, DWORD seed);

// A globally available seed for seeding the starmap and the plots it contains.
extern RandomContext *StarGenRNG;
// JSD end of changes to this file

#define INTERNAL_STAR_INDEX -1

extern BOOLEAN isStarMarked (const int star_index,
		const char *marker_state);
extern void setStarMarked (const int star_index, const char *marker_state);

#if defined(__cplusplus)
}
#endif

#endif  /* STARMAP_H_INCL_ */

