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
#include "chenjesu.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 36
#define MAX_ENERGY 30
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 4
#define MAX_THRUST /* DISPLAY_TO_WORLD (7) */ 27
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 3
#define THRUST_WAIT 4
#define TURN_WAIT 6
#define SHIP_MASS 10

// Photon Shard
#define WEAPON_ENERGY_COST 5
#define WEAPON_WAIT 0
#define CHENJESU_OFFSET RES_SCALE(16)
#define MISSILE_OFFSET 0
#define MISSILE_SPEED DISPLAY_TO_WORLD (RES_SCALE(16))
#define MISSILE_LIFE 90
		/* actually, it's as long as you hold the button down. */
#define MISSILE_HITS 10
#define MISSILE_DAMAGE 6
#define NUM_SPARKLES 8

// Shrapnel
#define FRAGMENT_OFFSET RES_SCALE(2)
#define NUM_FRAGMENTS 8
#define FRAGMENT_LIFE 10
#define FRAGMENT_SPEED MISSILE_SPEED
#define FRAGMENT_RANGE (FRAGMENT_LIFE * FRAGMENT_SPEED)
		/* This bit is for the cyborg only. */
#define FRAGMENT_HITS 1
#define FRAGMENT_DAMAGE 2

// DOGI
#define SPECIAL_ENERGY_COST MAX_ENERGY
#define SPECIAL_WAIT 0
#define DOGGY_OFFSET RES_SCALE(18) + 5 * RESOLUTION_FACTOR
#define DOGGY_SPEED DISPLAY_TO_WORLD (RES_SCALE(8))
#define ENERGY_DRAIN 10
#define MAX_DOGGIES 4
#define DOGGY_HITS 3
#define DOGGY_MASS 4

static RACE_DESC chenjesu_desc =
{
	{ /* SHIP_INFO */
		"broodhome",
		FIRES_FORE | SEEKING_SPECIAL | SEEKING_WEAPON,
		28, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		CHENJESU_RACE_STRINGS,
		CHENJESU_ICON_MASK_PMAP_ANIM,
		CHENJESU_MICON_MASK_PMAP_ANIM,
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
			CHENJESU_BIG_MASK_PMAP_ANIM,
			CHENJESU_MED_MASK_PMAP_ANIM,
			CHENJESU_SML_MASK_PMAP_ANIM,
		},
		{
			SPARK_BIG_MASK_PMAP_ANIM,
			SPARK_MED_MASK_PMAP_ANIM,
			SPARK_SML_MASK_PMAP_ANIM,
		},
		{
			DOGGY_BIG_MASK_PMAP_ANIM,
			DOGGY_MED_MASK_PMAP_ANIM,
			DOGGY_SML_MASK_PMAP_ANIM,
		},
		{
			CHENJESU_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		CHENJESU_VICTORY_SONG,
		CHENJESU_SHIP_SOUNDS,
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
crystal_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.index = 1;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = 0;
	MissileBlock.speed = FRAGMENT_SPEED;
	MissileBlock.hit_points = FRAGMENT_HITS;
	MissileBlock.damage = FRAGMENT_DAMAGE;
	MissileBlock.life = FRAGMENT_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = FRAGMENT_OFFSET;

	for (MissileBlock.face = 0;
			MissileBlock.face < ANGLE_TO_FACING (FULL_CIRCLE);
			MissileBlock.face +=
			(ANGLE_TO_FACING (FULL_CIRCLE) / NUM_FRAGMENTS))
	{
		HELEMENT hFragment;

		hFragment = initialize_missile (&MissileBlock);
		if (hFragment)
		{
			ELEMENT *FragPtr;

			LockElement (hFragment, &FragPtr);
			SetElementStarShip (FragPtr, StarShipPtr);
			UnlockElement (hFragment);
			PutElement (hFragment);
		}
	}

	ProcessSound (SetAbsSoundIndex (
					/* CRYSTAL_FRAGMENTS */
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
}

static void
crystal_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->cur_status_flags & WEAPON)
		++ElementPtr->life_span; /* keep it going while key pressed */
	else
	{
		ElementPtr->life_span = 1;

		ElementPtr->postprocess_func = crystal_postprocess;
	}
}

static void
animate (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = ElementPtr->next_turn;
	}
}

static void
crystal_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	HELEMENT hBlastElement;

	hBlastElement =
			weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		ELEMENT *BlastElementPtr;

		LockElement (hBlastElement, &BlastElementPtr);
		BlastElementPtr->current.location = ElementPtr1->current.location;

		BlastElementPtr->life_span = NUM_SPARKLES;
		BlastElementPtr->turn_wait = BlastElementPtr->next_turn = 0;
		{
			BlastElementPtr->preprocess_func = animate;
		}

		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame =
				SetAbsFrameIndex (BlastElementPtr->current.image.farray[0],
				2); /* skip stones */

		UnlockElement (hBlastElement);
	}
}

static void
doggy_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	++StarShipPtr->special_counter;
	if (ElementPtr->thrust_wait > 0) /* could be non-zero after a collision */
		--ElementPtr->thrust_wait;
	else
	{
		COUNT facing, orig_facing;
		SIZE delta_facing;

		facing = orig_facing =
				NORMALIZE_FACING (ANGLE_TO_FACING (
				GetVelocityTravelAngle (&ElementPtr->velocity)
				));
		if ((delta_facing = TrackShip (ElementPtr, &facing)) < 0)
			facing = NORMALIZE_FACING (TFB_Random ());
		else
		{
			ELEMENT *ShipPtr;

			LockElement (ElementPtr->hTarget, &ShipPtr);
			facing = NORMALIZE_FACING (ANGLE_TO_FACING (
					ARCTAN (ShipPtr->current.location.x -
					ElementPtr->current.location.x,
					ShipPtr->current.location.y -
					ElementPtr->current.location.y)
					));
			delta_facing = NORMALIZE_FACING (facing -
					GetFrameIndex (ShipPtr->current.image.frame));
			UnlockElement (ElementPtr->hTarget);

			if (delta_facing > ANGLE_TO_FACING (HALF_CIRCLE - OCTANT) &&
					delta_facing < ANGLE_TO_FACING (HALF_CIRCLE + OCTANT))
			{
				if (delta_facing >= ANGLE_TO_FACING (HALF_CIRCLE))
					facing -= ANGLE_TO_FACING (QUADRANT);
				else
					facing += ANGLE_TO_FACING (QUADRANT);
			}

			facing = NORMALIZE_FACING (facing);
		}

		if (facing != orig_facing)
			SetVelocityVector (&ElementPtr->velocity,
					DOGGY_SPEED, facing);
	}

	// JMS_GFX: Doggy is animated in hi-res modes
	if (IS_HD)
	{
		if (ElementPtr->turn_wait > 0)
			--ElementPtr->turn_wait;
		else
		{
			if (GetFrameIndex (ElementPtr->current.image.frame) == 11)
				ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->current.image.frame, 0);
			else
				ElementPtr->next.image.frame =
					IncFrameIndex (ElementPtr->current.image.frame);
					
			ElementPtr->state_flags |= CHANGING;

			ElementPtr->turn_wait = 1;
		}
 	}
}

static void
doggy_death (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	ProcessSound (SetAbsSoundIndex (
					/* DOGGY_DIES */
			StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3), ElementPtr);

	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->state_flags |= NONSOLID | FINITE_LIFE;
	// JMS_GFX: Doggy's dying animation starts at different frame in hi-res modes.
	if (IS_HD){
		ElementPtr->current.image.frame = SetRelFrameIndex (
			ElementPtr->current.image.frame, 12);
	}
	ElementPtr->life_span = 6;
	ElementPtr->preprocess_func = animate;
	ElementPtr->death_func = NULL;
	ElementPtr->collision_func = NULL;
	ZeroVelocityComponents (&ElementPtr->velocity);

	ElementPtr->turn_wait = ElementPtr->next_turn = 0;
}

static void
doggy_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& !elementsOfSamePlayer (ElementPtr0, ElementPtr1))
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);
		ProcessSound (SetAbsSoundIndex (
						/* DOGGY_STEALS_ENERGY */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr0);
		GetElementStarShip (ElementPtr1, &StarShipPtr);
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < ENERGY_DRAIN)
			DeltaEnergy (ElementPtr1, -StarShipPtr->RaceDescPtr->ship_info.energy_level);
		else
			DeltaEnergy (ElementPtr1, -ENERGY_DRAIN);
	}
	if (ElementPtr0->thrust_wait <= COLLISION_THRUST_WAIT)
		ElementPtr0->thrust_wait += COLLISION_THRUST_WAIT << 1;
}

static void
spawn_doggy (ELEMENT *ElementPtr)
{
	HELEMENT hDoggyElement;

	if ((hDoggyElement = AllocElement ()) != 0)
	{
		COUNT angle;
		ELEMENT *DoggyElementPtr;
		STARSHIP *StarShipPtr;

		ElementPtr->state_flags |= DEFY_PHYSICS;

		PutElement (hDoggyElement);
		LockElement (hDoggyElement, &DoggyElementPtr);
		DoggyElementPtr->hit_points = DOGGY_HITS;
		DoggyElementPtr->mass_points = DOGGY_MASS;
		DoggyElementPtr->thrust_wait = 0;
		DoggyElementPtr->playerNr = ElementPtr->playerNr;
		DoggyElementPtr->state_flags = APPEARING;
		DoggyElementPtr->life_span = NORMAL_LIFE;
		SetPrimType (&(GLOBAL (DisplayArray))[DoggyElementPtr->PrimIndex],
				STAMP_PRIM);
		{
			DoggyElementPtr->preprocess_func = doggy_preprocess;
			DoggyElementPtr->postprocess_func = NULL;
			DoggyElementPtr->collision_func = doggy_collision;
			DoggyElementPtr->death_func = doggy_death;
		}

		GetElementStarShip (ElementPtr, &StarShipPtr);
		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		DoggyElementPtr->current.location.x = ElementPtr->next.location.x
				+ COSINE (angle, DISPLAY_TO_WORLD (CHENJESU_OFFSET + DOGGY_OFFSET));
		DoggyElementPtr->current.location.y = ElementPtr->next.location.y
				+ SINE (angle, DISPLAY_TO_WORLD (CHENJESU_OFFSET + DOGGY_OFFSET));
		DoggyElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
		DoggyElementPtr->current.image.frame = StarShipPtr->RaceDescPtr->ship_data.special[0];

		SetVelocityVector (&DoggyElementPtr->velocity,
				DOGGY_SPEED, NORMALIZE_FACING (ANGLE_TO_FACING (angle)));

		SetElementStarShip (DoggyElementPtr, StarShipPtr);

		ProcessSound (SetAbsSoundIndex (
						/* RELEASE_DOGGY */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 4), DoggyElementPtr);

		UnlockElement (hDoggyElement);
	}
}

static void
chenjesu_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	static DWORD old_dist[NUM_SIDES] = {(DWORD)~0, (DWORD)~0};

	GetElementStarShip (ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SPECIAL;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;

		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if ((lpEvalDesc->which_turn <= 16
				&& MANEUVERABILITY (
				&EnemyStarShipPtr->RaceDescPtr->cyborg_control
				) >= RESOLUTION_COMPENSATED(MEDIUM_SHIP)) // JMS_GFX
				|| (MANEUVERABILITY (
				&EnemyStarShipPtr->RaceDescPtr->cyborg_control
				) <= RESOLUTION_COMPENSATED(SLOW_SHIP) // JMS_GFX
				&& WEAPON_RANGE (
				&EnemyStarShipPtr->RaceDescPtr->cyborg_control
				) >= RES_SCALE(LONG_RANGE_WEAPON) * 3 / 4 // JMS_GFX
				&& (EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & SEEKING_WEAPON)))
			lpEvalDesc->MoveState = PURSUE;
	}

	if (StarShipPtr->special_counter == 1
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState == ENTICE
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn <= 8)
	{
		lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	}

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	if (lpEvalDesc->ObjectPtr)
	{
		HELEMENT h, hNext;
		ELEMENT *CrystalPtr;

		h = (StarShipPtr->old_status_flags & WEAPON) ?
				GetTailElement () : (HELEMENT)0;
		for (; h; h = hNext)
		{
			LockElement (h, &CrystalPtr);
			hNext = GetPredElement (CrystalPtr);
			if (!(CrystalPtr->state_flags & NONSOLID)
					&& CrystalPtr->next.image.farray == StarShipPtr->RaceDescPtr->ship_data.weapon
					&& CrystalPtr->preprocess_func
					&& CrystalPtr->life_span > 0
					&& elementsOfSamePlayer (CrystalPtr, ShipPtr))
			{
				if (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr)
				{
					COUNT which_turn;
					BOOLEAN crystal_would_miss = false;

					if (!IS_HD || lpEvalDesc != &ObjectsOfConcern[ENEMY_SHIP_INDEX])
					{
						crystal_would_miss = ((which_turn = PlotIntercept (CrystalPtr,
							ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							CrystalPtr->life_span, FRAGMENT_RANGE / 2)) == 0
						 || (which_turn == 1
							 && PlotIntercept (CrystalPtr,
								ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
								CrystalPtr->life_span, 0) == 0));
					} else {
						DWORD curr_dist = 0;
						SDWORD dx, dy;
						
						dx = CrystalPtr->next.location.x - 
								ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr->next.location.x;
						dy = CrystalPtr->next.location.y - 
								ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr->next.location.y;		
					
						curr_dist = square_root ((long)dx * dx + (long)dy * dy);
					
						/*which_turn = PlotIntercept (CrystalPtr,
							ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							CrystalPtr->life_span, FRAGMENT_RANGE / 3);*/
						crystal_would_miss = ((PlotIntercept (CrystalPtr,
							ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							CrystalPtr->life_span, FRAGMENT_RANGE * 8) == 0)
							|| curr_dist > old_dist[ShipPtr->playerNr]);
						old_dist[ShipPtr->playerNr] = curr_dist;
					}
					if (crystal_would_miss)
					{
						StarShipPtr->ship_input_state &= ~WEAPON;
						old_dist[ShipPtr->playerNr] = (DWORD)~0;
						
						// JMS: Let's try to stop Chenjesu's stupid over-rapid firing behavior in hires modes...
						if (IS_HD
							&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 16
							&& ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr
								 && ObjectsOfConcern[ENEMY_WEAPON_INDEX].which_turn > 8)
								|| !(ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr)))
							StarShipPtr->weapon_counter = 10;
					}
					else if (StarShipPtr->weapon_counter == 0)
					{
						StarShipPtr->ship_input_state |= WEAPON;
						lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
					}

					UnlockElement (h);
					break;
				}
				hNext = 0;
			}
			UnlockElement (h);
		}

		if (h == 0)
		{
			if (StarShipPtr->old_status_flags & WEAPON)
			{
				StarShipPtr->ship_input_state &= ~WEAPON;
				old_dist[ShipPtr->playerNr] = (DWORD)~0;
				if (lpEvalDesc == &ObjectsOfConcern[ENEMY_WEAPON_INDEX])
					StarShipPtr->weapon_counter = 3;
			}
			else if (StarShipPtr->weapon_counter == 0
					&& ship_weapons (ShipPtr, lpEvalDesc->ObjectPtr, FRAGMENT_RANGE / 2))
			{
				if (IS_HD)
				{
					COUNT num_weapons;
					ELEMENT Ship;
					HELEMENT Weapon[6];
					HELEMENT *WeaponPtr,w;
					ELEMENT *EPtr;
					STARSHIP *StarShipPtr2;
					
					Ship = *ShipPtr;
					GetElementStarShip (&Ship, &StarShipPtr2);
					num_weapons = (*StarShipPtr->RaceDescPtr->init_weapon_func) (ShipPtr, Weapon);
					WeaponPtr = &Weapon[0];
					
					w = *WeaponPtr;
					if (w)
					{
						LockElement (w, &EPtr);
						if (EPtr->state_flags & APPEARING)
						{
							EPtr->next = EPtr->current;
							InitIntersectStartPoint (EPtr);
							InitIntersectEndPoint (EPtr);
							InitIntersectFrame (EPtr);
						}
							
						if (PlotIntercept (EPtr, lpEvalDesc->ObjectPtr, EPtr->life_span, FRAGMENT_RANGE / 2) < 80)
						{
							StarShipPtr->ship_input_state |= WEAPON;
						}
							
						UnlockElement (w);
						FreeElement (w);
					}
				}
				else
				{
					StarShipPtr->ship_input_state |= WEAPON;
				}
			}
		}
	}

	if (StarShipPtr->special_counter < MAX_DOGGIES)
	{
		if (lpEvalDesc->ObjectPtr
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level <= SPECIAL_ENERGY_COST
				&& !(StarShipPtr->ship_input_state & WEAPON))
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static COUNT
initialize_crystal (ELEMENT *ShipPtr, HELEMENT CrystalArray[])
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
	MissileBlock.pixoffs = CHENJESU_OFFSET;
	MissileBlock.speed = MISSILE_SPEED;
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = crystal_preprocess;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	CrystalArray[0] = initialize_missile (&MissileBlock);

	if (CrystalArray[0])
	{
		ELEMENT *CrystalPtr;

		LockElement (CrystalArray[0], &CrystalPtr);
		CrystalPtr->collision_func = crystal_collision;
		UnlockElement (CrystalArray[0]);
	}

	return (1);
}

static void
chenjesu_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter < MAX_DOGGIES
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		spawn_doggy (ElementPtr);
	}

	StarShipPtr->special_counter = 1;
			/* say there is one doggy, because ship_postprocess will
			 * decrement special_counter */
}

static void
chenjesu_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->special_counter > 1) /* only when STANDARD
											 * computer opponent
											 */
		StarShipPtr->special_counter += MAX_DOGGIES;
	if (StarShipPtr->cur_status_flags
			& StarShipPtr->old_status_flags
			& WEAPON)
		++StarShipPtr->weapon_counter;
}

RACE_DESC*
init_chenjesu (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		chenjesu_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		chenjesu_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		chenjesu_desc.cyborg_control.WeaponRange = LONG_RANGE_WEAPON_HD;
	}

	chenjesu_desc.preprocess_func = chenjesu_preprocess;
	chenjesu_desc.postprocess_func = chenjesu_postprocess;
	chenjesu_desc.init_weapon_func = initialize_crystal;
	chenjesu_desc.cyborg_control.intelligence_func = chenjesu_intelligence;
	RaceDescPtr = &chenjesu_desc;

	return (RaceDescPtr);
}

