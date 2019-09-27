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
#include "human.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/globdata.h"

// Core characteristics
#define MAX_CREW 18
#define MAX_ENERGY 18
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 8
#define MAX_THRUST /* DISPLAY_TO_WORLD (6) */ 24
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 3
#define THRUST_WAIT 4
#define TURN_WAIT 1
#define SHIP_MASS 6

// Nuke
#define WEAPON_ENERGY_COST 9
#define WEAPON_WAIT 10
#define HUMAN_OFFSET RES_SCALE(42)
#define NUKE_OFFSET RES_SCALE(8)
#define MIN_MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(10))
#define MAX_MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(20))
#define MISSILE_SPEED (RES_SCALE(MAX_THRUST) >= MIN_MISSILE_SPEED ? RES_SCALE(MAX_THRUST) : MIN_MISSILE_SPEED)
#define THRUST_SCALE DISPLAY_TO_WORLD (RES_SCALE(1))
#define MISSILE_LIFE 60
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 4
#define TRACK_WAIT 3

// Point-Defense Laser
#define SPECIAL_ENERGY_COST 4
#define SPECIAL_WAIT 9
#define LASER_RANGE (UWORD)RES_SCALE(100)

static RACE_DESC human_desc =
{
	{ /* SHIP_INFO */
		"cruiser",
		FIRES_FORE | SEEKING_WEAPON | POINT_DEFENSE,
		11, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		HUMAN_RACE_STRINGS,
		HUMAN_ICON_MASK_PMAP_ANIM,
		HUMAN_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			1752, 1450,
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
			HUMAN_BIG_MASK_PMAP_ANIM,
			HUMAN_MED_MASK_PMAP_ANIM,
			HUMAN_SML_MASK_PMAP_ANIM,
		},
		{
			SATURN_BIG_MASK_PMAP_ANIM,
			SATURN_MED_MASK_PMAP_ANIM,
			SATURN_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			HUMAN_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		HUMAN_VICTORY_SONG,
		HUMAN_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		LONG_RANGE_WEAPON,
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
nuke_preprocess (ELEMENT *ElementPtr)
{
	COUNT facing;

	facing = GetFrameIndex (ElementPtr->next.image.frame);
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		if (TrackShip (ElementPtr, &facing) > 0)
		{
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->next.image.frame,
					facing);
			ElementPtr->state_flags |= CHANGING;
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}

	{
		SDWORD speed;

		if ((speed = MISSILE_SPEED +
				((MISSILE_LIFE - ElementPtr->life_span) *
				THRUST_SCALE)) > MAX_MISSILE_SPEED)
			speed = MAX_MISSILE_SPEED;
		SetVelocityVector (&ElementPtr->velocity,
				speed, facing);
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

		PaidFor = FALSE;

		LockElement (StarShipPtr->hShip, &ShipPtr);
		for (hObject = GetTailElement (); hObject; hObject = hNextObject)
		{
			ELEMENT *ObjectPtr;

			LockElement (hObject, &ObjectPtr);
			hNextObject = GetPredElement (ObjectPtr);
			if (ObjectPtr != ShipPtr && CollidingElement (ObjectPtr) &&
					!OBJECT_CLOAKED (ObjectPtr))
			{
				SIZE delta_x, delta_y;

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
						if (!DeltaEnergy (ShipPtr, -SPECIAL_ENERGY_COST))
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
					LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F);
					hPointDefense = initialize_laser (&LaserBlock);
					if (hPointDefense)
					{
						ELEMENT *PDPtr;

						LockElement (hPointDefense, &PDPtr);
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

static COUNT
initialize_nuke (ELEMENT *ShipPtr, HELEMENT NukeArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = MissileBlock.index = StarShipPtr->ShipFacing;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = 0;
	MissileBlock.pixoffs = HUMAN_OFFSET;
	MissileBlock.speed = RES_SCALE(MISSILE_SPEED);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = nuke_preprocess;
	MissileBlock.blast_offs = NUKE_OFFSET;
	NukeArray[0] = initialize_missile (&MissileBlock);

	if (NukeArray[0])
	{
		ELEMENT *NukePtr;

		LockElement (NukeArray[0], &NukePtr);
		NukePtr->turn_wait = TRACK_WAIT;
		UnlockElement (NukeArray[0]);
	}

	return (1);
}

static void
human_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr != NULL
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 2)
			|| (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr != NULL
			&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 4)))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
	ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = NULL;

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	if (StarShipPtr->weapon_counter == 0)
	{
		if (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr
				&& (!(StarShipPtr->ship_input_state & (LEFT | RIGHT /* | THRUST */))
				|| ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn <= 12))
			StarShipPtr->ship_input_state |= WEAPON;
	}
}

static void
human_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0)
	{
		spawn_point_defense (ElementPtr);
	}
}

RACE_DESC*
init_human (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		human_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		human_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		human_desc.cyborg_control.WeaponRange = LONG_RANGE_WEAPON_HD;
	}

	human_desc.postprocess_func = human_postprocess;
	human_desc.init_weapon_func = initialize_nuke;
	human_desc.cyborg_control.intelligence_func = human_intelligence;

	RaceDescPtr = &human_desc;

	return (RaceDescPtr);
}

