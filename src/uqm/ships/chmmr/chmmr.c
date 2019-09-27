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
#include "chmmr.h"
#include "resinst.h"
#include "../../setup.h"
#include "uqm/colors.h"
#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW MAX_CREW_SIZE
#define MAX_ENERGY MAX_ENERGY_SIZE
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 1
#define MAX_THRUST 35
#define THRUST_INCREMENT 7
#define THRUST_WAIT 5
#define TURN_WAIT 3
#define SHIP_MASS 10

// Laser
#define WEAPON_ENERGY_COST 2
#define WEAPON_WAIT 0
#define CHMMR_OFFSET 18
#define LASER_RANGE DISPLAY_TO_WORLD (150)
#define NUM_CYCLES 4

// Tractor Beam
#define SPECIAL_ENERGY_COST 1
#define SPECIAL_WAIT 0
#define NUM_SHADOWS 5

// Satellites
#define NUM_SATELLITES 3
#define SATELLITE_OFFSET DISPLAY_TO_WORLD (RES_SCALE(64))
#define SATELLITE_HITPOINTS 10
#define SATELLITE_MASS 10
#define DEFENSE_RANGE (UWORD)RES_SCALE(64)
#define DEFENSE_WAIT 2

static RACE_DESC chmmr_desc =
{
	{ /* SHIP_INFO */
		"avatar",
		FIRES_FORE | IMMEDIATE_WEAPON | SEEKING_SPECIAL | POINT_DEFENSE,
		30, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		CHMMR_RACE_STRINGS,
		CHMMR_ICON_MASK_PMAP_ANIM,
		CHMMR_MICON_MASK_PMAP_ANIM,
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
			CHMMR_BIG_MASK_PMAP_ANIM,
			CHMMR_MED_MASK_PMAP_ANIM,
			CHMMR_SML_MASK_PMAP_ANIM,
		},
		{
			MUZZLE_BIG_MASK_PMAP_ANIM,
			MUZZLE_MED_MASK_PMAP_ANIM,
			MUZZLE_SML_MASK_PMAP_ANIM,
		},
		{
			SATELLITE_BIG_MASK_PMAP_ANIM,
			SATELLITE_MED_MASK_PMAP_ANIM,
			SATELLITE_SML_MASK_PMAP_ANIM,
		},
		{
			CHMMR_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		CHMMR_VICTORY_SONG,
		CHMMR_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON,
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
laser_death (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->special_counter = ElementPtr->turn_wait;

	if (StarShipPtr->hShip)
	{
		SDWORD dx, dy;
		long dist;
		HELEMENT hIonSpots;
		ELEMENT *ShipPtr;

		LockElement (StarShipPtr->hShip, &ShipPtr);

		dx = ElementPtr->current.location.x
				- ShipPtr->current.location.x;
		dy = ElementPtr->current.location.y
				- ShipPtr->current.location.y;
		if (((BYTE)TFB_Random () & 0x07)
				&& (dist = (long)dx * dx + (long)dy * dy) >=
				(long)DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET + 10)) // JMS_GFX
				* DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET + 10)) // JMS_GFX
				&& (hIonSpots = AllocElement ()))
		{
			COUNT angle, magnitude;
			ELEMENT *IonSpotsPtr;

			LockElement (hIonSpots, &IonSpotsPtr);
			IonSpotsPtr->playerNr = ElementPtr->playerNr;
			IonSpotsPtr->state_flags = FINITE_LIFE | NONSOLID
					| IGNORE_SIMILAR | APPEARING;
			IonSpotsPtr->turn_wait = IonSpotsPtr->next_turn = 0;
			IonSpotsPtr->life_span = RES_BOOL(9, 14);

			angle = ARCTAN (dx, dy);
			magnitude = ((COUNT)TFB_Random ()
					% ((square_root (dist) + 1)
					- DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET + 10)))) // JMS_GFX
					+ DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET + 10)); // JMS_GFX
			IonSpotsPtr->current.location.x =
					ShipPtr->current.location.x
					+ COSINE (angle, magnitude);
			IonSpotsPtr->current.location.y =
					ShipPtr->current.location.y
					+ SINE (angle, magnitude);
			IonSpotsPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.weapon;
			IonSpotsPtr->current.image.frame = SetAbsFrameIndex (
					StarShipPtr->RaceDescPtr->ship_data.weapon[0],
					ANGLE_TO_FACING (FULL_CIRCLE) << 1
					);

			IonSpotsPtr->preprocess_func = animate;

			SetElementStarShip (IonSpotsPtr, StarShipPtr);

			SetPrimType (&(GLOBAL (DisplayArray))[
					IonSpotsPtr->PrimIndex
					], STAMP_PRIM);

			UnlockElement (hIonSpots);
			PutElement (hIonSpots);
		}

		UnlockElement (StarShipPtr->hShip);
	}
}

static COUNT
initialize_megawatt_laser (ELEMENT *ShipPtr, HELEMENT LaserArray[])
{
	RECT r;
	STARSHIP *StarShipPtr;
	LASER_BLOCK LaserBlock;
	static const Color cycle_array[] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7F),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7B),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7F),
	};

	GetElementStarShip (ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing;
	GetFrameRect (SetAbsFrameIndex (
			StarShipPtr->RaceDescPtr->ship_data.weapon[0], LaserBlock.face
			), &r);
	LaserBlock.cx = DISPLAY_ALIGN (ShipPtr->next.location.x)
			+ DISPLAY_TO_WORLD (r.corner.x);
	LaserBlock.cy = DISPLAY_ALIGN (ShipPtr->next.location.y)
			+ DISPLAY_TO_WORLD (r.corner.y);
	LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), RES_SCALE(LASER_RANGE)); // JMS_GFX
	LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), RES_SCALE(LASER_RANGE)); // JMS_GFX
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = IGNORE_SIMILAR;
	LaserBlock.pixoffs = 0;
	LaserBlock.color = cycle_array[StarShipPtr->special_counter];
	LaserArray[0] = initialize_laser (&LaserBlock);

	if (LaserArray[0])
	{
		ELEMENT *LaserPtr;

		LockElement (LaserArray[0], &LaserPtr);

		LaserPtr->mass_points = 2;
		LaserPtr->death_func = laser_death;
		LaserPtr->turn_wait = (BYTE)((StarShipPtr->special_counter + 1)
				% NUM_CYCLES);

		UnlockElement (LaserArray[0]);
	}

	return (1);
}

static void
chmmr_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	StarShipPtr->ship_input_state &= ~SPECIAL;
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr
			&& !(StarShipPtr->ship_input_state & WEAPON)
			&& lpEvalDesc->which_turn > 12
			&& NORMALIZE_ANGLE (
					GetVelocityTravelAngle (&ShipPtr->velocity)
					- (GetVelocityTravelAngle (&lpEvalDesc->ObjectPtr->velocity)
					+ HALF_CIRCLE) + QUADRANT
					) > HALF_CIRCLE)
		StarShipPtr->ship_input_state |= SPECIAL;
}

static void
chmmr_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	GetElementStarShip (ElementPtr, &StarShipPtr);

	if (StarShipPtr->cur_status_flags & WEAPON)
	{
		HELEMENT hMuzzleFlash;
		ELEMENT *MuzzleFlashPtr;

		LockElement (GetTailElement (), &MuzzleFlashPtr);
		if (MuzzleFlashPtr != ElementPtr
				&& elementsOfSamePlayer (MuzzleFlashPtr, ElementPtr)
				&& (MuzzleFlashPtr->state_flags & APPEARING)
				&& GetPrimType (&(GLOBAL (DisplayArray))[
						MuzzleFlashPtr->PrimIndex
						]) == LINE_PRIM
				&& !(StarShipPtr->special_counter & 1)
				&& (hMuzzleFlash = AllocElement ()))
		{
			LockElement (hMuzzleFlash, &MuzzleFlashPtr);
			MuzzleFlashPtr->playerNr = ElementPtr->playerNr;
			MuzzleFlashPtr->state_flags = FINITE_LIFE | NONSOLID
					| IGNORE_SIMILAR | APPEARING;
			MuzzleFlashPtr->life_span = 1;

			MuzzleFlashPtr->current = ElementPtr->next;
			MuzzleFlashPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.weapon;
			MuzzleFlashPtr->current.image.frame = SetAbsFrameIndex (
					StarShipPtr->RaceDescPtr->ship_data.weapon[0],
					StarShipPtr->ShipFacing + ANGLE_TO_FACING (FULL_CIRCLE)
					);

			SetElementStarShip (MuzzleFlashPtr, StarShipPtr);

			SetPrimType (&(GLOBAL (DisplayArray))[
					MuzzleFlashPtr->PrimIndex
					], STAMP_PRIM);

			UnlockElement (hMuzzleFlash);
			PutElement (hMuzzleFlash);
		}
		UnlockElement (GetTailElement ());
	}

	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		COUNT facing;
		ELEMENT *ShipElementPtr;

		LockElement (ElementPtr->hTarget, &ShipElementPtr);
		
		ProcessSound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
				ShipElementPtr);

		UnlockElement (ElementPtr->hTarget);

		facing = 0;
		if (TrackShip (ElementPtr, &facing) >= 0)
		{
			ELEMENT *ShipElementPtr;

			LockElement (ElementPtr->hTarget, &ShipElementPtr);
			if (!GRAVITY_MASS (ShipElementPtr->mass_points + 1))
			{
				SIZE i, dx, dy;
				COUNT angle, magnitude;
				STARSHIP *EnemyStarShipPtr;
				static const SIZE shadow_offs[] =
				{
					DISPLAY_TO_WORLD (8),
					DISPLAY_TO_WORLD (8 + 9),
					DISPLAY_TO_WORLD (8 + 9 + 11),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14),
					DISPLAY_TO_WORLD (8 + 9 + 11 + 14 + 18),
				};
				static const SIZE shadow_offs_hd[] =
				{
					DISPLAY_TO_WORLD (32),
					DISPLAY_TO_WORLD (32 + 36),
					DISPLAY_TO_WORLD (32 + 36 + 44),
					DISPLAY_TO_WORLD (32 + 36 + 44 + 56),
					DISPLAY_TO_WORLD (32 + 36 + 44 + 56 + 72),
				};
				static const Color color_tab[] =
				{
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x10), 0x53),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0E), 0x54),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x0C), 0x55),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x09), 0x56),
					BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x07), 0x57),
				};
				DWORD current_speed, max_speed;

				// calculate tractor beam effect
				angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
				dx = (ElementPtr->next.location.x
						+ COSINE (angle, (RES_SCALE(LASER_RANGE) / 3) // JMS_GFX
						+ DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET)))) // JMS_GFX
						- ShipElementPtr->next.location.x;
				dy = (ElementPtr->next.location.y
						+ SINE (angle, (RES_SCALE(LASER_RANGE) / 3) // JMS_GFX
						+ DISPLAY_TO_WORLD (RES_SCALE(CHMMR_OFFSET)))) // JMS_GFX
						- ShipElementPtr->next.location.y;
				angle = ARCTAN (dx, dy);
				magnitude = WORLD_TO_VELOCITY (RES_SCALE(12)) / ShipElementPtr->mass_points; // JMS_GFX
				DeltaVelocityComponents (&ShipElementPtr->velocity,
						COSINE (angle, magnitude), SINE (angle, magnitude));

				GetCurrentVelocityComponents (&ShipElementPtr->velocity,
						&dx, &dy);
				GetElementStarShip (ShipElementPtr, &EnemyStarShipPtr);

				// set the effected ship's speed flags
				current_speed = VelocitySquared (dx, dy);
				max_speed = VelocitySquared (WORLD_TO_VELOCITY (
						EnemyStarShipPtr->RaceDescPtr->characteristics.max_thrust),
						0);
				EnemyStarShipPtr->cur_status_flags &= ~(SHIP_AT_MAX_SPEED
						| SHIP_BEYOND_MAX_SPEED);
				if (current_speed > max_speed)
					EnemyStarShipPtr->cur_status_flags |= (SHIP_AT_MAX_SPEED
							| SHIP_BEYOND_MAX_SPEED);
				else if (current_speed == max_speed)
					EnemyStarShipPtr->cur_status_flags |= SHIP_AT_MAX_SPEED;

				// add tractor beam graphical effects
				for (i = 0; i < NUM_SHADOWS; ++i)
				{
					HELEMENT hShadow;

					hShadow = AllocElement ();
					if (hShadow)
					{
						ELEMENT *ShadowElementPtr;
						COUNT shadow_magnitude; // JMS_GFX

						// JMS_GFX
						shadow_magnitude = RES_BOOL(shadow_offs[i], shadow_offs_hd[i]);

						LockElement (hShadow, &ShadowElementPtr);
						ShadowElementPtr->playerNr = ShipElementPtr->playerNr;
						ShadowElementPtr->state_flags = FINITE_LIFE | NONSOLID
								| IGNORE_SIMILAR | POST_PROCESS;
						ShadowElementPtr->life_span = 1;

						ShadowElementPtr->current = ShipElementPtr->next;
						ShadowElementPtr->current.location.x +=
								COSINE (angle, shadow_magnitude);
						ShadowElementPtr->current.location.y +=
								SINE (angle, shadow_magnitude);
						ShadowElementPtr->next = ShadowElementPtr->current;

						SetElementStarShip (ShadowElementPtr, EnemyStarShipPtr);
						SetVelocityComponents (&ShadowElementPtr->velocity,
								dx, dy);

						SetPrimType (&(GLOBAL (DisplayArray))[
								ShadowElementPtr->PrimIndex
								], STAMPFILL_PRIM);
						SetPrimColor (&(GLOBAL (DisplayArray))[
								ShadowElementPtr->PrimIndex
								], color_tab[i]);

						UnlockElement (hShadow);
						InsertElement (hShadow, GetHeadElement ());
					}
				}
			}
			UnlockElement (ElementPtr->hTarget);
		}
	}

	StarShipPtr->special_counter = 0;
}

static void
satellite_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	++ElementPtr->life_span;

	ElementPtr->next.image.frame =
			SetAbsFrameIndex (ElementPtr->current.image.frame,
			(GetFrameIndex (ElementPtr->current.image.frame) + 1) & 7);
	ElementPtr->state_flags |= CHANGING;

	ElementPtr->turn_wait = (BYTE)NORMALIZE_ANGLE (
			ElementPtr->turn_wait + 1
			);

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		SDWORD dx, dy;
		ELEMENT *ShipPtr;

		StarShipPtr->RaceDescPtr->ship_info.ship_flags |= POINT_DEFENSE;

		LockElement (StarShipPtr->hShip, &ShipPtr);

		dx = (ShipPtr->next.location.x
				+ COSINE (ElementPtr->turn_wait, SATELLITE_OFFSET))
				- ElementPtr->current.location.x;
		dy = (ShipPtr->next.location.y
				+ SINE (ElementPtr->turn_wait, SATELLITE_OFFSET))
				- ElementPtr->current.location.y;
		dx = WRAP_DELTA_X (dx);
		dy = WRAP_DELTA_Y (dy);
		if ((long)dx * dx + (long)dy * dy
				<= DISPLAY_TO_WORLD (RES_SCALE(20L)) * DISPLAY_TO_WORLD (RES_SCALE(20L)))
			SetVelocityComponents (&ElementPtr->velocity,
					WORLD_TO_VELOCITY (dx),
					WORLD_TO_VELOCITY (dy));
		else
		{
			COUNT angle;

			angle = ARCTAN (dx, dy);
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE(20)))),
					SINE (angle, WORLD_TO_VELOCITY (DISPLAY_TO_WORLD (RES_SCALE(20)))));
		}

		UnlockElement (StarShipPtr->hShip);
	}
}

static void
spawn_point_defense (ELEMENT *ElementPtr)
{
	BYTE weakest;
	UWORD best_dist;
	STARSHIP *StarShipPtr;
	HELEMENT hObject, hNextObject, hBestObject;
	ELEMENT *ShipPtr;
	ELEMENT *SattPtr;
	ELEMENT *ObjectPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	hBestObject = 0;
	best_dist = DEFENSE_RANGE + RES_SCALE(1);
	weakest = 255;
	LockElement (StarShipPtr->hShip, &ShipPtr);
	LockElement (ElementPtr->hTarget, &SattPtr);
	for (hObject = GetPredElement (ElementPtr);
			hObject; hObject = hNextObject)
	{
		LockElement (hObject, &ObjectPtr);
		hNextObject = GetPredElement (ObjectPtr);
		if (!elementsOfSamePlayer (ObjectPtr, ShipPtr)
				&& ObjectPtr->playerNr != NEUTRAL_PLAYER_NUM
				&& CollisionPossible (ObjectPtr, ShipPtr)
				&& !OBJECT_CLOAKED (ObjectPtr))
		{
			SDWORD delta_x, delta_y;
			UWORD dist;

			delta_x = ObjectPtr->next.location.x
					- SattPtr->next.location.x;
			delta_y = ObjectPtr->next.location.y
					- SattPtr->next.location.y;
			if (delta_x < 0)
				delta_x = -delta_x;
			if (delta_y < 0)
				delta_y = -delta_y;
			delta_x = WORLD_TO_DISPLAY (delta_x);
			delta_y = WORLD_TO_DISPLAY (delta_y);
			if ((UWORD)delta_x <= DEFENSE_RANGE &&
					(UWORD)delta_y <= DEFENSE_RANGE &&
					(dist =
					(UWORD)delta_x * (UWORD)delta_x
					+ (UWORD)delta_y * (UWORD)delta_y) <=
					DEFENSE_RANGE * DEFENSE_RANGE
					&& (ObjectPtr->hit_points < weakest
					|| (ObjectPtr->hit_points == weakest
					&& dist < best_dist)))
			{
				hBestObject = hObject;
				best_dist = dist;
				weakest = ObjectPtr->hit_points;
			}
		}
		UnlockElement (hObject);
	}

	if (hBestObject)
	{
		LASER_BLOCK LaserBlock;
		HELEMENT hPointDefense;

		LockElement (hBestObject, &ObjectPtr);

		LaserBlock.cx = SattPtr->next.location.x;
		LaserBlock.cy = SattPtr->next.location.y;
		LaserBlock.face = 0;
		LaserBlock.ex = ObjectPtr->next.location.x
				- SattPtr->next.location.x;
		LaserBlock.ey = ObjectPtr->next.location.y
				- SattPtr->next.location.y;
		LaserBlock.sender = SattPtr->playerNr;
		LaserBlock.flags = IGNORE_SIMILAR;
		LaserBlock.pixoffs = 0;
		LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x00, 0x01, 0x1F), 0x4D);
		hPointDefense = initialize_laser (&LaserBlock);
		if (hPointDefense)
		{
			ELEMENT *PDPtr;

			LockElement (hPointDefense, &PDPtr);
			SetElementStarShip (PDPtr, StarShipPtr);
			PDPtr->hTarget = 0;
			UnlockElement (hPointDefense);

			PutElement (hPointDefense);

			SattPtr->thrust_wait = DEFENSE_WAIT;
		}

		UnlockElement (hBestObject);
	}

	UnlockElement (ElementPtr->hTarget);
	UnlockElement (StarShipPtr->hShip);
}

static void
satellite_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	if (ElementPtr->thrust_wait || ElementPtr->life_span == 0)
		--ElementPtr->thrust_wait;
	else
	{
		HELEMENT hDefense;

		hDefense = AllocElement ();
		if (hDefense)
		{
			ELEMENT *DefensePtr;
			
			PutElement (hDefense);

			LockElement (hDefense, &DefensePtr);
			DefensePtr->playerNr = ElementPtr->playerNr;
			DefensePtr->state_flags = APPEARING | NONSOLID | FINITE_LIFE;

			{
				ELEMENT *SuccPtr;

				LockElement (GetSuccElement (ElementPtr), &SuccPtr);
				DefensePtr->hTarget = GetPredElement (SuccPtr);
				UnlockElement (GetSuccElement (ElementPtr));

				DefensePtr->death_func = spawn_point_defense;
			}

			GetElementStarShip (ElementPtr, &StarShipPtr);
			SetElementStarShip (DefensePtr, StarShipPtr);
			
			UnlockElement (hDefense);
		}
	}
}

static void
satellite_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	(void) ElementPtr0;  /* Satisfying compiler (unused parameter) */
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) ElementPtr1;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
satellite_death (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->RaceDescPtr->ship_info.ship_flags &= ~POINT_DEFENSE;

	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
	ElementPtr->life_span = 4;
	ElementPtr->turn_wait = 1;
	ElementPtr->next_turn = 0;
	ElementPtr->next.image.frame =
			SetAbsFrameIndex (ElementPtr->current.image.frame, 8);

	ElementPtr->preprocess_func = animate;
	ElementPtr->death_func = NULL;
	ElementPtr->postprocess_func = NULL;
	ElementPtr->collision_func = NULL;
}

static void
spawn_satellites (ELEMENT *ElementPtr)
{
	COUNT i;
	STARSHIP *StarShipPtr;
	BYTE NumSatellites = NUM_SATELLITES;

	if (antiCheat(ElementPtr, FALSE)) {
		NumSatellites = NUM_SATELLITES + 2;
	}

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->hShip)
	{
		LockElement (StarShipPtr->hShip, &ElementPtr);
		for (i = 0; i < NumSatellites; ++i)
		{
			HELEMENT hSatellite;

			hSatellite = AllocElement ();
			if (hSatellite)
			{
				COUNT angle;
				ELEMENT *SattPtr;

				LockElement (hSatellite, &SattPtr);
				SattPtr->playerNr = ElementPtr->playerNr;
				SattPtr->state_flags = IGNORE_SIMILAR | APPEARING
						| FINITE_LIFE;
				SattPtr->life_span = NORMAL_LIFE + 1;
				SattPtr->hit_points = SATELLITE_HITPOINTS;
				SattPtr->mass_points = SATELLITE_MASS;

				angle = (i * FULL_CIRCLE + (NumSatellites >> 1))
						/ NumSatellites;
				SattPtr->turn_wait = (BYTE)angle;
				SattPtr->current.location.x = ElementPtr->next.location.x
						+ COSINE (angle, SATELLITE_OFFSET);
				SattPtr->current.location.y = ElementPtr->next.location.y
						+ SINE (angle, SATELLITE_OFFSET);
				SattPtr->current.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.special;
				SattPtr->current.image.frame = SetAbsFrameIndex (
						StarShipPtr->RaceDescPtr->ship_data.special[0],
						(COUNT)TFB_Random () & 0x07
						);

				SattPtr->preprocess_func = satellite_preprocess;
				SattPtr->postprocess_func = satellite_postprocess;
				SattPtr->death_func = satellite_death;
				SattPtr->collision_func = satellite_collision;

				SetElementStarShip (SattPtr, StarShipPtr);

				SetPrimType (&(GLOBAL (DisplayArray))[
						SattPtr->PrimIndex
						], STAMP_PRIM);

				UnlockElement (hSatellite);
				PutElement (hSatellite);
			}
		}
		UnlockElement (StarShipPtr->hShip);
	}
}

static void
chmmr_preprocess (ELEMENT *ElementPtr)
{
	HELEMENT hSatellite;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	hSatellite = AllocElement ();
	if (hSatellite)
	{
		ELEMENT *SattPtr;
		STARSHIP *StarShipPtr;

		LockElement (hSatellite, &SattPtr);
		SattPtr->playerNr = ElementPtr->playerNr;
		SattPtr->state_flags = FINITE_LIFE | NONSOLID | IGNORE_SIMILAR
				| APPEARING;
		SattPtr->life_span = HYPERJUMP_LIFE + 1;

		SattPtr->death_func = spawn_satellites;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		SetElementStarShip (SattPtr, StarShipPtr);

		SetPrimType (&(GLOBAL (DisplayArray))[
				SattPtr->PrimIndex
				], NO_PRIM);

		UnlockElement (hSatellite);
		PutElement (hSatellite);
	}

	StarShipPtr->RaceDescPtr->preprocess_func = 0;
}

RACE_DESC*
init_chmmr (void)
{
	RACE_DESC *RaceDescPtr;	

	if (IS_HD) {
		chmmr_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		chmmr_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		chmmr_desc.cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON_HD;
	}

	chmmr_desc.preprocess_func = chmmr_preprocess;
	chmmr_desc.postprocess_func = chmmr_postprocess;
	chmmr_desc.init_weapon_func = initialize_megawatt_laser;
	chmmr_desc.cyborg_control.intelligence_func = chmmr_intelligence;

	RaceDescPtr = &chmmr_desc;

	return (RaceDescPtr);
}

