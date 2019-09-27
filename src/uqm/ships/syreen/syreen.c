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
#include "syreen.h"
#include "resinst.h"
#include "../../setup.h"
#include "libs/mathlib.h"

// Core characteristics
#define SYREEN_MAX_CREW_SIZE MAX_CREW_SIZE
#define MAX_CREW 12
#define MAX_ENERGY 16
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST /* DISPLAY_TO_WORLD (8) */ 36
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 9
#define THRUST_WAIT 1
#define TURN_WAIT 1
#define SHIP_MASS 2

// Particle Beam Stiletto
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 8
#define SYREEN_OFFSET RES_SCALE(30)
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 2
#define MISSILE_OFFSET RES_SCALE(3)

// Syreen song
#define SPECIAL_ENERGY_COST 5
#define SPECIAL_WAIT 20
#define ABANDONER_RANGE RES_SCALE(208) /* originally SPACE_HEIGHT */
#define MAX_ABANDONERS 8

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (120)

static RACE_DESC syreen_desc =
{
	{ /* SHIP_INFO */
		"penetrator",
		FIRES_FORE,
		13, /* Super Melee cost */
		MAX_CREW, SYREEN_MAX_CREW_SIZE,
		MAX_ENERGY, MAX_ENERGY,
		SYREEN_RACE_STRINGS,
		SYREEN_ICON_MASK_PMAP_ANIM,
		SYREEN_MICON_MASK_PMAP_ANIM,
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
			SYREEN_BIG_MASK_PMAP_ANIM,
			SYREEN_MED_MASK_PMAP_ANIM,
			SYREEN_SML_MASK_PMAP_ANIM,
		},
		{
			DAGGER_BIG_MASK_PMAP_ANIM,
			DAGGER_MED_MASK_PMAP_ANIM,
			DAGGER_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			SYREEN_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		SYREEN_VICTORY_SONG,
		SYREEN_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		(MISSILE_SPEED * MISSILE_LIFE * 2 / 3),
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
initialize_dagger (ELEMENT *ShipPtr, HELEMENT DaggerArray[])
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
	MissileBlock.pixoffs = SYREEN_OFFSET;
	MissileBlock.speed = RES_BOOL(MISSILE_SPEED, MISSILE_SPEED_HD);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	DaggerArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void
spawn_crew (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		HELEMENT hCrew;

		hCrew = AllocElement ();
		if (hCrew != 0)
		{
			ELEMENT *CrewPtr;

			LockElement (hCrew, &CrewPtr);
			CrewPtr->next.location = ElementPtr->next.location;
			CrewPtr->playerNr = ElementPtr->playerNr;
			CrewPtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE;
			CrewPtr->life_span = 0;
			CrewPtr->death_func = spawn_crew;
			CrewPtr->pParent = ElementPtr->pParent;
			CrewPtr->hTarget = 0;
			UnlockElement (hCrew);

			PutElement (hCrew);
		}
	}
	else
	{
		HELEMENT hElement, hNextElement;

		for (hElement = GetHeadElement ();
				hElement != 0; hElement = hNextElement)
		{
			ELEMENT *ObjPtr;

			LockElement (hElement, &ObjPtr);
			hNextElement = GetSuccElement (ObjPtr);

			if ((ObjPtr->state_flags & PLAYER_SHIP)
					&& !elementsOfSamePlayer (ObjPtr, ElementPtr)
					&& ObjPtr->crew_level > 1)
			{
				SDWORD dx, dy;
				DWORD d_squared;

				dx = ObjPtr->next.location.x - ElementPtr->next.location.x;
				if (dx < 0)
					dx = -dx;
				dy = ObjPtr->next.location.y - ElementPtr->next.location.y;
				if (dy < 0)
					dy = -dy;

				dx = WORLD_TO_DISPLAY (dx);
				dy = WORLD_TO_DISPLAY (dy);
				if (dx <= ABANDONER_RANGE && dy <= ABANDONER_RANGE
						&& (d_squared = (DWORD)((UWORD)dx * (UWORD)dx)
						+ (DWORD)((UWORD)dy * (UWORD)dy)) <=
						(DWORD)((UWORD)ABANDONER_RANGE * (UWORD)ABANDONER_RANGE))
				{
					COUNT crew_loss;

					if (!antiCheat(ElementPtr, TRUE))
						crew_loss = ((MAX_ABANDONERS * (ABANDONER_RANGE - square_root(d_squared))) / ABANDONER_RANGE) + 1;
					else
						crew_loss = 0;

					if (crew_loss >= ObjPtr->crew_level)
						crew_loss = ObjPtr->crew_level - 1;

					AbandonShip (ObjPtr, ElementPtr, crew_loss);
				}
			}

			UnlockElement (hElement);
		}
	}
}

static void
syreen_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr != NULL)
	{
		STARSHIP *StarShipPtr;
		STARSHIP *EnemyStarShipPtr;

		GetElementStarShip (ShipPtr, &StarShipPtr);
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & CREW_IMMUNE)
				&& StarShipPtr->special_counter == 0
				&& lpEvalDesc->ObjectPtr->crew_level > 1
				&& lpEvalDesc->which_turn <= 14)
			StarShipPtr->ship_input_state |= SPECIAL;
		else
			StarShipPtr->ship_input_state &= ~SPECIAL;
	}
}

static void
syreen_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
						/* SYREEN_SONG */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		spawn_crew (ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

RACE_DESC*
init_syreen (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		syreen_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		syreen_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		syreen_desc.cyborg_control.WeaponRange = (MISSILE_SPEED_HD * MISSILE_LIFE * 2 / 3);
	}

	syreen_desc.postprocess_func = syreen_postprocess;
	syreen_desc.init_weapon_func = initialize_dagger;
	syreen_desc.cyborg_control.intelligence_func = syreen_intelligence;
	RaceDescPtr = &syreen_desc;

	return (RaceDescPtr);
}