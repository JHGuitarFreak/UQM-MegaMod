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
#include "spathi.h"
#include "resinst.h"

// Core characteristics
#define MAX_CREW 30
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 10
#define MAX_THRUST 48
#define THRUST_INCREMENT 12
#define THRUST_WAIT 1
#define TURN_WAIT 1
#define SHIP_MASS 5

// Forward gun
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 0
#define SPATHI_FORWARD_OFFSET RES_SCALE(16)
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET RES_SCALE(1)
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)
		/* This is for the cyborg only. */

// B.U.T.T.
#define SPECIAL_ENERGY_COST 3
#define SPECIAL_WAIT 7
#define SPATHI_REAR_OFFSET RES_SCALE(20)
#define DISCRIMINATOR_SPEED DISPLAY_TO_WORLD (RES_SCALE(8))
#define DISCRIMINATOR_LIFE 30
#define DISCRIMINATOR_HITS 1
#define DISCRIMINATOR_DAMAGE 2
#define DISCRIMINATOR_OFFSET RES_SCALE(4)
#define TRACK_WAIT 1

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (120)
#define MISSILE_RANGE_HD (MISSILE_SPEED_HD * MISSILE_LIFE)

static RACE_DESC spathi_desc =
{
	{ /* SHIP_INFO */
		"eluder",
		FIRES_FORE | FIRES_AFT | SEEKING_SPECIAL | DONT_CHASE,
		18, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		SPATHI_RACE_STRINGS,
		SPATHI_ICON_MASK_PMAP_ANIM,
		SPATHI_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		1000 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			2549, 3600,
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
			SPATHI_BIG_MASK_PMAP_ANIM,
			SPATHI_MED_MASK_PMAP_ANIM,
			SPATHI_SML_MASK_PMAP_ANIM,
		},
		{
			MISSILE_BIG_MASK_PMAP_ANIM,
			MISSILE_MED_MASK_PMAP_ANIM,
			MISSILE_SML_MASK_PMAP_ANIM,
		},
		{
			DISCRIM_BIG_MASK_PMAP_ANIM,
			DISCRIM_MED_MASK_PMAP_ANIM,
			DISCRIM_SML_MASK_PMAP_ANIM,
		},
		{
			SPATHI_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SPATHI_VICTORY_SONG,
		SPATHI_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		MISSILE_RANGE,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static void
butt_missile_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;

		facing = GetFrameIndex (ElementPtr->next.image.frame);
		if (TrackShip (ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->next.image.frame,
					facing);
			ElementPtr->state_flags |= CHANGING;
	
			SetVelocityVector (&ElementPtr->velocity,
					DISCRIMINATOR_SPEED, facing);
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}

static void
spawn_butt_missile (ELEMENT *ShipPtr)
{
	HELEMENT ButtMissile;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK ButtMissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	ButtMissileBlock.cx = ShipPtr->next.location.x;
	ButtMissileBlock.cy = ShipPtr->next.location.y;
	ButtMissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	ButtMissileBlock.face = ButtMissileBlock.index =
			NORMALIZE_FACING (StarShipPtr->ShipFacing
			+ ANGLE_TO_FACING (HALF_CIRCLE));
	ButtMissileBlock.sender = ShipPtr->playerNr;
	ButtMissileBlock.flags = 0;
	ButtMissileBlock.pixoffs = SPATHI_REAR_OFFSET;
	ButtMissileBlock.speed = DISCRIMINATOR_SPEED;
	ButtMissileBlock.hit_points = DISCRIMINATOR_HITS;
	ButtMissileBlock.damage = DISCRIMINATOR_DAMAGE;
	ButtMissileBlock.life = DISCRIMINATOR_LIFE;
	ButtMissileBlock.preprocess_func = butt_missile_preprocess;
	ButtMissileBlock.blast_offs = DISCRIMINATOR_OFFSET;
	ButtMissile = initialize_missile (&ButtMissileBlock);
	if (ButtMissile)
	{
		ELEMENT *ButtPtr;

		LockElement (ButtMissile, &ButtPtr);
		ButtPtr->turn_wait = TRACK_WAIT;
		SetElementStarShip (ButtPtr, StarShipPtr);

		ProcessSound (SetAbsSoundIndex (
					/* LAUNCH_BUTT_MISSILE */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ButtPtr);

		UnlockElement (ButtMissile);
		PutElement (ButtMissile);
	}
}

static void
spathi_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SPECIAL;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& lpEvalDesc->which_turn <= 24)
	{
		COUNT travel_facing, direction_facing;
		SDWORD delta_x, delta_y;

		travel_facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (GetVelocityTravelAngle (&ShipPtr->velocity)
				+ HALF_CIRCLE)
				);
		delta_x = lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x;
		delta_y = lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y;
		direction_facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (ARCTAN (delta_x, delta_y))
				);

		if (NORMALIZE_FACING (direction_facing
				- (StarShipPtr->ShipFacing + ANGLE_TO_FACING (HALF_CIRCLE))
				+ ANGLE_TO_FACING (QUADRANT))
				<= ANGLE_TO_FACING (HALF_CIRCLE)
				&& (lpEvalDesc->which_turn <= 8
				|| NORMALIZE_FACING (direction_facing
				+ ANGLE_TO_FACING (HALF_CIRCLE)
				- ANGLE_TO_FACING (GetVelocityTravelAngle (
						&lpEvalDesc->ObjectPtr->velocity
						))
				+ ANGLE_TO_FACING (QUADRANT))
				<= ANGLE_TO_FACING (HALF_CIRCLE))
				&& (!(StarShipPtr->cur_status_flags &
				(SHIP_BEYOND_MAX_SPEED | SHIP_IN_GRAVITY_WELL))
				|| NORMALIZE_FACING (direction_facing
				- travel_facing + ANGLE_TO_FACING (QUADRANT))
				<= ANGLE_TO_FACING (HALF_CIRCLE)))
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static COUNT
initialize_standard_missile (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = SPATHI_FORWARD_OFFSET;
	MissileBlock.speed = RES_BOOL(MISSILE_SPEED, MISSILE_SPEED_HD);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	MissileArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void
spathi_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		spawn_butt_missile (ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RACE_DESC*
init_spathi (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		spathi_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		spathi_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		spathi_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}

	spathi_desc.postprocess_func = spathi_postprocess;
	spathi_desc.init_weapon_func = initialize_standard_missile;
	spathi_desc.cyborg_control.intelligence_func = spathi_intelligence;

	RaceDescPtr = &spathi_desc;

	return (RaceDescPtr);
}
