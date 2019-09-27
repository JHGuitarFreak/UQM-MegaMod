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

#ifndef UQM_VELOCITY_H_
#define UQM_VELOCITY_H_

#include <string.h> /* for memset */
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct velocity_desc
{
	COUNT TravelAngle;
	EXTENT vector;
	EXTENT fract;
	EXTENT error;
	EXTENT incr;
} VELOCITY_DESC;

#define ZeroVelocityComponents(pv) memset(pv,0,sizeof (*(pv)))
#define GetVelocityTravelAngle(pv) (pv)->TravelAngle

extern void GetCurrentVelocityComponents (VELOCITY_DESC *velocityptr, SIZE *pdx, SIZE *pdy);

// JMS_GFX: New function to prevent overflows in hi-res.
extern void GetCurrentVelocityComponentsSdword (VELOCITY_DESC *velocityptr, SDWORD *pdx, SDWORD *pdy);

extern void GetNextVelocityComponents (VELOCITY_DESC *velocityptr, SIZE *pdx, SIZE *pdy, COUNT num_frames);

// JMS_GFX: New function to prevent overflows in hi-res.
extern void GetNextVelocityComponentsSdword (VELOCITY_DESC *velocityptr, SDWORD *pdx, SDWORD *pdy, DWORD num_frames);

extern void SetVelocityVector (VELOCITY_DESC *velocityptr, SDWORD magnitude, COUNT facing); // JMS_GFX: Changed SIZE to SDWORD to prevent overflows

extern void SetVelocityComponents (VELOCITY_DESC *velocityptr, SDWORD dx, SDWORD dy); // JMS_GFX: Changed SIZE to SDWORD to prevent overflows

extern void DeltaVelocityComponents (VELOCITY_DESC *velocityptr, SDWORD dx, SDWORD dy); // JMS_GFX: Changed SIZE to SDWORD to prevent overflows

static inline bool
IsVelocityZero (VELOCITY_DESC *vptr)
{
	return vptr->vector.width == 0 && vptr->vector.height == 0 &&
			vptr->incr.width == 0 && vptr->incr.height == 0 &&
			vptr->fract.width == 0 && vptr->fract.height == 0;
}

static inline DWORD
VelocitySquared (SIZE dx, SIZE dy)
{
	return (DWORD)((long)dx * dx + (long)dy * dy);
}

#define VELOCITY_SHIFT 5
#define VELOCITY_SCALE (1<<VELOCITY_SHIFT)

#define VELOCITY_TO_WORLD(v) ((v)>>VELOCITY_SHIFT)
#define WORLD_TO_VELOCITY(l) ((l)<<VELOCITY_SHIFT)

#if defined(__cplusplus)
}
#endif

#endif /* UQM_VELOCITY_H_ */

