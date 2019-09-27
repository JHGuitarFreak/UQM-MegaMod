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

#include "element.h"
#include "init.h"
#include "races.h"
#include "ship.h"
#include "status.h"
#include "setup.h"
#include "sounds.h"
#include "weapon.h"
#include "libs/mathlib.h"
#include "options.h" // For God Mode invincibility
#include "settings.h"
#include "intel.h"

void
spawn_planet (void)
{
	HELEMENT hPlanetElement;
	
	hPlanetElement = AllocElement ();
	if (hPlanetElement)
	{
		ELEMENT *PlanetElementPtr;
		extern FRAME planet[];

		LockElement (hPlanetElement, &PlanetElementPtr);
		PlanetElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		PlanetElementPtr->hit_points = 200;
		PlanetElementPtr->state_flags = APPEARING;
		PlanetElementPtr->life_span = NORMAL_LIFE + 1;
		SetPrimType (&DisplayArray[PlanetElementPtr->PrimIndex], STAMP_PRIM);
		PlanetElementPtr->current.image.farray = planet;
		PlanetElementPtr->current.image.frame =
				PlanetElementPtr->current.image.farray[0];
		PlanetElementPtr->collision_func = collision;
		PlanetElementPtr->postprocess_func =
				(void (*) (struct element *ElementPtr))CalculateGravity;
		ZeroVelocityComponents (&PlanetElementPtr->velocity);
		do
		{
			PlanetElementPtr->current.location.x =
					WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
			PlanetElementPtr->current.location.y =
					WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
		} while (CalculateGravity (PlanetElementPtr)
				|| TimeSpaceMatterConflict (PlanetElementPtr));
		PlanetElementPtr->mass_points = PlanetElementPtr->hit_points;
		UnlockElement (hPlanetElement);

		PutElement (hPlanetElement);
	}
}

extern FRAME asteroid[];

static void
spawn_rubble (ELEMENT *AsteroidElementPtr)
{
	HELEMENT hRubbleElement;
	
	hRubbleElement = AllocElement ();
	if (hRubbleElement)
	{
		ELEMENT *RubbleElementPtr;

		PutElement (hRubbleElement);
		LockElement (hRubbleElement, &RubbleElementPtr);
		RubbleElementPtr->playerNr = AsteroidElementPtr->playerNr;
		RubbleElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		RubbleElementPtr->life_span = 5;
		RubbleElementPtr->turn_wait = RubbleElementPtr->next_turn = 0;
		SetPrimType (&DisplayArray[RubbleElementPtr->PrimIndex], STAMP_PRIM);
		RubbleElementPtr->current.image.farray = asteroid;
		
		// JMS_GFX
		if (!IS_HD)
			RubbleElementPtr->current.image.frame = SetAbsFrameIndex (asteroid[0], ANGLE_TO_FACING (FULL_CIRCLE));
		else
			RubbleElementPtr->current.image.frame = SetAbsFrameIndex (asteroid[0], 29);

		RubbleElementPtr->current.location = AsteroidElementPtr->current.location;
		RubbleElementPtr->preprocess_func = animation_preprocess;
		RubbleElementPtr->death_func = spawn_asteroid;
		UnlockElement (hRubbleElement);
	}
}

static void
asteroid_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT frame_index;

		frame_index = GetFrameIndex (ElementPtr->current.image.frame);
		if (ElementPtr->thrust_wait & (1 << 7))
			--frame_index;
		else
			++frame_index;
		
		// JMS_GFX
		if (!IS_HD)
			ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, NORMALIZE_FACING (frame_index));
		else
			ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, frame_index % 30);

		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = (unsigned char)(ElementPtr->thrust_wait & ((1 << 7) - 1));
	}
}

void
spawn_asteroid (ELEMENT *ElementPtr)
{
	HELEMENT hAsteroidElement;

	if ((hAsteroidElement = AllocElement ()) == 0)
	{
		if (ElementPtr != 0)
		{
			ElementPtr->state_flags &= ~DISAPPEARING;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->life_span = 1;
		}
	}
	else
	{
		ELEMENT *AsteroidElementPtr;
		COUNT val;

		LockElement (hAsteroidElement, &AsteroidElementPtr);
		AsteroidElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		AsteroidElementPtr->hit_points = 1;
		AsteroidElementPtr->mass_points = 3;
		AsteroidElementPtr->state_flags = APPEARING;
		AsteroidElementPtr->life_span = NORMAL_LIFE;
		SetPrimType (&DisplayArray[AsteroidElementPtr->PrimIndex], STAMP_PRIM);
		if ((val = (COUNT)TFB_Random ()) & (1 << 0))
		{
			if (!(val & (1 << 1)))
				AsteroidElementPtr->current.location.x = 0;
			else
				AsteroidElementPtr->current.location.x = LOG_SPACE_WIDTH;
			AsteroidElementPtr->current.location.y =
					WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
		}
		else
		{
			AsteroidElementPtr->current.location.x =
					WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
			if (!(val & (1 << 1)))
				AsteroidElementPtr->current.location.y = 0;
			else
				AsteroidElementPtr->current.location.y = LOG_SPACE_HEIGHT;
		}

		{
			// Using these temporary variables because the execution order
			// of function arguments may vary per system, which may break
			// synchronisation on network games.
			SIZE magnitude =
					DISPLAY_TO_WORLD (((SIZE)TFB_Random () & 7) + 4);
			COUNT facing = (COUNT)TFB_Random ();
			SetVelocityVector (&AsteroidElementPtr->velocity, magnitude,
					facing);
		}
		AsteroidElementPtr->current.image.farray = asteroid;
		AsteroidElementPtr->current.image.frame =
				SetAbsFrameIndex (asteroid[0],
				NORMALIZE_FACING (TFB_Random ()));
		AsteroidElementPtr->turn_wait =
				AsteroidElementPtr->thrust_wait =
				(BYTE)TFB_Random () & (BYTE)((1 << 2) - 1);
		AsteroidElementPtr->thrust_wait |=
				(BYTE)TFB_Random () & (BYTE)(1 << 7);
		AsteroidElementPtr->preprocess_func = asteroid_preprocess;
		AsteroidElementPtr->death_func = spawn_rubble;
		AsteroidElementPtr->collision_func = collision;
		UnlockElement (hAsteroidElement);

		PutElement (hAsteroidElement);
	}
}

void
do_damage (ELEMENT *ElementPtr, SIZE damage)
{
	// God Mode, borrowed from the UQM-HD debug invincibility code
	if (antiCheat(ElementPtr, FALSE)) {
		damage = 0;
	}
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		if (!DeltaCrew (ElementPtr, -damage))
		{
			ElementPtr->life_span = 0;
			ElementPtr->state_flags |= NONSOLID;
		}
	}
	else if (!GRAVITY_MASS (ElementPtr->mass_points))
	{
		if ((BYTE)damage < ElementPtr->hit_points)
			ElementPtr->hit_points -= (BYTE)damage;
		else
		{
			ElementPtr->hit_points = 0;
			ElementPtr->life_span = 0;
			ElementPtr->state_flags |= NONSOLID;
		}
	}
}

#define CREW_COLOR_LOW_INTENSITY \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02)
#define CREW_COLOR_HIGH_INTENSITY \
		BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1E, 0x0A), 0x0A)
void
crew_preprocess (ELEMENT *ElementPtr)
{
	HELEMENT hTarget;

	// Switch from dark to light or vice versa:
	Color oldColor = GetPrimColor (&DisplayArray[ElementPtr->PrimIndex]);
	Color newColor = sameColor (oldColor, CREW_COLOR_LOW_INTENSITY) ?
			CREW_COLOR_HIGH_INTENSITY : CREW_COLOR_LOW_INTENSITY;
	SetPrimColor (&DisplayArray[ElementPtr->PrimIndex], newColor);

	ElementPtr->state_flags |= CHANGING;

	hTarget = ElementPtr->hTarget;
	if (hTarget == 0)
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if (StarShipPtr && StarShipPtr->RaceDescPtr->ship_info.crew_level)
			ElementPtr->hTarget = StarShipPtr->hShip;
		else
		{
			COUNT facing;

			facing = 0;
			TrackShip (ElementPtr, &facing);
		}
	}

	if (hTarget)
	{
#define CREW_DELTA RES_SCALE(SCALED_ONE)
		SIZE delta;
		ELEMENT *ShipPtr;

		LockElement (hTarget, &ShipPtr);
		delta = ShipPtr->current.location.x
				- ElementPtr->current.location.x;
		delta = WRAP_DELTA_X (delta);
		if (delta > 0)
			ElementPtr->next.location.x += CREW_DELTA;
		else if (delta < 0)
			ElementPtr->next.location.x -= CREW_DELTA;

		delta = ShipPtr->current.location.y -
				ElementPtr->current.location.y;
		delta = WRAP_DELTA_Y (delta);
		if (delta > 0)
			ElementPtr->next.location.y += CREW_DELTA;
		else if (delta < 0)
			ElementPtr->next.location.y -= CREW_DELTA;
		UnlockElement (hTarget);
	}
}

void
crew_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& ElementPtr1->life_span >= NORMAL_LIFE
			&& ElementPtr0->hit_points > 0)
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		if (!(StarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE))
		{
			ProcessSound (SetAbsSoundIndex (GameSounds, GRAB_CREW), ElementPtr1);
			DeltaCrew (ElementPtr1, 1);
		}
	}

	ElementPtr0->hit_points = 0;
	ElementPtr0->life_span = 0;
	ElementPtr0->state_flags |= COLLISION | DISAPPEARING | NONSOLID;
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

void
AbandonShip (ELEMENT *ShipPtr, ELEMENT *TargetPtr,
		COUNT crew_loss)
{
	SIZE dx, dy;
	COUNT direction;
	RECT r;
	STARSHIP *StarShipPtr;
	HELEMENT hCrew;
	INTERSECT_CONTROL ShipIntersect;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE)
		return;

	ShipIntersect = ShipPtr->IntersectControl;
	GetFrameRect (ShipIntersect.IntersectStamp.frame, &r);

	if ((direction = GetVelocityTravelAngle (
			&ShipPtr->velocity)) == FULL_CIRCLE)
		dx = dy = 0;
	else
	{
#define MORE_THAN_ENOUGH RES_SCALE(100) // JMS_GFX
		direction += HALF_CIRCLE;
		dx = COSINE (direction, MORE_THAN_ENOUGH);
		dy = SINE (direction, MORE_THAN_ENOUGH);
	}

	while (crew_loss-- && (hCrew = AllocElement ()))
	{
#define CREW_LIFE 300
		ELEMENT *CrewPtr;

		DeltaCrew (ShipPtr, -1);

		PutElement (hCrew);
		LockElement (hCrew, &CrewPtr);
		CrewPtr->playerNr = NEUTRAL_PLAYER_NUM;
		CrewPtr->hit_points = 1;
		CrewPtr->state_flags = APPEARING | FINITE_LIFE | CREW_OBJECT;
		CrewPtr->life_span = CREW_LIFE;
		if (!IS_HD) {
			SetPrimType (&DisplayArray[CrewPtr->PrimIndex], POINT_PRIM);
			CrewPtr->current.image.frame = DecFrameIndex (stars_in_space);
			CrewPtr->current.image.farray = &stars_in_space;
		} else {
			SetPrimType (&DisplayArray[CrewPtr->PrimIndex], STAMPFILL_PRIM);
			CrewPtr->current.image.frame = SetAbsFrameIndex (crew_dots[0], 0);
			CrewPtr->current.image.farray = crew_dots;
		}
		SetPrimColor (&DisplayArray[CrewPtr->PrimIndex],
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02));
		/*CrewPtr->current.image.frame = DecFrameIndex (stars_in_space);
		CrewPtr->current.image.farray = &stars_in_space;*/
		CrewPtr->preprocess_func = crew_preprocess;
		CrewPtr->collision_func = crew_collision;

		SetElementStarShip (CrewPtr, StarShipPtr);

		GetElementStarShip (TargetPtr, &StarShipPtr);
		CrewPtr->hTarget = StarShipPtr->hShip;

		{
			SIZE w, h;
			INTERSECT_CONTROL CrewIntersect;

			ShipIntersect.IntersectStamp.origin =
					ShipPtr->IntersectControl.EndPoint;

			w = (SIZE)((COUNT)TFB_Random () % r.extent.width);
			h = (SIZE)((COUNT)TFB_Random () % r.extent.height);
			CrewIntersect.EndPoint = ShipIntersect.EndPoint;
			CrewIntersect.IntersectStamp.frame = DecFrameIndex (stars_in_space);
			if (dx == 0 && dy == 0)
			{
				CrewIntersect.EndPoint.x += w - (r.extent.width >> 1);
				CrewIntersect.EndPoint.y += h - (r.extent.height >> 1);
				CrewIntersect.IntersectStamp.origin =
						TargetPtr->IntersectControl.EndPoint;
			}
			else
			{
				if (dx == 0)
					CrewIntersect.EndPoint.x += w - (r.extent.width >> 1);
				else if (dx > 0)
					CrewIntersect.EndPoint.x += w;
				else
					CrewIntersect.EndPoint.x -= w;
				if (dy == 0)
					CrewIntersect.EndPoint.y += h - (r.extent.height >> 1);
				else if (dy > 0)
					CrewIntersect.EndPoint.y += h;
				else
					CrewIntersect.EndPoint.y -= h;
				CrewIntersect.IntersectStamp.origin.x =
						CrewIntersect.EndPoint.x + dx;
				CrewIntersect.IntersectStamp.origin.y =
						CrewIntersect.EndPoint.y + dy;
			}

			DrawablesIntersect (&CrewIntersect,
					&ShipIntersect, MAX_TIME_VALUE);

			CrewPtr->current.location.x =
					DISPLAY_TO_WORLD (CrewIntersect.EndPoint.x);
			CrewPtr->current.location.y =
					DISPLAY_TO_WORLD (CrewIntersect.EndPoint.y);
		}
		UnlockElement (hCrew);
	}
}

