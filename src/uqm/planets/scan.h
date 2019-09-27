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

#ifndef UQM_PLANETS_SCAN_H_
#define UQM_PLANETS_SCAN_H_

typedef struct scan_desc SCAN_DESC;
typedef struct scan_block SCAN_BLOCK;

#include "libs/compiler.h"
#include "libs/gfxlib.h"
#include "planets.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct scan_desc
{
	POINT start;
	COUNT start_dot;
	COUNT num_dots;
	COUNT dots_per_semi;
};

struct scan_block
{
	POINT *line_base;
	COUNT num_scans;
	COUNT num_same_scans;
	SCAN_DESC *scan_base;
};

extern void ScanSystem (void);

extern void RepairBackRect (RECT *pRect, BOOLEAN Fullscreen);
extern void GeneratePlanetSide (void);
extern COUNT callGenerateForScanType (const SOLARSYS_STATE *,
		const PLANET_DESC *world, COUNT node, BYTE scanType, NODE_INFO *);
// Returns true if the node should be removed from the surface
extern bool callPickupForScanType (SOLARSYS_STATE *solarSys,
		PLANET_DESC *world, COUNT node, BYTE scanType);

extern void RedrawSurfaceScan (const POINT *newLoc);
extern CONTEXT GetScanContext (BOOLEAN *owner);
extern void DestroyScanContext (void);

bool isNodeRetrieved (PLANET_INFO *planetInfo, BYTE scanType, BYTE nodeNr);
COUNT countNodesRetrieved (PLANET_INFO *planetInfo, BYTE scanType);
void setNodeRetrieved (PLANET_INFO *planetInfo, BYTE scanType, BYTE nodeNr);
void setNodeNotRetrieved (PLANET_INFO *planetInfo, BYTE scanType, BYTE nodeNr);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_PLANETS_SCAN_H_ */

