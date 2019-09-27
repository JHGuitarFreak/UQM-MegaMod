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

#include "weapon.h"

#include "colors.h"
#include "globdata.h"
#include "status.h"
#include "init.h"
#include "races.h"
#include "ship.h"
#include "setup.h"
#include "sounds.h"
#include "units.h"
#include "libs/mathlib.h"

#include <stdio.h>

// A wrapper function for weapon_collision that discards the return value.
// This makes its signature match ElementCollisionFunc.
static void
weapon_collision_cb (ELEMENT *WeaponElementPtr, POINT *pWPt,
		ELEMENT *HitElementPtr, POINT *pHPt)
{
	weapon_collision (WeaponElementPtr, pWPt, HitElementPtr, pHPt);
}


HELEMENT
initialize_laser (LASER_BLOCK *pLaserBlock)
{
	HELEMENT hLaserElement;

	hLaserElement = AllocElement ();
	if (hLaserElement)
	{
#define LASER_LIFE 1
		ELEMENT *LaserElementPtr;

		LockElement (hLaserElement, &LaserElementPtr);
		LaserElementPtr->playerNr = pLaserBlock->sender;
		LaserElementPtr->hit_points = 1;
		LaserElementPtr->mass_points = 1;
		LaserElementPtr->state_flags = APPEARING | FINITE_LIFE
				| pLaserBlock->flags;
		LaserElementPtr->life_span = LASER_LIFE;
		LaserElementPtr->collision_func = weapon_collision_cb;
		LaserElementPtr->blast_offset = 1;

		LaserElementPtr->current.location.x = pLaserBlock->cx
				+ COSINE (FACING_TO_ANGLE (pLaserBlock->face),
				DISPLAY_TO_WORLD (pLaserBlock->pixoffs));
		LaserElementPtr->current.location.y = pLaserBlock->cy
				+ SINE (FACING_TO_ANGLE (pLaserBlock->face),
				DISPLAY_TO_WORLD (pLaserBlock->pixoffs));
		SetPrimType (&DisplayArray[LaserElementPtr->PrimIndex], LINE_PRIM);
		SetPrimColor (&DisplayArray[LaserElementPtr->PrimIndex],
				pLaserBlock->color);
		LaserElementPtr->current.image.frame = DecFrameIndex (stars_in_space);
		LaserElementPtr->current.image.farray = &stars_in_space;
		SetVelocityComponents (&LaserElementPtr->velocity,
				WORLD_TO_VELOCITY ((pLaserBlock->cx + pLaserBlock->ex)
				- LaserElementPtr->current.location.x),
				WORLD_TO_VELOCITY ((pLaserBlock->cy + pLaserBlock->ey)
				- LaserElementPtr->current.location.y));
		UnlockElement (hLaserElement);
	}

	return (hLaserElement);
}

HELEMENT
initialize_missile (MISSILE_BLOCK *pMissileBlock)
{
	HELEMENT hMissileElement;

	hMissileElement = AllocElement ();
	if (hMissileElement)
	{
		SIZE delta_x, delta_y;
		COUNT angle;
		ELEMENT *MissileElementPtr;

		LockElement (hMissileElement, &MissileElementPtr);
		MissileElementPtr->hit_points = (BYTE)pMissileBlock->hit_points;
		MissileElementPtr->mass_points = (BYTE)pMissileBlock->damage;
		MissileElementPtr->playerNr = pMissileBlock->sender;
		MissileElementPtr->state_flags = APPEARING | FINITE_LIFE
				| pMissileBlock->flags;
		MissileElementPtr->life_span = pMissileBlock->life;
		SetPrimType (&DisplayArray[MissileElementPtr->PrimIndex], STAMP_PRIM);
		MissileElementPtr->current.image.farray = pMissileBlock->farray;
		MissileElementPtr->current.image.frame =
				SetAbsFrameIndex (pMissileBlock->farray[0],
				pMissileBlock->index);
		MissileElementPtr->preprocess_func = pMissileBlock->preprocess_func;
		MissileElementPtr->collision_func = weapon_collision_cb;
		MissileElementPtr->blast_offset = (BYTE)pMissileBlock->blast_offs;

		angle = FACING_TO_ANGLE (pMissileBlock->face);
		MissileElementPtr->current.location.x = pMissileBlock->cx
				+ COSINE (angle, DISPLAY_TO_WORLD (pMissileBlock->pixoffs));
		MissileElementPtr->current.location.y = pMissileBlock->cy
				+ SINE (angle, DISPLAY_TO_WORLD (pMissileBlock->pixoffs));

		delta_x = COSINE (angle, WORLD_TO_VELOCITY (pMissileBlock->speed));
		delta_y = SINE (angle, WORLD_TO_VELOCITY (pMissileBlock->speed));
		SetVelocityComponents (&MissileElementPtr->velocity,
				delta_x, delta_y);

		MissileElementPtr->current.location.x -= VELOCITY_TO_WORLD (delta_x);
		MissileElementPtr->current.location.y -= VELOCITY_TO_WORLD (delta_y);
		UnlockElement (hMissileElement);
	}

	return (hMissileElement);
}

HELEMENT
weapon_collision (ELEMENT *WeaponElementPtr, POINT *pWPt,
		ELEMENT *HitElementPtr, POINT *pHPt)
{
	SIZE damage;
	HELEMENT hBlastElement;

	if (WeaponElementPtr->state_flags & COLLISION) /* if already did effect */
		return ((HELEMENT)0);

	damage = (SIZE)WeaponElementPtr->mass_points;
	if (damage
			&& ((HitElementPtr->state_flags & FINITE_LIFE)
			|| HitElementPtr->life_span == NORMAL_LIFE))
#ifdef NEVER
			&&
			/* lasers from the same ship can't hit each other */
			(GetPrimType (&DisplayArray[HitElementPtr->PrimIndex]) != LINE_PRIM
			|| GetPrimType (&DisplayArray[WeaponElementPtr->PrimIndex]) != LINE_PRIM
			|| !elementsOfSamePlayer (HitElementPtr, WeaponElementPtr)))
#endif /* NEVER */
	{
		do_damage (HitElementPtr, damage);
		if (HitElementPtr->hit_points)
			WeaponElementPtr->state_flags |= COLLISION;
	}

	if (!(HitElementPtr->state_flags & FINITE_LIFE)
			|| (!(/* WeaponElementPtr->state_flags
			& */ HitElementPtr->state_flags & COLLISION)
			&& WeaponElementPtr->hit_points <= HitElementPtr->mass_points))
	{
		if (damage)
		{
			damage = TARGET_DAMAGED_FOR_1_PT + (damage >> 1);
			if (damage > TARGET_DAMAGED_FOR_6_PLUS_PT)
				damage = TARGET_DAMAGED_FOR_6_PLUS_PT;
			ProcessSound (SetAbsSoundIndex (GameSounds, damage),
					HitElementPtr);
		}

		if (GetPrimType (&DisplayArray[WeaponElementPtr->PrimIndex])
				!= LINE_PRIM)
			WeaponElementPtr->state_flags |= DISAPPEARING;

		WeaponElementPtr->hit_points = 0;
		WeaponElementPtr->life_span = 0;
		WeaponElementPtr->state_flags |= COLLISION | NONSOLID;

		hBlastElement = AllocElement ();
		if (hBlastElement)
		{
			COUNT blast_index;
			SIZE blast_offs;
			COUNT angle, num_blast_frames;
			ELEMENT *BlastElementPtr;
			extern FRAME blast[];

			PutElement (hBlastElement);
			LockElement (hBlastElement, &BlastElementPtr);
			BlastElementPtr->playerNr = WeaponElementPtr->playerNr;
			BlastElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			SetPrimType (&DisplayArray[BlastElementPtr->PrimIndex], STAMP_PRIM);

			BlastElementPtr->current.location.x = DISPLAY_TO_WORLD (pWPt->x);
			BlastElementPtr->current.location.y = DISPLAY_TO_WORLD (pWPt->y);

			angle = GetVelocityTravelAngle (&WeaponElementPtr->velocity);
			if ((blast_offs = WeaponElementPtr->blast_offset) > 0)
			{
				BlastElementPtr->current.location.x +=
						COSINE (angle, DISPLAY_TO_WORLD (blast_offs));
				BlastElementPtr->current.location.y +=
						SINE (angle, DISPLAY_TO_WORLD (blast_offs));
			}

			blast_index =
					NORMALIZE_FACING (ANGLE_TO_FACING (angle + HALF_CIRCLE));
			blast_index = ((blast_index >> 2) << 1) +
					(blast_index & 0x3 ? 1 : 0);

			num_blast_frames =
					GetFrameCount (WeaponElementPtr->next.image.frame);
			if (num_blast_frames <= ANGLE_TO_FACING (FULL_CIRCLE))
			{
				BlastElementPtr->life_span = 2;
				BlastElementPtr->current.image.farray = blast;
				BlastElementPtr->current.image.frame =
						SetAbsFrameIndex (blast[0], blast_index);
			}
			else
			{
				BlastElementPtr->life_span = num_blast_frames
						- ANGLE_TO_FACING (FULL_CIRCLE);
				BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
				BlastElementPtr->preprocess_func = animation_preprocess;
				BlastElementPtr->current.image.farray =
						WeaponElementPtr->next.image.farray;
				BlastElementPtr->current.image.frame =
						SetAbsFrameIndex (
						BlastElementPtr->current.image.farray[0],
						ANGLE_TO_FACING (FULL_CIRCLE));
			}

			UnlockElement (hBlastElement);

			return (hBlastElement);
		}
	}

	(void) pHPt;  /* Satisfying compiler (unused parameter) */
	return ((HELEMENT)0);
}

FRAME
ModifySilhouette (ELEMENT *ElementPtr, STAMP *modify_stamp,
		BYTE modify_flags)
{
	FRAME f;
	RECT r, or;
	INTERSECT_CONTROL ShipIntersect, ObjectIntersect;
	STARSHIP *StarShipPtr;
	CONTEXT OldContext;

	f = 0;
	ObjectIntersect.IntersectStamp = *modify_stamp;
	GetFrameRect (ObjectIntersect.IntersectStamp.frame, &or);

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (modify_flags & MODIFY_IMAGE)
	{
		ShipIntersect.IntersectStamp.frame = SetAbsFrameIndex (
				StarShipPtr->RaceDescPtr->ship_info.icons, 1);
		if (ShipIntersect.IntersectStamp.frame == 0)
			return (0);

		GetFrameRect (ShipIntersect.IntersectStamp.frame, &r);

		ShipIntersect.IntersectStamp.origin.x = 0;
		ShipIntersect.IntersectStamp.origin.y = 0;
		ShipIntersect.EndPoint = ShipIntersect.IntersectStamp.origin;
		do
		{
			ObjectIntersect.IntersectStamp.origin.x = ((COUNT)TFB_Random ()
					% (r.extent.width - or.extent.width))
					+ ((or.extent.width - r.extent.width) >> 1);
			ObjectIntersect.IntersectStamp.origin.y = ((COUNT)TFB_Random ()
					% (r.extent.height - or.extent.height))
					+ ((or.extent.height - r.extent.height) >> 1);
			ObjectIntersect.EndPoint = ObjectIntersect.IntersectStamp.origin;
		} while (!DrawablesIntersect (&ObjectIntersect,
				&ShipIntersect, MAX_TIME_VALUE));

		ObjectIntersect.IntersectStamp.origin.x += STATUS_WIDTH >> 1;
		ObjectIntersect.IntersectStamp.origin.y += RES_SCALE(31); // JMS_GFX
	}

	ObjectIntersect.IntersectStamp.origin.y +=
			status_y_offsets[ElementPtr->playerNr];

	if (modify_flags & MODIFY_SWAP)
	{
		or.corner.x += ObjectIntersect.IntersectStamp.origin.x;
		or.corner.y += ObjectIntersect.IntersectStamp.origin.y;
		InitShipStatus (&StarShipPtr->RaceDescPtr->ship_info,
				StarShipPtr, &or, FALSE);
	}
	else
	{
		OldContext = SetContext (StatusContext);
		DrawStamp (&ObjectIntersect.IntersectStamp);
		SetContext (OldContext);
	}

	return (f);
}

// Find the closest possible target ship, to be set in Tracker->hTarget.
// *pfacing will be turned one angle unit into the direction towards the
// target.
// The return value will be the actual number of angle units to turn, or
// -1 if no target was found.
// Cloaked ships won't be detected, except when the APPEARING flag is
// set for the Tracker.
SIZE
TrackShip (ELEMENT *Tracker, COUNT *pfacing)
{
	SIZE best_delta_facing, best_delta;
	HELEMENT hShip, hNextShip;
	ELEMENT *Trackee;

	best_delta = 0;
	best_delta_facing = -1;

	hShip = Tracker->hTarget;
	if (hShip)
	{
		LockElement (hShip, &Trackee);
		Tracker->hTarget = hNextShip = 0;

		goto CheckTracking;
	}

	for (hShip = GetHeadElement (); hShip != 0; hShip = hNextShip)
	{
		LockElement (hShip, &Trackee);
		hNextShip = GetSuccElement (Trackee);
		if ((Trackee->state_flags & PLAYER_SHIP)
				&& !elementsOfSamePlayer (Trackee, Tracker)
				&& (!OBJECT_CLOAKED (Trackee)
				|| ((Tracker->state_flags & PLAYER_SHIP)
				&& (Tracker->state_flags & APPEARING))
				))
		{
			STARSHIP *StarShipPtr;

CheckTracking:
			GetElementStarShip (Trackee, &StarShipPtr);
			if (Trackee->life_span
					&& StarShipPtr->RaceDescPtr->ship_info.crew_level)
			{
				SIZE delta_x, delta_y, delta_facing;

				if (Tracker->state_flags & PRE_PROCESS)
				{
					delta_x = Trackee->next.location.x
							- Tracker->next.location.x;
					delta_y = Trackee->next.location.y
							- Tracker->next.location.y;
				}
				else
				{
					delta_x = Trackee->current.location.x
							- Tracker->current.location.x;
					delta_y = Trackee->current.location.y
							- Tracker->current.location.y;
				}

				delta_x = WRAP_DELTA_X (delta_x);
				delta_y = WRAP_DELTA_Y (delta_y);
				delta_facing = NORMALIZE_FACING (
						ANGLE_TO_FACING (ARCTAN (delta_x, delta_y)) - *pfacing
						);

				if (delta_x < 0)
					delta_x = -delta_x;
				if (delta_y < 0)
					delta_y = -delta_y;
				delta_x += delta_y;
						// 'delta_x + delta_y' is used as an approximation
						// of the actual distance 'sqrt(sqr(delta_x) +
						// sqr(delta_y))'.

				if (best_delta == 0 || delta_x < best_delta)
				{
					best_delta = delta_x;
					best_delta_facing = delta_facing;
					Tracker->hTarget = hShip;
				}
			}
		}
		UnlockElement (hShip);
	}

	if (best_delta_facing > 0)
	{
		COUNT facing;

		facing = *pfacing;
		if (best_delta_facing == ANGLE_TO_FACING (HALF_CIRCLE))
			facing += (((BYTE)TFB_Random () & 1) << 1) - 1;
		else if (best_delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
			++facing;
		else
			--facing;
		*pfacing = NORMALIZE_FACING (facing);
	}

	return (best_delta_facing);
}

