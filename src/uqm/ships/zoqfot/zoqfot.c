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
#include "zoqfot.h"
#include "resinst.h"

#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 10
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 4
#define MAX_THRUST 40
#define THRUST_INCREMENT 10
#define THRUST_WAIT 0
#define TURN_WAIT 1
#define SHIP_MASS 5

// Main weapon
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define ZOQFOTPIK_OFFSET RES_SCALE(13)
#define MISSILE_OFFSET 0
#define MISSILE_SPEED DISPLAY_TO_WORLD (10)
		/* Used by the cyborg only. */
#define MISSILE_LIFE 10
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)
#define MISSILE_DAMAGE 1
#define MISSILE_HITS 1
#define SPIT_WAIT 2
		/* Controls the main weapon color change animation's speed.
		 * The animation advances one frame every SPIT_WAIT frames. */

// Tongue
#define SPECIAL_ENERGY_COST (MAX_ENERGY * 3 / 4)
#define SPECIAL_WAIT 6
#define TONGUE_SPEED 0
#define TONGUE_HITS 1
#define TONGUE_DAMAGE 12
#define TONGUE_OFFSET RES_SCALE(4)

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (40)
#define MISSILE_RANGE_HD (MISSILE_SPEED_HD * MISSILE_LIFE)

static RACE_DESC zoqfotpik_desc =
{
	{ /* SHIP_INFO */
		"stinger",
		FIRES_FORE,
		6, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		ZOQFOTPIK_RACE_STRINGS,
		ZOQFOTPIK_ICON_MASK_PMAP_ANIM,
		ZOQFOTPIK_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		320 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			3761, 5333,
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
			ZOQFOTPIK_BIG_MASK_PMAP_ANIM,
			ZOQFOTPIK_MED_MASK_PMAP_ANIM,
			ZOQFOTPIK_SML_MASK_PMAP_ANIM,
		},
		{
			SPIT_BIG_MASK_PMAP_ANIM,
			SPIT_MED_MASK_PMAP_ANIM,
			SPIT_SML_MASK_PMAP_ANIM,
		},
		{
			STINGER_BIG_MASK_PMAP_ANIM,
			STINGER_MED_MASK_PMAP_ANIM,
			STINGER_SML_MASK_PMAP_ANIM,
		},
		{
			ZOQFOTPIK_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		ZOQFOTPIK_VICTORY_SONG,
		ZOQFOTPIK_SHIP_SOUNDS,
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
spit_preprocess (ELEMENT *ElementPtr)
{
	/* turn_wait is abused here to control the animation speed. */
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT index, angle, speed;

		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
		angle = GetVelocityTravelAngle (&ElementPtr->velocity);
		if ((index = GetFrameIndex (ElementPtr->next.image.frame)) == 1)
			angle = angle + (((COUNT)TFB_Random () % 3) - 1);

		speed = WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (
				RES_SCALE(GetFrameCount (ElementPtr->next.image.frame)) - index) << 1);
		SetVelocityComponents (&ElementPtr->velocity,
				(SIZE)COSINE (angle, speed),
				(SIZE)SINE (angle, speed));

		/* turn_wait is abused here to control the animation speed. */
		ElementPtr->turn_wait = SPIT_WAIT;
		ElementPtr->state_flags |= CHANGING;
	}
}

static COUNT
initialize_spit (ELEMENT *ShipPtr, HELEMENT SpitArray[])
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
	MissileBlock.pixoffs = ZOQFOTPIK_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (
			RES_SCALE(GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.weapon[0]))) << 1;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = spit_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	SpitArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void spawn_tongue (ELEMENT *ElementPtr);

static void
tongue_postprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait)
		spawn_tongue (ElementPtr);
}

static void
tongue_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (StarShipPtr->special_counter ==
			StarShipPtr->RaceDescPtr->characteristics.special_wait)
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);

	StarShipPtr->special_counter -= ElementPtr0->turn_wait;
	ElementPtr0->turn_wait = 0;
	ElementPtr0->state_flags |= NONSOLID;
}

static void
spawn_tongue (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK TongueBlock;
	HELEMENT Tongue;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	TongueBlock.cx = ElementPtr->next.location.x;
	TongueBlock.cy = ElementPtr->next.location.y;
	TongueBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	TongueBlock.face = TongueBlock.index = StarShipPtr->ShipFacing;
	TongueBlock.sender = ElementPtr->playerNr;
	TongueBlock.flags = IGNORE_SIMILAR;
	TongueBlock.pixoffs = 0;
	TongueBlock.speed = TONGUE_SPEED;
	TongueBlock.hit_points = TONGUE_HITS;
	TongueBlock.damage = TONGUE_DAMAGE;
	TongueBlock.life = 1;
	TongueBlock.preprocess_func = 0;
	TongueBlock.blast_offs = TONGUE_OFFSET;
	Tongue = initialize_missile (&TongueBlock);
	if (Tongue)
	{
		ELEMENT *TonguePtr;

		LockElement (Tongue, &TonguePtr);
		TonguePtr->postprocess_func = tongue_postprocess;
		TonguePtr->collision_func = tongue_collision;
		if (ElementPtr->state_flags & PLAYER_SHIP)
			TonguePtr->turn_wait = StarShipPtr->special_counter;
		else
		{
			COUNT angle;
			RECT r;
			SDWORD x_offs, y_offs;

			TonguePtr->turn_wait = ElementPtr->turn_wait - 1;

			GetFrameRect (TonguePtr->current.image.frame, &r);
			x_offs = DISPLAY_TO_WORLD (r.extent.width >> 1);
			y_offs = DISPLAY_TO_WORLD (r.extent.height >> 1);

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
			if (angle > HALF_CIRCLE && angle < FULL_CIRCLE)
				TonguePtr->current.location.x -= x_offs;
			else if (angle > 0 && angle < HALF_CIRCLE)
				TonguePtr->current.location.x += x_offs;
			if (angle < QUADRANT || angle > FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y -= y_offs;
			else if (angle > QUADRANT && angle < FULL_CIRCLE - QUADRANT)
				TonguePtr->current.location.y += y_offs;
		}

		SetElementStarShip (TonguePtr, StarShipPtr);
		UnlockElement (Tongue);
		PutElement (Tongue);
	}
}

static void
zoqfotpik_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	BOOLEAN GiveTongueJob;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	GiveTongueJob = FALSE;
	if (StarShipPtr->special_counter == 0)
	{
		EVALUATE_DESC *lpEnemyEvalDesc;

		StarShipPtr->ship_input_state &= ~SPECIAL;

		lpEnemyEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (lpEnemyEvalDesc->ObjectPtr
				&& lpEnemyEvalDesc->which_turn <= 4
#ifdef NEVER
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
#endif /* NEVER */
				)
		{
			SDWORD delta_x, delta_y;

			GiveTongueJob = TRUE;

			lpEnemyEvalDesc->MoveState = PURSUE;
			delta_x = lpEnemyEvalDesc->ObjectPtr->next.location.x
					- ShipPtr->next.location.x;
			delta_y = lpEnemyEvalDesc->ObjectPtr->next.location.y
					- ShipPtr->next.location.y;
			if (StarShipPtr->ShipFacing == NORMALIZE_FACING (
					ANGLE_TO_FACING (ARCTAN (delta_x, delta_y))
					))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}

	++StarShipPtr->weapon_counter;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--StarShipPtr->weapon_counter;

	if (StarShipPtr->weapon_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		if (!GiveTongueJob)
		{
			ObjectsOfConcern += ConcernCounter;
			while (ConcernCounter--)
			{
				--ObjectsOfConcern;
				if (ObjectsOfConcern->ObjectPtr
						&& (ConcernCounter == ENEMY_SHIP_INDEX
						|| (ConcernCounter == ENEMY_WEAPON_INDEX
						&& ObjectsOfConcern->MoveState != AVOID
#ifdef NEVER
						&& !(StarShipPtr->control & STANDARD_RATING)
#endif /* NEVER */
						))
						&& ship_weapons (ShipPtr,
						ObjectsOfConcern->ObjectPtr, DISPLAY_TO_WORLD (RES_SCALE(20))))
				{
					StarShipPtr->ship_input_state |= WEAPON;
					break;
				}
			}
		}
	}
}

static void
zoqfotpik_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
					/* STICK_OUT_TONGUE */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}

	if (StarShipPtr->special_counter)
		spawn_tongue (ElementPtr);
}

RACE_DESC*
init_zoqfotpik (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		zoqfotpik_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		zoqfotpik_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		zoqfotpik_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}

	zoqfotpik_desc.postprocess_func = zoqfotpik_postprocess;
	zoqfotpik_desc.init_weapon_func = initialize_spit;
	zoqfotpik_desc.cyborg_control.intelligence_func = zoqfotpik_intelligence;

	RaceDescPtr = &zoqfotpik_desc;

	return (RaceDescPtr);
}

