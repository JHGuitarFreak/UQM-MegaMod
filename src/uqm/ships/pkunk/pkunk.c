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
#include "pkunk.h"
#include "resinst.h"
#include "uqm/globdata.h"
#include "uqm/tactrans.h"
#include "libs/mathlib.h"
#include "../../settings.h" // JMS: For StopMusic

// Core characteristics
#define MAX_CREW 8
#define MAX_ENERGY 12
#define ENERGY_REGENERATION 0
#define ENERGY_WAIT 0
#define MAX_THRUST 64
#define THRUST_INCREMENT 16
#define THRUST_WAIT 0
#define TURN_WAIT 0
#define SHIP_MASS 1

// Triple Miniguns
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define PKUNK_OFFSET RES_SCALE(15)
#define MISSILE_OFFSET RES_SCALE(1)
#define MISSILE_SPEED DISPLAY_TO_WORLD (24)
#define MISSILE_LIFE 5
#define MISSILE_HITS 1
#define MISSILE_DAMAGE 1

// Taunt
#define SPECIAL_ENERGY_COST 2
#define SPECIAL_WAIT 16

// Respawn
#define PHOENIX_LIFE 12
#define START_PHOENIX_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)
#define TRANSITION_LIFE 1
#define TRANSITION_SPEED DISPLAY_TO_WORLD (RES_SCALE(20))

static RACE_DESC pkunk_desc =
{
	{ /* SHIP_INFO */
		"fury",
		FIRES_FORE | FIRES_LEFT | FIRES_RIGHT,
		20, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		PKUNK_RACE_STRINGS,
		PKUNK_ICON_MASK_PMAP_ANIM,
		PKUNK_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		666 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			502, 401,
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
		0, /* SPECIAL_WAIT */
		SHIP_MASS,
	},
	{
		{
			PKUNK_BIG_MASK_PMAP_ANIM,
			PKUNK_MED_MASK_PMAP_ANIM,
			PKUNK_SML_MASK_PMAP_ANIM,
		},
		{
			BUG_BIG_MASK_PMAP_ANIM,
			BUG_MED_MASK_PMAP_ANIM,
			BUG_SML_MASK_PMAP_ANIM,
		},
		{
			NULL_RESOURCE,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			PKUNK_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		PKUNK_VICTORY_SONG,
		PKUNK_SHIP_SOUNDS,
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		{ NULL, NULL, NULL },
		NULL, NULL
	},
	{
		0,
		CLOSE_RANGE_WEAPON + 1,
		NULL,
	},
	(UNINIT_FUNC *) NULL,
	(PREPROCESS_FUNC *) NULL,
	(POSTPROCESS_FUNC *) NULL,
	(INIT_WEAPON_FUNC *) NULL,
	0,
	0, /* CodeRef */
};

// Private per-instance ship data
typedef struct
{
	HELEMENT hPhoenix;
	ElementProcessFunc *saved_preprocess_func;
	ElementProcessFunc *saved_postprocess_func;
	ElementProcessFunc *saved_death_func;
	
} PKUNK_DATA;

// Local typedef
typedef PKUNK_DATA CustomShipData_t;

// Retrieve race-specific ship data from a race desc
static CustomShipData_t *
GetCustomShipData (RACE_DESC *pRaceDesc)
{
	return pRaceDesc->data;
}

// Set the race-specific data in a race desc
// (Re)Allocates its own storage for the data.
static void
SetCustomShipData (RACE_DESC *pRaceDesc, const CustomShipData_t *data)
{
	if (pRaceDesc->data == data) 
		return;  // no-op

	if (pRaceDesc->data) // Out with the old
	{
		HFree (pRaceDesc->data);
		pRaceDesc->data = NULL;
	}

	if (data) // In with the new
	{
		CustomShipData_t* newData = HMalloc (sizeof (*data));
		*newData = *data;
		pRaceDesc->data = newData;
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

static COUNT
initialize_bug_missile (ELEMENT *ShipPtr, HELEMENT MissileArray[])
{
	COUNT i;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	MissileBlock.cx = ShipPtr->next.location.x;
	MissileBlock.cy = ShipPtr->next.location.y;
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.weapon;
	MissileBlock.index = 0;
	MissileBlock.sender = ShipPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = PKUNK_OFFSET;
	MissileBlock.speed = RES_SCALE(MISSILE_SPEED);
	MissileBlock.hit_points = MISSILE_HITS;
	MissileBlock.damage = MISSILE_DAMAGE;
	MissileBlock.life = MISSILE_LIFE;
	MissileBlock.preprocess_func = NULL;
	MissileBlock.blast_offs = MISSILE_OFFSET;

	for (i = 0; i < 3; ++i)
	{
		MissileBlock.face =
				StarShipPtr->ShipFacing
				+ (ANGLE_TO_FACING (QUADRANT) * i);
		if (i == 2)
			MissileBlock.face += ANGLE_TO_FACING (QUADRANT);
		MissileBlock.face = NORMALIZE_FACING (MissileBlock.face);

		if ((MissileArray[i] = initialize_missile (&MissileBlock)))
		{
			SDWORD dx, dy;
			ELEMENT *MissilePtr;

			LockElement (MissileArray[i], &MissilePtr);
			GetCurrentVelocityComponentsSdword (&ShipPtr->velocity, &dx, &dy);
			DeltaVelocityComponents (&MissilePtr->velocity, dx, dy);
			MissilePtr->current.location.x -= VELOCITY_TO_WORLD (dx);
			MissilePtr->current.location.y -= VELOCITY_TO_WORLD (dy);

			MissilePtr->preprocess_func = animate;
			UnlockElement (MissileArray[i]);
		}
	}

	return (3);
}

static void
pkunk_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	STARSHIP *StarShipPtr;
	PKUNK_DATA *PkunkData;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	PkunkData = GetCustomShipData (StarShipPtr->RaceDescPtr);
	if (PkunkData->hPhoenix && (StarShipPtr->control & STANDARD_RATING))
	{
		RemoveElement (PkunkData->hPhoenix);
		FreeElement (PkunkData->hPhoenix);
		PkunkData->hPhoenix = 0;
	}

	if (StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy
			&& (StarShipPtr->special_counter == 0
			|| (BYTE)TFB_Random () < 20))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
	ship_intelligence (ShipPtr, ObjectsOfConcern, ConcernCounter);
}

static void pkunk_preprocess (ELEMENT *ElementPtr);

static void
new_pkunk (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	PKUNK_DATA *PkunkData;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	PkunkData = GetCustomShipData (StarShipPtr->RaceDescPtr);

	ElementPtr->state_flags = APPEARING | PLAYER_SHIP | IGNORE_SIMILAR;
	ElementPtr->mass_points = SHIP_MASS;
	// Restore the element processing callbacks after the explosion.
	// The callbacks were changed for the explosion sequence
	ElementPtr->preprocess_func = PkunkData->saved_preprocess_func;
	ElementPtr->postprocess_func = PkunkData->saved_postprocess_func;
	ElementPtr->death_func = PkunkData->saved_death_func;
	// preprocess_func() is called during the phoenix transition and
	// then cleared, so we need to restore it
	StarShipPtr->RaceDescPtr->preprocess_func = pkunk_preprocess;
	StarShipPtr->RaceDescPtr->ship_info.crew_level = MAX_CREW;
	StarShipPtr->RaceDescPtr->ship_info.energy_level = MAX_ENERGY;
				/* fix vux impairment */
	StarShipPtr->RaceDescPtr->characteristics.max_thrust = RES_SCALE(MAX_THRUST);
	StarShipPtr->RaceDescPtr->characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
	StarShipPtr->RaceDescPtr->characteristics.turn_wait = TURN_WAIT;
	StarShipPtr->RaceDescPtr->characteristics.thrust_wait = THRUST_WAIT;
	StarShipPtr->RaceDescPtr->characteristics.special_wait = 0;

	StarShipPtr->ship_input_state = 0;
	// Pkunk wins in a simultaneous destruction if it reincarnates
	StarShipPtr->cur_status_flags &= PLAY_VICTORY_DITTY;
	StarShipPtr->old_status_flags = 0;
	StarShipPtr->energy_counter = 0;
	StarShipPtr->weapon_counter = 0;
	StarShipPtr->special_counter = 0;
	ElementPtr->crew_level = 0;
	ElementPtr->turn_wait = 0;
	ElementPtr->thrust_wait = 0;
	ElementPtr->life_span = NORMAL_LIFE;

	StarShipPtr->ShipFacing = NORMALIZE_FACING (TFB_Random ());
	ElementPtr->current.image.farray = StarShipPtr->RaceDescPtr->ship_data.ship;
	ElementPtr->current.image.frame = SetAbsFrameIndex (
			StarShipPtr->RaceDescPtr->ship_data.ship[0],
			StarShipPtr->ShipFacing);
	SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex], STAMP_PRIM);

	do
	{
		ElementPtr->current.location.x =
				WRAP_X (DISPLAY_ALIGN_X (TFB_Random ()));
		ElementPtr->current.location.y =
				WRAP_Y (DISPLAY_ALIGN_Y (TFB_Random ()));
	} while (CalculateGravity (ElementPtr)
			|| TimeSpaceMatterConflict (ElementPtr));

	// XXX: Hack: Set hTarget!=0 so that ship_preprocess() does not
	//   call ship_transition() for us.
	ElementPtr->hTarget = StarShipPtr->hShip;
}

// This function is called when the ship dies but reincarnates.
// The generic ship_death() function is not called for the ship in this case.
static void
pkunk_reincarnation_death (ELEMENT *ShipPtr)
{
	// Simulate ship death
	StopAllBattleMusic ();
	StartShipExplosion (ShipPtr, true);
	// Once the explosion ends, we will get a brand new ship
	ShipPtr->death_func = new_pkunk;
}

static void
intercept_pkunk_death (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	PKUNK_DATA *PkunkData;
	ELEMENT *ShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	PkunkData = GetCustomShipData (StarShipPtr->RaceDescPtr);
	
	if (StarShipPtr->RaceDescPtr->ship_info.crew_level != 0)
	{	// Ship not dead yet.
		// Keep the Phoenix element alive.
		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->life_span = 1;
		return;
	}

	LockElement (StarShipPtr->hShip, &ShipPtr);
	// GRAVITY_MASS() indicates a warp-out here. If Pkunk dies while warping
	// out, there is no reincarnation.
	if (!GRAVITY_MASS (ShipPtr->mass_points + 1))
	{
		// XXX: Hack: Set mass_points to indicate a reincarnation to
		//   FindAliveStarShip()
		ShipPtr->mass_points = MAX_SHIP_MASS + 1;
		// Save the various element processing callbacks before the
		// explosion happens, because we were not the ones who set
		// these callbacks and they are about to be changed.
		PkunkData->saved_preprocess_func = ShipPtr->preprocess_func;
		PkunkData->saved_postprocess_func = ShipPtr->postprocess_func;
		PkunkData->saved_death_func = ShipPtr->death_func;

		ShipPtr->death_func = pkunk_reincarnation_death;
	}
	UnlockElement (StarShipPtr->hShip);
}

static void
spawn_phoenix_trail (ELEMENT *ElementPtr)
{
	static const Color colorTable[] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7a),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7b),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7c),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x00), 0x7d),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7e),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7f),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x00, 0x00), 0x2a),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2b),
		BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2c),
		BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2d),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2e),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0B, 0x00, 0x00), 0x2f),
	};
	const size_t colorTableCount = sizeof colorTable / sizeof colorTable[0];
	
	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTableCount)
	{
		ElementPtr->life_span = TRANSITION_LIFE;

		SetPrimColor (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
				colorTable[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
	} // else, the element disappears.
}

static void
phoenix_transition (ELEMENT *ElementPtr)
{
	HELEMENT hShipImage;
	ELEMENT *ShipImagePtr;
	STARSHIP *StarShipPtr;
	
	GetElementStarShip (ElementPtr, &StarShipPtr);
	LockElement (StarShipPtr->hShip, &ShipImagePtr);

	if (!(ShipImagePtr->state_flags & NONSOLID))
	{
		ElementPtr->preprocess_func = NULL;
	}
	else if ((hShipImage = AllocElement ()))
	{
		COUNT angle;

		PutElement (hShipImage);

		LockElement (hShipImage, &ShipImagePtr);
		ShipImagePtr->playerNr = NEUTRAL_PLAYER_NUM;
		ShipImagePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		ShipImagePtr->life_span = TRANSITION_LIFE;
		SetPrimType (&(GLOBAL (DisplayArray))[ShipImagePtr->PrimIndex],
				STAMPFILL_PRIM);
		SetPrimColor (
				&(GLOBAL (DisplayArray))[ShipImagePtr->PrimIndex],
				START_PHOENIX_COLOR);
		ShipImagePtr->colorCycleIndex = 0;
		ShipImagePtr->current.image = ElementPtr->current.image;
		ShipImagePtr->current.location = ElementPtr->current.location;
		if (!(ElementPtr->state_flags & PLAYER_SHIP))
		{
			angle = ElementPtr->mass_points;

			ShipImagePtr->current.location.x +=
					COSINE (angle, TRANSITION_SPEED);
			ShipImagePtr->current.location.y +=
					SINE (angle, TRANSITION_SPEED);
			ElementPtr->preprocess_func = NULL;
		}
		else
		{
			SDWORD temp_x, temp_y;
			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);

            // JMS_GFX: Circumventing overflows by using temp variables instead of
            // subtracting straight from the POINT sized ShipImagePtr->current.location.
            temp_x = (SDWORD)ShipImagePtr->current.location.x - 
				COSINE (angle, TRANSITION_SPEED) * (ElementPtr->life_span - 1);
            temp_y = (SDWORD)ShipImagePtr->current.location.y - 
				SINE (angle, TRANSITION_SPEED) * (ElementPtr->life_span - 1);
            
            ShipImagePtr->current.location.x = WRAP_X (temp_x);
            ShipImagePtr->current.location.y = WRAP_Y (temp_y);
		}

		ShipImagePtr->mass_points = (BYTE)angle;
		ShipImagePtr->preprocess_func = phoenix_transition;
		ShipImagePtr->death_func = spawn_phoenix_trail;
		SetElementStarShip (ShipImagePtr, StarShipPtr);

		UnlockElement (hShipImage);
	}

	UnlockElement (StarShipPtr->hShip);
}

static void
pkunk_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;
	PKUNK_DATA *PkunkData;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	PkunkData = GetCustomShipData (StarShipPtr->RaceDescPtr);
	if (ElementPtr->state_flags & APPEARING)
	{
		HELEMENT hPhoenix = 0;

		if (TFB_Random () & 1)
			hPhoenix = AllocElement ();

		// The hPhoenix element is created and placed at the head of the
		// queue so that it is preprocessed before any of the ships' elements
		// are, and so before death_func() is called for the dead Pkunk.
		// hPhoenix detects when the Pkunk ship dies and tweaks the ship,
		// starting the death + reincarnation sequence.
		if (hPhoenix)
		{
			ELEMENT *PhoenixPtr;

			LockElement (hPhoenix, &PhoenixPtr);
			PhoenixPtr->playerNr = ElementPtr->playerNr;
			PhoenixPtr->state_flags = FINITE_LIFE | NONSOLID | IGNORE_SIMILAR;
			PhoenixPtr->life_span = 1;

			PhoenixPtr->death_func = intercept_pkunk_death;

			SetElementStarShip (PhoenixPtr, StarShipPtr);

			UnlockElement (hPhoenix);
			InsertElement (hPhoenix, GetHeadElement ());
		}
		PkunkData->hPhoenix = hPhoenix;

		// XXX: Hack: new_pkunk() sets hTarget!=0 which indicates a
		//   reincarnation to us.
		if (ElementPtr->hTarget == 0)
		{
			// A brand new ship is preprocessed only once
			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		}
		else
		{	// Start the reincarnation sequence
			COUNT angle, facing;

			ProcessSound (SetAbsSoundIndex (
					StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1
					), ElementPtr);

			ElementPtr->life_span = PHOENIX_LIFE;
			SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;

			facing = StarShipPtr->ShipFacing;
			for (angle = OCTANT; angle < FULL_CIRCLE; angle += QUADRANT)
			{
				StarShipPtr->ShipFacing = NORMALIZE_FACING (
						facing + ANGLE_TO_FACING (angle)
						);
				phoenix_transition (ElementPtr);
			}
			StarShipPtr->ShipFacing = facing;
		}
	}

	if (StarShipPtr->RaceDescPtr->preprocess_func)
	{
		StarShipPtr->cur_status_flags &=
				~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);

		if (ElementPtr->life_span == NORMAL_LIFE)
		{
			ElementPtr->current.image.frame =
					ElementPtr->next.image.frame =
					SetEquFrameIndex (
					ElementPtr->current.image.farray[0],
					ElementPtr->current.image.frame);
			SetPrimType (&(GLOBAL (DisplayArray))[ElementPtr->PrimIndex],
					STAMP_PRIM);
			InitIntersectStartPoint (ElementPtr);
			InitIntersectEndPoint (ElementPtr);
			InitIntersectFrame (ElementPtr);
			ZeroVelocityComponents (&ElementPtr->velocity);
			ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
			ElementPtr->state_flags |= CHANGING;

			StarShipPtr->RaceDescPtr->preprocess_func = 0;
		}
	}
}
		
static COUNT LastSound = 0;

static void
pkunk_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if (StarShipPtr->RaceDescPtr->characteristics.special_wait)
		--StarShipPtr->RaceDescPtr->characteristics.special_wait;
	else if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level <
			StarShipPtr->RaceDescPtr->ship_info.max_energy)
	{
		COUNT CurSound;

		do
		{
			CurSound =
					2 + ((COUNT)TFB_Random ()
					% (GetSoundCount (StarShipPtr->RaceDescPtr->ship_data.ship_sounds) - 2));
		} while (CurSound == LastSound);
		ProcessSound (SetAbsSoundIndex (
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, CurSound
				), ElementPtr);
		LastSound = CurSound;

		DeltaEnergy (ElementPtr, SPECIAL_ENERGY_COST);

		StarShipPtr->RaceDescPtr->characteristics.special_wait = SPECIAL_WAIT;
	}
}

static void
uninit_pkunk (RACE_DESC *pRaceDesc)
{
	SetCustomShipData (pRaceDesc, NULL);
}

RACE_DESC*
init_pkunk (void)
{
	static RACE_DESC new_pkunk_desc;
	RACE_DESC *RaceDescPtr;
	PKUNK_DATA empty_data;

	if (IS_HD) {
		pkunk_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		pkunk_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		pkunk_desc.cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON_HD + 4;
	}

	// The caller of this func will copy the struct
	memset (&empty_data, 0, sizeof (empty_data));

	pkunk_desc.uninit_func = uninit_pkunk;
	pkunk_desc.preprocess_func = pkunk_preprocess;
	pkunk_desc.postprocess_func = pkunk_postprocess;
	pkunk_desc.init_weapon_func = initialize_bug_missile;
	pkunk_desc.cyborg_control.intelligence_func = pkunk_intelligence;

	/* copy initial ship settings to the new descriptor */
	new_pkunk_desc = pkunk_desc;
	SetCustomShipData (&new_pkunk_desc, &empty_data);

	RaceDescPtr = &new_pkunk_desc;

	LastSound = 0;
			// We need to reinitialise it at least each battle, to ensure
			// that NetPlay is synchronised if one player played another
			// game before playing against a networked opponent.

	return (RaceDescPtr);
}
