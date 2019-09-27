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
#include "mmrnmhrm.h"
#include "resinst.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 10
#define SHIP_MASS 3

// X-Wing characteristics
#define ENERGY_REGENERATION 2
#define ENERGY_WAIT 6
#define MAX_THRUST 20
#define THRUST_INCREMENT 5
#define THRUST_WAIT 1
#define TURN_WAIT 2

// Y-Wing characteristics
#define YWING_ENERGY_REGENERATION 1
#define YWING_SPECIAL_ENERGY_COST MAX_ENERGY
#define YWING_ENERGY_WAIT 6
#define YWING_MAX_THRUST RES_SCALE(50)
#define YWING_THRUST_INCREMENT RES_SCALE(10)
#define YWING_THRUST_WAIT 0
#define YWING_TURN_WAIT 14

// X-Wing Lasers
#define MMRNMHRM_OFFSET RES_SCALE(16)
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define CENTER_OFFS DISPLAY_TO_WORLD (RES_SCALE(4))
#define WING_OFFS DISPLAY_TO_WORLD (10)
#define LASER_RANGE DISPLAY_TO_WORLD (RES_SCALE(125) + MMRNMHRM_OFFSET)

// Y-Wing Missiles
#define YWING_WEAPON_ENERGY_COST 1
#define YWING_WEAPON_WAIT 20
#define LAUNCH_OFFS DISPLAY_TO_WORLD (RES_SCALE(4))
#define MISSILE_OFFSET 0
#define MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(20))
#define MISSILE_LIFE 40
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1
#define TRACK_WAIT 5

// Transform
#define SPECIAL_ENERGY_COST MAX_ENERGY
#define SPECIAL_WAIT 0
#define YWING_SPECIAL_WAIT 0

static RACE_DESC mmrnmhrm_desc =
{
	{ /* SHIP_INFO */
		"xform",
		FIRES_FORE | IMMEDIATE_WEAPON,
		19, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		MMRNMHRM_RACE_STRINGS,
		MMRNMHRM_ICON_MASK_PMAP_ANIM,
		MMRNMHRM_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			0, 0,
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
			MMRNMHRM_BIG_MASK_PMAP_ANIM,
			MMRNMHRM_MED_MASK_PMAP_ANIM,
			MMRNMHRM_SML_MASK_PMAP_ANIM,
		},
		{
			TORP_BIG_MASK_PMAP_ANIM,
			TORP_MED_MASK_PMAP_ANIM,
			TORP_SML_MASK_PMAP_ANIM,
		},
		{
			YWING_BIG_MASK_PMAP_ANIM,
			YWING_MED_MASK_PMAP_ANIM,
			YWING_SML_MASK_PMAP_ANIM,
		},
		{
			MMRNMHRM_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		MMRNMHRM_VICTORY_SONG,
		MMRNMHRM_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
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
typedef CHARACTERISTIC_STUFF MMRNMHRM_DATA;

// Local typedef
typedef MMRNMHRM_DATA CustomShipData_t;

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
missile_preprocess (ELEMENT *ElementPtr)
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
					MISSILE_SPEED, facing);
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}

static void
mmrnmhrm_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	BOOLEAN CanTransform;
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;
	STARSHIP *EnemyStarShipPtr = NULL;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	CanTransform = (BOOLEAN)(StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >=
			StarShipPtr->RaceDescPtr->characteristics.special_energy_cost);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	StarShipPtr->ship_input_state &= ~SPECIAL;
	if (CanTransform
			&& lpEvalDesc->ObjectPtr
			&& !(StarShipPtr->ship_input_state & WEAPON))
	{
		SDWORD delta_x, delta_y;
		COUNT travel_angle, direction_angle;

		GetCurrentVelocityComponentsSdword (&lpEvalDesc->ObjectPtr->velocity,
				&delta_x, &delta_y);
		if (delta_x == 0 && delta_y == 0)
			direction_angle = travel_angle = 0;
		else
		{
			delta_x = lpEvalDesc->ObjectPtr->current.location.x
					- ShipPtr->current.location.x;
			delta_y = lpEvalDesc->ObjectPtr->current.location.y
					- ShipPtr->current.location.y;
			direction_angle = ARCTAN (-delta_x, -delta_y);
			travel_angle = GetVelocityTravelAngle (
					&lpEvalDesc->ObjectPtr->velocity
					);
		}

		if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
		{
			if (lpEvalDesc->which_turn > 8)
			{
				if (MANEUVERABILITY (&EnemyStarShipPtr->RaceDescPtr->cyborg_control) <= RESOLUTION_COMPENSATED(SLOW_SHIP) // JMS_GFX
						|| NORMALIZE_ANGLE (
								direction_angle - travel_angle + QUADRANT
								) > HALF_CIRCLE)
					StarShipPtr->ship_input_state |= SPECIAL;
			}
		}
		else
		{
			SDWORD ship_delta_x, ship_delta_y;

			GetCurrentVelocityComponentsSdword (&ShipPtr->velocity,
					&ship_delta_x, &ship_delta_y);
			delta_x -= ship_delta_x;
			delta_y -= ship_delta_y;
			travel_angle = ARCTAN (delta_x, delta_y);
			if (lpEvalDesc->which_turn < 16)
			{
				if (lpEvalDesc->which_turn <= 8
						|| NORMALIZE_ANGLE (
								direction_angle - travel_angle + OCTANT
								) <= QUADRANT)
					StarShipPtr->ship_input_state |= SPECIAL;
			}
			else if (lpEvalDesc->which_turn > 32
					&& NORMALIZE_ANGLE (
							direction_angle - travel_angle + QUADRANT
							) > HALF_CIRCLE)
				StarShipPtr->ship_input_state |= SPECIAL;
		}
	}

	if (ShipPtr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
	{
		if (!(StarShipPtr->ship_input_state & SPECIAL)
				&& lpEvalDesc->ObjectPtr)
			StarShipPtr->ship_input_state |= WEAPON;
		else
			StarShipPtr->ship_input_state &= ~WEAPON;
	}
}

static void
twin_laser_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (!(ElementPtr1->state_flags & PLAYER_SHIP)
			|| !elementsOfSamePlayer (ElementPtr0, ElementPtr1))
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_dual_weapons (ELEMENT *ShipPtr, HELEMENT WeaponArray[])
{
	COORD cx, cy;
	COUNT facing, angle;
	SDWORD offs_x, offs_y;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	facing = StarShipPtr->ShipFacing;
	angle = FACING_TO_ANGLE (facing);
	cx = ShipPtr->next.location.x + COSINE (angle, CENTER_OFFS);
	cy = ShipPtr->next.location.y + SINE (angle, CENTER_OFFS);

	if (ShipPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
	{
		COORD ex, ey;
		LASER_BLOCK LaserBlock;
		ELEMENT *LaserPtr;

		LaserBlock.sender = ShipPtr->playerNr;
		LaserBlock.flags = 0;
		LaserBlock.pixoffs = 0;
		LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C);
		LaserBlock.face = facing;

		ex = cx + COSINE (angle, LASER_RANGE);
		ey = cy + SINE (angle, LASER_RANGE);
		offs_x = -SINE (angle, WING_OFFS);
		offs_y = COSINE (angle, WING_OFFS);

		LaserBlock.cx = cx + offs_x;
		LaserBlock.cy = cy + offs_y;
		LaserBlock.ex = ex - LaserBlock.cx;
		LaserBlock.ey = ey - LaserBlock.cy;
		if ((WeaponArray[0] = initialize_laser (&LaserBlock)))
		{
			LockElement (WeaponArray[0], &LaserPtr);
			LaserPtr->collision_func = twin_laser_collision;
			UnlockElement (WeaponArray[0]);
		}

		LaserBlock.cx = cx - offs_x;
		LaserBlock.cy = cy - offs_y;
		LaserBlock.ex = ex - LaserBlock.cx;
		LaserBlock.ey = ey - LaserBlock.cy;
		if ((WeaponArray[1] = initialize_laser (&LaserBlock)))
		{
			LockElement (WeaponArray[1], &LaserPtr);
			LaserPtr->collision_func = twin_laser_collision;
			UnlockElement (WeaponArray[1]);
		}
	}
	else
	{
		MISSILE_BLOCK TorpBlock;
		ELEMENT *TorpPtr;

		TorpBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
		TorpBlock.sender = ShipPtr->playerNr;
		TorpBlock.flags = IGNORE_SIMILAR;
		TorpBlock.pixoffs = 0;
		TorpBlock.speed = MISSILE_SPEED;
		TorpBlock.hit_points = MISSILE_HITS;
		TorpBlock.damage = MISSILE_DAMAGE;
		TorpBlock.life = MISSILE_LIFE;
		TorpBlock.preprocess_func = missile_preprocess;
		TorpBlock.blast_offs = MISSILE_OFFSET;

		TorpBlock.face = TorpBlock.index = NORMALIZE_FACING (facing - 1);
		offs_x = -SINE (FACING_TO_ANGLE (TorpBlock.face), LAUNCH_OFFS);
		offs_y = COSINE (FACING_TO_ANGLE (TorpBlock.face), LAUNCH_OFFS);

		TorpBlock.cx = cx + offs_x;
		TorpBlock.cy = cy + offs_y;
		if ((WeaponArray[0] = initialize_missile (&TorpBlock)))
		{
			LockElement (WeaponArray[0], &TorpPtr);
			TorpPtr->turn_wait = TRACK_WAIT;
			UnlockElement (WeaponArray[0]);
		}

		TorpBlock.face = TorpBlock.index = NORMALIZE_FACING (facing + 1);

		TorpBlock.cx = cx - offs_x;
		TorpBlock.cy = cy - offs_y;
		if ((WeaponArray[1] = initialize_missile (&TorpBlock)))
		{
			LockElement (WeaponArray[1], &TorpPtr);
			TorpPtr->turn_wait = TRACK_WAIT;
			UnlockElement (WeaponArray[1]);
		}
	}

	return (2);
}

static void
mmrnmhrm_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
			/* take care of transform effect */
	if (ElementPtr->next.image.farray != ElementPtr->current.image.farray)
	{
		MMRNMHRM_DATA tempShipData;
		MMRNMHRM_DATA *otherwing_desc;

		ProcessSound (SetAbsSoundIndex (
						/* TRANSFORM */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		StarShipPtr->weapon_counter = 0;

		/* Swap characteristics descriptors around */
		otherwing_desc = GetCustomShipData (StarShipPtr->RaceDescPtr);
		if (!otherwing_desc)
			return;  // No ship data (?!)

		tempShipData = *otherwing_desc;
		SetCustomShipData (StarShipPtr->RaceDescPtr, &StarShipPtr->RaceDescPtr->characteristics);
		StarShipPtr->RaceDescPtr->characteristics = tempShipData;
		StarShipPtr->RaceDescPtr->cyborg_control.ManeuverabilityIndex = 0;

		if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.special)
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = RES_SCALE(LONG_RANGE_WEAPON - 1);
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~IMMEDIATE_WEAPON;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= SEEKING_WEAPON;
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds =
					SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2);

			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
		}
		else
		{
			StarShipPtr->RaceDescPtr->cyborg_control.WeaponRange = RES_SCALE(CLOSE_RANGE_WEAPON);
			StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~SEEKING_WEAPON;
			StarShipPtr->RaceDescPtr->ship_info.ship_flags |= IMMEDIATE_WEAPON;
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds =
					SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 0);

			if (StarShipPtr->cur_status_flags
					& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
				StarShipPtr->cur_status_flags |=
						SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED;
		}
	}
}

static void
mmrnmhrm_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (!(ElementPtr->state_flags & APPEARING))
	{
		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& StarShipPtr->special_counter == 0)
		{
			/* Either we transform or text will flash */
			if (DeltaEnergy (ElementPtr,
					-StarShipPtr->RaceDescPtr->characteristics.special_energy_cost))
			{
				if (ElementPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.ship)
					ElementPtr->next.image.farray =
							StarShipPtr->RaceDescPtr->ship_data.special;
				else
					ElementPtr->next.image.farray =
							StarShipPtr->RaceDescPtr->ship_data.ship;
				ElementPtr->next.image.frame =
						SetEquFrameIndex (ElementPtr->next.image.farray[0],
						ElementPtr->next.image.frame);
				ElementPtr->state_flags |= CHANGING;
	
				StarShipPtr->special_counter =
						StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}
		}
	}
}

static void
uninit_mmrnmhrm (RACE_DESC *pRaceDesc)
{
	SetCustomShipData (pRaceDesc, NULL);
}

RACE_DESC*
init_mmrnmhrm (void)
{
	RACE_DESC *RaceDescPtr;

	// The caller of this func will copy the struct
	static RACE_DESC new_mmrnmhrm_desc;
	MMRNMHRM_DATA otherwing_desc;

	if (IS_HD) {
		mmrnmhrm_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		mmrnmhrm_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		mmrnmhrm_desc.cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON_HD;
	}

	mmrnmhrm_desc.uninit_func = uninit_mmrnmhrm;
	mmrnmhrm_desc.preprocess_func = mmrnmhrm_preprocess;
	mmrnmhrm_desc.postprocess_func = mmrnmhrm_postprocess;
	mmrnmhrm_desc.init_weapon_func = initialize_dual_weapons;
	mmrnmhrm_desc.cyborg_control.intelligence_func = mmrnmhrm_intelligence;

	new_mmrnmhrm_desc = mmrnmhrm_desc;

	otherwing_desc.max_thrust = YWING_MAX_THRUST;
	otherwing_desc.thrust_increment = YWING_THRUST_INCREMENT;
	otherwing_desc.energy_regeneration = YWING_ENERGY_REGENERATION;
	otherwing_desc.weapon_energy_cost = YWING_WEAPON_ENERGY_COST;
	otherwing_desc.special_energy_cost = YWING_SPECIAL_ENERGY_COST;
	otherwing_desc.energy_wait = YWING_ENERGY_WAIT;
	otherwing_desc.turn_wait = YWING_TURN_WAIT;
	otherwing_desc.thrust_wait = YWING_THRUST_WAIT;
	otherwing_desc.weapon_wait = YWING_WEAPON_WAIT;
	otherwing_desc.special_wait = YWING_SPECIAL_WAIT;
	otherwing_desc.ship_mass = SHIP_MASS;

	SetCustomShipData (&new_mmrnmhrm_desc, &otherwing_desc);

	RaceDescPtr = &new_mmrnmhrm_desc;

	return (RaceDescPtr);
}

