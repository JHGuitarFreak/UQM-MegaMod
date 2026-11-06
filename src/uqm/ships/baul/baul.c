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
#include "baul.h"
#include "resinst.h"

#include "uqm/globdata.h"
#include "libs/log.h"


#define MAX_CREW 28
#define MAX_ENERGY 22
#define ENERGY_REGENERATION 3
#define WEAPON_ENERGY_COST 0
#define SPECIAL_ENERGY_COST 2
#define ENERGY_WAIT 24
#define MAX_THRUST 32
#define THRUST_INCREMENT 6
#define TURN_WAIT 2
#define THRUST_WAIT 5
#define WEAPON_WAIT 18
#define SPECIAL_WAIT 7

#define SHIP_MASS 9
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 10
#define MISSILE_RANGE (MISSILE_SPEED * MISSILE_LIFE)

// Weapon graphics
#define LAST_SPRAY_INDEX 5
#define SHOCKWAVE_FRAMES 8
#define LAST_GAS_INDEX 8
#define LAST_SHOCKWAVE_INDEX (LAST_GAS_INDEX + SHOCKWAVE_FRAMES)
#define NUM_DISSOLVE_FRAMES 4
#define LAST_DISSOLVE_INDEX (LAST_SHOCKWAVE_INDEX + NUM_DISSOLVE_FRAMES)
#define NUM_EMERGE_FRAMES 3

// Weapon attributes
#define SHOCKWAVE_RANGE RES_SCALE (150)
#define MAX_DESTRUCTION (RES_DESCALE (SHOCKWAVE_RANGE) / 25)

#define GAS_HITS 100
#define GAS_DAMAGE 0
#define GAS_LIFE 480 // How many 1/24 secs the gas lives.
#define GAS_OFFSET RES_SCALE (4)
#define GAS_INIT_SPEED RES_SCALE (100) // Baul's gas now flies forward.
#define GAS_HORZ_OFFSET (DISPLAY_TO_WORLD (RES_SCALE (5)))
#define GAS_BATCH_SIZE 1

#define SPRAY_HORZ_OFFSET (DISPLAY_TO_WORLD (RES_SCALE (-5)))
#define MISSILE_HITS 2
#define MISSILE_DAMAGE 1 // Must be at least 1 to make the weapon hit gas clouds.
#define MISSILE_OFFSET RES_SCALE (3)
#define NUM_SPRAYS 5
#define SPRAY_DIST 4

// HD Values
#define MISSILE_SPEED_HD RES_SCALE (MISSILE_SPEED)
#define MISSILE_RANGE_HD (MISSILE_SPEED_HD * MISSILE_LIFE)

#include "../orz/orz.h"
#define ORZ_MARINE(ptr) (ptr->preprocess_func == intruder_preprocess && ptr->collision_func == marine_collision)
#define IS_GAS(ptr) (ptr->preprocess_func == gas_preprocess && ptr->collision_func == gas_collision && ptr->life_span > 1)

static RACE_DESC baul_desc =
{
	{ /* SHIP_INFO */
		"punisher",
		FIRES_FORE,
		21, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		BAUL_RACE_STRINGS,
		BAUL_ICON_MASK_PMAP_ANIM,
		BAUL_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		0, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			0,0,
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
			BAUL_BIG_MASK_PMAP_ANIM,
			BAUL_MED_MASK_PMAP_ANIM,
			BAUL_SML_MASK_PMAP_ANIM,
		},
		{
			BAULSPRAY_BIG_MASK_PMAP_ANIM,
			BAULSPRAY_MED_MASK_PMAP_ANIM,
			BAULSPRAY_SML_MASK_PMAP_ANIM,
		},
		{
			BAULGAS_BIG_MASK_PMAP_ANIM,
			BAULGAS_MED_MASK_PMAP_ANIM,
			BAULGAS_SML_MASK_PMAP_ANIM,
		},
		{
			BAUL_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL,
			0, 0, 0, 0, 0
		},
		BAUL_VICTORY_SONG,
		BAUL_SHIP_SOUNDS,
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

// Forward declarations
static void
gas_preprocess (ELEMENT *ElementPtr);

static void
gas_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1);

static void
gas_death (ELEMENT *ElementPtr);

static COUNT
initialize_spray (ELEMENT *ShipPtr, HELEMENT SprayArray[]);

// This is used by AI for testing would it hit the enemy ship with gas.
static COUNT
initialize_test_gas (ELEMENT *ElementPtr, HELEMENT GasArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;
	MissileBlock.index = LAST_DISSOLVE_INDEX;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags =  GASSY_SUBSTANCE | IGNORE_VELOCITY;
	MissileBlock.pixoffs = GAS_OFFSET;
	MissileBlock.speed = GAS_INIT_SPEED;
	MissileBlock.hit_points = GAS_HITS;
	MissileBlock.damage = GAS_DAMAGE;
	MissileBlock.life = 20; // Not GAS_LIFE because this test gas doesn't slow down at all -> life has to be short.
	MissileBlock.preprocess_func = gas_preprocess;
	MissileBlock.blast_offs = 0;
	GasArray[0] = initialize_missile (&MissileBlock);
	
	return (1);
}

static void
baul_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	BYTE old_count;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;
	BYTE in_gas_cloud = 0;
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	old_count = StarShipPtr->weapon_counter;
	
	// Don't spawn gas unless specifically told to.
	StarShipPtr->ship_input_state &= ~SPECIAL;
	
	// See if there is gas sticking to OUR ship.
	lpEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
	if (lpEvalDesc->ObjectPtr)
	{
		if (lpEvalDesc->ObjectPtr->state_flags & GASSY_SUBSTANCE 
				&& lpEvalDesc->ObjectPtr->mass_points == 0
				&& lpEvalDesc->which_turn <= 1)
			in_gas_cloud = 1;
	}
	
	// Now that we're done with the gas examinations, examine enemy ship.
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	
	// Actions towards enemy ship: 
	if (lpEvalDesc->ObjectPtr)
	{
#define STATIONARY_SPEED WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE (4)))
		SIZE dx, dy;
		
		GetCurrentVelocityComponents (&lpEvalDesc->ObjectPtr->velocity, &dx, &dy);
		
		// Chase the ship if it's within a reasonable distance and there's no gas sticking to OUR ship.
		if ((lpEvalDesc->which_turn < 20
			|| (long)dx * dx + (long)dy * dy <= (long)STATIONARY_SPEED * STATIONARY_SPEED)
			&& !in_gas_cloud)
			lpEvalDesc->MoveState = PURSUE;
		// Otherwise, entice.
		else
			lpEvalDesc->MoveState = ENTICE;
	}
	
	// Normal ship intelligence.
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	
	// We don't want the Baul shooting all the time so it doesn't kill itself so easily.
	if (lpEvalDesc->ObjectPtr)
	{	
		STARSHIP *EnemyStarShipPtr;
		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		
		// If the enemy is very close, don't shoot him with the primary to avoid damage to self.
		// We shoot however, if the enemy ship has lazer or tries to shoot us.
		// The philosophy: Better to kill both than do nothing and die.
		if (StarShipPtr->ship_input_state & WEAPON && lpEvalDesc->which_turn < 8
			&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags & IMMEDIATE_WEAPON)
			&& !(EnemyStarShipPtr->ship_input_state & WEAPON)
			&& !(EnemyStarShipPtr->ship_input_state & SPECIAL) )
			StarShipPtr->ship_input_state &= ~WEAPON;
	}
	
	// Drop gas whenever the battery tops off and when we are far away from the enemy.
	if ((StarShipPtr->RaceDescPtr->ship_info.energy_level 
		 == StarShipPtr->RaceDescPtr->ship_info.max_energy)
		&& lpEvalDesc->which_turn > 12
		&& !in_gas_cloud)
		StarShipPtr->ship_input_state |= SPECIAL;
	
	// Consider dropping gas also when the enemy is in our sights.
	if (StarShipPtr->special_counter == 0)
	{
		BYTE old_input_state;
		old_input_state = StarShipPtr->ship_input_state;
		
		// The final decision of "to gas or not to gas" is made by evaluating a test weapon function.
		StarShipPtr->RaceDescPtr->init_weapon_func = initialize_test_gas;
		ship_intelligence (ShipPtr, ObjectsOfConcern, ENEMY_SHIP_INDEX + 1);
		
		// Since we faked using primary weapon even though we really are gonna use special,
		// change the WEAPON button press to SPECIAL.
		if (StarShipPtr->ship_input_state & WEAPON)
		{
			StarShipPtr->ship_input_state &= ~WEAPON;
			StarShipPtr->ship_input_state |= SPECIAL;
		}
		
		StarShipPtr->ship_input_state = (unsigned char)(old_input_state | (StarShipPtr->ship_input_state & SPECIAL));
	}
	
	// Return the original stats.
	StarShipPtr->weapon_counter = old_count;
	StarShipPtr->RaceDescPtr->init_weapon_func = initialize_spray;
}

static void
shockwave_preprocess (ELEMENT *ElementPtr)
{
	ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
	
	// This makes the shockwave animate even if the ships are not moving and the screen is stationary.
	ElementPtr->state_flags |= CHANGING;
}

// We cannot use the normal generate_shockwave as death_func since it has two input parameters.
// To circumvent this, we define an otherwise similar function generate_shockwave_2 here, but
// which only has 1 input parameter.
// This function is required for daisy-chaining shockwave explosions.
static void
generate_shockwave_2 (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	BYTE which_player;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	which_player = -1;//ElementPtr->playerNr; XXX
	
	// Gas is still 'solid' when it's hit by the spray. Let's make a shockwave and kill the gas cloud. 
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		HELEMENT hShockwave;
		
		hShockwave = AllocElement ();
		if (hShockwave)
		{
			ELEMENT *ShockwavePtr;
			STARSHIP *StarShipPtr;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			
			PutElement (hShockwave);
			LockElement (hShockwave, &ShockwavePtr);
			SetElementStarShip (ShockwavePtr, StarShipPtr);
			ShockwavePtr->hit_points = ShockwavePtr->mass_points = 0;
			ShockwavePtr->playerNr = which_player; // Can damage both ships. //XXX Don't damage self.
			ShockwavePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID | IGNORE_SIMILAR;
			ShockwavePtr->life_span = SHOCKWAVE_FRAMES;
			SetPrimType (&(GLOBAL (DisplayArray))[ShockwavePtr->PrimIndex], STAMP_PRIM);
			ShockwavePtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
			ShockwavePtr->current.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0], LAST_GAS_INDEX);
			ShockwavePtr->next.image.frame = SetAbsFrameIndex(ElementPtr->current.image.frame, LAST_GAS_INDEX);
			ShockwavePtr->current.location = ElementPtr->current.location;
			ShockwavePtr->preprocess_func = shockwave_preprocess;
			ShockwavePtr->postprocess_func = NULL;
			ShockwavePtr->death_func = NULL;
			ZeroVelocityComponents (&ShockwavePtr->velocity);
			UnlockElement (hShockwave);
		}
		
		// Gas dies on the next turn.
		ElementPtr->state_flags |= NONSOLID;
		
		// Explosion sounds.
		ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);
		ProcessSound (SetAbsSoundIndex (GameSounds, TARGET_DAMAGED_FOR_6_PLUS_PT), ElementPtr);
	}
	
	{
		// This is called during PostProcessQueue(), close to or at the end,
		// for the temporary shockwave element to apply the damage.
		// The effects are not seen until the next frame.
		HELEMENT hElement, hNextElement;
		
		for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
		{
			ELEMENT *ObjPtr;
			
			LockElement (hElement, &ObjPtr);
			hNextElement = GetSuccElement (ObjPtr);
			
			if (IS_GAS (ObjPtr))
			{
				SIZE delta_x, delta_y;
				DWORD dist;
				
				if ((delta_x = ObjPtr->next.location.x - ElementPtr->next.location.x) < 0) delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y - ElementPtr->next.location.y) < 0) delta_y = -delta_y;
				
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				
				if (delta_x <= SHOCKWAVE_RANGE && delta_y <= SHOCKWAVE_RANGE
					&& (dist = (DWORD)(delta_x * delta_x) + (DWORD)(delta_y * delta_y)) <= (DWORD)(SHOCKWAVE_RANGE * SHOCKWAVE_RANGE))
				{
					SIZE destruction;
					
					destruction = ((MAX_DESTRUCTION * (SHOCKWAVE_RANGE - square_root (dist))) / SHOCKWAVE_RANGE) + 1;
					
					// The shockwave is delayed according to how far it is from the shockwave that set it off.
					ObjPtr->life_span = (10 / destruction);
					ObjPtr->death_func = generate_shockwave_2;
					ObjPtr->playerNr = which_player;
				}
			}
			else if (CollidingElement (ObjPtr) || ORZ_MARINE (ObjPtr))
			{
				SIZE delta_x, delta_y;
				DWORD dist;
				
				if ((delta_x = ObjPtr->next.location.x - ElementPtr->next.location.x) < 0) delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y - ElementPtr->next.location.y) < 0) delta_y = -delta_y;
				
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				
				if (delta_x <= SHOCKWAVE_RANGE && delta_y <= SHOCKWAVE_RANGE
					&& (dist = (DWORD)(delta_x * delta_x) + (DWORD)(delta_y * delta_y)) <= (DWORD)(SHOCKWAVE_RANGE * SHOCKWAVE_RANGE))
				{
					SIZE destruction;
					
					destruction = ((MAX_DESTRUCTION * (SHOCKWAVE_RANGE - square_root (dist))) / SHOCKWAVE_RANGE) + 1;
					
					if (ObjPtr->state_flags & PLAYER_SHIP && ObjPtr->playerNr != which_player)
					{
						STARSHIP *EnemyShipPtr;
						
						GetElementStarShip (ObjPtr, &EnemyShipPtr);
						
						// Deal damage to ships except shield-using Yehat & Utwig.
						if (!((EnemyShipPtr->SpeciesID == YEHAT_ID || EnemyShipPtr->SpeciesID == UTWIG_ID) 
							  && ObjPtr->life_span > NORMAL_LIFE))
						{
							if (!DeltaCrew (ObjPtr, -destruction))
								ObjPtr->life_span = 0;
						}
						// Charge Utwig shield.
						else if (EnemyShipPtr->SpeciesID == UTWIG_ID && ObjPtr->life_span > NORMAL_LIFE)
							ObjPtr->life_span += destruction;
					}
					else if (!GRAVITY_MASS (ObjPtr->mass_points) && ObjPtr->playerNr != which_player)
					{
						if ((BYTE)destruction < ObjPtr->hit_points)
							ObjPtr->hit_points -= (BYTE)destruction;
						else
						{
							ObjPtr->hit_points = 0;
							ObjPtr->life_span = 0;
						}
					}
				}
			}
			
			UnlockElement (hElement);
		}
	}
}

// When hit by Baul spray, gas clouds explodes transforming into a lethal shockwave.
static void
generate_shockwave (ELEMENT *ElementPtr, BYTE which_player)
{
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	// Gas is still 'solid' when it's hit by the spray. Let's make a shockwave and kill the gas cloud. 
	if (!(ElementPtr->state_flags & NONSOLID))
	{
		HELEMENT hShockwave;
				
		hShockwave = AllocElement ();
		if (hShockwave)
		{
			ELEMENT *ShockwavePtr;
			STARSHIP *StarShipPtr;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			
			PutElement (hShockwave);
			LockElement (hShockwave, &ShockwavePtr);
			SetElementStarShip (ShockwavePtr, StarShipPtr);
			ShockwavePtr->hit_points = ShockwavePtr->mass_points = 0;
			ShockwavePtr->playerNr = which_player;
			ShockwavePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID | IGNORE_SIMILAR;
			ShockwavePtr->life_span = SHOCKWAVE_FRAMES;
			SetPrimType (&(GLOBAL (DisplayArray))[ShockwavePtr->PrimIndex], STAMP_PRIM);
			ShockwavePtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.special;
			ShockwavePtr->current.image.frame = SetAbsFrameIndex(StarShipPtr->RaceDescPtr->ship_data.special[0], LAST_GAS_INDEX);
			ShockwavePtr->next.image.frame = SetAbsFrameIndex(ElementPtr->current.image.frame, LAST_GAS_INDEX);
			ShockwavePtr->current.location = ElementPtr->current.location;
			ShockwavePtr->preprocess_func = shockwave_preprocess;
			ShockwavePtr->postprocess_func = NULL;
			ShockwavePtr->death_func = NULL;
			ZeroVelocityComponents (&ShockwavePtr->velocity);
			UnlockElement (hShockwave);
		}
		
		// Gas dies on the next turn.
		ElementPtr->state_flags |= NONSOLID;
		
		// Explosion sounds.
		ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr);
		ProcessSound (SetAbsSoundIndex (GameSounds, TARGET_DAMAGED_FOR_6_PLUS_PT), ElementPtr);
	}

	{
		// This is called during PostProcessQueue(), close to or at the end,
		// for the temporary shockwave element to apply the damage.
		// The effects are not seen until the next frame.
		HELEMENT hElement, hNextElement;
		
		for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
		{
			ELEMENT *ObjPtr;
			
			LockElement (hElement, &ObjPtr);
			hNextElement = GetSuccElement (ObjPtr);
			
			if (IS_GAS (ObjPtr))
			{
				SIZE delta_x, delta_y;
				DWORD dist;
				
				if ((delta_x = ObjPtr->next.location.x - ElementPtr->next.location.x) < 0) delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y - ElementPtr->next.location.y) < 0) delta_y = -delta_y;
				
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				
				if (delta_x <= SHOCKWAVE_RANGE && delta_y <= SHOCKWAVE_RANGE
					&& (dist = (DWORD)(delta_x * delta_x) + (DWORD)(delta_y * delta_y)) <= (DWORD)(SHOCKWAVE_RANGE * SHOCKWAVE_RANGE))
				{
					SIZE destruction;
					
					destruction = ((MAX_DESTRUCTION * (SHOCKWAVE_RANGE - square_root (dist))) / SHOCKWAVE_RANGE) + 1;
					
					// The shockwave is delayed according to how far it is from the shockwave that set it off.
					ObjPtr->life_span = (10 / destruction);
					ObjPtr->death_func = generate_shockwave_2;
					ObjPtr->playerNr = which_player;
				}
			}
			else if (CollidingElement (ObjPtr) || ORZ_MARINE (ObjPtr))
			{
				SIZE delta_x, delta_y;
				DWORD dist;
				
				if ((delta_x = ObjPtr->next.location.x - ElementPtr->next.location.x) < 0) delta_x = -delta_x;
				if ((delta_y = ObjPtr->next.location.y - ElementPtr->next.location.y) < 0) delta_y = -delta_y;
				
				delta_x = WORLD_TO_DISPLAY (delta_x);
				delta_y = WORLD_TO_DISPLAY (delta_y);
				
				if (delta_x <= SHOCKWAVE_RANGE && delta_y <= SHOCKWAVE_RANGE
					&& (dist = (DWORD)(delta_x * delta_x) + (DWORD)(delta_y * delta_y)) <= (DWORD)(SHOCKWAVE_RANGE * SHOCKWAVE_RANGE))
				{
					SIZE destruction;
					
					destruction = ((MAX_DESTRUCTION * (SHOCKWAVE_RANGE - square_root (dist))) / SHOCKWAVE_RANGE) + 1;
					
					if (ObjPtr->state_flags & PLAYER_SHIP && ObjPtr->playerNr != which_player)
					{
						STARSHIP *EnemyShipPtr;
						
						GetElementStarShip (ObjPtr, &EnemyShipPtr);
						
						// Deal damage to ships except shield-using Yehat & Utwig.
						if (!((EnemyShipPtr->SpeciesID == YEHAT_ID || EnemyShipPtr->SpeciesID == UTWIG_ID) 
							  && ObjPtr->life_span > NORMAL_LIFE))
						{
							if (!DeltaCrew (ObjPtr, -destruction))
								ObjPtr->life_span = 0;
						}
						// Charge Utwig shield.
						else if (EnemyShipPtr->SpeciesID == UTWIG_ID && ObjPtr->life_span > NORMAL_LIFE)
							ObjPtr->life_span += destruction;
					}
					else if (!GRAVITY_MASS (ObjPtr->mass_points) && ObjPtr->playerNr != which_player)
					{
						if ((BYTE)destruction < ObjPtr->hit_points)
							ObjPtr->hit_points -= (BYTE)destruction;
						else
						{
							ObjPtr->hit_points = 0;
							ObjPtr->life_span = 0;
						}
					}
				}
			}
			
			UnlockElement (hElement);
		}
	}
}

// This forwards the gas dissolving animation.
static void
gas_death_animation (ELEMENT *ElementPtr)
{
	ElementPtr->next.image.frame =
			IncFrameIndex (ElementPtr->current.image.frame);
	ElementPtr->state_flags |= CHANGING;
}

// When gas expires, display animation of the gas dissolving.
static void
gas_death (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if (StarShipPtr->hShip)
	{
		HELEMENT hDissolve;
		ELEMENT *ShipPtr;
		
		LockElement (StarShipPtr->hShip, &ShipPtr);
	
		if ((hDissolve = AllocElement ()))
		{
			ELEMENT *DissolvePtr;
			
			LockElement (hDissolve, &DissolvePtr);
			DissolvePtr->playerNr = ElementPtr->playerNr;
			DissolvePtr->state_flags =
					FINITE_LIFE | NONSOLID | IGNORE_SIMILAR | APPEARING;
			DissolvePtr->turn_wait = 0;
			DissolvePtr->life_span = NUM_DISSOLVE_FRAMES;
			DissolvePtr->current.location.x = ElementPtr->current.location.x;
			DissolvePtr->current.location.y = ElementPtr->current.location.y;
			DissolvePtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
			DissolvePtr->current.image.frame =
					SetAbsFrameIndex (ElementPtr->current.image.frame,
						LAST_SHOCKWAVE_INDEX);
			DissolvePtr->preprocess_func = gas_death_animation;
			SetElementStarShip (DissolvePtr, StarShipPtr);
			SetPrimType (&(GLOBAL (DisplayArray))[DissolvePtr->PrimIndex],
					STAMP_PRIM);
			
			UnlockElement (hDissolve);
			PutElement (hDissolve);
		}
		
		UnlockElement (StarShipPtr->hShip);
	}
}

static void
gas_preprocess (ELEMENT *ElementPtr)
{	
	STARSHIP *StarShipPtr;
	SDWORD dx, dy;
	
	// Baul's gas now flies forward. Slow down the gas smoothly.
	GetCurrentVelocityComponentsSdword (&ElementPtr->velocity, &dx, &dy);
	if (dx != 0 || dy != 0)
	{
		dx = (SDWORD)(dx * 9 / 10);
		dy = (SDWORD)(dy * 9 / 10);
		SetVelocityComponents (&ElementPtr->velocity, dx, dy);
	}
		
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	// Move to next image frame. (Abusing thrust_wait to slow down the anim.)
	if (ElementPtr->thrust_wait > 0)
		--ElementPtr->thrust_wait;
	else
	{
		// Abusing thrust_wait to slow down the anim.
		// (Should help performance a bit.)
		ElementPtr->thrust_wait = 1;
		
		// This makes the gas animate even if the ships are not moving and
		// the screen is stationary.
		ElementPtr->state_flags |= CHANGING;
		
		if (GetFrameIndex (ElementPtr->current.image.frame) >= LAST_DISSOLVE_INDEX
				&& GetFrameIndex (ElementPtr->current.image.frame) < LAST_DISSOLVE_INDEX + NUM_EMERGE_FRAMES)
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
		else if (GetFrameIndex (ElementPtr->current.image.frame) < LAST_GAS_INDEX - 1)
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
		else
			ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, 0);
	}
	
	// If enemy ship dies, remove the gas (this prevents game crashing upon
	// enemy ship dying with gas on it).
	if ((!(ElementPtr->state_flags & IGNORE_VELOCITY)
			&& ElementPtr->hTarget == 0)
			|| StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING;
	}
	// When the gas has collided with enemy ship, it sticks to the ship until
	// expires.
	// (When the gas is sticking to enemy ship, the gas's IGNORE_VELOCITY
	// flag is disabled.)
	else if (!(ElementPtr->state_flags & IGNORE_VELOCITY)
			&& !(ElementPtr->state_flags & DISAPPEARING))
	{
		ELEMENT *eptr;
		SIZE offs_x, offs_y;
		SBYTE leftOrRight, upOrDown;
		COUNT angle, angleCorrect;
		static BYTE alignment[NUM_SIDES]={0,0};
		
		// eptr points to enemy ship now.
		LockElement (ElementPtr->hTarget, &eptr);
		
		// Make gas's location the same as the enemy ship's.
		ElementPtr->next.location = eptr->next.location;
		
		// Randomize the gas's location so every gas cloud doesn't stick to
		// the same place on the enemy ship.
		GetElementStarShip (eptr, &StarShipPtr);
		angle = (ElementPtr->weapon_element_index) % 16;
		alignment[ElementPtr->playerNr] = ElementPtr->weapon_element_index % 4;
		if (alignment[ElementPtr->playerNr] == 0)
		{
			leftOrRight = -1;
			upOrDown = 1;
			angleCorrect = 0;
		}
		else if (alignment[ElementPtr->playerNr] == 1)
		{
			leftOrRight = 1;
			upOrDown = -1;
			angleCorrect = 0;
		}
		else if (alignment[ElementPtr->playerNr] == 2)
		{
			leftOrRight = -1;
			upOrDown = 1;
			angleCorrect = HALF_CIRCLE / 2;
		}
		else
		{
			leftOrRight = 1;
			upOrDown = -1;
			angleCorrect = HALF_CIRCLE / 2;
		}
		offs_x = SINE (angle - angleCorrect,
				(ElementPtr->weapon_element_index % 16) * RES_SCALE (5));
		offs_y = COSINE (angle - angleCorrect,
				(ElementPtr->weapon_element_index % 16) * RES_SCALE (5));
		ElementPtr->next.location.x =
				ElementPtr->next.location.x + leftOrRight * offs_x;
		ElementPtr->next.location.y =
				ElementPtr->next.location.y + upOrDown * offs_y;
	
		if (ElementPtr->turn_wait)
		{
			HELEMENT hEffect;
			
			hEffect = AllocElement ();
			if (hEffect)
			{
				// eptr points to the new gas element now.
				LockElement (hEffect, &eptr);
				eptr->playerNr = ElementPtr->playerNr;
				eptr->state_flags = FINITE_LIFE | GASSY_SUBSTANCE | CHANGING;
				eptr->life_span = 1;
				eptr->thrust_wait = 1;
				eptr->weapon_element_index = ElementPtr->weapon_element_index;
				eptr->current = eptr->next = ElementPtr->next;
				eptr->preprocess_func = gas_preprocess;
				eptr->collision_func = gas_collision;
				// No need to have death_func here: It carries on from the
				// declaration in gas_collision.
				// In fact, if gas_death is put here as death_func,
				// it just messes up the graphics.
				
				SetPrimType (&(GLOBAL (DisplayArray))[eptr->PrimIndex],
						STAMP_PRIM);
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

static void
gas_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	STARSHIP *StarShipPtr;
	STARSHIP *EnemyStarShipPtr;
	BYTE	 enemyShipIsBaul = 0;
	BYTE	 enemyShipIsChmmr = 0;
	
	// This is the ship this gas cloud belongs to.
	GetElementStarShip (ElementPtr0, &StarShipPtr);
	
	// Check if the colliding element is a ship. If it is not, check if it's a projectile from Baul or Chmmr ship.
	if (!elementsOfSamePlayer(ElementPtr0, ElementPtr1) && !(ElementPtr1->state_flags & PLAYER_SHIP) 
		&& ElementPtr1->playerNr > -1)
	{
		GetElementStarShip (ElementPtr1, &EnemyStarShipPtr);
		if (EnemyStarShipPtr->SpeciesID == BAUL_ID)
			enemyShipIsBaul = 1;
		else if (EnemyStarShipPtr->SpeciesID == CHMMR_ID) 
			enemyShipIsChmmr = 1; // This is important because the gas can stick to zapsats.
	}
	
	// If colliding with Baul's spray weapon or shockwave, EXPLODE!!!
	if (ElementPtr1->current.image.farray == StarShipPtr->RaceDescPtr->ship_data.weapon
		|| (enemyShipIsBaul && ElementPtr1->current.image.farray == EnemyStarShipPtr->RaceDescPtr->ship_data.weapon))
	{
		// Move to shockwave graphics.
		ElementPtr0->current.image.frame = SetAbsFrameIndex (ElementPtr0->current.image.frame, LAST_GAS_INDEX);
		ElementPtr0->next.image.frame = SetAbsFrameIndex (ElementPtr0->current.image.frame, LAST_GAS_INDEX);
		
		// Remove the lock on enemy ship and make the gas die on next turn.
		ElementPtr0->hTarget = 0;
		ElementPtr0->life_span = 1;
		
		// Don't do the gas dissolve anim now that the shockwave appears.
		ElementPtr0->death_func = NULL;
		
		// Generate the actual shockwave.
		generate_shockwave (ElementPtr0, -1); // XXX ElementPtr1->playerNr);
	}
	// If colliding with enemy ship, stick to the ship.
	// Also stick to Chmmr's zapsats.
	else if (ElementPtr0->state_flags & IGNORE_VELOCITY
			 && ElementPtr1->playerNr != ElementPtr0->playerNr
			 && (ElementPtr1->state_flags & PLAYER_SHIP 
				 || (enemyShipIsChmmr && ElementPtr1->mass_points == 10) ))
	{
		HELEMENT hGasElement;
		HELEMENT hTargetElement;
		ELEMENT *GasPtr;
		
		// Create a new gas element which is sticking to the enemy ship.
		if ((hGasElement = AllocElement ()))
		{
			LockElement (hGasElement, &GasPtr);
			
			if (GasPtr->state_flags == 0) /* not allocated before */
			{
				InsertElement (hGasElement, GetHeadElement ());
				
				GasPtr->current = ElementPtr0->next;
				GasPtr->next = GasPtr->current;
				GasPtr->playerNr = ElementPtr0->playerNr;
				GasPtr->state_flags = FINITE_LIFE | GASSY_SUBSTANCE | CHANGING;
				GasPtr->preprocess_func = gas_preprocess;
				GasPtr->collision_func = gas_collision;
				SetPrimType (&(GLOBAL (DisplayArray))[GasPtr->PrimIndex], NO_PRIM);
				
				SetElementStarShip (GasPtr, StarShipPtr);
				GetElementStarShip (ElementPtr1, &StarShipPtr);
				
				// Ships and Chmmr Zapsats require different ways of making them the target of the gas cloud.
				if (ElementPtr1->state_flags & PLAYER_SHIP)
					GasPtr->hTarget = StarShipPtr->hShip;
				else
				{
					GasPtr->life_span = 0;
					LockElement (ElementPtr1, &hTargetElement);
					GasPtr->hTarget = hTargetElement;
				}
			}
			GasPtr->hit_points = ElementPtr0->hit_points;
			GasPtr->life_span = ElementPtr0->life_span;
			GasPtr->thrust_wait = 1;
			GasPtr->weapon_element_index = ElementPtr0->weapon_element_index;
			GasPtr->turn_wait = (BYTE)(1 << ((BYTE)TFB_Random () & 1)); /* LEFT or RIGHT */
			GasPtr->death_func = gas_death;
			
			UnlockElement (hGasElement);
		}
		
		// Erase the original gas element.
		ElementPtr0->hit_points = 0;
		ElementPtr0->life_span = 0;
		ElementPtr0->state_flags |= DISAPPEARING | COLLISION | NONSOLID;
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

// Secondary weapon: Gas cloud.
// The IGNORE_VELOCITY flag is very important: It doesn't only stop the gas from reacting to gravity,
// (see collide.h) but it also makes it possible for the gas to stick to enemy ship (see this file's other gas functions).
static void spawn_gas (ELEMENT *ShipPtr)
{	
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	HELEMENT Missile;
	SIZE offs_x, offs_y;
	COUNT angle;
	static COUNT gas_side[NUM_SIDES]   = {0, 0};
	static COUNT gas_number[NUM_SIDES] = {0, 0};
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	
	gas_number[ShipPtr->playerNr] = (gas_number[ShipPtr->playerNr] + 1) % 32;
	gas_side[ShipPtr->playerNr] = (gas_side[ShipPtr->playerNr] + 1) % 2;
	angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
	
	// This mechanism can be used to alter the "pipe" from which the gas clouds come.
	if(gas_side[ShipPtr->playerNr])
	{
		offs_x = -SINE (angle, GAS_HORZ_OFFSET);
		offs_y = COSINE (angle, GAS_HORZ_OFFSET);
	}
	else
	{
		offs_x = -SINE (angle, GAS_HORZ_OFFSET);
		offs_y = COSINE (angle, GAS_HORZ_OFFSET);
	}
		
	MissileBlock.cx = ShipPtr->next.location.x + offs_x;
	MissileBlock.cy = ShipPtr->next.location.y + offs_y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing;// Baul's gas now flies forward. (this was: (StarShipPtr->ShipFacing - 8) % 16;)
	MissileBlock.index = LAST_DISSOLVE_INDEX; // Start with the gas emerge animation which is the last .pngs in gasXX.ani
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = GASSY_SUBSTANCE | IGNORE_VELOCITY; // Don't erase the IGNORE_VELOCITY. It's very important.
	MissileBlock.pixoffs = GAS_OFFSET;
	MissileBlock.speed = GAS_INIT_SPEED;
	MissileBlock.hit_points = GAS_HITS;
	MissileBlock.damage = GAS_DAMAGE;
	MissileBlock.life = GAS_LIFE;
	MissileBlock.preprocess_func = gas_preprocess;
	MissileBlock.blast_offs = 0;
	Missile = initialize_missile (&MissileBlock);
	
	if (Missile)
	{
		ELEMENT *GasPtr;
		SIZE	dx, dy; // Baul's gas now flies forward.
		
		LockElement (Missile, &GasPtr);
		
		// Baul's gas now flies forward.
		GetCurrentVelocityComponents (&ShipPtr->velocity, &dx, &dy);
		DeltaVelocityComponents (&GasPtr->velocity, dx, dy);
		GasPtr->current.location.x -= VELOCITY_TO_WORLD (dx);
		GasPtr->current.location.y -= VELOCITY_TO_WORLD (dy);
		
		GasPtr->collision_func = gas_collision;
		GasPtr->death_func = gas_death;
		GasPtr->thrust_wait = 1;
		GasPtr->weapon_element_index = gas_number[ShipPtr->playerNr];
		SetElementStarShip (GasPtr, StarShipPtr);
		ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), GasPtr);
		UnlockElement (Missile);
		PutElement (Missile);
	}
}

// The spray preprocess function animates spray.
static void
spray_preprocess (ELEMENT *ElementPtr)
{
	// Abusing thrust_wait to slow down the anim.
	if (ElementPtr->thrust_wait > 0)
		--ElementPtr->thrust_wait;
	// Move to next frame.
	else
	{
		// Abusing thrust_wait to slow down the anim. (Should help performance a bit.)
		ElementPtr->thrust_wait = 1;
		
		// This makes the gas animate even if the ships are not moving and the screen is stationary.
		ElementPtr->state_flags |= CHANGING;
		
		if (GetFrameIndex (ElementPtr->current.image.frame) < LAST_SPRAY_INDEX)
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);
		// This is a safeguard to prevent going over frame boundaries if someone messes up the 
		// MISSILE_LIFE <-> LAST_SPRAY_INDEX <-> thrust_wait correspondence.
		else
			ElementPtr->next.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, 0);
	}
}

// This makes the spray not collide with anything (except gas clouds: gas_collision handles them.)
static void
spray_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	(void) ElementPtr0;  /* Satisfying compiler (unused parameter) */
	(void) ElementPtr1;  /* Satisfying compiler (unused parameter) */
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

// Primary weapon. It must deal at least 1 damage, otherwise it won't interact with other 
// elements, not even gas. However, we can prevent this damage with a separate collision function.
static COUNT
initialize_spray (ELEMENT *ShipPtr, HELEMENT SprayArray[])
{
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;
	SIZE offs_x, offs_y;
	COUNT i, angle;
	static COUNT spray_side[NUM_SIDES]={0,0};
	
	GetElementStarShip (ShipPtr, &StarShipPtr);
	angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
	
	for (i = 0; i < NUM_SPRAYS; i++)
	{
		BYTE damage;
		
		// Only the foremost one deals damage
		if (i == NUM_SPRAYS-1)
			damage = 1;
		else
			damage = 0;
		
		// This mechanism can be used to alter the "pipe" from which the spray particles come.
		spray_side[ShipPtr->playerNr] = (spray_side[ShipPtr->playerNr] + 1) % 2;
		if(spray_side[ShipPtr->playerNr])
		{
			offs_x = -SINE (angle,  SPRAY_HORZ_OFFSET + RES_SCALE (i));
			offs_y = COSINE (angle, SPRAY_HORZ_OFFSET + RES_SCALE (i));
		}
		else
		{
			offs_x = -SINE (angle,  SPRAY_HORZ_OFFSET + RES_SCALE (i));
			offs_y = COSINE (angle, SPRAY_HORZ_OFFSET + RES_SCALE (i));
		}
	
		MissileBlock.cx = ShipPtr->next.location.x + offs_x;
		MissileBlock.cy = ShipPtr->next.location.y + offs_y;
		MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
		MissileBlock.face = StarShipPtr->ShipFacing;
		MissileBlock.index = 0;
		MissileBlock.sender = ShipPtr->playerNr;
		MissileBlock.flags = IGNORE_SIMILAR | GASSY_SUBSTANCE;
		MissileBlock.pixoffs = 4 + RES_SCALE (i * SPRAY_DIST);
		MissileBlock.speed = RES_SCALE (MISSILE_SPEED);
		MissileBlock.hit_points = MISSILE_HITS;
		MissileBlock.damage = damage;
		MissileBlock.life = MISSILE_LIFE;
		MissileBlock.preprocess_func = spray_preprocess;
		MissileBlock.blast_offs = MISSILE_OFFSET;
		SprayArray[i] = initialize_missile (&MissileBlock);
		
		if (SprayArray[i])
		{
			ELEMENT *SprayPtr;
			
			LockElement (SprayArray[i], &SprayPtr);
			SprayPtr->collision_func = spray_collision;
			SprayPtr->thrust_wait = 1;
			
			// This makes the spray shoot in a slight angle towards the centerline.
			// If you want a "curved" shot, put this mechanism into spray_preprocess
			// where it accelerates the whot towards the centerline on every frame.
			offs_x = -SINE (angle, RES_SCALE (100));
			offs_y = COSINE (angle, RES_SCALE (100));
			DeltaVelocityComponents (&SprayPtr->velocity, offs_x, offs_y);
			
			UnlockElement (SprayArray[i]);
		}
	}
	
	return (NUM_SPRAYS);
}

// Gas spawning happens in postprocess, because  the game seems to like to put specials on the playing field
// in postprocess (I guess it has something to do with keeping the queue of elements in the right order.)
static void
baul_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	
	if ((StarShipPtr->cur_status_flags & SPECIAL)
		&& StarShipPtr->special_counter == 0
		&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		int i;
		
		ProcessSound (SetAbsSoundIndex // Spawn gas.
					  (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		
		for (i = 0; i < GAS_BATCH_SIZE; i++)
			spawn_gas (ElementPtr);
		
		StarShipPtr->special_counter = SPECIAL_WAIT;
	}
}

static void
baul_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
}

RACE_DESC*
init_baul (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD)
	{
		baul_desc.characteristics.max_thrust = RES_SCALE (MAX_THRUST);
		baul_desc.characteristics.thrust_increment = RES_SCALE (THRUST_INCREMENT);
		baul_desc.cyborg_control.WeaponRange = MISSILE_RANGE_HD;
	}
	else
	{
		baul_desc.characteristics.max_thrust = MAX_THRUST;
		baul_desc.characteristics.thrust_increment = THRUST_INCREMENT;
		baul_desc.cyborg_control.WeaponRange = MISSILE_RANGE;
	}

	baul_desc.preprocess_func = baul_preprocess;
	baul_desc.postprocess_func = baul_postprocess;
	baul_desc.init_weapon_func = initialize_spray;
	baul_desc.cyborg_control.intelligence_func = baul_intelligence;
	RaceDescPtr = &baul_desc;
	
	return (RaceDescPtr);
}
