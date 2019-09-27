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

#ifndef UQM_HYPER_H_
#define UQM_HYPER_H_

#include "element.h"
#include "units.h"
		// for UNIT_SCREEN_WIDTH/HEIGHT

#if defined(__cplusplus)
extern "C" {
#endif

#define NUM_RADAR_SCREENS 12

#define RADAR_SCAN_WIDTH RES_DESCALE(UNIT_SCREEN_WIDTH * NUM_RADAR_SCREENS)
#define RADAR_SCAN_HEIGHT RES_DESCALE(UNIT_SCREEN_HEIGHT * NUM_RADAR_SCREENS)

// Hyperspace coordinates of the naturally occuring portal into QuasiSpace
#define ARILOU_SPACE_X   438
#define ARILOU_SPACE_Y  6372

// QuasiSpace coordinates of the same portal
#define QUASI_SPACE_X  5000
#define QUASI_SPACE_Y  5000

// QuasiSpace coordinates of the Arilou home world
#define ARILOU_HOME_X 6134 // (QUASI_SPACE_X + ((RADAR_SCAN_WIDTH >> 1) * 3))
#define ARILOU_HOME_Y 5900 // (QUASI_SPACE_Y + ((RADAR_SCAN_HEIGHT >> 1) * 3))

// HyperSpace coordinates of the locations where the QuasiSpace portals
// take you.
#define QUASISPACE_PORTALS_HYPERSPACE_ENDPOINTS \
		{ \
			{4091, 7748}, \
			{3184, 4906}, \
			{9211, 6104}, \
			{5673, 1207}, \
			{1910,  926}, \
			{8607,  151}, \
			{  50, 1647}, \
			{6117, 4131}, \
			{5658, 9712}, \
			{2302, 3988}, \
			{ 112, 9409}, \
			{7752, 8906}, \
			{ 368, 6332}, \
			{9735, 3153}, \
			{5850, 6213}, \
		}

// Hyperspace coordinates of the Sol system
// Should be the same as in plandata.c
#define SOL_X  1752
#define SOL_Y  1450
#define VELA_X  3345
#define VELA_Y  1931


extern BOOLEAN LoadHyperspace (void);
extern BOOLEAN FreeHyperspace (void);
extern void SeedUniverse (void);
extern void MoveSIS (SDWORD *pdx, SDWORD *pdy);

extern void FreeHyperData (void);
extern void check_hyperspace_encounter (void);
extern BOOLEAN hyper_transition (ELEMENT *ElementPtr);

extern void HyperspaceMenu (void);
extern void SaveSisHyperState (void);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_HYPER_H_ */

