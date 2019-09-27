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

#include "colors.h"
#include "collide.h"
#include "element.h"
#include "ship.h"
#include "globdata.h"
#include "intel.h"
#include "setup.h"
#include "units.h"
#include "libs/mathlib.h"
#include "libs/log.h"

//#define DEBUG_CYBORG

COUNT
PlotIntercept (ELEMENT *ElementPtr0, ELEMENT *ElementPtr1,
			   COUNT max_turns, COUNT margin_of_error)
{
	SDWORD dy;
	SDWORD time_y_0, time_y_1;
	DPOINT dst[2];
	RECT r0 = {{0, 0}, {0, 0}};
	RECT r1 = {{0, 0}, {0, 0}};
	SDWORD dx_0, dy_0, dx_1, dy_1; // JMS:These were SIZE. No overflows now.
	
	if ((ElementPtr0->state_flags | ElementPtr1->state_flags) & FINITE_LIFE)
	{
		//log_add (log_Debug, "E0:%d, E1:%d, max:%d",ElementPtr0->life_span,ElementPtr1->life_span, max_turns);
		
		if (!(ElementPtr0->state_flags & FINITE_LIFE))
		{
			if (ElementPtr1->life_span < max_turns)
				max_turns = ElementPtr1->life_span;
		}
		else if (!(ElementPtr1->state_flags & FINITE_LIFE))
		{
			if (ElementPtr0->life_span < max_turns)
				max_turns = ElementPtr0->life_span;
		}
		else
		{
			if (ElementPtr0->life_span < max_turns)
				max_turns = ElementPtr0->life_span;
			if (ElementPtr1->life_span < max_turns)
				max_turns = ElementPtr1->life_span;
		}
	}
	
	dst[0].x = (SDWORD)ElementPtr0->current.location.x;
	dst[0].y = (SDWORD)ElementPtr0->current.location.y;
	GetCurrentVelocityComponentsSdword (&ElementPtr0->velocity, &dx_0, &dy_0);
	dx_0 = (SDWORD)VELOCITY_TO_WORLD ((long)dx_0 * (long)max_turns);
	dy_0 = (SDWORD)VELOCITY_TO_WORLD ((long)dy_0 * (long)max_turns);
	
	dst[1].x = (SDWORD)ElementPtr1->current.location.x;
	dst[1].y = (SDWORD)ElementPtr1->current.location.y;
	GetCurrentVelocityComponentsSdword (&ElementPtr1->velocity, &dx_1, &dy_1);
	dx_1 = (SDWORD)VELOCITY_TO_WORLD ((long)dx_1 * (long)max_turns);
	dy_1 = (SDWORD)VELOCITY_TO_WORLD ((long)dy_1 * (long)max_turns);
	
	if (margin_of_error)
	{
		dst[1].y -= margin_of_error;
		time_y_0 = 1;
		time_y_1 = margin_of_error << 1;
	}
	else
	{
		GetFrameRect (ElementPtr0->IntersectControl.IntersectStamp.frame, &r0);
		GetFrameRect (ElementPtr1->IntersectControl.IntersectStamp.frame, &r1);
		
		dst[0].y += DISPLAY_TO_WORLD (r0.corner.y);
		dst[1].y += DISPLAY_TO_WORLD (r1.corner.y);
		time_y_0 = DISPLAY_TO_WORLD (r0.extent.height);
		time_y_1 = DISPLAY_TO_WORLD (r1.extent.height);
	}
	
	dy = dst[1].y - dst[0].y;
	time_y_0 = dy - time_y_0 + 1;
	time_y_1 = dy + time_y_1 - 1;
	dy = dy_0 - dy_1;
	
	if ((time_y_0 <= 0 && time_y_1 >= 0)
		|| (time_y_0 > 0 && dy >= time_y_0)
		|| (time_y_1 < 0 && dy <= time_y_1))
	{
		SDWORD dx;
		SDWORD time_x_0, time_x_1;
		
		if (margin_of_error)
		{
			dst[1].x -= margin_of_error;
			time_x_0 = 1;
			time_x_1 = margin_of_error << 1;
		}
		else
		{
			dst[0].x += DISPLAY_TO_WORLD (r0.corner.x);
			dst[1].x += DISPLAY_TO_WORLD (r1.corner.x);
			time_x_0 = DISPLAY_TO_WORLD (r0.extent.width);
			time_x_1 = DISPLAY_TO_WORLD (r1.extent.width);
		}
		
		dx = dst[1].x - dst[0].x;
		time_x_0 = dx - time_x_0 + 1;
		time_x_1 = dx + time_x_1 - 1;
		dx = dx_0 - dx_1;
		
		if ((time_x_0 <= 0 && time_x_1 >= 0)
			|| (time_x_0 > 0 && dx >= time_x_0)
			|| (time_x_1 < 0 && dx <= time_x_1))
		{
			if (dx == 0 && dy == 0)
				time_y_0 = time_y_1 = 0;
			else
			{
				SDWORD t;
				long time_beg, time_end, fract;
				
				if (time_y_1 < 0)
				{
					t = time_y_0;
					time_y_0 = -time_y_1;
					time_y_1 = -t;
				}
				else if (time_y_0 <= 0)
				{
					if (dy < 0)
						time_y_1 = -time_y_0;
					time_y_0 = 0;
				}
				if (dy < 0)
					dy = -dy;
				if (dy < time_y_1)
					time_y_1 = dy;
				
				if (time_x_1 < 0)
				{
					t = time_x_0;
					time_x_0 = -time_x_1;
					time_x_1 = -t;
				}
				else if (time_x_0 <= 0)
				{
					if (dx < 0)
						time_x_1 = -time_x_0;
					time_x_0 = 0;
				}
				if (dx < 0)
					dx = -dx;
				if (dx < time_x_1)
					time_x_1 = dx;
				
				if (dx == 0)
				{
					time_beg = time_y_0;
					time_end = time_y_1;
					fract = dy;
				}
				else if (dy == 0)
				{
					time_beg = time_x_0;
					time_end = time_x_1;
					fract = dx;
				}
				else
				{
					long time_x, time_y;
					
					time_x = (long)time_x_0 * (long)dy;
					time_y = (long)time_y_0 * (long)dx;
					time_beg = time_x < time_y ? time_y : time_x;
					
					time_x = (long)time_x_1 * (long)dy;
					time_y = (long)time_y_1 * (long)dx;
					time_end = time_x > time_y ? time_y : time_x;
					
					fract = (long)dx * (long)dy;
				}
				
				if ((time_beg *= max_turns) < fract)
					time_y_0 = 0;
				else
					time_y_0 = (SDWORD)(time_beg / fract);
				
				if (time_end >= fract) /* just in case of overflow */
					time_y_1 = max_turns - 1;
				else
					time_y_1 = (SDWORD)((time_end * max_turns) / fract);
			}
			
			if (time_y_0 <= time_y_1)
			{
				if (margin_of_error != 0)
					return ((COUNT)time_y_0 + 1);
				else
				{
					DPOINT Pt0, Pt1;
					VELOCITY_DESC Velocity0, Velocity1;
					INTERSECT_CONTROL Control0, Control1;
					
					Pt0.x = (SDWORD)ElementPtr0->current.location.x;
					Pt0.y = (SDWORD)ElementPtr0->current.location.y;
					Velocity0 = ElementPtr0->velocity;
					Control0 = ElementPtr0->IntersectControl;
					
					Pt1.x = (SDWORD)ElementPtr1->current.location.x;
					Pt1.y = (SDWORD)ElementPtr1->current.location.y;
					Velocity1 = ElementPtr1->velocity;
					Control1 = ElementPtr1->IntersectControl;
					
					if (time_y_0)
					{
						GetNextVelocityComponentsSdword (&Velocity0, &dx_0, &dy_0, time_y_0);
						Pt0.x += dx_0;
						Pt0.y += dy_0;
						Control0.EndPoint.x = WORLD_TO_DISPLAY (Pt0.x);
						Control0.EndPoint.y = WORLD_TO_DISPLAY (Pt0.y);
						
						GetNextVelocityComponentsSdword (&Velocity1, &dx_1, &dy_1, time_y_0);
						Pt1.x += dx_1;
						Pt1.y += dy_1;
						Control1.EndPoint.x = WORLD_TO_DISPLAY (Pt1.x);
						Control1.EndPoint.y = WORLD_TO_DISPLAY (Pt1.y);
					}
					
					do
					{
						TIME_VALUE when;
						
						++time_y_0;
						
						GetNextVelocityComponentsSdword (&Velocity0, &dx_0, &dy_0, 1);
						Pt0.x += dx_0;
						Pt0.y += dy_0;
						
						GetNextVelocityComponentsSdword (&Velocity1, &dx_1, &dy_1, 1);
						Pt1.x += dx_1;
						Pt1.y += dy_1;
						
						Control0.IntersectStamp.origin = Control0.EndPoint;
						Control0.EndPoint.x = WORLD_TO_DISPLAY (Pt0.x);
						Control0.EndPoint.y = WORLD_TO_DISPLAY (Pt0.y);
						
						Control1.IntersectStamp.origin = Control1.EndPoint;
						Control1.EndPoint.x = WORLD_TO_DISPLAY (Pt1.x);
						Control1.EndPoint.y = WORLD_TO_DISPLAY (Pt1.y);
						
						when = DrawablesIntersect (&Control0, &Control1, MAX_TIME_VALUE);
						
						if (when)
						{
							if (when == 1
								&& time_y_0 == 1
								&& ((ElementPtr0->state_flags
									 | ElementPtr1->state_flags) & APPEARING))
							{
								when = 0;
								Control0.EndPoint.x = WORLD_TO_DISPLAY (Pt0.x);
								Control0.EndPoint.y = WORLD_TO_DISPLAY (Pt0.y);
								
								Control1.EndPoint.x = WORLD_TO_DISPLAY (Pt1.x);
								Control1.EndPoint.y = WORLD_TO_DISPLAY (Pt1.y);
							}
							
							if (when)
								return ((COUNT)time_y_0);
						}
					} while (time_y_0 < time_y_1);
				}
			}
		}
	}
	
	return (0);
}

static void
InitCyborg (STARSHIP *StarShipPtr)
{
	COUNT Index, Divisor;
	
	Index = StarShipPtr->RaceDescPtr->characteristics.max_thrust
	* StarShipPtr->RaceDescPtr->characteristics.thrust_increment;
	if ((Divisor = StarShipPtr->RaceDescPtr->characteristics.turn_wait
		 + StarShipPtr->RaceDescPtr->characteristics.thrust_wait) > 0)
		Index /= Divisor;
	else
		Index >>= 1;
#ifdef PRINT_MI
	{
		char *shipName;
		
		shipName = GetStringAddress (
									 StarShipPtr->RaceDescPtr->ship_data.race_strings);
		log_add (log_Debug, "MI(%s) -- <%u:%u> = %u", shipName,
				 StarShipPtr->RaceDescPtr->characteristics.max_thrust *
				 StarShipPtr->RaceDescPtr->characteristics.thrust_increment,
				 Divisor, Index);
	}
#endif /* PRINT_MI */
	StarShipPtr->RaceDescPtr->cyborg_control.ManeuverabilityIndex = Index;
}

static void
ship_movement (ELEMENT *ShipPtr, EVALUATE_DESC *EvalDescPtr)
{
	if (EvalDescPtr->which_turn == 0)
		EvalDescPtr->which_turn = 1;
	
	switch (EvalDescPtr->MoveState)
	{
		case PURSUE:
			Pursue (ShipPtr, EvalDescPtr);
			break;
		case AVOID:
#ifdef NOTYET
			Avoid (ShipPtr, EvalDescPtr);
			break;
#endif /* NOTYET */
		case ENTICE:
			Entice (ShipPtr, EvalDescPtr);
			break;
		case NO_MOVEMENT:
			break;
	}
}

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
BOOLEAN
ship_weapons (ELEMENT *ShipPtr, ELEMENT *OtherPtr, COUNT margin_of_error)
{
	SDWORD delta_x, delta_y;
	COUNT n, num_weapons;
	ELEMENT Ship;
	HELEMENT Weapon[6];
	STARSHIP *StarShipPtr;
	
	if (OBJECT_CLOAKED (OtherPtr))
		margin_of_error += DISPLAY_TO_WORLD (RES_SCALE(40)); // JMS_GFX
	
	Ship = *ShipPtr;
	GetNextVelocityComponentsSdword (&Ship.velocity, &delta_x, &delta_y, 1);
	Ship.next.location.x =
		Ship.current.location.x + delta_x;
	Ship.next.location.y =
		Ship.current.location.y + delta_y;
	
	Ship.current.location = Ship.next.location;
	
	GetElementStarShip (&Ship, &StarShipPtr);
	num_weapons =
	(*StarShipPtr->RaceDescPtr->init_weapon_func) (&Ship, Weapon);
	
	if ((n = num_weapons))
	{
		HELEMENT *WeaponPtr, w;
		//STARSHIP *StarShipPtr;
		ELEMENT *EPtr;
		
		WeaponPtr = &Weapon[0];
		do
		{
			w = *WeaponPtr;
			if (w)
			{
				LockElement (w, &EPtr);
				if (EPtr->state_flags & APPEARING)
				{
					EPtr->next = EPtr->current;
					InitIntersectStartPoint (EPtr);
					InitIntersectEndPoint (EPtr);
					InitIntersectFrame (EPtr);
				}
				
				if (PlotIntercept (EPtr, OtherPtr, EPtr->life_span, margin_of_error))
				{
					UnlockElement (w);
					break;
				}
				
				UnlockElement (w);
				FreeElement (w);
			}
			++WeaponPtr;
		} while (--n);
		
		if ((num_weapons = n))
		{
			do
			{
				w = *WeaponPtr++;
				if (w)
					FreeElement (w);
			} while (--n);
		}
	}
	
	//if (num_weapons > 0)
	//	log_add (log_Debug, "dx:%d, dy:%d, currx:%d, curry:%d, nextx:%d, nexty:%d", delta_x, delta_y, Ship.current.location.x, Ship.current.location.y, Ship.next.location.x, Ship.next.location.y);
	
	return (num_weapons > 0);
}

void
ship_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
				   COUNT ConcernCounter)
{
	BOOLEAN ShipMoved, ShipFired;
	COUNT margin_of_error;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *ObjectsOfConcernEWeapon;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	
	ShipMoved = TRUE;
	if (ShipPtr->turn_wait == 0)
		ShipMoved = FALSE;
	if (ShipPtr->thrust_wait == 0)
		ShipMoved = FALSE;
	
	ShipFired = TRUE;
	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		if (!(StarShipPtr->RaceDescPtr->ship_info.ship_flags & SEEKING_WEAPON))
			ShipFired = FALSE;
	}
	
	if (StarShipPtr->control & AWESOME_RATING)
		margin_of_error = 0;
	else if (StarShipPtr->control & GOOD_RATING)
		margin_of_error = DISPLAY_TO_WORLD (RES_SCALE(20)); // JMS_GFX
	else /* if (StarShipPtr->control & STANDARD_RATING) */
		margin_of_error = DISPLAY_TO_WORLD (RES_SCALE(40)); // JMS_GFX
	
	ObjectsOfConcern += ConcernCounter;
	
	ObjectsOfConcernEWeapon = ObjectsOfConcern - ConcernCounter + ENEMY_WEAPON_INDEX;
	
	while (ConcernCounter--)
	{
		--ObjectsOfConcern;
		if (ObjectsOfConcern->ObjectPtr)
		{
			if (!ShipMoved
				&& (ConcernCounter != ENEMY_WEAPON_INDEX
					|| ObjectsOfConcern->MoveState == PURSUE
					|| (ObjectsOfConcern->ObjectPtr->state_flags & CREW_OBJECT)
					|| MANEUVERABILITY (
										&StarShipPtr->RaceDescPtr->cyborg_control
										) >= RESOLUTION_COMPENSATED(MEDIUM_SHIP) // JMS_GFX
					)
				)
			{
				ship_movement (ShipPtr, ObjectsOfConcern);
				ShipMoved = TRUE;
			}
			if (!ShipFired
				&& (ConcernCounter == ENEMY_SHIP_INDEX
					|| (ConcernCounter == ENEMY_WEAPON_INDEX
						&& ObjectsOfConcern->MoveState != AVOID
#ifdef NEVER
						&& !(StarShipPtr->control & STANDARD_RATING)
#endif /* NEVER */		
						)
					)
				)
			{
				ShipFired = ship_weapons (ShipPtr,ObjectsOfConcern->ObjectPtr, margin_of_error);
				
				if (ShipFired)
					StarShipPtr->ship_input_state |= WEAPON;
			}
		}
	}
}

BOOLEAN
TurnShip (ELEMENT *ShipPtr, COUNT angle)
{
	COUNT f, ship_delta_facing;
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	f = StarShipPtr->ShipFacing;
	ship_delta_facing = NORMALIZE_FACING (ANGLE_TO_FACING (angle) - f);
	if (ship_delta_facing)
	{
		if (ship_delta_facing == ANGLE_TO_FACING (HALF_CIRCLE))
			ship_delta_facing =
			NORMALIZE_FACING (ship_delta_facing +
							  (TFB_Random () & 1 ?
							   ANGLE_TO_FACING (OCTANT >> 1) :
							   -ANGLE_TO_FACING (OCTANT >> 1)));
		
		if (ship_delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
		{
			StarShipPtr->ship_input_state |= RIGHT;
			++f;
			ShipPtr->next.image.frame =
			IncFrameIndex (ShipPtr->current.image.frame);
		}
		else
		{
			StarShipPtr->ship_input_state |= LEFT;
			--f;
			ShipPtr->next.image.frame =
			DecFrameIndex (ShipPtr->current.image.frame);
		}
		
#ifdef NOTYET
		if (((StarShipPtr->ship_input_state & (LEFT | RIGHT))
			 ^ (StarShipPtr->cur_status_flags & (LEFT | RIGHT))) == (LEFT | RIGHT))
			StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
		else
#endif /* NOTYET */
		{
			StarShipPtr->ShipFacing = NORMALIZE_FACING (f);
			
			return (TRUE);
		}
	}
	
	return (FALSE);
}

BOOLEAN
ThrustShip (ELEMENT *ShipPtr, COUNT angle)
{
	BOOLEAN ShouldThrust;
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->ship_input_state & THRUST)
		ShouldThrust = TRUE;
	else if (NORMALIZE_FACING (ANGLE_TO_FACING (angle)
							   - ANGLE_TO_FACING (GetVelocityTravelAngle (&ShipPtr->velocity))) == 0
			 && (StarShipPtr->cur_status_flags
				 & (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
			 && !(StarShipPtr->cur_status_flags & SHIP_IN_GRAVITY_WELL))
		ShouldThrust = FALSE;
	else
	{
		SIZE ship_delta_facing;
		
		ship_delta_facing =
		NORMALIZE_FACING (ANGLE_TO_FACING (angle)
						  - StarShipPtr->ShipFacing + ANGLE_TO_FACING (QUADRANT));
		if (ship_delta_facing == ANGLE_TO_FACING (QUADRANT)
			|| ((StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED)
				&& ship_delta_facing <= ANGLE_TO_FACING (HALF_CIRCLE)))
			ShouldThrust = TRUE;
		else
			ShouldThrust = FALSE;
	}
	
	if (ShouldThrust)
	{
		inertial_thrust (ShipPtr);
		
		StarShipPtr->ship_input_state |= THRUST;
	}
	
	return (ShouldThrust);
}

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
void
Pursue (ELEMENT *ShipPtr, EVALUATE_DESC *EvalDescPtr)
{
	BYTE maneuver_state;
	COUNT desired_thrust_angle, desired_turn_angle;
	SDWORD delta_x, delta_y;
	SDWORD ship_delta_x, ship_delta_y;
	SDWORD other_delta_x, other_delta_y;
	ELEMENT *OtherObjPtr;
	VELOCITY_DESC ShipVelocity, OtherVelocity;
	COUNT distance_to_give_up_and_turn; // JMS
	
	ShipVelocity = ShipPtr->velocity;
	GetNextVelocityComponentsSdword (&ShipVelocity,
		&ship_delta_x, &ship_delta_y, EvalDescPtr->which_turn);
	ShipPtr->next.location.x =
		ShipPtr->current.location.x + ship_delta_x;
	ShipPtr->next.location.y =
		ShipPtr->current.location.y + ship_delta_y;
	
	OtherObjPtr = EvalDescPtr->ObjectPtr;
	
	OtherVelocity = OtherObjPtr->velocity;
	GetNextVelocityComponentsSdword (&OtherVelocity,
							   &other_delta_x, &other_delta_y, EvalDescPtr->which_turn);
	
	delta_x = (OtherObjPtr->current.location.x + other_delta_x)
		- ShipPtr->next.location.x;
	delta_y = (OtherObjPtr->current.location.y + other_delta_y)
		- ShipPtr->next.location.y;
	delta_x = WRAP_DELTA_X (delta_x);
	delta_y = WRAP_DELTA_Y (delta_y);
	desired_thrust_angle = ARCTAN (delta_x, delta_y);
	
	maneuver_state = 0;
	if (ShipPtr->turn_wait == 0)
		maneuver_state |= LEFT | RIGHT;
	if (ShipPtr->thrust_wait == 0
		&& ((OtherObjPtr->state_flags & PLAYER_SHIP)
			|| elementsOfSamePlayer (OtherObjPtr, ShipPtr)
			|| OtherObjPtr->preprocess_func == crew_preprocess
			)
		)
		maneuver_state |= THRUST;
	
	desired_turn_angle = NORMALIZE_ANGLE (desired_thrust_angle + HALF_CIRCLE);
	/* other player's ship */
	if ((OtherObjPtr->state_flags & PLAYER_SHIP)
		&& OtherObjPtr->mass_points <= MAX_SHIP_MASS)
	{
		STARSHIP *StarShipPtr;
		STARSHIP *EnemyStarShipPtr;
		
		GetElementStarShip (ShipPtr, &StarShipPtr);
		GetElementStarShip (OtherObjPtr, &EnemyStarShipPtr);
		if ((MANEUVERABILITY (
							  &StarShipPtr->RaceDescPtr->cyborg_control
							  ) >= RESOLUTION_COMPENSATED(FAST_SHIP) // JMS_GFX
			 && WEAPON_RANGE (&StarShipPtr->RaceDescPtr->cyborg_control)
			 > RES_SCALE(CLOSE_RANGE_WEAPON)) // JMS_GFX
			|| (EvalDescPtr->which_turn >= 24
				&& (StarShipPtr->RaceDescPtr->characteristics.max_thrust * 2 / 3 <
					EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust
					|| (EnemyStarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED))))
		{
			UWORD ship_flags;
			
			ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
			/* you're maneuverable */
			if (MANEUVERABILITY (
				&StarShipPtr->RaceDescPtr->cyborg_control
				) >= RESOLUTION_COMPENSATED(MEDIUM_SHIP)) // JMS_GFX
			{
				UWORD fire_flags;
				COUNT facing;
				
				for (fire_flags = FIRES_FORE, facing = EvalDescPtr->facing;
					 fire_flags <= FIRES_LEFT;
					 fire_flags <<= 1, facing += QUADRANT)
				{
					if
						(
						 /* he's dangerous in this direction */
						 (ship_flags & fire_flags)
						 /* he's facing direction you want to go */
						 && NORMALIZE_ANGLE (
							desired_turn_angle - facing + OCTANT) <= QUADRANT
						 && (
							 /* he's moving */
							 (other_delta_x != 0 || other_delta_y != 0)
							 &&
							 /* he's coasting backwards */
							 NORMALIZE_ANGLE (
								(GetVelocityTravelAngle (&OtherVelocity) + HALF_CIRCLE)
								- facing + (OCTANT + (OCTANT >> 1)))
							 <= ((OCTANT + (OCTANT >> 1)) << 1))
						 )
					{
						/* catch him on the back side */
						desired_thrust_angle = desired_turn_angle;
						break;
					}
				}
			}
			
			// This code prevents Kohr-Ah, Ur-Quan and ISD from turning around mid-chase while pursuing Earthling.
			if (StarShipPtr->SpeciesID == (KOHR_AH_ID | UR_QUAN_ID)
				&& EnemyStarShipPtr->SpeciesID == EARTHLING_ID 
				&& !(EnemyStarShipPtr->cur_status_flags & (SHIP_BEYOND_MAX_SPEED | SHIP_IN_GRAVITY_WELL)))
				distance_to_give_up_and_turn = 44;
			else
				distance_to_give_up_and_turn = 24;
			
			if (desired_thrust_angle != desired_turn_angle
				&& (other_delta_x || other_delta_y)
				&& EvalDescPtr->which_turn >= distance_to_give_up_and_turn
				&& NORMALIZE_ANGLE (desired_thrust_angle
									- GetVelocityTravelAngle (&OtherVelocity)
									+ OCTANT) <= QUADRANT
				&& ((NORMALIZE_ANGLE (
									  GetVelocityTravelAngle (&OtherVelocity)
									  - GetVelocityTravelAngle (&ShipVelocity)
									  + OCTANT) <= QUADRANT
					 && (((StarShipPtr->cur_status_flags & SHIP_AT_MAX_SPEED)
						  && !(StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED))
						 || (ship_flags & DONT_CHASE)))
					|| NORMALIZE_ANGLE (
										desired_turn_angle
										- FACING_TO_ANGLE (StarShipPtr->ShipFacing)
										+ OCTANT) <= QUADRANT))
				desired_thrust_angle = desired_turn_angle;
		}
	}
	
	if (maneuver_state & (LEFT | RIGHT))
		TurnShip (ShipPtr, desired_thrust_angle);
	
	if (maneuver_state & THRUST)
		ThrustShip (ShipPtr, desired_thrust_angle);
}

// JMS:GFX Made SIZEs SDWORDs and changed the GetNextVelocityComponents to GetNextVelocityComponentsSdword
void
Entice (ELEMENT *ShipPtr, EVALUATE_DESC *EvalDescPtr)
{
	BYTE maneuver_state;
	COUNT desired_thrust_angle, desired_turn_angle;
	COUNT cone_of_fire, travel_angle;
	SDWORD delta_x, delta_y;
	SDWORD ship_delta_x, ship_delta_y;
	SDWORD other_delta_x, other_delta_y;
	ELEMENT *OtherObjPtr;
	VELOCITY_DESC ShipVelocity, OtherVelocity;
	STARSHIP *StarShipPtr;
	RACE_DESC *RDPtr;
	
	ShipVelocity = ShipPtr->velocity;
	GetNextVelocityComponentsSdword (&ShipVelocity,
		&ship_delta_x, &ship_delta_y, EvalDescPtr->which_turn);
	ShipPtr->next.location.x =
		ShipPtr->current.location.x + ship_delta_x;
	ShipPtr->next.location.y =
		ShipPtr->current.location.y + ship_delta_y;
	
	OtherObjPtr = EvalDescPtr->ObjectPtr;
	OtherVelocity = OtherObjPtr->velocity;
	GetNextVelocityComponentsSdword (&OtherVelocity,
		&other_delta_x, &other_delta_y, EvalDescPtr->which_turn);
	
	delta_x = (OtherObjPtr->current.location.x + other_delta_x)
		- ShipPtr->next.location.x;
	delta_y = (OtherObjPtr->current.location.y + other_delta_y)
		- ShipPtr->next.location.y;
	delta_x = WRAP_DELTA_X (delta_x);
	delta_y = WRAP_DELTA_Y (delta_y);
	desired_thrust_angle = ARCTAN (delta_x, delta_y);
	
	maneuver_state = 0;
	if (ShipPtr->turn_wait == 0)
		maneuver_state |= LEFT | RIGHT;
	if (ShipPtr->thrust_wait == 0)
		maneuver_state |= THRUST;
	
	delta_x = ship_delta_x - other_delta_x;
	delta_y = ship_delta_y - other_delta_y;
	travel_angle = ARCTAN (delta_x, delta_y);
	desired_turn_angle = NORMALIZE_ANGLE (desired_thrust_angle + HALF_CIRCLE);
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	RDPtr = StarShipPtr->RaceDescPtr;
	if (EvalDescPtr->MoveState == AVOID)
	{
		desired_turn_angle =
		NORMALIZE_ANGLE (desired_turn_angle - EvalDescPtr->facing);
		
		if (NORMALIZE_FACING (ANGLE_TO_FACING (desired_turn_angle)))
		{
			if (desired_turn_angle <= HALF_CIRCLE)
				desired_thrust_angle = RIGHT;
			else /* if (desired_turn_angle > HALF_CIRCLE) */
				desired_thrust_angle = LEFT;
		}
		else
		{
			desired_turn_angle = NORMALIZE_ANGLE (
				FACING_TO_ANGLE (StarShipPtr->ShipFacing)
				- EvalDescPtr->facing);
			if ((desired_turn_angle & (HALF_CIRCLE - 1)) == 0)
				desired_thrust_angle = TFB_Random () & 1 ? RIGHT : LEFT;
			else
				desired_thrust_angle = desired_turn_angle < HALF_CIRCLE ? RIGHT : LEFT;
		}
		
		if (desired_thrust_angle == LEFT)
		{
#define FLANK_LEFT -QUADRANT
#define SHIP_LEFT -OCTANT
			desired_thrust_angle = EvalDescPtr->facing
			+ FLANK_LEFT - (SHIP_LEFT >> 1);
		}
		else
		{
#define FLANK_RIGHT QUADRANT
#define SHIP_RIGHT OCTANT
			desired_thrust_angle = EvalDescPtr->facing
			+ FLANK_RIGHT - (SHIP_RIGHT >> 1);
		}
		
		desired_thrust_angle = NORMALIZE_ANGLE (desired_thrust_angle);
	}
	else if (GRAVITY_MASS (OtherObjPtr->mass_points))
	{
		COUNT planet_facing;
		
		planet_facing = NORMALIZE_FACING (ANGLE_TO_FACING (desired_thrust_angle));
		cone_of_fire = NORMALIZE_FACING (planet_facing - StarShipPtr->ShipFacing
			+ ANGLE_TO_FACING (QUADRANT));
		
		if (RDPtr->characteristics.thrust_increment !=
			RDPtr->characteristics.max_thrust)
			maneuver_state &= ~THRUST;
		
		/* if not pointing towards planet */
		if (cone_of_fire > ANGLE_TO_FACING (QUADRANT << 1))
			desired_turn_angle = desired_thrust_angle;
		/* if pointing directly at planet */
		else if (cone_of_fire == ANGLE_TO_FACING (QUADRANT)
				 && NORMALIZE_FACING (ANGLE_TO_FACING (travel_angle)) != planet_facing)
			desired_turn_angle = travel_angle;
		else if (cone_of_fire == 0
				 || cone_of_fire == ANGLE_TO_FACING (QUADRANT << 1)
				 || (!(maneuver_state & THRUST)
					 && (cone_of_fire < ANGLE_TO_FACING (OCTANT)
						 || cone_of_fire > ANGLE_TO_FACING ((QUADRANT << 1) - OCTANT))))
		{
			desired_turn_angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			if (NORMALIZE_ANGLE (desired_turn_angle
								 - travel_angle + QUADRANT) > HALF_CIRCLE)
				desired_turn_angle = travel_angle;
			if (ShipPtr->thrust_wait == 0)
				maneuver_state |= THRUST;
		}
		
		desired_thrust_angle = desired_turn_angle;
	}
	else
	{
		COUNT WRange;
		
		WRange = WEAPON_RANGE (
							   &RDPtr->cyborg_control
							   );
		
		cone_of_fire = NORMALIZE_ANGLE (desired_turn_angle
										- EvalDescPtr->facing + OCTANT);
		if (OtherObjPtr->state_flags & PLAYER_SHIP)
		{
			UWORD fire_flags, ship_flags;
			COUNT facing;
			STARSHIP *EnemyStarShipPtr;
			
			GetElementStarShip (OtherObjPtr, &EnemyStarShipPtr);
			ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
			for (fire_flags = FIRES_FORE, facing = EvalDescPtr->facing;
				 fire_flags <= FIRES_LEFT;
				 fire_flags <<= 1, facing += QUADRANT)
			{
				if
					(
					 /* he's dangerous in this direction */
					 (ship_flags & fire_flags)
					 /* he's facing direction you want to go */
					 && (cone_of_fire = NORMALIZE_ANGLE (
						desired_turn_angle - facing + OCTANT)) <= QUADRANT
					 /* he's moving */
					 && ((other_delta_x != 0 || other_delta_y != 0)
						 /* he's coasting backwards */
						 && NORMALIZE_ANGLE (
											 (GetVelocityTravelAngle (&OtherVelocity) + HALF_CIRCLE)
											 - facing + OCTANT) <= QUADRANT)
					 )
				{
					/* need to be close for a kill */
					if (WRange < RES_SCALE(LONG_RANGE_WEAPON)
						&& EvalDescPtr->which_turn <= 32)
					{
						/* catch him on the back side */
						desired_thrust_angle = desired_turn_angle;
						goto DoManeuver;
					}
					
					break;
				}
			}
			
			if (EvalDescPtr->which_turn <= 8
				&& RDPtr->characteristics.max_thrust <=
				EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust)
				goto DoManeuver;
		}
		
		if
			(
#ifdef NOTYET
			 WRange < RES_SCALE(LONG_RANGE_WEAPON)
			 &&
#endif /* NOTYET */
			 /* not at full speed */
			 !(StarShipPtr->cur_status_flags
			   & (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
			 && (PlotIntercept (ShipPtr, OtherObjPtr, 40, RES_SCALE(CLOSE_RANGE_WEAPON) << 1) // JMS_GFX
#ifdef NOTYET
				 ||
				 (
				  /* object's facing direction you want to go */
				  cone_of_fire <= QUADRANT
				  /* and you're basically going in that direction */
				  && (travel_angle == FULL_CIRCLE
					  || NORMALIZE_ANGLE (travel_angle
										  - desired_thrust_angle + QUADRANT) <= HALF_CIRCLE)
				  /* and object's in range */
				  && PlotIntercept (ShipPtr, OtherObjPtr, 1, WRange)
				  )
#endif /* NOTYET */
				 )
			 )
		{
			if
				(
				 /* pointed straight at him */
				 NORMALIZE_ANGLE (desired_thrust_angle
								  - FACING_TO_ANGLE (StarShipPtr->ShipFacing) + OCTANT) <= QUADRANT
				 /* or not exposed to business end */
				 || cone_of_fire > QUADRANT
				 )
			{
				desired_thrust_angle = desired_turn_angle;
			}
			else
			{
#ifdef NOTYET
				if
					(
					 travel_angle != FULL_CIRCLE
					 && NORMALIZE_ANGLE (travel_angle
										 - desired_turn_angle + OCTANT) <= QUADRANT
					 )
				{
					desired_turn_angle =
					NORMALIZE_ANGLE ((EvalDescPtr->facing + HALF_CIRCLE)
									 + (travel_angle - desired_turn_angle));
					if (!(maneuver_state & (LEFT | RIGHT)))
						maneuver_state &= ~THRUST;
				}
				
				if (maneuver_state & (LEFT | RIGHT))
				{
					TurnShip (ShipPtr, desired_turn_angle);
					maneuver_state &= ~(LEFT | RIGHT);
				}
#endif /* NOTYET */
				
				desired_thrust_angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
				desired_turn_angle = desired_thrust_angle;
			}
		}
		else if ((cone_of_fire = PlotIntercept (
												ShipPtr, OtherObjPtr, 10, WRange
#ifdef OLD
												- (WRange >> 3)
#else /* !OLD */
												- (WRange >> 2)
#endif /* OLD */
												)))
		{
			if (RDPtr->characteristics.thrust_increment !=
				RDPtr->characteristics.max_thrust
				/* and already at full speed */
				&& (StarShipPtr->cur_status_flags
					& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
				/* and facing away from enemy */
				&& (NORMALIZE_ANGLE (desired_turn_angle
									 - ARCTAN (ship_delta_x, ship_delta_y)
									 + (OCTANT + 2)) <= ((OCTANT + 2) << 1)
					/* or not on collision course */
					|| !PlotIntercept (ShipPtr, OtherObjPtr, 30, RES_SCALE(CLOSE_RANGE_WEAPON) << 1))) // JMS_GFX
				maneuver_state &= ~THRUST;
			/* veer off */
			else if (cone_of_fire == 1
					 || RDPtr->characteristics.thrust_increment !=
					 RDPtr->characteristics.max_thrust)
			{
				if (maneuver_state & (LEFT | RIGHT))
				{
					TurnShip (ShipPtr, desired_turn_angle);
					maneuver_state &= ~(LEFT | RIGHT);
				}
				
				if (NORMALIZE_ANGLE (desired_thrust_angle
									 - ARCTAN (ship_delta_x, ship_delta_y)
									 + (OCTANT + 2)) <= ((OCTANT + 2) << 1))
					desired_thrust_angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
				else
					desired_thrust_angle = desired_turn_angle;
			}
		}
	}
	
DoManeuver:
	if (maneuver_state & (LEFT | RIGHT))
		TurnShip (ShipPtr, desired_thrust_angle);
	
	if (maneuver_state & THRUST)
		ThrustShip (ShipPtr, desired_thrust_angle);
}

void
Avoid (ELEMENT *ShipPtr, EVALUATE_DESC *EvalDescPtr)
{
	(void) ShipPtr;  /* Satisfying compiler (unused parameter) */
	(void) EvalDescPtr;  /* Satisfying compiler (unused parameter) */
}

BATTLE_INPUT_STATE
tactical_intelligence (ComputerInputContext *context, STARSHIP *StarShipPtr)
{
	ELEMENT *ShipPtr;
	ELEMENT Ship;
	COUNT ShipFacing;
	HELEMENT hElement, hNextElement;
	COUNT ConcernCounter;
	EVALUATE_DESC ObjectsOfConcern[10];
	BOOLEAN ShipMoved, UltraManeuverable;
	STARSHIP *EnemyStarShipPtr;
	RACE_DESC *RDPtr;
	RACE_DESC *EnemyRDPtr;
	
	RDPtr = StarShipPtr->RaceDescPtr;
	
	if (RDPtr->cyborg_control.ManeuverabilityIndex == 0)
		InitCyborg (StarShipPtr);
	
	LockElement (StarShipPtr->hShip, &ShipPtr);
	if (RDPtr->ship_info.crew_level == 0
		|| GetPrimType (&DisplayArray[ShipPtr->PrimIndex]) == NO_PRIM)
	{
		UnlockElement (StarShipPtr->hShip);
		return (0);
	}
	
	ShipMoved = TRUE;
	/* Disable ship's special completely for the Standard AI */
	if (StarShipPtr->control & STANDARD_RATING)
		++StarShipPtr->special_counter;
	
#ifdef DEBUG_CYBORG
	if (!(ShipPtr->state_flags & FINITE_LIFE)
		&& ShipPtr->life_span == NORMAL_LIFE)
		ShipPtr->life_span += 2; /* make ship invulnerable */
#endif /* DEBUG_CYBORG */
	Ship = *ShipPtr;
	UnlockElement (StarShipPtr->hShip);
	ShipFacing = StarShipPtr->ShipFacing;
	
	for (ConcernCounter = 0;
		 ConcernCounter <= FIRST_EMPTY_INDEX; ++ConcernCounter)
	{
		ObjectsOfConcern[ConcernCounter].ObjectPtr = 0;
		ObjectsOfConcern[ConcernCounter].MoveState = NO_MOVEMENT;
		ObjectsOfConcern[ConcernCounter].which_turn = (COUNT)~0;
	}
	--ConcernCounter;
	
	UltraManeuverable = (BOOLEAN)(
		RDPtr->characteristics.thrust_increment == RDPtr->characteristics.max_thrust
		&& MANEUVERABILITY (&RDPtr->cyborg_control) >= RESOLUTION_COMPENSATED(MEDIUM_SHIP) // JMS_GFX
		);
	
	if (Ship.turn_wait == 0)
	{
		ShipMoved = FALSE;
		StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);
	}
	if (Ship.thrust_wait == 0)
	{
		ShipMoved = FALSE;
		StarShipPtr->ship_input_state &= ~THRUST;
	}
	
	for (hElement = GetHeadElement ();
		 hElement != 0; hElement = hNextElement)
	{
		EVALUATE_DESC ed;
		
		ed.MoveState = NO_MOVEMENT;
		
		LockElement (hElement, &ed.ObjectPtr);
		hNextElement = GetSuccElement (ed.ObjectPtr);
		if (CollisionPossible (ed.ObjectPtr, &Ship))
		{
			SDWORD dx, dy;
			
			dx = ed.ObjectPtr->next.location.x
				- Ship.next.location.x;
			dy = ed.ObjectPtr->next.location.y
				- Ship.next.location.y;
			dx = WRAP_DELTA_X (dx);
			dy = WRAP_DELTA_Y (dy);
			if (GRAVITY_MASS (ed.ObjectPtr->mass_points))
			{
				COUNT maneuver_turn, ship_bounds;
				RECT ship_footprint;
				
				if (UltraManeuverable)
					maneuver_turn = 16;
				else if (MANEUVERABILITY (&RDPtr->cyborg_control) <= RESOLUTION_COMPENSATED(MEDIUM_SHIP)) // JMS_GFX
					maneuver_turn = 48;
				else
					maneuver_turn = 32;
				
				GetFrameRect (SetAbsFrameIndex (
												Ship.IntersectControl.IntersectStamp.frame, 0
												), &ship_footprint);
				ship_bounds = (COUNT)(ship_footprint.extent.width
									  + ship_footprint.extent.height);
				
				if (!ShipMoved && (ed.which_turn =
								   PlotIntercept (ed.ObjectPtr, &Ship, maneuver_turn,
												  DISPLAY_TO_WORLD (RES_SCALE(30) + (ship_bounds * 3 /* << 2 */))))) // JMS_GFX
				{
					if (ed.which_turn > 1
						|| PlotIntercept (ed.ObjectPtr, &Ship, 1,
										  DISPLAY_TO_WORLD (RES_SCALE(35) + ship_bounds)) // JMS_GFX
						|| PlotIntercept (ed.ObjectPtr, &Ship,
										  maneuver_turn << 1,
										  DISPLAY_TO_WORLD (RES_SCALE(40) + ship_bounds)) > 1) // JMS_GFX
					{
						ed.facing = ARCTAN (-dx, -dy);
						if (UltraManeuverable)
							ed.MoveState = AVOID;
						else // Try a gravity whip
							ed.MoveState = ENTICE;
						
						ObjectsOfConcern[GRAVITY_MASS_INDEX] = ed;
					}
					else if (!UltraManeuverable &&
							 !IsVelocityZero (&Ship.velocity))
					{	// Try an orbital insertion, don't thrust
						++Ship.thrust_wait;
						if (Ship.turn_wait)
							ShipMoved = TRUE;
					}
				}
			}
			else if (ed.ObjectPtr->state_flags & PLAYER_SHIP)
			{
				GetElementStarShip (ed.ObjectPtr, &EnemyStarShipPtr);
				EnemyRDPtr = EnemyStarShipPtr->RaceDescPtr;
				if (EnemyRDPtr->cyborg_control.ManeuverabilityIndex == 0)
					InitCyborg (EnemyStarShipPtr);
				
				ed.which_turn = (WORLD_TO_TURN (square_root ((long)dx * dx + (long)dy * dy)));
				
				//log_add(log_Debug,"SQR:%d (dx:%d), (dy:%d), norm:%d rezzed:%d", square_root ((long)dx * dx + (long)dy * dy), dx, dy, (WORLD_TO_TURN (square_root ((long)dx * dx + (long)dy * dy))), ed.which_turn);
				
				if (RES_DESCALE(ed.which_turn) > ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn)
				{
					UnlockElement (hElement);
					continue;
				}
				else if (ed.which_turn == 0)
					ed.which_turn = 1;
				
				ed.which_turn >>= RESOLUTION_FACTOR; // JMS_GFX
				
				ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr = ed.ObjectPtr;
				ObjectsOfConcern[ENEMY_SHIP_INDEX].facing =
#ifdef MAYBE
				OBJECT_CLOAKED (ed.ObjectPtr) ? GetVelocityTravelAngle (&ed.ObjectPtr->velocity) :
#endif /* MAYBE */
				FACING_TO_ANGLE (EnemyStarShipPtr->ShipFacing);
				ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn = ed.which_turn;
				
				if (ShipMoved
					|| ed.ObjectPtr->mass_points > MAX_SHIP_MASS
					|| (WEAPON_RANGE (&RDPtr->cyborg_control) < RES_SCALE(LONG_RANGE_WEAPON)
						&& (WEAPON_RANGE (&RDPtr->cyborg_control) <= RES_SCALE(CLOSE_RANGE_WEAPON)
							|| (WEAPON_RANGE (&EnemyRDPtr->cyborg_control) >= RES_SCALE(LONG_RANGE_WEAPON)
								&& (EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & SEEKING_WEAPON))
							|| (
#ifdef OLD
								MANEUVERABILITY (&RDPtr->cyborg_control) <
								MANEUVERABILITY (&EnemyRDPtr->cyborg_control)
#else /* !OLD */
								RDPtr->characteristics.max_thrust <
								EnemyRDPtr->characteristics.max_thrust
#endif /* !OLD */
								&& WEAPON_RANGE (&RDPtr->cyborg_control) <
								WEAPON_RANGE (&EnemyRDPtr->cyborg_control)))))
					ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = PURSUE;
				else
					ObjectsOfConcern[ENEMY_SHIP_INDEX].MoveState = ENTICE;
				
				if ((EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & IMMEDIATE_WEAPON)
					&& ship_weapons (ed.ObjectPtr, &Ship, 0))
				{
					ed.which_turn = 1;
					ed.MoveState = AVOID;
					ed.facing = ObjectsOfConcern[ENEMY_SHIP_INDEX].facing;
					
					ObjectsOfConcern[ENEMY_WEAPON_INDEX] = ed;
				}
			}
			else if (ed.ObjectPtr->pParent == 0)
			{
				if (!(ed.ObjectPtr->state_flags & FINITE_LIFE))
				{
					ed.which_turn = RES_DESCALE(WORLD_TO_TURN (square_root ((long)dx * dx + (long)dy * dy))); // JMS_GFX
					
					if (ed.which_turn < ObjectsOfConcern[FIRST_EMPTY_INDEX].which_turn)
					{
						ed.MoveState = PURSUE;
						ed.facing = GetVelocityTravelAngle (
									&ed.ObjectPtr->velocity);
						
						ObjectsOfConcern[FIRST_EMPTY_INDEX] = ed;
					}
				}
			}
			else if (!elementsOfSamePlayer (ed.ObjectPtr, &Ship)
					 && ed.ObjectPtr->preprocess_func != crew_preprocess
					 && ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn > 1
					 && ed.ObjectPtr->life_span > 0)
			{
				GetElementStarShip (ed.ObjectPtr, &EnemyStarShipPtr);
				EnemyRDPtr = EnemyStarShipPtr->RaceDescPtr;
				if (((EnemyRDPtr->ship_info.ship_flags & SEEKING_WEAPON)
					 && ed.ObjectPtr->next.image.farray !=
					 EnemyRDPtr->ship_data.special)
					|| ((EnemyRDPtr->ship_info.ship_flags & SEEKING_SPECIAL)
						&& ed.ObjectPtr->next.image.farray ==
						EnemyRDPtr->ship_data.special))
				{
					if ((!(ed.ObjectPtr->state_flags & (FINITE_LIFE | CREW_OBJECT))
						 && RDPtr->characteristics.max_thrust > DISPLAY_TO_WORLD (RES_SCALE(8))) // JMS_GFX
						|| NORMALIZE_ANGLE (GetVelocityTravelAngle (
							&ed.ObjectPtr->velocity
							) - ARCTAN (-dx, -dy) + QUADRANT) > HALF_CIRCLE)
						ed.which_turn = 0;
					else
					{
						ed.which_turn = RES_DESCALE(WORLD_TO_TURN (square_root ((long)dx * dx + (long)dy * dy))); // JMS_GFX;
						
						ed.MoveState = ENTICE;
						
						if (ed.which_turn == 0)
							ed.which_turn = 1;
						/* Shiver: The cap on which_turn for seeking weapons raised from 16 to 20.
						 The horrible cap of 8 for above-medium speed ships has been obliterated. */
						else if (ed.which_turn > 20)
							ed.which_turn = 0;
					}
				}
				else if (!(StarShipPtr->control & AWESOME_RATING))
					ed.which_turn = 0;
				else
				{
					ed.which_turn =
					PlotIntercept (ed.ObjectPtr,
						&Ship, ed.ObjectPtr->life_span,
						DISPLAY_TO_WORLD (RES_SCALE(40))); // JMS_GFX
					ed.MoveState = AVOID;
				}
				
				if (ed.which_turn > 0
					&& (ed.which_turn <
						ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn
						|| (ed.which_turn ==
							ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn
							&& ed.MoveState == AVOID)))
				{
					ed.facing = GetVelocityTravelAngle (
									&ed.ObjectPtr->velocity);
					
					ObjectsOfConcern[ENEMY_WEAPON_INDEX] = ed;
				}
			}
			else if ((ed.ObjectPtr->state_flags & CREW_OBJECT)
					 && ((!(ed.ObjectPtr->state_flags & IGNORE_SIMILAR)
						  && elementsOfSamePlayer (ed.ObjectPtr, &Ship))
						 || ed.ObjectPtr->preprocess_func == crew_preprocess)
					 && ObjectsOfConcern[CREW_OBJECT_INDEX].which_turn > 1)
			{
				ed.which_turn = RES_DESCALE(WORLD_TO_TURN (square_root ((long)dx * dx + (long)dy * dy))); // JMS_GFX
				
				if (ed.which_turn == 0)
					ed.which_turn = 1;
				
				if (ObjectsOfConcern[CREW_OBJECT_INDEX].which_turn >
					ed.which_turn
					&& (ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 32
						|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 8
							&& StarShipPtr->hShip == ed.ObjectPtr->hTarget)))
				{
					ed.MoveState = PURSUE;
					ed.facing = 0;
					ObjectsOfConcern[CREW_OBJECT_INDEX] = ed;
				}
			}
		}
		UnlockElement (hElement);
	}
	
	RDPtr->cyborg_control.intelligence_func (&Ship, ObjectsOfConcern,
											 ConcernCounter);
#ifdef DEBUG_CYBORG
	StarShipPtr->ship_input_state &= ~SPECIAL;
#endif /* DEBUG_CYBORG */
	
	StarShipPtr->ShipFacing = ShipFacing;
	{
		BATTLE_INPUT_STATE InputState;
		
		InputState = 0;
		if (StarShipPtr->ship_input_state & LEFT)
			InputState |= BATTLE_LEFT;
		else if (StarShipPtr->ship_input_state & RIGHT)
			InputState |= BATTLE_RIGHT;
		if (StarShipPtr->ship_input_state & THRUST)
			InputState |= BATTLE_THRUST;
		if (StarShipPtr->ship_input_state & WEAPON)
			InputState |= BATTLE_WEAPON;
		if (StarShipPtr->ship_input_state & SPECIAL)
			InputState |= BATTLE_SPECIAL;
		
		(void) context;
		return (InputState);
	}
}

