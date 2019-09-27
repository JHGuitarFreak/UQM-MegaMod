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
#include "shofixti.h"
#include "resinst.h"
#include "../../setup.h"
#include "uqm/globdata.h"
#include "uqm/tactrans.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 6
#define MAX_ENERGY 4
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 9
#define MAX_THRUST 35
#define THRUST_INCREMENT 5
#define TURN_WAIT 1
#define THRUST_WAIT 0
#define WEAPON_WAIT 3
#define SPECIAL_WAIT 0
#define SHIP_MASS 1

// Dart Gun
#define WEAPON_ENERGY_COST 1
#define SHOFIXTI_OFFSET RES_SCALE(15)
#define MISSILE_OFFSET RES_SCALE(1)
#define MISSILE_SPEED DISPLAY_TO_WORLD (24)
#define MISSILE_LIFE 10
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1

// Glory Device
#define SPECIAL_ENERGY_COST 0
#define SPECIAL_SAUCE 180
#define DESTRUCT_RANGE RES_SCALE(SPECIAL_SAUCE)
#define MAX_DESTRUCTION (SPECIAL_SAUCE / 10)

// Full game: Tanaka/Katana's damaged ships
#define NUM_LIMPETS 3

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (96)

static RACE_DESC shofixti_desc =
{
	{ /* SHIP_INFO */
		"scout",
		FIRES_FORE,
		5, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		SHOFIXTI_RACE_STRINGS,
		SHOFIXTI_ICON_MASK_PMAP_ANIM,
		SHOFIXTI_MICON_MASK_PMAP_ANIM,
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
			SHOFIXTI_BIG_MASK_PMAP_ANIM,
			SHOFIXTI_MED_MASK_PMAP_ANIM,
			SHOFIXTI_SML_MASK_PMAP_ANIM,
		},
		{
			DART_BIG_MASK_PMAP_ANIM,
			DART_MED_MASK_PMAP_ANIM,
			DART_SML_MASK_PMAP_ANIM,
		},
		{
			DESTRUCT_BIG_MASK_ANIM,
			DESTRUCT_MED_MASK_ANIM,
			DESTRUCT_SML_MASK_ANIM,
		},
		{
			SHOFIXTI_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
	        SHOFIXTI_VICTORY_SONG,
		SHOFIXTI_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		MISSILE_SPEED * MISSILE_LIFE,
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
initialize_standard_missile (ELEMENT *ShipPtr, HELEMENT MissileArray[])
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
	MissileBlock.pixoffs = SHOFIXTI_OFFSET;
	MissileBlock.speed = RES_SCALE(MISSILE_SPEED);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	MissileArray[0] = initialize_missile (&MissileBlock);

	return (1);
}

static void
destruct_preprocess (ELEMENT *ElementPtr)
{
#define DESTRUCT_SWITCH ((NUM_EXPLOSION_FRAMES * 3) - 3)
	PRIMITIVE *lpPrim;

	// ship_death() set the ship element's life_span to
	// (NUM_EXPLOSION_FRAMES * 3)
	lpPrim = &(GLOBAL (DisplayArray))[ElementPtr->PrimIndex];
	ElementPtr->state_flags |= CHANGING;
	if (ElementPtr->life_span > DESTRUCT_SWITCH)
	{
		// First, stamp-fill the ship's own element with changing colors
		// for 3 frames. No explosion element yet.
		SetPrimType (lpPrim, STAMPFILL_PRIM);
		if (ElementPtr->life_span == DESTRUCT_SWITCH + 2)
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E));
		else
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
	}
	else if (ElementPtr->life_span < DESTRUCT_SWITCH)
	{
		// Stamp-fill the explosion element with cycling colors for the
		// remainder of the glory explosion frames.
		Color color = GetPrimColor (lpPrim);

		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		if (sameColor (color,
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F)))
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E));
		else if (sameColor (color,
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x0A), 0x0E)))
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C));
		else if (sameColor (color,
				BUILD_COLOR (MAKE_RGB15 (0x1F, 0x0A, 0x0A), 0x0C)))
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x14, 0x0A, 0x00), 0x06));
		else if (sameColor (color,
				BUILD_COLOR (MAKE_RGB15 (0x14, 0x0A, 0x00), 0x06)))
			SetPrimColor (lpPrim,
					BUILD_COLOR (MAKE_RGB15 (0x14, 0x00, 0x00), 0x04));
	}
	else
	{
		HELEMENT hDestruct;

		SetPrimType (lpPrim, NO_PRIM);
		// The ship's own element will not be drawn anymore but will remain
		// alive all through the glory explosion.
		ElementPtr->preprocess_func = NULL;

		// Spawn a separate glory explosion element.
		// XXX: Why? Why not keep using the ship's element?
		//   Is it because of conflicting state_flags, hit_points or
		//   mass_points?
		hDestruct = AllocElement ();
		if (hDestruct)
		{
			ELEMENT *DestructPtr;
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);

			PutElement (hDestruct);
			LockElement (hDestruct, &DestructPtr);
			SetElementStarShip (DestructPtr, StarShipPtr);
			DestructPtr->hit_points = 0;
			DestructPtr->mass_points = 0;
			DestructPtr->playerNr = NEUTRAL_PLAYER_NUM;
			DestructPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			SetPrimType (&(GLOBAL (DisplayArray))[DestructPtr->PrimIndex],
					STAMPFILL_PRIM);
			SetPrimColor (&(GLOBAL (DisplayArray))[DestructPtr->PrimIndex],
					BUILD_COLOR (MAKE_RGB15 (0x1F, 0x1F, 0x1F), 0x0F));
			DestructPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
			DestructPtr->current.image.frame =
					StarShipPtr->RaceDescPtr->ship_data.special[0];
			DestructPtr->life_span = GetFrameCount (
					DestructPtr->current.image.frame);
			DestructPtr->current.location = ElementPtr->current.location;
			DestructPtr->preprocess_func = destruct_preprocess;
			DestructPtr->postprocess_func = NULL;
			DestructPtr->death_func = NULL;
			ZeroVelocityComponents (&DestructPtr->velocity);
			UnlockElement (hDestruct);
		}
	}
}

/* In order to detect any Orz Marines that have boarded the ship
   when it self-destructs, we'll need to see some Orz functions */
#include "../orz/orz.h"
#define ORZ_MARINE(ptr) (ptr->preprocess_func == intruder_preprocess && \
		ptr->collision_func == marine_collision)

static void
self_destruct_kill_objects (ELEMENT *ElementPtr)
{
	// This is called during PostProcessQueue(), close to or at the end,
	// for the temporary destruct element to apply the effects of glory
	// explosion. The effects are not seen until the next frame.
	HELEMENT hElement, hNextElement;

	for (hElement = GetHeadElement (); hElement != 0; hElement = hNextElement)
	{
		ELEMENT *ObjPtr;
		SDWORD delta_x, delta_y;
		DWORD dist;

		LockElement (hElement, &ObjPtr);
		hNextElement = GetSuccElement (ObjPtr);

		if (!CollidingElement (ObjPtr) && !ORZ_MARINE (ObjPtr))
		{
			UnlockElement (hElement);
			continue;
		}

		delta_x = ObjPtr->next.location.x - ElementPtr->next.location.x;
		if (delta_x < 0)
			delta_x = -delta_x;
		delta_y = ObjPtr->next.location.y - ElementPtr->next.location.y;
		if (delta_y < 0)
			delta_y = -delta_y;
		delta_x = WORLD_TO_DISPLAY (delta_x);
		delta_y = WORLD_TO_DISPLAY (delta_y);
		dist = delta_x * delta_x + delta_y * delta_y;
		if (delta_x <= DESTRUCT_RANGE && delta_y <= DESTRUCT_RANGE
				&& dist <= DESTRUCT_RANGE * DESTRUCT_RANGE)
		{
			int destruction = 1 + MAX_DESTRUCTION *
					(DESTRUCT_RANGE - square_root (dist)) / DESTRUCT_RANGE;

			// XXX: Why not simply call do_damage()?
			if (ObjPtr->state_flags & PLAYER_SHIP)
			{
				if (!antiCheat(ElementPtr, TRUE)) {
					if (!DeltaCrew (ObjPtr, -destruction))
						ObjPtr->life_span = 0;
				}
			}
			else if (!GRAVITY_MASS (ObjPtr->mass_points))
			{
				if (destruction < ObjPtr->hit_points)
					ObjPtr->hit_points -= destruction;
				else
				{
					ObjPtr->hit_points = 0;
					ObjPtr->life_span = 0;
				}
			}
		}

		UnlockElement (hElement);
	}
}

// This function is called when the ship dies via Glory Device.
// The generic ship_death() function is not called for the ship in this case.
static void
shofixti_destruct_death (ELEMENT *ShipPtr)
{
	STARSHIP *StarShip;
	STARSHIP *winner;

	GetElementStarShip (ShipPtr, &StarShip);

	StopAllBattleMusic ();

	StartShipExplosion (ShipPtr, false);
	// We process the explosion ourselves because it is different
	ShipPtr->preprocess_func = destruct_preprocess;
	
	PlaySound (SetAbsSoundIndex (StarShip->RaceDescPtr->ship_data.ship_sounds,
			1), CalcSoundPosition (ShipPtr), ShipPtr, GAME_SOUND_PRIORITY + 1);

	winner = GetWinnerStarShip ();
	if (winner == NULL)
	{	// No winner determined yet
		winner = FindAliveStarShip (ShipPtr);
		if (winner == NULL)
		{	// No ships left alive after the Glory Device thus Shofixti wins
			winner = StarShip;
		}
		SetWinnerStarShip (winner);
	}
	else if (winner == StarShip)
	{	// This ship is the winner
		// It may have self-destructed before the ditty started playing,
		// and in that case, there should be no ditty
		StarShip->cur_status_flags &= ~PLAY_VICTORY_DITTY;
	}
	RecordShipDeath (ShipPtr);
}

static void
self_destruct (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	HELEMENT hDestruct;

	GetElementStarShip (ElementPtr, &StarShipPtr);
		
	// Spawn a temporary element, which dies in this same frame, in order
	// to defer the effects of the glory explosion.
	// It will be the last element (or one of the last) for which the
	// death_func() will be called from PostProcessQueue() in this frame.
	// XXX: Why at the end? Why not just do it now?
	hDestruct = AllocElement ();
	if (hDestruct)
	{
		ELEMENT *DestructPtr;

		LockElement (hDestruct, &DestructPtr);
		DestructPtr->playerNr = ElementPtr->playerNr;
		DestructPtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE;
		DestructPtr->next.location = ElementPtr->next.location;
		DestructPtr->life_span = 0;
		DestructPtr->pParent = ElementPtr->pParent;
		DestructPtr->hTarget = 0;

		DestructPtr->death_func = self_destruct_kill_objects;

		UnlockElement (hDestruct);

		PutElement (hDestruct);
	}

	// Must kill off the remaining crew ourselves
	DeltaCrew (ElementPtr, -(int)ElementPtr->crew_level);

	ElementPtr->state_flags |= NONSOLID;
	ElementPtr->life_span = 0;
	// The ship is now dead. It's death_func, i.e. shofixti_destruct_death(),
	// will be called the next frame.
	ElementPtr->death_func = shofixti_destruct_death;
}

static void
shofixti_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter != 0)
		return;

	if (StarShipPtr->ship_input_state & SPECIAL)
		StarShipPtr->ship_input_state &= ~SPECIAL;
	else
	{
		EVALUATE_DESC *lpWeaponEvalDesc;
		EVALUATE_DESC *lpShipEvalDesc;

		lpWeaponEvalDesc = &ObjectsOfConcern[ENEMY_WEAPON_INDEX];
		lpShipEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
		if (StarShipPtr->RaceDescPtr->ship_data.special[0]
				&& (GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.
				captain_control.special)
				- GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.
				captain_control.special) > 5
				|| (lpShipEvalDesc->ObjectPtr != NULL
				&& lpShipEvalDesc->which_turn <= 4)
				|| (lpWeaponEvalDesc->ObjectPtr != NULL
							/* means IMMEDIATE WEAPON */
				&& (((lpWeaponEvalDesc->ObjectPtr->state_flags & PLAYER_SHIP)
				&& ShipPtr->crew_level == 1)
				|| (PlotIntercept (lpWeaponEvalDesc->ObjectPtr, ShipPtr, 2, 0)
				&& lpWeaponEvalDesc->ObjectPtr->mass_points >=
				ShipPtr->crew_level
				&& (TFB_Random () & 1))))))
			StarShipPtr->ship_input_state |= SPECIAL;
	}
}

static void
shofixti_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags
			^ StarShipPtr->old_status_flags) & SPECIAL)
	{
		StarShipPtr->RaceDescPtr->ship_data.captain_control.special =
				IncFrameIndex (StarShipPtr->RaceDescPtr->ship_data.
				captain_control.special);
		if (GetFrameCount (StarShipPtr->RaceDescPtr->ship_data.
				captain_control.special)
				- GetFrameIndex (StarShipPtr->RaceDescPtr->ship_data.
				captain_control.special) == 3)
			self_destruct (ElementPtr);
	}
}

RACE_DESC*
init_shofixti (void)
{
	RACE_DESC *RaceDescPtr;
	// The caller of this func will copy the struct
	static RACE_DESC new_shofixti_desc;

	if (IS_HD) {
		shofixti_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		shofixti_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		shofixti_desc.cyborg_control.WeaponRange = MISSILE_SPEED_HD * MISSILE_LIFE;
	}

	shofixti_desc.postprocess_func = shofixti_postprocess;
	shofixti_desc.init_weapon_func = initialize_standard_missile;
	shofixti_desc.cyborg_control.intelligence_func = shofixti_intelligence;

	new_shofixti_desc = shofixti_desc;
	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			&& !GET_GAME_STATE (SHOFIXTI_RECRUITED))
	{
		// Tanaka/Katana flies in a damaged ship.
		COUNT i;

		new_shofixti_desc.ship_data.ship_rsc[0] = OLDSHOF_BIG_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.ship_rsc[1] = OLDSHOF_MED_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.ship_rsc[2] = OLDSHOF_SML_MASK_PMAP_ANIM;
		new_shofixti_desc.ship_data.special_rsc[0] = NULL_RESOURCE;
		new_shofixti_desc.ship_data.special_rsc[1] = NULL_RESOURCE;
		new_shofixti_desc.ship_data.special_rsc[2] = NULL_RESOURCE;
		new_shofixti_desc.ship_data.captain_control.captain_rsc =
				OLDSHOF_CAPTAIN_MASK_PMAP_ANIM;
		// JMS: Tanaka also has a corresponding limpeted ship status icon in melee.
		new_shofixti_desc.ship_info.icons_rsc = OLDSHOF_ICON_MASK_PMAP_ANIM;

		/* Weapon doesn't work as well */
		if (!DIF_HARD)
			new_shofixti_desc.characteristics.weapon_wait = 10;
		
		/* Simulate VUX limpets */
		for (i = 0; i < NUM_LIMPETS; ++i)
		{
			if (!DIF_HARD) {
				if (++new_shofixti_desc.characteristics.turn_wait == 0)
					--new_shofixti_desc.characteristics.turn_wait;
				if (++new_shofixti_desc.characteristics.thrust_wait == 0)
					--new_shofixti_desc.characteristics.thrust_wait;

				/* This should be the same as MIN_THRUST_INCREMENT in vux.c */
#define MIN_THRUST_INCREMENT DISPLAY_TO_WORLD (RES_SCALE(1))

				if (new_shofixti_desc.characteristics.thrust_increment <=
					MIN_THRUST_INCREMENT)
				{
					new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment << 1;
				}
				else
				{
					COUNT num_thrusts;

					num_thrusts = new_shofixti_desc.characteristics.max_thrust /
						new_shofixti_desc.characteristics.thrust_increment;
					new_shofixti_desc.characteristics.thrust_increment -= RES_SCALE(1); // JMS_GFX
					new_shofixti_desc.characteristics.max_thrust =
						new_shofixti_desc.characteristics.thrust_increment *
						num_thrusts;
				}
			}
		}
	}

	RaceDescPtr = &new_shofixti_desc;

	return (RaceDescPtr);
}

