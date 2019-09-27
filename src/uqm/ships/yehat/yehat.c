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
#include "yehat.h"
#include "resinst.h"

#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 10
#define ENERGY_REGENERATION 2
#define ENERGY_WAIT 6
#define MAX_THRUST 30
#define THRUST_INCREMENT 6
#define THRUST_WAIT 2
#define TURN_WAIT 2
#define SHIP_MASS 3

// Twin Pulse Cannon
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define YEHAT_OFFSET RES_SCALE(16)
#define LAUNCH_OFFS DISPLAY_TO_WORLD (RES_SCALE(8))
#define MISSILE_SPEED DISPLAY_TO_WORLD (20)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define MISSILE_OFFSET RES_SCALE(1)

// Force Shield
#define SPECIAL_ENERGY_COST 3
#define SPECIAL_WAIT 2
#define SHIELD_LIFE 10

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (80)

static RACE_DESC yehat_desc =
{
	{ /* SHIP_INFO */
		"terminator",
		FIRES_FORE | SHIELD_DEFENSE,
		23, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		YEHAT_RACE_STRINGS,
		YEHAT_ICON_MASK_PMAP_ANIM,
		YEHAT_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		750 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			4970, 40,
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
			YEHAT_BIG_MASK_PMAP_ANIM,
			YEHAT_MED_MASK_PMAP_ANIM,
			YEHAT_SML_MASK_PMAP_ANIM,
		},
		{
			YEHAT_CANNON_BIG_MASK_PMAP_ANIM,
			YEHAT_CANNON_MED_MASK_PMAP_ANIM,
			YEHAT_CANNON_SML_MASK_PMAP_ANIM,
		},
		{
			SHIELD_BIG_MASK_ANIM,
			SHIELD_MED_MASK_ANIM,
			SHIELD_SML_MASK_ANIM,
		},
		{
			YEHAT_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		YEHAT_VICTORY_SONG,
		YEHAT_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		MISSILE_SPEED * MISSILE_LIFE / 3,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static COUNT
initialize_standard_missiles (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	SDWORD offs_x, offs_y;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = YEHAT_OFFSET;
	MissileBlock.speed = RES_BOOL(MISSILE_SPEED, MISSILE_SPEED_HD);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	offs_x = -SINE (FACING_TO_ANGLE (MissileBlock.face), LAUNCH_OFFS);
	offs_y = COSINE (FACING_TO_ANGLE (MissileBlock.face), LAUNCH_OFFS);

	MissileBlock.cx = ShipPtr->next.location.x + offs_x;
	MissileBlock.cy = ShipPtr->next.location.y + offs_y;
	MissileArray[0] = initialize_missile (&MissileBlock);

	MissileBlock.cx = ShipPtr->next.location.x - offs_x;
	MissileBlock.cy = ShipPtr->next.location.y - offs_y;
	MissileArray[1] = initialize_missile (&MissileBlock);

	return (2);
}

static void
yehat_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	SIZE ShieldStatus;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	ShieldStatus = -1;
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		ShieldStatus = 0;
		if (!(lpEvalDesc->ObjectPtr->state_flags & (FINITE_LIFE | CREW_OBJECT)))
			lpEvalDesc->MoveState = PURSUE;
		else if (lpEvalDesc->ObjectPtr->mass_points
				|| (lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
		{
			if (!(lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE))
				lpEvalDesc->which_turn <<= 1;
			else
			{
				if ((lpEvalDesc->which_turn >>= 1) == 0)
					lpEvalDesc->which_turn = 1;

				if (lpEvalDesc->ObjectPtr->mass_points)
					lpEvalDesc->ObjectPtr = 0;
				else
					lpEvalDesc->MoveState = PURSUE;
			}
			ShieldStatus = 1;
		}
	}

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		if (ShieldStatus)
		{
			if (ShipPtr->life_span <= NORMAL_LIFE + 1
					&& (ShieldStatus > 0 || lpEvalDesc->ObjectPtr)
					&& lpEvalDesc->which_turn <= 2
					&& (ShieldStatus > 0
					|| (lpEvalDesc->ObjectPtr->state_flags
					& PLAYER_SHIP) /* means IMMEDIATE WEAPON */
					|| PlotIntercept (lpEvalDesc->ObjectPtr,
					ShipPtr, 2, 0))
					&& (TFB_Random () & 3))
				StarShipPtr->ship_input_state |= SPECIAL;

			if (lpEvalDesc->ObjectPtr
					&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
				lpEvalDesc->ObjectPtr = 0;
		}
	}

	if ((lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX])->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;

		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				& IMMEDIATE_WEAPON))
			lpEvalDesc->MoveState = PURSUE;
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
/*
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <= SPECIAL_ENERGY_COST)
		StarShipPtr->ship_input_state &= ~WEAPON;
*/
}

static void
yehat_postprocess (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
				/* take care of shield effect */
		if (StarShipPtr->special_counter > 0)
		{
			if (ElementPtr->life_span == NORMAL_LIFE)
				StarShipPtr->special_counter = 0;
			else
			{
#ifdef OLD
				SetPrimColor (
						&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
						BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)
						);
				SetPrimType (
						&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
						STAMPFILL_PRIM
						);
#endif /* OLD */

				ProcessSound (SetAbsSoundIndex (
								/* YEHAT_SHIELD_ON */
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
				DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);
			}
		}

#ifdef OLD
		if (ElementPtr->life_span > NORMAL_LIFE)
		{
			HELEMENT hShipElement;

			if (hShipElement = AllocElement ())
			{
				ELEMENT *ShipElementPtr;

				InsertElement (hShipElement, GetSuccElement (ElementPtr));
				LockElement (hShipElement, &ShipElementPtr);
				ShipElementPtr->playerNr = ElementPtr->playerNr;
				ShipElementPtr->state_flags =
							/* in place of APPEARING */
						(CHANGING | PRE_PROCESS | POST_PROCESS)
						| FINITE_LIFE | NONSOLID;
				SetPrimType (
						&(GLOBAL (DisplayArray))[ShipElementPtr->PrimIndex],
						STAMP_PRIM
						);

				ShipElementPtr->life_span = 0; /* because preprocessing
													 * will not be done
													 */
				ShipElementPtr->current.location = ElementPtr->next.location;
				ShipElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
				ShipElementPtr->current.image.frame =
						SetAbsFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
						StarShipPtr->ShipFacing);
				ShipElementPtr->next = ShipElementPtr->current;
				ShipElementPtr->preprocess_func =
						ShipElementPtr->postprocess_func =
						ShipElementPtr->death_func = NULL;
				ZeroVelocityComponents (&ShipElementPtr->velocity);

				UnlockElement (hShipElement);
			}
		}
#endif /* OLD */
	}
}

static void
yehat_preprocess (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & APPEARING))
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if ((ElementPtr->life_span > NORMAL_LIFE
				/* take care of shield effect */
				&& --ElementPtr->life_span == NORMAL_LIFE)
				|| (ElementPtr->life_span == NORMAL_LIFE
				&& ElementPtr->next.image.farray
						== StarShipPtr->RaceDescPtr->ship_data.special))
		{
#ifdef NEVER
			SetPrimType (
					&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					STAMP_PRIM
					);
#endif /* NEVER */

			ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
			ElementPtr->next.image.frame =
					SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.ship[0],
					ElementPtr->next.image.frame);
			ElementPtr->state_flags |= CHANGING;
		}

		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& StarShipPtr->special_counter == 0)
		{
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST)
				DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST); /* so text will flash */
			else
			{
				ElementPtr->life_span = SHIELD_LIFE + NORMAL_LIFE;

				ElementPtr->next.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
				ElementPtr->next.image.frame =
						SetEquFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0],
						ElementPtr->next.image.frame);
				ElementPtr->state_flags |= CHANGING;

				StarShipPtr->special_counter =
						StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}
		}
	}
}

RACE_DESC*
init_yehat (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		yehat_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		yehat_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		yehat_desc.cyborg_control.WeaponRange = MISSILE_SPEED_HD * MISSILE_LIFE / 3;
	}

	yehat_desc.preprocess_func = yehat_preprocess;
	yehat_desc.postprocess_func = yehat_postprocess;
	yehat_desc.init_weapon_func = initialize_standard_missiles;
	yehat_desc.cyborg_control.intelligence_func = yehat_intelligence;

	RaceDescPtr = &yehat_desc;

	return (RaceDescPtr);
}
