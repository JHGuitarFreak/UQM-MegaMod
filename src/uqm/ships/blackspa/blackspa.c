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
#include "blackspa.h"
#include "resinst.h"

 // Core characteristics
#define MAX_CREW 30
#define ACTUAL_CREW 13
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 10
#define MAX_THRUST 40
#define THRUST_INCREMENT 12
#define THRUST_WAIT 1
#define TURN_WAIT 1
#define SHIP_MASS 6

// Forward gun
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 7


#define BLACKSPA_FORWARD_OFFSET 16
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 2
#define MISSILE_OFFSET 1
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)
		/* This is for the cyborg only. */

// B.U.T.T.
#define SPECIAL_ENERGY_COST 1
#define SPECIAL_WAIT 2


#define BLACKSPA_REAR_OFFSET 20
#define DISCRIMINATOR_SPEED DISPLAY_TO_WORLD (18)
#define DISCRIMINATOR_LIFE 22
#define DISCRIMINATOR_HITS 1
#define DISCRIMINATOR_DAMAGE 3
#define DISCRIMINATOR_OFFSET 4
#define TRACK_WAIT 1

static RACE_DESC blackspa_desc =
{
	{ /* SHIP_INFO */
		"bss",
		FIRES_FORE | FIRES_AFT | SEEKING_SPECIAL,
		21, /* Super Melee cost */
		ACTUAL_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		BLACKSPA_RACE_STRINGS,
		BLACKSPA_ICON_MASK_PMAP_ANIM,
		BLACKSPA_MICON_MASK_PMAP_ANIM,
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
			BLACKSPA_BIG_MASK_PMAP_ANIM,
			BLACKSPA_MED_MASK_PMAP_ANIM,
			BLACKSPA_SML_MASK_PMAP_ANIM,
		},
		{
			DISCRIM_BIG_MASK_PMAP_ANIM,
			DISCRIM_MED_MASK_PMAP_ANIM,
			DISCRIM_SML_MASK_PMAP_ANIM,
		},
		{
			MISSILE_BIG_MASK_PMAP_ANIM,
			MISSILE_MED_MASK_PMAP_ANIM,
			MISSILE_SML_MASK_PMAP_ANIM,
		},
		{
			BLACKSPA_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL,
			0, 0, 0, 0, 0
		},
		BLACKSPA_VICTORY_SONG,
		BLACKSPA_SHIP_SOUNDS,
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

static COUNT
spawn_butt_prm (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	HELEMENT ButtMissile;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK ButtMissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	ButtMissileBlock.cx = ShipPtr->next.location.x;
	ButtMissileBlock.cy = ShipPtr->next.location.y;
	ButtMissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	ButtMissileBlock.face = ButtMissileBlock.index = NORMALIZE_FACING (StarShipPtr->ShipFacing
		+ ANGLE_TO_FACING (HALF_CIRCLE));
	ButtMissileBlock.sender = ShipPtr->playerNr;
	ButtMissileBlock.flags = 0;
	ButtMissileBlock.pixoffs = BLACKSPA_REAR_OFFSET;
	ButtMissileBlock.speed = DISCRIMINATOR_SPEED;
	ButtMissileBlock.hit_points = DISCRIMINATOR_HITS;
	ButtMissileBlock.damage = DISCRIMINATOR_DAMAGE;
	ButtMissileBlock.life = DISCRIMINATOR_LIFE;
	ButtMissileBlock.preprocess_func = butt_missile_preprocess;
	ButtMissileBlock.blast_offs = DISCRIMINATOR_OFFSET;
	MissileArray[0] = initialize_missile (&ButtMissileBlock);
	return(1);
}

static void
blackspa_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
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
		SIZE delta_x, delta_y;

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

static void
initialize_prm_missile (ELEMENT *ShipPtr)
{
	HELEMENT Missile;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = MissileBlock.index = NORMALIZE_FACING (StarShipPtr->ShipFacing
		+ ANGLE_TO_FACING (HALF_CIRCLE));
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = BLACKSPA_FORWARD_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	Missile = initialize_missile (&MissileBlock);

	if (Missile)
	{
		ELEMENT *MsslPtr;

		LockElement (Missile, &MsslPtr);
		MsslPtr->turn_wait = TRACK_WAIT;
		SetElementStarShip (MsslPtr, StarShipPtr);

		ProcessSound (SetAbsSoundIndex (
			/* LAUNCH_BUTT_MISSILE */
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), MsslPtr);

		UnlockElement (Missile);
		PutElement (Missile);
	}
}

static void
blackspa_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		initialize_prm_missile (ElementPtr);

		StarShipPtr->special_counter =
			StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}
static void
blackspa_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	SIZE add_facing;

	add_facing = 0;
	if (StarShipPtr->cur_status_flags & THRUST)
	{
		if (ElementPtr->thrust_wait == 0)
			++ElementPtr->thrust_wait;

		add_facing = ANGLE_TO_FACING (HALF_CIRCLE);
	}

	if (add_facing)
	{
		COUNT facing;
		STATUS_FLAGS thrust_status;

		facing = StarShipPtr->ShipFacing;
		StarShipPtr->ShipFacing = NORMALIZE_FACING (
			facing + add_facing
		);
		thrust_status = inertial_thrust (ElementPtr);
		StarShipPtr->cur_status_flags &=
			~(SHIP_AT_MAX_SPEED
				| SHIP_BEYOND_MAX_SPEED
				| SHIP_IN_GRAVITY_WELL);
		StarShipPtr->cur_status_flags |= thrust_status;
		StarShipPtr->ShipFacing = facing;
	}
}

RACE_DESC *
init_blackspa (void)
{
	RACE_DESC *RaceDescPtr;

	blackspa_desc.preprocess_func = blackspa_preprocess;
	blackspa_desc.postprocess_func = blackspa_postprocess;
	blackspa_desc.init_weapon_func = spawn_butt_prm;
	blackspa_desc.cyborg_control.intelligence_func = blackspa_intelligence;

	RaceDescPtr = &blackspa_desc;

	return (RaceDescPtr);
}

