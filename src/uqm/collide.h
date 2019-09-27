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

#ifndef UQM_COLLIDE_H_
#define UQM_COLLIDE_H_

#include "element.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define COLLISION_TURN_WAIT 1
#define COLLISION_THRUST_WAIT 3

#define SKIP_COLLISION (NONSOLID | DISAPPEARING)
#define CollidingElement(e) \
		(!((e)->state_flags & SKIP_COLLISION))
#define CollisionPossible(e0,e1) \
		(CollidingElement (e0) \
		&& (!(((e1)->state_flags & (e0)->state_flags) & COLLISION) \
		&& ((!(((e1)->state_flags & (e0)->state_flags) & IGNORE_SIMILAR) \
		|| (e1)->pParent != (e0)->pParent)) \
		&& ((e1)->mass_points || (e0)->mass_points)))

#define InitIntersectStartPoint(eptr) \
{ \
	(eptr)->IntersectControl.IntersectStamp.origin.x = \
			WORLD_TO_DISPLAY ((eptr)->current.location.x); \
	(eptr)->IntersectControl.IntersectStamp.origin.y = \
			WORLD_TO_DISPLAY ((eptr)->current.location.y); \
}

#define InitIntersectEndPoint(eptr) \
{ \
	(eptr)->IntersectControl.EndPoint.x = \
			WORLD_TO_DISPLAY ((eptr)->next.location.x); \
	(eptr)->IntersectControl.EndPoint.y = \
			WORLD_TO_DISPLAY ((eptr)->next.location.y); \
}

#define InitIntersectFrame(eptr) \
{ \
	(eptr)->IntersectControl.IntersectStamp.frame = \
			 SetEquFrameIndex ((eptr)->next.image.farray[0], \
			 (eptr)->next.image.frame); \
}

extern void collide (ELEMENT *ElementPtr0, ELEMENT *ElementPtr1);

#if defined(__cplusplus)
}
#endif

#endif /* UQM_COLLIDE_H_ */

