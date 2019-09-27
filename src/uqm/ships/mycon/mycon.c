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
#include "mycon.h"
#include "resinst.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 40
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 4
#define MAX_THRUST /* DISPLAY_TO_WORLD (7) */ 27
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 9
#define THRUST_WAIT 6
#define TURN_WAIT 6
#define SHIP_MASS 7

// Plasmoid
#define WEAPON_ENERGY_COST 20
#define WEAPON_WAIT 5
#define MYCON_OFFSET RES_SCALE(24)
#define MISSILE_OFFSET 0
#define NUM_PLASMAS 11
#define NUM_GLOBALLS 8
#define PLASMA_DURATION 13
#define MISSILE_LIFE (NUM_PLASMAS * PLASMA_DURATION)
#define MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(8))
#define MISSILE_DAMAGE 10
#define TRACK_WAIT 1

// Regenerate
#define SPECIAL_ENERGY_COST MAX_ENERGY
#define SPECIAL_WAIT 0
#define REGENERATION_AMOUNT 4

static RACE_DESC mycon_desc =
{
	{ /* SHIP_INFO */
		"podship",
		FIRES_FORE | SEEKING_WEAPON,
		21, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		MYCON_RACE_STRINGS,
		MYCON_ICON_MASK_PMAP_ANIM,
		MYCON_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		1070 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			6392, 2200,
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
			MYCON_BIG_MASK_PMAP_ANIM,
			MYCON_MED_MASK_PMAP_ANIM,
			MYCON_SML_MASK_PMAP_ANIM,
		},
		{
			PLASMA_BIG_MASK_PMAP_ANIM,
			PLASMA_MED_MASK_PMAP_ANIM,
			PLASMA_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			MYCON_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		MYCON_VICTORY_SONG,
		MYCON_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		DISPLAY_TO_WORLD (800),
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
plasma_preprocess (ELEMENT *ElementPtr)
{
	COUNT plasma_index;

	if (ElementPtr->mass_points > ElementPtr->hit_points)
		ElementPtr->life_span = ElementPtr->hit_points * PLASMA_DURATION;
	else
		ElementPtr->hit_points = (BYTE)((ElementPtr->life_span *
				MISSILE_DAMAGE + (MISSILE_LIFE - 1)) / MISSILE_LIFE);
	ElementPtr->mass_points = ElementPtr->hit_points;
	plasma_index = NUM_PLASMAS - ((ElementPtr->life_span +
			(PLASMA_DURATION - 1)) / PLASMA_DURATION);
	if (plasma_index != GetFrameIndex (ElementPtr->next.image.frame))
	{
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->next.image.frame,
				plasma_index);
		ElementPtr->state_flags |= CHANGING;
	}

	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)
				));
		if (TrackShip (ElementPtr, &facing) > 0)
			SetVelocityVector (&ElementPtr->velocity,
					MISSILE_SPEED, facing);

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}

static void
plasma_blast_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->life_span >= ElementPtr->thrust_wait)
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->next.image.frame);
	else
		ElementPtr->next.image.frame =
				DecFrameIndex (ElementPtr->next.image.frame);
	if (ElementPtr->hTarget)
	{
		ELEMENT *ShipPtr;

		LockElement (ElementPtr->hTarget, &ShipPtr);
		ElementPtr->next.location = ShipPtr->next.location;
		UnlockElement (ElementPtr->hTarget);
	}

	ElementPtr->state_flags |= CHANGING;
}

static void
plasma_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	SIZE old_mass;
	HELEMENT hBlastElement;

	old_mass = (SIZE)ElementPtr0->mass_points;
	if ((ElementPtr0->pParent != ElementPtr1->pParent
			|| (ElementPtr1->state_flags & PLAYER_SHIP))
			&& (hBlastElement =
			weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1)))
	{
		SIZE num_animations;
		ELEMENT *BlastElementPtr;

		LockElement (hBlastElement, &BlastElementPtr);
		BlastElementPtr->pParent = ElementPtr0->pParent;
		if (!(ElementPtr1->state_flags & PLAYER_SHIP))
			BlastElementPtr->hTarget = 0;
		else
		{
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr1, &StarShipPtr);
			BlastElementPtr->hTarget = StarShipPtr->hShip;
		}

		BlastElementPtr->current.location = ElementPtr1->current.location;

		if ((num_animations =
				(old_mass * NUM_GLOBALLS +
				(MISSILE_DAMAGE - 1)) / MISSILE_DAMAGE) == 0)
			num_animations = 1;

		BlastElementPtr->thrust_wait = (BYTE)num_animations;
		BlastElementPtr->life_span = (num_animations << 1) - 1;
		{
			BlastElementPtr->preprocess_func = plasma_blast_preprocess;
		}
		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
				SetAbsFrameIndex (BlastElementPtr->current.image.farray[0],
				NUM_PLASMAS);

		UnlockElement (hBlastElement);
	}
}

static void
mycon_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr && lpEvalDesc->MoveState == ENTICE)
	{
		if ((lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT))
			lpEvalDesc->MoveState = AVOID;
		else
			lpEvalDesc->MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState == PURSUE)
		StarShipPtr->ship_input_state &= ~THRUST; /* don't pursue seekers */

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->weapon_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& (lpEvalDesc->which_turn <= 16
			|| ShipPtr->crew_level == StarShipPtr->RaceDescPtr->ship_info.max_crew))
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
				- StarShipPtr->ShipFacing
				+ ANGLE_TO_FACING (QUADRANT))
				<= ANGLE_TO_FACING (HALF_CIRCLE)
				&& (!(StarShipPtr->cur_status_flags &
				(SHIP_BEYOND_MAX_SPEED | SHIP_IN_GRAVITY_WELL))
				|| NORMALIZE_FACING (direction_facing
				- travel_facing + ANGLE_TO_FACING (OCTANT))
				<= ANGLE_TO_FACING (QUADRANT)))
			StarShipPtr->ship_input_state |= WEAPON;
	}

	if (StarShipPtr->special_counter == 0)
	{
		StarShipPtr->ship_input_state &= ~SPECIAL;
		StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = DISPLAY_TO_WORLD (RES_SCALE(800));
		if (ShipPtr->crew_level < StarShipPtr->RaceDescPtr->ship_info.max_crew)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = MISSILE_SPEED * MISSILE_LIFE;
			if (StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
					&& !(StarShipPtr->ship_input_state & WEAPON))
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}
}

static COUNT
initialize_plasma (ELEMENT *ShipPtr, HELEMENT PlasmaArray[])
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
	MissileBlock.flags = 0;
	MissileBlock.pixoffs = MYCON_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_DAMAGE;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = plasma_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	PlasmaArray[0] = initialize_missile (&MissileBlock);

	if (PlasmaArray[0])
	{
		ELEMENT *PlasmaPtr;

		LockElement (PlasmaArray[0], &PlasmaPtr);
		PlasmaPtr->collision_func = plasma_collision;
		PlasmaPtr->turn_wait = TRACK_WAIT + 2;
		UnlockElement (PlasmaArray[0]);
	}

	return (1);
}

static void
mycon_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& ElementPtr->crew_level != StarShipPtr->RaceDescPtr->ship_info.max_crew
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		SIZE add_crew;

		ProcessSound (SetAbsSoundIndex (
						/* GROW_NEW_CREW */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		if ((add_crew = REGENERATION_AMOUNT) >
				StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level)
			add_crew = StarShipPtr->RaceDescPtr->ship_info.max_crew - ElementPtr->crew_level;
		DeltaCrew (ElementPtr, add_crew);
		
		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RACE_DESC*
init_mycon (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		mycon_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		mycon_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		mycon_desc.cyborg_control.WeaponRange = DISPLAY_TO_WORLD(3200);
	}

	mycon_desc.postprocess_func = mycon_postprocess;
	mycon_desc.init_weapon_func = initialize_plasma;
	mycon_desc.cyborg_control.intelligence_func = mycon_intelligence;

	RaceDescPtr = &mycon_desc;

	return (RaceDescPtr);
}

