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

// JMS_GFX 2011: Merged the resolution Factor stuff from UQM-HD.

#include "../ship.h"
#include "explorer.h"
#include "resinst.h"
#include "uqm/colors.h"
#include "uqm/controls.h"
#include "uqm/globdata.h"
#include "uqm/hyper.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "uqm/sis.h" // For GUN_WEAPON and CANNON_WEAPON

#define MAX_CREW 50 // EXPLORER_CREW_CAPACITY
#define MAX_ENERGY 42
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 5
#define SHIP_MASS 10

// These affect Chmmr Explorer in Super Melee and adventure mode.
#define EXPLORER_MAX_THRUST 36
#define EXPLORER_THRUST_INCREMENT 4
#define EXPLORER_TURN_WAIT 1
#define EXPLORER_THRUST_WAIT 1

#define WEAPON_ENERGY_COST 3
#define WEAPON_WAIT 2
#define BLASTER_SPEED DISPLAY_TO_WORLD (24)
#define BLASTER_LIFE 9
#define BLASTER_HITS 2
#define BLASTER_DAMAGE 2
#define BLASTER_OFFSET RES_SCALE (8)
#define BLASTER_RANGE (BLASTER_SPEED * BLASTER_LIFE)
#define EXP_VERT_OFFSET 28
#define EXP_HORZ_OFFSET 20
#define EXP_HORZ_OFFSET_2 (DISPLAY_TO_WORLD (RES_SCALE (5)))
#define EXP_HORZ_OFFSET_3 (DISPLAY_TO_WORLD (RES_SCALE (-5)))

#define SPECIAL_ENERGY_COST 15
#define SPECIAL_WAIT 20
#define STUNBALL_SPEED DISPLAY_TO_WORLD (RES_SCALE (16))
#define STUNBALL_LIFE 90 // Duration is as long as you hold down the button.
#define STUNBALL_HITS 10
#define STUNBALL_DAMAGE 0
#define STUNBALL_START_OFFSET RES_SCALE (36)
#define STUNBALL_BLAST_OFFSET RES_SCALE (4)
#define SHOCKWAVE_RANGE_MID RES_SCALE (85)
#define SHOCKWAVE_RANGE_END RES_SCALE (116)
#define FULL_STUN_DURATION 90
#define PARTIAL_STUN_DURATION 60

//#define MAX_THRUST  10
//#define TURN_WAIT   17 // JMS: Kept these defines for now since they might have some effect
//#define THRUST_WAIT  6 // in the speed calculations in some point. Not sure though, got to check in some point!

// HD
#define BLASTER_SPEED_HD RES_SCALE (BLASTER_SPEED)
#define BLASTER_RANGE_HD (BLASTER_SPEED_HD * BLASTER_LIFE)

static RACE_DESC exp_desc =
{
	{ /* SHIP_INFO */
		"explorer",
		FIRES_FORE,
		25, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		EXP_RACE_STRINGS,
		EXP_ICON_MASK_PMAP_ANIM,	// Explorer icons
		EXP_MICON_MASK_PMAP_ANIM,	// Explorer melee icons
		NULL, NULL, NULL, SHIP_IS_NOT_DAMAGED
	},
	{ /* FLEET_STUFF */
		0, /* Initial sphere of influence radius */
		{ /* Known location (center of SoI) */
			0, 0,
		},
	},
	{
		EXPLORER_MAX_THRUST,
		EXPLORER_THRUST_INCREMENT,
		ENERGY_REGENERATION,
		WEAPON_ENERGY_COST,
		SPECIAL_ENERGY_COST,
		ENERGY_WAIT,
		EXPLORER_TURN_WAIT,
		EXPLORER_THRUST_WAIT,
		WEAPON_WAIT,
		SPECIAL_WAIT,
		SHIP_MASS,
	},
	{
		{
			EXP_BIG_MASK_PMAP_ANIM,
			EXP_MED_MASK_PMAP_ANIM,
			EXP_SML_MASK_PMAP_ANIM,
		},
		{
			PULSE_BIG_MASK_PMAP_ANIM,
			PULSE_MED_MASK_PMAP_ANIM,
			PULSE_SML_MASK_PMAP_ANIM,
		},
		{
			STUNNER_BIG_MASK_PMAP_ANIM,
			STUNNER_MED_MASK_PMAP_ANIM,
			STUNNER_SML_MASK_PMAP_ANIM,
		},
		{
			SIS_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL,
			0, 0, 0, 0, 0
		},
		EXP_VICTORY_SONG,
		EXP_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		BLASTER_RANGE,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

// Stunner status effect.
static void electrify (ELEMENT *ElementPtr)
{
	if (ElementPtr->hTarget == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING;
	}
	else
	{
		ELEMENT *eptr;
		
		LockElement (ElementPtr->hTarget, &eptr);

		ElementPtr->next.location = eptr->next.location;

		if (ElementPtr->turn_wait)
		{
			HELEMENT hEffect;
			STARSHIP *StarShipPtr;
			COUNT firstElectrifyFrame = RES_BOOL (15, 27);
				
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);

			if (GetFrameIndex (ElementPtr->next.image.frame) < firstElectrifyFrame)
				ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->next.image.frame, firstElectrifyFrame);

			GetElementStarShip (eptr, &StarShipPtr);

			// Disable energy regeneration. Do not prevent Androsynth Blazer degeneration.
			if (!(eptr->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.special))
				StarShipPtr->energy_counter += 1;

			// Disable propulsion and weapons.
			StarShipPtr->ship_input_state = (StarShipPtr->ship_input_state &
				~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL));

			hEffect = AllocElement ();
			if (hEffect)
			{
				LockElement (hEffect, &eptr);
			
				eptr->state_flags = FINITE_LIFE | NONSOLID | CHANGING;
				eptr->playerNr = ElementPtr->playerNr;
				eptr->life_span = 1;
				eptr->current = ElementPtr->current;
				eptr->next = ElementPtr->next;
				eptr->preprocess_func = electrify;
				SetPrimType (&(GLOBAL (DisplayArray))[eptr->PrimIndex], STAMP_PRIM);
			
				GetElementStarShip (ElementPtr, &StarShipPtr);
				SetElementStarShip (eptr, StarShipPtr);
				eptr->hTarget = ElementPtr->hTarget;
			
				UnlockElement (hEffect);
				PutElement (hEffect);
			}
		}

		UnlockElement (ElementPtr->hTarget);
	}
}

// This is used to fluctuate the stunner.
static void animate (ELEMENT *ElementPtr)
{
	ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
	ElementPtr->state_flags |= CHANGING;
}

// This is used to fluctuate the blaster weapon.
static void animate_blaster (ELEMENT *ElementPtr)
{
	if (GetFrameIndex (ElementPtr->current.image.frame) >= 3)
		ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, 0);
	else
		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
	
	ElementPtr->state_flags |= CHANGING;
}

// Stun ball collision.
static void stunner_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	COUNT firstElectrifyFrame = RES_BOOL (15, 27);
	
	if (ElementPtr1->state_flags & PLAYER_SHIP)
	{
		HELEMENT hStunElement, hNextElement;
		ELEMENT *StunPtr;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);

		// Check to see if the enemy ship is already electrified.
		for (hStunElement = GetHeadElement ();
			hStunElement; hStunElement = hNextElement)
		{
			LockElement (hStunElement, &StunPtr);
			if (elementsOfSamePlayer(StunPtr, ElementPtr0)
				&& (StunPtr->preprocess_func == electrify))
			{
				UnlockElement (hStunElement);
				break;
			}
			hNextElement = GetSuccElement (StunPtr);
			UnlockElement (hStunElement);
		}

		if (hStunElement || (hStunElement = AllocElement ()))
		{
			LockElement (hStunElement, &StunPtr);

			// Put a status effect on the enemy ship if one is not already there.
			if (StunPtr->state_flags == 0)
			{
				InsertElement (hStunElement, GetHeadElement ());

				StunPtr->current = ElementPtr0->next;
				StunPtr->current.image.frame = SetAbsFrameIndex (StunPtr->current.image.frame, firstElectrifyFrame);
				StunPtr->next = StunPtr->current;
				StunPtr->playerNr = ElementPtr0->playerNr;
				StunPtr->state_flags = FINITE_LIFE | NONSOLID | CHANGING;
				StunPtr->preprocess_func = electrify;
				SetPrimType (&(GLOBAL (DisplayArray))[StunPtr->PrimIndex],	NO_PRIM);

				SetElementStarShip (StunPtr, StarShipPtr);
				GetElementStarShip (ElementPtr1, &StarShipPtr);
				StunPtr->hTarget = StarShipPtr->hShip;
			}

			StunPtr->life_span = FULL_STUN_DURATION;
			StunPtr->turn_wait = 1;
			UnlockElement (hStunElement);
		}

		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= DISAPPEARING | COLLISION | NONSOLID;
	}
	// Asteroids and planets break the stun ball.
	else if (ElementPtr1->playerNr == NEUTRAL_PLAYER_NUM)
	{
		HELEMENT hBlastElement;

		hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
		if (hBlastElement)
		{
			ELEMENT *BlastElementPtr;

			LockElement (hBlastElement, &BlastElementPtr);
			BlastElementPtr->life_span = 9;
			BlastElementPtr->preprocess_func = animate;

			BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
			BlastElementPtr->current.image.frame = SetAbsFrameIndex (BlastElementPtr->current.image.farray[0], firstElectrifyFrame);

			ZeroVelocityComponents (&ElementPtr0->velocity);

			UnlockElement (hBlastElement);
		}
	}
	// Pass through enemy weapons. Sustain damage from them upon contact, but do not collide.
}

// Stunner explosion.
static void shockwave (ELEMENT *ElementPtr)
{
	COUNT firstShockwaveFrame = RES_BOOL (3, 1);
	COUNT lastShockwaveFrame = RES_BOOL (14, 26);
	COUNT firstElectrifyFrame = RES_BOOL (15, 27);
	
	ZeroVelocityComponents (&ElementPtr->velocity);

	ElementPtr->state_flags |= (FINITE_LIFE | NONSOLID | CHANGING);
	
	ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);

	if (GetFrameIndex (ElementPtr->next.image.frame) > lastShockwaveFrame 
		|| GetFrameIndex (ElementPtr->next.image.frame) < firstShockwaveFrame)
		ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->next.image.frame, firstShockwaveFrame);
	
	// A shockwave hit will occur at two different intervals.
	if (ElementPtr->life_span == 6 || ElementPtr->life_span == 2)
	{
		HELEMENT hElement, hNextElement, hTarget;
		STARSHIP *StarShipPtr, *EnemyStarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		hTarget = 0;

		// Cycle through all objects in the arena.
		for (hElement = GetHeadElement ();
			hElement != 0; hElement = hNextElement)
		{
			ELEMENT *ObjPtr;

			LockElement (hElement, &ObjPtr);
			hNextElement = GetSuccElement (ObjPtr);
			if (CollidingElement (ObjPtr))
			{
				SIZE delta_x, delta_y;

				// Distance check.
				if ((delta_x = ObjPtr->next.location.x
						- ElementPtr->next.location.x) < 0)
					delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y
						- ElementPtr->next.location.y) < 0)
					delta_y = -delta_y;
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				
				// Only pay attention to objects within the shockwave.
				if ((ElementPtr->life_span == 6
					&& delta_x <= SHOCKWAVE_RANGE_MID && delta_y <= SHOCKWAVE_RANGE_MID
						&& ((long)(delta_x * delta_x) + (long)(delta_y * delta_y)) <=
							(long)(SHOCKWAVE_RANGE_MID * SHOCKWAVE_RANGE_MID))
				|| (ElementPtr->life_span == 2
					&& delta_x <= SHOCKWAVE_RANGE_END && delta_y <= SHOCKWAVE_RANGE_END
						&& ((long)(delta_x * delta_x) + (long)(delta_y * delta_y)) <=
							(long)(SHOCKWAVE_RANGE_END * SHOCKWAVE_RANGE_END)))
				{
					// Enemy ship check.
					if ((ObjPtr->state_flags & PLAYER_SHIP)
					    && !elementsOfSamePlayer(ElementPtr, ObjPtr))
					{
						hTarget = hElement;
						GetElementStarShip (ObjPtr, &EnemyStarShipPtr);
					}
				}
			}

			UnlockElement (hElement);
		}

		if (hTarget) // Did the enemy ship get hit?
		{
			ELEMENT *StunPtr;

			// Check to see if the enemy ship is already electrified.
			for (hElement = GetHeadElement ();
				hElement; hElement = hNextElement)
			{
				LockElement (hElement, &StunPtr);
				if (elementsOfSamePlayer(StunPtr, ElementPtr)
					&& (StunPtr->preprocess_func == electrify))
				{
					UnlockElement (hElement);
					break;
				}
				hNextElement = GetSuccElement (StunPtr);
				UnlockElement (hElement);
			}

			if (hElement || (hElement = AllocElement ()))
			{
				LockElement (hElement, &StunPtr);

				// Put a status effect on the enemy ship if one is not already there.
				if (StunPtr->state_flags == 0)
				{
					InsertElement (hElement, GetHeadElement ());
				
					StunPtr->current = ElementPtr->next;
					StunPtr->current.image.frame = SetAbsFrameIndex (StunPtr->current.image.frame, firstElectrifyFrame + 1);
					StunPtr->next = StunPtr->current;
					StunPtr->state_flags = FINITE_LIFE | NONSOLID | CHANGING;
					StunPtr->playerNr = ElementPtr->playerNr;
					StunPtr->preprocess_func = electrify;
					StunPtr->collision_func = stunner_collision;
					SetPrimType (&(GLOBAL (DisplayArray))[StunPtr->PrimIndex], NO_PRIM);

					SetElementStarShip (StunPtr, StarShipPtr);
					StunPtr->hTarget = EnemyStarShipPtr->hShip;
				}

				StunPtr->life_span = PARTIAL_STUN_DURATION;
				StunPtr->turn_wait = 1;
				UnlockElement (hElement);
			}
		}

		UnlockElement (hTarget);
	}
}

// This code maintains the stun ball projectile.
static void stunner_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	COUNT lastStunballFrame = !IS_HD ? 2 : 0;

	GetElementStarShip (ElementPtr, &StarShipPtr);

	ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
	ElementPtr->state_flags |= CHANGING;
	
	if (GetFrameIndex (ElementPtr->current.image.frame) == lastStunballFrame)
		ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, 0);

	if (StarShipPtr->cur_status_flags & SPECIAL)
	{
		++ElementPtr->life_span; // Keep it going while the key is pressed.
	}
	else
	{
		// Trigger stunner explosion.
		ElementPtr->life_span = 13;
		ElementPtr->preprocess_func = shockwave;

		ProcessSound (SetAbsSoundIndex // Shockwave!
					(StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);

		ElementPtr->current.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, lastStunballFrame + 1);
	}
}

static void initialize_stunner (ELEMENT *ShipPtr)
{
	HELEMENT SMissile;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = STUNBALL_START_OFFSET;
	MissileBlock.speed = STUNBALL_SPEED;
	MissileBlock.hit_points = STUNBALL_HITS;
	MissileBlock.damage = STUNBALL_DAMAGE;
	MissileBlock.life = STUNBALL_LIFE;
	MissileBlock.preprocess_func = stunner_preprocess;
	MissileBlock.blast_offs = STUNBALL_BLAST_OFFSET;
	SMissile = initialize_missile (&MissileBlock);
	
	if (SMissile)
	{
		ELEMENT *StunballPtr;
		
		LockElement (SMissile, &StunballPtr);
		SetElementStarShip (StunballPtr, StarShipPtr);
		StunballPtr->death_func = NULL;
		StunballPtr->collision_func = stunner_collision;
		UnlockElement (SMissile);
		PutElement (SMissile);
	}
}

static COUNT
initialize_teststunner (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = STUNBALL_START_OFFSET;
	MissileBlock.speed = STUNBALL_SPEED;
	MissileBlock.hit_points = STUNBALL_HITS;
	MissileBlock.damage = STUNBALL_DAMAGE;
	MissileBlock.life = STUNBALL_LIFE;
	MissileBlock.preprocess_func = stunner_preprocess;
	MissileBlock.blast_offs = STUNBALL_BLAST_OFFSET;
	MissileArray[0] = initialize_missile (&MissileBlock);
	
	return (1);
}

// Melee-related postprocess.
/* static void exp_battle_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	GetElementStarShip (ElementPtr, &StarShipPtr);
} */

// Melee-related preprocess.
static void exp_battle_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	GetElementStarShip (ElementPtr, &StarShipPtr);

	// Only let one stun ball out at a time, but allow special_counter to count down as normal.
	if (StarShipPtr->special_counter == 1
			&& (StarShipPtr->cur_status_flags & StarShipPtr->old_status_flags & SPECIAL))
		++StarShipPtr->special_counter;

	if ((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST)
	{
		initialize_stunner (ElementPtr);

		ProcessSound (SetAbsSoundIndex // Launch stunner ball.
			(StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);

		DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);
		StarShipPtr->special_counter = SPECIAL_WAIT;
	}
}

static void blaster_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	HELEMENT hBlastElement;

	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		ELEMENT *BlastElementPtr;

		LockElement (hBlastElement, &BlastElementPtr);
		BlastElementPtr->life_span = 2;
		BlastElementPtr->current.image.farray = ElementPtr0->next.image.farray;
		BlastElementPtr->current.image.frame = SetAbsFrameIndex (ElementPtr0->current.image.frame, 0);
		BlastElementPtr->preprocess_func = NULL;

		UnlockElement (hBlastElement);
	}
}

static COUNT initialize_explorer_weaponry (ELEMENT *ShipPtr, HELEMENT BlasterArray[])
{
	COUNT num_blasters;
	
	BYTE nt;
	COUNT i;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock[6];
	MISSILE_BLOCK *lpMB;

	
	COORD cx, cy;
	static COUNT blaster_side[NUM_SIDES]={0,0};
	COUNT facing, angle;
	SIZE offs_x, offs_y;
	
	num_blasters = 0;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	facing = StarShipPtr->ShipFacing;
	angle = FACING_TO_ANGLE (facing);

	blaster_side[ShipPtr->playerNr] = 
		(blaster_side[ShipPtr->playerNr] + 1) % 2;
	
	cx = ShipPtr->next.location.x;
	cy = ShipPtr->next.location.y;
	
	if(blaster_side[ShipPtr->playerNr])
	{
		offs_x = -SINE (angle, EXP_HORZ_OFFSET_2);
		offs_y = COSINE (angle, EXP_HORZ_OFFSET_2);
	}
	else
	{
		offs_x = -SINE (angle, EXP_HORZ_OFFSET_3);
		offs_y = COSINE (angle, EXP_HORZ_OFFSET_3);
	}
		
	for (i = 0, lpMB = &MissileBlock[0]; i < 1; ++i)
	{
		BYTE which_gun;

		// JMS: Chmmr Explorer always has weapon in nose weapon slot.
		if(i == 0)
			which_gun = GUN_WEAPON;
		else
			which_gun = 0;
		
		if (which_gun >= GUN_WEAPON && which_gun <= CANNON_WEAPON)
		{
			which_gun -= GUN_WEAPON - 1;
			lpMB->cx = cx + offs_x;
			lpMB->cy = cy + offs_y;
			lpMB->farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
			lpMB->sender = ShipPtr->playerNr;
			lpMB->flags = IGNORE_SIMILAR;
			lpMB->blast_offs = BLASTER_OFFSET;
			lpMB->speed = BLASTER_SPEED_HD;
			lpMB->preprocess_func = animate_blaster;
			lpMB->hit_points = BLASTER_HITS * which_gun;
			lpMB->damage = BLASTER_DAMAGE * which_gun;
			lpMB->life = BLASTER_LIFE + ((BLASTER_LIFE >> 2) * (which_gun - 1));

			// Which weapon graphics to use
			if (which_gun == 1)
				lpMB->index = 0;
			else if (which_gun == 2)
				lpMB->index = 9;
			else
				lpMB->index = 16;
			
			switch (i)
			{
				case 0: /* NOSE WEAPON */
					lpMB->pixoffs = EXP_VERT_OFFSET;
					lpMB->face = StarShipPtr->ShipFacing;
					break;
				case 1: /* SPREAD WEAPON */
					lpMB->pixoffs = EXP_VERT_OFFSET;
					lpMB->face = NORMALIZE_FACING (StarShipPtr->ShipFacing + 1);
					lpMB[1] = lpMB[0];
					++lpMB;
					lpMB->face = NORMALIZE_FACING (StarShipPtr->ShipFacing - 1);
					break;
				case 2: /* SIDE WEAPON */
					lpMB->pixoffs = EXP_HORZ_OFFSET;
					lpMB->face = NORMALIZE_FACING (StarShipPtr->ShipFacing + ANGLE_TO_FACING (QUADRANT));
					lpMB[1] = lpMB[0];
					++lpMB;
					lpMB->face = NORMALIZE_FACING (StarShipPtr->ShipFacing - ANGLE_TO_FACING (QUADRANT));
					break;
			}
			
			++lpMB;
		}
	}
	
	nt = (BYTE)((4 - 0) & 3); // Was (BYTE)((4 - num_trackers) & 3);
	num_blasters = lpMB - &MissileBlock[0];
	for (i = 0, lpMB = &MissileBlock[0]; i < num_blasters; ++i, ++lpMB)
	{
		if ((BlasterArray[i] = initialize_missile (lpMB)))
		{
			ELEMENT *BlasterPtr;
			
			LockElement (BlasterArray[i], &BlasterPtr);
			BlasterPtr->collision_func = blaster_collision;
			BlasterPtr->turn_wait = MAKE_BYTE (nt, nt);
			UnlockElement (BlasterArray[i]);
		}
	
	}
	return (num_blasters);

}

static void exp_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern, COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (MANEUVERABILITY (&StarShipPtr->RaceDescPtr->cyborg_control) < RES_SCALE (MEDIUM_SHIP)
				&& lpEvalDesc->MoveState == ENTICE
				&& (!(lpEvalDesc->ObjectPtr->state_flags & CREW_OBJECT)
				|| lpEvalDesc->which_turn <= 8)
				&& (!(lpEvalDesc->ObjectPtr->state_flags & FINITE_LIFE)
				|| (lpEvalDesc->ObjectPtr->mass_points >= 4
				&& lpEvalDesc->which_turn == 2
				&& ObjectsOfConcern[ENEMY_SHIP_INDEX].which_turn > 16)))
			lpEvalDesc->MoveState = PURSUE;
	}

	// Act like Melnorme towards enemy ships: Chase most, entice ships with lazers
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < MAX_ENERGY / 6)
			lpEvalDesc->MoveState = AVOID;
		else if (StarShipPtr->RaceDescPtr->ship_info.energy_level < MAX_ENERGY / 3)
			lpEvalDesc->MoveState = ENTICE;
		else
		{
			STARSHIP *EnemyStarShipPtr;
			
			GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
			if (!(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & IMMEDIATE_WEAPON))
				lpEvalDesc->MoveState = PURSUE;
		}
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	
	// JMS: Use special like Chenjesu uses its primary weapon
	if (lpEvalDesc->ObjectPtr)
	{
		HELEMENT h, hNext;
		ELEMENT *StunnerPtr;
		
		h = (StarShipPtr->old_status_flags & SPECIAL) ? GetTailElement () : (HELEMENT)0;
		for (; h; h = hNext)
		{
			LockElement (h, &StunnerPtr);
			hNext = GetPredElement (StunnerPtr);
			if (!(StunnerPtr->state_flags & NONSOLID)
				&& StunnerPtr->preprocess_func
				&& StunnerPtr->life_span > 0
				&& elementsOfSamePlayer (StunnerPtr, ShipPtr))
			{
				if (ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr)
				{
					COUNT which_turn;
					
					if ((which_turn = PlotIntercept (StunnerPtr, ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							StunnerPtr->life_span, RES_SCALE (200))) == 0
						|| (which_turn == 1
							&& PlotIntercept (StunnerPtr, ObjectsOfConcern[ENEMY_SHIP_INDEX].ObjectPtr,
							StunnerPtr->life_span, 0) == 0))
						StarShipPtr->ship_input_state &= ~SPECIAL;
					else if (StarShipPtr->weapon_counter == 0)
					{
						StarShipPtr->ship_input_state |= SPECIAL;
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
			StarShipPtr->RaceDescPtr->init_weapon_func = initialize_teststunner;
			
			if (StarShipPtr->old_status_flags & SPECIAL)
			{
				StarShipPtr->ship_input_state &= ~SPECIAL;
				if (lpEvalDesc == &ObjectsOfConcern[ENEMY_WEAPON_INDEX])
					StarShipPtr->weapon_counter = 3;
			}
			else if (StarShipPtr->weapon_counter == 0
					 && ship_weapons (ShipPtr, lpEvalDesc->ObjectPtr, RES_SCALE (200))
					 && StarShipPtr->RaceDescPtr->ship_info.energy_level > MAX_ENERGY * 2 / 3)
				StarShipPtr->ship_input_state |= SPECIAL;
			
			StarShipPtr->RaceDescPtr->init_weapon_func = initialize_explorer_weaponry;
		}
	}
}

RACE_DESC*
init_explorer (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD)
	{
		exp_desc.characteristics.max_thrust = RES_SCALE (EXPLORER_MAX_THRUST);
		exp_desc.characteristics.thrust_increment = RES_SCALE (EXPLORER_THRUST_INCREMENT);
		exp_desc.cyborg_control.WeaponRange = BLASTER_RANGE_HD;
	}
	else
	{
		exp_desc.characteristics.max_thrust = EXPLORER_MAX_THRUST;
		exp_desc.characteristics.thrust_increment = EXPLORER_THRUST_INCREMENT;
		exp_desc.cyborg_control.WeaponRange = BLASTER_RANGE;
	}

	exp_desc.preprocess_func = exp_battle_preprocess;
	exp_desc.init_weapon_func = initialize_explorer_weaponry;
	exp_desc.cyborg_control.intelligence_func = exp_intelligence;

	RaceDescPtr = &exp_desc;

	return (RaceDescPtr);
}
