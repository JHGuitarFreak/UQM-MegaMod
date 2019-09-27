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

#include "build.h"
#include "colors.h"
#include "controls.h"
#include "menustat.h"
#include "fmv.h"
#include "gameopt.h"
#include "gamestr.h"
#include "supermelee/melee.h"
#include "master.h"
#include "options.h"
#include "races.h"
#include "nameref.h"
#include "resinst.h"
#include "settings.h"
#include "starbase.h"
#include "setup.h"
#include "sis.h"
#include "units.h"
#include "sounds.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "uqmdebug.h"

#ifdef USE_3DO_HANGAR
// 3DO 4x3 hangar layout
#	define HANGAR_SHIPS_ROW  4
#	define HANGAR_Y          64
#	define HANGAR_DY         44

static const COORD hangar_x_coords[HANGAR_SHIPS_ROW] =
{
	19, 60, 116, 157
};

#else // use PC hangar
// modified PC 6x2 hangar layout
#	define HANGAR_SHIPS_ROW  6

// The Y position of the upper line of hangar bay doors.
# define HANGAR_Y	RES_SCALE(88) // JMS_GFX

// The Y position of the lower line of hangar bay doors.
# define HANGAR_DY	RES_SCALE(84) // JMS_GFX


// The X positions of the hangar bay doors for each resolution mode.
// Calculated from the right edge of the left grey border bar on the screen.
static const COORD hangar_x_coords_orig[HANGAR_SHIPS_ROW] = {
	0, 38, 76, 131, 169, 207
};
static const COORD hangar_x_coords_hd[HANGAR_SHIPS_ROW] = {
	55, 207, 359, 579, 731, 883
};
#endif // USE_3DO_HANGAR

#define HANGAR_SHIPS      12
#define HANGAR_ROWS       (HANGAR_SHIPS / HANGAR_SHIPS_ROW)
#define HANGAR_ANIM_RATE  15 // fps

enum
{
	SHIPYARD_CREW,
	SHIPYARD_SAVELOAD,
	SHIPYARD_EXIT
};

// Editing mode for DoModifyShips()
typedef enum {
	DMS_Mode_navigate,   // Navigating the ship slots.
	DMS_Mode_addEscort,  // Selecting a ship to add to an empty slot.
	DMS_Mode_editCrew,   // Hiring or dismissing crew.
	DMS_Mode_exit,       // Leaving DoModifyShips() mode.
} DMS_Mode;

static COUNT ShipCost[] =
{
	RACE_SHIP_COST
};

static BOOLEAN DoShipSpins;

static void
animatePowerLines (MENU_STATE *pMS)
{
	static STAMP s; 
	static COLORMAP ColorMap;
	static TimeCount NextTime = 0;
	TimeCount Now = GetTimeCounter ();

	if (pMS)
	{	// Init animation
		s.origin.x = 0;
		s.origin.y = 0;
		s.frame = SetAbsFrameIndex (pMS->ModuleFrame, 25);
		ColorMap = SetAbsColorMapIndex (pMS->CurString, 0);
	}

	if (Now >= NextTime || pMS)
	{
		NextTime = Now + (ONE_SECOND / HANGAR_ANIM_RATE);

		SetColorMap (GetColorMapAddress (ColorMap));
		DrawStamp (&s);
		// Advance colomap cycle
		ColorMap = SetRelColorMapIndex (ColorMap, 1);
	}
}

static void
SpinStarShip (MENU_STATE *pMS, HFLEETINFO hStarShip)
{
	int Index;
	FLEET_INFO *FleetPtr;

	FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	Index = FindMasterShipIndex (FleetPtr->SpeciesID);
	UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
				
	if (Index >= 0 && Index < NUM_MELEE_SHIPS)
	{
		DoShipSpins = TRUE;
		DoShipSpin (Index, pMS->hMusic);
	}
	DoShipSpins = FALSE;
}

static void
on_input_frame(void)
{
	CONTEXT oldContext;

	oldContext = SetContext(SpaceContext);
	if (!DoShipSpins)
		animatePowerLines(NULL);
	SetContext(oldContext);
}

// Count the ships which can be built by the player.
static COUNT
GetAvailableRaceCount (void)
{
	COUNT Index;
	HFLEETINFO hStarShip, hNextShip;

	Index = 0;
	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		if (FleetPtr->allied_state == GOOD_GUY || FleetPtr->allied_state == CAN_BUILD)
			++Index;

		hNextShip = _GetSuccLink (FleetPtr);
		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}

	return Index;
}

static HFLEETINFO
GetAvailableRaceFromIndex (BYTE Index)
{
	HFLEETINFO hStarShip, hNextShip;

	for (hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		if (FleetPtr->allied_state == GOOD_GUY && Index-- == 0 || FleetPtr->allied_state == CAN_BUILD && Index-- == 0)
		{
			UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
			return hStarShip;
		}

		hNextShip = _GetSuccLink (FleetPtr);
		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}

	return 0;
}

static void
DrawRaceStrings (MENU_STATE *pMS, BYTE NewRaceItem)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	

	OldContext = SetContext (StatusContext);
	GetContextClipRect (&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y;
	r.corner.x = s.origin.x - 1;
	r.corner.y = s.origin.y - RES_STAT_SCALE(11); // JMS_GFX
	r.extent.width = RADAR_WIDTH + 2;
	r.extent.height = RES_STAT_SCALE(11); // JMS_GFX
	BatchGraphics ();
	ClearSISRect (CLEAR_SIS_RADAR);
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	DrawFilledRectangle (&r);
	r.corner = s.origin;
	r.extent.width = RADAR_WIDTH;
	r.extent.height = RADAR_HEIGHT;
	SetContextForeGroundColor (BLACK_COLOR);
	DrawFilledRectangle (&r);
	if (NewRaceItem != (BYTE)~0)
	{
		TEXT t;
		HFLEETINFO hStarShip;
		FLEET_INFO *FleetPtr;
		UNICODE buf[30];

		hStarShip = GetAvailableRaceFromIndex (NewRaceItem);
		NewRaceItem = GetIndexFromStarShip (&GLOBAL (avail_race_q),
				hStarShip);

		// Draw the ship name, above the ship image.
		s.frame = SetAbsFrameIndex (pMS->ModuleFrame, 3 + NewRaceItem);
		DrawStamp (&s);

		// Draw the ship image.
		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		s.frame = FleetPtr->melee_icon;
		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		t.baseline.x = s.origin.x + RADAR_WIDTH - 2;
		t.baseline.y = s.origin.y + RADAR_HEIGHT - 2;
		s.origin.x += (RADAR_WIDTH >> 1);
		s.origin.y += (RADAR_HEIGHT >> 1);
		DrawStamp (&s);

		// Print the ship cost.
		t.align = ALIGN_RIGHT;
		t.CharCount = (COUNT)~0;
		t.pStr = buf;
		sprintf (buf, "%u", ShipCost[NewRaceItem]);
		SetContextFont (TinyFont);
		if ((ShipCost[NewRaceItem]) <= (GLOBAL_SIS (ResUnits))) {
			SetContextForeGroundColor (BRIGHT_GREEN_COLOR);
		} else if ((ShipCost[NewRaceItem]) > (GLOBAL_SIS (ResUnits)))
		{ /* We don't have enough to purchase this ship. */
			SetContextForeGroundColor (BRIGHT_RED_COLOR);
		}
		font_DrawText (&t);
	}
	UnbatchGraphics ();
	SetContext (OldContext);

}

// Width of an escort ship window.
#define SHIP_WIN_WIDTH RES_SCALE(34) // JMS_GFX

// Height of an escort ship window.
#define SHIP_WIN_HEIGHT (SHIP_WIN_WIDTH + RES_SCALE(6)) // JMS_GFX

// For how many animation frames' time the escort ship bay doors
// are slid left and right when opening them. If this number is not large
// enough, part of the doors are left visible upon opening.
#define SHIP_WIN_FRAMES ((SHIP_WIN_WIDTH >> 1) + 1)

// Print the crew count of an escort ship on top of its (already drawn)
// image, either as '30' (full), '28/30' (partially full), or 'SCRAP'
// (empty).
// pRect is the rectangle of the ship image.
static void
ShowShipCrew (SHIP_FRAGMENT *StarShipPtr, const RECT *pRect)
{
	RECT r;
	TEXT t;
	UNICODE buf[80];
	HFLEETINFO hTemplate;
	FLEET_INFO *TemplatePtr;
	COUNT maxCrewLevel;

	hTemplate = GetStarShipFromIndex (&GLOBAL (avail_race_q),
			StarShipPtr->race_id);
	TemplatePtr = LockFleetInfo (&GLOBAL (avail_race_q), hTemplate);
	maxCrewLevel = TemplatePtr->crew_level;
	UnlockFleetInfo (&GLOBAL (avail_race_q), hTemplate);

	if (StarShipPtr->crew_level >= maxCrewLevel)
		sprintf (buf, "%u", StarShipPtr->crew_level);
	else if (StarShipPtr->crew_level == 0)
		// XXX: "SCRAP" needs to be moved to starcon.txt
		utf8StringCopy (buf, sizeof (buf), "SCRAP");
	else
		sprintf (buf, "%u/%u", StarShipPtr->crew_level, maxCrewLevel);

	r = *pRect;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + r.extent.height - 1; // JMS_GFX
	t.align = ALIGN_CENTER;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	if (r.corner.y)
	{
		r.corner.y = t.baseline.y - RES_SCALE(6); //JMS_GFX
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = RES_SCALE(6); // JMS_GFX
		SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledRectangle (&r);
	}
	SetContextForeGroundColor ((StarShipPtr->crew_level != 0) ?
			(BUILD_COLOR (MAKE_RGB15 (0x00, 0x14, 0x00), 0x02)):
			(BUILD_COLOR (MAKE_RGB15 (0x12, 0x00, 0x00), 0x2B)));
	font_DrawText (&t);
}

static void
ShowCombatShip (MENU_STATE *pMS, COUNT which_window,
		SHIP_FRAGMENT *YankedStarShipPtr)
{
	COUNT i;
	COUNT num_ships;
	HSHIPFRAG hStarShip, hNextShip;
	SHIP_FRAGMENT *StarShipPtr;
	static const COORD *hangar_x_coords;
	struct
	{
		SHIP_FRAGMENT *StarShipPtr;
		POINT finished_s;
		STAMP ship_s;
		STAMP lfdoor_s;
		STAMP rtdoor_s;
	} ship_win_info[MAX_BUILT_SHIPS], *pship_win_info;

	hangar_x_coords = RES_BOOL(hangar_x_coords_orig, hangar_x_coords_hd);

	num_ships = 1;
	pship_win_info = &ship_win_info[0];
	if (YankedStarShipPtr)
	{
		pship_win_info->StarShipPtr = YankedStarShipPtr;

		pship_win_info->lfdoor_s.origin.x = -(SHIP_WIN_WIDTH >> 1);
		pship_win_info->rtdoor_s.origin.x = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->lfdoor_s.origin.y = 0;
		pship_win_info->rtdoor_s.origin.y = 0;
		pship_win_info->lfdoor_s.frame = IncFrameIndex (pMS->ModuleFrame);
		pship_win_info->rtdoor_s.frame =
				IncFrameIndex (pship_win_info->lfdoor_s.frame);

		pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
		pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
		pship_win_info->ship_s.frame = YankedStarShipPtr->melee_icon;

		pship_win_info->finished_s.x = hangar_x_coords[
				which_window % HANGAR_SHIPS_ROW];
		pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY *
				(which_window / HANGAR_SHIPS_ROW));
	}
	else
	{
		if (which_window == (COUNT)~0)
		{
			hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			num_ships = CountLinks (&GLOBAL (built_ship_q));
		}
		else
		{
			HSHIPFRAG hTailShip;

			hTailShip = GetTailLink (&GLOBAL (built_ship_q));
			RemoveQueue (&GLOBAL (built_ship_q), hTailShip);

			hStarShip = GetHeadLink (&GLOBAL (built_ship_q));
			while (hStarShip)
			{
				StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
				if (StarShipPtr->index > which_window)
				{
					UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
					break;
				}
				hNextShip = _GetSuccLink (StarShipPtr);
				UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

				hStarShip = hNextShip;
			}
			InsertQueue (&GLOBAL (built_ship_q), hTailShip, hStarShip);

			hStarShip = hTailShip;
			StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			StarShipPtr->index = which_window;
			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
		}

		for (i = 0; i < num_ships; ++i)
		{
			StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			hNextShip = _GetSuccLink (StarShipPtr);

			pship_win_info->StarShipPtr = StarShipPtr;
					// XXX BUG: this looks wrong according to the original
					// semantics of LockShipFrag(): StarShipPtr is not valid
					// anymore after UnlockShipFrag() is called, but it is
					// used thereafter.

			pship_win_info->lfdoor_s.origin.x = -1;
			pship_win_info->rtdoor_s.origin.x = 1;
			pship_win_info->lfdoor_s.origin.y = 0;
			pship_win_info->rtdoor_s.origin.y = 0;
			pship_win_info->lfdoor_s.frame = IncFrameIndex (pMS->ModuleFrame);
			pship_win_info->rtdoor_s.frame =
					IncFrameIndex (pship_win_info->lfdoor_s.frame);

			pship_win_info->ship_s.origin.x = (SHIP_WIN_WIDTH >> 1) + 1;
			pship_win_info->ship_s.origin.y = (SHIP_WIN_WIDTH >> 1);
			pship_win_info->ship_s.frame = StarShipPtr->melee_icon;

			which_window = StarShipPtr->index;
			pship_win_info->finished_s.x = hangar_x_coords[
					which_window % HANGAR_SHIPS_ROW];
			pship_win_info->finished_s.y = HANGAR_Y + (HANGAR_DY *
					(which_window / HANGAR_SHIPS_ROW));
			++pship_win_info;

			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
			hStarShip = hNextShip;
		}
	}

	if (num_ships)
	{
		BOOLEAN AllDoorsFinished;
		DWORD TimeIn;
		RECT r;
		CONTEXT OldContext;
		RECT OldClipRect;
		int j;

		AllDoorsFinished = FALSE;
		r.corner.x = r.corner.y = 0;
		r.extent.width = SHIP_WIN_WIDTH;
		r.extent.height = SHIP_WIN_HEIGHT;
		FlushInput ();
		TimeIn = GetTimeCounter ();

		for (j = 0; (j < (int)SHIP_WIN_FRAMES) && !AllDoorsFinished; j++)
		{
			SleepThreadUntil (TimeIn + ONE_SECOND / RES_SCALE(24));
			TimeIn = GetTimeCounter ();
			if (AnyButtonPress (FALSE))
			{
				if (YankedStarShipPtr != 0)
				{	// Fully close the doors
					ship_win_info[0].lfdoor_s.origin.x = 0;
					ship_win_info[0].rtdoor_s.origin.x = 0;
				}
				AllDoorsFinished = TRUE;
			}

			OldContext = SetContext (SpaceContext);
			GetContextClipRect (&OldClipRect);
			SetContextBackGroundColor (BLACK_COLOR);

			BatchGraphics ();
			pship_win_info = &ship_win_info[0];
			for (i = 0; i < num_ships; ++i)
			{
				{
					RECT ClipRect;

					ClipRect.corner.x = SIS_ORG_X +
							pship_win_info->finished_s.x;
					ClipRect.corner.y = SIS_ORG_Y +
							pship_win_info->finished_s.y;
					ClipRect.extent.width = SHIP_WIN_WIDTH;
					ClipRect.extent.height = SHIP_WIN_HEIGHT;
					SetContextClipRect (&ClipRect);
					
					ClearDrawable ();
					DrawStamp (&pship_win_info->ship_s);
					ShowShipCrew (pship_win_info->StarShipPtr, &r);
					if (!AllDoorsFinished || YankedStarShipPtr)
					{
						DrawStamp (&pship_win_info->lfdoor_s);
						DrawStamp (&pship_win_info->rtdoor_s);
						if (YankedStarShipPtr)
						{	// Close the doors
							++pship_win_info->lfdoor_s.origin.x;
							--pship_win_info->rtdoor_s.origin.x;
						}
						else
						{	// Open the doors
							--pship_win_info->lfdoor_s.origin.x;
							++pship_win_info->rtdoor_s.origin.x;
						}
					}
				}
				++pship_win_info;
			}

			SetContextClipRect (&OldClipRect);
#ifndef USE_3DO_HANGAR
			animatePowerLines (NULL);
#endif
			UnbatchGraphics ();
			SetContext (OldContext);
		}
	}
}

static void
CrewTransaction (SIZE crew_delta)
{
	if (crew_delta)
	{
		SIZE crew_bought;

		crew_bought = (SIZE)MAKE_WORD (
				GET_GAME_STATE (CREW_PURCHASED0),
				GET_GAME_STATE (CREW_PURCHASED1)) + crew_delta;
		if (crew_bought < 0)
		{
			if (crew_delta < 0)
				crew_bought = 0;
			else
				crew_bought = 0x7FFF;
		}
		else if (crew_delta > 0)
		{
			if (crew_bought >= CREW_EXPENSE_THRESHOLD
					&& crew_bought - crew_delta < CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL (CrewCost) += 2;
				DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
			}
		}
		else
		{
			if (crew_bought < CREW_EXPENSE_THRESHOLD
					&& crew_bought - crew_delta >= CREW_EXPENSE_THRESHOLD)
			{
				GLOBAL (CrewCost) -= 2;
				DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
			}
		}
		if (CheckAlliance (SHOFIXTI_SHIP) != GOOD_GUY)
		{
			SET_GAME_STATE (CREW_PURCHASED0, LOBYTE (crew_bought));
			SET_GAME_STATE (CREW_PURCHASED1, HIBYTE (crew_bought));
		}
	}
}

static void
DMS_FlashFlagShip (void)
{
	RECT r;
	r.corner.x = 0;
	r.corner.y = 0;
	r.extent.width = SIS_SCREEN_WIDTH;
	r.extent.height = RES_BOOL(61, 295); // JMS_GFX
	SetFlashRect (&r);
}

static void
DMS_GetEscortShipRect (RECT *rOut, BYTE slotNr)
{
	BYTE row = slotNr / HANGAR_SHIPS_ROW;
	BYTE col = slotNr % HANGAR_SHIPS_ROW;
	static const COORD *hangar_x_coords;

	hangar_x_coords = RES_BOOL(hangar_x_coords_orig, hangar_x_coords_hd);

	rOut->corner.x = hangar_x_coords[col];
	rOut->corner.y = HANGAR_Y + (HANGAR_DY * row);
	rOut->extent.width = SHIP_WIN_WIDTH;
	rOut->extent.height = SHIP_WIN_HEIGHT;
}

static void
DMS_FlashEscortShip (BYTE slotNr)
{
	RECT r;
	DMS_GetEscortShipRect (&r, slotNr);
	SetFlashRect (&r);
}

static void
DMS_FlashFlagShipCrewCount (void)
{
	RECT r;
	SetContext (StatusContext);
	GetGaugeRect (&r, TRUE);
	SetFlashRect (&r);
	SetContext (SpaceContext);
}

static void
DMS_FlashEscortShipCrewCount (BYTE slotNr)
{
	RECT r;
	BYTE row = slotNr / HANGAR_SHIPS_ROW;
	BYTE col = slotNr % HANGAR_SHIPS_ROW;
	static const COORD *hangar_x_coords;

	hangar_x_coords = RES_BOOL(hangar_x_coords_orig, hangar_x_coords_hd);

	r.corner.x = hangar_x_coords[col];
	r.corner.y = (HANGAR_Y + (HANGAR_DY * row)) + (SHIP_WIN_HEIGHT - RES_SCALE(6));
	r.extent.width = SHIP_WIN_WIDTH;
	r.extent.height = RES_SCALE(5); // JMS_GFX

	SetContext (SpaceContext);
	SetFlashRect (&r);
}

// Helper function for DoModifyShips(). Called to change the flash
// rectangle to the currently selected ship (flagship or escort ship).
static void
DMS_FlashActiveShip (MENU_STATE *pMS)
{
	if (HINIBBLE (pMS->CurState))
	{
		// Flash the flag ship.
		DMS_FlashFlagShip ();
	}
	else
	{
		// Flash the current escort ship slot.
		DMS_FlashEscortShip (pMS->CurState);
	}
}

// Helper function for DoModifyShips(). Called to switch between
// the various edit modes.
// XXX: right now, this only switches the sound and flash rectangle.
// Perhaps we should move more of the code to modify other aspects
// here too.
static void
DMS_SetMode (MENU_STATE *pMS, DMS_Mode mode)
{
	switch (mode) {
		case DMS_Mode_navigate:
			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			DMS_FlashActiveShip (pMS);
			break;
		case DMS_Mode_addEscort:
			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			SetFlashRect (SFR_MENU_ANY);
			break;
		case DMS_Mode_editCrew:
			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_PAGE | MENU_SOUND_ACTION);
			if (HINIBBLE (pMS->CurState))
			{
				// Enter crew editing mode for the flagship.
				DMS_FlashFlagShipCrewCount ();
			}
			else
			{
				// Enter crew editing mode for an escort ship.
				DMS_FlashEscortShipCrewCount (pMS->CurState);
			}
			break;
		case DMS_Mode_exit:
			SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
			SetFlashRect (SFR_MENU_3DO);
			break;
	}
}

#define MODIFY_CREW_FLAG (1 << 8)
// Helper function for DoModifyShips(), called when the player presses the
// special button.
// It works both when the cursor is over an escort ship, while not editing
// the crew, and when a new ship is added.
// hStarShip is the ship in the slot under the cursor (or 0 if no such ship).
static BOOLEAN
DMS_SpinShip (MENU_STATE *pMS, HSHIPFRAG hStarShip)
{
	HFLEETINFO hSpinShip = 0;
	CONTEXT OldContext;
	RECT OldClipRect;
	
	// No spinning the flagship.
	if (HINIBBLE (pMS->CurState) != 0)
		return FALSE;

	// We must either be hovering over a used ship slot, or adding a new
	// ship to the fleet.
	if ((hStarShip == 0) == !(pMS->delta_item & MODIFY_CREW_FLAG))
		return FALSE;

	if (!hStarShip)
	{
		// Selecting a ship to build.
		hSpinShip = GetAvailableRaceFromIndex (LOBYTE (pMS->delta_item));
		if (!hSpinShip)
			return FALSE;
	}
	else
	{
		// Hovering over an escort ship.
		SHIP_FRAGMENT *FragPtr = LockShipFrag (
				&GLOBAL (built_ship_q), hStarShip);
		hSpinShip = GetStarShipFromIndex (
				&GLOBAL (avail_race_q), FragPtr->race_id);
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	}
	
	SetFlashRect (NULL);

	OldContext = SetContext (ScreenContext);
	GetContextClipRect (&OldClipRect);

	SpinStarShip (pMS, hSpinShip);

	SetContextClipRect (&OldClipRect);
	SetContext (OldContext);

	return TRUE;
}

// Helper function for DoModifyShips(), called when the player presses the
// up button when modifying the crew of the flagship.
// Buy crew for the flagship.
// Returns the change in crew (1 on success, 0 on failure).
static SIZE
DMS_HireFlagShipCrew (void)
{
	RECT r;
	
	if (GetCPodCapacity (&r.corner) <= GetCrewCount ())
	{
		// At capacity.
		return 0;
	}
		
	if (GLOBAL_SIS (ResUnits) < (DWORD)GLOBAL (CrewCost))
	{
		// Not enough RUs.
		return 0;
	}

	// Draw a crew member.
	// Crew dots/rectangles for Original and HD graphics.
	if (!IS_HD) {
		r.extent.width = RES_SCALE(1);
		r.extent.height = r.extent.width;
		DrawFilledRectangle (&r);
	} else {
		r.corner.x += 1;
		r.extent.width = RES_SCALE(1) - 2;
		r.extent.height = RES_SCALE(1);
		DrawFilledRectangle (&r);
									
		r.corner.x -= 1;
		r.corner.y += 1;
		r.extent.width = RES_SCALE(1);
		r.extent.height = RES_SCALE(1) - 2;
		DrawFilledRectangle (&r);
									
		r.corner.y -= 1;
	}

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect ();
	DeltaSISGauges (1, 0, -GLOBAL (CrewCost));
	PostUpdateFlashRect ();

	return 1;
}

// Helper function for DoModifyShips(), called when the player presses the
// down button when modifying the crew of the flagship.
// Dismiss crew from the flagship.
// Returns the change in crew (-1 on success, 0 on failure).
static SIZE
DMS_DismissFlagShipCrew (void)
{
	SIZE crew_bought;
	RECT r;

	if (GetCrewCount () == 0)
	{
		// No crew to dismiss.
		return 0;
	}

	crew_bought = (SIZE)MAKE_WORD (
			GET_GAME_STATE (CREW_PURCHASED0),
			GET_GAME_STATE (CREW_PURCHASED1));

	// Update the crew counter and RU. Note that the crew counter is
	// flashing.
	PreUpdateFlashRect ();
	DeltaSISGauges (-1, 0, GLOBAL (CrewCost) -
			(crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
	PostUpdateFlashRect ();

	// Remove the pixel representing the crew member.
	GetCPodCapacity (&r.corner);
	r.extent.width = RES_SCALE(1);
	r.extent.height = r.extent.width;
	SetContextForeGroundColor (BLACK_COLOR);
	DrawFilledRectangle (&r);

	return -1;
}

// Helper function for DoModifyShips(), called when the player presses the
// up button when modifying the crew of an escort ship.
// Buy crew for an escort ship
// Returns the change in crew (1 on success, 0 on failure).
static SIZE
DMS_HireEscortShipCrew (SHIP_FRAGMENT *StarShipPtr)
{
	COUNT templateMaxCrew;
	RECT r;

	{
		// XXX Split this off into a separate function?
		HFLEETINFO hTemplate = GetStarShipFromIndex (&GLOBAL (avail_race_q),
				StarShipPtr->race_id);
		FLEET_INFO *TemplatePtr =
				LockFleetInfo (&GLOBAL (avail_race_q), hTemplate);
		templateMaxCrew = TemplatePtr->crew_level;
		UnlockFleetInfo (&GLOBAL (avail_race_q), hTemplate);
	}
	
	if (GLOBAL_SIS (ResUnits) < (DWORD)GLOBAL (CrewCost))
	{
		// Not enough money to hire a crew member.
		return 0;
	}

	if (StarShipPtr->crew_level >= StarShipPtr->max_crew)
	{
		// This ship cannot handle more crew.
		return 0;
	}

	if (StarShipPtr->crew_level >= templateMaxCrew)
	{
		// A ship of this type cannot handle more crew.
		return 0;
	}

	if (StarShipPtr->crew_level > 0)
	{
		DeltaSISGauges (0, 0, -GLOBAL (CrewCost));
	}
	else
	{
		// Buy a ship.
		DeltaSISGauges (0, 0, -(COUNT)ShipCost[StarShipPtr->race_id]);
	}

	++StarShipPtr->crew_level;

	PreUpdateFlashRect ();
	DMS_GetEscortShipRect (&r, StarShipPtr->index);
	ShowShipCrew (StarShipPtr, &r);
	PostUpdateFlashRect ();

	return 1;
}

// Helper function for DoModifyShips(), called when the player presses the
// down button when modifying the crew of an escort ship.
// Dismiss crew from an escort ship
// Returns the change in crew (-1 on success, 0 on failure).
static SIZE
DMS_DismissEscortShipCrew (SHIP_FRAGMENT *StarShipPtr)
{
	SIZE crew_delta = 0;
	RECT r;

	if (StarShipPtr->crew_level > 0)
	{
		if (StarShipPtr->crew_level > 1)
		{
			// The ship was not at 'scrap'.
			// Give one crew member worth of RU.
			SIZE crew_bought = (SIZE)MAKE_WORD (
					GET_GAME_STATE (CREW_PURCHASED0),
					GET_GAME_STATE (CREW_PURCHASED1));

			DeltaSISGauges (0, 0, GLOBAL (CrewCost)
					- (crew_bought == CREW_EXPENSE_THRESHOLD ? 2 : 0));
		}
		else
		{
			// With the last crew member, the ship will be scrapped.
			// Give RU for the ship.
			DeltaSISGauges (0, 0, (COUNT)ShipCost[StarShipPtr->race_id]);
		}
		crew_delta = -1;
		--StarShipPtr->crew_level;
	}
	else
	{	// no crew to dismiss
		PlayMenuSound (MENU_SOUND_FAILURE);
	}

	PreUpdateFlashRect ();
	DMS_GetEscortShipRect (&r, StarShipPtr->index);
	ShowShipCrew (StarShipPtr, &r);
	PostUpdateFlashRect ();

	return crew_delta;
}

// Helper function for DoModifyShips(), called when the player presses the
// up or down button when modifying the crew of the flagship or of an escort
// ship.
// 'hStarShip' is the currently escort ship, or 0 if no ship is
// selected.
// 'dy' is -1 if the 'up' button was pressed, or '1' if the down button was
// pressed.
static void
DMS_ModifyCrew (MENU_STATE *pMS, HSHIPFRAG hStarShip, SBYTE dy)
{
	SIZE crew_delta = 0;
	SHIP_FRAGMENT *StarShipPtr = NULL;
	COUNT loop;
	COUNT DoLoop = 1;
	RECT r;

	if (dy == -10 || dy == 10)
		DoLoop = 10;

	if (dy == -50 || dy == 50)
		PlayMenuSound(MENU_SOUND_INVOKED);

	if (hStarShip)
		StarShipPtr = LockShipFrag (&GLOBAL (built_ship_q), hStarShip);

	if (hStarShip == 0) {
		if (dy == -50)
			DoLoop = GetCPodCapacity(&r.corner) - GetCrewCount();
		else if (dy == 50)
			DoLoop = GetCrewCount();

		// Add/Dismiss crew for the flagship.
		for (loop = 0; loop < DoLoop; loop++) {
			if (dy < 0) {
				// Add crew for the flagship.
				crew_delta += DMS_HireFlagShipCrew ();
			} else {
				// Dismiss crew from the flagship.
				crew_delta -= DMS_DismissFlagShipCrew ();
			}
		}

		if (crew_delta != 0)
			DMS_FlashFlagShipCrewCount ();
	} else {
		if (dy == -50)
			DoLoop = StarShipPtr->max_crew - StarShipPtr->crew_level;
		else if (dy == 50)
			DoLoop = StarShipPtr->crew_level;

		// Add/Dismiss crew for an escort ship.
		for (loop = 0; loop < DoLoop; loop++) {
			if (dy < 0) {
				// Add crew for an escort ship.
				crew_delta = DMS_HireEscortShipCrew (StarShipPtr);
			} else {
				// Dismiss crew from an escort ship.
				crew_delta = DMS_DismissEscortShipCrew (StarShipPtr);
			}
		}

		if (crew_delta != 0)
			DMS_FlashEscortShipCrewCount (StarShipPtr->index);
	}

	if (crew_delta == 0)
		PlayMenuSound (MENU_SOUND_FAILURE);

	if (hStarShip)
	{
		UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

		// Clear out the bought ship index so that flash rects work
		// correctly.
		pMS->delta_item &= MODIFY_CREW_FLAG;
	}

	CrewTransaction (crew_delta);
}

// Helper function for DoModifyShips(), called when the player presses the
// select button when the cursor is over an empty escort ship slot.
// Try to add the currently selected ship as an escort ship.
static void
DMS_TryAddEscortShip (MENU_STATE *pMS)
{
	HFLEETINFO shipInfo = GetAvailableRaceFromIndex (
			LOBYTE (pMS->delta_item));
	COUNT Index = GetIndexFromStarShip (&GLOBAL (avail_race_q), shipInfo);
	BYTE MaxBuild = 2;

	if ((DIF_HARD && CountEscortShips(Index) < MaxBuild || !DIF_HARD)
			&& GLOBAL_SIS (ResUnits) >= (DWORD)ShipCost[Index]
			&& CloneShipFragment (Index, &GLOBAL (built_ship_q), 1))
	{
		ShowCombatShip (pMS, pMS->CurState, NULL);
				// Reset flash rectangle
		DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);

		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA,
				-((int)ShipCost[Index]));
		DMS_SetMode (pMS, DMS_Mode_editCrew);
	}
	else
	{
		// not enough RUs to build, cloning the ship failed, 
		// or reached max ship limit in hard mode
		PlayMenuSound (MENU_SOUND_FAILURE);
	}
}

// Helper function for DoModifyShips(), called when the player is in the
// mode to add a new escort ship to the fleet (after pressing select on an
// empty slot).
// LOBYTE (pMS->delta_item) is used to store the currently highlighted ship.
// Returns FALSE if the flash rectangle needs to be updated.
static void
DMS_AddEscortShip (MENU_STATE *pMS, BOOLEAN special, BOOLEAN select,
		BOOLEAN cancel, SBYTE dx, SBYTE dy)
{
	assert (pMS->delta_item & MODIFY_CREW_FLAG);

	if (special)
	{
		HSHIPFRAG hStarShip = GetEscortByStarShipIndex (pMS->delta_item);
		if (DMS_SpinShip (pMS, hStarShip))
			DMS_SetMode (pMS, DMS_Mode_addEscort);
		return;
	}

	if (cancel)
	{
		// Cancel selecting an escort ship.
		pMS->delta_item &= ~MODIFY_CREW_FLAG;
		SetFlashRect (NULL);
		DrawMenuStateStrings (PM_CREW, SHIPYARD_CREW);
		DMS_SetMode (pMS, DMS_Mode_navigate);
	}
	else if (select)
	{
		// Selected a ship to be inserted in an empty escort
		// ship slot.
		DMS_TryAddEscortShip (pMS);
	}
	else if (dx || dy)
	{
		// Motion key pressed while selecting a ship to be
		// inserted in an empty escort ship slot.
		COUNT availableCount = GetAvailableRaceCount ();
		BYTE currentShip = LOBYTE (pMS->delta_item);
		if (dx < 0 || dy < 0)
		{
			if (currentShip-- == 0)
				currentShip = availableCount - 1;
		}
		else if (dx > 0 || dy > 0)
		{
			if (++currentShip == availableCount)
				currentShip = 0;
		}
		
		if (currentShip != LOBYTE (pMS->delta_item))
		{
			PreUpdateFlashRect ();
			DrawRaceStrings (pMS, currentShip);
			PostUpdateFlashRect ();
			pMS->delta_item = currentShip | MODIFY_CREW_FLAG;
		}
	}
}

// Helper function for DoModifyShips(), called when the player presses
// 'select' or 'cancel' after selling all the crew.
static void
DMS_ScrapEscortShip (MENU_STATE *pMS, HSHIPFRAG hStarShip)
{
	SHIP_FRAGMENT *StarShipPtr =
			LockShipFrag (&GLOBAL (built_ship_q), hStarShip);
	BYTE slotNr;

	SetFlashRect (NULL);
	ShowCombatShip (pMS, pMS->CurState, StarShipPtr);

	slotNr = StarShipPtr->index;
	UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

	RemoveQueue (&GLOBAL (built_ship_q), hStarShip);
	FreeShipFrag (&GLOBAL (built_ship_q), hStarShip);
	// refresh SIS display
	DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);

	SetContext (SpaceContext);
	DMS_SetMode (pMS, DMS_Mode_navigate);
}

// Helper function for DoModifyShips(), called when the player presses
// one of the motion keys when not in crew modification mode.
static BYTE
DMS_MoveCursor (BYTE curState, SBYTE dx, SBYTE dy)
{
	BYTE row = LONIBBLE(curState) / HANGAR_SHIPS_ROW;
	BYTE col = LONIBBLE(curState) % HANGAR_SHIPS_ROW;
	BOOLEAN isFlagShipSelected = (HINIBBLE(curState) != 0);

	if (dy)
	{
		// Vertical motion.
	
		// We consider the flagship an extra row (on the bottom),
		// to ease operations.
		if (isFlagShipSelected)
			row = HANGAR_ROWS;

		// Move up/down, wrapping around:
		row = (row + (HANGAR_ROWS + 1) + dy) % (HANGAR_ROWS + 1);

		// If we moved to the 'extra row', this means the flag ship.
		isFlagShipSelected = (row == HANGAR_ROWS);
		if (isFlagShipSelected)
			row = 0;
	}
	else if (dx)
	{
		// Horizontal motion.
		if (!isFlagShipSelected)
		{
			// Moving horizontally through the escort ship slots,
			// wrapping around if necessary.
			col = (col + HANGAR_SHIPS_ROW + dx) % HANGAR_SHIPS_ROW;
		}
	}
		
	return MAKE_BYTE(row * HANGAR_SHIPS_ROW + col,
			isFlagShipSelected ? 0xf : 0);
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in crew editing mode.
static void
DMS_EditCrewMode (MENU_STATE *pMS, HSHIPFRAG hStarShip,
		BOOLEAN select, BOOLEAN cancel, SBYTE dy)
{
	if (select || cancel)
	{
		// Leave crew editing mode.
		if (hStarShip != 0)
		{
			// Exiting crew editing mode for an escort ship.
			SHIP_FRAGMENT *StarShipPtr = LockShipFrag (
					&GLOBAL (built_ship_q), hStarShip);
			COUNT crew_level = StarShipPtr->crew_level;
			UnlockShipFrag (&GLOBAL (built_ship_q), hStarShip);

			if (crew_level == 0)
			{
				// Scrapping the escort ship before exiting crew edit
				// mode.
				DMS_ScrapEscortShip (pMS, hStarShip);
			}
		}

		pMS->delta_item &= ~MODIFY_CREW_FLAG;
		DMS_SetMode (pMS, DMS_Mode_navigate);
	}
	else if (dy)
	{
		// Hire or dismiss crew for the flagship or an escort
		// ship.
		DMS_ModifyCrew (pMS, hStarShip, dy);
	}
}

// Helper function for DoModifyShips(), called every time DoModifyShip() is
// called when we are in the mode where you can select a ship or empty slot.
static void
DMS_NavigateShipSlots (MENU_STATE *pMS, BOOLEAN special, BOOLEAN select,
		BOOLEAN cancel, SBYTE dx, SBYTE dy)
{
	HSHIPFRAG hStarShip = GetEscortByStarShipIndex (pMS->CurState);

	if (dx || dy)
	{
		// Moving through the ship slots.
		BYTE NewState = DMS_MoveCursor (pMS->CurState, dx, dy);
		if (NewState != pMS->CurState)
		{
			pMS->CurState = NewState;
			DMS_FlashActiveShip(pMS);
		}
	}

	if (special)
	{
		if (DMS_SpinShip (pMS, hStarShip))
			DMS_SetMode (pMS, DMS_Mode_navigate);
	}
	else if (select)
	{
		if (hStarShip == 0 && HINIBBLE (pMS->CurState) == 0)
		{
			// Select button was pressed over an empty escort
			// ship slot. Switch to 'add escort ship' mode.
			pMS->delta_item = MODIFY_CREW_FLAG;
			DrawRaceStrings (pMS, 0);
			DMS_SetMode (pMS, DMS_Mode_addEscort);
		}
		else
		{
			// Select button was pressed over an escort ship or
			// the flagship. Entering crew editing mode
			pMS->delta_item |= MODIFY_CREW_FLAG;
			DMS_SetMode (pMS, DMS_Mode_editCrew);
		}
	}
	else if (cancel) {
		// Leave escort ship editor.
		pMS->InputFunc = DoShipyard;
		pMS->CurState = SHIPYARD_CREW;
		DrawMenuStateStrings (PM_CREW, pMS->CurState);
		DMS_SetMode (pMS, DMS_Mode_exit);
	}
}

/* In this routine, the least significant byte of pMS->CurState is used
 * to store the current selected ship index
 * a special case for the row is hi-nibble == -1 (0xf), which specifies
 * SIS as the selected ship
 * some bitwise math is still done to scroll through ships, for it to work
 * ships per row number must divide 0xf0 without remainder
 */
static BOOLEAN
DoModifyShips (MENU_STATE *pMS)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->InputFunc = DoShipyard;
		return TRUE;
	}

	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoModifyShips;
		pMS->Initialized = TRUE;
		pMS->CurState = MAKE_BYTE (0, 0xF);
		pMS->delta_item = 0;

		SetContext (SpaceContext);
		DMS_SetMode (pMS, DMS_Mode_navigate);
	}
	else
	{
		BOOLEAN special = (PulsedInputState.menu[KEY_MENU_SPECIAL] != 0);
		BOOLEAN select = (PulsedInputState.menu[KEY_MENU_SELECT] != 0);
		BOOLEAN cancel = (PulsedInputState.menu[KEY_MENU_CANCEL] != 0);
		SBYTE dx = 0;
		SBYTE dy = 0;

		if (PulsedInputState.menu[KEY_MENU_RIGHT])
			dx = 1;
		if (PulsedInputState.menu[KEY_MENU_LEFT])
			dx = -1;
		if (PulsedInputState.menu[KEY_MENU_UP])
			dy = -1;
		if (PulsedInputState.menu[KEY_MENU_DOWN])
			dy = 1;
		if (PulsedInputState.menu[KEY_MENU_PAGE_UP])
			dy = -10;
		if (PulsedInputState.menu[KEY_MENU_PAGE_DOWN])
			dy = 10;
		if (PulsedInputState.menu[KEY_MENU_HOME])
			dy = -50;
		if (PulsedInputState.menu[KEY_MENU_END])
			dy = 50;


		if (!(pMS->delta_item & MODIFY_CREW_FLAG))
		{
			// Navigating through the ship slots.
			DMS_NavigateShipSlots (pMS, special, select, cancel, dx, dy);
		}
		else
		{
			// Add an escort ship or edit the crew of a ship.
			HSHIPFRAG hStarShip = GetEscortByStarShipIndex (pMS->CurState);

			if (hStarShip == 0 && HINIBBLE (pMS->CurState) == 0)
			{
				// Cursor is over an empty escort ship slot, while we're
				// in 'add escort ship' mode.
				DMS_AddEscortShip (pMS, special, select, cancel, dx, dy);
			}
			else
			{
				// Crew editing mode.
				DMS_EditCrewMode (pMS, hStarShip, select, cancel, dy);
			}
		}

	}

	SleepThread (ONE_SECOND / 30);

	return TRUE;
}

static void
DrawBluePrint (MENU_STATE *pMS)
{
	COUNT num_frames;
	STAMP s;
	FRAME ModuleFrame;

	ModuleFrame = CaptureDrawable (LoadGraphic (SISBLU_MASK_ANIM));

	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = DecFrameIndex (ModuleFrame);
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x16), 0x01));
	DrawFilledStamp (&s);

	for (num_frames = 0; num_frames < NUM_DRIVE_SLOTS; ++num_frames)
	{
		DrawShipPiece (ModuleFrame, GLOBAL_SIS (DriveSlots[num_frames]),
				num_frames, TRUE);
	}
	for (num_frames = 0; num_frames < NUM_JET_SLOTS; ++num_frames)
	{
		DrawShipPiece (ModuleFrame, GLOBAL_SIS (JetSlots[num_frames]),
				num_frames, TRUE);
	}
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		BYTE which_piece;

		which_piece = GLOBAL_SIS (ModuleSlots[num_frames]);

		if (!(pMS->CurState == SHIPYARD && which_piece == CREW_POD))
			DrawShipPiece (ModuleFrame, which_piece, num_frames, TRUE);
	}

	if (!IS_HD)
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x1F), 0x09));
	for (num_frames = 0; num_frames < NUM_MODULE_SLOTS; ++num_frames)
	{
		BYTE which_piece;

		which_piece = GLOBAL_SIS (ModuleSlots[num_frames]);
		if (pMS->CurState == SHIPYARD && which_piece == CREW_POD)
			DrawShipPiece (ModuleFrame, which_piece, num_frames, TRUE);
	}

	{
		num_frames = GLOBAL_SIS (CrewEnlisted);
		GLOBAL_SIS (CrewEnlisted) = 0;

		while (num_frames--)
		{
			RECT r;
			// Crew dots/rectangles for Original and HD graphics.
			if (!IS_HD) {
				r.extent.width = RES_SCALE(1);
				r.extent.height = r.extent.width;
				
				GetCPodCapacity (&r.corner);
				DrawFilledRectangle (&r);
			} else {
				GetCPodCapacity (&r.corner);
				
				r.corner.x += 1;
				r.extent.width = RES_SCALE(1) - 2;
				r.extent.height = RES_SCALE(1);
				DrawFilledRectangle (&r);
				
				r.corner.x -= 1;
				r.corner.y += 1;
				r.extent.width = RES_SCALE(1);
				r.extent.height = RES_SCALE(1) - 2;
				DrawFilledRectangle (&r);
				
				r.corner.y -= 1;
			}

			++GLOBAL_SIS (CrewEnlisted);
		}
	}
	{
		RECT r;

		num_frames = GLOBAL_SIS (TotalElementMass);
		GLOBAL_SIS (TotalElementMass) = 0;

		r.extent.width = RES_SCALE(9); // JMS_GFX
		r.extent.height = RES_SCALE(1); // JMS_GFX
		while (num_frames)
		{
			COUNT m;

			m = num_frames < SBAY_MASS_PER_ROW ?
					num_frames : SBAY_MASS_PER_ROW;
			GLOBAL_SIS (TotalElementMass) += m;
			GetSBayCapacity (&r.corner);
			DrawFilledRectangle (&r);
			num_frames -= m;
		}
	}
	if (GLOBAL_SIS (FuelOnBoard) > FUEL_RESERVE)
	{
		DWORD FuelVolume;
		RECT r;

		FuelVolume = GLOBAL_SIS (FuelOnBoard) - FUEL_RESERVE;
		GLOBAL_SIS (FuelOnBoard) = FUEL_RESERVE;

		r.extent.width = RES_SCALE(3) + RESOLUTION_FACTOR; // JMS_GFX
		r.extent.height = 1; // JMS_GFX
		while (FuelVolume)
		{
			COUNT m;

			// JMS_GFX
			COUNT slotNr = 0;
			DWORD compartmentNr = 0;
			BYTE moduleType;
			DWORD fuelAmount;
			DWORD volume;
			
			// JMS_GFX
			fuelAmount = GLOBAL_SIS (FuelOnBoard);
			if (fuelAmount >= FUEL_RESERVE)
			{
				COUNT slotI;
				DWORD capacity = FUEL_RESERVE;
				
				slotI = NUM_MODULE_SLOTS;
				while (slotI--)
				{
					BYTE moduleType = GLOBAL_SIS (ModuleSlots[slotI]);
					
					capacity += GetModuleFuelCapacity (moduleType);
					
					//log_add (log_Debug, "fuelAmount %d, capacity %d, moduletype %d, slotI %d", fuelAmount, capacity, moduleType, slotI);
					
					if (fuelAmount < capacity)
					{
						slotNr = slotI;
						compartmentNr = capacity - fuelAmount;
						break;
					}
				}
				
				moduleType = GLOBAL_SIS (ModuleSlots[slotNr]);
				volume = GetModuleFuelCapacity (moduleType);
			}

				
			GetFTankCapacity (&r.corner);
			//log_add(log_Debug, "volume on %u, hefueltankcapacity %u", volume, HEFUEL_TANK_CAPACITY);
			r.corner.y -= volume == HEFUEL_TANK_CAPACITY ? IF_HD(19) : IF_HD(28); // JMS_GFX
			r.corner.x += volume == HEFUEL_TANK_CAPACITY ? IF_HD(2) : IF_HD(1); // JMS_GFX
			DrawPoint (&r.corner);
			r.corner.x += r.extent.width + 1;
			DrawPoint (&r.corner);
			r.corner.x -= r.extent.width;
			SetContextForeGroundColor (
					SetContextBackGroundColor (BLACK_COLOR));
			DrawFilledRectangle (&r);
			m = FuelVolume < FUEL_VOLUME_PER_ROW ?
					(COUNT)FuelVolume : FUEL_VOLUME_PER_ROW;
			GLOBAL_SIS (FuelOnBoard) += m;
			FuelVolume -= m;
		}
	}

	DestroyDrawable (ReleaseDrawable (ModuleFrame));
}

BOOLEAN
DoShipyard (MENU_STATE *pMS)
{
	BOOLEAN select, cancel;
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		goto ExitShipyard;

	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];

	OutfitOrShipyard = 3;

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	if (!pMS->Initialized)
	{
		pMS->InputFunc = DoShipyard;

		{
			STAMP s;
			RECT r, old_r;

			pMS->ModuleFrame = CaptureDrawable (
					LoadGraphic (SHIPYARD_PMAP_ANIM));

			pMS->CurString = CaptureColorMap (
					LoadColorMap (HANGAR_COLOR_TAB));

			pMS->hMusic = LoadMusic (SHIPYARD_MUSIC);

			SetTransitionSource (NULL);
			BatchGraphics ();
			DrawSISFrame ();
			DrawSISMessage (GAME_STRING (STARBASE_STRING_BASE + 3));
			DrawSISTitle (GAME_STRING (STARBASE_STRING_BASE));
			SetContext (SpaceContext);
			DrawBluePrint (pMS);

			pMS->CurState = SHIPYARD_CREW;
			DrawMenuStateStrings (PM_CREW, pMS->CurState);

			SetContext (SpaceContext);
			s.origin.x = 0;
			s.origin.y = 0;
			s.frame = SetAbsFrameIndex (pMS->ModuleFrame, 0);
#ifdef USE_3DO_HANGAR
			DrawStamp (&s);
#else // PC hangar
			// the PC ship dock needs to overwrite the border
			// expand the clipping rect by 1 pixel
			GetContextClipRect (&old_r);
			r = old_r;
			r.corner.x--;
			r.extent.width += 2;
			r.extent.height += 1;
			SetContextClipRect (&r);
			DrawStamp (&s);
			SetContextClipRect (&old_r);
			animatePowerLines (pMS);
#endif // USE_3DO_HANGAR
			
			SetContextFont (TinyFont);

			ScreenTransition (3, NULL);
			UnbatchGraphics ();

			PlayMusic (pMS->hMusic, TRUE, 1);

			ShowCombatShip (pMS, (COUNT)~0, NULL);

			SetInputCallback (on_input_frame);

			SetFlashRect (SFR_MENU_3DO);
		}

		pMS->Initialized = TRUE;
	}
	else if (cancel || (select && pMS->CurState == SHIPYARD_EXIT))
	{
ExitShipyard:
		SetInputCallback (NULL);

		DestroyDrawable (ReleaseDrawable (pMS->ModuleFrame));
		pMS->ModuleFrame = 0;
		DestroyColorMap (ReleaseColorMap (pMS->CurString));
		pMS->CurString = 0;

		return FALSE;
	}
	else if (select)
	{
		if (pMS->CurState != SHIPYARD_SAVELOAD)
		{
			pMS->Initialized = FALSE;
			DoModifyShips (pMS);
		}
		else
		{
			// Clearing FlashRect is not necessary
			if (!GameOptions ())
				goto ExitShipyard;
			DrawMenuStateStrings (PM_CREW, pMS->CurState);
			SetFlashRect (SFR_MENU_3DO);
		}
	}
	else
	{
		DoMenuChooser (pMS, PM_CREW);
	}

	if(optInfiniteRU)		
		GLOBAL_SIS (ResUnits) = 1000000L;

	return TRUE;
}

