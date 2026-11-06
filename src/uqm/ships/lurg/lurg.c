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
#include "lurg.h"
#include "resinst.h"
#include "libs/mathlib.h"


// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST 20
#define THRUST_INCREMENT 5
#define THRUST_WAIT 2
#define TURN_WAIT 1
#define SHIP_MASS 6

// Primary weapon
#define WEAPON_ENERGY_COST 4
#define WEAPON_WAIT 10
#define MISSILE_SPEED DISPLAY_TO_WORLD (18)
#define MISSILE_LIFE 23
#define MISSILE_HITS 5
#define MISSILE_DAMAGE 4
#define MISSILE_OFFSET RES_SCALE (2)
#define LURG_OFFSET RES_SCALE (23)
#define MISSILE_RANGE ((MISSILE_SPEED * MISSILE_LIFE) * 4 / 5)

// Secondary weapon
#define SPECIAL_ENERGY_COST 2
#define SPECIAL_WAIT 2
#define OIL_BATCH_SIZE 6
#define OIL_SPEED DISPLAY_TO_WORLD (RES_SCALE (2))
#define OIL_INIT_SPEED DISPLAY_TO_WORLD (RES_SCALE (6))
#define OIL_HITS 3
#define OIL_DAMAGE 1 // Oil inflicts damage against projectiles, not ships.
#define OIL_SPREAD_MINIMUM 6
#define OIL_SPREAD_VARIATION 4
#define OIL_LIFE 300
#define OIL_LIFE_VARIATION 100
#define OIL_DELAY 6
#define OIL_DELAY_MAX 36
#define OIL_SNARE WORLD_TO_VELOCITY (-1)
#define OIL_OFFSET (RES_SCALE (3))
#define LURG_OFFSET_2 (RES_SCALE (16))

// Bonus ability
#define REPAIR_WAIT 192

// HD
#define MISSILE_SPEED_HD RES_SCALE (MISSILE_SPEED)
#define MISSILE_RANGE_HD ((MISSILE_SPEED_HD * MISSILE_LIFE) * 4 / 5)

static RACE_DESC lurg_desc =
{
	{ /* SHIP_INFO */
		"prawn",
		FIRES_FORE | SEEKING_SPECIAL | LIGHT_POINT_DEFENSE,
		20, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		LURG_RACE_STRINGS,
		LURG_ICON_MASK_PMAP_ANIM,
		LURG_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, //1525 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			3710, 4389,
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
			LURG_BIG_MASK_PMAP_ANIM,
			LURG_MED_MASK_PMAP_ANIM,
			LURG_SML_MASK_PMAP_ANIM,
		},
		{
			LURGGOB_BIG_MASK_PMAP_ANIM,
			LURGGOB_MED_MASK_PMAP_ANIM,
			LURGGOB_SML_MASK_PMAP_ANIM,
		},
		{
			OIL_BIG_MASK_PMAP_ANIM,
			OIL_MED_MASK_PMAP_ANIM,
			OIL_SML_MASK_PMAP_ANIM,
		},
		{
			LURG_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL,
			0, 0, 0, 0, 0
		},
		LURG_VICTORY_SONG,
		LURG_SHIP_SOUNDS,
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
lurg_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern, COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);

	// Don't use the special unless specifically told to.
	StarShipPtr->ship_input_state &= ~SPECIAL;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;
		SIZE EnemyTravelFacing, TestFacing, CurrentEnemySpeed, delta_x, delta_y;

		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);

		// Be more responsive against slow ships.
		if (MANEUVERABILITY (&EnemyStarShipPtr->RaceDescPtr->cyborg_control) <= RES_SCALE (SLOW_SHIP))
			lpEvalDesc->which_turn = lpEvalDesc->which_turn * 4/5;

		// Start of crazy calculations //
		// Transform enemy velocity into cute little world units for easy comparison.
		GetCurrentVelocityComponents (&lpEvalDesc->ObjectPtr->velocity, &delta_x, &delta_y);
		CurrentEnemySpeed = VELOCITY_TO_WORLD (square_root (VelocitySquared (delta_x, delta_y)));

		// Draw a line from the enemy's current position to the enemy's projected future position.
		EnemyTravelFacing = ANGLE_TO_FACING (ARCTAN (
			lpEvalDesc->ObjectPtr->next.location.x	
				- lpEvalDesc->ObjectPtr->current.location.x,
			lpEvalDesc->ObjectPtr->next.location.y
				- lpEvalDesc->ObjectPtr->current.location.y));

		// Draw a line from the Prawn to the enemy.
		TestFacing = ANGLE_TO_FACING (ARCTAN (
			lpEvalDesc->ObjectPtr->current.location.x
				- ShipPtr->current.location.x,
			lpEvalDesc->ObjectPtr->current.location.y
				- ShipPtr->current.location.y));

		// Compare enemy's direction of travel to the Prawn's position.
		TestFacing = NORMALIZE_FACING (TestFacing - EnemyTravelFacing);
		// End of crazy calculations //

		// Avoid contact when the enemy is warping in or moving directly away.
		if (lpEvalDesc->ObjectPtr->state_flags & APPEARING
			|| (TestFacing > 6 && TestFacing < 10
				&& CurrentEnemySpeed >= RES_SCALE (16)))
		{
			lpEvalDesc->MoveState = AVOID;
		}
		// Attack when the enemy is ensnared, attempting an escape sequence or at short range.
		else if ((lpEvalDesc->which_turn <= 32
			&& lpEvalDesc->ObjectPtr->turn_wait >= OIL_DELAY
			&& lpEvalDesc->ObjectPtr->thrust_wait >= OIL_DELAY)
			|| lpEvalDesc->which_turn <= 15
			|| lpEvalDesc->ObjectPtr->mass_points > MAX_SHIP_MASS)
		{
			lpEvalDesc->MoveState = PURSUE;

			// Disregard enemy weapons in these circumstances.
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = NULL;
		}
		// Stall for time when the enemy is far away.
		else if (lpEvalDesc->which_turn > 20 && lpEvalDesc->which_turn < 47)
		{
			lpEvalDesc->MoveState = AVOID;
		}
		// Entice rather than retreat when the enemy is very far away.
		else if (lpEvalDesc->which_turn >= 47)
		{
			lpEvalDesc->MoveState = ENTICE;
		}
	}

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];

	// Drop oil in the way of various incoming projectiles.
	if (lpEvalDesc->ObjectPtr
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= 8
		&& lpEvalDesc->ObjectPtr->hit_points < 4 // Forget high HP projectiles.
		&& (lpEvalDesc->ObjectPtr->mass_points
			|| lpEvalDesc->ObjectPtr->state_flags & (CREW_OBJECT | FINITE_LIFE))
		&& lpEvalDesc->which_turn <= 10)
	{
		StarShipPtr->ship_input_state |= SPECIAL;
	}
	
	// Otherwise disregard all enemy weapons unless there's a crew weapon or huge weapon present.
	if (lpEvalDesc->ObjectPtr
			&& !(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT)
			&& !(lpEvalDesc->ObjectPtr->hit_points >= 6)
			&& !(lpEvalDesc->ObjectPtr->mass_points >= 6))
	{
		ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr = NULL;
	}
	
	// Basic ship intelligence.
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		// Cut thrust when attacking...
		if (lpEvalDesc->MoveState == PURSUE
			// ...unless the enemy is ensnared...
			&& (lpEvalDesc->ObjectPtr->turn_wait < OIL_DELAY
				|| lpEvalDesc->ObjectPtr->thrust_wait < OIL_DELAY)
			// ...or the enemy is attempting an escape sequence...
			&& (lpEvalDesc->ObjectPtr->mass_points <= MAX_SHIP_MASS)
			// ...or the Prawn is completely stationary.
			&& ShipPtr->next.location.x != ShipPtr->current.location.x
			&& ShipPtr->next.location.y != ShipPtr->current.location.y)
		{
			StarShipPtr->ship_input_state &= ~THRUST;
		}

		// Don't shoot unless specifically told to OR a particular enemy weapon type is active.
		if (ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr == NULL)
		{
			StarShipPtr->ship_input_state &= ~WEAPON;
		}

		// Frequently disregard good firing opportunities.
		if (ship_weapons (ShipPtr, lpEvalDesc->ObjectPtr, DISPLAY_TO_WORLD (RES_SCALE (10))))
		{
			if (TFB_Random () & 3)
				StarShipPtr->ship_input_state |= WEAPON;
		}
		// Sometimes take shots that don't line up with the opponent's current trajectory.
		else if (ship_weapons (ShipPtr, lpEvalDesc->ObjectPtr, DISPLAY_TO_WORLD (RES_SCALE (20))))
		{
			if (TFB_Random () & 5)
				StarShipPtr->ship_input_state |= WEAPON;
		}
		else if (ship_weapons (ShipPtr, lpEvalDesc->ObjectPtr, DISPLAY_TO_WORLD (RES_SCALE (30))))
		{
			if (TFB_Random () % 11)
				StarShipPtr->ship_input_state |= WEAPON;
		}
		
		// Drop several batches of oil whenever the battery tops off.
		if ((StarShipPtr->RaceDescPtr->ship_info.energy_level ==
					StarShipPtr->RaceDescPtr->ship_info.max_energy)
				|| (StarShipPtr->RaceDescPtr->ship_info.energy_level >
						StarShipPtr->RaceDescPtr->ship_info.max_energy - 6
					&& StarShipPtr->old_status_flags & SPECIAL))
		{
			StarShipPtr->ship_input_state |= SPECIAL;
		}
	}
}

static void
acid_preprocess (ELEMENT *ElementPtr)
{
	COUNT facing;

	facing = (GetFrameIndex (ElementPtr->next.image.frame)) % 16;
	// JMS: Modulo 16 here ensures that the explosion frames are not used in wrong place.
	
	if (ElementPtr->thrust_wait == 1) // Left start.
	{
		--facing;
		ElementPtr->turn_wait += 1;
		ElementPtr->thrust_wait = 0;
		ElementPtr->turn_wait = 3;
	}

	if (ElementPtr->thrust_wait == 2) // Right start.
	{
		++facing;
		ElementPtr->turn_wait += 1;
		ElementPtr->thrust_wait = 0;
		ElementPtr->turn_wait = 0;
	}

	if (ElementPtr->thrust_wait == 0) // Main loop.
	{
		if (ElementPtr->turn_wait < 2) // Turn left.
		{
			--facing;
			ElementPtr->turn_wait += 1;
		} 
		else if (ElementPtr->turn_wait < 3) // Wait.
		{
			ElementPtr->turn_wait += 1;
		}
		else if (ElementPtr->turn_wait < 5) // Turn right.
		{
			++facing;
			ElementPtr->turn_wait += 1;
		}
		else if (ElementPtr->turn_wait < 6) // Wait.
		{
			ElementPtr->turn_wait += 1;
		}
		else if (ElementPtr->turn_wait == 6) // Wait and reset the loop.
		{
			ElementPtr->turn_wait = 0;
		}
	}

	ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->next.image.frame, facing);
	ElementPtr->state_flags |= CHANGING;
	
	SetVelocityVector (&ElementPtr->velocity, (MISSILE_SPEED_HD), facing);
}

static COUNT
initialize_acid (ELEMENT *ShipPtr, HELEMENT AcidArray[])
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
	MissileBlock.pixoffs = LURG_OFFSET;
	MissileBlock.speed = MISSILE_SPEED_HD;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	MissileBlock.preprocess_func = acid_preprocess;

	AcidArray[0] = initialize_missile (&MissileBlock);
	
	if (AcidArray[0])
	{
		ELEMENT *AcidPtr;

		LockElement (AcidArray[0], &AcidPtr);
		if ((BYTE)TFB_Random () & 1) // Left or right?
			AcidPtr->thrust_wait = 1;
		else
			AcidPtr->thrust_wait = 2;
		UnlockElement (AcidArray[0]);
	}
	
	return (1);
}

static void
oil_preprocess (ELEMENT *ElementPtr)
{
	BYTE thrust_wait, turn_wait;

	thrust_wait = HINIBBLE (ElementPtr->turn_wait);
	turn_wait = LONIBBLE (ElementPtr->turn_wait);

	if (thrust_wait > 0)
		--thrust_wait;
	else
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		ElementPtr->state_flags |= CHANGING;

		thrust_wait = (BYTE)((COUNT)TFB_Random () & 7);
	}

	if (turn_wait > 0)
		--turn_wait;
	else
	{
		COUNT facing;

		facing = (COUNT)TFB_Random ();

		SetVelocityVector (&ElementPtr->velocity,
				OIL_SPEED, facing);

		turn_wait = 4;
	}

	ElementPtr->turn_wait = MAKE_BYTE (turn_wait, thrust_wait);
}

static void
oil_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (!(ElementPtr1->state_flags & (APPEARING | PLAYER_SHIP | FINITE_LIFE))
	    && (ElementPtr1->playerNr == NEUTRAL_PLAYER_NUM)
	    && !GRAVITY_MASS (ElementPtr1->mass_points))
	{
		ElementPtr0->mass_points = 0;
		// Oil does no damage against asteroids
	}
	else if (!elementsOfSamePlayer(ElementPtr0, ElementPtr1))
	{
		STARSHIP *StarShipPtr;
		STARSHIP *EnemyStarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);
		GetElementStarShip (ElementPtr1, &EnemyStarShipPtr);
		
		if (ElementPtr1->state_flags & PLAYER_SHIP)
		{
			COUNT facing;

			ElementPtr1->thrust_wait += OIL_DELAY;
			if (ElementPtr1->thrust_wait > OIL_DELAY_MAX)
				ElementPtr1->thrust_wait = OIL_DELAY_MAX;
			ElementPtr1->turn_wait += OIL_DELAY;
			if (ElementPtr1->turn_wait > OIL_DELAY_MAX)
				ElementPtr1->turn_wait = OIL_DELAY_MAX;

			if(EnemyStarShipPtr->SpeciesID == SUPOX_ID)
			{
				EnemyStarShipPtr->special_counter += OIL_DELAY;
				if (EnemyStarShipPtr->special_counter > OIL_DELAY_MAX)
					EnemyStarShipPtr->special_counter = OIL_DELAY_MAX;
			}
				
			
			facing = NORMALIZE_FACING (
				ANGLE_TO_FACING (GetVelocityTravelAngle (&ElementPtr1->velocity) + HALF_CIRCLE));
		
			DeltaVelocityComponents (&ElementPtr1->velocity,
				COSINE (facing, OIL_SNARE),
				SINE (facing, OIL_SNARE));

			ElementPtr0->mass_points = 0;

			// ProcessSound (SetAbsSoundIndex (
					/* Sound effect?? */
			// StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr0);
		}
		else
		{
			if (ElementPtr1->hit_points > 3)
			{
				SIZE xHits;
				xHits = ElementPtr1->hit_points;
				ElementPtr0->mass_points = (TFB_Random () & 3) == 1;
				// Oil is ineffective against large projectiles
			}
			else
				ElementPtr0->mass_points = 1;
				// Do full, guaranteed damage against small projectiles
		}
	}

	weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static void spill_oil (ELEMENT *ShipPtr)
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT Missile;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = (COUNT)TFB_Random (); // Deploy at random in every direction.
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = LURG_OFFSET_2;
	MissileBlock.life = OIL_LIFE + (TFB_Random () & OIL_LIFE_VARIATION);
	MissileBlock.speed = OIL_INIT_SPEED;
	MissileBlock.hit_points = OIL_HITS;
	MissileBlock.damage = OIL_DAMAGE;
	MissileBlock.preprocess_func = oil_preprocess;
	MissileBlock.blast_offs = OIL_OFFSET;
	Missile = initialize_missile (&MissileBlock);

	if (Missile)
	{
		ELEMENT *OilPtr;

		LockElement (Missile, &OilPtr);
		// OilPtr->turn_wait affects how long the projectile travels at OIL_INIT_SPEED.
		OilPtr->turn_wait = OIL_SPREAD_MINIMUM + ((COUNT)TFB_Random () & OIL_SPREAD_VARIATION);
		SetElementStarShip (OilPtr, StarShipPtr);
		OilPtr->collision_func = oil_collision;
		UnlockElement (Missile);
		PutElement (Missile);
	}
}

static void
lurg_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		int i;

		ProcessSound (SetAbsSoundIndex // Spill oil.
			(StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		
		for (i = 0; i < OIL_BATCH_SIZE; i++)
			spill_oil (ElementPtr);

		StarShipPtr->special_counter = SPECIAL_WAIT;
	}

	// Start timer for passive regeneration when damaged.
	if (ElementPtr->hit_points < MAX_CREW
		&& StarShipPtr->auxiliary_counter == 0)
	{
		StarShipPtr->auxiliary_counter += REPAIR_WAIT;
	}

	// Slowly regenerate crew.
	if (StarShipPtr->auxiliary_counter == 1
			&& ElementPtr->hit_points > 0)
	{
		DeltaCrew (ElementPtr, 1);

		ProcessSound (SetAbsSoundIndex // Regenerate.
			(StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);
	}
}

RACE_DESC*
init_lurg (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD)
	{
		lurg_desc.characteristics.max_thrust = RES_SCALE (MAX_THRUST);
		lurg_desc.characteristics.thrust_increment = RES_SCALE (THRUST_INCREMENT);
		lurg_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}
	else
	{
		lurg_desc.characteristics.max_thrust = MAX_THRUST;
		lurg_desc.characteristics.thrust_increment = THRUST_INCREMENT;
		lurg_desc.cyborg_control.WeaponRange = MISSILE_RANGE;
	}

	lurg_desc.postprocess_func = lurg_postprocess;
	lurg_desc.init_weapon_func = initialize_acid;
	lurg_desc.cyborg_control.intelligence_func = lurg_intelligence;
	RaceDescPtr = &lurg_desc;

	return (RaceDescPtr);
}
