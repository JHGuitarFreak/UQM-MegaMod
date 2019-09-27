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

#include "../ship.h"
#include "androsyn.h"
#include "resinst.h"
#include "../../units.h"

#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 24
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 8
#define MAX_THRUST 24
#define THRUST_INCREMENT 3
#define TURN_WAIT 4
#define THRUST_WAIT 0
#define SHIP_MASS 6

// Bubbles
#define WEAPON_ENERGY_COST 3
#define WEAPON_WAIT 0
#define ANDROSYNTH_OFFSET RES_SCALE(14)
#define MISSILE_OFFSET RES_SCALE(3)
#define MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(8))
#define MISSILE_LIFE 200
#define MISSILE_HITS 3
#define MISSILE_DAMAGE 2
#define TRACK_WAIT 2

// Blazer
#define SPECIAL_ENERGY_COST 2
#define BLAZER_DEGENERATION (-1)
#define SPECIAL_WAIT 0
#define BLAZER_OFFSET RES_SCALE(10)
#define BLAZER_THRUST RES_SCALE(60)
#define BLAZER_TURN_WAIT 1
#define BLAZER_DAMAGE 3
#define BLAZER_MASS 1

static RACE_DESC androsynth_desc =
{
	{ /* SHIP_INFO */
		"guardian",
		FIRES_FORE | SEEKING_WEAPON,
		15, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		ANDROSYNTH_RACE_STRINGS,
		ANDROSYNTH_ICON_MASK_PMAP_ANIM,
		ANDROSYNTH_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		INFINITE_RADIUS, /* Initial sphere of influence radius */
				// XXX: Why infinite radius? Bug?
		{ /* Known location (center of SoI) */
			MAX_X_UNIVERSE >> 1, MAX_Y_UNIVERSE >> 1,
		},
	},
	{ /* CHARACTERISTIC_STUFF */
		MAX_THRUST,
		THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		TURN_WAIT,
		THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{ /* DATA_STUFF */
		{
			ANDROSYNTH_BIG_MASK_PMAP_ANIM,
			ANDROSYNTH_MED_MASK_PMAP_ANIM,
			ANDROSYNTH_SML_MASK_PMAP_ANIM,
		},
		{
			BUBBLE_BIG_MASK_PMAP_ANIM,
			BUBBLE_MED_MASK_PMAP_ANIM,
			BUBBLE_SML_MASK_PMAP_ANIM,
		},
		{
			BLAZER_BIG_MASK_PMAP_ANIM,
			BLAZER_MED_MASK_PMAP_ANIM,
			BLAZER_SML_MASK_PMAP_ANIM,
		},
		{
			ANDROSYNTH_CAPT_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		ANDROSYNTH_VICTORY_SONG,
		ANDROSYNTH_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{ /* INTEL_STUFF */
		0,
		LONG_RANGE_WEAPON >> 2,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

// Private per-instance ship data
typedef struct
{
	ElementCollisionFunc* collision_func;
} ANDROSYNTH_DATA;

// Local typedef
typedef ANDROSYNTH_DATA CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t *
GetCustomShipData (RACE_DESC *pRaceDesc)
{
	return pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void
SetCustomShipData (RACE_DESC *pRaceDesc, const CustomShipData_t *data)
{
	if (pRaceDesc->data == data) 
		return;  // no-op

	if (pRaceDesc->data) // Out with the old
	{
		HFree (pRaceDesc->data);
		pRaceDesc->data = NULL;
	}

	if (data) // In with the new
	{
		CustomShipData_t* newData = HMalloc (sizeof (*data));
		*newData = *data;
		pRaceDesc->data = newData;
	}
}

static void
blazer_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	BYTE old_offs;
	COUNT old_crew_level;
	COUNT old_life;

	old_crew_level = ElementPtr0->crew_level;
	old_life = ElementPtr0->life_span;
	old_offs = ElementPtr0->blast_offset;
	ElementPtr0->blast_offset = BLAZER_OFFSET;
	ElementPtr0->mass_points = BLAZER_DAMAGE;
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	ElementPtr0->mass_points = BLAZER_MASS;
	ElementPtr0->blast_offset = old_offs;
	ElementPtr0->life_span = old_life;
	ElementPtr0->crew_level = old_crew_level;

	ElementPtr0->state_flags &= ~(DISAPPEARING | NONSOLID);
	collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void
bubble_preprocess (ELEMENT *ElementPtr)
{
	BYTE thrust_wait, turn_wait;

	thrust_wait = HINIBBLE (ElementPtr->turn_wait);
	turn_wait = LONIBBLE (ElementPtr->turn_wait);
	if (thrust_wait > 0)
		--thrust_wait;
	else
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		ElementPtr->state_flags |= CHANGING;

		thrust_wait = (BYTE)((COUNT)TFB_Random () & 3);
	}

	if (turn_wait > 0)
		--turn_wait;
	else
	{
		COUNT facing;
		SIZE delta_facing;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)));
		if ((delta_facing = TrackShip (ElementPtr, &facing)) == -1)
			facing = (COUNT)TFB_Random ();
		else if (delta_facing <= ANGLE_TO_FACING (HALF_CIRCLE))
			facing += (COUNT)TFB_Random () & (ANGLE_TO_FACING (HALF_CIRCLE) - 1);
		else
			facing -= (COUNT)TFB_Random () & (ANGLE_TO_FACING (HALF_CIRCLE) - 1);
		SetVelocityVector (&ElementPtr->velocity,
				MISSILE_SPEED, facing);
		turn_wait = TRACK_WAIT;
	}

	ElementPtr->turn_wait = MAKE_BYTE (turn_wait, thrust_wait);
}

static COUNT
initialize_bubble (ELEMENT *ShipPtr, HELEMENT BubbleArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = ANDROSYNTH_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = bubble_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	BubbleArray[0] = initialize_missile (&MissileBlock);

	if (BubbleArray[0])
	{
		ELEMENT *BubblePtr;

		LockElement (BubbleArray[0], &BubblePtr);
		BubblePtr->turn_wait = 0;
		UnlockElement (BubbleArray[0]);
	}

	return (1);
}

static void
androsynth_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
				/* in blazer form */
	if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		ObjectsOfConcern[CREW_OBJECT_INDEX].ObjectPtr = 0;
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
		{
			if ((lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
					&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
				lpEvalDesc->MoveState = AVOID;
			else
				lpEvalDesc->ObjectPtr = 0;
		}

		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	}
	else
	{
		STARSHIP *pEnemyStarShip = NULL;

		lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (lpEvalDesc->ObjectPtr)
		{
			GetElementStarShip (lpEvalDesc->ObjectPtr, &pEnemyStarShip);
			if (lpEvalDesc->which_turn <= 16
					&& (StarShipPtr->special_counter > 0
					|| StarShipPtr->RaceDescPtr->ship_info.energy_level < MAX_ENERGY / 3
					|| ((WEAPON_RANGE (&pEnemyStarShip->RaceDescPtr->cyborg_control) <= RES_SCALE(CLOSE_RANGE_WEAPON)
					&& lpEvalDesc->ObjectPtr->crew_level > BLAZER_DAMAGE)
					|| (lpEvalDesc->ObjectPtr->crew_level > (BLAZER_DAMAGE * 3)
					&& MANEUVERABILITY (&pEnemyStarShip->RaceDescPtr->cyborg_control) > RESOLUTION_COMPENSATED(SLOW_SHIP)))))
				lpEvalDesc->MoveState = ENTICE;
		}

		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (StarShipPtr->special_counter == 0)
		{
			StarShipPtr->ship_input_state &= ~SPECIAL;
			if ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
					&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 4)
					|| (lpEvalDesc->ObjectPtr
					&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= MAX_ENERGY / 3
					&& (WEAPON_RANGE (&pEnemyStarShip->RaceDescPtr->cyborg_control) >=
					WEAPON_RANGE (&StarShipPtr->RaceDescPtr->cyborg_control) << 1
					|| (lpEvalDesc->which_turn < 16
					&& (WEAPON_RANGE (&pEnemyStarShip->RaceDescPtr->cyborg_control) > RES_SCALE(CLOSE_RANGE_WEAPON)
					|| lpEvalDesc->ObjectPtr->crew_level <= BLAZER_DAMAGE)
					&& (lpEvalDesc->ObjectPtr->crew_level <= (BLAZER_DAMAGE * 3)
					|| MANEUVERABILITY (&pEnemyStarShip->RaceDescPtr->cyborg_control) <=
					RESOLUTION_COMPENSATED(SLOW_SHIP))))))
				StarShipPtr->ship_input_state |= SPECIAL;
		}

		if (!(StarShipPtr->ship_input_state & SPECIAL)
				&& StarShipPtr->weapon_counter == 0
				&& lpEvalDesc->ObjectPtr)
		{
			if (lpEvalDesc->which_turn <= 4)
				StarShipPtr->ship_input_state |= WEAPON;
			else if (lpEvalDesc->MoveState != PURSUE
					&& lpEvalDesc->which_turn <= 12)
			{
				COUNT travel_facing, direction_facing;
				SDWORD delta_x, delta_y,
							ship_delta_x, ship_delta_y,
							other_delta_x, other_delta_y;

				GetCurrentVelocityComponentsSdword (&ShipPtr->velocity,
						&ship_delta_x, &ship_delta_y);
				GetCurrentVelocityComponentsSdword (&lpEvalDesc->ObjectPtr->velocity,
						&other_delta_x, &other_delta_y);
				delta_x = ship_delta_x - other_delta_x;
				delta_y = ship_delta_y - other_delta_y;
				travel_facing = ARCTAN (delta_x, delta_y);

				delta_x =
						lpEvalDesc->ObjectPtr->next.location.x -
						ShipPtr->next.location.x;
				delta_y =
						lpEvalDesc->ObjectPtr->next.location.y -
						ShipPtr->next.location.y;
				direction_facing = ARCTAN (delta_x, delta_y);

				if (NORMALIZE_ANGLE (travel_facing
						- direction_facing + OCTANT) <= QUADRANT)
					StarShipPtr->ship_input_state |= WEAPON;
			}
		}
	}
}

static void
androsynth_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
			/* take care of blazer effect */
	if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				|| StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		{
			StarShipPtr->RaceDescPtr->characteristics.energy_regeneration =
					(BYTE)BLAZER_DEGENERATION;
			StarShipPtr->energy_counter = ENERGY_WAIT;

			if (StarShipPtr->cur_status_flags & SPECIAL)
			{
				ProcessSound (SetAbsSoundIndex (
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						ElementPtr);  /* COMET_ON */
				ElementPtr->turn_wait = 0;
				ElementPtr->thrust_wait = 0;
				StarShipPtr->RaceDescPtr->characteristics.special_wait =
						StarShipPtr->RaceDescPtr->characteristics.turn_wait;
				ElementPtr->mass_points = BLAZER_MASS;
				StarShipPtr->RaceDescPtr->characteristics.turn_wait
						= BLAZER_TURN_WAIT;

				/* Save the current collision func because we were not the
				 * ones who set it */
				{
					const ANDROSYNTH_DATA shipData = { ElementPtr->collision_func };
					SetCustomShipData (StarShipPtr->RaceDescPtr, &shipData);
					ElementPtr->collision_func = blazer_collision;
				}
			}
		}

		if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
				/* if blazer wasn't able to change back into ship
				 * give it a little more juice to try to get out
				 * of its predicament.
				 */
		{
			DeltaEnergy (ElementPtr, -BLAZER_DEGENERATION);
			StarShipPtr->energy_counter = 1;
		}
	}
}

static void
androsynth_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	STATUS_FLAGS cur_status_flags;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	cur_status_flags = StarShipPtr->cur_status_flags;
	if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
	{
		if (cur_status_flags & SPECIAL)
		{
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST)
				DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST); /* so text will flash */
			else
			{
				cur_status_flags &= ~WEAPON;

				ElementPtr->next.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.special;
				ElementPtr->next.image.frame =
						SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0],
						ElementPtr->next.image.frame);
				ElementPtr->state_flags |= CHANGING;
			}
		}
	}
	else
	{
		cur_status_flags &= ~(THRUST | WEAPON | SPECIAL);

		/* keep the special "on" for the duration of blazer mode, a la SC1 */
		StarShipPtr->RaceDescPtr->ship_data.captain_control.special =
			SetRelFrameIndex(StarShipPtr->RaceDescPtr->ship_data.captain_control.special, 2);

					/* protection against vux */
		if (StarShipPtr->RaceDescPtr->characteristics.turn_wait > BLAZER_TURN_WAIT)
		{
			StarShipPtr->RaceDescPtr->characteristics.special_wait +=
					StarShipPtr->RaceDescPtr->characteristics.turn_wait
					- BLAZER_TURN_WAIT;
			StarShipPtr->RaceDescPtr->characteristics.turn_wait = BLAZER_TURN_WAIT;
		}

		if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		{
			/* turn special off */
			StarShipPtr->RaceDescPtr->ship_data.captain_control.special =
				SetRelFrameIndex(StarShipPtr->RaceDescPtr->ship_data.captain_control.special, -2);

			ZeroVelocityComponents (&ElementPtr->velocity);
			cur_status_flags &= ~(LEFT | RIGHT
					| SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

			DrawCaptainsWindow(StarShipPtr);

			StarShipPtr->RaceDescPtr->characteristics.turn_wait =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
			StarShipPtr->RaceDescPtr->characteristics.energy_regeneration = ENERGY_REGENERATION;
			ElementPtr->mass_points = SHIP_MASS;
			ElementPtr->collision_func = 
					GetCustomShipData (StarShipPtr->RaceDescPtr)->collision_func;
			ElementPtr->next.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->next.image.frame =
					SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
					ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;
		}
		else
		{
			if (ElementPtr->thrust_wait)
				--ElementPtr->thrust_wait;
			else
			{
				COUNT facing;

				facing = StarShipPtr->ShipFacing;
				if (ElementPtr->turn_wait == 0
						&& (cur_status_flags & (LEFT | RIGHT)))
				{
					if (cur_status_flags & LEFT)
						--facing;
					else
						++facing;
				}

				SetVelocityVector (&ElementPtr->velocity,
						BLAZER_THRUST, NORMALIZE_FACING (facing));
				cur_status_flags |= SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED;
			}
		}
	}
	StarShipPtr->cur_status_flags = cur_status_flags;
}

static void
uninit_androsynth (RACE_DESC *pRaceDesc)
{
	SetCustomShipData (pRaceDesc, NULL);
}


RACE_DESC*
init_androsynth (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		androsynth_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		androsynth_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		androsynth_desc.cyborg_control.WeaponRange = LONG_RANGE_WEAPON_HD >> 2;
	}

	androsynth_desc.preprocess_func = androsynth_preprocess;
	androsynth_desc.postprocess_func = androsynth_postprocess;
	androsynth_desc.init_weapon_func = initialize_bubble;
	androsynth_desc.cyborg_control.intelligence_func = androsynth_intelligence;
	RaceDescPtr = &androsynth_desc;

	return (RaceDescPtr);
}