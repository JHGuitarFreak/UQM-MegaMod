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

#include "hyper.h"

#include "build.h"
#include "collide.h"
#include "colors.h"
#include "controls.h"
#include "gameopt.h"
#include "menustat.h"
		// for DrawMenuStateStrings()
#include "encount.h"
#include "starmap.h"
#include "ship.h"
#include "shipcont.h"
#include "process.h"
#include "globdata.h"
#include "sis.h"
#include "units.h"
#include "init.h"
#include "nameref.h"
#include "resinst.h"
#include "setup.h"
#include "sounds.h"
#include "options.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "libs/log.h"

#define XOFFS ((RADAR_SCAN_WIDTH + (UNIT_SCREEN_WIDTH << 2)) >> 1)
#define YOFFS ((RADAR_SCAN_HEIGHT + (UNIT_SCREEN_HEIGHT << 2)) >> 1)

static FRAME npcbubble;			// BW: animated bubble
static FRAME quasiportal;       // JMS: animated quasispace portal in hyperspace
static FRAME Falayalaralfali;        // JMS: Arilou homeworld in quasispace
static FRAME hyperholes[3];		// BW: One for each flavour of space
static FRAME hyperstars[4];
static COLORMAP hypercmaps[2];
static BYTE fuel_ticks;
static COUNT hyper_dx, hyper_dy, hyper_extra;

// HyperspaceMenu() items
enum HyperMenuItems
{
	// XXX: Must match the enum in menustat.h
	STARMAP = 1,
	EQUIP_DEVICE,
	CARGO,
	ROSTER,
	GAME_MENU,
	NAVIGATION,
};

/*
 * draws the melee icon for the battle group inside the black holes,
 * so you can see who's chasing you.
 */
static void
decorate_vortex(ELEMENT * ElementPtr)
{
	HENCOUNTER hEncounter, hNextEncounter;
	FRAME f = NULL;
	static FRAME vortex_ships[NUM_AVAILABLE_RACES];

	// The element is still spawning, nothing to do yet
	if (ElementPtr->death_func)
		return;

	// The element doesn't know what kind of ship it is, that
	// info is stored in the encounter queue.  I'm guessing this
	// needs refactoring
	for (hEncounter = GetHeadEncounter();
		hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER *EncounterPtr;

		LockEncounter(hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter(EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT *EncounterElementPtr;

			LockElement(EncounterPtr->hElement, &EncounterElementPtr);
			if (EncounterElementPtr == ElementPtr)
			{
				HFLEETINFO hFleet;
				FLEET_INFO *FleetPtr;

				if (vortex_ships[EncounterPtr->race_id])
				{
					if (ElementPtr->next.image.frame != vortex_ships[EncounterPtr->race_id])
						ElementPtr->next.image.frame = vortex_ships[EncounterPtr->race_id];
				}
				else
				{
					hFleet = GetStarShipFromIndex(&GLOBAL(avail_race_q),
						EncounterPtr->race_id);
					if (hFleet)
					{
						FleetPtr = LockFleetInfo(&GLOBAL(avail_race_q),
							hFleet);
						f = SetAbsFrameIndex(FleetPtr->melee_icon, 1);
						UnlockFleetInfo(&GLOBAL(avail_race_q), hFleet);
					}

					// now make a frame, and use a context to scribble
					// into it with DrawStamp().  uses a static array to
					// reuse generated frames for the life of the game
					// (or multiple games) as a dodge around figuring out
					// a sensible memory management strategy  ;)
					if (f)
					{
						CONTEXT tmp, old;
						Color trans;
						STAMP s;

						vortex_ships[EncounterPtr->race_id] = CaptureDrawable(
							CreateDrawable(WANT_PIXMAP | WANT_ALPHA,
								GetFrameWidth(ElementPtr->next.image.frame),
								GetFrameHeight(ElementPtr->next.image.frame), 1));
						tmp = CreateContext("HyperSpaceContext");
						old = SetContext(tmp);
						SetContextFGFrame(vortex_ships[EncounterPtr->race_id]);
						trans = BUILD_COLOR(MAKE_RGB15(0x10, 0x00, 0x10), 0x00);
						SetContextBackGroundColor(trans);
						ClearDrawable();
						SetFrameTransparentColor(vortex_ships[EncounterPtr->race_id], trans);

						// the original element
						s.frame = ElementPtr->current.image.frame;
						s.origin = GetFrameHot(s.frame);
						DrawStamp(&s);

						// the overlaid gfx
						s.frame = f;
						DrawStamp(&s);

						// important to make sure the
						// collision animation looks correct
						SetFrameHot(vortex_ships[EncounterPtr->race_id], s.origin);

						// cleanup
						SetContext(old);
						DestroyContext(tmp);
						// vortex_ships[EncounterPtr->race_id]->parent = ElementPtr->current.image.frame->parent;
						// ElementPtr->next.image.frame = vortex_ships[EncounterPtr->race_id];
					}
				}
			}
			UnlockElement(EncounterPtr->hElement);
		}
		UnlockEncounter(hEncounter);
	}
}

void
MoveSIS (SDWORD *pdx, SDWORD *pdy)
{
	SDWORD new_dx, new_dy;

	new_dx = *pdx;
	GLOBAL_SIS (log_x) -= new_dx;
	if (GLOBAL_SIS (log_x) < 0)
	{
		new_dx += (SIZE)GLOBAL_SIS (log_x);
		GLOBAL_SIS (log_x) = 0;
	}
	else if (GLOBAL_SIS (log_x) > MAX_X_LOGICAL)
	{
		new_dx += (SIZE)(GLOBAL_SIS (log_x) - MAX_X_LOGICAL);
		GLOBAL_SIS (log_x) = MAX_X_LOGICAL;
	}

	new_dy = *pdy;
	GLOBAL_SIS (log_y) -= new_dy;
	if (GLOBAL_SIS (log_y) < 0)
	{
		new_dy += (SIZE)GLOBAL_SIS (log_y);
		GLOBAL_SIS (log_y) = 0;
	}
	else if (GLOBAL_SIS (log_y) > MAX_Y_LOGICAL)
	{
		new_dy += (SIZE)(GLOBAL_SIS (log_y) - MAX_Y_LOGICAL);
		GLOBAL_SIS (log_y) = MAX_Y_LOGICAL;
	}

	if (new_dx != *pdx || new_dy != *pdy)
	{
		HELEMENT hElement, hNextElement;

		*pdx = new_dx;
		*pdy = new_dy;

		for (hElement = GetTailElement ();
				hElement != 0; hElement = hNextElement)
		{
			ELEMENT *ElementPtr;

			LockElement (hElement, &ElementPtr);

			if (!(ElementPtr->state_flags & PLAYER_SHIP))
				hNextElement = GetPredElement (ElementPtr);
			else
			{
				ElementPtr->next.location.x = (LOG_SPACE_WIDTH >> 1) - new_dx;
				ElementPtr->next.location.y = (LOG_SPACE_HEIGHT >> 1) - new_dy;
				hNextElement = 0;
			}

			UnlockElement (hElement);
		}
	}

	if (GLOBAL_SIS (FuelOnBoard) && GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		COUNT cur_fuel_ticks;
		COUNT hyper_dist;
		DWORD adj_dx, adj_dy;

		if (new_dx < 0)
			new_dx = -new_dx;
		hyper_dx += new_dx;
		if (new_dy < 0)
			new_dy = -new_dy;
		hyper_dy += new_dy;

		/* These macros are also used in the fuel estimate on the starmap. */
		adj_dx = LOGX_TO_UNIVERSE(16 * hyper_dx);
		adj_dy = MAX_Y_UNIVERSE - LOGY_TO_UNIVERSE(16 * hyper_dy);

		hyper_dist = square_root (adj_dx * adj_dx + adj_dy * adj_dy) 
					+ hyper_extra;
		cur_fuel_ticks = hyper_dist >> 4;

		if (cur_fuel_ticks > (COUNT)fuel_ticks)
		{
#ifndef TESTING
			if (!optInfiniteFuel)
				DeltaSISGauges (0, fuel_ticks - cur_fuel_ticks, 0);
			
#endif /* TESTING */
			if (cur_fuel_ticks > 0x00FF)
			{
				hyper_dx = 0;
				hyper_extra = hyper_dist & ((1 << 4) - 1);
				hyper_dy = 0;
				cur_fuel_ticks = 0;
			}

			fuel_ticks = (BYTE)cur_fuel_ticks;
		}
	}
}

void
check_hyperspace_encounter (void)
{
	BYTE Type;
	POINT universe;
	HFLEETINFO hStarShip, hNextShip;
	COUNT EncounterPercent[] =
	{
		RACE_HYPERSPACE_PERCENT
	};
	
	universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q)), Type = 0;
			hStarShip && (GLOBAL (CurrentActivity) & IN_BATTLE);
			hStarShip = hNextShip, ++Type)
	{
		COUNT encounter_radius;
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		encounter_radius = FleetPtr->actual_strength;
		if (encounter_radius)
		{
			BYTE encounter_flags;
			SIZE dx, dy;
			COUNT percent;
			HENCOUNTER hEncounter;
			ENCOUNTER *EncounterPtr;

			encounter_flags = 0;
			percent = EncounterPercent[Type];
			
			if (encounter_radius != INFINITE_RADIUS)
			{
				encounter_radius =
						(encounter_radius * SPHERE_RADIUS_INCREMENT) >> 1;
			}
			else /* encounter_radius == infinity */
			{
				HENCOUNTER hNextEncounter;

				encounter_radius = (MAX_X_UNIVERSE + 1) << 1;
				if (Type == SLYLANDRO_SHIP)
				{
					encounter_flags = ONE_SHOT_ENCOUNTER;
					if (!GET_GAME_STATE (STARBASE_AVAILABLE) && !NOMAD)
						percent = 100;
					else
						percent *= GET_GAME_STATE (SLYLANDRO_MULTIPLIER);
				}
				else if (Type == MELNORME_SHIP
						&& (GLOBAL_SIS (FuelOnBoard) == 0
						|| GET_GAME_STATE (USED_BROADCASTER))
						&& GET_GAME_STATE (MELNORME_ANGER) < 3)
				{
					if (!GET_GAME_STATE (USED_BROADCASTER))
						percent = 30;
					else
						percent = 100;
					encounter_flags = ONE_SHOT_ENCOUNTER;
				}

				// There can be only one! (of either Slylandro or Melnorme)
				for (hEncounter = GetHeadEncounter ();
						hEncounter; hEncounter = hNextEncounter)
				{
					LockEncounter (hEncounter, &EncounterPtr);
					hNextEncounter = GetSuccEncounter (EncounterPtr);
					if (EncounterPtr->race_id == Type)
					{
						percent = 0;
						hNextEncounter = 0;
					}
					UnlockEncounter (hEncounter);
				}


				if (percent == 100 && Type == MELNORME_SHIP)
				{
					SET_GAME_STATE (BROADCASTER_RESPONSE, 1);
				}
			}

			dx = universe.x - FleetPtr->loc.x;
			if (dx < 0)
				dx = -dx;
			dy = universe.y - FleetPtr->loc.y;
			if (dy < 0)
				dy = -dy;
			if ((COUNT)dx < encounter_radius
					&& (COUNT)dy < encounter_radius
					&& (DWORD)dx * dx + (DWORD)dy * dy <
					(DWORD)encounter_radius * encounter_radius
					&& ((COUNT)TFB_Random () % 100) < percent)
			{
				// Ship spawned for encounter.
				hEncounter = AllocEncounter ();
				if (hEncounter)
				{
					LockEncounter (hEncounter, &EncounterPtr);
					memset (EncounterPtr, 0, sizeof (*EncounterPtr));
					EncounterPtr->origin = FleetPtr->loc;
					EncounterPtr->radius = encounter_radius;
					EncounterPtr->flags = encounter_flags;
					EncounterPtr->race_id = Type;
					UnlockEncounter (hEncounter);

					PutEncounter (hEncounter);
				}
			}
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}

	SET_GAME_STATE (USED_BROADCASTER, 0);
}

void
FreeHyperData (void)
{
	if (IS_HD) {
		DestroyDrawable (ReleaseDrawable (hyperholes[1]));
		hyperholes[1] = 0;
		DestroyDrawable (ReleaseDrawable (hyperholes[2]));
		hyperholes[2] = 0;
		// BW: TODO left out for demo
		// DestroyDrawable (ReleaseDrawable (hyperspacesuns));
		// hyperspacesuns = 0;
		DestroyDrawable (ReleaseDrawable (npcbubble));
		npcbubble = 0;
		DestroyDrawable (ReleaseDrawable (quasiportal));
		quasiportal = 0;
		DestroyDrawable (ReleaseDrawable (Falayalaralfali));
		Falayalaralfali = 0;
	}
	
	DestroyDrawable (ReleaseDrawable (hyperstars[0]));
	hyperstars[0] = 0;
	DestroyDrawable (ReleaseDrawable (hyperstars[1]));
	hyperstars[1] = 0;
	DestroyDrawable (ReleaseDrawable (hyperstars[2]));
	hyperstars[2] = 0;
	DestroyDrawable (ReleaseDrawable (hyperstars[3]));
	hyperstars[3] = 0;

	DestroyColorMap (ReleaseColorMap (hypercmaps[0]));
	hypercmaps[0] = 0;
	DestroyColorMap (ReleaseColorMap (hypercmaps[1]));
	hypercmaps[1] = 0;
}

static void
LoadHyperData (void)
{
	if (IS_HD) {
		if (hyperholes[1] == 0) {
			hyperholes[1] = CaptureDrawable (
				LoadGraphic (HYPERHOLES_MASK_PMAP_ANIM));
			hyperholes[2] = CaptureDrawable (
				LoadGraphic (ARIHOLES_MASK_PMAP_ANIM));
			hyperstars[3] = CaptureDrawable (
				LoadGraphic (ARI_AMBIENT_MASK_PMAP_ANIM));
		}
		if (npcbubble == 0)
			npcbubble = CaptureDrawable (LoadGraphic (NPCBUBBLE_MASK_PMAP_ANIM));
		if (quasiportal == 0)
			quasiportal = CaptureDrawable (LoadGraphic (QUASIPORTAL_MASK_PMAP_ANIM));
		if (Falayalaralfali == 0)
			Falayalaralfali = CaptureDrawable (LoadGraphic (FALAYALARALFALI_MASK_PMAP_ANIM));
	}
	
	if (hyperstars[0] == 0) {
		hyperstars[0] = CaptureDrawable (
				LoadGraphic (AMBIENT_MASK_PMAP_ANIM));
		hyperstars[1] = CaptureDrawable (
				LoadGraphic (HYPERSTARS_MASK_PMAP_ANIM));
		hyperstars[2] = CaptureDrawable (
				LoadGraphic (ARISPACE_MASK_PMAP_ANIM));

		hypercmaps[0] = CaptureColorMap (LoadColorMap (HYPER_COLOR_TAB));		
		hypercmaps[1] = CaptureColorMap (LoadColorMap (ARISPACE_COLOR_TAB));
	}
}

BOOLEAN
LoadHyperspace (void)
{
	hyper_dx = 0;
	hyper_dy = 0;
	hyper_extra = 0;
	fuel_ticks = 1;

	GLOBAL (ShipStamp.origin.x) = -MAX_X_UNIVERSE;
	GLOBAL (ShipStamp.origin.y) = -MAX_Y_UNIVERSE;

	LoadHyperData ();
	{
		FRAME F, FQ;
		
		F = hyperstars[0];
		hyperstars[0] = stars_in_space;
		stars_in_space = F;

		if (IS_HD) {
			FQ = hyperstars[3];
			hyperstars[3] = stars_in_quasispace;
			stars_in_quasispace = FQ;
		}
	}

	if (!(LastActivity & CHECK_LOAD))
		RepairSISBorder ();
	else
	{
		if (LOBYTE (LastActivity) == 0)
		{
			DrawSISFrame ();
		}
		else
		{
			ClearSISRect (DRAW_SIS_DISPLAY);
			RepairSISBorder ();
		}
	}
	DrawSISMessage (NULL);

	SetContext (RadarContext);
	SetContextBackGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x0E, 0x00), 0x6C));

	SetContext (SpaceContext);
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{	// Hyperspace Color
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x07, 0x00, 0x00), 0x2F));
		SetColorMap (GetColorMapAddress (hypercmaps[0]));
	}
	else
	{	// Quasispace Color
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x1A, 0x00), 0x2F));
		SetColorMap (GetColorMapAddress (hypercmaps[1]));
		SET_GAME_STATE (USED_BROADCASTER, 0);
		SET_GAME_STATE (BROADCASTER_RESPONSE, 0);
	}
//    ClearDrawable ();

	ClearSISRect (CLEAR_SIS_RADAR);

	return TRUE;
}

BOOLEAN
FreeHyperspace (void)
{
	{
		FRAME F, FQ;
		
		F = hyperstars[0];
		hyperstars[0] = stars_in_space;
		stars_in_space = F;

		if (IS_HD) {
			FQ = hyperstars[3];
			hyperstars[3] = stars_in_quasispace;
			stars_in_quasispace = FQ;
		}
	}
//    FreeHyperData ();

	return TRUE;
}

static void
ElementToUniverse (ELEMENT *ElementPtr, POINT *pPt)
{
	SDWORD log_x, log_y;

	log_x = GLOBAL_SIS (log_x)
			+ (ElementPtr->next.location.x - (LOG_SPACE_WIDTH >> 1));
	log_y = GLOBAL_SIS (log_y)
			+ (ElementPtr->next.location.y - (LOG_SPACE_HEIGHT >> 1));
	pPt->x = LOGX_TO_UNIVERSE (log_x);
	pPt->y = LOGY_TO_UNIVERSE (log_y);
}

static void
cleanup_hyperspace (void)
{
	HENCOUNTER hEncounter, hNextEncounter;

	for (hEncounter = GetHeadEncounter ();
			hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT *ElementPtr;

			LockElement (EncounterPtr->hElement, &ElementPtr);

			if (ElementPtr->hTarget)
			{	// This is the encounter that collided with flagship
				// Move the encounter to the head of the queue so that
				// comm.c:RaceCommunication() gets the right one.
				RemoveEncounter (hEncounter);
				InsertEncounter (hEncounter, GetHeadEncounter ());
			}

			UnlockElement (EncounterPtr->hElement);
		}
		EncounterPtr->hElement = 0;
		UnlockEncounter (hEncounter);
	}
}

typedef enum
{
	RANDOM_ENCOUNTER_TRANSITION,
	INTERPLANETARY_TRANSITION,
	ARILOU_SPACE_TRANSITION
} TRANSITION_TYPE;

static void
InterplanetaryTransition (ELEMENT *ElementPtr)
{
	GLOBAL (ip_planet) = 0;
	GLOBAL (in_orbit) = 0;
	GLOBAL (ShipFacing) = 0; /* Not reentering the system */
	SET_GAME_STATE (USED_BROADCASTER, 0);
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		// Enter a solar system from HyperSpace.
		GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
		SET_GAME_STATE (ESCAPE_COUNTER, 0);
	}
	else
	{
		POINT pt;

		GLOBAL (autopilot.x) = ~0;
		GLOBAL (autopilot.y) = ~0;

		ElementToUniverse (ElementPtr, &pt);
		CurStarDescPtr = FindStar (NULL, &pt, 5, 5);

		// JMS: Debugging helpers
		/*{
			STAR_DESC *SDPtr, *SDPtr2;
			SDPtr = CurStarDescPtr;
			SDPtr2 = FindStar (NULL, &pt, 500, 500);
			log_add(log_Debug, "SDPtr.x %d, SDPtr.y %d SDPtr2.x %d, SDPtr2.y %d, pt.x %d pt.y %d", 
				SDPtr->star_pt.x, SDPtr->star_pt.y, SDPtr2->star_pt.x, SDPtr2->star_pt.y, pt.x, pt.y);
		}*/

		if (CurStarDescPtr->star_pt.x == ARILOU_HOME_X
				&& CurStarDescPtr->star_pt.y == ARILOU_HOME_Y)
		{
			// Meet the Arilou.
			GLOBAL (CurrentActivity) |= START_ENCOUNTER;

			// JMS: The arilou homeworld name can now be shown on QS map.
			SET_GAME_STATE (KNOW_QS_PORTAL_15, 1);
		}
		else
		{
			// Transition from QuasiSpace to HyperSpace through
			// one of the permanent portals.
			COUNT index;
			const POINT portal_pt[] = QUASISPACE_PORTALS_HYPERSPACE_ENDPOINTS;
			RandomContext *portal_rng_x, *portal_rng_y;

			index = CurStarDescPtr - &star_array[NUM_SOLAR_SYSTEMS + 1];

			portal_rng_x = RandomContext_Set(portal_pt[index].x);
			portal_rng_y = RandomContext_Set(portal_pt[index].y);

			if (!PrimeSeed || DIF_HARD) {
				GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(RandomContext_Random(portal_rng_x) % (MAX_X_UNIVERSE + 1));
				GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(RandomContext_Random(portal_rng_y) % (MAX_Y_UNIVERSE + 1));
			} else {
				GLOBAL_SIS(log_x) = UNIVERSE_TO_LOGX(portal_pt[index].x);
				GLOBAL_SIS(log_y) = UNIVERSE_TO_LOGY(portal_pt[index].y);
			}

			// JMS: This QS portal's HS coordinates are revealed on QS map
			// the next time the player visits QS.
			if (PrimeSeed && !DIF_HARD) {
				SET_QS_PORTAL_KNOWN(index);
			}

			SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
		}
	}
}

/* Enter QuasiSpace from HyperSpace by any portal, or HyperSpace from
 * QuasiSpace through the periodically opening portal.
 */
static void
ArilouSpaceTransition (void)
{
	GLOBAL (ShipFacing) = 0; /* Not reentering the system */
	SET_GAME_STATE (USED_BROADCASTER, 0);
	GLOBAL (autopilot.x) = ~0;
	GLOBAL (autopilot.y) = ~0;
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
	{
		// From HyperSpace to QuasiSpace.
		GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (QUASI_SPACE_X);
		GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (QUASI_SPACE_Y);
		if (GET_GAME_STATE (PORTAL_COUNTER) == 0)
		{
			// Periodically appearing portal.
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
		}
		else
		{
			// Player-induced portal.
			SET_GAME_STATE (PORTAL_COUNTER, 0);
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
		}
	}
	else
	{
		// From QuasiSpace to HyperSpace through the periodically appearing
		// portal.
		GLOBAL_SIS (log_x) = UNIVERSE_TO_LOGX (ARILOU_SPACE_X);
		GLOBAL_SIS (log_y) = UNIVERSE_TO_LOGY (ARILOU_SPACE_Y);
		SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
	}
}

static void
unhyper_transition (ELEMENT *ElementPtr)
{
	COUNT frame_index;

	// JMS: If leaving interplanetary on autopilot, always arrive HS with
	// the ship's nose pointed into correct direction.
	if ((GLOBAL (autopilot)).x != ~0 && (GLOBAL (autopilot)).y != ~0) {
		STARSHIP *StarShipPtr;
		POINT universe;
		SIZE facing;
		SDWORD udx = 0, udy = 0;
			
		GetElementStarShip (ElementPtr, &StarShipPtr);
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		udx = (GLOBAL (autopilot)).x - universe.x;
		udy = -((GLOBAL (autopilot)).y - universe.y);
			
		facing = NORMALIZE_FACING (ANGLE_TO_FACING (ARCTAN (udx, udy)));
		StarShipPtr->ShipFacing = facing;
		SetElementStarShip(ElementPtr, StarShipPtr);
	}

	ElementPtr->state_flags |= CHANGING;

	frame_index = GetFrameIndex (ElementPtr->current.image.frame);
	if (frame_index == 0)
		frame_index += ANGLE_TO_FACING (FULL_CIRCLE);
	else if (frame_index < ANGLE_TO_FACING (FULL_CIRCLE))
		frame_index = NORMALIZE_FACING (frame_index + 1);
	else if (++frame_index == GetFrameCount (ElementPtr->current.image.frame))
	{
		cleanup_hyperspace ();

		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
		switch ((TRANSITION_TYPE) ElementPtr->turn_wait)
		{
			case RANDOM_ENCOUNTER_TRANSITION:
				SaveSisHyperState ();
				GLOBAL (CurrentActivity) |= START_ENCOUNTER;
				break;
			case INTERPLANETARY_TRANSITION:
				InterplanetaryTransition (ElementPtr);
				break;
			case ARILOU_SPACE_TRANSITION:
				ArilouSpaceTransition ();
				break;
		}

		ZeroVelocityComponents (&ElementPtr->velocity);
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		return;
	}
	ElementPtr->next.image.frame =
			SetAbsFrameIndex (ElementPtr->current.image.frame, frame_index);
}

static void
init_transition (ELEMENT *ElementPtr0, ELEMENT *ElementPtr1,
		TRANSITION_TYPE which_transition)
{
	SIZE dx, dy;
	SIZE num_turns;
	STARSHIP *StarShipPtr;

	dx = WORLD_TO_VELOCITY (ElementPtr0->next.location.x
			- ElementPtr1->next.location.x);
	dy = WORLD_TO_VELOCITY (ElementPtr0->next.location.y
			- ElementPtr1->next.location.y);

	ElementPtr1->state_flags |= NONSOLID;
	ElementPtr1->preprocess_func = unhyper_transition;
	ElementPtr1->postprocess_func = NULL;
	ElementPtr1->turn_wait = (BYTE) which_transition;

	GetElementStarShip (ElementPtr1, &StarShipPtr);
	num_turns = GetFrameCount (ElementPtr1->next.image.frame)
			- ANGLE_TO_FACING (FULL_CIRCLE)
			+ NORMALIZE_FACING (ANGLE_TO_FACING (FULL_CIRCLE)
			- StarShipPtr->ShipFacing);
	if (num_turns == 0)
		num_turns = 1;

	SetVelocityComponents (&ElementPtr1->velocity,
			dx / num_turns, dy / num_turns);
}

BOOLEAN
hyper_transition (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & APPEARING)
	{
		if (LastActivity & CHECK_LOAD)
		{
			LastActivity &= ~CHECK_LOAD;

			ElementPtr->current = ElementPtr->next;
			SetUpElement (ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;

			return FALSE;
		}
		else
		{
			ElementPtr->preprocess_func =
					(void (*) (struct element *ElementPtr)) hyper_transition;
			ElementPtr->postprocess_func = NULL;
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->next.image.frame =
					SetAbsFrameIndex (ElementPtr->current.image.frame,
					GetFrameCount (ElementPtr->current.image.frame) - 1);
		}
	}
	else
	{
		COUNT frame_index;
		
		// JMS: If leaving interplanetary on autopilot, always arrive HS with
		// the ship's nose pointed into correct direction.
		if ((GLOBAL (autopilot)).x != ~0 && (GLOBAL (autopilot)).y != ~0) {
			STARSHIP *StarShipPtr;
			POINT universe;
			SIZE facing;
			SDWORD udx = 0, udy = 0;
			
			GetElementStarShip (ElementPtr, &StarShipPtr);
			universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
			universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
			udx = (GLOBAL (autopilot)).x - universe.x;
			udy = -((GLOBAL (autopilot)).y - universe.y);
			
			facing = NORMALIZE_FACING (ANGLE_TO_FACING (ARCTAN (udx, udy)));
			StarShipPtr->ShipFacing = facing;
			SetElementStarShip(ElementPtr, StarShipPtr);
		}

		frame_index = GetFrameIndex (ElementPtr->current.image.frame);
		if (frame_index-- <= ANGLE_TO_FACING (FULL_CIRCLE))
		{
			STARSHIP *StarShipPtr;

			if (frame_index == ANGLE_TO_FACING (FULL_CIRCLE) - 1)
				frame_index = 0;
			else
				frame_index = NORMALIZE_FACING (frame_index);

			GetElementStarShip (ElementPtr, &StarShipPtr);
			if (frame_index == StarShipPtr->ShipFacing)
			{
				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}

		ElementPtr->state_flags |= CHANGING;
		ElementPtr->next.image.frame =
				SetAbsFrameIndex (ElementPtr->current.image.frame,
				frame_index);

		if (!(ElementPtr->state_flags & NONSOLID))
		{
			ElementPtr->current = ElementPtr->next;
			SetUpElement (ElementPtr);

			ElementPtr->state_flags |= DEFY_PHYSICS;
		}
	}

	return TRUE;
}

static void
hyper_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	if ((ElementPtr1->state_flags & PLAYER_SHIP)
			&& GET_GAME_STATE (PORTAL_COUNTER) == 0)
	{
		SIZE dx, dy;
		POINT pt;
		STAR_DESC *SDPtr;
		STARSHIP *StarShipPtr;

		ElementToUniverse (ElementPtr0, &pt);

		SDPtr = FindStar (NULL, &pt, 5, 5);

		GetElementStarShip (ElementPtr1, &StarShipPtr);
		GetCurrentVelocityComponents (&ElementPtr1->velocity, &dx, &dy);
		if (SDPtr == CurStarDescPtr
				|| (ElementPtr1->state_flags & APPEARING)
				|| !(dx || dy || (StarShipPtr->cur_status_flags
				& (LEFT | RIGHT | THRUST | WEAPON | SPECIAL))))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
		}
		else if ((GLOBAL (CurrentActivity) & IN_BATTLE)
				&& (GLOBAL (autopilot.x) == ~0
				|| GLOBAL (autopilot.y) == ~0
				|| (GLOBAL (autopilot.x) == SDPtr->star_pt.x
				&& GLOBAL (autopilot.y) == SDPtr->star_pt.y)))
		{
			CurStarDescPtr = SDPtr;
			ElementPtr0->state_flags |= COLLISION;

			init_transition (ElementPtr0, ElementPtr1,
					INTERPLANETARY_TRANSITION);
		}
	}
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static void
hyper_death (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
			&& (GLOBAL (CurrentActivity) & IN_BATTLE))
		CurStarDescPtr = 0;
}

static void
arilou_space_death (ELEMENT *ElementPtr)
{
	if (!(ElementPtr->state_flags & DEFY_PHYSICS)
			|| GET_GAME_STATE (ARILOU_SPACE_COUNTER) == 0)
	{
		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 0);
		}
		else
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, 3);
		}
	}
}

static void
arilou_space_collision (ELEMENT *ElementPtr0,
		POINT *pPt0, ELEMENT *ElementPtr1, POINT *pPt1)
{
	COUNT which_side;

	if (!(ElementPtr1->state_flags & PLAYER_SHIP))
		return;

	which_side = GET_GAME_STATE (ARILOU_SPACE_SIDE);
	if (which_side == 0 || which_side == 3)
	{
		if (ElementPtr1->state_flags & DEFY_PHYSICS)
		{
			SET_GAME_STATE (ARILOU_SPACE_SIDE, which_side ^ 1);
		}
		else
		{
			init_transition (ElementPtr0, ElementPtr1,
					ARILOU_SPACE_TRANSITION);
		}
	}

	ElementPtr0->state_flags |= DEFY_PHYSICS | COLLISION;
	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AllocHyperElement (const POINT *elem_pt)
{
	HELEMENT hHyperSpaceElement;

	hHyperSpaceElement = AllocElement ();
	if (hHyperSpaceElement)
	{
		ELEMENT *HyperSpaceElementPtr;

		LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		HyperSpaceElementPtr->state_flags = CHANGING | FINITE_LIFE;
		HyperSpaceElementPtr->life_span = 1;
		HyperSpaceElementPtr->mass_points = 1;

		{
			long lx, ly;

			lx = UNIVERSE_TO_LOGX (elem_pt->x)
					+ (LOG_SPACE_WIDTH >> 1) - GLOBAL_SIS (log_x);
			HyperSpaceElementPtr->current.location.x = WRAP_X (lx);

			ly = UNIVERSE_TO_LOGY (elem_pt->y)
					+ (LOG_SPACE_HEIGHT >> 1) - GLOBAL_SIS (log_y);
			HyperSpaceElementPtr->current.location.y = WRAP_Y (ly);
		}

		SetPrimType (&DisplayArray[HyperSpaceElementPtr->PrimIndex],
				STAMP_PRIM);
		HyperSpaceElementPtr->current.image.farray =
				&hyperstars[1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1)];

		UnlockElement (hHyperSpaceElement);
	}

	return hHyperSpaceElement;
}

static void
AddAmbientElement (void)
{
	HELEMENT hHyperSpaceElement;

	hHyperSpaceElement = AllocElement ();
	if (hHyperSpaceElement)
	{
		SIZE dx, dy;
		DWORD rand_val;
		ELEMENT *HyperSpaceElementPtr;

		LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
		HyperSpaceElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		HyperSpaceElementPtr->state_flags =
				APPEARING | FINITE_LIFE | NONSOLID;
		SetPrimType (&DisplayArray[HyperSpaceElementPtr->PrimIndex],
				STAMP_PRIM);
		HyperSpaceElementPtr->preprocess_func = animation_preprocess;

		rand_val = TFB_Random ();
		dy = LOWORD (rand_val);
		
		// JMS_GFX
		if (!IS_HD) {
			dx = (SIZE)(LOBYTE (dy) % SPACE_WIDTH) - (SPACE_WIDTH >> 1);
			dy = (SIZE)(HIBYTE (dy) % SPACE_HEIGHT) - (SPACE_HEIGHT >> 1);
			HyperSpaceElementPtr->current.image.farray = &stars_in_space;
		} else {
			dx = (SIZE)((HIWORD (rand_val)) % SPACE_WIDTH) - (SPACE_WIDTH >> 1);
			dy = (SIZE)(dy % SPACE_HEIGHT) - (SPACE_HEIGHT >> 1);
			
			if ((GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1))
				HyperSpaceElementPtr->current.image.farray = &stars_in_space;
			else
				HyperSpaceElementPtr->current.image.farray = &stars_in_quasispace;
		}
		
		HyperSpaceElementPtr->current.location.x = (LOG_SPACE_WIDTH >> 1) + DISPLAY_TO_WORLD (dx);
		HyperSpaceElementPtr->current.location.y = (LOG_SPACE_HEIGHT >> 1) + DISPLAY_TO_WORLD (dy);

		if (HIWORD (rand_val) & 7)
		{
			HyperSpaceElementPtr->life_span = 14;
			if (!IS_HD || (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1))
				HyperSpaceElementPtr->current.image.frame = stars_in_space;
			else
				HyperSpaceElementPtr->current.image.frame = stars_in_quasispace;
		}
		else
		{
			HyperSpaceElementPtr->life_span = 12;
			if (!IS_HD || (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1))
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (stars_in_space, 14);
			else
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (stars_in_quasispace, 14);
		}

		UnlockElement (hHyperSpaceElement);

		InsertElement (hHyperSpaceElement, GetHeadElement ());
	}
}

#define NUM_VORTEX_TRANSITIONS 9
#define VORTEX_WAIT 1

static void
encounter_transition (ELEMENT *ElementPtr)
{
	ElementPtr->state_flags &= ~DISAPPEARING;
	ElementPtr->life_span = 1;
	if (ElementPtr->turn_wait)
	{
		--ElementPtr->turn_wait;
	}
	else
	{
		FRAME f;

		if (ElementPtr->hit_points)
		{
			f = DecFrameIndex (ElementPtr->current.image.frame);
			ElementPtr->next.image.frame = f;
		}
		else
		{
			f = IncFrameIndex (ElementPtr->current.image.frame);
			if (f != ElementPtr->current.image.farray[0])
				ElementPtr->next.image.frame = f;
			else {
 				ElementPtr->death_func = NULL;
				// BW: the bubble has reached full size so we start animation
				if (IS_HD) {
					ElementPtr->current.image.farray = &npcbubble;
					ElementPtr->next.image.farray = &npcbubble;
					ElementPtr->current.image.frame = SetAbsFrameIndex(npcbubble, 0);
					ElementPtr->next.image.frame = SetAbsFrameIndex(npcbubble, 0);
				}
			}
		}

		ElementPtr->turn_wait = VORTEX_WAIT;
	}
}

static HELEMENT
getSisElement (void)
{
	HSTARSHIP hSis;
	HELEMENT hShip;
	STARSHIP *StarShipPtr;

	hSis = GetHeadLink (&race_q[RPG_PLAYER_NUM]);
	if (!hSis)
		return NULL;

	StarShipPtr = LockStarShip (&race_q[RPG_PLAYER_NUM], hSis);
	hShip = StarShipPtr->hShip;
	UnlockStarShip (&race_q[RPG_PLAYER_NUM], hSis);

#ifdef DEBUG
	{
		ELEMENT *ElementPtr;
		LockElement (hShip, &ElementPtr);
		assert (ElementPtr->state_flags & PLAYER_SHIP);
		UnlockElement (hShip);
	}
#endif

	return hShip;
}

static void
encounter_collision (ELEMENT *ElementPtr0, POINT *pPt0,
		ELEMENT *ElementPtr1, POINT *pPt1)
{
	HENCOUNTER hEncounter;
	HENCOUNTER hNextEncounter;

	if (!(ElementPtr1->state_flags & PLAYER_SHIP)
			|| !(GLOBAL (CurrentActivity) & IN_BATTLE))
		return;

	init_transition (ElementPtr0, ElementPtr1, RANDOM_ENCOUNTER_TRANSITION);

	for (hEncounter = GetHeadEncounter ();
			hEncounter != 0; hEncounter = hNextEncounter)
	{
		ENCOUNTER *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);
		if (EncounterPtr->hElement)
		{
			ELEMENT *ElementPtr;

			LockElement (EncounterPtr->hElement, &ElementPtr);
			ElementPtr->state_flags |= NONSOLID | IGNORE_SIMILAR;
			UnlockElement (EncounterPtr->hElement);
		}
		UnlockEncounter (hEncounter);
	}

	// Mark this element as collided with flagship
	// XXX: We could simply set hTarget to 1 or to ElementPtr1,
	//   but that would be too hacky ;)
	ElementPtr0->hTarget = getSisElement ();
	ZeroVelocityComponents (&ElementPtr0->velocity);

	(void) pPt0;  /* Satisfying compiler (unused parameter) */
	(void) pPt1;  /* Satisfying compiler (unused parameter) */
}

static HELEMENT
AddEncounterElement (ENCOUNTER *EncounterPtr, POINT *puniverse)
{
	BOOLEAN NewEncounter;
	HELEMENT hElement;
	POINT enc_pt;
	
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) >= 2)
		return 0;

	if (EncounterPtr->flags & ENCOUNTER_REFORMING)
	{
		EncounterPtr->flags &= ~ENCOUNTER_REFORMING;

		EncounterPtr->transition_state = 100;
		if ((EncounterPtr->flags & ONE_SHOT_ENCOUNTER)
				|| EncounterPtr->num_ships == 0)
			return 0;
	}

	if (EncounterPtr->num_ships)
	{
		NewEncounter = FALSE;
		enc_pt = EncounterPtr->loc_pt;
	}
	else
	{
		BYTE Type;
		SIZE dx, dy;
		COUNT i;
		COUNT NumShips;
		DWORD radius_squared;
		BYTE EncounterMakeup[] =
		{
			RACE_ENCOUNTER_MAKEUP
		};

		NewEncounter = TRUE;

		radius_squared = (DWORD)EncounterPtr->radius * EncounterPtr->radius;

		Type = EncounterPtr->race_id;
		NumShips = LONIBBLE (EncounterMakeup[Type]);
		for (i = HINIBBLE (EncounterMakeup[Type]) - NumShips; i; --i)
		{
			if ((COUNT)TFB_Random () % 100 < 50)
				++NumShips;
		}

		if (NumShips > MAX_HYPER_SHIPS)
			NumShips = MAX_HYPER_SHIPS;

		EncounterPtr->num_ships = NumShips;
		for (i = 0; i < NumShips; ++i)
		{
			BRIEF_SHIP_INFO *BSIPtr = &EncounterPtr->ShipList[i];
			HFLEETINFO hStarShip =
					GetStarShipFromIndex (&GLOBAL (avail_race_q), Type);
			FLEET_INFO *FleetPtr =
					LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
			BSIPtr->race_id = Type;
			BSIPtr->crew_level = FleetPtr->crew_level;
			BSIPtr->max_crew = FleetPtr->max_crew;
			BSIPtr->max_energy = FleetPtr->max_energy;
			UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		}

		do
		{
			DWORD rand_val;

			rand_val = TFB_Random ();

			enc_pt.x = puniverse->x
					+ (LOWORD (rand_val) % (XOFFS << 1)) - XOFFS;
			if (enc_pt.x < 0)
				enc_pt.x = 0;
			else if (enc_pt.x > MAX_X_UNIVERSE)
				enc_pt.x = MAX_X_UNIVERSE;
			enc_pt.y = puniverse->y
					+ (HIWORD (rand_val) % (YOFFS << 1)) - YOFFS;
			if (enc_pt.y < 0)
				enc_pt.y = 0;
			else if (enc_pt.y > MAX_Y_UNIVERSE)
				enc_pt.y = MAX_Y_UNIVERSE;

			dx = enc_pt.x - EncounterPtr->origin.x;
			dy = enc_pt.y - EncounterPtr->origin.y;
		} while ((DWORD)((long)dx * dx + (long)dy * dy) > radius_squared);

		EncounterPtr->loc_pt = enc_pt;
		EncounterPtr->log_x = UNIVERSE_TO_LOGX (enc_pt.x);
		EncounterPtr->log_y = UNIVERSE_TO_LOGY (enc_pt.y);
	}

	hElement = AllocHyperElement (&enc_pt);
	if (hElement)
	{
		SIZE i;
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		
		i = EncounterPtr->transition_state;
		if (i || NewEncounter)
		{
			if (i < 0)
			{
				i = -i;
				ElementPtr->hit_points = 1;
			}
			if (i == 0 || i > NUM_VORTEX_TRANSITIONS)
				i = NUM_VORTEX_TRANSITIONS;

			ElementPtr->current.image.frame = SetRelFrameIndex (
					ElementPtr->current.image.farray[0], -i);
			ElementPtr->death_func = encounter_transition;
		}
		else
		{
			if (IS_HD) {
				ElementPtr->current.image.farray = &npcbubble;
				ElementPtr->next.image.farray = &npcbubble;
				ElementPtr->current.image.frame = SetAbsFrameIndex(npcbubble, 0);
				ElementPtr->next.image.frame = SetAbsFrameIndex(npcbubble, 0);
			} else {
				ElementPtr->current.image.frame = DecFrameIndex (ElementPtr->current.image.farray[0]);
			}
		}

		ElementPtr->turn_wait = VORTEX_WAIT;
		ElementPtr->preprocess_func = NULL;
		ElementPtr->postprocess_func = NULL; // decorate_vortex;
		ElementPtr->collision_func = encounter_collision;

		SetUpElement (ElementPtr);

		ElementPtr->IntersectControl.IntersectStamp.frame =
				DecFrameIndex (stars_in_space);
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
		ElementPtr->state_flags |= NONSOLID | IGNORE_VELOCITY;

		UnlockElement (hElement);

		InsertElement (hElement, GetTailElement ());
	}
	
	EncounterPtr->hElement = hElement;
	return hElement;
}

#define GRID_OFFSET 200

static void
DrawHyperGrid (COORD ux, COORD uy, COORD ox, COORD oy)
{
	COORD sx, sy, ex, ey;
	RECT r;

	ClearDrawable ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x10, 0x00), 0x6B));

	sx = ux - (RADAR_SCAN_WIDTH >> 1);
	if (sx  < 0)
		sx = 0;
	else
		sx -= sx % GRID_OFFSET;
	ex = ux + (RADAR_SCAN_WIDTH >> 1);
	if (ex > MAX_X_UNIVERSE + 1)
		ex = MAX_X_UNIVERSE + 1;

	sy = uy - (RADAR_SCAN_HEIGHT >> 1);
	if (sy < 0)
		sy = 0;
	else
		sy -= sy % GRID_OFFSET;
	ey = uy + (RADAR_SCAN_HEIGHT >> 1);
	if (ey > MAX_Y_UNIVERSE + 1)
		ey = MAX_Y_UNIVERSE + 1;

	r.corner.y = (COORD) ((long)(MAX_Y_UNIVERSE - ey)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
	r.extent.width = 1;
	r.extent.height = ((COORD) ((long)(MAX_Y_UNIVERSE - sy)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy) - r.corner.y + 1;
	for (ux = sx; ux <= ex; ux += GRID_OFFSET)
	{
		r.corner.x = (COORD) ((long)ux * RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
		DrawFilledRectangle (&r);
	}

	r.corner.x = (COORD) ((long)sx * RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
	r.extent.width = ((COORD) ((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH)
			- ox) - r.corner.x + 1;
	r.extent.height = 1;
	for (uy = sy; uy <= ey; uy += GRID_OFFSET)
	{
		r.corner.y = (COORD)((long)(MAX_Y_UNIVERSE - uy)
				* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
		DrawFilledRectangle (&r);
	}
}

// Returns false iff the encounter is to be removed.
static bool
ProcessEncounter (ENCOUNTER *EncounterPtr, POINT *puniverse,
		COORD ox, COORD oy, STAMP *stamp)
{
	ELEMENT *ElementPtr;
	COORD ex, ey;

	if (EncounterPtr->hElement == 0
			&& AddEncounterElement (EncounterPtr, puniverse) == 0)
		return false;

	LockElement (EncounterPtr->hElement, &ElementPtr);

	if (ElementPtr->death_func)
	{
		if (EncounterPtr->transition_state && ElementPtr->turn_wait == 0)
		{
			--EncounterPtr->transition_state;
			if (EncounterPtr->transition_state >= NUM_VORTEX_TRANSITIONS)
				++ElementPtr->turn_wait;
			else if (EncounterPtr->transition_state ==
					-NUM_VORTEX_TRANSITIONS)
			{
				ElementPtr->death_func = NULL;
				UnlockElement (EncounterPtr->hElement);
				return false;
			}
			else
				SetPrimType (&DisplayArray[ElementPtr->PrimIndex],
						STAMP_PRIM);
		}
	}
	else
	{
		SIZE delta_x, delta_y;
		COUNT encounter_radius;

		ElementPtr->life_span = 1;
		GetNextVelocityComponents (&ElementPtr->velocity,
				&delta_x, &delta_y, 1);
		if (ElementPtr->thrust_wait)
			--ElementPtr->thrust_wait;
		else if (!ElementPtr->hTarget)
		{	// This is an encounter that did not collide with flagship
			// The colliding encounter does not move
			COUNT cur_facing, delta_facing;

			cur_facing = ANGLE_TO_FACING (
					GetVelocityTravelAngle (&ElementPtr->velocity));
			delta_facing = NORMALIZE_FACING (cur_facing - ANGLE_TO_FACING (
					ARCTAN (puniverse->x - EncounterPtr->loc_pt.x,
					puniverse->y - EncounterPtr->loc_pt.y)));
			if (delta_facing || (delta_x == 0 && delta_y == 0))
			{
				SIZE speed;
				const SIZE RaceHyperSpeed[] =
				{
					RACE_HYPER_SPEED
				};

#define ENCOUNTER_TRACK_WAIT 3
				speed = RaceHyperSpeed[EncounterPtr->race_id];
				if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
					--cur_facing;
				else
					++cur_facing;
				if (NORMALIZE_FACING (delta_facing + ANGLE_TO_FACING (OCTANT))
						> ANGLE_TO_FACING (QUADRANT))
				{
					if (delta_facing < ANGLE_TO_FACING (HALF_CIRCLE))
						--cur_facing;
					else
						++cur_facing;
					speed >>= 1;
				}
				cur_facing = FACING_TO_ANGLE (cur_facing);
				SetVelocityComponents (&ElementPtr->velocity,
						COSINE (cur_facing, speed), SINE (cur_facing, speed));
				GetNextVelocityComponents (&ElementPtr->velocity,
						&delta_x, &delta_y, 1);

				ElementPtr->thrust_wait = ENCOUNTER_TRACK_WAIT;
			}
		}
		EncounterPtr->log_x += delta_x;
		EncounterPtr->log_y -= delta_y;
		EncounterPtr->loc_pt.x = LOGX_TO_UNIVERSE (EncounterPtr->log_x);
		EncounterPtr->loc_pt.y = LOGY_TO_UNIVERSE (EncounterPtr->log_y);

		// BW: Animate the NPC bubble in hi-res modes.
		if (IS_HD)
			ElementPtr->next.image.frame = IncFrameIndex (ElementPtr->current.image.frame);

		encounter_radius = EncounterPtr->radius + (GRID_OFFSET >> 1);
		delta_x = EncounterPtr->loc_pt.x - EncounterPtr->origin.x;
		if (delta_x < 0)
			delta_x = -delta_x;
		delta_y = EncounterPtr->loc_pt.y - EncounterPtr->origin.y;
		if (delta_y < 0)
			delta_y = -delta_y;
		if ((COUNT)delta_x >= encounter_radius
				|| (COUNT)delta_y >= encounter_radius
				|| (DWORD)delta_x * delta_x + (DWORD)delta_y * delta_y >=
				(DWORD)encounter_radius * encounter_radius)
		{
			// Encounter globe traveled outside the SoI and now disappears
			ElementPtr->state_flags |= NONSOLID;
			ElementPtr->life_span = 0;

			if (EncounterPtr->transition_state == 0)
			{
				ElementPtr->death_func = encounter_transition;
				EncounterPtr->transition_state = -1;
				ElementPtr->hit_points = 1;
			}
			else
			{
				ElementPtr->death_func = NULL;
				UnlockElement (EncounterPtr->hElement);
				return false;
			}
		}
	}

	ex = EncounterPtr->loc_pt.x;
	ey = EncounterPtr->loc_pt.y;
	if (ex - puniverse->x >= -UNIT_SCREEN_WIDTH
			&& ex - puniverse->x <= UNIT_SCREEN_WIDTH
			&& ey - puniverse->y >= -UNIT_SCREEN_HEIGHT
			&& ey - puniverse->y <= UNIT_SCREEN_HEIGHT)
	{
		ElementPtr->next.location.x =
				(SIZE)(EncounterPtr->log_x - GLOBAL_SIS (log_x))
				+ (LOG_SPACE_WIDTH >> 1);
		ElementPtr->next.location.y =
				(SIZE)(EncounterPtr->log_y - GLOBAL_SIS (log_y))
				+ (LOG_SPACE_HEIGHT >> 1);
		if ((ElementPtr->state_flags & NONSOLID)
				&& EncounterPtr->transition_state == 0)
		{
			ElementPtr->current.location = ElementPtr->next.location;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex],
					STAMP_PRIM);
			if (ElementPtr->death_func == 0)
			{
				InitIntersectStartPoint (ElementPtr);
				ElementPtr->state_flags &= ~NONSOLID;
			}
		}
	}
	else
	{
		ElementPtr->state_flags |= NONSOLID;
		if (ex - puniverse->x < -XOFFS || ex - puniverse->x > XOFFS
				|| ey - puniverse->y < -YOFFS || ey - puniverse->y > YOFFS)
		{
			ElementPtr->life_span = 0;
			ElementPtr->death_func = NULL;
			UnlockElement (EncounterPtr->hElement);
			return false;
		}

		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
	}

	UnlockElement (EncounterPtr->hElement);
		
	stamp->origin.x = (COORD)((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH) - ox;
	stamp->origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey) * RADAR_HEIGHT
			/ RADAR_SCAN_HEIGHT) - oy;
	DrawStamp (stamp);

	return true;
}

static void
ProcessEncounters (POINT *puniverse, COORD ox, COORD oy)
{
	STAMP stamp;
	HENCOUNTER hEncounter, hNextEncounter;

	stamp.frame = SetAbsFrameIndex (stars_in_space, 91);
	for (hEncounter = GetHeadEncounter ();
			hEncounter; hEncounter = hNextEncounter)
	{
		ENCOUNTER *EncounterPtr;

		LockEncounter (hEncounter, &EncounterPtr);
		hNextEncounter = GetSuccEncounter (EncounterPtr);

		if (!ProcessEncounter (EncounterPtr, puniverse, ox, oy, &stamp))
		{
			UnlockEncounter (hEncounter);
			RemoveEncounter (hEncounter);
			FreeEncounter (hEncounter);
			continue;
		}

		UnlockEncounter (hEncounter);
	}
}

#define NUM_HOLES_FRAMES 32 // BW
#define NUM_SUNS_FRAMES 32 // BW
#define NUM_QUASIPORTAL_IN_HS_FRAMES 30 // JMS

void
SeedUniverse (void)
{
	COORD ox, oy;
	COORD sx, sy, ex, ey;
	SWORD portalCounter, arilouSpaceCounter, arilouSpaceSide;
	POINT universe;
	FRAME blip_frame;
	STAMP s;
	STAR_DESC *SDPtr;
	HELEMENT hHyperSpaceElement;
	ELEMENT *HyperSpaceElementPtr;

	static COUNT frameCounter; // BW
	
	frameCounter++; // BW

	universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));

	blip_frame = SetAbsFrameIndex (stars_in_space, 90);

	SetContext (RadarContext);
	BatchGraphics ();
	
	ox = (COORD)((long)universe.x * RADAR_WIDTH / RADAR_SCAN_WIDTH)
			- (RADAR_WIDTH >> 1);
	oy = (COORD)((long)(MAX_Y_UNIVERSE - universe.y)
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - (RADAR_HEIGHT >> 1);

	ex = (COORD)((long)GLOBAL (ShipStamp.origin.x)
			* RADAR_WIDTH / RADAR_SCAN_WIDTH) - (RADAR_WIDTH >> 1);
	ey = (COORD)((long)(MAX_Y_UNIVERSE - GLOBAL (ShipStamp.origin.y))
			* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - (RADAR_HEIGHT >> 1);

	arilouSpaceCounter = GET_GAME_STATE (ARILOU_SPACE_COUNTER);
	arilouSpaceSide = GET_GAME_STATE (ARILOU_SPACE_SIDE);

//    if (ox != ex || oy != ey)
	{
		DrawHyperGrid (universe.x, universe.y, ox, oy);

		{
			SDPtr = 0;
			while ((SDPtr = FindStar (SDPtr, &universe, XOFFS, YOFFS)))
			{
				BYTE star_type;

				ex = SDPtr->star_pt.x;
				ey = SDPtr->star_pt.y;
				star_type = STAR_TYPE (SDPtr->Type);
				if (arilouSpaceSide >= 2 &&
						ex == ARILOU_HOME_X && ey == ARILOU_HOME_Y)
					star_type = SUPER_GIANT_STAR;

				s.origin.x = (COORD)((long)ex * RADAR_WIDTH
						/ RADAR_SCAN_WIDTH) - ox;
				s.origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey)
						* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
				s.frame = SetRelFrameIndex (blip_frame,
						star_type + 2);
				DrawStamp (&s);
			}
		}
	}

	portalCounter = GET_GAME_STATE (PORTAL_COUNTER);
	if (portalCounter || arilouSpaceCounter)
	{
		COUNT i;
		STAR_DESC SD[2];
				// This array is filled with the STAR_DESC's of
				// QuasiSpace portals that need to be taken into account.
				// i is set to the number of active portals (max 2).

		i = 0;
		if (portalCounter)
		{
			// A player-created QuasiSpace portal is opening.
			static POINT portal_pt;

			SD[i].Index = ((portalCounter - 1) >> 1) + 18;
			if (portalCounter == 1)
				portal_pt = universe;
			SD[i].star_pt = portal_pt;
			++i;

			if (++portalCounter == (10 + 1))
				portalCounter = (9 + 1);

			SET_GAME_STATE (PORTAL_COUNTER, portalCounter);
		}

		if (arilouSpaceCounter)
		{
			// The periodically appearing QuasiSpace portal is open.
			SD[i].Index = arilouSpaceCounter >> 1;
			if (arilouSpaceSide <= 1)
			{
				// The player is in HyperSpace
				SD[i].Index += 18;
				SD[i].star_pt.x = ARILOU_SPACE_X;
				SD[i].star_pt.y = ARILOU_SPACE_Y;
			}
			else
			{
				// The player is in QuasiSpace
				SD[i].star_pt.x = QUASI_SPACE_X;
				SD[i].star_pt.y = QUASI_SPACE_Y;
			}
			++i;
		}

		// Process the i portals from SD.
		do
		{
			--i;
			sx = SD[i].star_pt.x - universe.x + XOFFS;
			sy = SD[i].star_pt.y - universe.y + YOFFS;
			if (sx < 0 || sy < 0 || sx >= (XOFFS << 1) || sy >= (YOFFS << 1))
				continue;

			ex = SD[i].star_pt.x;
			ey = SD[i].star_pt.y;
			s.origin.x = (COORD)((long)ex * RADAR_WIDTH / RADAR_SCAN_WIDTH)
					- ox;
			s.origin.y = (COORD)((long)(MAX_Y_UNIVERSE - ey)
					* RADAR_HEIGHT / RADAR_SCAN_HEIGHT) - oy;
			s.frame = SetAbsFrameIndex (stars_in_space, 95);
			DrawStamp (&s);

			ex -= universe.x;
			if (ex < 0)
				ex = -ex;
			ey -= universe.y;
			if (ey < 0)
				ey = -ey;

			if (ex > (XOFFS / NUM_RADAR_SCREENS)
				|| ey > (YOFFS / NUM_RADAR_SCREENS))
			continue;

			hHyperSpaceElement = AllocHyperElement (&SD[i].star_pt);
			if (hHyperSpaceElement == 0)
				continue;

			LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
			if (!IS_HD
				|| (SD[i].Index < 22 && arilouSpaceSide <= 1)
				|| (SD[i].Index < 4 && arilouSpaceSide > 1))
			{
				// The QS portal is still growing (Or when playing in 1x resolution).
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
					hyperstars[1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1)],
					SD[i].Index);
			} else if (arilouSpaceSide > 1) {
				// QS. The QS portal has done its growing animation: in HD switch to the full-size anim.
				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex (quasiportal, frameCounter % NUM_HOLES_FRAMES);
				HyperSpaceElementPtr->current.image.farray = &hyperholes[2];
			} else {
				// HS. The QS portal has done its growing animation: in HD switch to the full-size anim.
				HyperSpaceElementPtr->current.image.frame =
					SetAbsFrameIndex (quasiportal, frameCounter % NUM_QUASIPORTAL_IN_HS_FRAMES);
				HyperSpaceElementPtr->current.image.farray = &quasiportal;
			}

			HyperSpaceElementPtr->preprocess_func = NULL;
			HyperSpaceElementPtr->postprocess_func = NULL;
			HyperSpaceElementPtr->collision_func = arilou_space_collision;

			SetUpElement (HyperSpaceElementPtr);

			if (arilouSpaceSide == 1 || arilouSpaceSide == 2)
				HyperSpaceElementPtr->death_func = arilou_space_death;
			else
			{
				HyperSpaceElementPtr->death_func = NULL;
				HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
						DecFrameIndex (stars_in_space);
			}

			UnlockElement (hHyperSpaceElement);

			InsertElement (hHyperSpaceElement, GetHeadElement ());
		} while (i);
	}

	{
		SDPtr = 0;
		while ((SDPtr = FindStar (SDPtr, &universe, XOFFS, YOFFS)))
		{
			BYTE star_type;
			int which_spaces_star_gfx;

			ex = SDPtr->star_pt.x - universe.x;
			if (ex < 0)
				ex = -ex;
			ey = SDPtr->star_pt.y - universe.y;
			if (ey < 0)
				ey = -ey;
			if (ex > (XOFFS / NUM_RADAR_SCREENS)
					|| ey > (YOFFS / NUM_RADAR_SCREENS))
				continue;

			star_type = SDPtr->Type;

			if (!IS_HD) {
				hHyperSpaceElement = AllocHyperElement (&SDPtr->star_pt);
				if (hHyperSpaceElement == 0)
					continue;
				
				LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
				which_spaces_star_gfx = 1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1);
				
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
					hyperstars[which_spaces_star_gfx],
					STAR_TYPE (star_type) * NUM_STAR_COLORS
					+ STAR_COLOR (star_type));
				
				HyperSpaceElementPtr->preprocess_func = NULL;
				HyperSpaceElementPtr->postprocess_func = NULL;
				HyperSpaceElementPtr->collision_func = hyper_collision;
				
				SetUpElement (HyperSpaceElementPtr);
				
				if (SDPtr == CurStarDescPtr
				    && GET_GAME_STATE (PORTAL_COUNTER) == 0)
					HyperSpaceElementPtr->death_func = hyper_death;
				else
				{
					HyperSpaceElementPtr->death_func = NULL;
					HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
					DecFrameIndex (stars_in_space);
				}
				UnlockElement (hHyperSpaceElement);
				
				InsertElement (hHyperSpaceElement, GetHeadElement ());
			} else {
				// BW: first the actual star
				if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1 
					|| ((GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1) && STAR_COLOR (star_type) == YELLOW_BODY))
				{
					hHyperSpaceElement = AllocHyperElement (&SDPtr->star_pt);
					if (hHyperSpaceElement == 0)
						continue;
				
					LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
				
					// JMS_GFX: Draw stars in hyperspace.
					if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
					{
						// The color, then the size and finally
						// the frame offset for the actual animation
						HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
						hyperspacesuns, STAR_COLOR (star_type) * NUM_STAR_TYPES * NUM_SUNS_FRAMES
						+ STAR_TYPE (star_type) * NUM_SUNS_FRAMES
						+ frameCounter % NUM_SUNS_FRAMES);
					
						HyperSpaceElementPtr->current.image.farray = &hyperspacesuns;
						HyperSpaceElementPtr->death_func = NULL;
					}
					// JMS_GFX: Draw Arilou homeworld in quasispace.
					else if ((GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1) && STAR_COLOR (star_type) == YELLOW_BODY)
					{
						// JMS_GFX: Draw Arilou homeworld in quasispace | Serosis: Draw *animated* Arilou homeworld
						HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (Falayalaralfali, frameCounter % NUM_HOLES_FRAMES);
						HyperSpaceElementPtr->current.image.farray = &Falayalaralfali;
					}
					HyperSpaceElementPtr->death_func = NULL;
					HyperSpaceElementPtr->preprocess_func = NULL;
					HyperSpaceElementPtr->postprocess_func = NULL;
					HyperSpaceElementPtr->collision_func = hyper_collision;
				
					SetUpElement (HyperSpaceElementPtr);
				
					HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame = DecFrameIndex (stars_in_space);
				
					UnlockElement (hHyperSpaceElement);
				
					InsertElement (hHyperSpaceElement, GetHeadElement ());
				
					// JMS_GFX: Don't draw hole for arilou homeworld - it already has a nice planet gfx.
					if ((GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1) && STAR_COLOR (star_type) == YELLOW_BODY && !IS_HD)
						continue;
				
				}
				
				// BW: and then the animated hyperspace portal
				hHyperSpaceElement = AllocHyperElement (&SDPtr->star_pt);
				if (hHyperSpaceElement == 0)
					continue;
				
				LockElement (hHyperSpaceElement, &HyperSpaceElementPtr);
				which_spaces_star_gfx = 1 + (GET_GAME_STATE (ARILOU_SPACE_SIDE) >> 1);
				
				// Most holes go 100, 150, 200 or 150, 200, 250
				HyperSpaceElementPtr->current.image.frame = SetAbsFrameIndex (
					hyperholes[which_spaces_star_gfx],
					STAR_TYPE (star_type) * NUM_HOLES_FRAMES);
				
				// Green, orange and yellow need bigger holes
				if (STAR_COLOR (star_type) == GREEN_BODY 
					|| STAR_COLOR (star_type) == ORANGE_BODY 
					|| STAR_COLOR (star_type) == YELLOW_BODY)
					HyperSpaceElementPtr->current.image.frame = SetRelFrameIndex (
						HyperSpaceElementPtr->current.image.frame,
						NUM_HOLES_FRAMES);
				
				// Super giant blue needs a bigger hole
				if (STAR_COLOR (star_type) == BLUE_BODY 
					&& STAR_TYPE (star_type) == SUPER_GIANT_STAR)
					HyperSpaceElementPtr->current.image.frame = SetRelFrameIndex (
						HyperSpaceElementPtr->current.image.frame,
						NUM_HOLES_FRAMES);
				
				// The actual animation
				HyperSpaceElementPtr->current.image.frame = SetRelFrameIndex (
					HyperSpaceElementPtr->current.image.frame,
					frameCounter % NUM_HOLES_FRAMES);

				HyperSpaceElementPtr->current.image.farray = &hyperholes[which_spaces_star_gfx];
				HyperSpaceElementPtr->preprocess_func = NULL;
				HyperSpaceElementPtr->postprocess_func = NULL;
				HyperSpaceElementPtr->collision_func = hyper_collision;
				
				SetUpElement (HyperSpaceElementPtr);
				
				if ((SDPtr == CurStarDescPtr && GET_GAME_STATE (PORTAL_COUNTER) == 0)) {
					HyperSpaceElementPtr->death_func = hyper_death;
				} else {
					HyperSpaceElementPtr->death_func = NULL;
					HyperSpaceElementPtr->IntersectControl.IntersectStamp.frame =
					DecFrameIndex (stars_in_space);
				}
				UnlockElement (hHyperSpaceElement);
				
				InsertElement (hHyperSpaceElement, GetHeadElement ());	
			}
		}
		ProcessEncounters (&universe, ox, oy);
	}

	s.origin.x = RADAR_WIDTH >> 1;
	s.origin.y = RADAR_HEIGHT >> 1;
	s.frame = blip_frame;
	DrawStamp (&s);

	{
		// draws borders to mini-map
		
		RECT r;
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0E, 0x0E, 0x0E), 0x00));
		r.corner.x = 0;
		r.corner.y = 0;
		r.extent.width = RADAR_WIDTH - 1;
		r.extent.height = 1;
		DrawFilledRectangle (&r);
		r.extent.width = 1;
		r.extent.height = RADAR_HEIGHT - 1;
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x06, 0x06, 0x06), 0x00));
		r.corner.x = RADAR_WIDTH - 1;
		r.corner.y = 1;
		r.extent.height = RADAR_HEIGHT - 1;
		DrawFilledRectangle (&r);
		r.corner.x = 1;
		r.corner.y = RADAR_HEIGHT - 1;
		r.extent.width = RADAR_WIDTH - 2;
		r.extent.height = 1;
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x08, 0x08, 0x08), 0x00));
		r.corner.x = 0;
		DrawPoint (&r.corner);
		r.corner.x = RADAR_WIDTH - 1;
		r.corner.y = 0;
		DrawPoint (&r.corner);
	}

	UnbatchGraphics ();

	SetContext (StatusContext);

	if (!(LOWORD (TFB_Random ()) & 7))
		AddAmbientElement ();

	if (universe.x != GLOBAL (ShipStamp.origin.x)
			|| universe.y != GLOBAL (ShipStamp.origin.y))
	{
		GLOBAL (ShipStamp.origin) = universe;
		DrawHyperCoords (universe);
	}
}

static BOOLEAN
DoHyperspaceMenu (MENU_STATE *pMS)
{
	BOOLEAN select = PulsedInputState.menu[KEY_MENU_SELECT];
	BOOLEAN handled;

	if ((GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
			|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
		return FALSE;

	handled = DoMenuChooser (pMS, PM_STARMAP);
	if (handled)
		return TRUE;

	if (!select)
		return TRUE;

	SetFlashRect (NULL);

	switch (pMS->CurState)
	{
		case EQUIP_DEVICE:
			select = DevicesMenu ();
			if (GET_GAME_STATE (PORTAL_COUNTER))
			{	// A player-induced portal to QuasiSpace is opening
				return FALSE;
			}
			if (GLOBAL (CurrentActivity) & START_ENCOUNTER)
			{	// Selected Talking Pet, going into conversation
				return FALSE;
			}
			break;
		case CARGO:
			CargoMenu ();
			break;
		case ROSTER:
			select = RosterMenu ();
			break;
		case GAME_MENU:
			if (!GameOptions ())
				return FALSE; // abort or load
			break;
		case STARMAP:
			StarMap ();
			return FALSE;
		case NAVIGATION:
			return FALSE;
	}

	if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		if (select)
		{	// 3DO menu jumps to NAVIGATE after a successful submenu run
			if (optWhichMenu != OPT_PC)
				pMS->CurState = NAVIGATION;
			DrawMenuStateStrings (PM_STARMAP, pMS->CurState);
		}
		SetFlashRect (SFR_MENU_3DO);
	}

	return TRUE;
}

void
HyperspaceMenu (void)
{
	Color OldColor;
	CONTEXT OldContext;
	MENU_STATE MenuState;

UnbatchGraphics ();

	OldContext = SetContext (SpaceContext);
	OldColor = SetContextBackGroundColor (BLACK_COLOR);


	memset (&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoHyperspaceMenu;
	MenuState.Initialized = TRUE;
	MenuState.CurState = STARMAP;

	DrawMenuStateStrings (PM_STARMAP, STARMAP);
	SetFlashRect (SFR_MENU_3DO);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	DoInput (&MenuState, TRUE);

	SetFlashRect (NULL);

	SetContext (SpaceContext);

	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		ClearSISRect (CLEAR_SIS_RADAR);
		WaitForNoInput (ONE_SECOND / 2, FALSE);
	}

	SetContextBackGroundColor (OldColor);
	SetContext (OldContext);
	if (!(GLOBAL (CurrentActivity) & IN_BATTLE))
		cleanup_hyperspace ();

BatchGraphics ();
}

void
SaveSisHyperState (void)
{
	HELEMENT hSisElement;
	ELEMENT *ElementPtr;
	STARSHIP *StarShipPtr;

	// Update 'GLOBAL (ShipFacing)' to the direction the flagship is facing
	hSisElement = getSisElement ();
	if (!hSisElement)
	{	// Happens when saving a game from Hyperspace encounter screen
		return;
	}
	//if (ElementPtr->state_flags & PLAYER_SHIP)
	LockElement (hSisElement, &ElementPtr);
	GetElementStarShip (ElementPtr, &StarShipPtr);
	// XXX: Solar system reentry test depends on ShipFacing != 0
	GLOBAL (ShipFacing) = StarShipPtr->ShipFacing + 1;
	UnlockElement (hSisElement);
}

