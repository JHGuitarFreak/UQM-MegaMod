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
#include "umgah.h"
#include "resinst.h"

#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 10
#define MAX_ENERGY 30
#define ENERGY_REGENERATION MAX_ENERGY
#define ENERGY_WAIT 150
#define MAX_THRUST /* DISPLAY_TO_WORLD (5) */ 18
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 6
#define THRUST_WAIT 3
#define TURN_WAIT 4
#define SHIP_MASS 1

// Antimatter cone
#define WEAPON_ENERGY_COST 0
#define WEAPON_WAIT 0
#define UMGAH_OFFSET 0
#define CONE_OFFSET 0
#define CONE_SPEED 0
#define CONE_HITS 100
#define CONE_DAMAGE 1
#define CONE_LIFE 1

// Retropropulsion
#define SPECIAL_ENERGY_COST 1
#define SPECIAL_WAIT 2
#define JUMP_DIST DISPLAY_TO_WORLD (RES_SCALE(40))

static RACE_DESC umgah_desc =
{
	{ /* SHIP_INFO */
		"drone",
		FIRES_FORE | IMMEDIATE_WEAPON,
		7, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		UMGAH_RACE_STRINGS,
		UMGAH_ICON_MASK_PMAP_ANIM,
		UMGAH_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		833 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			1798, 6000,
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
			UMGAH_BIG_MASK_PMAP_ANIM,
			UMGAH_MED_MASK_PMAP_ANIM,
			UMGAH_SML_MASK_PMAP_ANIM,
		},
		{
			SPRITZ_MASK_PMAP_ANIM,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			CONE_BIG_MASK_ANIM,
			CONE_MED_MASK_ANIM,
			CONE_SML_MASK_ANIM,
		},
		{
			UMGAH_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		UMGAH_VICTORY_SONG,
		UMGAH_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		(LONG_RANGE_WEAPON << 2),
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
	UWORD prevFacing;
} UMGAH_DATA;

// Local typedef
typedef UMGAH_DATA CustomShipData_t;

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
cone_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->ship_data.special[0] =
			SetRelFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0],
			ANGLE_TO_FACING (FULL_CIRCLE));

	ElementPtr->state_flags |= APPEARING;
}

static void
cone_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	HELEMENT hBlastElement;

	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		RemoveElement (hBlastElement);
		FreeElement (hBlastElement);

		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
}

static void
umgah_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;
	STARSHIP *EnemyStarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		if (lpEvalDesc->which_turn > 3
				|| (StarShipPtr->old_status_flags & SPECIAL))
			lpEvalDesc->ObjectPtr = 0;
		else if ((lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
			lpEvalDesc->MoveState = AVOID;
		else
			lpEvalDesc->MoveState = PURSUE;
	}

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->special_counter
			|| ObjectsOfConcern[GRAVITY_MASS_INDEX].ObjectPtr
			|| lpEvalDesc->ObjectPtr == 0)
	{
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (lpEvalDesc->which_turn < 16)
			StarShipPtr->ship_input_state |= WEAPON;
		StarShipPtr->ship_input_state &= ~SPECIAL;
	}
	else
	{
		BYTE this_turn;
		SDWORD delta_x, delta_y;
		BOOLEAN EnemyBehind, EnoughJuice;

		if (lpEvalDesc->which_turn >= 0xFF + 1)
			this_turn = 0xFF;
		else
			this_turn = (BYTE)lpEvalDesc->which_turn;

		EnoughJuice = (BOOLEAN)((WORLD_TO_TURN (RES_DESCALE(
				JUMP_DIST * StarShipPtr->RaceDescPtr->ship_info.energy_level
				/ SPECIAL_ENERGY_COST
				))) > this_turn); // JMS_GFX
		delta_x = lpEvalDesc->ObjectPtr->next.location.x -
				ShipPtr->next.location.x;
		delta_y = lpEvalDesc->ObjectPtr->next.location.y -
				ShipPtr->next.location.y;
		EnemyBehind = (BOOLEAN)(NORMALIZE_ANGLE (
				ARCTAN (delta_x, delta_y)
				- (FACING_TO_ANGLE (StarShipPtr->ShipFacing)
				+ HALF_CIRCLE) + (OCTANT + (OCTANT >> 2))
				) <= ((OCTANT + (OCTANT >> 2)) << 1));
		
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (EnoughJuice
				&& ((StarShipPtr->old_status_flags & SPECIAL)
				|| EnemyBehind
				|| (this_turn > 6
				&& MANEUVERABILITY (
				&EnemyStarShipPtr->RaceDescPtr->cyborg_control
				) <= RESOLUTION_COMPENSATED(SLOW_SHIP)) // JMS_GFX
				|| (this_turn >= 16 && this_turn <= 24)))
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = (LONG_RANGE_WEAPON << 3);
		else
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON;

		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

		if (StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange == CLOSE_RANGE_WEAPON)
			StarShipPtr->ship_input_state &= ~SPECIAL;
		else
		{
			BOOLEAN LinedUp;

			StarShipPtr->ship_input_state &= ~THRUST;
			LinedUp = (BOOLEAN)(ShipPtr->turn_wait == 0
					&& !(StarShipPtr->old_status_flags & (LEFT | RIGHT)));
			if (((StarShipPtr->old_status_flags & SPECIAL)
					&& this_turn <= StarShipPtr->RaceDescPtr->characteristics.special_wait)
					|| (!(StarShipPtr->old_status_flags & SPECIAL)
					&& EnemyBehind && (LinedUp || this_turn < 16)))
			{
				StarShipPtr->ship_input_state |= SPECIAL;
				StarShipPtr->RaceDescPtr->characteristics.special_wait = this_turn;

				/* don't want him backing straight into ship */
				if (this_turn <= 8 && LinedUp)
				{
					if (TFB_Random () & 1)
						StarShipPtr->ship_input_state |= LEFT;
					else
						StarShipPtr->ship_input_state |= RIGHT;
				}
			}
			else if (StarShipPtr->old_status_flags & SPECIAL)
			{
				StarShipPtr->ship_input_state &= ~(SPECIAL | LEFT | RIGHT);
				StarShipPtr->ship_input_state |= THRUST;
			}
		}

		if (this_turn < 16 && !EnemyBehind)
			StarShipPtr->ship_input_state |= WEAPON;
	}

	if (!(StarShipPtr->ship_input_state & SPECIAL))
		StarShipPtr->RaceDescPtr->characteristics.special_wait = 0xFF;
}

static COUNT
initialize_cone (ELEMENT *ShipPtr, HELEMENT ConeArray[])
{
	STARSHIP *StarShipPtr;
	UMGAH_DATA* UmgahData;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = UMGAH_OFFSET;
	MissileBlock.speed = CONE_SPEED;
	MissileBlock.hit_points = CONE_HITS;
	MissileBlock.damage = CONE_DAMAGE;
	MissileBlock.life = CONE_LIFE;
	MissileBlock.preprocess_func = cone_preprocess;
	MissileBlock.blast_offs = CONE_OFFSET;

	// This func is called every frame while the player is holding down WEAPON
	// Don't reset the cone FRAME to the first image every time
	UmgahData = GetCustomShipData (StarShipPtr->RaceDescPtr);
	if (!UmgahData || StarShipPtr->ShipFacing != UmgahData->prevFacing)
	{
		const UMGAH_DATA shipData = {StarShipPtr->ShipFacing};

		SetCustomShipData (StarShipPtr->RaceDescPtr, &shipData);

		StarShipPtr->RaceDescPtr->ship_data.special[0] =
				SetAbsFrameIndex (
				StarShipPtr->RaceDescPtr->ship_data.special[0],
				StarShipPtr->ShipFacing);
	}
	
	MissileBlock.index = GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.special[0]);
	ConeArray[0] = initialize_missile (&MissileBlock);

	if (ConeArray[0])
	{
		ELEMENT *ConePtr;

		LockElement (ConeArray[0], &ConePtr);
		ConePtr->collision_func = cone_collision;
		ConePtr->state_flags &= ~APPEARING;
		ConePtr->next = ConePtr->current;
		InitIntersectStartPoint (ConePtr);
		InitIntersectEndPoint (ConePtr);
		ConePtr->IntersectControl.IntersectStamp.frame =
				StarShipPtr->RaceDescPtr->ship_data.special[0];
		UnlockElement (ConeArray[0]);
	}

	return (1);
}

static void
umgah_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->special_counter > 0)
	{
		StarShipPtr->special_counter = 0;

		ZeroVelocityComponents (&ElementPtr->velocity);
	}
}

static void
umgah_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if (ElementPtr->state_flags & APPEARING)
	{
		// Reset the value just in case
		SetCustomShipData (StarShipPtr->RaceDescPtr, NULL);
	}
	else
	{
		if (ElementPtr->thrust_wait == 0
				&& (StarShipPtr->cur_status_flags & SPECIAL)
				&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
		{
			COUNT facing;

			ProcessSound (SetAbsSoundIndex (
							/* ZIP_BACKWARDS */
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
			facing = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
			DeltaVelocityComponents (&ElementPtr->velocity,
					COSINE (facing, WORLD_TO_VELOCITY (JUMP_DIST)),
					SINE (facing, WORLD_TO_VELOCITY (JUMP_DIST)));
			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

			StarShipPtr->special_counter = SPECIAL_WAIT;
		}
	}
}

static void
uninit_umgah (RACE_DESC *pRaceDesc)
{
	SetCustomShipData (pRaceDesc, NULL);
}

RACE_DESC*
init_umgah (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		umgah_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		umgah_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		umgah_desc.cyborg_control.WeaponRange = (LONG_RANGE_WEAPON_HD << 2);
	}

	umgah_desc.uninit_func = uninit_umgah;
	umgah_desc.preprocess_func = umgah_preprocess;
	umgah_desc.postprocess_func = umgah_postprocess;
	umgah_desc.init_weapon_func = initialize_cone;
	umgah_desc.cyborg_control.intelligence_func = umgah_intelligence;

	RaceDescPtr = &umgah_desc;

	return (RaceDescPtr);
}

