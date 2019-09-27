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
#include "vux.h"
#include "resinst.h"
#include "../../setup.h"
#include "uqm/globdata.h"
#include "libs/mathlib.h"

// Core characteristics
#define MAX_CREW 20
#define MAX_ENERGY 40
#define ENERGY_REGENERATION 1
#define ENERGY_WAIT 8
#define MAX_THRUST /* DISPLAY_TO_WORLD (5) */ 21
#define THRUST_INCREMENT /* DISPLAY_TO_WORLD (2) */ 7
#define THRUST_WAIT 4
#define TURN_WAIT 6
#define SHIP_MASS 6

// Laser
#define WEAPON_ENERGY_COST 1
#define WEAPON_WAIT 0
#define VUX_OFFSET RES_SCALE(12)
#define LASER_BASE RES_SCALE(150)
#define LASER_RANGE DISPLAY_TO_WORLD (LASER_BASE + VUX_OFFSET)

// Limpet
#define SPECIAL_ENERGY_COST 2
#define SPECIAL_WAIT 7
#define LIMPET_SPEED RES_SCALE(25)
#define LIMPET_OFFSET RES_SCALE(8)
#define LIMPET_LIFE 80
#define LIMPET_HITS 1
#define LIMPET_DAMAGE 0
#define MIN_THRUST_INCREMENT DISPLAY_TO_WORLD (RES_SCALE(1))

// Aggressive Entry
#define WARP_OFFSET RES_SCALE(46)
		/* How far outside of the laser range can the ship warp in. */
#define MAXX_ENTRY_DIST DISPLAY_TO_WORLD ((LASER_BASE + VUX_OFFSET + WARP_OFFSET) << 1)
#define MAXY_ENTRY_DIST DISPLAY_TO_WORLD ((LASER_BASE + VUX_OFFSET + WARP_OFFSET) << 1)
		/* Originally, the warp distance was:
		 * DISPLAY_TO_WORLD (SPACE_HEIGHT << 1)
		 * where SPACE_HEIGHT = SCREEN_HEIGHT
		 * But in reality this should be relative to the laser-range. */

static RACE_DESC vux_desc =
{
	{ /* SHIP_INFO */
		"intruder",
		FIRES_FORE | SEEKING_SPECIAL | IMMEDIATE_WEAPON,
		12, /* Super Melee cost */
		MAX_CREW, MAX_CREW,
		MAX_ENERGY, MAX_ENERGY,
		VUX_RACE_STRINGS,
		VUX_ICON_MASK_PMAP_ANIM,
		VUX_MICON_MASK_PMAP_ANIM,
		NULL, NULL, NULL
	},
	{ /* FLEET_STUFF */
		900 / SPHERE_RADIUS_INCREMENT * 2, /* Initial SoI radius */
		{ /* Known location (center of SoI) */
			4412, 1558,
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
			VUX_BIG_MASK_PMAP_ANIM,
			VUX_MED_MASK_PMAP_ANIM,
			VUX_SML_MASK_PMAP_ANIM,
		},
		{
			SLIME_MASK_PMAP_ANIM,
			NULL_RESOURCE,
			NULL_RESOURCE,
		},
		{
			LIMPETS_BIG_MASK_PMAP_ANIM,
			LIMPETS_MED_MASK_PMAP_ANIM,
			LIMPETS_SML_MASK_PMAP_ANIM,
		},
		{
			VUX_CAPTAIN_MASK_PMAP_ANIM,
			NULL, NULL, NULL, NULL, NULL
		},
		VUX_VICTORY_SONG,
		VUX_SHIP_SOUNDS,
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
limpet_preprocess (ELEMENT *ElementPtr)
{
	COUNT facing, orig_facing;
	SIZE delta_facing;

	facing = orig_facing = NORMALIZE_FACING (ANGLE_TO_FACING (
			GetVelocityTravelAngle (&ElementPtr->velocity)
			));
	if ((delta_facing = TrackShip (ElementPtr, &facing)) > 0)
	{
		facing = orig_facing + delta_facing;
		SetVelocityVector (&ElementPtr->velocity, LIMPET_SPEED, facing);
	}
	ElementPtr->next.image.frame =
			 IncFrameIndex (ElementPtr->next.image.frame);

	ElementPtr->state_flags |= CHANGING;
}

static void
limpet_collision (ELEMENT *ElementPtr0, POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1) {
	STAMP s;
	STARSHIP *StarShipPtr;
	RACE_DESC *RDPtr;
	if (ElementPtr1->state_flags & PLAYER_SHIP) {
		GetElementStarShip (ElementPtr1, &StarShipPtr);
		RDPtr = StarShipPtr->RaceDescPtr;
		if (!antiCheat(ElementPtr1, FALSE)) {
			if (++RDPtr->characteristics.turn_wait == 0)
				--RDPtr->characteristics.turn_wait;
			if (++RDPtr->characteristics.thrust_wait == 0)
				--RDPtr->characteristics.thrust_wait;
			if (RDPtr->characteristics.thrust_increment <= MIN_THRUST_INCREMENT) {
				RDPtr->characteristics.max_thrust = RDPtr->characteristics.thrust_increment << 1;
			} else {
				COUNT num_thrusts;
				num_thrusts = RDPtr->characteristics.max_thrust / RDPtr->characteristics.thrust_increment;
				RDPtr->characteristics.thrust_increment -= RES_SCALE(1);
				RDPtr->characteristics.max_thrust = RDPtr->characteristics.thrust_increment * num_thrusts;
			}
			RDPtr->cyborg_control.ManeuverabilityIndex = 0;
			GetElementStarShip (ElementPtr0, &StarShipPtr);
			ProcessSound (SetAbsSoundIndex (StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr1); // LIMPET_AFFIXES
			s.frame = SetAbsFrameIndex (StarShipPtr->RaceDescPtr->ship_data.weapon[0], (COUNT)TFB_Random ());
			ModifySilhouette (ElementPtr1, &s, MODIFY_IMAGE);
		}
		else {
			ProcessSound(SetAbsSoundIndex(StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 2), ElementPtr1); // LIMPET_AFFIXES
		}
	}

	ElementPtr0->hit_points = 0;
	ElementPtr0->life_span = 0;
	ElementPtr0->state_flags |= COLLISION | DISAPPEARING;

	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
spawn_limpets (ELEMENT *ElementPtr)
{
	HELEMENT Limpet;
	STARSHIP *StarShipPtr;
	MISSILE_BLOCK MissileBlock;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	MissileBlock.farray = StarShipPtr->RaceDescPtr->ship_data.special;
	MissileBlock.face = StarShipPtr->ShipFacing + HALF_CIRCLE;
	MissileBlock.index = 0;
	MissileBlock.sender = ElementPtr->playerNr;
	MissileBlock.flags = IGNORE_SIMILAR;
	MissileBlock.pixoffs = LIMPET_OFFSET;
	MissileBlock.speed = LIMPET_SPEED;
	MissileBlock.hit_points = LIMPET_HITS;
	MissileBlock.damage = LIMPET_DAMAGE;
	MissileBlock.life = LIMPET_LIFE;
	MissileBlock.preprocess_func = limpet_preprocess;
	MissileBlock.blast_offs = 0;

	MissileBlock.cx = ElementPtr->next.location.x;
	MissileBlock.cy = ElementPtr->next.location.y;
	Limpet = initialize_missile (&MissileBlock);
	if (Limpet)
	{
		ELEMENT *LimpetPtr;

		LockElement (Limpet, &LimpetPtr);
		LimpetPtr->collision_func = limpet_collision;
		SetElementStarShip (LimpetPtr, StarShipPtr);
		UnlockElement (Limpet);

		PutElement (Limpet);
	}
}

static COUNT
initialize_horrific_laser (ELEMENT *ShipPtr, HELEMENT LaserArray[])
{
	STARSHIP *StarShipPtr;
	LASER_BLOCK LaserBlock;

	GetElementStarShip (ShipPtr, &StarShipPtr);
	LaserBlock.face = StarShipPtr->ShipFacing;
	LaserBlock.cx = ShipPtr->next.location.x;
	LaserBlock.cy = ShipPtr->next.location.y;
	LaserBlock.ex = COSINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.ey = SINE (FACING_TO_ANGLE (LaserBlock.face), LASER_RANGE);
	LaserBlock.sender = ShipPtr->playerNr;
	LaserBlock.flags = IGNORE_SIMILAR;
	LaserBlock.pixoffs = VUX_OFFSET;
	LaserBlock.color = BUILD_COLOR (MAKE_RGB15 (0x0A, 0x1F, 0x0A), 0x0A);
	LaserArray[0] = initialize_laser (&LaserBlock);

	return (1);
}

static void
vux_intelligence (ELEMENT *ShipPtr, EVALUATE_DESC *ObjectsOfConcern,
		COUNT ConcernCounter)
{
	EVALUATE_DESC *lpEvalDesc;
	STARSHIP *StarShipPtr;

	lpEvalDesc = &ObjectsOfConcern[ENEMY_SHIP_INDEX];
	 lpEvalDesc->MoveState = PURSUE;
	if (ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr != 0
			&& ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState == ENTICE)
	{
		if ((ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr->state_flags
				& FINITE_LIFE)
				&& !(ObjectsOfConcern[ENEMY_WEAPON_INDEX].ObjectPtr->state_flags
				& CREW_OBJECT))
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState = AVOID;
		else
			ObjectsOfConcern[ENEMY_WEAPON_INDEX].MoveState = PURSUE;
	}

	ship_intelligence (ShipPtr,
			ObjectsOfConcern, ConcernCounter);

	GetElementStarShip (ShipPtr, &StarShipPtr);
	if (StarShipPtr->special_counter == 0
			&& lpEvalDesc->ObjectPtr != 0
			&& lpEvalDesc->which_turn <= 12
			&& (StarShipPtr->ship_input_state & (LEFT | RIGHT))
			&& StarShipPtr->RaceDescPtr->ship_info.energy_level >=
			(BYTE)(StarShipPtr->RaceDescPtr->ship_info.max_energy >> 1))
		StarShipPtr->ship_input_state |= SPECIAL;
	else
		StarShipPtr->ship_input_state &= ~SPECIAL;
}

static void
vux_postprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ElementPtr, &StarShipPtr);
	if ((StarShipPtr->cur_status_flags & SPECIAL)
			&& StarShipPtr->special_counter == 0
			&& DeltaEnergy (ElementPtr, -SPECIAL_ENERGY_COST))
	{
		ProcessSound (SetAbsSoundIndex (
						/* LAUNCH_LIMPET */
				StarShipPtr->RaceDescPtr->ship_data.ship_sounds, 1), ElementPtr);
		spawn_limpets (ElementPtr);

		StarShipPtr->special_counter =
				StarShipPtr->RaceDescPtr->characteristics.special_wait;
	}
}

static void
vux_preprocess (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		COUNT facing;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		facing = StarShipPtr->ShipFacing;
		if (LOBYTE (GLOBAL (CurrentActivity)) != IN_ENCOUNTER
				&& TrackShip (ElementPtr, &facing) >= 0)
		{
			ELEMENT *OtherShipPtr;
			SDWORD SA_MATRA_EXTRA_DIST = 0;
			LockElement (ElementPtr->hTarget, &OtherShipPtr);

			// JMS: Not REALLY necessary as VUX can ordinarily never be played against Sa-Matra. 
            // But handy in debugging as a single VUX limpet incapacitates Sa-Matra completely.
            if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE) {
				SA_MATRA_EXTRA_DIST += RES_SCALE(1000);
			}
			do {
                // JMS_GFX: Circumventing overflows by using temp variables instead of
                // subtracting straight from the POINT sized ShipImagePtr->current.location.
				SDWORD dx, dy;

				SDWORD temp_x =
						((SDWORD)OtherShipPtr->current.location.x -
						(MAXX_ENTRY_DIST >> 1)) +
						((COUNT)TFB_Random () % MAXX_ENTRY_DIST);
				SDWORD temp_y =
						((SDWORD)OtherShipPtr->current.location.y -
						(MAXY_ENTRY_DIST >> 1)) +
						((COUNT)TFB_Random () % MAXY_ENTRY_DIST);
				temp_x += temp_x > 0 ? SA_MATRA_EXTRA_DIST : -SA_MATRA_EXTRA_DIST;
				temp_y += temp_y > 0 ? SA_MATRA_EXTRA_DIST : -SA_MATRA_EXTRA_DIST;
                
				dx = OtherShipPtr->current.location.x - temp_x;
				dy = OtherShipPtr->current.location.y - temp_y;
				facing = NORMALIZE_FACING ( ANGLE_TO_FACING (ARCTAN (dx, dy)) );
				ElementPtr->current.image.frame = SetAbsFrameIndex (ElementPtr->current.image.frame, facing);

				ElementPtr->current.location.x = WRAP_X (DISPLAY_ALIGN (temp_x));
				ElementPtr->current.location.y = WRAP_Y (DISPLAY_ALIGN (temp_y));
			} while (CalculateGravity (ElementPtr)
					|| TimeSpaceMatterConflict (ElementPtr));

			UnlockElement (ElementPtr->hTarget);
			ElementPtr->hTarget = 0;

			ElementPtr->next = ElementPtr->current;
			InitIntersectStartPoint (ElementPtr);
			InitIntersectEndPoint (ElementPtr);
			InitIntersectFrame (ElementPtr);

			StarShipPtr->ShipFacing = facing;
		}

		StarShipPtr->RaceDescPtr->preprocess_func = 0;
	}
}

RACE_DESC*
init_vux (void)
{
	RACE_DESC *RaceDescPtr;

	if (IS_HD) {
		vux_desc.characteristics.max_thrust = RES_SCALE(MAX_THRUST);
		vux_desc.characteristics.thrust_increment = RES_SCALE(THRUST_INCREMENT);
		vux_desc.cyborg_control.WeaponRange = CLOSE_RANGE_WEAPON_HD;
	}

	vux_desc.preprocess_func = vux_preprocess;
	vux_desc.postprocess_func = vux_postprocess;
	vux_desc.init_weapon_func = initialize_horrific_laser;
	vux_desc.cyborg_control.intelligence_func = vux_intelligence;

	RaceDescPtr = &vux_desc;

	return (RaceDescPtr);
}

