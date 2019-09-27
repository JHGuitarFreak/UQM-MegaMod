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
#include "druuge.h"
#include "resinst.h"

// Core characteristics
#define MAX_CREW 14
#define MAX_ENERGY 32
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 50
#define MAX_THRUST 20
#define THRUST_INCREMENT 2
#define THRUST_WAIT 1
#define TURN_WAIT 4
#define SHIP_MASS 5

// Mass Driver
#define WEAPON_ENERGY_COST 4
#define WEAPON_WAIT 10
#define DRUUGE_OFFSET RES_SCALE(24)
#define MISSILE_OFFSET RES_SCALE(6)
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 20
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)
#define MISSILE_HITS 4
#define MISSILE_DAMAGE 6
#define RECOIL_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE(6)))
#define MAX_RECOIL_VELOCITY (RECOIL_VELOCITY * 4)

// Furnace
#define SPECIAL_ENERGY_COST 16
#define SPECIAL_WAIT 30

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (120)
#define MISSILE_RANGE_HD (MISSILE_SPEED_HD * MISSILE_LIFE)

static RACE_DESC druuge_desc =
{
	{ /* SHIP_INFO */
		"mauler",
		FIRES_FORE,
		17, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		DRUUGE_RACE_STRINGS,
		DRUUGE_ICON_MASK_PMAP_ANIM,
		DRUUGE_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		1400 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			9500, 2792,
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
			DRUUGE_BIG_MASK_PMAP_ANIM,
			DRUUGE_MED_MASK_PMAP_ANIM,
			DRUUGE_SML_MASK_PMAP_ANIM,
		},
		{
			DRUUGE_CANNON_BIG_MASK_PMAP_ANIM,
			DRUUGE_CANNON_MED_MASK_PMAP_ANIM,
			DRUUGE_CANNON_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			DRUUGE_CAPT_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		DRUUGE_VICTORY_SONG,
		DRUUGE_SHIP_SOUNDS,
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
cannon_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& ElementPtr1->crew_level
			&& !GRAVITY_MASS (ElementPtr1->mass_points + 1))
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

		angle = FACING_TO_ANGLE (
				GetFrameIndex (ElementPtr0->next.image.frame)
				);
		DeltaVelocityComponents (&ElementPtr1->velocity,
				COSINE (angle, RECOIL_VELOCITY),
				SINE (angle, RECOIL_VELOCITY));
		GetCurrentVelocityComponents (&ElementPtr1->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr1->velocity,
					COSINE (angle, MAX_RECOIL_VELOCITY),
					SINE (angle, MAX_RECOIL_VELOCITY));
		}
	}
}

static COUNT
initialize_cannon (ELEMENT *ShipPtr, HELEMENT CannonArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = MissileBlock.face;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = DRUUGE_OFFSET;
	MissileBlock.speed = RES_BOOL(MISSILE_SPEED, MISSILE_SPEED_HD);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	CannonArray[0] = initialize_missile (&MissileBlock);

	if (CannonArray[0])
	{
		ELEMENT *CannonPtr;

		LockElement (CannonArray[0], &CannonPtr);
		CannonPtr->collision_func = cannon_collision;
		UnlockElement (CannonArray[0]);
	}

	return (1);
}

static void
druuge_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	UWORD ship_flags = 0;
	STARSHIP *StarShipPtr;
	STARSHIP *EnemyStarShipPtr = NULL;
	EVALUATE_DESC *lpEvalDesc;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED)
		lpEvalDesc->MoveState = ENTICE;
	else if (lpEvalDesc->ObjectPtr
			&& lpEvalDesc->which_turn <= WORLD_TO_TURN (MISSILE_RANGE * 3 / 4))
	{
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		ship_flags = EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags;
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags &=
				~(FIRES_FORE | FIRES_RIGHT | FIRES_AFT | FIRES_LEFT);

		lpEvalDesc->MoveState = PURSUE;
		if (ShipPtr->thrust_wait == 0)
			++ShipPtr->thrust_wait;
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	if (EnemyStarShipPtr)
	{
		EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags = ship_flags;
	}

	if (!(StarShipPtr->cur_status_flags & SHIP_BEYOND_MAX_SPEED)
			&& (lpEvalDesc->which_turn <= 12
			|| (
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 6
			)))
	{
		 StarShipPtr->ship_input_state |= WEAPON;
		 if (ShipPtr->thrust_wait < WEAPON_WAIT + 1)
			ShipPtr->thrust_wait = WEAPON_WAIT + 1;
	}


	if ((StarShipPtr->ship_input_state & WEAPON)
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level < WEAPON_ENERGY_COST
			&& ShipPtr->crew_level > 1)
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
}

static void
druuge_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
			/* if just fired cannon */
	if ((StarShipPtr->cur_status_flags & WEAPON)
			&& StarShipPtr->weapon_counter ==
			StarShipPtr->RaceDescPtr->characteristics.weapon_wait)
	{
		COUNT angle;
		SDWORD cur_delta_x, cur_delta_y;

		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;

		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		DeltaVelocityComponents (&ElementPtr->velocity,
				COSINE (angle, RECOIL_VELOCITY),
				SINE (angle, RECOIL_VELOCITY));
		GetCurrentVelocityComponentsSdword (&ElementPtr->velocity,
				&cur_delta_x, &cur_delta_y);
		if ((long)cur_delta_x * (long)cur_delta_x
				+ (long)cur_delta_y * (long)cur_delta_y
				> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, MAX_RECOIL_VELOCITY),
					SINE (angle, MAX_RECOIL_VELOCITY));
		}
	}
}

static void
druuge_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & SPECIAL)
	{
		if (StarShipPtr->special_counter
				|| ElementPtr->crew_level == 1
				|| StarShipPtr->RaceDescPtr->ship_info.energy_level
				== StarShipPtr->RaceDescPtr->ship_info.max_energy)
			StarShipPtr->cur_status_flags &= ~SPECIAL;
		else
		{
			ProcessSound (SetAbsSoundIndex (
							/* BURN UP CREW */
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

			DeltaCrew (ElementPtr, -1);
			DeltaEnergy (ElementPtr, SPECIAL_ENERGY_COST);

			StarShipPtr->special_counter =
					StarShipPtr->RaceDescPtr->characteristics.special_wait;
		}
	}
}

RACE_DESC*
init_druuge (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		druuge_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		druuge_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		druuge_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}

	druuge_desc.preprocess_func = druuge_preprocess;
	druuge_desc.postprocess_func = druuge_postprocess;
	druuge_desc.init_weapon_func = initialize_cannon;
	druuge_desc.cyborg_control.intelligence_func = druuge_intelligence;

	RaceDescPtr = &druuge_desc;

	return (RaceDescPtr);
}

