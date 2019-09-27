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
#include "orz.h"
#include "resinst.h"
#include "../../setup.h"
#include "uqm/colors.h"
#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 16
#define MAX_ENERGY 20
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 6
#define MAX_THRUST 35
#define THRUST_INCREMENT 5
#define THRUST_WAIT 0
#define TURN_WAIT 1
#define SHIP_MASS 4

// Howitzer
#define WEAPON_ENERGY_COST (MAX_ENERGY / 3)
#define WEAPON_WAIT 4
#define ORZ_OFFSET 9
#define MISSILE_SPEED DISPLAY_TO_WORLD (30)
#define MISSILE_LIFE 12
#define MISSILE_HITS 2
#define MISSILE_DAMAGE 3
#define MISSILE_OFFSET 1

// Marine
#define SPECIAL_ENERGY_COST 0
#define SPECIAL_WAIT 12
#define MARINE_MAX_THRUST RES_SCALE(32)
#define MARINE_THRUST_INCREMENT RES_SCALE(8)
#define MARINE_HIT_POINTS 3
#define MARINE_MASS_POINTS 1
#define MAX_MARINES 8
#define MARINE_WAIT 12
#define ION_LIFE 1
#define START_ION_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)

// Rotating Turret
#define TURRET_OFFSET RES_SCALE(14)
#define TURRET_WAIT 3

// HD
#define MISSILE_SPEED_HD DISPLAY_TO_WORLD (120)

static RACE_DESC orz_desc =
{
	{ /* SHIP_INFO */
		"nemesis",
		FIRES_FORE | SEEKING_SPECIAL,
		23, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		ORZ_RACE_STRINGS,
		ORZ_ICON_MASK_PMAP_ANIM,
		ORZ_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		333 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			3608, 2637,
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
			ORZ_BIG_MASK_PMAP_ANIM,
			ORZ_MED_MASK_PMAP_ANIM,
			ORZ_SML_MASK_PMAP_ANIM,
		},
		{
			HOWITZER_BIG_MASK_PMAP_ANIM,
			HOWITZER_MED_MASK_PMAP_ANIM,
			HOWITZER_SML_MASK_PMAP_ANIM,
		},
		{
			TURRET_BIG_MASK_PMAP_ANIM,
			TURRET_MED_MASK_PMAP_ANIM,
			TURRET_SML_MASK_PMAP_ANIM,
		},
		{
			ORZ_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		ORZ_VICTORY_SONG,
		ORZ_SHIP_SOUNDS,
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

static void
howitzer_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if (!elementsOfSamePlayer (ElementPtr0, ElementPtr1))
		weapon_collision (ElementPtr0, pPt0, ElementPtr1, pPt1);
}

static COUNT
initialize_turret_missile (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	ELEMENT *TurretPtr;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;

	LockElement (GetSuccElement (ShipPtr), &TurretPtr);
	if (TurretPtr->turn_wait == 0
			&& (StarShipPtr->cur_status_flags & SPECIAL)
			&& (StarShipPtr->cur_status_flags & (LEFT | RIGHT)))
	{
		if (StarShipPtr->cur_status_flags & RIGHT)
			++TurretPtr->thrust_wait;
		else
			--TurretPtr->thrust_wait;

		TurretPtr->turn_wait = TURRET_WAIT + 1;
	}
	MissileBlock.face = MissileBlock.index =
			NORMALIZE_FACING (StarShipPtr->ShipFacing
			+ TurretPtr->thrust_wait);
	UnlockElement (GetSuccElement (ShipPtr));

	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = TURRET_OFFSET;
	MissileBlock.speed = RES_SCALE(MISSILE_SPEED);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;
	MissileArray[0] = initialize_missile (&MissileBlock);

	if (MissileArray[0])
	{
		ELEMENT *HowitzerPtr;

		LockElement (MissileArray[0], &HowitzerPtr);
		HowitzerPtr->collision_func = howitzer_collision;
		UnlockElement (MissileArray[0]);
	}

	return (1);
}

static BYTE
count_marines (STARSHIP *StarShipPtr, BOOLEAN FindSpot)
{
	BYTE num_marines, id_use[MAX_MARINES];
	HELEMENT hElement, hNextElement;

	num_marines = MAX_MARINES;
	while (num_marines--)
		id_use[num_marines] = 0;

	num_marines = 0;
	for (hElement = GetTailElement (); hElement; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		hNextElement = GetPredElement (ElementPtr);
		if (ElementPtr->current.image.farray ==
				StarShipPtr->RaceDescPtr->ship_data.special
				&& ElementPtr->life_span
				&& !(ElementPtr->state_flags & (FINITE_LIFE | DISAPPEARING)))
		{
			if (ElementPtr->state_flags & NONSOLID)
			{
				id_use[ElementPtr->turn_wait] = 1;
			}

			if (++num_marines == MAX_MARINES)
			{
				UnlockElement (hElement);
				hNextElement = 0;
			}
		}
		UnlockElement (hElement);
	}

	if (FindSpot)
	{
		num_marines = 0;
		while (id_use[num_marines])
			++num_marines;
	}

	return (num_marines);
}

static void
orz_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	ELEMENT *TurretPtr;
	STARSHIP *StarShipPtr;
	EVALUATE_DESC *lpEvalDesc;

	LockElement (GetSuccElement (ShipPtr), &TurretPtr);

	++TurretPtr->turn_wait;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
	--TurretPtr->turn_wait;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	if (lpEvalDesc->ObjectPtr == 0)
		StarShipPtr->ship_input_state &= ~SPECIAL;
	else if (StarShipPtr->special_counter != 1)
	{
		STARSHIP *EnemyStarShipPtr;

		if (ShipPtr->turn_wait == 0
				&& lpEvalDesc->MoveState == ENTICE
				&& lpEvalDesc->which_turn < 24
				&& (StarShipPtr->cur_status_flags
				& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED))
				&& !(StarShipPtr->ship_input_state & THRUST)
				&& NORMALIZE_ANGLE (
				GetVelocityTravelAngle (&ShipPtr->velocity)
				- ARCTAN (
						lpEvalDesc->ObjectPtr->next.location.x
						- ShipPtr->next.location.x,
						lpEvalDesc->ObjectPtr->next.location.y
						- ShipPtr->next.location.y
				) + (QUADRANT - (OCTANT >> 1))) >=
				((QUADRANT - (OCTANT >> 1)) << 1))
			StarShipPtr->ship_input_state &= ~(LEFT | RIGHT);

		StarShipPtr->ship_input_state &= ~SPECIAL;
		if (ShipPtr->turn_wait == 0
				&& !(StarShipPtr->ship_input_state & (LEFT | RIGHT | WEAPON))
				&& TurretPtr->turn_wait == 0)
		{
			SIZE delta_facing;
			COUNT facing;//, orig_facing;

			facing = NORMALIZE_FACING (StarShipPtr->ShipFacing
					+ TurretPtr->thrust_wait);
			if ((delta_facing = TrackShip (TurretPtr, &facing)) > 0)
			{
				StarShipPtr->ship_input_state |= SPECIAL;
				if (delta_facing == ANGLE_TO_FACING (HALF_CIRCLE))
					delta_facing += (((BYTE)TFB_Random () & 1) << 1) - 1;

				if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
					StarShipPtr->ship_input_state |= RIGHT;
				else
					StarShipPtr->ship_input_state |= LEFT;
			}
		}

		GetElementStarShip (lpEvalDesc->ObjectPtr, &EnemyStarShipPtr);
		if (StarShipPtr->special_counter == 0
				&& !(StarShipPtr->ship_input_state & WEAPON)
				&& StarShipPtr->RaceDescPtr->ship_info.crew_level >
				(BYTE)(StarShipPtr->RaceDescPtr->ship_info.max_crew >> 2)
				&& !(EnemyStarShipPtr->RaceDescPtr->ship_info.ship_flags
				& POINT_DEFENSE)
				&& (MANEUVERABILITY (
						&EnemyStarShipPtr->RaceDescPtr->cyborg_control
						) < RESOLUTION_COMPENSATED(SLOW_SHIP)
				|| lpEvalDesc->which_turn <= 12
				|| count_marines (StarShipPtr, FALSE) < 2))
		{
			StarShipPtr->ship_input_state |= WEAPON | SPECIAL;
		}
	}

	UnlockElement (GetSuccElement (ShipPtr));
}

static void
ion_preprocess (ELEMENT *ElementPtr)
{
	/* Originally, this table also contained the now commented out
	 * entries. It then used some if statements to skip over these.
	 * The current behaviour is the same as the old behavior.
	 */
	static const Color colorTable[] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7a),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7b),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7c),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x00), 0x7d),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7e),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7f),

		//BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x00, 0x00), 0x2a),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2b),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2c),
		BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2d),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2e),
		//BUILD_COLOR (MAKE_RGB15_INIT (0x0B, 0x00, 0x00), 0x2f),
	};
	const size_t colorTabCount = sizeof colorTable / sizeof colorTable[0];

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTabCount)
	{
		ElementPtr->life_span = ElementPtr->thrust_wait;

		SetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
				colorTable[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
	}
}

static void marine_preprocess (ELEMENT *ElementPtr);

void
intruder_preprocess (ELEMENT *ElementPtr)
{
	HELEMENT hElement, hNextElement;
	ELEMENT *ShipPtr;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement (StarShipPtr->hShip, &ShipPtr);
	if (ShipPtr->crew_level == 0
			&& ShipPtr->life_span == 1
			&& (ShipPtr->state_flags & (FINITE_LIFE | NONSOLID)) ==
			(FINITE_LIFE | NONSOLID))
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING;
	}
	UnlockElement (StarShipPtr->hShip);

	if (ElementPtr->thrust_wait)
		--ElementPtr->thrust_wait;

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		LockElement (hElement, &ShipPtr);
		if ((ShipPtr->state_flags & PLAYER_SHIP)
				&& !elementsOfSamePlayer (ShipPtr, ElementPtr))
		{
			STAMP s;

			if (ElementPtr->thrust_wait == MARINE_WAIT)
			{
				--ElementPtr->thrust_wait;

				if (!IS_HD) {
					s.origin.x = RES_SCALE(16) + (ElementPtr->turn_wait & 3) * RES_SCALE(9 + RESOLUTION_FACTOR * 6);
					s.origin.y = RES_SCALE(14) + (ElementPtr->turn_wait >> 2) * RES_SCALE(11 + RESOLUTION_FACTOR * 6);
				} else {
					s.origin.x = RES_SCALE(16 - (RESOLUTION_FACTOR * 3 / 2) + (ElementPtr->turn_wait & 3) * (9 + RESOLUTION_FACTOR * 3 / 2)); // JMS_GFX
					s.origin.y = RES_SCALE(14 + (ElementPtr->turn_wait >> 2) * (11 + RESOLUTION_FACTOR)); // JMS_GFX
				}
				s.frame = SetAbsFrameIndex (ElementPtr->next.image.farray[0], GetFrameCount (ElementPtr->next.image.farray[0]) - 2);

				ModifySilhouette (ShipPtr, &s, 0);
			}

			ElementPtr->next.location = ShipPtr->next.location;

			if (ShipPtr->crew_level == 0
					|| ElementPtr->life_span == 0)
			{
				UnlockElement (hElement);
				hElement = 0;
LeftShip:
				if (!IS_HD) {
					s.origin.x = RES_SCALE(16) + (ElementPtr->turn_wait & 3) * RES_SCALE(9 + RESOLUTION_FACTOR * 6);
					s.origin.y = RES_SCALE(14) + (ElementPtr->turn_wait >> 2) * RES_SCALE(11 + RESOLUTION_FACTOR * 6);
				} else {
					s.origin.x = RES_SCALE(16 - (RESOLUTION_FACTOR * 3 / 2) + (ElementPtr->turn_wait & 3) * (9 + RESOLUTION_FACTOR * 3 / 2)); // JMS_GFX
					s.origin.y = RES_SCALE(14 + (ElementPtr->turn_wait >> 2) * (11 + RESOLUTION_FACTOR)); // JMS_GFX
				}
				s.frame = ElementPtr->next.image.frame;
				ModifySilhouette (ShipPtr, &s, MODIFY_SWAP);
			}
			else if (ElementPtr->thrust_wait == 0)
			{
				BYTE randval;

				ElementPtr->thrust_wait = MARINE_WAIT;

				randval = (BYTE)TFB_Random ();
				if (randval < (0x0100 / 16))
				{
					ElementPtr->life_span = 0;
					ElementPtr->state_flags |= DISAPPEARING;

					ProcessSound (SetAbsSoundIndex (
							StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 4), ElementPtr);
					goto LeftShip;
				}
				else if (randval < (0x0100 / 2 + 0x0100 / 16))
				{
					if (!antiCheat(ElementPtr, TRUE)) {
						if (!DeltaCrew (ShipPtr, -1))
							ShipPtr->life_span = 0;
					}

					++ElementPtr->thrust_wait;
					if (!IS_HD) {
						s.origin.x = RES_SCALE(16) + (ElementPtr->turn_wait & 3) * RES_SCALE(9 + RESOLUTION_FACTOR * 6);
						s.origin.y = RES_SCALE(14) + (ElementPtr->turn_wait >> 2) * RES_SCALE(11 + RESOLUTION_FACTOR * 6);
					} else {
						s.origin.x = RES_SCALE(16 - (RESOLUTION_FACTOR * 3 / 2) + (ElementPtr->turn_wait & 3) * (9 + RESOLUTION_FACTOR * 3 / 2)); // JMS_GFX
						s.origin.y = RES_SCALE(14 + (ElementPtr->turn_wait >> 2) * (11 + RESOLUTION_FACTOR)); // JMS_GFX
					}
					s.frame = SetAbsFrameIndex (ElementPtr->next.image.farray[0], GetFrameCount (ElementPtr->next.image.farray[0]) - 1);

					ModifySilhouette (ShipPtr, &s, 0);
					ProcessSound (SetAbsSoundIndex (
							StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 3), ElementPtr);
				}
			}

			UnlockElement (hElement);
			break;
		}
		hNextElement = GetSuccElement (ShipPtr);
		UnlockElement (hElement);
	}

	if (hElement == 0 && ElementPtr->life_span)
	{
		ElementPtr->state_flags &= ~NONSOLID;
		ElementPtr->state_flags |= CHANGING | CREW_OBJECT;
		SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
				STAMP_PRIM);

		ElementPtr->current.image.frame =
				ElementPtr->next.image.frame =
						SetAbsFrameIndex (
						StarShipPtr->RaceDescPtr->ship_data.special[0], 21);
		ElementPtr->thrust_wait = 0;
		ElementPtr->turn_wait =
				MAKE_BYTE (0, NORMALIZE_FACING ((BYTE)TFB_Random ()));
		ElementPtr->preprocess_func = marine_preprocess;
	}
}

// XXX: merge this with spawn_ion_trail from tactrans.c?
static void
spawn_marine_ion_trail (ELEMENT *ElementPtr, STARSHIP *StarShipPtr,
		COUNT facing)
{
	HELEMENT hIonElement;

	hIonElement = AllocElement ();
	if (hIonElement)
	{
		COUNT angle;
		ELEMENT *IonElementPtr;

		angle = FACING_TO_ANGLE (facing) + HALF_CIRCLE;

		InsertElement (hIonElement, GetHeadElement ());
		LockElement (hIonElement, &IonElementPtr);
		IonElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		IonElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		IonElementPtr->thrust_wait = ION_LIFE;
		IonElementPtr->life_span = IonElementPtr->thrust_wait;
				// When the element "dies", in the death_func
				// 'cycle_ion_trail', it is given new life a number of
				// times, by setting life_span to thrust_wait.
		SetPrimType (&(GLOBAL (DisplayArray))[IonElementPtr->PrimIndex],
				POINT_PRIM); // Actual marine ion trail
		SetPrimColor (&(GLOBAL (DisplayArray))[IonElementPtr->PrimIndex],
				START_ION_COLOR);
		IonElementPtr->colorCycleIndex = 0;
		IonElementPtr->current.location = ElementPtr->current.location;
		IonElementPtr->current.location.x +=
				(COORD)COSINE (angle, DISPLAY_TO_WORLD (RES_SCALE(2)));
		IonElementPtr->current.location.y +=
				(COORD)SINE (angle, DISPLAY_TO_WORLD (RES_SCALE(2)));
		IonElementPtr->death_func = ion_preprocess;

		SetElementStarShip (IonElementPtr, StarShipPtr);

		{
			/* normally done during preprocess, but because
			 * object is being inserted at head rather than
			 * appended after tail it may never get preprocessed.
			 */
			IonElementPtr->next = IonElementPtr->current;
			--IonElementPtr->life_span;
			IonElementPtr->state_flags |= PRE_PROCESS;
		}

		UnlockElement (hIonElement);
	}
}

static void
marine_preprocess (ELEMENT *ElementPtr)
{
	ELEMENT *ShipPtr;
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement (StarShipPtr->hShip, &ShipPtr);
	if (ShipPtr->crew_level == 0
			&& ShipPtr->life_span == 1
			&& (ShipPtr->state_flags & (FINITE_LIFE | NONSOLID)) ==
			(FINITE_LIFE | NONSOLID))
	{
		ElementPtr->life_span = 0;
		ElementPtr->state_flags |= DISAPPEARING | NONSOLID;
		ElementPtr->turn_wait = 1;
	}
	UnlockElement (StarShipPtr->hShip);

	if (LONIBBLE (ElementPtr->turn_wait))
		--ElementPtr->turn_wait;
	else
	{
		COUNT facing, pfacing = 0;
		SDWORD delta_x, delta_y, delta_facing;
		HELEMENT hObject, hNextObject, hTarget;
		ELEMENT *ObjectPtr;

		// XXX: thrust_wait is abused to store marine speed and
		//   gravity well flags
		ElementPtr->thrust_wait &= ~(SHIP_IN_GRAVITY_WELL >> 6);

		hTarget = 0;
		for (hObject = GetHeadElement ();
				hObject; hObject = hNextObject)
		{
			LockElement (hObject, &ObjectPtr);
			hNextObject = GetSuccElement (ObjectPtr);
			if (GRAVITY_MASS (ObjectPtr->mass_points))
			{
				delta_x = ObjectPtr->current.location.x
						- ElementPtr->current.location.x;
				delta_x = WRAP_DELTA_X (delta_x);

				delta_y = ObjectPtr->current.location.y
						- ElementPtr->current.location.y;
				delta_y = WRAP_DELTA_Y (delta_y);
				if ((long)delta_x * delta_x + (long)delta_y * delta_y <=
						(long)(DISPLAY_TO_WORLD (GRAVITY_THRESHOLD)
						* DISPLAY_TO_WORLD (GRAVITY_THRESHOLD)))
				{
					pfacing = ANGLE_TO_FACING (ARCTAN (delta_x, delta_y));
					delta_facing = NORMALIZE_FACING (
							pfacing - ANGLE_TO_FACING (
							GetVelocityTravelAngle (&ElementPtr->velocity))
							+ ANGLE_TO_FACING (OCTANT));
					if (delta_facing <= ANGLE_TO_FACING (QUADRANT))
					{
						hTarget = hObject;
						hNextObject = 0;
					}

					ElementPtr->thrust_wait |= (SHIP_IN_GRAVITY_WELL >> 6);
				}
			}
			else if ((ObjectPtr->state_flags & PLAYER_SHIP)
					&& ObjectPtr->crew_level
					&& !OBJECT_CLOAKED (ObjectPtr))
			{
				if (!elementsOfSamePlayer (ObjectPtr, ElementPtr))
				{
					if (ElementPtr->state_flags & IGNORE_SIMILAR)
						hTarget = hObject;
				}
				else if (hTarget == 0)
					hTarget = hObject;
			}
			UnlockElement (hObject);
		}

		facing = HINIBBLE (ElementPtr->turn_wait);
		if (hTarget == 0)
			delta_facing = -1;
		else
		{
			LockElement (hTarget, &ObjectPtr);
			delta_x = ObjectPtr->current.location.x
					- ElementPtr->current.location.x;
			delta_x = WRAP_DELTA_X (delta_x);
			delta_y = ObjectPtr->current.location.y
					- ElementPtr->current.location.y;
			delta_y = WRAP_DELTA_Y (delta_y);
			if (GRAVITY_MASS (ObjectPtr->mass_points))
			{
				delta_facing = NORMALIZE_FACING (pfacing - facing
						+ ANGLE_TO_FACING (OCTANT));

				if (delta_facing > ANGLE_TO_FACING (QUADRANT))
					delta_facing = 0;
				else
				{
					if (delta_facing == ANGLE_TO_FACING (OCTANT))
						facing += (((SIZE)TFB_Random () & 1) << 1) - 1;
					else if (delta_facing < ANGLE_TO_FACING (OCTANT))
						++facing;
					else
						--facing;
				}
			}
			else
			{
				DWORD num_frames;
				VELOCITY_DESC ShipVelocity;

				if (elementsOfSamePlayer (ObjectPtr, ElementPtr)
						&& (ElementPtr->state_flags & IGNORE_SIMILAR))
				{
					ElementPtr->next.image.frame = SetAbsFrameIndex (
							StarShipPtr->RaceDescPtr->ship_data.special[0],
							21);
					ElementPtr->state_flags &= ~IGNORE_SIMILAR;
					ElementPtr->state_flags |= CHANGING;
				}

				num_frames = RES_DESCALE(WORLD_TO_TURN (
						square_root ((long)delta_x * delta_x
						+ (long)delta_y * delta_y)));

				if (num_frames == 0)
					num_frames = 1;

				ShipVelocity = ObjectPtr->velocity;
				GetNextVelocityComponentsSdword (&ShipVelocity,
						&delta_x, &delta_y, num_frames);

				delta_x = ((SDWORD)ObjectPtr->current.location.x + delta_x)
						- (SDWORD)ElementPtr->current.location.x;
				delta_y = ((SDWORD)ObjectPtr->current.location.y + delta_y)
						- (SDWORD)ElementPtr->current.location.y;

				delta_facing = NORMALIZE_FACING (
						ANGLE_TO_FACING (ARCTAN (delta_x, delta_y)) - facing);

				if (delta_facing > 0)
				{
					if (delta_facing == ANGLE_TO_FACING (HALF_CIRCLE))
						facing += (((BYTE)TFB_Random () & 1) << 1) - 1;
					else if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
						++facing;
					else
						--facing;
				}
			}
			UnlockElement (hTarget);
		}

		ElementPtr->turn_wait = MAKE_BYTE (0, NORMALIZE_FACING (facing));
		if (delta_facing == 0
				 || ((ElementPtr->thrust_wait & (SHIP_BEYOND_MAX_SPEED >> 6))
				 && !(ElementPtr->thrust_wait & (SHIP_IN_GRAVITY_WELL >> 6))))
		{
			STATUS_FLAGS thrust_status;
			COUNT OldFacing;
			STATUS_FLAGS OldStatus;
			COUNT OldIncrement, OldThrust;
			STARSHIP *StarShipPtr;

			GetElementStarShip (ElementPtr, &StarShipPtr);

			// XXX: Hack: abusing the primary STARSHIP struct in order
			//   to call inertial_thrust() for a marine
			OldFacing = StarShipPtr->ShipFacing;
			OldStatus = StarShipPtr->cur_status_flags;
			OldIncrement = StarShipPtr->RaceDescPtr->characteristics.
					thrust_increment;
			OldThrust = StarShipPtr->RaceDescPtr->characteristics.max_thrust;

			StarShipPtr->ShipFacing = facing;
			// XXX: thrust_wait is abused to store marine speed and
			//   gravity well flags
			StarShipPtr->cur_status_flags = ElementPtr->thrust_wait << 6;
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
					MARINE_THRUST_INCREMENT;
			StarShipPtr->RaceDescPtr->characteristics.max_thrust =
					MARINE_MAX_THRUST;

			thrust_status = inertial_thrust (ElementPtr);

			StarShipPtr->RaceDescPtr->characteristics.max_thrust = OldThrust;
			StarShipPtr->RaceDescPtr->characteristics.thrust_increment =
					OldIncrement;
			StarShipPtr->cur_status_flags = OldStatus;
			StarShipPtr->ShipFacing = OldFacing;

			if ((ElementPtr->thrust_wait & (SHIP_IN_GRAVITY_WELL >> 6))
					|| delta_facing
					|| !(thrust_status
					& (SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED)))
			{
				spawn_marine_ion_trail (ElementPtr, StarShipPtr, facing);
			}

			// XXX: thrust_wait is abused to store marine speed and
			//   gravity well flags
			ElementPtr->thrust_wait = (BYTE)(thrust_status >> 6);
		}
	}
}

void
marine_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1) {	
	STAMP s;
	STARSHIP *StarShipPtr;
	GetElementStarShip (ElementPtr0, &StarShipPtr);
	if (ElementPtr0->life_span && !(ElementPtr0->state_flags & (NONSOLID | COLLISION)) && !(ElementPtr1->state_flags & FINITE_LIFE)) {
		if (!elementsOfSamePlayer (ElementPtr0, ElementPtr1)) {
			ElementPtr0->turn_wait = MAKE_BYTE (5, HINIBBLE (ElementPtr0->turn_wait));
			ElementPtr0->thrust_wait &= ~((SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED) >> 6);
			ElementPtr0->state_flags |= COLLISION;
		}
		if (GRAVITY_MASS (ElementPtr1->mass_points)) {
			ElementPtr0->state_flags |= NONSOLID | FINITE_LIFE;
			ElementPtr0->hit_points = 0;
			ElementPtr0->life_span = 0;
		} else if ((ElementPtr1->state_flags & PLAYER_SHIP) && ((ElementPtr1->state_flags & FINITE_LIFE) || ElementPtr1->life_span == NORMAL_LIFE)) {
			ElementPtr1->state_flags &= ~COLLISION;
			if (!(ElementPtr0->state_flags & COLLISION)) {
				DeltaCrew (ElementPtr1, 1);
				ElementPtr0->state_flags |= DISAPPEARING | NONSOLID | FINITE_LIFE;
				ElementPtr0->hit_points = 0;
				ElementPtr0->life_span = 0;
			} else if ((ElementPtr0->state_flags & IGNORE_SIMILAR) && ElementPtr1->crew_level) {
				if (!antiCheat(ElementPtr1, FALSE)) {
					if (!DeltaCrew (ElementPtr1, -1)){
						ElementPtr1->life_span = 0;
					}					
				}
				ElementPtr0->turn_wait = count_marines (StarShipPtr, TRUE);
				ElementPtr0->thrust_wait = MARINE_WAIT;
				ElementPtr0->next.image.frame = SetAbsFrameIndex (ElementPtr0->next.image.farray[0], 22 + ElementPtr0->turn_wait);
				ElementPtr0->state_flags |= NONSOLID;
				ElementPtr0->state_flags &= ~CREW_OBJECT;
				SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr0->PrimIndex], NO_PRIM);
				ElementPtr0->preprocess_func = intruder_preprocess;
				if (!IS_HD) {
					s.origin.x = RES_SCALE(16) + (ElementPtr0->turn_wait & 3) * RES_SCALE(9 + RESOLUTION_FACTOR * 6);
					s.origin.y = RES_SCALE(14) + (ElementPtr0->turn_wait >> 2) * RES_SCALE(11 + RESOLUTION_FACTOR * 6);
				} else {
					s.origin.x = RES_SCALE(16 - (RESOLUTION_FACTOR * 3 / 2) + (ElementPtr0->turn_wait & 3) * (9 + RESOLUTION_FACTOR * 3 / 2)); // JMS_GFX
					s.origin.y = RES_SCALE(14 + (ElementPtr0->turn_wait >> 2) * (11 + RESOLUTION_FACTOR)); // JMS_GFX
				}
				s.frame = ElementPtr0->next.image.frame;
				ModifySilhouette (ElementPtr1, &s, 0);
				ElementPtr0->next.image.frame = SetAbsFrameIndex (ElementPtr0->next.image.farray[0], 22 + ElementPtr0->turn_wait);
				ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr1);
			}
			ElementPtr0->state_flags &= ~COLLISION;
		}			
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
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
turret_postprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->life_span == 0)
	{
		STARSHIP *StarShipPtr;

		SetPrimType (&(GLOBAL (DisplayArray))[
				ElementPtr->PrimIndex], NO_PRIM);

		GetElementStarShip (ElementPtr, &StarShipPtr);
		if (StarShipPtr->hShip)
		{
			COUNT facing;
			HELEMENT hTurret, hSpaceMarine;
			ELEMENT *ShipPtr;

			LockElement (StarShipPtr->hShip, &ShipPtr);
			hTurret = AllocElement ();
			if (hTurret)
			{
				ELEMENT *TurretPtr;

				LockElement (hTurret, &TurretPtr);
				TurretPtr->playerNr = ElementPtr->playerNr;
				TurretPtr->state_flags = FINITE_LIFE | NONSOLID
						| IGNORE_SIMILAR | CHANGING | PRE_PROCESS
						| POST_PROCESS;
				TurretPtr->life_span = 1;
				TurretPtr->current.image = ElementPtr->current.image;
				TurretPtr->current.location = ShipPtr->next.location;
				TurretPtr->turn_wait = ElementPtr->turn_wait;
				TurretPtr->thrust_wait = ElementPtr->thrust_wait;

				if (TurretPtr->turn_wait)
					--TurretPtr->turn_wait;
				else if ((StarShipPtr->cur_status_flags & SPECIAL)
						&& (StarShipPtr->cur_status_flags & (LEFT | RIGHT)))
				{
					if (StarShipPtr->cur_status_flags & RIGHT)
						++TurretPtr->thrust_wait;
					else
						--TurretPtr->thrust_wait;

					TurretPtr->turn_wait = TURRET_WAIT;
				}
				facing = NORMALIZE_FACING (StarShipPtr->ShipFacing
						+ TurretPtr->thrust_wait);
				StarShipPtr->RaceDescPtr->ship_info.ship_flags &=
						~(FIRES_FORE | FIRES_RIGHT | FIRES_AFT | FIRES_LEFT);
				StarShipPtr->RaceDescPtr->ship_info.ship_flags |= FIRES_FORE
						<< (NORMALIZE_FACING (facing + ANGLE_TO_FACING (OCTANT))
						/ ANGLE_TO_FACING (QUADRANT));
				TurretPtr->current.image.frame = SetAbsFrameIndex (
						TurretPtr->current.image.frame, facing);
				facing = FACING_TO_ANGLE (facing);
				if (StarShipPtr->cur_status_flags & WEAPON)
				{
					HELEMENT hTurretEffect;
					ELEMENT *TurretEffectPtr;

					LockElement (GetTailElement (), &TurretEffectPtr);
					if (TurretEffectPtr != ElementPtr
							&& elementsOfSamePlayer (TurretEffectPtr, ElementPtr)
							&& (TurretEffectPtr->state_flags & APPEARING)
							&& GetPrimType (&(GLOBAL (DisplayArray))[
									TurretEffectPtr->PrimIndex
									]) == STAMP_PRIM
							&& (hTurretEffect = AllocElement ()))
					{
						TurretPtr->current.location.x -=
								COSINE (facing, DISPLAY_TO_WORLD (2));
						TurretPtr->current.location.y -=
								SINE (facing, DISPLAY_TO_WORLD (2));

						LockElement (hTurretEffect, &TurretEffectPtr);
						TurretEffectPtr->playerNr = ElementPtr->playerNr;
						TurretEffectPtr->state_flags = FINITE_LIFE
								| NONSOLID | IGNORE_SIMILAR | APPEARING;
						TurretEffectPtr->life_span = 4;

						TurretEffectPtr->current.location.x =
								TurretPtr->current.location.x
								+ COSINE (facing,
								DISPLAY_TO_WORLD (TURRET_OFFSET));
						TurretEffectPtr->current.location.y =
								TurretPtr->current.location.y
								+ SINE (facing,
								DISPLAY_TO_WORLD (TURRET_OFFSET));
						TurretEffectPtr->current.image.farray =
								StarShipPtr->RaceDescPtr->ship_data.special;
						TurretEffectPtr->current.image.frame =
								SetAbsFrameIndex (
								StarShipPtr->RaceDescPtr->ship_data.special[0],
								ANGLE_TO_FACING (FULL_CIRCLE));

						TurretEffectPtr->preprocess_func = animate;

						SetElementStarShip (TurretEffectPtr, StarShipPtr);

						SetPrimType (&(GLOBAL (DisplayArray))[
								TurretEffectPtr->PrimIndex], STAMP_PRIM);

						UnlockElement (hTurretEffect);
						PutElement (hTurretEffect);
					}
					UnlockElement (GetTailElement ());
				}
				TurretPtr->next = TurretPtr->current;

				SetPrimType (&(GLOBAL (DisplayArray))[
						TurretPtr->PrimIndex],
						GetPrimType (&(GLOBAL (DisplayArray))[
						ShipPtr->PrimIndex]));
				SetPrimColor (&(GLOBAL (DisplayArray))[
						TurretPtr->PrimIndex],
						GetPrimColor (&(GLOBAL (DisplayArray))[
						ShipPtr->PrimIndex]));

				TurretPtr->postprocess_func = ElementPtr->postprocess_func;

				SetElementStarShip (TurretPtr, StarShipPtr);

				UnlockElement (hTurret);
				InsertElement (hTurret, GetSuccElement (ElementPtr));
			}

			if (StarShipPtr->special_counter == 0
					&& (StarShipPtr->cur_status_flags & SPECIAL)
					&& (StarShipPtr->cur_status_flags & WEAPON)
					&& ShipPtr->crew_level > 1
					&& count_marines (StarShipPtr, FALSE) < MAX_MARINES
					&& TrackShip (ShipPtr, &facing) >= 0
					&& (hSpaceMarine = AllocElement ()))
			{
				ELEMENT *SpaceMarinePtr;

				LockElement (hSpaceMarine, &SpaceMarinePtr);
				SpaceMarinePtr->playerNr = ElementPtr->playerNr;
				SpaceMarinePtr->state_flags = IGNORE_SIMILAR | APPEARING
						| CREW_OBJECT;
				SpaceMarinePtr->life_span = NORMAL_LIFE;
				SpaceMarinePtr->hit_points = MARINE_HIT_POINTS;
				SpaceMarinePtr->mass_points = MARINE_MASS_POINTS;

				facing = FACING_TO_ANGLE (StarShipPtr->ShipFacing);
				SpaceMarinePtr->current.location.x =
						ShipPtr->current.location.x
						- COSINE (facing, DISPLAY_TO_WORLD (TURRET_OFFSET << ((RESOLUTION_FACTOR + 1)/2))); // JMS_GFX
				SpaceMarinePtr->current.location.y =
						ShipPtr->current.location.y
						- SINE (facing, DISPLAY_TO_WORLD (TURRET_OFFSET << ((RESOLUTION_FACTOR + 1)/2))); // JMS_GFX
				SpaceMarinePtr->current.image.farray =
						StarShipPtr->RaceDescPtr->ship_data.special;
				SpaceMarinePtr->current.image.frame = SetAbsFrameIndex (
						StarShipPtr->RaceDescPtr->ship_data.special[0], 20);

				SpaceMarinePtr->turn_wait =
						MAKE_BYTE (0, NORMALIZE_FACING (
						ANGLE_TO_FACING (facing + HALF_CIRCLE)));
				SpaceMarinePtr->preprocess_func = marine_preprocess;
				SpaceMarinePtr->collision_func = marine_collision;

				SetElementStarShip (SpaceMarinePtr, StarShipPtr);

				SetPrimType (&(GLOBAL (DisplayArray))[
						SpaceMarinePtr->PrimIndex], STAMP_PRIM);

				UnlockElement (hSpaceMarine);
				PutElement (hSpaceMarine);

				if (!antiCheat(ElementPtr, FALSE)) {
					DeltaCrew (ShipPtr, -1);
				}
				ProcessSound (SetAbsSoundIndex (
						StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1),
						SpaceMarinePtr);

				StarShipPtr->special_counter =
						StarShipPtr->RaceDescPtr->characteristics.special_wait;
			}

			UnlockElement (StarShipPtr->hShip);
		}
	}
}

static void
orz_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (!(ElementPtr->state_flags & APPEARING))
	{
		if (((StarShipPtr->cur_status_flags
				| StarShipPtr->old_status_flags) & SPECIAL)
				&& (StarShipPtr->cur_status_flags & (LEFT | RIGHT))
				&& ElementPtr->turn_wait == 0)
		{
			++ElementPtr->turn_wait;
		}

		if ((StarShipPtr->cur_status_flags & SPECIAL)
				&& (StarShipPtr->cur_status_flags & WEAPON)
				&& StarShipPtr->weapon_counter == 0)
		{
			++StarShipPtr->weapon_counter;
		}
	}
	else
	{
		HELEMENT hTurret;

		hTurret = AllocElement ();
		if (hTurret)
		{
			ELEMENT *TurretPtr;

			LockElement (hTurret, &TurretPtr);
			TurretPtr->playerNr = ElementPtr->playerNr;
			TurretPtr->state_flags = FINITE_LIFE | NONSOLID | IGNORE_SIMILAR;
			TurretPtr->life_span = 1;
			TurretPtr->current.image.farray =
					StarShipPtr->RaceDescPtr->ship_data.special;
			TurretPtr->current.image.frame = SetAbsFrameIndex (
					StarShipPtr->RaceDescPtr->ship_data.special[0],
					StarShipPtr->ShipFacing);

			TurretPtr->postprocess_func = turret_postprocess;

			SetElementStarShip (TurretPtr, StarShipPtr);

			UnlockElement (hTurret);
			InsertElement (hTurret, GetSuccElement (ElementPtr));
		}
	}
}

RACE_DESC*
init_orz (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		orz_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		orz_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		orz_desc.cyborg_control.WeaponRange = MISSILE_SPEED_HD * MISSILE_LIFE;
	}

	orz_desc.preprocess_func = orz_preprocess;
	orz_desc.init_weapon_func = initialize_turret_missile;
	orz_desc.cyborg_control.intelligence_func = orz_intelligence;

	RaceDescPtr = &orz_desc;

	return (RaceDescPtr);
}

