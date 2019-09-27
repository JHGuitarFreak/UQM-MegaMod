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
#include "sis_ship.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/controls.h"
#include "uqm/globdata.h"
#include "uqm/hyper.h"
#include "libs/mathlib.h"
#include "../../setup.h"

/* Core characteristics.
 * All of these are changed at init time by some module, except for
 * MAX_ENERGY, THRUST_INCREMENT, and SHIP_MASS. */

#define MAX_CREW MAX_CREW_SIZE
		/* This value gets thrown out - actual max crew is determined by the
		 * number of crew pods. The minimum value is 1 (just the Captain). */

#define MAX_ENERGY MAX_ENERGY_SIZE
#define ENERGY_REGENERATION 1
		/* Shiva furnaces increase this by 1 each. */
#define SHIVA_ENERGY_REGEN_INC 1

#define ENERGY_WAIT 10
		/* Dynamos decrease this by 2 each, to a minimum of 4. */
#define MIN_ENERGY_WAIT 4
#define DYNAMO_UNIT_ENERGY_WAIT_DEC 2

#define MAX_THRUST 10
		/* Thrusters increase this and decrease THRUST_WAIT based on
		 * THRUST_INCREMENT, see InitDriveSlots near the bottom of this file
		 * for details. */
#define THRUST_INCREMENT 4
#define THRUST_WAIT 6
#define TURN_WAIT 17
		/* Turning jets decrease by 2 each */
#define SHIP_MASS MAX_SHIP_MASS


/* Primary weapon - energy cost and damage change at init time based on
 * the number and type of weapon modules installed. */

#define BLASTER_DAMAGE 2
		/* This is the damage value for the basic ion bolt guns. Fusion
		 * blasters and hellbore cannons end up doing (BLASTER_DAMAGE * 2)
		 * and (BLASTER_DAMAGE * 3) damage, respectively, but this depends
		 * on enum values. */

#define BLASTER_HITS 2 /* Hitpoints for ion bolt guns, see BLASTER_DAMAGE */

#define WEAPON_ENERGY_COST 1
		/* This value gets thrown out and reset in an ugly manner based on
		 * the enum that is used for module IDs. Bigger gun = higher value.
		 */
#define WEAPON_WAIT 6
#define BLASTER_SPEED DISPLAY_TO_WORLD (24)
#define BLASTER_LIFE 12
		/* This value is greatly increased, based in part on the enum used
		 * for module IDs (bigger gun == longer life). See the first half of
		 * InitWeaponSlots */
#define MAX_TRACKING 3
#define TRACKER_ENERGY_COST IF_HARD(3, 6)
#define BLASTER_OFFSET RES_SCALE(8)
#define SIS_VERT_OFFSET RES_SCALE(28)
		/* Used for foward, spread, and rear slots */
#define SIS_HORZ_OFFSET RES_SCALE(20)
		/* Used for side slot */

/* Secondary weapon */
#define SPECIAL_ENERGY_COST 0
		/* Increased by 1 for each point defense module */
#define ANTIMISSILE_ENERGY_INC 1
#define SPECIAL_WAIT 9
#define LASER_RANGE (UWORD)RES_SCALE(100)
#define MAX_DEFENSE 8

// HD
#define BLASTER_SPEED_HD DISPLAY_TO_WORLD (96)
#define BLASTER_SPEED_BOOL RES_BOOL(BLASTER_SPEED, BLASTER_SPEED_HD)


static RACE_DESC sis_desc =
{
	{ /* SHIP_INFO */
		"flagship",
		0,
		16, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		NULL_RESOURCE,
		SIS_ICON_MASK_PMAP_ANIM,
		NULL_RESOURCE,
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
			SIS_BIG_MASK_PMAP_ANIM,
			SIS_MED_MASK_PMAP_ANIM,
			SIS_SML_MASK_PMAP_ANIM,
		},
		{
			BLASTER_BIG_MASK_PMAP_ANIM,
			BLASTER_MED_MASK_PMAP_ANIM,
			BLASTER_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			SIS_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SIS_VICTORY_SONG,
		SIS_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		BLASTER_SPEED * BLASTER_LIFE,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

// Private per-instance SIS data
typedef struct
{
	COUNT num_trackers;
	COUNT num_blasters;
	MISSILE_BLOCK MissileBlock[6];

} SIS_DATA;

static void InitWeaponSlots (RACE_DESC *RaceDescPtr,
		const BYTE *ModuleSlots);
static void InitModuleSlots (RACE_DESC *RaceDescPtr,
		const BYTE *ModuleSlots);
static void InitDriveSlots (RACE_DESC *RaceDescPtr,
		const BYTE *DriveSlots);
static void InitJetSlots (RACE_DESC *RaceDescPtr,
		const BYTE *JetSlots);
static void uninit_sis (RACE_DESC *pRaceDesc);


// Local typedef
typedef SIS_DATA CustomShipData_t;

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
sis_hyper_preprocess (ELEMENT *ElementPtr)
{
	SIZE dx = 0, dy = 0;
	SDWORD udx = 0, udy = 0, dtempx, dtempy;
	SIZE AccelerateDirection;
	STARSHIP *StarShipPtr;

	if (ElementPtr->state_flags & APPEARING)
		ElementPtr->velocity = GLOBAL (velocity);
	
	AccelerateDirection = 0;
	GetElementStarShip (ElementPtr, &StarShipPtr);
	++StarShipPtr->weapon_counter; /* no shooting in hyperspace! */
	
	if ((GLOBAL (autopilot)).x == ~0
		|| (GLOBAL (autopilot)).y == ~0
		|| (StarShipPtr->cur_status_flags & (LEFT | RIGHT | THRUST))
		|| !(GET_GAME_STATE(AUTOPILOT_OK))) // JMS: This check makes autopilot engage only after coming to full stop
	{
	LeaveAutoPilot:
		
		// JMS: This re-check is now needed because of the added autopilot_ok variable to previous check
		if ((GLOBAL (autopilot)).x == ~0 || (GLOBAL (autopilot)).y == ~0 || (StarShipPtr->cur_status_flags & (LEFT | RIGHT | THRUST)))
			(GLOBAL (autopilot)).x = (GLOBAL (autopilot)).y = ~0;
		
		if (!(StarShipPtr->cur_status_flags & THRUST)
			|| (GLOBAL_SIS (FuelOnBoard) == 0
				&& (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)))
		{
			AccelerateDirection = -1;
			GetCurrentVelocityComponents (&ElementPtr->velocity, &dx, &dy);
			
			// JMS: Engage autopilot only after coming to full stop
			if (dx==0 && dy==0)
				SET_GAME_STATE (AUTOPILOT_OK, 1);
			else
				SET_GAME_STATE (AUTOPILOT_OK, 0);
			
			dtempx = (SDWORD)dx;
			dtempy = (SDWORD)dy;
			
			udx = dtempx;
			udy = dtempy;
			
			StarShipPtr->cur_status_flags &= ~THRUST;
		}
	}
	else
	{
		SIZE facing;
		POINT universe;

		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		udx = (GLOBAL (autopilot)).x - universe.x;
		udy = -((GLOBAL (autopilot)).y - universe.y);
		if ((dx = (SIZE)udx) < 0)
			dx = -dx;
		if ((dy = (SIZE)udy) < 0)
			dy = -dy;
		if (dx <= RES_SCALE(1) && dy <= RES_SCALE(1))
			goto LeaveAutoPilot;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (ARCTAN (udx, udy)));

		/* This prevents ship from flying backwards on auto-pilot.
		 * It could also theoretically abort autopilot in a bad savegame */
		if ((StarShipPtr->cur_status_flags & SHIP_AT_MAX_SPEED)
				/*|| (ElementPtr->state_flags & APPEARING)*/ )
		{
			if (NORMALIZE_FACING (StarShipPtr->ShipFacing
					+ ANGLE_TO_FACING (QUADRANT)
					- facing) > ANGLE_TO_FACING (HALF_CIRCLE))
				goto LeaveAutoPilot;

			facing = StarShipPtr->ShipFacing;
		}
		else if ((int)facing != (int)StarShipPtr->ShipFacing
				&& ElementPtr->turn_wait == 0)
		{
			if (NORMALIZE_FACING (
					StarShipPtr->ShipFacing - facing
					) >= ANGLE_TO_FACING (HALF_CIRCLE))
			{
				facing = NORMALIZE_FACING (facing - 1);
				StarShipPtr->cur_status_flags |= RIGHT;
			}
			else if ((int)StarShipPtr->ShipFacing != (int)facing)
			{
				facing = NORMALIZE_FACING (facing + 1);
				StarShipPtr->cur_status_flags |= LEFT;
			}

			if ((int)facing == (int)StarShipPtr->ShipFacing)
			{
				ZeroVelocityComponents (&ElementPtr->velocity);
			}
		}

		GetCurrentVelocityComponents (&ElementPtr->velocity, &dx, &dy);
		if ((GLOBAL_SIS (FuelOnBoard)
				|| GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
				&& (int)facing == (int)StarShipPtr->ShipFacing)
		{
			StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;
			AccelerateDirection = 1;
		}
		else
		{
			AccelerateDirection = -1;
			udx = dx;// << 4;
			udy = dy;// << 4;
		}
	}

	if (ElementPtr->thrust_wait == 0 && AccelerateDirection)
	{
		COUNT dist;
		SIZE speed, velocity_increment;

		velocity_increment = WORLD_TO_VELOCITY (
				StarShipPtr->RaceDescPtr->characteristics.thrust_increment);

		if ((dist = square_root ((long)udx * udx + (long)udy * udy)) == 0)
			dist = 1; /* prevent divide by zero */

		speed = square_root ((long)dx * dx + (long)dy * dy);
		if (AccelerateDirection < 0)
		{
			dy = (speed / velocity_increment - 1) * velocity_increment;
			if (dy < speed - velocity_increment)
				dy = speed - velocity_increment;
			if ((speed = dy) < 0)
				speed = 0;

			StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
		}
		else
		{
			SIZE max_velocity;

			AccelerateDirection = 0;

			max_velocity = WORLD_TO_VELOCITY (StarShipPtr->RaceDescPtr->characteristics.max_thrust);
			dy = (speed / velocity_increment + 1) * velocity_increment;

			if (dy < speed + velocity_increment)
				dy = speed + velocity_increment;
			if ((speed = dy) > max_velocity)
			{
				speed = max_velocity;
				StarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;
			}
		}

		dtempx = (SDWORD)((long)udx * speed / (long)dist);
		dtempy = (SDWORD)((long)udy * speed / (long)dist);
		
		SetVelocityComponents (&ElementPtr->velocity, dtempx, dtempy);
		ElementPtr->thrust_wait =StarShipPtr->RaceDescPtr->characteristics.thrust_wait;
	}
}

static void
sis_hyper_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GLOBAL (velocity) = ElementPtr->velocity;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (((StarShipPtr->cur_status_flags & WEAPON) ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
			&& StarShipPtr->special_counter == 0)
	{
#define MENU_DELAY 10
		HyperspaceMenu ();
		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
		StarShipPtr->special_counter = MENU_DELAY;
	}
}

static void
spawn_point_defense (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hDefense;

		hDefense = AllocElement ();
		if (hDefense)
		{
			ELEMENT *DefensePtr;

			LockElement (hDefense, &DefensePtr);
			DefensePtr->playerNr = ElementPtr->playerNr;
			DefensePtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE;
			DefensePtr->death_func = spawn_point_defense;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			SetElementStarShip (DefensePtr, StarShipPtr);
			UnlockElement (hDefense);

			PutElement (hDefense);
		}
	}
	else
	{
		BOOLEAN PaidFor;
		HELEMENT hObject, hNextObject;
		ELEMENT *ShipPtr;
		Color LaserColor;
		static const Color ColorRange[] =
		{
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7F),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7E),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x00), 0x7D),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7C),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7B),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7A),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x18, 0x00), 0x79),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x1C, 0x00), 0x78),
		};

		PaidFor = FALSE;

		LaserColor = ColorRange[
				StarShipPtr->RaceDescPtr->characteristics.special_energy_cost
				];
		LockElement (StarShipPtr->hShip, &ShipPtr);
		for (hObject = GetTailElement (); hObject; hObject = hNextObject)
		{
			ELEMENT *ObjectPtr;

			LockElement (hObject, &ObjectPtr);
			hNextObject = GetPredElement (ObjectPtr);
			if (ObjectPtr != ShipPtr && CollidingElement (ObjectPtr) &&
					!OBJECT_CLOAKED (ObjectPtr))
			{
				SDWORD delta_x, delta_y;

				delta_x = ObjectPtr->next.location.x -
						ShipPtr->next.location.x;
				delta_y = ObjectPtr->next.location.y -
						ShipPtr->next.location.y;
				if (delta_x < 0)
					delta_x = -delta_x;
				if (delta_y < 0)
					delta_y = -delta_y;
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				if ((UWORD)delta_x <= LASER_RANGE &&
						(UWORD)delta_y <= LASER_RANGE &&
						(UWORD)delta_x * (UWORD)delta_x +
						(UWORD)delta_y * (UWORD)delta_y <=
						LASER_RANGE * LASER_RANGE)
				{
					HELEMENT hPointDefense;
					LASER_BLOCK LaserBlock;

					if (!PaidFor)
					{
						if (!DeltaEnergy (ShipPtr,
								-(StarShipPtr->RaceDescPtr->characteristics.special_energy_cost
								<< 2)))
							break;

						ProcessSound (SetAbsSoundIndex (
										/* POINT_DEFENSE_LASER */
								StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
						StarShipPtr->special_counter =
								StarShipPtr->RaceDescPtr->characteristics.special_wait;
						PaidFor = TRUE;
					}

					LaserBlock.cx = ShipPtr->next.location.x;
					LaserBlock.cy = ShipPtr->next.location.y;
					LaserBlock.face = 0;
					LaserBlock.ex = ObjectPtr->next.location.x
							- ShipPtr->next.location.x;
					LaserBlock.ey = ObjectPtr->next.location.y
							- ShipPtr->next.location.y;
					LaserBlock.sender = ShipPtr->playerNr;
					LaserBlock.flags = IGNORE_SIMILAR;
					LaserBlock.pixoffs = 0;
					LaserBlock.color = LaserColor;
					hPointDefense = initialize_laser (&LaserBlock);
					if (hPointDefense)
					{
						ELEMENT *PDPtr;

						LockElement (hPointDefense, &PDPtr);
						PDPtr->mass_points =
								StarShipPtr->RaceDescPtr->characteristics.special_energy_cost;
						SetElementStarShip (PDPtr, StarShipPtr);
						PDPtr->hTarget = 0;
						UnlockElement (hPointDefense);

						PutElement (hPointDefense);
					}
				}
			}
			UnlockElement (hObject);
		}
		UnlockElement (StarShipPtr->hShip);
	}
}

static void
sis_battle_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.special_energy_cost == 0)
	{
		StarShipPtr->cur_status_flags &= ~SPECIAL;
		StarShipPtr->special_counter = 2;
	}
	if (!(StarShipPtr->RaceDescPtr->ship_info.ship_flags
			& (FIRES_FORE | FIRES_RIGHT | FIRES_AFT | FIRES_LEFT)))
	{
		StarShipPtr->cur_status_flags &= ~WEAPON;
		StarShipPtr->weapon_counter = 2;
	}
}

static void
sis_battle_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& StarShipPtr->RaceDescPtr->characteristics.special_energy_cost)
	{
		spawn_point_defense (ElementPtr);
	}
}

static void
blaster_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	HELEMENT hBlastElement;

	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		ELEMENT *BlastElementPtr;

		LockElement (hBlastElement, &BlastElementPtr);
		switch (ElementPtr0->mass_points)
		{
			case BLASTER_DAMAGE * 1:
				BlastElementPtr->life_span = 2;
				BlastElementPtr->current.image.frame =
						SetAbsFrameIndex (ElementPtr0->current.image.frame, 0);
				BlastElementPtr->preprocess_func = NULL;
				break;
			case BLASTER_DAMAGE * 2:
				BlastElementPtr->life_span = 6;
				BlastElementPtr->current.image.frame =
						IncFrameIndex (ElementPtr0->current.image.frame);
				break;
			case BLASTER_DAMAGE * 3:
				BlastElementPtr->life_span = 7;
				BlastElementPtr->current.image.frame =
						SetAbsFrameIndex (ElementPtr0->current.image.frame, 20);
				break;
		}
		UnlockElement (hBlastElement);
	}
}

static void
blaster_preprocess (ELEMENT *ElementPtr)
{
	BYTE wait;

	switch (ElementPtr->mass_points)
	{
		case BLASTER_DAMAGE * 1:
			if (GetFrameIndex (ElementPtr->current.image.frame) < 8)
			{
				ElementPtr->next.image.frame =
						IncFrameIndex (ElementPtr->current.image.frame);
				ElementPtr->state_flags |= CHANGING;
			}
			break;
		case BLASTER_DAMAGE * 3:
			if (GetFrameIndex (ElementPtr->current.image.frame) < 19)
				ElementPtr->next.image.frame =
						IncFrameIndex (ElementPtr->current.image.frame);
			else
				ElementPtr->next.image.frame =
						SetAbsFrameIndex (ElementPtr->current.image.frame, 16);
			ElementPtr->state_flags |= CHANGING;
			break;
	}

	if (LONIBBLE (ElementPtr->turn_wait))
		--ElementPtr->turn_wait;
	else if ((wait = HINIBBLE (ElementPtr->turn_wait)))
	{
		COUNT facing;

		facing = NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)));
		if (TrackShip (ElementPtr, &facing) > 0)
			SetVelocityVector (&ElementPtr->velocity, BLASTER_SPEED_BOOL, facing);

		ElementPtr->turn_wait = MAKE_BYTE (wait, wait);
	}
}

static COUNT
initialize_blasters (ELEMENT *ShipPtr, HELEMENT BlasterArray[])
{
	BYTE nt;
	COUNT i;
	STARSHIP *StarShipPtr;
	SIS_DATA *SisData;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	SisData = GetCustomShipData (StarShipPtr->RaceDescPtr);

	nt = (BYTE)((4 - SisData->num_trackers) & 3);

	for (i = 0; i < SisData->num_blasters; ++i)
	{
		MISSILE_BLOCK MissileBlock = SisData->MissileBlock[i];

		MissileBlock.cx = ShipPtr->next.location.x;
		MissileBlock.cy = ShipPtr->next.location.y;
		MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
		MissileBlock.sender = ShipPtr->playerNr;
		MissileBlock.face = NORMALIZE_FACING (StarShipPtr->ShipFacing
				+ MissileBlock.face);

		BlasterArray[i] = initialize_missile (&MissileBlock);
		if (BlasterArray[i])
		{
			ELEMENT *BlasterPtr;

			LockElement (BlasterArray[i], &BlasterPtr);
			BlasterPtr->collision_func = blaster_collision;
			BlasterPtr->turn_wait = MAKE_BYTE (nt, nt);
			UnlockElement (BlasterArray[i]);
		}
	
	}

	return SisData->num_blasters;
}

static void
sis_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;
	SIS_DATA *SisData;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	SisData = GetCustomShipData (StarShipPtr->RaceDescPtr);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->characteristics.special_energy_cost)
		{
			if (StarShipPtr->special_counter == 0
					&& ((lpEvalDesc->ObjectPtr
					&& lpEvalDesc->which_turn <= 2)
					|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr != NULL
					&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 4)))
				StarShipPtr->ship_input_state |= SPECIAL;
			else
				StarShipPtr->ship_input_state &= ~SPECIAL;
			lpEvalDesc->ObjectPtr = NULL;
		}
		else if (MANEUVERABILITY (&StarShipPtr->RaceDescPtr->cyborg_control)
				< RESOLUTION_COMPENSATED(MEDIUM_SHIP) // JMS_GFX
				&& lpEvalDesc->MoveState == ENTICE
				&& (!(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT)
				|| lpEvalDesc->which_turn <= 8)
				&& (!(lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				|| (lpEvalDesc->ObjectPtr->mass_points >= 4
				&& lpEvalDesc->which_turn == 2
				&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 16)))
			lpEvalDesc->MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (SisData->num_trackers
			&& StarShipPtr->weapon_counter == 0
			&& !(StarShipPtr->ship_input_state & WEAPON)
			&& lpEvalDesc->ObjectPtr
			&& lpEvalDesc->which_turn <= 16)
	{
		COUNT direction_facing;
		SDWORD delta_x, delta_y;
		UWORD fire_flags, ship_flags;
		COUNT facing;

		delta_x = lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x;
		delta_y = lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y;
		direction_facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (ARCTAN (delta_x, delta_y)));

		ship_flags = StarShipPtr->RaceDescPtr->ship_info.ship_flags;
		for (fire_flags = FIRES_FORE, facing = StarShipPtr->ShipFacing;
				fire_flags <= FIRES_LEFT;
				fire_flags <<= 1, facing += QUADRANT)
		{
			if ((ship_flags & fire_flags) && NORMALIZE_FACING (
					direction_facing - facing + ANGLE_TO_FACING (OCTANT)
					) <= ANGLE_TO_FACING (QUADRANT))
			{
				StarShipPtr->ship_input_state |= WEAPON;
				break;
			}
		}
	}
}

static void
InitWeaponSlots (RACE_DESC *RaceDescPtr, const BYTE *ModuleSlots)
{
	COUNT i;
	SIS_DATA *SisData = GetCustomShipData (RaceDescPtr);
	MISSILE_BLOCK *lpMB = SisData->MissileBlock;

	SisData->num_blasters = 0;
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		COUNT which_gun;
		BOOLEAN IfHard = DIF_HARD && (i == 1 || i == 2) ? TRUE : FALSE;

		if (i == 3)
			i = NUM_MODULE_SLOTS - 1;
		
		which_gun = ModuleSlots[(NUM_MODULE_SLOTS - 1) - i];
		
		if (which_gun < GUN_WEAPON || which_gun > CANNON_WEAPON)
			continue; /* not a gun */

		which_gun -= GUN_WEAPON - 1;
		RaceDescPtr->characteristics.weapon_energy_cost +=
				which_gun * (IfHard ? 4 : 2);
		
		lpMB->flags = IGNORE_SIMILAR;
		lpMB->blast_offs = BLASTER_OFFSET;
		lpMB->speed = BLASTER_SPEED_BOOL;
		lpMB->preprocess_func = blaster_preprocess;
		lpMB->hit_points = BLASTER_HITS * which_gun;
		lpMB->damage = BLASTER_DAMAGE * which_gun;
		lpMB->life = BLASTER_LIFE + ((BLASTER_LIFE >> 2) * (which_gun - 1));

		if (which_gun == 1)
			lpMB->index = 0;
		else if (which_gun == 2)
			lpMB->index = 9;
		else
			lpMB->index = 16;

		switch (i)
		{
			case 0: /* NOSE WEAPON */
				RaceDescPtr->ship_info.ship_flags |= FIRES_FORE;
				lpMB->pixoffs = SIS_VERT_OFFSET;
				lpMB->face = 0;
				break;
			case 1: /* SPREAD WEAPON */
				RaceDescPtr->ship_info.ship_flags |= FIRES_FORE;
				lpMB->pixoffs = SIS_VERT_OFFSET;
				lpMB->face = +1;
				/* copy it because there are two */
				lpMB[1] = lpMB[0];
				++lpMB;
				++SisData->num_blasters;
				lpMB->face = NORMALIZE_FACING (-1);
				break;
			case 2: /* SIDE WEAPON */
				RaceDescPtr->ship_info.ship_flags |=
						FIRES_LEFT | FIRES_RIGHT;
				lpMB->pixoffs = SIS_HORZ_OFFSET;
				lpMB->face = ANGLE_TO_FACING (QUADRANT);
				/* copy it because there are two */
				lpMB[1] = lpMB[0];
				++lpMB;
				++SisData->num_blasters;
				lpMB->face = NORMALIZE_FACING (-ANGLE_TO_FACING (QUADRANT));
				break;
			case NUM_MODULE_SLOTS - 1: /* TAIL WEAPON */
				RaceDescPtr->ship_info.ship_flags |= FIRES_AFT;
				lpMB->pixoffs = SIS_VERT_OFFSET;
				lpMB->face = ANGLE_TO_FACING (HALF_CIRCLE);
				break;
		}

		++lpMB;
		++SisData->num_blasters;
	}
}

static void
InitModuleSlots (RACE_DESC *RaceDescPtr, const BYTE *ModuleSlots)
{
	COUNT i;
	COUNT num_trackers;
	SIS_DATA *SisData = GetCustomShipData (RaceDescPtr);

	RaceDescPtr->ship_info.max_crew = 0;
	num_trackers = 0;
	for (i = 0; i < NUM_MODULE_SLOTS; ++i)
	{
		BYTE which_mod;

		which_mod = ModuleSlots[(NUM_MODULE_SLOTS - 1) - i];
		switch (which_mod)
		{
			case CREW_POD:
				RaceDescPtr->ship_info.max_crew += CREW_POD_CAPACITY;
				break;
			case TRACKING_SYSTEM:
				++num_trackers;
				break;
			case ANTIMISSILE_DEFENSE:
				RaceDescPtr->characteristics.special_energy_cost +=
						ANTIMISSILE_ENERGY_INC;
				break;
			case SHIVA_FURNACE:
				RaceDescPtr->characteristics.energy_regeneration +=
						SHIVA_ENERGY_REGEN_INC;
				break;
			case DYNAMO_UNIT:
				RaceDescPtr->characteristics.energy_wait -=
						DYNAMO_UNIT_ENERGY_WAIT_DEC;
				if (RaceDescPtr->characteristics.energy_wait < MIN_ENERGY_WAIT)
					RaceDescPtr->characteristics.energy_wait = MIN_ENERGY_WAIT;
				break;
		}
	}

	if (num_trackers > MAX_TRACKING)
		num_trackers = MAX_TRACKING;
	RaceDescPtr->characteristics.weapon_energy_cost +=
			num_trackers * TRACKER_ENERGY_COST;
	SisData->num_trackers = num_trackers;
	if (RaceDescPtr->characteristics.special_energy_cost)
	{
		RaceDescPtr->ship_info.ship_flags |= POINT_DEFENSE;
		if (RaceDescPtr->characteristics.special_energy_cost > MAX_DEFENSE)
			RaceDescPtr->characteristics.special_energy_cost = MAX_DEFENSE;
	}
}

static void
InitDriveSlots (RACE_DESC *RaceDescPtr, const BYTE *DriveSlots)
{
	COUNT i;

	// NB. RaceDescPtr->characteristics.max_thrust is already initialised.
	RaceDescPtr->characteristics.thrust_wait = 0;
	for (i = 0; i < NUM_DRIVE_SLOTS; ++i)
	{
		switch (DriveSlots[i])
		{
			case FUSION_THRUSTER:
				RaceDescPtr->characteristics.max_thrust += RES_SCALE(2);
				++RaceDescPtr->characteristics.thrust_wait;
				break;
		}
	}
	RaceDescPtr->characteristics.thrust_wait = (BYTE)(
			THRUST_WAIT - (RaceDescPtr->characteristics.thrust_wait >> 1));
	RaceDescPtr->characteristics.max_thrust =
			((RaceDescPtr->characteristics.max_thrust /
			RaceDescPtr->characteristics.thrust_increment) + 1)
			* RaceDescPtr->characteristics.thrust_increment;
}

static void
InitJetSlots (RACE_DESC *RaceDescPtr, const BYTE *JetSlots)
{
	COUNT i;

	for (i = 0; i < NUM_JET_SLOTS; ++i)
	{
		switch (JetSlots[i])
		{
			case TURNING_JETS:
				RaceDescPtr->characteristics.turn_wait -= 2;
				break;
		}
	}
}

RACE_DESC*
init_sis (void)
{
	RACE_DESC *RaceDescPtr;
	COUNT i;
	// The caller of this func will copy the struct
	static RACE_DESC new_sis_desc;
	SIS_DATA empty_data;
	memset (&empty_data, 0, sizeof (empty_data));

	if (IS_HD) {
		sis_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		sis_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		sis_desc.cyborg_control.WeaponRange = BLASTER_SPEED_HD * BLASTER_LIFE;
	}

	/* copy initial ship settings to new_sis_desc */
	new_sis_desc = sis_desc;
	
	new_sis_desc.uninit_func = uninit_sis;

	if (inHQSpace ())
	{
		for (i = 0; i < NUM_VIEWS; ++i)
		{
			new_sis_desc.ship_data.ship_rsc[i] = NULL_RESOURCE;
			new_sis_desc.ship_data.weapon_rsc[i] = NULL_RESOURCE;
			new_sis_desc.ship_data.special_rsc[i] = NULL_RESOURCE;
		}
		new_sis_desc.ship_info.icons_rsc = NULL_RESOURCE;
		new_sis_desc.ship_data.captain_control.captain_rsc = NULL_RESOURCE;
		new_sis_desc.ship_data.victory_ditty_rsc = NULL_RESOURCE;
		new_sis_desc.ship_data.ship_sounds_rsc = NULL_RESOURCE;

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1 && (IS_HD || EndlessSCLoaded))
			new_sis_desc.ship_data.ship_rsc[0] = SIS_QUASI_MASK_PMAP_ANIM;
		else
			new_sis_desc.ship_data.ship_rsc[0] = SIS_HYPER_MASK_PMAP_ANIM;

		new_sis_desc.preprocess_func = sis_hyper_preprocess;
		new_sis_desc.postprocess_func = sis_hyper_postprocess;

		new_sis_desc.characteristics.max_thrust -= RES_SCALE(4);
	}
	else
	{
		new_sis_desc.preprocess_func = sis_battle_preprocess;
		new_sis_desc.postprocess_func = sis_battle_postprocess;
		new_sis_desc.init_weapon_func = initialize_blasters;
		new_sis_desc.cyborg_control.intelligence_func = sis_intelligence;

		if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 3)
			SET_GAME_STATE (BOMB_CARRIER, 1);
	}

	SetCustomShipData (&new_sis_desc, &empty_data);
	InitModuleSlots (&new_sis_desc, GLOBAL_SIS (ModuleSlots));
	InitWeaponSlots (&new_sis_desc, GLOBAL_SIS (ModuleSlots));
	InitDriveSlots (&new_sis_desc, GLOBAL_SIS (DriveSlots));
	InitJetSlots (&new_sis_desc, GLOBAL_SIS (JetSlots));
	
	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
	{
		new_sis_desc.ship_info.crew_level = new_sis_desc.ship_info.max_crew;
	}
	else
	{
		// Count the captain too.
		new_sis_desc.ship_info.max_crew++;
		new_sis_desc.ship_info.crew_level = GLOBAL_SIS (CrewEnlisted) + 1;
		new_sis_desc.ship_info.ship_flags |= PLAYER_CAPTAIN;
	}
	
	new_sis_desc.ship_info.energy_level = new_sis_desc.ship_info.max_energy;

	RaceDescPtr = &new_sis_desc;

	return (RaceDescPtr);
}

static void
uninit_sis (RACE_DESC *pRaceDesc)
{
	if (!inHQSpace ())
	{
		GLOBAL_SIS (CrewEnlisted) = pRaceDesc->ship_info.crew_level;
		if (pRaceDesc->ship_info.ship_flags & PLAYER_CAPTAIN)
			GLOBAL_SIS (CrewEnlisted)--;
	}

	SetCustomShipData (pRaceDesc, NULL);
}


