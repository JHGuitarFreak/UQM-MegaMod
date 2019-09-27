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

#ifndef UQM_MASTER_H_
#define UQM_MASTER_H_

#include "races.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef HLINK HMASTERSHIP;

typedef struct
{
	// LINK elements; must be first
	HMASTERSHIP pred;
	HMASTERSHIP succ;

	SPECIES_ID SpeciesID;

	SHIP_INFO ShipInfo;
	FLEET_STUFF Fleet;
			// FLEET_STUFF is only necessary here because avail_race_q
			// is initialized in part from master_q (kinda hacky)
} MASTER_SHIP_INFO;

extern QUEUE master_q;
		/* List of ships available in SuperMelee;
		 * queue element is MASTER_SHIP_INFO */

static inline MASTER_SHIP_INFO *
LockMasterShip (const QUEUE *pq, HMASTERSHIP h)
{
	assert (GetLinkSize (pq) == sizeof (MASTER_SHIP_INFO));
	return (MASTER_SHIP_INFO *) LockLink (pq, h);
}

#define UnlockMasterShip(pq, h) UnlockLink (pq, h)
#define FreeMasterShip(pq, h) FreeLink (pq, h)

extern void LoadMasterShipList (void (* YieldProcessing)(void));
extern void FreeMasterShipList (void);
extern HMASTERSHIP FindMasterShip (SPECIES_ID ship_ref);
extern int FindMasterShipIndex (SPECIES_ID ship_ref);
COUNT GetShipCostFromIndex (unsigned Index);
FRAME GetShipIconsFromIndex (unsigned Index);
FRAME GetShipMeleeIconsFromIndex (unsigned Index);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_MASTER_H_ */

