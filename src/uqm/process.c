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

#include "process.h"

#include "races.h"
#include "collide.h"
#include "options.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "hyper.h"
#include "element.h"
#include "battle.h"
#include "weapon.h"
#include "libs/graphics/drawable.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/gfx_common.h"
#include "libs/log.h"
#include "libs/misc.h"


//#define DEBUG_PROCESS

COUNT DisplayFreeList;
PRIMITIVE DisplayArray[MAX_DISPLAY_PRIMS];
extern DPOINT SpaceOrg;

COUNT zoom_out = 1 << ZOOM_SHIFT;
static SIZE opt_max_zoom_out;

#if 0
static inline void
CALC_ZOOM_STUFF (COUNT* idx, COUNT* sc)
{
	int i, z;

	z = 1 << ZOOM_SHIFT;
	for (i = 0; (z <<= 1) <= zoom_out; i++)
		;
	*idx = i;
	*sc = ((1 << i) << (ZOOM_SHIFT + 8)) / zoom_out;
}
#else
static inline void
CALC_ZOOM_STUFF (COUNT* idx, COUNT* sc)
{
	int i;

	if (zoom_out < 2 << ZOOM_SHIFT)
		i = 0;
	else if (zoom_out < 4 << ZOOM_SHIFT)
		i = 1;
	else
		i = 2;
	*idx = i;
	*sc = (1 << (i + ZOOM_SHIFT + 8)) / zoom_out;
}
#endif

HELEMENT
AllocElement (void)
{
	HELEMENT hElement;

	hElement = AllocLink (&disp_q);
	if (hElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		memset (ElementPtr, 0, sizeof (*ElementPtr));
		ElementPtr->PrimIndex = AllocDisplayPrim ();
		if (ElementPtr->PrimIndex == END_OF_LIST)
		{
			log_add (log_Error, "AllocElement: Out of display prims!");
			explode ();
		}
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		UnlockElement (hElement);
	}

	return (hElement);
}

void
FreeElement (HELEMENT hElement)
{
	if (hElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		FreeDisplayPrim (ElementPtr->PrimIndex);
		UnlockElement (hElement);

		FreeLink (&disp_q, hElement);
	}
}

void
SetUpElement (ELEMENT *ElementPtr)
{
	ElementPtr->next = ElementPtr->current;
	if (CollidingElement (ElementPtr))
	{
		InitIntersectStartPoint (ElementPtr);
		InitIntersectEndPoint (ElementPtr);
		InitIntersectFrame (ElementPtr);
	}
}

static void
PreProcess (ELEMENT *ElementPtr)
{
	ELEMENT_FLAGS state_flags;

	if (ElementPtr->life_span == 0)
	{
		if (ElementPtr->pParent) /* untarget this dead element */
			Untarget (ElementPtr);

		ElementPtr->state_flags |= DISAPPEARING;
		if (ElementPtr->death_func)
			(*ElementPtr->death_func) (ElementPtr);
	}

	state_flags = ElementPtr->state_flags;
	if (!(state_flags & DISAPPEARING))
	{
		if (state_flags & APPEARING)
		{
			SetUpElement (ElementPtr);

			if (state_flags & PLAYER_SHIP)
				state_flags &= ~APPEARING; /* want to preprocess ship */
		}

		if (ElementPtr->preprocess_func && !(state_flags & APPEARING))
		{
			(*ElementPtr->preprocess_func) (ElementPtr);

			state_flags = ElementPtr->state_flags;
			if ((state_flags & CHANGING) && CollidingElement (ElementPtr))
				InitIntersectFrame (ElementPtr);
		}

		if (!(state_flags & IGNORE_VELOCITY))
		{
			SIZE delta_x, delta_y;

			GetNextVelocityComponents (&ElementPtr->velocity,
					&delta_x, &delta_y, 1);
			if (delta_x != 0 || delta_y != 0)
			{
				state_flags |= CHANGING;
				ElementPtr->next.location.x += delta_x;
				ElementPtr->next.location.y += delta_y;
			}
		}

		if (CollidingElement (ElementPtr))
			InitIntersectEndPoint (ElementPtr);

		if (state_flags & FINITE_LIFE)
			--ElementPtr->life_span;
	}

	ElementPtr->state_flags = (state_flags & ~(POST_PROCESS | COLLISION))
			| PRE_PROCESS;
}

static void
PostProcess (ELEMENT *ElementPtr)
{
	if (ElementPtr->postprocess_func)
		(*ElementPtr->postprocess_func) (ElementPtr);
	ElementPtr->current = ElementPtr->next;

	if (CollidingElement (ElementPtr))
	{
		InitIntersectStartPoint (ElementPtr);
		InitIntersectEndPoint (ElementPtr);
	}

	ElementPtr->state_flags = (ElementPtr->state_flags
			& ~(PRE_PROCESS | CHANGING | APPEARING))
			| POST_PROCESS;
}

static COUNT
CalcReduction (SDWORD dx, SDWORD dy)
{
	COUNT next_reduction;

#ifdef KDEBUG
	log_add (log_Debug, "CalcReduction:");
#endif

	if (optMeleeScale == TFB_SCALE_STEP)
	{
		SDWORD sdx, sdy;

		if (LOBYTE (GLOBAL (CurrentActivity)) > IN_ENCOUNTER)
			return (0);

		sdx = dx;
		sdy = dy;
		for (next_reduction = MAX_VIS_REDUCTION;
				(dx <<= REDUCTION_SHIFT) <= TRANSITION_WIDTH
				&& (dy <<= REDUCTION_SHIFT) <= TRANSITION_HEIGHT
				&& next_reduction > 0;
				next_reduction -= REDUCTION_SHIFT)
			;

				/* check for "real" zoom in */
		if (next_reduction < zoom_out
				&& zoom_out <= MAX_VIS_REDUCTION)
		{
#define HYSTERESIS_X DISPLAY_TO_WORLD(RES_SCALE(24)) // JMS_GFX
#define HYSTERESIS_Y DISPLAY_TO_WORLD(RES_SCALE(20)) // JMS_GFX
		if (((sdx + HYSTERESIS_X)
				<< (MAX_VIS_REDUCTION - next_reduction)) > TRANSITION_WIDTH
				|| ((sdy + HYSTERESIS_Y)
				<< (MAX_VIS_REDUCTION - next_reduction)) > TRANSITION_HEIGHT)
		   /* if we don't zoom in, we want to stay at next+1 */
		   next_reduction += REDUCTION_SHIFT;
		}

		if (next_reduction == 0
				&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			next_reduction += REDUCTION_SHIFT;
	}
	else
	{
		if (LOBYTE (GLOBAL (CurrentActivity)) > IN_ENCOUNTER)
			return (1 << ZOOM_SHIFT);
			
		dx = (dx * MAX_ZOOM_OUT) / (LOG_SPACE_WIDTH >> 2);
		if (dx < (1 << ZOOM_SHIFT))
			dx = 1 << ZOOM_SHIFT;
		else if (dx > MAX_ZOOM_OUT)
			dx = MAX_ZOOM_OUT;
			
		dy = (dy * MAX_ZOOM_OUT) / (LOG_SPACE_HEIGHT >> 2);
		if (dy < (1 << ZOOM_SHIFT))
			dy = 1 << ZOOM_SHIFT;
		else if (dy > MAX_ZOOM_OUT)
			dy = MAX_ZOOM_OUT;
			
		if (dy > dx)
			next_reduction = dy;
		else
			next_reduction = dx;

		if (next_reduction < (2 << ZOOM_SHIFT)
				&& LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			next_reduction = (2 << ZOOM_SHIFT);
	}

#ifdef KDEBUG
	log_add (log_Debug, "CalcReduction: exit");
#endif

	return (next_reduction);
}

static VIEW_STATE
CalcView (DPOINT *pNewScrollPt, SIZE next_reduction,
		SDWORD *pdx, SDWORD *pdy, COUNT ships_alive)
{
	SDWORD dx, dy;
	VIEW_STATE view_state;

#ifdef KDEBUG
	log_add (log_Debug, "CalcView:");
#endif
	dx = ((SDWORD)(LOG_SPACE_WIDTH >> 1) - pNewScrollPt->x);
	dy = ((SDWORD)(LOG_SPACE_HEIGHT >> 1) - pNewScrollPt->y);
	dx = WRAP_DELTA_X (dx);
	dy = WRAP_DELTA_Y (dy);
	if (ships_alive == 1)
	{
#define ORG_JUMP_X ((SDWORD)DISPLAY_ALIGN(LOG_SPACE_WIDTH / 75))
#define ORG_JUMP_Y ((SDWORD)DISPLAY_ALIGN(LOG_SPACE_HEIGHT / 75))
		if (dx > ORG_JUMP_X)
			dx = ORG_JUMP_X;
		else if (dx < -ORG_JUMP_X)
			dx = -ORG_JUMP_X;
		if (dy > ORG_JUMP_Y)
			dy = ORG_JUMP_Y;
		else if (dy < -ORG_JUMP_Y)
			dy = -ORG_JUMP_Y;
	}

	if ((dx || dy) && inHQSpace ())
		MoveSIS (&dx, &dy);

	if (zoom_out == next_reduction)
		view_state = dx == 0 && dy == 0 && !inHQSpace ()
				? VIEW_STABLE : VIEW_SCROLL;
	else
	{
		if (optMeleeScale == TFB_SCALE_STEP)
		{
			SpaceOrg.x = (SDWORD)(LOG_SPACE_WIDTH >> 1) - (LOG_SPACE_WIDTH >> ((MAX_REDUCTION + 1) - next_reduction));
			SpaceOrg.y = (SDWORD)(LOG_SPACE_HEIGHT >> 1) - (LOG_SPACE_HEIGHT >> ((MAX_REDUCTION + 1) - next_reduction));
		}
		else
		{
#define ZOOM_JUMP ((1 << ZOOM_SHIFT) >> 3)
			if (ships_alive == 1
					&& zoom_out > next_reduction
					&& zoom_out <= MAX_ZOOM_OUT
					&& zoom_out - next_reduction > ZOOM_JUMP)
				next_reduction = zoom_out - ZOOM_JUMP;
				
			// Always align the origin on a whole pixel to reduce the
			// amount of object positioning jitter
			SpaceOrg.x = DISPLAY_ALIGN((int)(LOG_SPACE_WIDTH >> 1) - (LOG_SPACE_WIDTH * next_reduction / (MAX_ZOOM_OUT << 2)));
			SpaceOrg.y = DISPLAY_ALIGN((int)(LOG_SPACE_HEIGHT >> 1) - (LOG_SPACE_HEIGHT * next_reduction / (MAX_ZOOM_OUT << 2)));
 		
		}
		zoom_out = next_reduction;
		view_state = VIEW_CHANGE;
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) <= IN_HYPERSPACE)
		MoveGalaxy (view_state, dx, dy);

	*pdx = dx;
	*pdy = dy;

#ifdef KDEBUG
	log_add (log_Debug, "CalcView: exit");
#endif
	return (view_state);
}


static ELEMENT_FLAGS
ProcessCollisions (HELEMENT hSuccElement, ELEMENT *ElementPtr,
		TIME_VALUE min_time, ELEMENT_FLAGS process_flags)
{
	HELEMENT hTestElement;

	while ((hTestElement = hSuccElement) != 0)
	{
		ELEMENT *TestElementPtr;

		LockElement (hTestElement, &TestElementPtr);
		if (!(TestElementPtr->state_flags & process_flags))
			PreProcess (TestElementPtr);
		hSuccElement = GetSuccElement (TestElementPtr);

		if (TestElementPtr == ElementPtr)
		{
			UnlockElement (hTestElement);
			continue;
		}

		if (CollisionPossible (TestElementPtr, ElementPtr))
		{
			ELEMENT_FLAGS state_flags, test_state_flags;
			TIME_VALUE time_val;

			state_flags = ElementPtr->state_flags;
			test_state_flags = TestElementPtr->state_flags;
			if (((state_flags | test_state_flags) & FINITE_LIFE)
					&& (((state_flags & APPEARING)
					&& ElementPtr->life_span > 1)
					|| ((test_state_flags & APPEARING)
					&& TestElementPtr->life_span > 1)))
				time_val = 0;
			else
			{
				while ((time_val = DrawablesIntersect (&ElementPtr->IntersectControl,
						&TestElementPtr->IntersectControl, min_time)) == 1
						&& !((state_flags | test_state_flags) & FINITE_LIFE))
				{
#ifdef DEBUG_PROCESS
					log_add (log_Debug, "BAD NEWS 0x%x <--> 0x%x", ElementPtr,
							TestElementPtr);
#endif /* DEBUG_PROCESS */
					if (state_flags & COLLISION)
					{
						InitIntersectEndPoint (TestElementPtr);
						TestElementPtr->IntersectControl.IntersectStamp.origin =
								TestElementPtr->IntersectControl.EndPoint;
						time_val = DrawablesIntersect (&ElementPtr->IntersectControl,
								&TestElementPtr->IntersectControl, 1);
						InitIntersectStartPoint (TestElementPtr);
					}

					if (time_val == 1)
					{
						FRAME CurFrame, NextFrame,
								TestCurFrame, TestNextFrame;

						CurFrame = ElementPtr->current.image.frame;
						NextFrame = ElementPtr->next.image.frame;
						TestCurFrame = TestElementPtr->current.image.frame;
						TestNextFrame = TestElementPtr->next.image.frame;
						if (NextFrame == CurFrame
								&& TestNextFrame == TestCurFrame)
						{
							if (test_state_flags & APPEARING)
							{
								do_damage (TestElementPtr, TestElementPtr->hit_points);
								if (TestElementPtr->pParent) /* untarget this dead element */
									Untarget (TestElementPtr);

								TestElementPtr->state_flags |= (COLLISION | DISAPPEARING);
								if (TestElementPtr->death_func)
									(*TestElementPtr->death_func) (TestElementPtr);
							}
							if (state_flags & APPEARING)
							{
								do_damage (ElementPtr, ElementPtr->hit_points);
								if (ElementPtr->pParent) /* untarget this dead element */
									Untarget (ElementPtr);

								ElementPtr->state_flags |= (COLLISION | DISAPPEARING);
								if (ElementPtr->death_func)
									(*ElementPtr->death_func) (ElementPtr);

								UnlockElement (hTestElement);
								return (COLLISION);
							}

							time_val = 0;
						}
						else
						{
							if (GetFrameIndex (CurFrame) !=
									GetFrameIndex (NextFrame))
								ElementPtr->next.image.frame =
										SetEquFrameIndex (NextFrame,
										CurFrame);
							else if (NextFrame != CurFrame)
							{
								ElementPtr->next.image =
										ElementPtr->current.image;
								if (ElementPtr->life_span > NORMAL_LIFE)
									ElementPtr->life_span = NORMAL_LIFE;
							}

							if (GetFrameIndex (TestCurFrame) !=
									GetFrameIndex (TestNextFrame))
								TestElementPtr->next.image.frame =
										SetEquFrameIndex (TestNextFrame,
										TestCurFrame);
							else if (TestNextFrame != TestCurFrame)
							{
								TestElementPtr->next.image =
										TestElementPtr->current.image;
								if (TestElementPtr->life_span > NORMAL_LIFE)
									TestElementPtr->life_span = NORMAL_LIFE;
							}

							InitIntersectStartPoint (ElementPtr);
							InitIntersectEndPoint (ElementPtr);
							InitIntersectFrame (ElementPtr);
							if (state_flags & PLAYER_SHIP)
							{
								STARSHIP *StarShipPtr;

								GetElementStarShip (ElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
										GetFrameIndex (
										ElementPtr->next.image.frame);
							}

							InitIntersectStartPoint (TestElementPtr);
							InitIntersectEndPoint (TestElementPtr);
							InitIntersectFrame (TestElementPtr);
							if (test_state_flags & PLAYER_SHIP)
							{
								STARSHIP *StarShipPtr;

								GetElementStarShip (TestElementPtr, &StarShipPtr);
								StarShipPtr->ShipFacing =
										GetFrameIndex (
										TestElementPtr->next.image.frame);
							}
						}
					}

					if (time_val == 0)
					{
						InitIntersectEndPoint (ElementPtr);
						InitIntersectEndPoint (TestElementPtr);

						break;
					}
				}
			}

			if (time_val > 0)
			{
				POINT SavePt, TestSavePt;

#ifdef DEBUG_PROCESS
				log_add (log_Debug, "0x%x <--> 0x%x at %u", ElementPtr,
						TestElementPtr, time_val);
#endif /* DEBUG_PROCESS */
				SavePt = ElementPtr->IntersectControl.EndPoint;
				TestSavePt = TestElementPtr->IntersectControl.EndPoint;
				InitIntersectEndPoint (ElementPtr);
				InitIntersectEndPoint (TestElementPtr);
				if (time_val == 1
						|| (((state_flags & COLLISION)
						|| !ProcessCollisions (hSuccElement, ElementPtr,
						time_val - 1, process_flags))
						&& ((test_state_flags & COLLISION)
						|| !ProcessCollisions (
						!(TestElementPtr->state_flags & APPEARING) ?
						GetSuccElement (ElementPtr) :
						GetHeadElement (), TestElementPtr,
						time_val - 1, process_flags))))
				{
					state_flags = ElementPtr->state_flags;
					test_state_flags = TestElementPtr->state_flags;

#ifdef DEBUG_PROCESS
					log_add (log_Debug, "PROCESSING 0x%x <--> 0x%x at %u",
							ElementPtr, TestElementPtr, time_val);
#endif /* DEBUG_PROCESS */
					if (test_state_flags & PLAYER_SHIP)
					{
						(*TestElementPtr->collision_func) (
								TestElementPtr, &TestSavePt,
								ElementPtr, &SavePt
								);
						(*ElementPtr->collision_func) (
								ElementPtr, &SavePt,
								TestElementPtr, &TestSavePt
								);
					}
					else
					{
						(*ElementPtr->collision_func) (
								ElementPtr, &SavePt,
								TestElementPtr, &TestSavePt
								);
						(*TestElementPtr->collision_func) (
								TestElementPtr, &TestSavePt,
								ElementPtr, &SavePt
								);
					}

					if (TestElementPtr->state_flags & COLLISION)
					{
						if (!(test_state_flags & COLLISION))
						{
							TestElementPtr->IntersectControl.IntersectStamp.origin =
									TestSavePt;
							TestElementPtr->next.location.x =
									DISPLAY_TO_WORLD (TestSavePt.x);
							TestElementPtr->next.location.y =
									DISPLAY_TO_WORLD (TestSavePt.y);
							InitIntersectEndPoint (TestElementPtr);
						}
					}

					if (ElementPtr->state_flags & COLLISION)
					{
						if (!(state_flags & COLLISION))
						{
							ElementPtr->IntersectControl.IntersectStamp.origin =
									SavePt;
							ElementPtr->next.location.x =
									DISPLAY_TO_WORLD (SavePt.x);
							ElementPtr->next.location.y =
									DISPLAY_TO_WORLD (SavePt.y);
							InitIntersectEndPoint (ElementPtr);

							if (!(state_flags & FINITE_LIFE) &&
									!(test_state_flags & FINITE_LIFE))
							{
								collide (ElementPtr, TestElementPtr);

								ProcessCollisions (GetHeadElement (), ElementPtr,
										MAX_TIME_VALUE, process_flags);
								ProcessCollisions (GetHeadElement (), TestElementPtr,
										MAX_TIME_VALUE, process_flags);
							}
						}
						UnlockElement (hTestElement);
						return (COLLISION);
					}

					if (!CollidingElement (ElementPtr))
					{
						ElementPtr->state_flags |= COLLISION;
						UnlockElement (hTestElement);
						return (COLLISION);
					}
				}
			}
		}

		UnlockElement (hTestElement);
	}

	return (ElementPtr->state_flags & COLLISION);
}

static VIEW_STATE
PreProcessQueue (SDWORD *pscroll_x, SDWORD *pscroll_y)
{
	SIZE min_reduction, max_reduction;
	COUNT sides_active;
	DPOINT Origin;
	HELEMENT hElement;
	COUNT ships_alive;

#ifdef KDEBUG
	log_add (log_Debug, "PreProcess:");
#endif
	sides_active = (battle_counter[0] ? 1 : 0)
			+ (battle_counter[1] ? 1 : 0);

	if (optMeleeScale == TFB_SCALE_STEP)
		min_reduction = max_reduction = MAX_VIS_REDUCTION + 1;
	else
		min_reduction = max_reduction = MAX_ZOOM_OUT + (1 << ZOOM_SHIFT);

	Origin.x = (SDWORD)(LOG_SPACE_WIDTH >> 1);
	Origin.y = (SDWORD)(LOG_SPACE_HEIGHT >> 1);

	hElement = GetHeadElement ();
	ships_alive = 0;
	while (hElement != 0)
	{
		ELEMENT *ElementPtr;
		HELEMENT hNextElement;

		LockElement (hElement, &ElementPtr);

		if (!(ElementPtr->state_flags & PRE_PROCESS))
			PreProcess (ElementPtr);
		hNextElement = GetSuccElement (ElementPtr);

		if (CollidingElement (ElementPtr)
				&& !(ElementPtr->state_flags & COLLISION))
			ProcessCollisions (hNextElement, ElementPtr,
					MAX_TIME_VALUE, PRE_PROCESS);

		if (ElementPtr->state_flags & PLAYER_SHIP)
		{
			SDWORD dx, dy;

			ships_alive++;
			if (max_reduction > opt_max_zoom_out
					&& min_reduction > opt_max_zoom_out)
			{
				Origin.x = DISPLAY_ALIGN (ElementPtr->next.location.x);
				Origin.y = DISPLAY_ALIGN (ElementPtr->next.location.y);
			}

			dx = DISPLAY_ALIGN (ElementPtr->next.location.x) - Origin.x;
			dx = WRAP_DELTA_X (dx);
			dy = DISPLAY_ALIGN (ElementPtr->next.location.y) - Origin.y;
			dy = WRAP_DELTA_Y (dy);

			if (sides_active <= 2 || ElementPtr->playerNr == 0)
			{
				Origin.x = DISPLAY_ALIGN (Origin.x + (dx >> 1));
				Origin.y = DISPLAY_ALIGN (Origin.y + (dy >> 1));

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				max_reduction = CalcReduction (dx, dy);
			}
			else if (max_reduction > opt_max_zoom_out
					&& min_reduction <= opt_max_zoom_out)
			{
				Origin.x = DISPLAY_ALIGN (Origin.x + (dx >> 1));
				Origin.y = DISPLAY_ALIGN (Origin.y + (dy >> 1));

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				min_reduction = CalcReduction (dx, dy);
			}
			else
			{
				SIZE reduction;

				if (dx < 0)
					dx = -dx;
				if (dy < 0)
					dy = -dy;
				reduction = CalcReduction (dx << 1, dy << 1);

				if (min_reduction > opt_max_zoom_out
						|| reduction < min_reduction)
					min_reduction = reduction;
			}
//			log_add (log_Debug, "dx = %d dy = %d min_red = %d max_red = %d",
//					dx, dy, min_reduction, max_reduction);
		}

		UnlockElement (hElement);
		hElement = hNextElement;
	}

	if ((min_reduction > opt_max_zoom_out || min_reduction <= max_reduction)
			&& (min_reduction = max_reduction) > opt_max_zoom_out
			&& (min_reduction = zoom_out) > opt_max_zoom_out)
	{
		if (optMeleeScale == TFB_SCALE_STEP)
			min_reduction = 0;
		else
			min_reduction = 1 << ZOOM_SHIFT;
	}

#ifdef KDEBUG
	log_add (log_Debug, "PreProcess: exit");
#endif
	return (CalcView (&Origin, min_reduction, pscroll_x, pscroll_y, ships_alive));
}

void
InsertPrim (PRIM_LINKS *pLinks, COUNT primIndex, COUNT iPI)
{
	COUNT Link;
	PRIM_LINKS PL;

	if (iPI == END_OF_LIST)
	{
		Link = GetSuccLink (*pLinks); /* get tail */
		if (Link == END_OF_LIST)
			*pLinks = MakeLinks (primIndex, primIndex);
		else
			*pLinks = MakeLinks (GetPredLink (*pLinks), primIndex);
	}
	else
	{
		PL = GetPrimLinks (&DisplayArray[iPI]);
		if (iPI != GetPredLink (*pLinks)) /* if not the head */
			Link = GetPredLink (PL);
		else
		{
			Link = END_OF_LIST;
			*pLinks = MakeLinks (primIndex, GetSuccLink (*pLinks));
		}
		SetPrimLinks (&DisplayArray[iPI], primIndex, GetSuccLink (PL));
	}

	if (Link != END_OF_LIST)
	{
		PL = GetPrimLinks (&DisplayArray[Link]);
		SetPrimLinks (&DisplayArray[Link], GetPredLink (PL), primIndex);
	}
	SetPrimLinks (&DisplayArray[primIndex], Link, iPI);
}

PRIM_LINKS DisplayLinks;

static inline COORD
CalcDisplayCoord (SDWORD c, SDWORD orgc, SIZE reduction)
{
	if (optMeleeScale == TFB_SCALE_STEP)
	{	/* old fixed-step zoom style */
		return (c - orgc) >> reduction;
	}
	else
	{	/* new continuous zoom style */
		return ((c - orgc) << ZOOM_SHIFT) / reduction;
	}
}

static void
PostProcessQueue (VIEW_STATE view_state, SDWORD scroll_x, SDWORD scroll_y)
{
	DPOINT delta;
	SIZE reduction;
	HELEMENT hElement;

#ifdef KDEBUG
	log_add (log_Debug, "PostProcess:");
#endif
	if (optMeleeScale == TFB_SCALE_STEP)
		reduction = zoom_out + ONE_SHIFT;
	else
		reduction = zoom_out << ONE_SHIFT;

	hElement = GetHeadElement ();
	while (hElement != 0)
	{
		ELEMENT_FLAGS state_flags;
		ELEMENT *ElementPtr;
		HELEMENT hNextElement;

		LockElement (hElement, &ElementPtr);

		state_flags = ElementPtr->state_flags;
		if (state_flags & PRE_PROCESS)
		{
			if (!(state_flags & COLLISION))
				ElementPtr->state_flags &= ~DEFY_PHYSICS;
			else
				ElementPtr->state_flags &= ~COLLISION;

			if (state_flags & POST_PROCESS)
			{
				delta.x = 0;
				delta.y = 0;
			}
			else
			{
				delta.x = scroll_x;
				delta.y = scroll_y;
			}
		}
		else
		{
			HELEMENT hPostElement;

			hPostElement = hElement;
			do
			{
				ELEMENT *PostElementPtr;

				LockElement (hPostElement, &PostElementPtr);
				if (!(PostElementPtr->state_flags & PRE_PROCESS))
					PreProcess (PostElementPtr);
				hNextElement = GetSuccElement (PostElementPtr);

				if (CollidingElement (PostElementPtr)
						&& !(PostElementPtr->state_flags & COLLISION))
					ProcessCollisions (GetHeadElement (), PostElementPtr,
							MAX_TIME_VALUE, PRE_PROCESS | POST_PROCESS);
				UnlockElement (hPostElement);
				hPostElement = hNextElement;
			} while (hPostElement != 0);

			scroll_x = 0;
			scroll_y = 0;
			delta.x = 0;
			delta.y = 0;
					/* because these are newly added elements that are
					 * already in adjusted coordinates */
			state_flags = ElementPtr->state_flags;
		}

		if (state_flags & DISAPPEARING)
		{
			hNextElement = GetSuccElement (ElementPtr);
			UnlockElement (hElement);
			RemoveElement (hElement);
			FreeElement (hElement);
		}
		else
		{
			GRAPHICS_PRIM ObjType;

			ObjType = GetPrimType (&DisplayArray[ElementPtr->PrimIndex]);
			if (view_state != VIEW_STABLE
					|| (state_flags & (APPEARING | CHANGING)))
			{
				POINT next;

				if (ObjType == LINE_PRIM)
				{
					SDWORD dx, dy;
					
					dx = (SDWORD)ElementPtr->next.location.x - (SDWORD)ElementPtr->current.location.x;
					dy = (SDWORD)ElementPtr->next.location.y - (SDWORD)ElementPtr->current.location.y;
					
					next.x = WRAP_X ((SDWORD)ElementPtr->current.location.x + (SDWORD)delta.x);
					next.y = WRAP_Y ((SDWORD)ElementPtr->current.location.y + (SDWORD)delta.y);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x = CalcDisplayCoord (next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y = CalcDisplayCoord (next.y, SpaceOrg.y, reduction);

					next.x += dx;
					next.y += dy;
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.x = CalcDisplayCoord (next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Line.second.y = CalcDisplayCoord (next.y, SpaceOrg.y, reduction);
				}
				else
				{
					next.x = WRAP_X ((SDWORD)ElementPtr->next.location.x + (SDWORD)delta.x);
					next.y = WRAP_Y ((SDWORD)ElementPtr->next.location.y + (SDWORD)delta.y);
					
					DisplayArray[ElementPtr->PrimIndex].Object.Point.x = CalcDisplayCoord (next.x, SpaceOrg.x, reduction);
					DisplayArray[ElementPtr->PrimIndex].Object.Point.y = CalcDisplayCoord (next.y, SpaceOrg.y, reduction);

					if (ObjType == STAMP_PRIM || ObjType == STAMPFILL_PRIM)
					{
						if (view_state == VIEW_CHANGE
								|| (state_flags & (APPEARING | CHANGING)))
						{
							COUNT index, scale = GSCALE_IDENTITY;
						
							if (optMeleeScale == TFB_SCALE_STEP)
								index = zoom_out;
							else
								CALC_ZOOM_STUFF (&index, &scale);

							ElementPtr->next.image.frame = SetEquFrameIndex (
									ElementPtr->next.image.farray[index],
									ElementPtr->next.image.frame);

							if (optMeleeScale == TFB_SCALE_TRILINEAR &&
									index < 2 && scale != GSCALE_IDENTITY)
							{
								// enqueues drawcommand to assign next
								// (smaller) zoom level image as mipmap,
								// needed for trilinear scaling

								FRAME frame = ElementPtr->next.image.frame;
								FRAME mmframe = SetEquFrameIndex (
										ElementPtr->next.image.farray[
										index + 1], frame);

								// TODO: This is currently hacky, this code
								//   really should not dereference FRAME.
								//   Perhaps make mipmap part of STAMP prim?
								if (frame && mmframe)
								{
									HOT_SPOT mmhs = GetFrameHot (mmframe);
									TFB_DrawScreen_SetMipmap (frame->image,
											mmframe->image, mmhs.x, mmhs.y);
								}
							}
						}
						DisplayArray[ElementPtr->PrimIndex].Object.Stamp.frame =
								ElementPtr->next.image.frame;
					}
				}

				ElementPtr->next.location = next;
			}

			PostProcess (ElementPtr);

			if (ObjType < NUM_PRIMS)
				InsertPrim (&DisplayLinks, ElementPtr->PrimIndex, END_OF_LIST);

			hNextElement = GetSuccElement (ElementPtr);
			UnlockElement (hElement);
		}

		hElement = hNextElement;
	}
#ifdef KDEBUG
	log_add (log_Debug, "PostProcess: exit");
#endif
}

void
InitDisplayList (void)
{
	COUNT i;

	if (optMeleeScale == TFB_SCALE_STEP)
	{
		zoom_out = MAX_VIS_REDUCTION + 1;
		opt_max_zoom_out = MAX_VIS_REDUCTION;
	}
	else
	{
		zoom_out = MAX_ZOOM_OUT + (1 << ZOOM_SHIFT);
		opt_max_zoom_out = MAX_ZOOM_OUT;
	}

	ReinitQueue (&disp_q);

	for (i = 0; i < MAX_DISPLAY_PRIMS; ++i)
		SetPrimLinks (&DisplayArray[i], END_OF_LIST, i + 1);
	SetPrimLinks (&DisplayArray[i - 1], END_OF_LIST, END_OF_LIST);
	DisplayFreeList = 0;
	DisplayLinks = MakeLinks (END_OF_LIST, END_OF_LIST);
}

UWORD nth_frame = 0;

void
RedrawQueue (BOOLEAN clear)
{
	SDWORD scroll_x, scroll_y;
	VIEW_STATE view_state;

	SetContext (StatusContext);

	view_state = PreProcessQueue (&scroll_x, &scroll_y);
	PostProcessQueue (view_state, scroll_x, scroll_y);

	if (optStereoSFX)
		UpdateSoundPositions ();

	SetContext (SpaceContext);
	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE
		|| !(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		BYTE skip_frames;

		skip_frames = HIBYTE (nth_frame);
		if (skip_frames != (BYTE)~0
			&& (skip_frames == 0 || (--nth_frame & 0x00FF) == 0))
		{
			nth_frame += skip_frames;
			if (clear)
				ClearDrawable (); // this is for BATCH_BUILD_PAGE effect, but not scaled by SetGraphicScale

			if (optMeleeScale != TFB_SCALE_STEP)
			{
				COUNT index, scale;

				CALC_ZOOM_STUFF (&index, &scale);
				SetGraphicScale (scale);
			}

			DrawBatch (DisplayArray, DisplayLinks, 0);//BATCH_BUILD_PAGE);
			SetGraphicScale (0);
		}

		FlushSounds ();
	}
	else
	{	// sfx queue needs to be flushed when aborting
		ProcessSound ((SOUND)~0, NULL);
		FlushSounds ();
	}

	DisplayLinks = MakeLinks (END_OF_LIST, END_OF_LIST);
}

// Set the hTarget field to 0 for all elements in the display list that
// have hTarget set to ElementPtr.
void
Untarget (ELEMENT *ElementPtr)
{
	HELEMENT hElement, hNextElement;

	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		HELEMENT hTarget;
		ELEMENT *ListPtr;

		LockElement (hElement, &ListPtr);
		hNextElement = GetSuccElement (ListPtr);

		hTarget = ListPtr->hTarget;
		if (hTarget)
		{
			ELEMENT *TargetElementPtr;

			LockElement (hTarget, &TargetElementPtr);
			if (TargetElementPtr == ElementPtr)
				ListPtr->hTarget = 0;
			UnlockElement (hTarget);
		}

		UnlockElement (hElement);
	}
}

void
RemoveElement (HLINK hLink)
{
	if (optStereoSFX)
	{
		ELEMENT *ElementPtr;

		LockElement (hLink, &ElementPtr);
		if (ElementPtr != NULL)
			RemoveSoundsForObject(ElementPtr);
		UnlockElement (hLink);
	}
	RemoveQueue (&disp_q, hLink);
}


