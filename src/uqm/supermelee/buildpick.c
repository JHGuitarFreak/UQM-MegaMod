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

#include "buildpick.h"

#include "../controls.h"
#include "../colors.h"
#include "../fmv.h"
#include "../master.h"
#include "../setup.h"
#include "../sounds.h"
#include "libs/gfxlib.h"
#include "../gamestr.h"
#include "libs/inplib.h"

static FRAME BuildPickFrame;

#define NUM_PICK_SLOTS (NUM_PICK_COLS * NUM_PICK_ROWS)
#define NUM_PICK_PAGES_ACTUAL \
		((NUM_MELEE_SHIPS + NUM_PICK_SLOTS - 1) / NUM_PICK_SLOTS)
#define NUM_PICK_PAGES NUM_PICK_PAGES_ACTUAL

static RECT BuildPickRects[NUM_PICK_SLOTS];

#define PICK_PAGE(a) (a / NUM_PICK_SLOTS)
#define PICK_PAGE_START(a) (a * NUM_PICK_SLOTS)
#define PICK_SLOT(a) (a % NUM_PICK_SLOTS)

static inline COUNT
PickPageStop (COUNT page)
{
	COUNT stop = PICK_PAGE_START (page) + NUM_PICK_SLOTS;

	return (stop > NUM_MELEE_SHIPS) ? NUM_MELEE_SHIPS : stop;
}

static MeleeShip
PickStep (MeleeShip ship, int dx, int dy, int override)
{
	COUNT page = PICK_PAGE (ship);
	COUNT slot = PICK_SLOT (ship);
	COUNT row = slot / NUM_PICK_COLS;
	COUNT col = slot % NUM_PICK_COLS;
	COUNT next;

	if (override != 0)
	{
		if (override == 1)
			page = 0;
		else if (override == 2)
			page = 1;

		slot = 0;
		col = 0;
		row = 0;
	}

	if (dx)
	{	// Horizontal wrap
		col += dx;

		if (col < 0 || col >= NUM_PICK_COLS)
		{
			COUNT page_var, col_var;

			if (dx > 0)
			{
				page_var = 1;
				col_var = 0;
			}
			else
			{
				page_var = NUM_PICK_PAGES - 1;
				col_var = NUM_PICK_COLS - 1;
			}

			page = (page + page_var) % NUM_PICK_PAGES;
			col = col_var;
		}
		else
		{
			COUNT next_slot = PICK_PAGE_START (page)
					+ (slot / NUM_PICK_COLS) * NUM_PICK_COLS + col;

			if (next_slot >= NUM_MELEE_SHIPS)
			{
				page = (page + 1) % NUM_PICK_PAGES;
				col = 0;
			}
		}
	}
	else
	{	// Vertical wrap
		row = (row + dy + NUM_PICK_ROWS) % NUM_PICK_ROWS;
	}

	next = PICK_PAGE_START (page) + row * NUM_PICK_COLS + col;

	if (next >= NUM_MELEE_SHIPS)
		next = NUM_MELEE_SHIPS - 1;

	return (MeleeShip)next;
}

static void
PickSlotOrigin (COUNT slot, POINT *pt)
{
	pt->x = RES_SCALE (20) + (slot % NUM_PICK_COLS) * RES_SCALE (18);
	pt->y = RES_SCALE (5) + (slot / NUM_PICK_COLS) * RES_SCALE (18);
}

static void
DrawPickPageIndicator (COUNT page)
{
	RECT r, pick_r;
	Color oldColor;
	COORD pick_center;
	COORD indicator_width = RES_SCALE (16);
	COORD indicator_space = RES_SCALE (8);

	oldColor = SetContextForeGroundColor (BUILD_SHADE_RGBA (87));

	GetFrameRect (BuildPickFrame, &pick_r);

	pick_center = pick_r.corner.x + (pick_r.extent.width >> 1);

	r.extent.width = (indicator_width << 1) + indicator_space;
	r.extent.height = RES_SCALE (2);
	r.corner.x = pick_center - (r.extent.width >> 1);
	r.corner.y = pick_r.corner.y + pick_r.extent.height - RES_SCALE (3);
	DrawFilledRectangle (&r);

	if (page == 0)
		SetContextForeGroundColor (WIDGET_ACTIVE_COLOR);
	else
		SetContextForeGroundColor (WIDGET_INACTIVE_COLOR);
	r.extent.width = indicator_width;
	DrawFilledRectangle (&r);

	if (page == 1)
		SetContextForeGroundColor (WIDGET_ACTIVE_COLOR);
	else
		SetContextForeGroundColor (WIDGET_INACTIVE_COLOR);
	r.corner.x += r.extent.width + indicator_space;
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (oldColor);
}

static void
DrawPickPanel (void)
{
	STAMP s;

	s.frame = SetAbsFrameIndex (BuildPickFrame, 0);
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);
}

static void
DrawPickPage (MeleeShip ship)
{
	COUNT i;
	COUNT page = PICK_PAGE (ship);

	for (i = PICK_PAGE_START (page); i < PickPageStop (page); ++i)
		DrawPickIcon ((MeleeShip)i, true);

	DrawPickPageIndicator (page);
}

void
BuildBuildPickFrame (void)
{
	STAMP s;
	RECT  r;
	CONTEXT OldContext = SetContext (OffScreenContext);
	
	// create team building ship selection box
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex (MeleeFrame, SHIP_PICKER);
			// 5x5 grid of ships to pick from
	GetFrameRect (s.frame, &r);

	BuildPickFrame = CaptureDrawable (CreateDrawable (
			WANT_PIXMAP, r.extent.width, r.extent.height, 1));
	SetContextFGFrame (BuildPickFrame);
	SetFrameHot (s.frame, MAKE_HOT_SPOT (0, 0));
	DrawStamp (&s);
	DrawShipPickerText (s);

	SetContext (OldContext);
}

void
DestroyBuildPickFrame (void)
{
	DestroyDrawable (ReleaseDrawable (BuildPickFrame));
	BuildPickFrame = 0;
}

Color
GetShipFlashColor (void)
{
	if (is3DO (optWhichMenu))
	{
		return BLACK_COLOR;
	}
	else
	{
		static BYTE cycle_index = 0;

		static const Color cycle_tab[] = SHIP_SELECT_COLOR_CYCLE_TABLE;
		const size_t cycleCount = ARRAY_SIZE (cycle_tab);

		cycle_index = (cycle_index + 1) % cycleCount;

		return cycle_tab[cycle_index];
	}
}

// Draw part of the frame underneath the ship (removes artifacts in HD)
void
RepairBuildPickFrame (RECT *pRect, POINT *origin)
{
	RECT r;
	CONTEXT OldContext;
	RECT OldRect;
	POINT oldOrigin;
	STAMP s;

	r.corner.x = origin->x;
	r.corner.y = origin->y;
	r.extent.width = r.extent.height = RES_SCALE (16);

	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&OldRect);
	SetContextClipRect (&r);
	if (IS_PAD)
	{
		oldOrigin = SetContextOrigin (MAKE_POINT (r.corner.x, r.corner.y));
	}
	else
	{
		oldOrigin =
				SetContextOrigin (MAKE_POINT (-r.corner.x, -r.corner.y));
	}

	s.origin.x = pRect->corner.x;
	s.origin.y = pRect->corner.y;
	s.frame = SetAbsFrameIndex (MeleeFrame, SHIP_PICKER);

	DrawStamp (&s);

	SetContextOrigin (oldOrigin);
	SetContextClipRect (&OldRect);
	SetContext (OldContext);
}

// Draw a ship icon in the ship selection popup.
void
DrawPickIcon (MeleeShip ship, bool DrawErase)
{
	STAMP s;
	RECT r;
	POINT pt;

	GetFrameRect (BuildPickFrame, &r);

	PickSlotOrigin (PICK_SLOT (ship), &pt);
	s.origin.x = r.corner.x + pt.x;
	s.origin.y = r.corner.y + pt.y;

	s.frame = GetShipIconsFromIndex (ship);

	// Draw a rectangle below ship to remove artifacts in HD
	if (IS_HD)
		RepairBuildPickFrame (&r, &s.origin);

	if (DrawErase)
	{	// draw icon
		DrawStamp (&s);
	}
	else
	{	// erase icon
		Color OldColor;

		OldColor = SetContextForeGroundColor (GetShipFlashColor ());
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);
	}
}

static void
DrawTooltipBox (void)
{
	STAMP s;

	s.origin.x = s.origin.y = 0;
	s.frame = SetAbsFrameIndex (MeleeFrame, SHIP_DESC);

	DrawStamp (&s);
}

void
GetToolTipFrameRect (RECT *r)
{
	GetFrameRect (SetAbsFrameIndex (MeleeFrame, SHIP_DESC), r);
}

#define RACE_NAME_OFFSET 0
#define RACE_SHIP_OFFSET 3

void
DrawTooltip (SHIP_INFO *SIPtr)
{
	UNICODE *ptr;
	UNICODE buf[PATH_MAX];
	TEXT Text;
	CONTEXT oldContext;
	FONT oldFont;
	Color oldColor;
	RECT r;
	UNICODE delim[] = "\n";

	GetToolTipFrameRect (&r);
	
	sprintf (buf, "%s %s",
			GET_STRING (SIPtr->race_strings, RACE_NAME_OFFSET),
			GET_STRING (SIPtr->race_strings, RACE_SHIP_OFFSET));

	Text.pStr = buf;
	Text.CharCount = (COUNT)utf8StringCount (buf);
	Text.align = ALIGN_CENTER;
	Text.baseline.y = r.corner.y + RES_SCALE (8) + RES_SCALE (1);
	Text.baseline.x = r.corner.x + (r.extent.width >> 1) + RES_SCALE (1);

	oldContext = SetContext (SpaceContext);
	DrawTooltipBox ();

	oldFont = SetContextFont (StarConFont);
	oldColor = SetContextForeGroundColor (TOOLTIP_COLOR_NAME_BACK);
	font_DrawText (&Text);

	SetContextForeGroundColor (TOOLTIP_COLOR_NAME_FRONT);
	Text.baseline.x -= RES_SCALE (1);
	Text.baseline.y -= RES_SCALE (1);
	font_DrawText (&Text);

	SetContextForeGroundColor (TOOLTIP_COLOR_DESC_FRONT);

	utf8StringCopy (buf, sizeof buf,
			GET_STRING (SIPtr->race_strings,
				GetStringTableCount (SIPtr->race_strings) - 1));
	ptr = strtok (buf, delim);

	Text.baseline.y += RES_SCALE (2);

	while (ptr != NULL)
	{
		Text.pStr = ptr;
		Text.CharCount = (COUNT)utf8StringCount (ptr);
		Text.baseline.y += RES_SCALE (9);
		font_DrawText (&Text);
		ptr = strtok (NULL, delim);
	}

	SetContextForeGroundColor (oldColor);
	SetContextFont (oldFont);
	SetContext (oldContext);
}

void
DrawPickFrame (MELEE_STATE *pMS)
{
	RECT r, r0, r1, ship_r;
	STAMP s;

	GetShipBox (&r0, 0, 0, 0),
	GetShipBox (&r1, 1, NUM_MELEE_ROWS - 1, NUM_MELEE_COLUMNS - 1),
	BoxUnion (&r0, &r1, &ship_r);

	s.frame = SetAbsFrameIndex (BuildPickFrame, 0);
	GetFrameRect (s.frame, &r);
	r.corner.x = -(ship_r.corner.x
			+ ((ship_r.extent.width - r.extent.width) >> 1));
	if (pMS->side)
		r.corner.y = -ship_r.corner.y;
	else
		r.corner.y = -(ship_r.corner.y
				+ (ship_r.extent.height - r.extent.height));
	SetFrameHot (s.frame, MAKE_HOT_SPOT (r.corner.x, r.corner.y));
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);
	DrawPickPage (pMS->currentShip);
	DrawMeleeShipStrings (pMS, pMS->currentShip);

	if (optMouseInput)
	{
		BYTE i;
		for (i = 0; i < NUM_PICK_SLOTS; ++i)
		{
			POINT pt;
			PickSlotOrigin (i, &pt);
			BuildPickRects[i].corner.x = -r.corner.x + pt.x;
			BuildPickRects[i].corner.y = -r.corner.y + pt.y;
			BuildPickRects[i].extent.width = RES_SCALE (16);
			BuildPickRects[i].extent.height = RES_SCALE (16);
		}
	}

	if (isPC (optWhichMenu))
	{	// if PC menu is selected - draw flash on the current ship as soon
		// as we pop up otherwise for 1 frame it will look like nothing is
		// selected
		DrawPickIcon (pMS->currentShip, false);
	}
}

void
GetBuildPickFrameRect (RECT *r)
{
	GetFrameRect (BuildPickFrame, r);
}

static BOOLEAN
DoPickShip (MELEE_STATE *pMS)
{
	DWORD TimeIn = GetTimeCounter ();
	BOOLEAN select, cancel, special, pgup, pgdn;

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->buildPickConfirmed = false;
		return FALSE;
	}

	select = PulsedInputState.menu[KEY_MENU_SELECT] ||
			CtxMouseClicker (BuildPickRects[PICK_SLOT (pMS->currentShip)]);
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL] ||
			PulsedInputState.menu[MOUSE_BTN_RIGHT];
	special = PulsedInputState.menu[KEY_MENU_SPECIAL] ||
			PulsedInputState.menu[MOUSE_BTN_MIDDLE];

	pgup = PulsedInputState.menu[KEY_MENU_PAGE_UP] ||
			PulsedInputState.menu[MOUSE_WHEEL_UP];
	pgdn = PulsedInputState.menu[KEY_MENU_PAGE_DOWN] ||
			PulsedInputState.menu[MOUSE_WHEEL_DOWN];

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (select || cancel)
	{
		// Confirm selection or cancel.
		pMS->buildPickConfirmed = !cancel;
		return FALSE;
	}
	
	if (special && (pMS->currentShip != MELEE_NONE))
	{
		if (isPC (optWhichIntro))
			PlayMenuSound (MENU_SOUND_SUCCESS);
		// Show ship spin video.
		DoShipSpin (pMS->currentShip, pMS->hMusic);

		return TRUE;
	}

	{
		MeleeShip newSelectedShip = pMS->currentShip;

		if (PulsedInputState.menu[KEY_MENU_LEFT])
			newSelectedShip = PickStep (newSelectedShip, -1, 0, 0);
		else if (PulsedInputState.menu[KEY_MENU_RIGHT])
			newSelectedShip = PickStep (newSelectedShip, +1, 0, 0);

		if (PulsedInputState.menu[KEY_MENU_UP])
			newSelectedShip = PickStep (newSelectedShip, 0, -1, 0);
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
			newSelectedShip = PickStep (newSelectedShip, 0, +1, 0);

		if (pgup)
			newSelectedShip = PickStep (newSelectedShip, 0, 0, 2);
		else if (pgdn)
			newSelectedShip = PickStep (newSelectedShip, 0, 0, 1);

		if (SetMouseContext (SpaceContext))
		{
			COUNT i;
			int cursor = CURSOR_POINTER;
			MeleeShip hovered_item = newSelectedShip;
			COUNT hoverPage = PICK_PAGE (newSelectedShip);

			for (i = PICK_PAGE_START (hoverPage); i < PickPageStop (hoverPage);
				++i)
			{
				if (MouseInRect (BuildPickRects[PICK_SLOT (i)]))
				{
					hovered_item = (MeleeShip)i;
					cursor = CURSOR_POINTER_HILITE;
					break;
				}
			}

			UQM_SetCursor (cursor);

			if (hovered_item != newSelectedShip)
			{
				newSelectedShip = hovered_item;
				PlayMenuSound (MENU_SOUND_MOVE);
			}
		}

		if (newSelectedShip != pMS->currentShip)
		{
			MeleeShip cur_page = PICK_PAGE (pMS->currentShip);
			MeleeShip pick_page = PICK_PAGE (newSelectedShip);

			// A new ship has been selected.
			if (pick_page == cur_page)
				DrawPickIcon (pMS->currentShip, true);
			else
			{
				DrawPickPanel ();
				DrawPickPage (newSelectedShip);
			}
			pMS->currentShip = newSelectedShip;
			DrawMeleeShipStrings (pMS, newSelectedShip);
		}
	}

	Melee_flashSelection (pMS);

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return TRUE;
}

// Returns true if a ship has been selected, or false if the operation has
// been cancelled or if the general abort key was pressed (in which case
// 'GLOBAL (CurrentActivity) & CHECK_ABORT' is true as usual.
// If a ship was selected, pMS->currentShip is set to the selected ship.
bool
BuildPickShip (MELEE_STATE *pMS)
{
	FlushInput ();

	if (pMS->currentShip == MELEE_NONE)
		pMS->currentShip = 0;

	DrawPickFrame (pMS);

	pMS->InputFunc = DoPickShip;
	DoInput (pMS, FALSE);

	return pMS->buildPickConfirmed;
}
