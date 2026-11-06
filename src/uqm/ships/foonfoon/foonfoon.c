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
#include "foonfoon.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/log.h"
#include "libs/mathlib.h"
#include "libs/sound/sound.h" // For StopSource
#include <math.h> // For sqrt

#define MAX_CREW 12
#define MAX_ENERGY 32
#define ENERGY_REGENERATION 1
#define WEAPON_ENERGY_COST 3
#define SPECIAL_ENERGY_COST 10
#define ENERGY_WAIT 2
#define MAX_THRUST 52
#define THRUST_INCREMENT 12
#define TURN_WAIT 1
#define THRUST_WAIT 1
#define WEAPON_WAIT 1
#define SPECIAL_WAIT 0

#define SHIP_MASS 2
#define MISSILE_SPEED DISPLAY_TO_WORLD (27)
#define MISSILE_LIFE 6
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)

// Weapon gfx
#define REVERSE_DIR (BYTE)(1 << 7)
#define NUM_SHIP_FACINGS 16
#define FOCUSBALL_OFFSET (RES_SCALE (7))
#define FOCUSBALL_FRAME_STARTINDEX 64
#define NUM_FOCUSBALL_FRAMES 3
#define NUM_BURST_FRAMES 4
// Weapon attributes
#define BURST_CHARGE_TIME 39 // Divide this by BATTLE_FRAME_RATE to get the time in seconds.
#define NUM_SABERS 5
#define DERVISH_DEGENERATION (-1)
#define DERVISH_COOLDOWN_TIME 36 // Seconds *BATTLE_FRAME_RATE
#define DERVISH_THRUST RES_SCALE (80)
#define RECOIL_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE (6)))
#define MAX_RECOIL_VELOCITY (RECOIL_VELOCITY * 6)

// HD
#define MAX_THRUST_HD RES_SCALE (MAX_THRUST)
#define CONST_MAX_THRUST_HD 208
#define MISSILE_SPEED_HD RES_SCALE (MISSILE_SPEED)
#define MISSILE_RANGE_HD (MISSILE_SPEED_HD * MISSILE_LIFE)
#define THRUST_INCREMENT_HD RES_SCALE (THRUST_INCREMENT)

static RACE_DESC foonfoon_desc =
{
	{ /* SHIP_INFO */
		"typhoon",
		FIRES_FORE | IMMEDIATE_WEAPON,
		18, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		FOONFOON_RACE_STRINGS,
		FOONFOON_ICON_MASK_PMAP_ANIM,
		FOONFOON_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial SoI radius */
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
			FOONFOON_BIG_MASK_PMAP_ANIM,
			FOONFOON_MED_MASK_PMAP_ANIM,
			FOONFOON_SML_MASK_PMAP_ANIM,
		},
		{
			FOONFOON_BURST_BIG_MASK_PMAP_ANIM,
			FOONFOON_BURST_MED_MASK_PMAP_ANIM,
			FOONFOON_BURST_SML_MASK_PMAP_ANIM,
		},
		{
			FOONFOON_SABRE_BIG_MASK_PMAP_ANIM,
			FOONFOON_SABRE_MED_MASK_PMAP_ANIM,
			FOONFOON_SABRE_SML_MASK_PMAP_ANIM,
		},
		{
			FOONFOON_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL,
			0, 0, 0, 0, 0
		},
		FOONFOON_VICTORY_SONG,
		FOONFOON_SHIP_SOUNDS,
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

// Forward declarations.
static COUNT
initialize_test_burst (ELEMENT *ElementPtr, HELEMENT BurstArray[]);

static COUNT
initialize_test_saber (ELEMENT *ElementPtr, HELEMENT SabreArray[]);

static COUNT 
initialize_focusball (ELEMENT *ShipPtr, HELEMENT FocusArray[]);

static COUNT
initialize_focusball_which_bursts (ELEMENT *ShipPtr, HELEMENT BurstArray[]);

static void
focusball_postprocess (ELEMENT *ElementPtr);

// The Almighty AI.
static void
foonfoon_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern, COUNT ConcernCounter)
{
#define IS_FOCUSBALL(ptr) (ptr->postprocess_func == focusball_postprocess && ptr->mass_points > 0)
	BYTE old_count;
	STARSHIP *StarShipPtr, *FBStarShipPtr;
	EVALUATE_DESC *lpEvalDesc;
	HELEMENT hElement, hNextElement;
	ELEMENT  *FocusballCandidatePtr;
	SIZE charge_amount = 0;
	SIZE charge_time_left = 255;
	BYTE enemy_is_Chmmr = 0;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	
	// Find the focusball element (if one exists).
	for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
	{
		LockElement (hElement, &FocusballCandidatePtr);
		hNextElement = GetSuccElement (FocusballCandidatePtr);
		
		// Ensure this is not the enemy's focusball.
		if (IS_FOCUSBALL(FocusballCandidatePtr))
		{
			GetElementStarShip (FocusballCandidatePtr, &FBStarShipPtr);
			
			// If this is this ship's focusball, see how charged the focusball is.
			if (FBStarShipPtr == StarShipPtr)
			{
				charge_amount	 = FocusballCandidatePtr->mass_points;
				charge_time_left = FocusballCandidatePtr->thrust_wait;
				break;
			}
		}
	}
	
	// Release the SPECIAL button when energy is drained.
	if (StarShipPtr->RaceDescPtr->ship_info.energy_level == 0 || StarShipPtr->RaceDescPtr->ship_info.energy_level == MAX_ENERGY)
		StarShipPtr->ship_input_state &= ~SPECIAL;
	// Otherwise, when dervish has been started, keep dervishing until all energy is drained.
	else if (StarShipPtr->ship_input_state & SPECIAL 
			 && StarShipPtr->RaceDescPtr->ship_info.energy_level > 0)
	{
		StarShipPtr->ship_input_state &= ~WEAPON;
		StarShipPtr->ship_input_state |= SPECIAL;
		
		lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
		goto end_intelligence;
	}
	
	// We test the possibility of primary weapon hitting with a test function.
	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_burst;
	old_count = StarShipPtr->weapon_counter;
	
	// This, like the lot of this code is copied from Melnorme AI.
	// I don't know if the following has any real meaning, but since this works fine, let's keep it...
	if (StarShipPtr->weapon_counter == WEAPON_WAIT)
		StarShipPtr->weapon_counter = 0;
	
	// Action towards enemy ship: If we don't have weapon ready and don't have enough battery,
	// escape until battery is charged. When we have enough battery, pursue the enemy!!
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		
		// Check if the enemy ship is Chmmr. We want to use the dervish more against it.
		if(EnemyStarShipPtr->SpeciesID == CHMMR_ID)
			enemy_is_Chmmr = 1;
		else
			enemy_is_Chmmr = 0;
		
		if (StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST + WEAPON_ENERGY_COST
			&& !(StarShipPtr->old_status_flags & WEAPON)
			&& !enemy_is_Chmmr)
			lpEvalDesc->MoveState = ENTICE;
		else if (enemy_is_Chmmr && StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST + WEAPON_ENERGY_COST)
			lpEvalDesc->MoveState = ENTICE;
		else
			lpEvalDesc->MoveState = PURSUE;
	}
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	
	// Charge focusball...
	if (StarShipPtr->weapon_counter == 0
		&& (old_count != 0
			|| ((StarShipPtr->special_counter
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level >= MAX_ENERGY - 2 * WEAPON_ENERGY_COST)
				&& !(StarShipPtr->ship_input_state & WEAPON))
			)
		)
		StarShipPtr->ship_input_state ^= WEAPON;
	
	// ...but let it go just before it would start inducing damage to own ship.
	if (charge_amount >= 4 && charge_time_left < 3)
		StarShipPtr->ship_input_state &= ~WEAPON;
	
	// Consider dervishing if we don't have a heavily charged primary weapon handy and we have enough battery.
	if (StarShipPtr->special_counter == 0
		&& (enemy_is_Chmmr
			|| (charge_amount < 2 && StarShipPtr->RaceDescPtr->ship_info.energy_level >= MAX_ENERGY - 4 * WEAPON_ENERGY_COST))
		)
	{
		BYTE old_input_state;
		old_input_state = StarShipPtr->ship_input_state;
		
		// The final decision of "to dervish or not to dervish" is made by evaluating a test weapon function,
		// which is pretty similar to the primary weapon test function. This one only has different stats.
		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_saber;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ENEMY_SHIP_INDEX + 1);
		
		// Since we faked using primary weapon even though we really are gonna use special,
		// change the WEAPON button press to SPECIAL.
		if (StarShipPtr->ship_input_state & WEAPON)
		{
			BYTE right_or_left;
			right_or_left = TFB_Random () % 2;
			
			StarShipPtr->ship_input_state &= ~WEAPON;
			StarShipPtr->ship_input_state |= SPECIAL;
			
			// Also turn the ship to either direction, otherwise the dervish won't activate.
			if (right_or_left)
				StarShipPtr->ship_input_state |= RIGHT;
			else
				StarShipPtr->ship_input_state |= LEFT;
		}
		
		StarShipPtr->ship_input_state = (unsigned char)(old_input_state | (StarShipPtr->ship_input_state & SPECIAL));
	}
	
	// Check if the enemy weapon is a zapsat. We want to use the dervish more against it.
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		STARSHIP *EnemyStarShipPtr;
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		
		if(EnemyStarShipPtr->SpeciesID == CHMMR_ID 
		   && lpEvalDesc->ObjectPtr->mass_points == 10
		   && StarShipPtr->special_counter == 0
		   && StarShipPtr->RaceDescPtr->ship_info.energy_level >= MAX_ENERGY / 2)
		{
			BYTE old_input_state;
			old_input_state = StarShipPtr->ship_input_state;
			
			// The final decision of "to dervish or not to dervish" is made by evaluating a test weapon function,
			// which is pretty similar to the primary weapon test function. This one only has different stats.
			StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_saber;
			ship_intelligence (ShipPtr, ObjectsOfConcern, ENEMY_SHIP_INDEX + 1);
			
			// Since we faked using primary weapon even though we really are gonna use special,
			// change the WEAPON button press to SPECIAL.
			if (StarShipPtr->ship_input_state & WEAPON)
			{
				BYTE right_or_left;
				right_or_left = TFB_Random () % 2;
				
				StarShipPtr->ship_input_state &= ~WEAPON;
				StarShipPtr->ship_input_state |= SPECIAL;
				
				// Also turn the ship to either direction, otherwise the dervish won't activate.
				if (right_or_left)
					StarShipPtr->ship_input_state |= RIGHT;
				else
					StarShipPtr->ship_input_state |= LEFT;
			}
			
			StarShipPtr->ship_input_state = (unsigned char)(old_input_state | (StarShipPtr->ship_input_state & SPECIAL));
		}
	}
	
	StarShipPtr->weapon_counter = old_count;
	
end_intelligence:
	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_focusball_which_bursts;
}

// This animates the primary burst.
static void
animate_burst (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if ((GetFrameIndex (ElementPtr->next.image.frame) > 1) && !((GetFrameIndex (ElementPtr->next.image.frame) + 1) % NUM_BURST_FRAMES))
		ElementPtr->next.image.frame = DecFrameIndex (ElementPtr->current.image.frame);
	else
		ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
	
	ElementPtr->state_flags |= CHANGING;
}

// This turns the focusball into a primary burst.
static void
fire_burst (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT hMissile;
	ELEMENT *ShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement (StarShipPtr->hShip, &ShipPtr);
	
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = (StarShipPtr->ShipFacing) * NUM_BURST_FRAMES;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags =  IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed = (MISSILE_SPEED << RESOLUTION_FACTOR);
	MissileBlock.hit_points = ElementPtr->mass_points;
	MissileBlock.damage = ElementPtr->mass_points;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = animate_burst;
	MissileBlock.blast_offs = RES_SCALE (50); // Don't change this value. Otherwise a special clause in weapon.c will stop working :(
	
	hMissile = initialize_missile (&MissileBlock);
	
	if (hMissile)
	{
		ELEMENT *BurstPtr;
		SIZE dx, dy;
		
		LockElement (hMissile, &BurstPtr);
		SetElementStarShip (BurstPtr, StarShipPtr);
		GetCurrentVelocityComponents (&ShipPtr->velocity, &dx, &dy);
		DeltaVelocityComponents (&BurstPtr->velocity, dx, dy);
		BurstPtr->current.location.x -= VELOCITY_TO_WORLD (dx);
		BurstPtr->current.location.y -= VELOCITY_TO_WORLD (dy);
		UnlockElement (hMissile);
		PutElement (hMissile);

		// Play different 'shot release' sound according to how powerful the shot is.
		if (ElementPtr->mass_points >= 7)
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 10), ElementPtr);
		else if (ElementPtr->mass_points == 4)
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 9), ElementPtr);
		else if (ElementPtr->mass_points == 2)
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 8), ElementPtr);
		else
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 7), ElementPtr);
	}

	// Recoil the ship with powerful enough shots.
	if (ElementPtr->mass_points >= 4)
	{
		COUNT angle;
		SIZE cur_delta_x, cur_delta_y;
		BYTE mult;
		
		// More powerful shots knock the ship back more.
		if (ElementPtr->mass_points >= 7)
		{
			mult = 2;
		}
		else
			mult = 1;
		
		StarShipPtr->cur_status_flags &= ~SHIP_AT_MAX_SPEED;
		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		
		DeltaVelocityComponents (&ShipPtr->velocity,
								 COSINE (angle, RECOIL_VELOCITY * mult),
								 SINE (angle, RECOIL_VELOCITY * mult));
		GetCurrentVelocityComponents (&ShipPtr->velocity, &cur_delta_x, &cur_delta_y);
		
		if ((long)cur_delta_x * (long)cur_delta_x
			+ (long)cur_delta_y * (long)cur_delta_y
			> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
		{
			angle = ARCTAN (cur_delta_x, cur_delta_y);
			SetVelocityComponents (&ShipPtr->velocity,
								   COSINE (angle, MAX_RECOIL_VELOCITY),
								   SINE (angle, MAX_RECOIL_VELOCITY));
		}
		
		UnlockElement (StarShipPtr->hShip);
	}
}

// This function plays the primary weapon charge-up sound.
static void
focusball_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->thrust_wait == 1 && ElementPtr->mass_points < 7)
	{
		BYTE chargesound, i;
		ELEMENT *ShipPtr;
		STARSHIP *StarShipPtr;
		
		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipPtr);
		
		// End the old charge-up sound.
		for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
		{
			ELEMENT *posobj;
			if (!ChannelPlaying(i))
				continue;
			
			posobj = GetPositionalObject (i);
			if (posobj == ShipPtr)
				StopSource (i);
		}
		
		// Start playing the new charge-up sound.
		chargesound = 3 + (BYTE)(log(ElementPtr->mass_points * 2) / log(2));
		ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, chargesound), ShipPtr);
		
		UnlockElement (StarShipPtr->hShip);
	}	
}

// This function handles the life and death of a focusball.
static void
focusball_postprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
		ZeroVelocityComponents (&ElementPtr->velocity);
	else
	{
		HELEMENT hFocusBall;
		ELEMENT *EPtr;
		ELEMENT *ShipPtr;
		STARSHIP *StarShipPtr;
		COUNT frame_index;
		static BYTE shake[NUM_SIDES] = {0};
		
		// The original focusball actually dies in the end of this function.
		// But here we create a new one to replace it so no one notices.
		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipPtr);
		initialize_focusball (ShipPtr, &hFocusBall);
		
		// When holding WEAPON button down, don't drain energy.
		if (StarShipPtr->cur_status_flags & WEAPON)
		{
			DeltaEnergy (ShipPtr, 0);
			StarShipPtr->weapon_counter = WEAPON_WAIT;
		}
		// Ensure the focusball doesn't reach ship-damaging charge when holding the SPECIAL button down.
		else if (StarShipPtr->cur_status_flags & SPECIAL)
		{
			ElementPtr->mass_points = 0;
			ElementPtr->thrust_wait = 255;
		}
			
		// Copy the old focusball's values to the new focusball element.
		LockElement (hFocusBall, &EPtr);
		SetElementStarShip (EPtr, StarShipPtr);
		EPtr->thrust_wait = ElementPtr->thrust_wait;
		EPtr->mass_points = ElementPtr->mass_points;
		EPtr->turn_wait = ElementPtr->turn_wait;
		EPtr->current.image.frame = ElementPtr->current.image.frame;
		
		// Animate the focusball ping-pong between frames 0...2.
		frame_index = GetFrameIndex (EPtr->current.image.frame) - FOCUSBALL_FRAME_STARTINDEX;
		if	(((EPtr->turn_wait & REVERSE_DIR) && (frame_index % NUM_FOCUSBALL_FRAMES) != 0)
			||  (!(EPtr->turn_wait & REVERSE_DIR) && ((frame_index + 1) % NUM_FOCUSBALL_FRAMES) == 0))
		{
			--frame_index;
			EPtr->turn_wait |= REVERSE_DIR;
		}
		else
		{
			++frame_index;
			EPtr->turn_wait &= ~REVERSE_DIR;
		}
		EPtr->current.image.frame = SetAbsFrameIndex (EPtr->current.image.frame, FOCUSBALL_FRAME_STARTINDEX + frame_index);
		
		// When player releases WEAPON or SPECIAL key, kill the focusball.
		if (!(StarShipPtr->cur_status_flags & StarShipPtr->old_status_flags & WEAPON)
			&& !(StarShipPtr->cur_status_flags & StarShipPtr->old_status_flags & SPECIAL))
		{
			BYTE i;
			
			EPtr->life_span = 1;
			EPtr->preprocess_func = 0;
			if (ElementPtr->mass_points)
				EPtr->death_func = fire_burst;
			
			// End the charge-up sound.
			if (ShipPtr->crew_level > 0)
			{
				for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
				{
					ELEMENT *posobj;
					if (!ChannelPlaying(i))
						continue;
				
					posobj = GetPositionalObject (i);
					if (posobj == ShipPtr)
						StopSource (i);
				}
			}
			
			if (EPtr->mass_points >= 7)
			{
				// Also suffer some damage if the shot is big enough!
				if (!DeltaCrew (ShipPtr, -2))
					ShipPtr->life_span = 0;
				ShipPtr->state_flags |= CHANGING;
				ProcessSound (SetAbsSoundIndex (GameSounds, 4), ShipPtr);
			}
				
			// Return the ship to normal graphics if the ship was pulsating.
			if ((GetFrameIndex (ShipPtr->current.image.frame)) >= NUM_SHIP_FACINGS)
			{
				ShipPtr->next.image.frame = SetAbsFrameIndex (ShipPtr->next.image.frame, StarShipPtr->ShipFacing);
				ShipPtr->state_flags |= CHANGING;
			}
		}
		
		// Charge the primary weapon when holding down the WEAPON key.
		// The more charged up the primary, the more it deals damage.
		if (EPtr->thrust_wait == 0)
		{
			EPtr->mass_points <<= 1;
			EPtr->thrust_wait = BURST_CHARGE_TIME;
			
			// Nerf the most powerful shot's strength. Also give some 'mercy time' to avoid exploding your own ship too easily.
			if (EPtr->mass_points == 8)
			{
				EPtr->mass_points = 7;
				EPtr->thrust_wait += (BURST_CHARGE_TIME / 3);
			}
		}
		else
			--EPtr->thrust_wait;
		
		// At dangerously large charge levels, the ship starts pulsating.
		if (EPtr->mass_points == 4)
		{
			// Alternate between normal and blurred graphics.
			if ((GetFrameIndex (ShipPtr->current.image.frame)) < NUM_SHIP_FACINGS && EPtr->life_span > 1)
				ShipPtr->next.image.frame = SetAbsFrameIndex (ShipPtr->next.image.frame, StarShipPtr->ShipFacing + NUM_SHIP_FACINGS);
			else
				ShipPtr->next.image.frame = SetAbsFrameIndex (ShipPtr->next.image.frame, StarShipPtr->ShipFacing);
		}
		else if (EPtr->mass_points >= 7)
		{
			shake[ShipPtr->playerNr] = ((shake[ShipPtr->playerNr] + (BYTE)TFB_Random()) % 4) - 1;
			if (shake[ShipPtr->playerNr] > 1)
				shake[ShipPtr->playerNr] = 0;
			
			// Alternate between normal and blurred graphics.
			if ((GetFrameIndex (ShipPtr->current.image.frame)) < NUM_SHIP_FACINGS && EPtr->life_span > 1)
				ShipPtr->next.image.frame = SetAbsFrameIndex (ShipPtr->next.image.frame, 
					(StarShipPtr->ShipFacing + shake[ShipPtr->playerNr]) + NUM_SHIP_FACINGS);
			else
				ShipPtr->next.image.frame = SetAbsFrameIndex (ShipPtr->next.image.frame, StarShipPtr->ShipFacing);
		}
		
		// If the player charges the primary too much, the whole ship explodes!!!
		if (EPtr->mass_points >= 10)
		{
			EPtr->mass_points = 7;
			
			if (!DeltaCrew (ShipPtr, -MAX_CREW))
				ShipPtr->life_span = 0;
			ShipPtr->state_flags |= CHANGING;
		}
		
		UnlockElement (StarShipPtr->hShip);
			
		// Put the focusball to the queue of elements.
		UnlockElement (hFocusBall);
		PutElement (hFocusBall);
		
		SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],  NO_PRIM);
		ElementPtr->state_flags |= NONSOLID;
	}
}

// This is the collision function for both the secondary saber.
static void
saber_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	HELEMENT hBlastElement;
	STARSHIP *StarShipPtr;	

	GetElementStarShip (ElementPtr0, &StarShipPtr);
	StarShipPtr->weapon_counter = 0;
	
	hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
	if (hBlastElement)
	{
		RemoveElement (hBlastElement);
		FreeElement (hBlastElement);
		
		ElementPtr0->state_flags &= ~DISAPPEARING;
	}
}

// This is used by AI for testing would the primary weapon hit if shot.
static COUNT
initialize_test_burst (ELEMENT *ElementPtr, HELEMENT BurstArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = (StarShipPtr->ShipFacing) * NUM_BURST_FRAMES;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags =  IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed = MISSILE_SPEED_HD;
	MissileBlock.hit_points = ElementPtr->mass_points;
	MissileBlock.damage = ElementPtr->mass_points;
	MissileBlock.life = NUM_BURST_FRAMES;
	MissileBlock.preprocess_func = animate_burst;
	MissileBlock.blast_offs = RES_SCALE (50);
	BurstArray[0] = initialize_missile (&MissileBlock);
	
	return (1);
}

// This is used by AI for testing would it hit the enemy ship with dervish mode.
static COUNT
initialize_test_saber (ELEMENT *ElementPtr, HELEMENT SaberArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = (StarShipPtr->ShipFacing) * NUM_BURST_FRAMES;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags =  IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed = DERVISH_THRUST;
	MissileBlock.hit_points = ElementPtr->mass_points;
	MissileBlock.damage = ElementPtr->mass_points;
	MissileBlock.life = 10;
	MissileBlock.preprocess_func = animate_burst;
	MissileBlock.blast_offs = RES_SCALE (25);
	SaberArray[0] = initialize_missile (&MissileBlock);
	
	return (1);
}

// This generates a focus ball.
static COUNT
initialize_focusball (ELEMENT *ShipPtr, HELEMENT FocusArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = FOCUSBALL_FRAME_STARTINDEX;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags =  NONSOLID | IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (FOCUSBALL_OFFSET);
	MissileBlock.hit_points = 1;
	MissileBlock.damage = 0;
	MissileBlock.life = 2;
	MissileBlock.preprocess_func = focusball_preprocess;
	MissileBlock.blast_offs = 0;
	FocusArray[0] = initialize_missile (&MissileBlock);
	
	if (FocusArray[0])
	{
		ELEMENT *FocusPtr;
		
		LockElement (FocusArray[0], &FocusPtr);
		FocusPtr->postprocess_func = focusball_postprocess;
		FocusPtr->thrust_wait = 255; // Let's ensure the secondary sabre's focusball doesn't grow it's damage amount.
		UnlockElement (FocusArray[0]);
	}
	
	return (1);
}

// Primary weapon: This generates a focusball which turns into beam burst when the weapon button is released.
static COUNT
initialize_focusball_which_bursts (ELEMENT *ShipPtr, HELEMENT BurstArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = FOCUSBALL_FRAME_STARTINDEX;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags =  NONSOLID | IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed = DISPLAY_TO_WORLD (FOCUSBALL_OFFSET);
	MissileBlock.hit_points = 100;
	MissileBlock.damage = 1;
	MissileBlock.life = 2;
	MissileBlock.preprocess_func = focusball_preprocess;
	MissileBlock.blast_offs = 0;
	
	if (!(StarShipPtr->old_status_flags & WEAPON))
	{
		BurstArray[0] = initialize_missile (&MissileBlock);
	
		if (BurstArray[0])
		{
			ELEMENT *FocusPtr;
		
			LockElement (BurstArray[0], &FocusPtr);
			FocusPtr->postprocess_func = focusball_postprocess;
			FocusPtr->thrust_wait = BURST_CHARGE_TIME;
			UnlockElement (BurstArray[0]);
		}
	
		return (1);
	}
	else
	{
		DeltaEnergy (ShipPtr, 0);
		return (0);
	}
}

// Secondary weapon: This generates the wide saber.
static COUNT
initialize_saber (ELEMENT *ShipPtr, HELEMENT SaberArray[], COUNT facingfix)
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	COUNT facing; 
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	facing = (StarShipPtr->ShipFacing + NUM_SABERS - 1 - facingfix) % NUM_SHIP_FACINGS;
	
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray  = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.index   = MissileBlock.face = facing;
	MissileBlock.sender  = ShipPtr->playerNr;
	MissileBlock.flags   = IGNORE_SIMILAR;
	MissileBlock.pixoffs = FOCUSBALL_OFFSET;
	MissileBlock.speed   = DERVISH_THRUST;
	MissileBlock.damage  = 1;
	MissileBlock.life    = 1;
	MissileBlock.hit_points = 100;
	MissileBlock.preprocess_func = 0;
	MissileBlock.blast_offs      = 0;
			
	if ((SaberArray[0] = initialize_missile (&MissileBlock)))
	{
		ELEMENT *SaberPtr;
		
		LockElement (SaberArray[0], &SaberPtr);
		SetElementStarShip (SaberPtr, StarShipPtr);
		SaberPtr->collision_func = saber_collision;
		InitIntersectStartPoint (SaberPtr);
		InitIntersectEndPoint (SaberPtr);
		SaberPtr->IntersectControl.IntersectStamp.frame = StarShipPtr->RaceDescPtr->ship_data.special[facing];
		UnlockElement (SaberArray[0]);
	}
	
	return (1);
}

static void
foonfoon_postprocess (ELEMENT *ElementPtr)
{
	RACE_DESC *RDPtr;
	STARSHIP *StarShipPtr;
	STATUS_FLAGS cur_status_flags;
	BYTE frame_index, i;
	static BYTE not_had_pause[NUM_SIDES]	= {0};
	static BYTE focusball_exists[NUM_SIDES]	= {0};
	
	// For keeping the VUX limpet effects after spinning blade.
	static BYTE basic_max_thrust[2 * NUM_SIDES] = { MAX_THRUST, MAX_THRUST, CONST_MAX_THRUST_HD, CONST_MAX_THRUST_HD };
	static BYTE basic_thrust_wait[NUM_SIDES]= {THRUST_WAIT, THRUST_WAIT};
	static BYTE basic_turn_wait[NUM_SIDES]	= {TURN_WAIT, TURN_WAIT};
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	RDPtr = StarShipPtr->RaceDescPtr;
	
	cur_status_flags = StarShipPtr->cur_status_flags;
	
	frame_index = GetFrameIndex (ElementPtr->current.image.frame);
	
	// Store the original turning rate & max speed if not in dervish mode, or the slow state after dervish.
	if (StarShipPtr->special_counter == 0 
		&& RDPtr->characteristics.turn_wait > 0
		&& (frame_index < NUM_SHIP_FACINGS && !(StarShipPtr->cur_status_flags & WEAPON)) // Weapon check because the primary weapon also can set the ship graphics
		&& !(StarShipPtr->RaceDescPtr->ship_info.damage_flags & DAMAGE_THRUST))
	{
		basic_max_thrust[2 * NUM_SIDES]  = RDPtr->characteristics.max_thrust;
		basic_thrust_wait[ElementPtr->playerNr] = RDPtr->characteristics.thrust_wait;
		basic_turn_wait[ElementPtr->playerNr]	= RDPtr->characteristics.turn_wait;
	}
	
	// After using special: Thrust is sloooow for a while.
	if (StarShipPtr->special_counter > 0)
	{
		// Slooooow.
		if ((MAX_THRUST_HD) / 4 < basic_max_thrust[2 * RESOLUTION_FACTOR + ElementPtr->playerNr])
			RDPtr->characteristics.max_thrust = (MAX_THRUST_HD) / 4;
		else
			RDPtr->characteristics.max_thrust = basic_max_thrust[2 * RESOLUTION_FACTOR + ElementPtr->playerNr];
		
		if (basic_thrust_wait[ElementPtr->playerNr] > 3)
			RDPtr->characteristics.thrust_wait	= basic_thrust_wait[ElementPtr->playerNr];
		else
			RDPtr->characteristics.thrust_wait	= 3;

		// Blue ion trail tells the player when dervish mode is unusable.
		StarShipPtr->RaceDescPtr->ship_info.damage_flags |= DAMAGE_THRUST;
		
		// Turning just returns to normal.
		RDPtr->characteristics.turn_wait = basic_turn_wait[ElementPtr->playerNr];
		
		// Battery charges normally when not in dervish mode.
		RDPtr->characteristics.energy_regeneration = ENERGY_REGENERATION;
		ElementPtr->state_flags |= CHANGING;
	}
	
	// Special: The dervish mode.
	else if (StarShipPtr->cur_status_flags & SPECIAL
			 && RDPtr->ship_info.energy_level > 0
			 && frame_index >= NUM_SHIP_FACINGS
			 && !not_had_pause[ElementPtr->playerNr])
	{
		// Add sabre elements.
		// The more sabres there are, the wider it is (and the more damage it deals).
		for (i = 0; i < NUM_SABERS; i++)
		{
			HELEMENT Saber;
			
			initialize_saber (ElementPtr, &Saber, i);
			if (Saber)
			{
				ELEMENT *SMissilePtr;
				LockElement (Saber, &SMissilePtr);
			
				UnlockElement (Saber);
				PutElement (Saber);
			}
		}
		
		// Add a nice little focusball.
		if (!focusball_exists[ElementPtr->playerNr])
		{
			HELEMENT Focusball;
			
			initialize_focusball (ElementPtr, &Focusball);
			if (Focusball)
			{
				ELEMENT *FBMissilePtr;
				LockElement (Focusball, &FBMissilePtr);
				
				SetElementStarShip (FBMissilePtr, StarShipPtr);
				FBMissilePtr->mass_points = 0;
				
				UnlockElement (Focusball);
				PutElement (Focusball);
			}
			
			// Don't create multiple focusballs. This makes 
			// a) the game run better, b) focusball animation visible. 
			focusball_exists[ElementPtr->playerNr] = 1;
		}
		
		// Turning rate & max speed increase.
		RDPtr->characteristics.thrust_wait = 0;
		RDPtr->characteristics.turn_wait   = 0;
		RDPtr->characteristics.max_thrust  = DERVISH_THRUST;
		StarShipPtr->RaceDescPtr->ship_info.damage_flags &= ~(DAMAGE_THRUST);
		
		// Battery slowly drops.
		RDPtr->characteristics.energy_regeneration = (BYTE)DERVISH_DEGENERATION;
		ElementPtr->state_flags |= CHANGING;
	}
	// Not using special.
	else
	{
		// When dervish ends...
		if ((StarShipPtr->old_status_flags & SPECIAL || RDPtr->ship_info.energy_level == 0)
			&& frame_index >= NUM_SHIP_FACINGS)
		{
			COUNT i;
			
			// Set a cooldown period before next dervish.
			StarShipPtr->special_counter = DERVISH_COOLDOWN_TIME;
			
			// Using dervish always drains the battery totally.
			DeltaEnergy (ElementPtr, -StarShipPtr->RaceDescPtr->ship_info.energy_level);
			
			// Slow down.
			SetVelocityVector (&ElementPtr->velocity, (MAX_THRUST_HD / 4), NORMALIZE_FACING (StarShipPtr->ShipFacing));
			
			// We're not past max speed any more.
			cur_status_flags &= ~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
			
			// Return to normal graphics.
			if ((GetFrameIndex (ElementPtr->current.image.frame)) >= NUM_SHIP_FACINGS)
				ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->next.image.frame, StarShipPtr->ShipFacing);
			
			// End the spinning blade sound.
			for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
			{
				ELEMENT *posobj;
				if (!ChannelPlaying(i))
					continue;
				
				posobj = GetPositionalObject (i);
				if (posobj == ElementPtr)
					StopSource (i);
			}
			
			// Play the ending sound.
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
			
			// The focusball dies with the dervish mode.
			focusball_exists[ElementPtr->playerNr] = 0;
			
			// Prevent spamming the special key.
			cur_status_flags &= ~(SPECIAL);
			StarShipPtr->cur_status_flags = cur_status_flags;
			not_had_pause[ElementPtr->playerNr] = 1;
		}
		
		// Turning rate & max speed are normal when not in dervish mode.
		RDPtr->characteristics.max_thrust	= basic_max_thrust[(IS_HD ? NUM_SIDES : 0) + ElementPtr->playerNr];
		RDPtr->characteristics.thrust_wait	= basic_thrust_wait[ElementPtr->playerNr];
		RDPtr->characteristics.turn_wait	= basic_turn_wait[ElementPtr->playerNr];
		StarShipPtr->RaceDescPtr->ship_info.damage_flags &= ~(DAMAGE_THRUST);
		
		// Battery charges normally when not in dervish mode.
		RDPtr->characteristics.energy_regeneration = ENERGY_REGENERATION;
		ElementPtr->state_flags |= CHANGING;
	}
	
	// The player must let go of the special key between dervishes.
	// This requirement eliminates all sorts of bugs.
	if (!(StarShipPtr->old_status_flags & SPECIAL) 
		&& !(StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level > SPECIAL_ENERGY_COST)
		not_had_pause[ElementPtr->playerNr] = 0;
}

static void
foonfoon_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	STATUS_FLAGS cur_status_flags;
	BYTE frame_index;
	static BYTE not_had_pause[NUM_SIDES] = {0};
	static BYTE turn_direction[NUM_SIDES] = {0};
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	cur_status_flags = StarShipPtr->cur_status_flags;
	
	frame_index = GetFrameIndex (ElementPtr->current.image.frame);
	
	if (StarShipPtr->cur_status_flags & SPECIAL
		&& !not_had_pause[ElementPtr->playerNr])
	{
		// Can't use primary weapon and special simultaneously.
		cur_status_flags &= ~(WEAPON);
		
		// Not enough juice for dervish mode.
		if ((StarShipPtr->RaceDescPtr->ship_info.energy_level < SPECIAL_ENERGY_COST && !(StarShipPtr->old_status_flags & SPECIAL))
			|| StarShipPtr->RaceDescPtr->ship_info.energy_level == 0)
		{
			// Flash the 'batt' text.
			DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);
			
			// Prevent spamming the special key.
			cur_status_flags &= ~(SPECIAL);
			StarShipPtr->old_status_flags &= ~(SPECIAL);
			not_had_pause[ElementPtr->playerNr] = 1;
			turn_direction[ElementPtr->playerNr] = 0;
		}
		else if (!not_had_pause[ElementPtr->playerNr])
		{
			COUNT facing;
			SIZE  speedx, speedy;
			SDWORD totalspeed;
			
			// Find out the ship facing and speed.
			facing = StarShipPtr->ShipFacing;
			GetCurrentVelocityComponents (&ElementPtr->velocity, &speedx, &speedy);
			totalspeed = sqrt (speedx * speedx + speedy * speedy);

			// Upon pressing the special key, speed rapidly to the direction which the ship was heading to.
			if (frame_index < NUM_SHIP_FACINGS
				&& StarShipPtr->RaceDescPtr->ship_info.energy_level >= SPECIAL_ENERGY_COST
				&& StarShipPtr->cur_status_flags & (LEFT | RIGHT)
				)//&& totalspeed < DERVISH_THRUST)
			{
				// Decrement battery.
				DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST);
				
				// Go FAAAAST!
				SetVelocityVector (&ElementPtr->velocity, DERVISH_THRUST, NORMALIZE_FACING (facing));
				
				// Dervish swoosh sound.
				ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
				
				// Spinning blade sound.
				ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);
				
				if (StarShipPtr->cur_status_flags & LEFT)
					turn_direction[ElementPtr->playerNr] = 1;
				else if (StarShipPtr->cur_status_flags & RIGHT)
					turn_direction[ElementPtr->playerNr] = 2;
			}
			

			// Can't turn or use primary weapon in dervish mode.
			cur_status_flags &= ~(THRUST | WEAPON | LEFT | RIGHT);
			
			// Turn ship around whilst in dervish mode.
			if (turn_direction[ElementPtr->playerNr] == 1)
				facing -= 2;
			else if (turn_direction[ElementPtr->playerNr] == 2)
				facing += 2;
			facing %= NUM_SHIP_FACINGS;
			StarShipPtr->ShipFacing = facing;
			
			// Change to blur graphics.
			if (turn_direction[ElementPtr->playerNr])
			{
				ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, facing + NUM_SHIP_FACINGS);
				cur_status_flags |= SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED;
			}
		}
	}
	// The player must let go of the special key between dervishes.
	// This requirement eliminates all sorts of bugs.
	else if (!(StarShipPtr->old_status_flags & SPECIAL) 
		&& !(StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->RaceDescPtr->ship_info.energy_level > SPECIAL_ENERGY_COST)
	{
		not_had_pause[ElementPtr->playerNr] = 0;
		turn_direction[ElementPtr->playerNr] = 0;
	}
	
	StarShipPtr->cur_status_flags = cur_status_flags;
}

RACE_DESC*
init_foonfoon (void)
{
	RACE_DESC *RaceDescPtr;
	
	if (IS_HD)
	{
		foonfoon_desc.characteristics.max_thrust = MAX_THRUST_HD;
		foonfoon_desc.characteristics.thrust_increment = THRUST_INCREMENT_HD;
		foonfoon_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}
	else
	{
		foonfoon_desc.characteristics.max_thrust = MAX_THRUST;
		foonfoon_desc.characteristics.thrust_increment = THRUST_INCREMENT;
		foonfoon_desc.cyborg_control.WeaponRange = MISSILE_RANGE;
	}

	foonfoon_desc.postprocess_func = foonfoon_postprocess;
	foonfoon_desc.preprocess_func  = foonfoon_preprocess;
	foonfoon_desc.init_weapon_func = initialize_focusball_which_bursts;
	foonfoon_desc.cyborg_control.intelligence_func = foonfoon_intelligence;
	RaceDescPtr = &foonfoon_desc;
	
	return (RaceDescPtr);
}
