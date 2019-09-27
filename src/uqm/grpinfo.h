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

#ifndef UQM_GRPINFO_H_
#define UQM_GRPINFO_H_

#include "port.h"
#include "libs/compiler.h"
#include "displist.h"
#include "libs/gfxlib.h"
		// for POINT
#include <assert.h>

#if defined(__cplusplus)
extern "C" {
#endif

// XXX: Needed to maintain savegame compatibility
#define NUM_SAVED_BATTLE_GROUPS 64

typedef HLINK HIPGROUP;

typedef struct
{
	// LINK elements; must be first
	HIPGROUP pred;
	HIPGROUP succ;

	UWORD group_counter;
	BYTE race_id;
	BYTE sys_loc;
	BYTE task;  // AKA mission
	BYTE in_system;
			// a simple != 0 flag
			// In older savegames this will be >1, because
			//   CloneShipFragment was used to spawn groups,
			//   and it set this to crew_level values
	
	BYTE dest_loc;
	BYTE orbit_pos;
			/* Also: saved prev dest_loc before intercept call,
			 *   restored to dest_loc on all-clear */
	BYTE group_id;
	POINT loc;

	FRAME melee_icon;
	
	// JMS: direction memory prevents jittering of battle group icons when they change direction they're flying to.
	BYTE lastDirection;
} IP_GROUP;

enum
{
	IN_ORBIT = 0,
	EXPLORE,
	FLEE,
	ON_STATION,

	IGNORE_FLAGSHIP = 1 << 2,
	REFORM_GROUP = 1 << 3
};
#define MAX_REVOLUTIONS 5

#define STATION_RADIUS 1600
#define ORBIT_RADIUS 2400

static inline IP_GROUP *
LockIpGroup (const QUEUE *pq, HIPGROUP h)
{
	assert (GetLinkSize (pq) == sizeof (IP_GROUP));
	return (IP_GROUP *) LockLink (pq, h);
}

#define UnlockIpGroup(pq, h)  UnlockLink (pq, h)
#define FreeIpGroup(pq, h)    FreeLink (pq, h)

extern HIPGROUP BuildGroup (QUEUE *pDstQueue, BYTE race_id);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_GRPINFO_H_ */
