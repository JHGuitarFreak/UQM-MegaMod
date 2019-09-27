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
#include "lastbat.h"
#include "resinst.h"

#include "uqm/colors.h"
#include "uqm/globdata.h"
#include "uqm/battle.h"
		// For BATTLE_FRAME_RATE
#include "libs/mathlib.h"
#include "libs/timelib.h"

#define num_generators characteristics.max_thrust

// Core characteristics
#define MAX_CREW 1
#define MAX_ENERGY MAX_ENERGY_SIZE
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST 0
#define THRUST_INCREMENT 0
#define TURN_WAIT 0
#define THRUST_WAIT 0
#define SHIP_MASS (MAX_SHIP_MASS * 10)
#define TURRET_WAIT 0 /* Controls animation of the Sa-Matra's central
                       * 'furnace', a new frame is displayed once every
                       * TURRET_WAIT frames. */

// Yellow comet
#define WEAPON_WAIT ((ONE_SECOND / BATTLE_FRAME_RATE) * 10)
#define COMET_DAMAGE 2
#define COMET_OFFSET 0
#define COMET_HITS 12
#define COMET_SPEED DISPLAY_TO_WORLD (RES_SCALE(12))
#define COMET_LIFE 2
#define COMET_TURN_WAIT 3
#define MAX_COMETS 3
#define WEAPON_ENERGY_COST 2
		/* Used for samatra_desc.weapon_energy_cost, but the value isn't
		 * actually used. */

// Green sentinel
#define SPECIAL_WAIT ((ONE_SECOND / BATTLE_FRAME_RATE) * 3)
#define SENTINEL_SPEED DISPLAY_TO_WORLD (RES_SCALE(8))
#define SENTINEL_LIFE 2
#define SENTINEL_OFFSET 0
#define SENTINEL_HITS 10
#define SENTINEL_DAMAGE 1
#define TRACK_WAIT 1
#define ANIMATION_WAIT 1
#define RECOIL_VELOCITY WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE(10)))
#define MAX_RECOIL_VELOCITY (RECOIL_VELOCITY * 4)
#define MAX_SENTINELS 4
#define SPECIAL_ENERGY_COST 3
		/* Used for samatra_desc.special_energy_cost, but the value isn't
		 * actually used. */

// Blue force field
#define GATE_DAMAGE 1
#define GATE_HITS 100

// Red generators
#define GENERATOR_HITS 15
#define MAX_GENERATORS 8

static RACE_DESC samatra_desc =
{
	{ /* SHIP_INFO */
		"samatra",
		/* FIRES_FORE | */ IMMEDIATE_WEAPON | CREW_IMMUNE,
		16, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		NULL_RESOURCE,
		NULL_RESOURCE,
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
			SAMATRA_BIG_MASK_ANIM,
			SAMATRA_MED_MASK_PMAP_ANIM,
			SAMATRA_SML_MASK_PMAP_ANIM,
		},
		{
			SENTINEL_BIG_MASK_ANIM,
			SENTINEL_MED_MASK_PMAP_ANIM,
			SENTINEL_SML_MASK_PMAP_ANIM,
		},
		{
			GENERATOR_BIG_MASK_ANIM,
			GENERATOR_MED_MASK_PMAP_ANIM,
			GENERATOR_SML_MASK_PMAP_ANIM,
		},
		{
			SAMATRA_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		NULL_RESOURCE,
		SAMATRA_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		0,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

static HELEMENT spawn_comet (ELEMENT *ElementPtr);

static void
comet_preprocess (ELEMENT *ElementPtr)
{
	COUNT frame_index;

	frame_index = GetFrameIndex (ElementPtr->current.image.frame) + 1;
	if (frame_index < 29)
	{
		if (frame_index == 25)
		{
			SDWORD cur_delta_x, cur_delta_y;
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);
			++StarShipPtr->RaceDescPtr->characteristics.weapon_wait;
			spawn_comet (ElementPtr);
			ElementPtr->state_flags |= NONSOLID;

			GetCurrentVelocityComponentsSdword (&ElementPtr->velocity,
					&cur_delta_x, &cur_delta_y);
			SetVelocityComponents (&ElementPtr->velocity,
					cur_delta_x / 2, cur_delta_y / 2);
		}
		++ElementPtr->life_span;
	}

	ElementPtr->next.image.frame =
			SetAbsFrameIndex (
			ElementPtr->current.image.frame, frame_index
			);
	ElementPtr->state_flags |= CHANGING;
}

static void
comet_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (ElementPtr1->playerNr == RPG_PLAYER_NUM)
	{
		BYTE old_hits;
		COUNT old_life;
		HELEMENT hBlastElement;

		if (ElementPtr1->state_flags & PLAYER_SHIP)
			ElementPtr0->mass_points = COMET_DAMAGE;
		else
			ElementPtr0->mass_points = 50;

		old_hits = ElementPtr0->hit_points;
		old_life = ElementPtr0->life_span;
		hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
		if (ElementPtr1->state_flags & PLAYER_SHIP)
		{
			ElementPtr0->hit_points = old_hits;
			ElementPtr0->life_span = old_life;
			ElementPtr0->state_flags &= ~(DISAPPEARING | NONSOLID | COLLISION);

			if (hBlastElement)
			{
				RemoveElement (hBlastElement);
				FreeElement (hBlastElement);
			}
		}

		if (ElementPtr0->state_flags & DISAPPEARING)
		{
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr0, &StarShipPtr);
			--StarShipPtr->RaceDescPtr->characteristics.weapon_wait;
		}
	}
}

static HELEMENT
spawn_comet (ELEMENT *ElementPtr)
{
	MISSILE_BLOCK MissileBlock;
	HELEMENT hComet;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.face = 0;
	MissileBlock.index = 24;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = 0;
	MissileBlock.speed = 0;
	MissileBlock.hit_points = COMET_HITS;
	MissileBlock.damage = COMET_DAMAGE;
	MissileBlock.life = COMET_LIFE;
	MissileBlock.preprocess_func = comet_preprocess;
	MissileBlock.blast_offs = COMET_OFFSET;
	hComet = initialize_missile (&MissileBlock);

	if (hComet)
	{
		ELEMENT *CometPtr;

		PutElement (hComet);

		LockElement (hComet, &CometPtr);
		CometPtr->collision_func = comet_collision;
		SetElementStarShip (CometPtr, StarShipPtr);
		{
			COUNT facing;

			CometPtr->turn_wait = ElementPtr->turn_wait;
			CometPtr->hTarget = ElementPtr->hTarget;
			if (ElementPtr->state_flags & PLAYER_SHIP)
			{
				CometPtr->turn_wait = 0;
				facing = (COUNT)TFB_Random ();
				SetVelocityVector (&CometPtr->velocity,
						COMET_SPEED, facing);
			}
			else
			{
				CometPtr->velocity = ElementPtr->velocity;
				CometPtr->hit_points = ElementPtr->hit_points;
				facing = ANGLE_TO_FACING (
						GetVelocityTravelAngle (&CometPtr->velocity)
						);
			}

			if (CometPtr->turn_wait)
				--CometPtr->turn_wait;
			else
			{
				facing = NORMALIZE_FACING (facing);
				if (TrackShip (CometPtr, &facing) > 0)
					SetVelocityVector (&CometPtr->velocity,
							COMET_SPEED, facing);
				CometPtr->turn_wait = COMET_TURN_WAIT;
			}
		}
		UnlockElement (hComet);
	}

	return (hComet);
}

static void
turret_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame,
				(GetFrameIndex (ElementPtr->current.image.frame) % 10) + 1);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->turn_wait = TURRET_WAIT;
	}
}

static void
gate_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (ElementPtr1->playerNr == RPG_PLAYER_NUM)
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr0, &StarShipPtr);
		if (StarShipPtr->RaceDescPtr->num_generators == 0)
		{
			if (!(ElementPtr1->state_flags & FINITE_LIFE))
				ElementPtr0->state_flags |= COLLISION;

			if ((ElementPtr1->state_flags & PLAYER_SHIP)
					&& GetPrimType (
					&GLOBAL (DisplayArray[ElementPtr0->PrimIndex])
					) == STAMPFILL_PRIM
					&& GET_GAME_STATE (BOMB_CARRIER))
			{
				GLOBAL (CurrentActivity) &= ~IN_BATTLE;
			}
		}
		else
		{
			HELEMENT hBlastElement;

			if (ElementPtr1->state_flags & PLAYER_SHIP)
				ElementPtr0->mass_points = GATE_DAMAGE;
			else
				ElementPtr0->mass_points = 50;
				
			ElementPtr0->hit_points = GATE_HITS;
			hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
			ElementPtr0->state_flags &= ~(DISAPPEARING | NONSOLID | COLLISION);
			ElementPtr0->life_span = NORMAL_LIFE;

			if (hBlastElement)
			{
				RemoveElement (hBlastElement);
				FreeElement (hBlastElement);
			}
		}
	}
}

static void
gate_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->num_generators == 0)
	{
		ElementPtr->mass_points = SHIP_MASS;
		ElementPtr->state_flags &= ~FINITE_LIFE;
		ElementPtr->life_span = NORMAL_LIFE + 1;
		ElementPtr->preprocess_func = 0;
		SetPrimColor (
				&GLOBAL (DisplayArray[ElementPtr->PrimIndex]),
				BLACK_COLOR
				);
		SetPrimType (
				&GLOBAL (DisplayArray[ElementPtr->PrimIndex]),
				STAMPFILL_PRIM
				);
	}
	else
	{
		++ElementPtr->life_span;
		ElementPtr->next.image.frame =
				IncFrameIndex (ElementPtr->current.image.frame);
		if (GetFrameIndex (ElementPtr->next.image.frame) == 0)
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (
					ElementPtr->next.image.frame, 11
					);

		ElementPtr->state_flags |= CHANGING;
	}
}

static void
generator_death (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & FINITE_LIFE))
	{
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		--StarShipPtr->RaceDescPtr->num_generators;
		ElementPtr->state_flags |= FINITE_LIFE | NONSOLID;
		ElementPtr->preprocess_func = 0;
		ElementPtr->turn_wait = 12;
		ElementPtr->thrust_wait = 0;

		ElementPtr->current.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame, 10 - 1);
	}

	if (ElementPtr->thrust_wait)
	{
		--ElementPtr->thrust_wait;
		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
		++ElementPtr->life_span;
	}
	else if (ElementPtr->turn_wait--)
	{
		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
		++ElementPtr->life_span;

		ElementPtr->next.image.frame = IncFrameIndex (
				ElementPtr->current.image.frame
				);

		ElementPtr->thrust_wait = 1;
	}
}

static void
generator_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else if ((ElementPtr->turn_wait =
			(BYTE)((GENERATOR_HITS
			- ElementPtr->hit_points) / 5)) < 3)
	{
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame,
				(GetFrameIndex (ElementPtr->current.image.frame) + 1) % 10);
		ElementPtr->state_flags |= CHANGING;
	}
}

static void
generator_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (!(ElementPtr1->state_flags & FINITE_LIFE))
	{
		ElementPtr0->state_flags |= COLLISION;
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
sentinel_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	++StarShipPtr->RaceDescPtr->characteristics.special_wait;
	++ElementPtr->life_span;

	if (ElementPtr->thrust_wait)
		--ElementPtr->thrust_wait;
	else
	{
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame,
				(GetFrameIndex (ElementPtr->current.image.frame) + 1) % 6);
		ElementPtr->state_flags |= CHANGING;

		ElementPtr->thrust_wait = ANIMATION_WAIT;
	}

	if (ElementPtr->turn_wait > 0)
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing;
		HELEMENT hTarget;

		if (!(ElementPtr->state_flags & NONSOLID))
			facing = ANGLE_TO_FACING (
					GetVelocityTravelAngle (&ElementPtr->velocity)
					);
		else
		{
			ElementPtr->state_flags &= ~NONSOLID;
			facing = (COUNT)TFB_Random ();
			SetVelocityVector (&ElementPtr->velocity,
					SENTINEL_SPEED, facing);
		}
		facing = NORMALIZE_FACING (facing);
		if (ElementPtr->hTarget == 0)
		{
			COUNT f;

			f = facing;
			TrackShip (ElementPtr, &f);
		}

		if (ElementPtr->hTarget == 0)
			hTarget = StarShipPtr->hShip;
		else if (StarShipPtr->hShip == 0)
			hTarget = ElementPtr->hTarget;
		else
		{
			SDWORD delta_x0, delta_y0, delta_x1, delta_y1;
			ELEMENT *ShipPtr;
			ELEMENT *EnemyShipPtr;

			LockElement (ElementPtr->hTarget, &EnemyShipPtr);

			LockElement (StarShipPtr->hShip, &ShipPtr);
			delta_x0 = (SDWORD)ShipPtr->current.location.x
					- (SDWORD)ElementPtr->current.location.x;
			delta_y0 = (SDWORD)ShipPtr->current.location.y
					- (SDWORD)ElementPtr->current.location.y;

			delta_x1 = (SDWORD)ShipPtr->current.location.x
					- (SDWORD)EnemyShipPtr->current.location.x;
			delta_y1 = (SDWORD)ShipPtr->current.location.y
					- (SDWORD)EnemyShipPtr->current.location.y;
			UnlockElement (StarShipPtr->hShip);

			if ((long)delta_x0 * delta_x0
					+ (long)delta_y0 * delta_y0 >
					(long)delta_x1 * delta_x1
					+ (long)delta_y1 * delta_y1)
				hTarget = StarShipPtr->hShip;
			else
				hTarget = ElementPtr->hTarget;

			UnlockElement (ElementPtr->hTarget);
		}

		if (hTarget)
		{
			COUNT num_frames;
			SDWORD delta_x, delta_y;
			ELEMENT *TargetPtr;
			VELOCITY_DESC TargetVelocity;

			LockElement (hTarget, &TargetPtr);

			delta_x = (SDWORD)TargetPtr->current.location.x
					- (SDWORD)ElementPtr->current.location.x;
			delta_x = WRAP_DELTA_X (delta_x);
			delta_y = (SDWORD)TargetPtr->current.location.y
					- (SDWORD)ElementPtr->current.location.y;
			delta_y = WRAP_DELTA_Y (delta_y);

			if ((num_frames = RES_DESCALE (WORLD_TO_TURN (
					square_root ((long)delta_x * delta_x
					+ (long)delta_y * delta_y)
					))) == 0)
				num_frames = 1;

			TargetVelocity = TargetPtr->velocity;
			GetNextVelocityComponentsSdword (&TargetVelocity,
					&delta_x, &delta_y, num_frames);

			delta_x = ((SDWORD)TargetPtr->current.location.x + (SDWORD)delta_x)
					- (SDWORD)ElementPtr->current.location.x;
			delta_x = WRAP_DELTA_X (delta_x);
			delta_y = ((SDWORD)TargetPtr->current.location.y + (SDWORD)delta_y)
					- (SDWORD)ElementPtr->current.location.y;
			delta_y = WRAP_DELTA_Y (delta_y);

			UnlockElement (hTarget);

			delta_x = NORMALIZE_FACING (
					ANGLE_TO_FACING (ARCTAN (delta_x, delta_y)) - facing
					);

			if (delta_x > 0)
			{
				if (delta_x <= ANGLE_TO_FACING (HALF_CIRCLE))
					++facing;
				else
					--facing;
			}

			SetVelocityVector (&ElementPtr->velocity,
					SENTINEL_SPEED, facing);
		}

		ElementPtr->turn_wait = TRACK_WAIT;
	}
}

static void
sentinel_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	COUNT angle;
	STARSHIP *StarShipPtr;

	if (ElementPtr1->playerNr == NPC_PLAYER_NUM)
	{
		if (ElementPtr0->preprocess_func == ElementPtr1->preprocess_func
				&& !(ElementPtr0->state_flags & DEFY_PHYSICS)
				&& (pPt0->x != ElementPtr0->IntersectControl.IntersectStamp.origin.x
				|| pPt0->y != ElementPtr0->IntersectControl.IntersectStamp.origin.y))
		{
			angle = ARCTAN (pPt0->x - pPt1->x, pPt0->y - pPt1->y);

			SetVelocityComponents (&ElementPtr0->velocity,
					COSINE (angle, WORLD_TO_VELOCITY (SENTINEL_SPEED)),
					SINE (angle, WORLD_TO_VELOCITY (SENTINEL_SPEED)));
			ElementPtr0->turn_wait = TRACK_WAIT;
			ElementPtr0->state_flags |= COLLISION | DEFY_PHYSICS;
		}
	}
	else
	{
		BYTE old_hits;
		COUNT old_life;
		HELEMENT hBlastElement;

		old_hits = ElementPtr0->hit_points;
		old_life = ElementPtr0->life_span;
		ElementPtr0->blast_offset = 0;
		hBlastElement = weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
		ElementPtr0->thrust_wait = 0;

		if ((ElementPtr1->state_flags & PLAYER_SHIP)
				&& ElementPtr1->crew_level
				&& !GRAVITY_MASS (ElementPtr1->mass_points + 1))
		{
			SDWORD cur_delta_x, cur_delta_y;

			ElementPtr0->life_span = old_life;
			ElementPtr0->hit_points = old_hits;
			ElementPtr0->state_flags &= ~DISAPPEARING;
			ElementPtr0->state_flags |= DEFY_PHYSICS;
			ElementPtr0->turn_wait = (ONE_SECOND / BATTLE_FRAME_RATE) >> 1;

			GetElementStarShip (ElementPtr1, &StarShipPtr);
			StarShipPtr->cur_status_flags &=
					~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);
			if (ElementPtr1->turn_wait < COLLISION_TURN_WAIT)
				ElementPtr1->turn_wait += COLLISION_TURN_WAIT;
			if (ElementPtr1->thrust_wait < COLLISION_THRUST_WAIT)
				ElementPtr1->thrust_wait += COLLISION_THRUST_WAIT;

			angle = GetVelocityTravelAngle (&ElementPtr0->velocity);
			DeltaVelocityComponents (&ElementPtr1->velocity,
					COSINE (angle, RECOIL_VELOCITY),
					SINE (angle, RECOIL_VELOCITY));
			GetCurrentVelocityComponentsSdword (&ElementPtr1->velocity,
					&cur_delta_x, &cur_delta_y);
			if ((long)cur_delta_x * (long)cur_delta_x
					+ (long)cur_delta_y * (long)cur_delta_y
					> (long)MAX_RECOIL_VELOCITY * (long)MAX_RECOIL_VELOCITY)
			{
				angle = ARCTAN (cur_delta_x, cur_delta_y);
				SetVelocityComponents (&ElementPtr1->velocity,
						COSINE (angle, MAX_RECOIL_VELOCITY),
						SINE (angle, MAX_RECOIL_VELOCITY));
			}

			ZeroVelocityComponents (&ElementPtr0->velocity);
		}

		if (ElementPtr0->state_flags & DISAPPEARING)
		{
			GetElementStarShip (ElementPtr0, &StarShipPtr);
			--StarShipPtr->RaceDescPtr->characteristics.special_wait;
			if (hBlastElement)
			{
				ELEMENT *BlastElementPtr;

				LockElement (hBlastElement, &BlastElementPtr);
				BlastElementPtr->life_span = 6;
				BlastElementPtr->current.image.frame =
						SetAbsFrameIndex (
						BlastElementPtr->current.image.farray[0], 6
						);
				UnlockElement (hBlastElement);
			}
		}
	}
}

static void
samatra_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void
samatra_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->num_generators)
	{
		if (StarShipPtr->weapon_counter == 0
				&& StarShipPtr->RaceDescPtr->characteristics.weapon_wait < MAX_COMETS
				&& spawn_comet (ElementPtr))
		{
			StarShipPtr->weapon_counter = WEAPON_WAIT;
		}

		if (StarShipPtr->special_counter == 0
				&& StarShipPtr->RaceDescPtr->characteristics.special_wait < MAX_SENTINELS)
		{
			MISSILE_BLOCK MissileBlock;
			HELEMENT hSentinel;

			MissileBlock.cx = ElementPtr->next.location.x;
			MissileBlock.cy = ElementPtr->next.location.y;
			MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
			MissileBlock.face = 0;
			MissileBlock.index = 0;
			MissileBlock.sender = ElementPtr->playerNr;
			MissileBlock.flags = 0;
			MissileBlock.pixoffs = 0;
			MissileBlock.speed = SENTINEL_SPEED;
			MissileBlock.hit_points = SENTINEL_HITS;
			MissileBlock.damage = SENTINEL_DAMAGE;
			MissileBlock.life = SENTINEL_LIFE;
			MissileBlock.preprocess_func = sentinel_preprocess;
			MissileBlock.blast_offs = SENTINEL_OFFSET;
			hSentinel = initialize_missile (&MissileBlock);

			if (hSentinel)
			{
				ELEMENT *SentinelPtr;

				LockElement (hSentinel, &SentinelPtr);
				SentinelPtr->collision_func = sentinel_collision;
				SentinelPtr->turn_wait = TRACK_WAIT + 2;
				SetElementStarShip (SentinelPtr, StarShipPtr);
				UnlockElement (hSentinel);

				StarShipPtr->special_counter = SPECIAL_WAIT;

				PutElement (hSentinel);
			}
		}
	}
}

static void
samatra_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->characteristics.weapon_wait = 0;
	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;
	if (!(ElementPtr->state_flags & APPEARING))
	{
		++ElementPtr->turn_wait;
		++ElementPtr->thrust_wait;
	}
	else
	{
		POINT offs_orig[] =
		{
			{-127-9,  -53+18},
			{ -38-9,  -88+18},
			{  44-9,  -85+18},
			{ 127-9,  -60+18},
			{ 124-9,   28+18},
			{  73-9,   61+18},
			{ -87-9,   58+18},
			{-136-9,   29+18},
		};
		
		POINT offs_hd[] =
		{
			{-305, -234}, // Top left generator
			{-414, -96 }, // The one below the top left generator
			{-396,  140},
			{-208,  262},
			{215,   262},
			{410,   140},
			{441,  -87 },
			{329,  -214}, // Top right generator
		};
		
		POINT *offs;

		offs = RES_BOOL(offs_orig, offs_hd);

		for (StarShipPtr->RaceDescPtr->num_generators = 0;
				StarShipPtr->RaceDescPtr->num_generators < MAX_GENERATORS;
				++StarShipPtr->RaceDescPtr->num_generators)
		{
			HELEMENT hGenerator;

			hGenerator = AllocElement ();
			if (hGenerator)
			{
				ELEMENT *GeneratorPtr;

				LockElement (hGenerator, &GeneratorPtr);
				GeneratorPtr->hit_points = GENERATOR_HITS;
				GeneratorPtr->mass_points = MAX_SHIP_MASS * 10;
				GeneratorPtr->life_span = NORMAL_LIFE;
				GeneratorPtr->playerNr = ElementPtr->playerNr;
				GeneratorPtr->state_flags = APPEARING | IGNORE_SIMILAR;
				SetPrimType (
						&GLOBAL (DisplayArray[GeneratorPtr->PrimIndex]),
						STAMP_PRIM
						);
				GeneratorPtr->current.location.x =
						((LOG_SPACE_WIDTH >> 1)
						+ DISPLAY_TO_WORLD ((offs[StarShipPtr->RaceDescPtr->num_generators].x)))
						& ~((SCALED_ONE << MAX_VIS_REDUCTION) - 1);
				GeneratorPtr->current.location.y =
						((LOG_SPACE_HEIGHT >> 1)
						+ DISPLAY_TO_WORLD ((offs[StarShipPtr->RaceDescPtr->num_generators].y)))
						& ~((SCALED_ONE << MAX_VIS_REDUCTION) - 1);
				GeneratorPtr->current.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.special;
				GeneratorPtr->current.image.frame =
						SetAbsFrameIndex (
								StarShipPtr->RaceDescPtr->ship_data.special[0],
								(BYTE)TFB_Random () % 10
								);

				GeneratorPtr->preprocess_func = generator_preprocess;
				GeneratorPtr->collision_func = generator_collision;
				GeneratorPtr->death_func = generator_death;

				SetElementStarShip (GeneratorPtr, StarShipPtr);
				UnlockElement (hGenerator);

				InsertElement (hGenerator, GetHeadElement ());
			}
		}

		{
			HELEMENT hTurret;

			hTurret = AllocElement ();
			if (hTurret)
			{
				ELEMENT *TurretPtr;

				LockElement (hTurret, &TurretPtr);
				TurretPtr->hit_points = 1;
				TurretPtr->life_span = NORMAL_LIFE;
				TurretPtr->playerNr = ElementPtr->playerNr;
				TurretPtr->state_flags = APPEARING | IGNORE_SIMILAR | NONSOLID;
				SetPrimType (
						&GLOBAL (DisplayArray[TurretPtr->PrimIndex]),
						STAMP_PRIM
						);
				TurretPtr->current.location.x = LOG_SPACE_WIDTH >> 1;
				TurretPtr->current.location.y = LOG_SPACE_HEIGHT >> 1;
				TurretPtr->current.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.ship;
				TurretPtr->current.image.frame =
						SetAbsFrameIndex (
						StarShipPtr->RaceDescPtr->ship_data.ship[0], 1
						);

				TurretPtr->preprocess_func = turret_preprocess;

				SetElementStarShip (TurretPtr, StarShipPtr);
				UnlockElement (hTurret);

				InsertElement (hTurret, GetSuccElement (ElementPtr));
			}
		}

		{
			HELEMENT hGate;

			hGate = AllocElement ();
			if (hGate)
			{
				ELEMENT *GatePtr;

				LockElement (hGate, &GatePtr);
				GatePtr->hit_points = GATE_HITS;
				GatePtr->mass_points = GATE_DAMAGE;
				GatePtr->life_span = 2;
				GatePtr->playerNr = ElementPtr->playerNr;
				GatePtr->state_flags = APPEARING | FINITE_LIFE
						| IGNORE_SIMILAR;
				SetPrimType (
						&GLOBAL (DisplayArray[GatePtr->PrimIndex]),
						STAMP_PRIM
						);
				GatePtr->current.location.x = LOG_SPACE_WIDTH >> 1;
				GatePtr->current.location.y = LOG_SPACE_HEIGHT >> 1;
				GatePtr->current.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.ship;
				GatePtr->current.image.frame =
						SetAbsFrameIndex (
						StarShipPtr->RaceDescPtr->ship_data.ship[0], 11
						);

				GatePtr->preprocess_func = gate_preprocess;
				GatePtr->collision_func = gate_collision;

				SetElementStarShip (GatePtr, StarShipPtr);
				UnlockElement (hGate);

				InsertElement (hGate, GetSuccElement (ElementPtr));
			}
		}

		StarShipPtr->weapon_counter = WEAPON_WAIT >> 1;
		StarShipPtr->special_counter = SPECIAL_WAIT >> 1;
	}
}

RACE_DESC*
init_samatra (void)
{
	RACE_DESC *RaceDescPtr;

	samatra_desc.preprocess_func = samatra_preprocess;
	samatra_desc.postprocess_func = samatra_postprocess;
	samatra_desc.cyborg_control.intelligence_func = samatra_intelligence;

	RaceDescPtr = &samatra_desc;

	return (RaceDescPtr);
}

