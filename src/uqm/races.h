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

#ifndef UQM_RACES_H_
#define UQM_RACES_H_

#include "types.h"
#include "libs/compiler.h"
#include "units.h"
#include "displist.h"

typedef struct STARSHIP STARSHIP;
typedef HLINK HSTARSHIP;

#include "element.h"
#include "libs/sndlib.h"
#include "libs/reslib.h"

#if defined(__cplusplus)
extern "C" {
#endif


// TODO: remove RACES_PER_PLAYER remnant of SC1
#define RACES_PER_PLAYER 7
#define MAX_SHIPS_PER_SIDE 14

/* SHIP_INFO.ship_flags - ship specific flags */
/* bits 0 and 1 are now available */
#define SEEKING_WEAPON    (1 << 2)
#define SEEKING_SPECIAL   (1 << 3)
#define POINT_DEFENSE     (1 << 4)
		/* Ship has some point-defense capabilities */
#define IMMEDIATE_WEAPON  (1 << 5)
#define CREW_IMMUNE       (1 << 6)
#define FIRES_FORE        (1 << 7)
#define FIRES_RIGHT       (1 << 8)
#define FIRES_AFT         (1 << 9)
#define FIRES_LEFT        (1 << 10)
#define SHIELD_DEFENSE    (1 << 11)
#define DONT_CHASE        (1 << 12)
#define PLAYER_CAPTAIN    (1 << 13)
		/* The protagonist himself is on board. He gets a different color. */
#define LIGHT_POINT_DEFENSE (1 << 14)
#define HEAVY_POINT_DEFENSE (1 << 15)

typedef UWORD STATUS_FLAGS;

/* STATUS_FLAGS - heat of battle specific flags */
#define LEFT                   (1 << 0)
#define RIGHT                  (1 << 1)
#define THRUST                 (1 << 2)
#define WEAPON                 (1 << 3)
#define SPECIAL                (1 << 4)
#define LOW_ON_ENERGY          (1 << 5)
#define SHIP_BEYOND_MAX_SPEED  (1 << 6)
#define SHIP_AT_MAX_SPEED      (1 << 7)
#define SHIP_IN_GRAVITY_WELL   (1 << 8)
#define PLAY_VICTORY_DITTY     (1 << 9)

/* These track the old resource package orderings for the ship resource indices */
typedef enum
{
	NO_ID,
	ARILOU_ID,
	CHMMR_ID,
	EARTHLING_ID,
	ORZ_ID,
	PKUNK_ID,
	SHOFIXTI_ID,
	SPATHI_ID,
	SUPOX_ID,
	THRADDASH_ID,
	UTWIG_ID,
	VUX_ID,
	YEHAT_ID,
	MELNORME_ID,
	DRUUGE_ID,
	ILWRATH_ID,
	MYCON_ID,
	SLYLANDRO_ID,
	UMGAH_ID,
	UR_QUAN_ID,
	ZOQFOTPIK_ID,
	SYREEN_ID,
	KOHR_AH_ID,
	ANDROSYNTH_ID,
	CHENJESU_ID,
	MMRNMHRM_ID,
	LAST_MELEE_ID = MMRNMHRM_ID,
	SIS_SHIP_ID,
	SA_MATRA_ID,
	UR_QUAN_PROBE_ID,
	NUM_SPECIES_ID
} SPECIES_ID;

typedef struct captain_stuff
{
	RESOURCE captain_rsc;
	FRAME background;
	FRAME turn;
	FRAME thrust;
	FRAME weapon;
	FRAME special;
} CAPTAIN_STUFF;

typedef enum
{
	PURSUE = 0,
	AVOID,
	ENTICE,
	NO_MOVEMENT
} MOVEMENT_STATE;

typedef struct
{
	ELEMENT *ObjectPtr;
	COUNT facing;
	COUNT which_turn;
	MOVEMENT_STATE MoveState;
} EVALUATE_DESC;

typedef void (IntelligenceFunc) (ELEMENT *ShipPtr,
		EVALUATE_DESC *ObjectsOfConcern, COUNT ConcernCounter);
typedef struct
{
	COUNT ManeuverabilityIndex;
	COUNT WeaponRange;
	IntelligenceFunc *intelligence_func;
} INTEL_STUFF;

typedef struct
{
	COUNT max_thrust;
	COUNT thrust_increment;
	BYTE energy_regeneration;
	BYTE weapon_energy_cost;
	BYTE special_energy_cost;
	BYTE energy_wait;
	BYTE turn_wait;
	BYTE thrust_wait;
	BYTE weapon_wait;
	BYTE special_wait;
	BYTE ship_mass;
} CHARACTERISTIC_STUFF;

typedef struct
{
	const char *idStr;
	UWORD ship_flags;
	BYTE ship_cost;
	
	COUNT crew_level;
	COUNT max_crew;
	BYTE energy_level;
	BYTE max_energy;

	RESOURCE race_strings_rsc;
	RESOURCE icons_rsc;
	RESOURCE melee_icon_rsc;

	STRING race_strings;
	FRAME icons;
	FRAME melee_icon;
} SHIP_INFO;

typedef struct
{
	COUNT strength;
	POINT known_loc;

#define INFINITE_RADIUS ((COUNT) ~0)
} FLEET_STUFF;

typedef struct
{
	RESOURCE ship_rsc[NUM_VIEWS];
	RESOURCE weapon_rsc[NUM_VIEWS];
	RESOURCE special_rsc[NUM_VIEWS];
	CAPTAIN_STUFF captain_control;
	RESOURCE victory_ditty_rsc;
	RESOURCE ship_sounds_rsc;

	FRAME ship[NUM_VIEWS];
	FRAME weapon[NUM_VIEWS];
	FRAME special[NUM_VIEWS];
	MUSIC_REF victory_ditty;
	SOUND ship_sounds;
} DATA_STUFF;


typedef struct race_desc RACE_DESC;

typedef void (PREPROCESS_FUNC) (ELEMENT *ElementPtr);
typedef void (POSTPROCESS_FUNC) (ELEMENT *ElementPtr);
typedef COUNT (INIT_WEAPON_FUNC) (ELEMENT *ElementPtr, HELEMENT Weapon[]);
typedef void (UNINIT_FUNC) (RACE_DESC *pRaceDesc);

struct race_desc
{
	SHIP_INFO ship_info _ALIGNED_ANY;
	FLEET_STUFF fleet _ALIGNED_ANY;
	CHARACTERISTIC_STUFF characteristics _ALIGNED_ANY;
	DATA_STUFF ship_data _ALIGNED_ANY;
	INTEL_STUFF cyborg_control _ALIGNED_ANY;

	UNINIT_FUNC *uninit_func;
	PREPROCESS_FUNC *preprocess_func;
	POSTPROCESS_FUNC *postprocess_func;
	INIT_WEAPON_FUNC *init_weapon_func;

	void* data;  // private ship data, ship code owns this

	void *CodeRef;
};

#define SHIP_BASE_COMMON \
	/* LINK elements; must be first */ \
	HLINK pred; \
	HLINK succ; \
	\
	SPECIES_ID SpeciesID; \
	BYTE captains_name_index \
			/* Also used in full-game to detect if a STARSHIP is an escort
			 * or the flagship (captains_name_index == 0) */

typedef struct
{
	SHIP_BASE_COMMON;
} SHIP_BASE;


struct STARSHIP
{
	SHIP_BASE_COMMON;
	
	RACE_DESC *RaceDescPtr;

	// Ship information
	COUNT crew_level;
			// In full-game battles: crew left
			// In SuperMelee: irrelevant
	COUNT max_crew;
	BYTE ship_cost;
			// In Super Melee ship queue: ship cost
			// In full-game: irrelevant
	COUNT index;
			// original queue index
	STRING race_strings;
	FRAME icons;

	// Battle states
	BYTE weapon_counter;
			// In battle: frames left before primary weapon can be used
	BYTE special_counter;
			// In battle: frames left before special can be used
	BYTE energy_counter;
			// In battle: frames left before energy regeneration

	BYTE ship_input_state;
	STATUS_FLAGS cur_status_flags;
	STATUS_FLAGS old_status_flags;

	HELEMENT hShip;
	COUNT ShipFacing;

	SIZE playerNr;
			//  0: bottom player; In full-game: the human player (RPG)
			//  1: top player; In full-game: the NPC opponent
			// -1: neutral; this should currently never happen (asserts)
	BYTE control;
			// HUMAN, COMPUTER or NETWORK control flags, see intel.h
};

#define RPG_PLAYER_NUM  0
#define NPC_PLAYER_NUM  1

static inline STARSHIP *
LockStarShip (const QUEUE *pq, HSTARSHIP h)
{
	assert (GetLinkSize (pq) == sizeof (STARSHIP));
	return (STARSHIP *) LockLink (pq, h);
}

#define UnlockStarShip(pq, h)  UnlockLink (pq, h)
#define FreeStarShip(pq, h)    FreeLink (pq, h)


typedef HLINK HSHIPFRAG;

typedef struct
{
	SHIP_BASE_COMMON;

	BYTE race_id;
	BYTE index;
	COUNT crew_level;
			/* For ships in npc_built_ship_q, the value INFINITE_FLEET for
			 * crew_level indicates an infinite number of ships. */
	COUNT max_crew;
	
	BYTE energy_level;
	BYTE max_energy;
			// XXX: energy_level and max_energy are unused. We save and load
			//   them, but otherwise nothing needs them atm.

	STRING race_strings;
	FRAME icons;
	FRAME melee_icon;  /* Only used by Shipyard */

#define INFINITE_FLEET ((COUNT) ~0)
} SHIP_FRAGMENT;

static inline SHIP_FRAGMENT *
LockShipFrag (const QUEUE *pq, HSHIPFRAG h)
{
	assert (GetLinkSize (pq) == sizeof (SHIP_FRAGMENT));
	return (SHIP_FRAGMENT *) LockLink (pq, h);
}

#define UnlockShipFrag(pq, h)  UnlockLink (pq, h)
#define FreeShipFrag(pq, h)    FreeLink (pq, h)


typedef HLINK HFLEETINFO;

typedef struct
{
	// LINK elements; must be first
	HFLEETINFO pred;
	HFLEETINFO succ;

	SPECIES_ID SpeciesID;
	const char *shipIdStr;

	UWORD allied_state; /* GOOD_GUY, BAD_GUY or DEAD_GUY */
	BYTE days_left;   /* Days left before the fleet reachers 'dest_loc'. */
	BYTE growth_fract;
	COUNT crew_level;
	COUNT max_crew;
	BYTE growth;
	BYTE max_energy;
	POINT loc;        /* Location of the fleet (center) */

	STRING race_strings;
			/* Race specific strings, see doc/devel/racestrings. */
	FRAME icons;
	FRAME melee_icon;

	COUNT actual_strength;
			/* Measure for the size of the sphere of influence.
			 * 0 if there is none and no ships will be generated.
	         * '(COUNT) ~0' if there is none, and the ship generation
			 * is handled separately. */
	COUNT known_strength;
			/* Measure for the size of the sphere of influence when last
			 * checked the starmap.
			 * 0 if the race's SoI is not known. */
	POINT known_loc;
			/* Location of the SoI (center) when last checked
			 * the starmap. */

	BYTE growth_err_term;
	BYTE func_index;
			/* Function index defined in clock.h (the same as in SetEvent())
			 * for the function to call when the fleet reaches 'dest_loc'.
			 * '(BYTE) ~0' means no function to call. */
	POINT dest_loc;
			/* Location to which the fleet (center) is moving. */

} FLEET_INFO;

// Values for FLEET_INFO.allied_state
enum
{
	DEAD_GUY = 0,  // Race is extinct
	GOOD_GUY,      // Race is allied with the player
	BAD_GUY,       // Race is not allied with the player
	CAN_BUILD,	   // Ships can be built regardless
};

static inline FLEET_INFO *
LockFleetInfo (const QUEUE *pq, HFLEETINFO h)
{
	assert (GetLinkSize (pq) == sizeof (FLEET_INFO));
	return (FLEET_INFO *) LockLink (pq, h);
}

#define UnlockFleetInfo(pq, h) UnlockLink (pq, h)

// Used as index into avail_race_q.
typedef enum
{
	ARILOU_SHIP,
	CHMMR_SHIP,
	HUMAN_SHIP,
	ORZ_SHIP,
	PKUNK_SHIP,
	SHOFIXTI_SHIP,
	SPATHI_SHIP,
	SUPOX_SHIP,
	THRADDASH_SHIP,
	UTWIG_SHIP,
	VUX_SHIP,
	YEHAT_SHIP,
	MELNORME_SHIP,
	DRUUGE_SHIP,
	ILWRATH_SHIP,
	MYCON_SHIP,
	SLYLANDRO_SHIP,
	UMGAH_SHIP,
	URQUAN_SHIP,
	ZOQFOTPIK_SHIP,

	SYREEN_SHIP,
	BLACK_URQUAN_SHIP,
	YEHAT_REBEL_SHIP,
	URQUAN_DRONE_SHIP,
	SAMATRA_SHIP = URQUAN_DRONE_SHIP,

	NUM_AVAILABLE_RACES
} RACE_ID;

#define RACE_COMMUNICATION \
		ARILOU_CONVERSATION,       /* ARILOU_SHIP */ \
		CHMMR_CONVERSATION,        /* CHMMR_SHIP */ \
		INVALID_CONVERSATION,      /* HUMAN_SHIP */ \
		ORZ_CONVERSATION,          /* ORZ_SHIP */ \
		PKUNK_CONVERSATION,        /* PKUNK_SHIP */ \
		SHOFIXTI_CONVERSATION,     /* SHOFIXTI_SHIP */ \
		SPATHI_CONVERSATION,       /* SPATHI_SHIP */ \
		SUPOX_CONVERSATION,        /* SUPOX_SHIP */ \
		THRADD_CONVERSATION,       /* THRADDASH_SHIP */ \
		UTWIG_CONVERSATION,        /* UTWIG_SHIP */ \
		VUX_CONVERSATION,          /* VUX_SHIP */ \
		YEHAT_CONVERSATION,        /* YEHAT_SHIP */ \
		MELNORME_CONVERSATION,     /* MELNORME_SHIP */ \
		DRUUGE_CONVERSATION,       /* DRUUGE_SHIP */ \
		ILWRATH_CONVERSATION,      /* ILWRATH_SHIP */ \
		MYCON_CONVERSATION,        /* MYCON_SHIP */ \
		SLYLANDRO_CONVERSATION,    /* SLYLANDRO_SHIP */ \
		UMGAH_CONVERSATION,        /* UMGAH_SHIP */ \
		URQUAN_CONVERSATION,       /* URQUAN_SHIP */ \
		ZOQFOTPIK_CONVERSATION,    /* ZOQFOTPIK_SHIP */ \
		INVALID_CONVERSATION,      /* SYREEN_SHIP */ \
		BLACKURQ_CONVERSATION,     /* BLACK_URQUAN_SHIP */ \
		YEHAT_REBEL_CONVERSATION,  /* YEHAT_REBEL_SHIP */ \
		URQUAN_DRONE_CONVERSATION, /* URQUAN_DRONE_SHIP */

#define RACE_SHIP_FOR_COMM \
	ARILOU_SHIP,       /* ARILOU_CONVERSATION */ \
	CHMMR_SHIP,        /* CHMMR_CONVERSATION */ \
	HUMAN_SHIP,        /* COMMANDER_CONVERSATION */ \
	ORZ_SHIP,          /* ORZ_CONVERSATION */ \
	PKUNK_SHIP,        /* PKUNK_CONVERSATION */ \
	SHOFIXTI_SHIP,     /* SHOFIXTI_CONVERSATION */ \
	SPATHI_SHIP,       /* SPATHI_CONVERSATION */ \
	SUPOX_SHIP,        /* SUPOX_CONVERSATION */ \
	THRADDASH_SHIP,    /* THRADD_CONVERSATION */ \
	UTWIG_SHIP,        /* UTWIG_CONVERSATION */ \
	VUX_SHIP,          /* VUX_CONVERSATION */ \
	YEHAT_SHIP,        /* YEHAT_CONVERSATION */ \
	MELNORME_SHIP,     /* MELNORME_CONVERSATION */ \
	DRUUGE_SHIP,       /* DRUUGE_CONVERSATION */ \
	ILWRATH_SHIP,      /* ILWRATH_CONVERSATION */ \
	MYCON_SHIP,        /* MYCON_CONVERSATION */ \
	SLYLANDRO_SHIP,    /* SLYLANDRO_CONVERSATION */ \
	UMGAH_SHIP,        /* UMGAH_CONVERSATION */ \
	URQUAN_SHIP,       /* URQUAN_CONVERSATION */ \
	ZOQFOTPIK_SHIP,    /* ZOQFOTPIK_CONVERSATION */ \
	SYREEN_SHIP,       /* SYREEN_CONVERSATION */ \
	BLACK_URQUAN_SHIP, /* BLACKURQ_CONVERSATION */ \
	UMGAH_SHIP,        /* TALKING_PET_CONVERSATION */ \
	SLYLANDRO_SHIP,    /* SLYLANDRO_HOME_CONVERSATION */ \
	URQUAN_DRONE_SHIP, /* URQUAN_DRONE_CONVERSATION */ \
	YEHAT_SHIP,        /* YEHAT_REBEL_CONVERSATION */ \
	HUMAN_SHIP         /* INVALID_CONVERSATION */
 
#define RACE_SHIP_COST \
		1600,  /* ARILOU_SHIP */ \
		3000,  /* CHMMR_SHIP */ \
		1100,  /* HUMAN_SHIP */ \
		2300,  /* ORZ_SHIP */ \
		2000,  /* PKUNK_SHIP */ \
		 500,  /* SHOFIXTI_SHIP */ \
		1800,  /* SPATHI_SHIP */ \
		1600,  /* SUPOX_SHIP */ \
		1000,  /* THRADDASH_SHIP */ \
		2200,  /* UTWIG_SHIP */ \
		1200,  /* VUX_SHIP */ \
		2300,  /* YEHAT_SHIP */ \
		3600,  /* MELNORME_SHIP */ \
		1700,  /* DRUUGE_SHIP */ \
		1000,  /* ILWRATH_SHIP */ \
		2100,  /* MYCON_SHIP */ \
		4400,  /* SLYLANDRO_SHIP */ \
		 700,  /* UMGAH_SHIP */ \
		3000,  /* URQUAN_SHIP */ \
		 600,  /* ZOQFOTPIK_SHIP */ \
		1300,  /* SYREEN_SHIP */ \
		3000,  /* BLACK_URQUAN_SHIP */ \
		2300,  /* YEHAT_REBEL_SHIP */

#define LOG_TO_IP(s) ((s) << 1)
#define RACE_IP_SPEED \
		LOG_TO_IP (40),  /* ARILOU_SHIP */ \
		LOG_TO_IP (27),  /* CHMMR_SHIP */ \
		LOG_TO_IP (24),  /* HUMAN_SHIP */ \
		LOG_TO_IP (40),  /* ORZ_SHIP */ \
		LOG_TO_IP (40),  /* PKUNK_SHIP */ \
		LOG_TO_IP (35),  /* SHOFIXTI_SHIP */ \
		LOG_TO_IP (48),  /* SPATHI_SHIP */ \
		LOG_TO_IP (40),  /* SUPOX_SHIP */ \
		LOG_TO_IP (28),  /* THRADDASH_SHIP */ \
		LOG_TO_IP (30),  /* UTWIG_SHIP */ \
		LOG_TO_IP (21),  /* VUX_SHIP */ \
		LOG_TO_IP (30),  /* YEHAT_SHIP */ \
		LOG_TO_IP (40),  /* MELNORME_SHIP */ \
		LOG_TO_IP (20),  /* DRUUGE_SHIP */ \
		LOG_TO_IP (25),  /* ILWRATH_SHIP */ \
		LOG_TO_IP (27),  /* MYCON_SHIP */ \
		LOG_TO_IP (60),  /* SLYLANDRO_SHIP */ \
		LOG_TO_IP (18),  /* UMGAH_SHIP */ \
		LOG_TO_IP (30),  /* URQUAN_SHIP */ \
		LOG_TO_IP (40),  /* ZOQFOTPIK_SHIP */ \
		LOG_TO_IP (36),  /* SYREEN_SHIP */ \
		LOG_TO_IP (30),  /* BLACK_URQUAN_SHIP */ \
		LOG_TO_IP (30),  /* YEHAT_REBEL_SHIP */ \
		LOG_TO_IP (90),  /* URQUAN_DRONE_SHIP */

#define LOG_TO_HYPER(s) (WORLD_TO_VELOCITY (s) >> 1)
#define RACE_HYPER_SPEED \
		LOG_TO_HYPER (RES_SCALE(40)),  /* ARILOU_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(27)),  /* CHMMR_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(24)),  /* HUMAN_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(40)),  /* ORZ_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(40)),  /* PKUNK_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(35)),  /* SHOFIXTI_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(48)),  /* SPATHI_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(40)),  /* SUPOX_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(50)),  /* THRADDASH_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(30)),  /* UTWIG_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(21)),  /* VUX_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(30)),  /* YEHAT_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(40)),  /* MELNORME_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(20)),  /* DRUUGE_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(25)),  /* ILWRATH_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(27)),  /* MYCON_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(60)),  /* SLYLANDRO_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(18)),  /* UMGAH_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(30)),  /* URQUAN_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(40)),  /* ZOQFOTPIK_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(36)),  /* SYREEN_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(30)),  /* BLACK_URQUAN_SHIP */ \
		LOG_TO_HYPER (RES_SCALE(30)),  /* YEHAT_REBEL_SHIP */

#define RACE_HYPERSPACE_PERCENT \
		20,  /* ARILOU_SHIP */ \
		 0,  /* CHMMR_SHIP */ \
		 0,  /* HUMAN_SHIP */ \
		20,  /* ORZ_SHIP */ \
		40,  /* PKUNK_SHIP */ \
		 0,  /* SHOFIXTI_SHIP */ \
		20,  /* SPATHI_SHIP */ \
		40,  /* SUPOX_SHIP */ \
		60,  /* THRADDASH_SHIP */ \
		40,  /* UTWIG_SHIP */ \
		40,  /* VUX_SHIP */ \
		60,  /* YEHAT_SHIP */ \
		 0,  /* MELNORME_SHIP */ \
		30,  /* DRUUGE_SHIP */ \
		60,  /* ILWRATH_SHIP */ \
		40,  /* MYCON_SHIP */ \
		 2,  /* SLYLANDRO_SHIP */ \
		30,  /* UMGAH_SHIP */ \
		70,  /* URQUAN_SHIP */ \
		 0,  /* ZOQFOTPIK_SHIP */ \
		 0,  /* SYREEN_SHIP */ \
		70,  /* BLACK_URQUAN_SHIP */ \
		60,  /* YEHAT_REBEL_SHIP */ \
		 0,  /* URQUAN_DRONE_SHIP */

#define RACE_INTERPLANETARY_PERCENT \
		 2,  /* ARILOU_SHIP */ \
		 0,  /* CHMMR_SHIP */ \
		 0,  /* HUMAN_SHIP */ \
		20,  /* ORZ_SHIP */ \
		20,  /* PKUNK_SHIP */ \
		 0,  /* SHOFIXTI_SHIP */ \
		10,  /* SPATHI_SHIP */ \
		20,  /* SUPOX_SHIP */ \
		20,  /* THRADDASH_SHIP */ \
		20,  /* UTWIG_SHIP */ \
		20,  /* VUX_SHIP */ \
		40,  /* YEHAT_SHIP */ \
		 0,  /* MELNORME_SHIP */ \
		20,  /* DRUUGE_SHIP */ \
		60,  /* ILWRATH_SHIP */ \
		20,  /* MYCON_SHIP */ \
		 5,  /* SLYLANDRO_SHIP */ \
		20,  /* UMGAH_SHIP */ \
		40,  /* URQUAN_SHIP */ \
		 0,  /* ZOQFOTPIK_SHIP */ \
		 0,  /* SYREEN_SHIP */ \
		40,  /* BLACK_URQUAN_SHIP */ \
		40,  /* YEHAT_REBEL_SHIP */ \
		 0,  /* URQUAN_DRONE_SHIP */

#define RACE_MUSIC_BOOL \
		1,  /* ARILOU_SHIP */ \
		0,  /* CHMMR_SHIP */ \
		0,  /* HUMAN_SHIP */ \
		1,  /* ORZ_SHIP */ \
		1,  /* PKUNK_SHIP */ \
		0,  /* SHOFIXTI_SHIP */ \
		1,  /* SPATHI_SHIP */ \
		1,  /* SUPOX_SHIP */ \
		1,  /* THRADDASH_SHIP */ \
		1,  /* UTWIG_SHIP */ \
		1,  /* VUX_SHIP */ \
		1,  /* YEHAT_SHIP */ \
		0,  /* MELNORME_SHIP */ \
		1,  /* DRUUGE_SHIP */ \
		1,  /* ILWRATH_SHIP */ \
		1,  /* MYCON_SHIP */ \
		0,  /* SLYLANDRO_SHIP */ \
		1,  /* UMGAH_SHIP */ \
		1,  /* URQUAN_SHIP */ \
		1,  /* ZOQFOTPIK_SHIP */ \
		0,  /* SYREEN_SHIP */ \
		1,  /* BLACK_URQUAN_SHIP */ \
		1,  /* YEHAT_REBEL_SHIP */ \
		0,  /* URQUAN_DRONE_SHIP */

#define HOMEWORLD_LOC \
		0,					/* ARILOU_SHIP */ \
		0,					/* CHMMR_SHIP */ \
		0,					/* HUMAN_SHIP */ \
		ORZ_DEFINED,		/* ORZ_SHIP */ \
		PKUNK_DEFINED,		/* PKUNK_SHIP */ \
		0,					/* SHOFIXTI_SHIP */ \
		SPATHI_DEFINED,		/* SPATHI_SHIP */ \
		SUPOX_DEFINED,		/* SUPOX_SHIP */ \
		THRADD_DEFINED,		/* THRADDASH_SHIP */ \
		UTWIG_DEFINED,		/* UTWIG_SHIP */ \
		VUX_DEFINED,		/* VUX_SHIP */ \
		YEHAT_DEFINED,		/* YEHAT_SHIP */ \
		0,					/* MELNORME_SHIP */ \
		DRUUGE_DEFINED,		/* DRUUGE_SHIP */ \
		ILWRATH_DEFINED,	/* ILWRATH_SHIP */ \
		MYCON_DEFINED,		/* MYCON_SHIP */ \
		0,					/* SLYLANDRO_SHIP */ \
		UMGAH_DEFINED,		/* UMGAH_SHIP */ \
		0,					/* URQUAN_SHIP */ \
		ZOQFOT_DEFINED,		/* ZOQFOTPIK_SHIP */ \
		0,					/* SYREEN_SHIP */ \
		0,					/* BLACK_URQUAN_SHIP */ \
		0,					/* YEHAT_REBEL_SHIP */ \

// How many ships will an encounter consist of.
// The first number specifies the minimum, the second the maximum.
// The chance is 50% for each ship past the minimum to be present.
#define RACE_ENCOUNTER_MAKEUP \
		MAKE_BYTE (1, 5),  /* ARILOU_SHIP */ \
		0,                 /* CHMMR_SHIP */ \
		0,                 /* HUMAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* ORZ_SHIP */ \
		MAKE_BYTE (1, 5),  /* PKUNK_SHIP */ \
		0,                 /* SHOFIXTI_SHIP */ \
		MAKE_BYTE (1, 5),  /* SPATHI_SHIP */ \
		MAKE_BYTE (1, 5),  /* SUPOX_SHIP */ \
		MAKE_BYTE (1, 5),  /* THRADDASH_SHIP */ \
		MAKE_BYTE (1, 5),  /* UTWIG_SHIP */ \
		MAKE_BYTE (1, 5),  /* VUX_SHIP */ \
		MAKE_BYTE (1, 5),  /* YEHAT_SHIP */ \
		MAKE_BYTE (1, 1),  /* MELNORME_SHIP */ \
		MAKE_BYTE (1, 5),  /* DRUUGE_SHIP */ \
		MAKE_BYTE (1, 5),  /* ILWRATH_SHIP */ \
		MAKE_BYTE (1, 5),  /* MYCON_SHIP */ \
		MAKE_BYTE (1, 1),  /* SLYLANDRO_SHIP */ \
		MAKE_BYTE (1, 5),  /* UMGAH_SHIP */ \
		MAKE_BYTE (1, 5),  /* URQUAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* ZOQFOTPIK_SHIP */ \
		0,                 /* SYREEN_SHIP */ \
		MAKE_BYTE (1, 5),  /* BLACK_URQUAN_SHIP */ \
		MAKE_BYTE (1, 5),  /* YEHAT_REBEL_SHIP */

#define RACE_COLORS \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x10), 0x53),  /* ARILOU_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x96, 0x96, 0x96), 0xFF),  /* CHMMR_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x01, 0x1f), 0x4D),  /* HUMAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0E, 0x00, 0x0E), 0x36),  /* ORZ_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x08), 0x62),  /* PKUNK_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x00), 0x00),  /* SHOFIXTI_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0C, 0x05, 0x00), 0x76),  /* SPATHI_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0C, 0x05, 0x00), 0x76),  /* SUPOX_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x08), 0x62),  /* THRADDASH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x08), 0x62),  /* UTWIG_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x00, 0x10), 0x53),  /* VUX_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0A, 0x00, 0x11), 0x3D),  /* YEHAT_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x06, 0x08), 0x62),  /* MELNORME_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),  /* DRUUGE_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0E, 0x00, 0x0E), 0x36),  /* ILWRATH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0E, 0x00, 0x0E), 0x36),  /* MYCON_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0C, 0x05, 0x00), 0x76),  /* SLYLANDRO_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0A, 0x00, 0x11), 0x3D),  /* UMGAH_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x00, 0x08, 0x00), 0x6E),  /* URQUAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2D),  /* ZOQFOTPIK_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x66, 0x00, 0xCC), 0xFF),  /* SYREEN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x06, 0x06, 0x06), 0x20),  /* BLACK_URQUAN_SHIP */ \
		BUILD_COLOR (MAKE_RGB15_INIT (0x14, 0x07, 0x1F), 0x39),  /* YEHAT_REBEL_SHIP */

// JMS: For showing the SC1-era situation in starmap
#define RACE_PREWAR_STRENGTHS \
		0,    /* ARILOU_SHIP */ \
		500,  /* CHMMR_SHIP masquerading as Mmrnmhrm */ \
		400,  /* HUMAN_SHIP */ \
		0,    /* ORZ_SHIP */ \
		642,  /* PKUNK_SHIP, should be marked 'Unknown' */ \
		400,  /* SHOFIXTI_SHIP */ \
		1003, /* SPATHI_SHIP */ \
		0,	  /* SUPOX_SHIP */ \
		835,  /* THRADDASH_SHIP, should be marked 'Unknown' */ \
		0,    /* UTWIG_SHIP masquerading as Chenjesu */ \
		700,  /* VUX_SHIP */ \
		1100, /* YEHAT_SHIP */ \
		0,	  /* MELNORME_SHIP */ \
		800,  /* DRUUGE_SHIP, should be marked 'Unknown' */ \
		500,  /* ILWRATH_SHIP */ \
		650,  /* MYCON_SHIP */ \
		750,  /* SLYLANDRO_SHIP masquerading as Chenjesu */ \
		633,  /* UMGAH_SHIP */ \
		0,    /* URQUAN_SHIP */ \
		0,	  /* ZOQFOTPIK_SHIP */ \
		450,  /* SYREEN_SHIP masquareding as Androsynth */ \
		0,	  /* BLACK_URQUAN_SHIP */ \
		0,	  /* YEHAT_REBEL_SHIP */
		
// JMS: For showing the SC1-era situation in starmap
#define RACE_PREWAR_LOCATIONS \
		{0,0},		 /* ARILOU_SHIP */ \
		{742,2968},	 /* CHMMR_SHIP masquerading as Mmrnmhrm */ \
		{1752,1450}, /* HUMAN_SHIP */ \
		{0,0 },		 /* ORZ_SHIP */ \
		{502,401},   /* PKUNK_SHIP, should be marked 'Unknown' */ \
		{2908,269},	 /* SHOFIXTI_SHIP */ \
		{2549,3600}, /* SPATHI_SHIP */ \
		{0,0},		 /* SUPOX_SHIP */ \
		{2535,8358}, /* THRADDASH_SHIP, should be marked 'Unknown' */ \
		{0,0},		 /* UTWIG_SHIP masquerading as Chenjesu */ \
		{4333,1687}, /* VUX_SHIP */ \
		{4923,294},  /* YEHAT_SHIP */ \
		{0,0},		 /* MELNORME_SHIP */ \
		{9469,2806}, /* DRUUGE_SHIP, should be marked 'Unknown' */ \
		{229,3666},  /* ILWRATH_SHIP */ \
		{6392,2200}, /* MYCON_SHIP */ \
		{0742,2268}, /* SLYLANDRO_SHIP masquerading as Chenjesu */ \
		{1978,5968}, /* UMGAH_SHIP */ \
		{0,0},		 /* URQUAN_SHIP */ \
		{0,0},		 /* ZOQFOTPIK_SHIP */ \
		{3587,2566}, /* SYREEN_SHIP masquerading as Androsynth */ \
		{0,0},		 /* BLACK_URQUAN_SHIP */ \
		{0,0},		 /* YEHAT_REBEL_SHIP */
		
// JMS: For showing the SC1-era situation in starmap
#define RACE_PREWAR_NAME_UNKNOWN \
		FALSE,  /* ARILOU_SHIP */ \
		FALSE,  /* CHMMR_SHIP masquerading as Mmrnmhrm */ \
		FALSE,  /* HUMAN_SHIP */ \
		FALSE,  /* ORZ_SHIP */ \
		TRUE,   /* PKUNK_SHIP, should be marked 'Unknown' */ \
		FALSE,  /* SHOFIXTI_SHIP */ \
		FALSE,  /* SPATHI_SHIP */ \
		FALSE,	/* SUPOX_SHIP */ \
		TRUE,   /* THRADDASH_SHIP, should be marked 'Unknown' */ \
		FALSE,  /* UTWIG_SHIP */ \
		FALSE,  /* VUX_SHIP */ \
		FALSE,  /* YEHAT_SHIP */ \
		FALSE,	/* MELNORME_SHIP */ \
		TRUE,   /* DRUUGE_SHIP, should be marked 'Unknown' */ \
		FALSE,  /* ILWRATH_SHIP */ \
		FALSE,  /* MYCON_SHIP */ \
		FALSE,	/* SLYLANDRO_SHIP masquerading as Chenjesu */ \
		FALSE,  /* UMGAH_SHIP */ \
		FALSE,  /* URQUAN_SHIP */ \
		FALSE,	/* ZOQFOTPIK_SHIP */ \
		FALSE,  /* SYREEN_SHIP masquareding as Androsynth */ \
		FALSE,	/* BLACK_URQUAN_SHIP */ \
		FALSE,	/* YEHAT_REBEL_SHIP */

#if defined(__cplusplus)
}
#endif

#endif /* UQM_RACES_H_ */

