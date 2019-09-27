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
#include "slylandr.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 12
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 0
#define ENERGY_WAIT 10
#define MAX_THRUST 60
#define THRUST_INCREMENT MAX_THRUST
#define THRUST_WAIT 0
#define TURN_WAIT 0
#define SHIP_MASS 1

// Lightning weapon
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 17
#define SLYLANDRO_OFFSET 9
#define LASER_LENGTH RES_SCALE(32)
		/* Total length of lighting bolts. Actual range is usually less than
		 * this, since the lightning rarely is straight. */

// Harvester
#define SPECIAL_ENERGY_COST 0
#define SPECIAL_WAIT 20
#define HARVEST_RANGE RES_SCALE(208 * 3 / 8)
		/* Was originally (SPACE_HEIGHT * 3 / 8) */

static RACE_DESC slylandro_desc =
{
	{ /* SHIP_INFO */
		"probe",
		SEEKING_WEAPON | CREW_IMMUNE,
		17, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		SLYLANDRO_RACE_STRINGS,
		SLYLANDRO_ICON_MASK_PMAP_ANIM,
		SLYLANDRO_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		INFINITE_RADIUS, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			333, 9812,
		},
	},
	{
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
	{
		{
			SLYLANDRO_BIG_MASK_PMAP_ANIM,
			SLYLANDRO_MED_MASK_PMAP_ANIM,
			SLYLANDRO_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			SLYLANDRO_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SLYLANDRO_VICTORY_SONG,
		SLYLANDRO_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON << 1,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static COUNT initialize_lightning (ELEMENT *ElementPtr,
		HELEMENT LaserArray[]);

static void
lightning_postprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait
			&& !(ElementPtr->state_flags & COLLISION))
	{
		HELEMENT Lightning;

		initialize_lightning (ElementPtr, &Lightning);
		if (Lightning)
			PutElement (Lightning);
	}
}

static void
lightning_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->weapon_counter > WEAPON_WAIT >> 1)
		StarShipPtr->weapon_counter =
				WEAPON_WAIT - StarShipPtr->weapon_counter;
	StarShipPtr->weapon_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;

	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_lightning (ELEMENT *ElementPtr, HELEMENT LaserArray[])
{
	LASER_BLOCK LaserBlock;

	LaserBlock.cx = ElementPtr->next.location.x;
	LaserBlock.cy = ElementPtr->next.location.y;
	LaserBlock.ex = 0;
	LaserBlock.ey = 0;

	LaserBlock.sender = ElementPtr->playerNr;
	LaserBlock.flags = IGNORE_SIMILAR;
	LaserBlock.face = 0;
	LaserBlock.pixoffs = 0;
	LaserArray[0] = initialize_laser (&LaserBlock);

	if (LaserArray[0])
	{
		SIZE delta;
		COUNT angle, facing;
		DWORD rand_val;
		ELEMENT *LaserPtr;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);

		LockElement (LaserArray[0], &LaserPtr);
		LaserPtr->postprocess_func = lightning_postprocess;
		LaserPtr->collision_func = lightning_collision;

		rand_val = TFB_Random ();

		if (!(ElementPtr->state_flags & PLAYER_SHIP))
		{
			angle = GetVelocityTravelAngle (&ElementPtr->velocity);
			facing = NORMALIZE_FACING (ANGLE_TO_FACING (angle));
			delta = TrackShip (ElementPtr, &facing);

			LaserPtr->turn_wait = ElementPtr->turn_wait - 1;

			SetPrimColor (&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
					GetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex]));
		}
		else
		{
			facing = StarShipPtr->ShipFacing;
			ElementPtr->hTarget = 0;
			delta = TrackShip (ElementPtr, &facing);
			ElementPtr->hTarget = 0;
			angle = FACING_TO_ANGLE (facing);

			if ((LaserPtr->turn_wait = StarShipPtr->weapon_counter) == 0)
				LaserPtr->turn_wait = WEAPON_WAIT;

			if (LaserPtr->turn_wait > WEAPON_WAIT >> 1)
				LaserPtr->turn_wait = WEAPON_WAIT - LaserPtr->turn_wait;

			switch (HIBYTE (LOWORD (rand_val)) & 3)
			{
				case 0:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
							);
					break;
				case 1:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x16, 0x17, 0x1F), 0x42)
							);
					break;
				case 2:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x06, 0x07, 0x1F), 0x4A)
							);
					break;
				case 3:
					SetPrimColor (
							&(GLOBAL (DisplayArray))[LaserPtr->PrimIndex],
							BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x18), 0x50)
							);
					break;
			}
		}

		if (delta == -1 || delta == ANGLE_TO_FACING (HALF_CIRCLE))
			angle += LOWORD (rand_val);
		else if (delta == 0)
			angle += LOWORD (rand_val) & 1 ? -1 : 1;
		else if (delta < ANGLE_TO_FACING (HALF_CIRCLE))
			angle += LOWORD (rand_val) & (QUADRANT - 1);
		else
			angle -= LOWORD (rand_val) & (QUADRANT - 1);
		delta = WORLD_TO_VELOCITY (
				DISPLAY_TO_WORLD ((HIWORD (rand_val) & (LASER_LENGTH - 1)) + 4)
				);
		SetVelocityComponents (&LaserPtr->velocity,
				COSINE (angle, delta), SINE (angle, delta));

		SetElementStarShip (LaserPtr, StarShipPtr);
		UnlockElement (LaserArray[0]);
	}

	return (1);
}

static void
slylandro_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	// no dodging in role playing game, unless you haven't
	// visited the starbase yet or difficulty is set to Hard
	if ((LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER) &&
			GET_GAME_STATE (STARBASE_AVAILABLE) && !DIF_HARD)
		ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = 0;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level == 0
			&& ObjectsOfConcern[GRAVITY_MASS_INDEX].ObjectPtr == 0)
		ConcernCounter = FIRST_EMPTY_INDEX + 1;
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == PURSUE
			&& lpEvalDesc->which_turn <= 6)
		lpEvalDesc->MoveState = ENTICE;

	++ShipPtr->thrust_wait;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--ShipPtr->thrust_wait;

	if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
		StarShipPtr->ship_input_state |= WEAPON;
	else
		StarShipPtr->ship_input_state &= ~WEAPON;

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		lpEvalDesc = &ObjectsOfConcern[FIRST_EMPTY_INDEX];
		if (lpEvalDesc->ObjectPtr && lpEvalDesc->which_turn <= 14)
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static BOOLEAN
harvest_space_junk (ELEMENT *ElementPtr)
{
	BOOLEAN retval;
	HELEMENT hElement, hNextElement;

	retval = FALSE;
	for (hElement = GetHeadElement ();
			hElement; hElement = hNextElement)
	{
		ELEMENT *ObjPtr;

		LockElement (hElement, &ObjPtr);
		hNextElement = GetSuccElement (ObjPtr);

		if (!(ObjPtr->state_flags & (APPEARING | PLAYER_SHIP | FINITE_LIFE))
				&& ObjPtr->playerNr == NEUTRAL_PLAYER_NUM
				&& !GRAVITY_MASS (ObjPtr->mass_points)
				&& CollisionPossible (ObjPtr, ElementPtr))
		{
			SDWORD dx, dy;

			if ((dx = ObjPtr->next.location.x
					- ElementPtr->next.location.x) < 0)
				dx = -dx;
			if ((dy = ObjPtr->next.location.y
					- ElementPtr->next.location.y) < 0)
				dy = -dy;
			dx = WORLD_TO_DISPLAY (dx);
			dy = WORLD_TO_DISPLAY (dy);
			if (dx <= HARVEST_RANGE && dy <= HARVEST_RANGE
					&& dx * dx + dy * dy <= HARVEST_RANGE * HARVEST_RANGE)
			{
				ObjPtr->life_span = 0;
				ObjPtr->state_flags |= NONSOLID;

				if (!retval)
				{
					STARSHIP *StarShipPtr;

					retval = TRUE;

					GetElementStarShip (ElementPtr, &StarShipPtr);
					ProcessSound (SetAbsSoundIndex (
							StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
					DeltaEnergy (ElementPtr, MAX_ENERGY);
				}
			}
		}

		UnlockElement (hElement);
	}

	return (retval);
}

static void
slylandro_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->weapon_counter
			&& StarShipPtr->weapon_counter < WEAPON_WAIT)
	{
		HELEMENT Lightning;

		initialize_lightning (ElementPtr, &Lightning);
		if (Lightning)
			PutElement (Lightning);
	}

	if (StarShipPtr->special_counter == 0
			&& (StarShipPtr->cur_status_flags & SPECIAL)
			&& harvest_space_junk (ElementPtr))
	{
		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void
slylandro_preprocess (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & (APPEARING | NONSOLID)))
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if ((StarShipPtr->cur_status_flags & THRUST)
				&& !(StarShipPtr->old_status_flags & THRUST))
			StarShipPtr->ShipFacing += ANGLE_TO_FACING (HALF_CIRCLE);

		if (ElementPtr->turn_wait == 0)
		{
			ElementPtr->turn_wait +=
					StarShipPtr->RaceDescPtr->characteristics.turn_wait + 1;
			if (StarShipPtr->cur_status_flags & LEFT)
				--StarShipPtr->ShipFacing;
			else if (StarShipPtr->cur_status_flags & RIGHT)
				++StarShipPtr->ShipFacing;
		}

		StarShipPtr->ShipFacing = NORMALIZE_FACING (StarShipPtr->ShipFacing);

		if (ElementPtr->thrust_wait == 0)
		{
			ElementPtr->thrust_wait +=
					StarShipPtr->RaceDescPtr->characteristics.thrust_wait + 1;

			SetVelocityVector (&ElementPtr->velocity,
					StarShipPtr->RaceDescPtr->characteristics.max_thrust,
					StarShipPtr->ShipFacing);
			StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;
			StarShipPtr->cur_status_flags &= ~SHIP_IN_GRAVITY_WELL;
		}

		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->next.image.frame);
		ElementPtr->state_flags |= CHANGING;
	}
}

RACE_DESC*
init_slylandro (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		slylandro_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		slylandro_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		slylandro_desc.cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON_HD << 1;
	}

	slylandro_desc.preprocess_func = slylandro_preprocess;
	slylandro_desc.postprocess_func = slylandro_postprocess;
	slylandro_desc.init_weapon_func = initialize_lightning;
	slylandro_desc.cyborg_control.intelligence_func = slylandro_intelligence;

	RaceDescPtr = &slylandro_desc;

	return (RaceDescPtr);
}