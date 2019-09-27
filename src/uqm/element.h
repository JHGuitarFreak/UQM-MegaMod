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

#ifndef UQM_ELEMENT_H_
#define UQM_ELEMENT_H_

#include "displist.h"
#include "units.h"
#include "velocity.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif


#define NORMAL_LIFE 1

typedef HLINK HELEMENT;

// Bits for ELEMENT_FLAGS:
// bits 0 and 1 are now available
#define PLAYER_SHIP (1 << 2)
		// The ELEMENT is a player controlable ship, and not some bullet,
		// crew, asteroid, fighter, etc. This does not mean that the ship
		// is actually controlled by a human; it may be a computer.

#define APPEARING (1 << 3)
#define DISAPPEARING (1 << 4)
#define CHANGING (1 << 5)
		// The element's graphical representation has changed.

#define NONSOLID (1 << 6)
#define COLLISION (1 << 7)
#define IGNORE_SIMILAR (1 << 8)
#define DEFY_PHYSICS (1 << 9)

#define FINITE_LIFE (1 << 10)

#define PRE_PROCESS (1 << 11)
		// PreProcess() is to be called for the ELEMENT.
#define POST_PROCESS (1 << 12)

#define IGNORE_VELOCITY (1 << 13)
#define CREW_OBJECT (1 << 14)
#define BACKGROUND_OBJECT (1 << 15)
		// The BACKGROUND_OBJECT flag existed originally but wasn't used.
		// It can now be used for objects that never influence the state
		// of other elements; elements that have this flag set are not
		// included in the checksum used for netplay games.
		// It can be used for graphical mods that don't impede netplay.


#define HYPERJUMP_LIFE 15

#define NUM_EXPLOSION_FRAMES 12

#define GAME_SOUND_PRIORITY 2

typedef enum
{
	VIEW_STABLE,
	VIEW_SCROLL,
	VIEW_CHANGE
} VIEW_STATE;

typedef UWORD ELEMENT_FLAGS;

#define NO_PRIM NUM_PRIMS

typedef struct state
{
	POINT location;
	struct
	{
		FRAME frame;
		FRAME *farray;
	} image;
} STATE;


typedef struct element ELEMENT;

typedef void (ElementProcessFunc) (ELEMENT *ElementPtr);
typedef void (ElementCollisionFunc) (ELEMENT *ElementPtr0, POINT *pPt0,
			ELEMENT *ElementPtr1, POINT *pPt1);

// Any physical object in the simulation.
struct element
{
	// LINK elements; must be first
	HELEMENT pred, succ;

	ElementProcessFunc *preprocess_func;
	ElementProcessFunc *postprocess_func;
	ElementCollisionFunc *collision_func;
	ElementProcessFunc *death_func;

	// Player this element belongs to
	// -1: neutral (planets, asteroids, crew, etc.)
	//  0: Melee: bottom player; Full-game: the human player
	//  1: Melee: top player;    Full-game: the NPC opponent
	SIZE playerNr;

	ELEMENT_FLAGS state_flags;
	union
	{
		COUNT life_span;
		COUNT scan_node; /* Planetside: scan type and node id */
	};
	union
	{
		COUNT crew_level;
		COUNT hit_points;
		COUNT facing; /* Planetside: lava-spot direction of travel */
		COUNT cycle;
				/* Planetside: lightning cycle length */
	};
	union
	{
		BYTE mass_points;
				/* Planetside:
				 * - for living bio: Index in CreatureData, possibly OR'ed
				 *   with CREATURE_AWARE
				 * - for canned bio: value of creature
				 */
		// TODO: Use a different name for Planetside bio, like
		// BYTE bio_state;
	};
	union
	{
		BYTE turn_wait;
		BYTE sys_loc; /* IP flagship: location in system */
	};
	union
	{
		BYTE thrust_wait;
		BYTE blast_offset;
		BYTE next_turn; /* Battle: animation interframe for some elements */
	};
	BYTE colorCycleIndex;
			// Melee: used to cycle ion trails and warp shadows, and
			//        to cycle the ship color when fleeing.

	VELOCITY_DESC velocity;
	INTERSECT_CONTROL IntersectControl;
	COUNT PrimIndex;
	STATE current, next;

	void *pParent;
			// The ship this element belongs to.
	HELEMENT hTarget;
};

#define NEUTRAL_PLAYER_NUM  -1

static inline BOOLEAN
elementsOfSamePlayer (ELEMENT *ElementPtr0, ELEMENT *ElementPtr1)
{
	return ElementPtr0->playerNr == ElementPtr1->playerNr;
}

extern QUEUE disp_q;
// The maximum number of elements is chosen to provide a slight margin.
// Currently, it is maximum *known used* in Melee + 30
#define MAX_DISPLAY_ELEMENTS 150

#define MAX_DISPLAY_PRIMS 330
extern COUNT DisplayFreeList;
extern PRIMITIVE DisplayArray[MAX_DISPLAY_PRIMS];

#define AllocDisplayPrim() DisplayFreeList; \
								DisplayFreeList = GetSuccLink (GetPrimLinks (&DisplayArray[DisplayFreeList]))
#define FreeDisplayPrim(p) SetPrimLinks (&DisplayArray[p], END_OF_LIST, DisplayFreeList); \
								DisplayFreeList = (p)

#define GetElementStarShip(e,ppsd) do { *(ppsd) = (e)->pParent; } while (0)
#define SetElementStarShip(e,psd)  do { (e)->pParent = psd; } while (0)

#define MAX_CREW_SIZE 42
#define MAX_ENERGY_SIZE 42
#define MAX_SHIP_MASS 10
#define GRAVITY_MASS(m) ((m) > MAX_SHIP_MASS * 10)
#define GRAVITY_THRESHOLD (COUNT)RES_SCALE(255) // JMS_GFX

#define OBJECT_CLOAKED(eptr) \
		(GetPrimType (&GLOBAL (DisplayArray[(eptr)->PrimIndex])) >= NUM_PRIMS \
		|| (GetPrimType (&GLOBAL (DisplayArray[(eptr)->PrimIndex])) == STAMPFILL_PRIM \
		&& sameColor (GetPrimColor (&GLOBAL (DisplayArray[(eptr)->PrimIndex])), BLACK_COLOR)))
#define UNDEFINED_LEVEL 0

extern HELEMENT AllocElement (void);
extern void FreeElement (HELEMENT hElement);
#define PutElement(h) PutQueue (&disp_q, h)
#define InsertElement(h,i) InsertQueue (&disp_q, h, i)
#define GetHeadElement() GetHeadLink (&disp_q)
#define GetTailElement() GetTailLink (&disp_q)
#define LockElement(h,ppe) (*(ppe) = (ELEMENT*)LockLink (&disp_q, h))
#define UnlockElement(h) UnlockLink (&disp_q, h)
#define GetPredElement(l) _GetPredLink (l)
#define GetSuccElement(l) _GetSuccLink (l)
extern void RemoveElement (HLINK hLink);

// XXX: The following functions should not really be here
extern void spawn_planet (void);
extern void spawn_asteroid (ELEMENT *ElementPtr);
extern void do_damage (ELEMENT *ElementPtr, SIZE damage);
extern void crew_preprocess (ELEMENT *ElementPtr);
extern void crew_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1);
extern void AbandonShip (ELEMENT *ShipPtr, ELEMENT *TargetPtr,
		COUNT crew_loss);
extern BOOLEAN TimeSpaceMatterConflict (ELEMENT *ElementPtr);
extern COUNT PlotIntercept (ELEMENT *ElementPtr0,
		ELEMENT *ElementPtr1, COUNT max_turns, COUNT margin_of_error);

extern void InitGalaxy (void);
extern void MoveGalaxy (VIEW_STATE view_state, SDWORD dx, SDWORD dy);

extern BOOLEAN CalculateGravity (ELEMENT *ElementPtr);


#if defined(__cplusplus)
}
#endif

#endif /* UQM_ELEMENT_H_ */

