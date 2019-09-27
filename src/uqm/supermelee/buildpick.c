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

static FRAME BuildPickFrame;

void
BuildBuildPickFrame (void)
{
	STAMP	s;
	RECT    r;
	COUNT   i;
	CONTEXT OldContext = SetContext (OffScreenContext);
	
	// create team building ship selection box
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = SetAbsFrameIndex (MeleeFrame, 27);
			// 5x5 grid of ships to pick from
	GetFrameRect (s.frame, &r);

	BuildPickFrame = CaptureDrawable (CreateDrawable (
			WANT_PIXMAP, r.extent.width, r.extent.height, 1));
	SetContextFGFrame (BuildPickFrame);
	SetFrameHot (s.frame, MAKE_HOT_SPOT (0, 0));
	DrawStamp (&s);

	for (i = 0; i < NUM_PICK_COLS * NUM_PICK_ROWS; ++i)
		DrawPickIcon (i, true);

	SetContext (OldContext);
}

void
DestroyBuildPickFrame (void)
{
	DestroyDrawable (ReleaseDrawable (BuildPickFrame));
	BuildPickFrame = 0;
}

// Draw a ship icon in the ship selection popup.
void
DrawPickIcon (MeleeShip ship, bool DrawErase)
{
	STAMP s;
	RECT r;

	GetFrameRect (BuildPickFrame, &r);

	s.origin.x = r.corner.x + RES_SCALE(20) + (ship % NUM_PICK_COLS) * RES_SCALE(18) - IF_HD(2);
	s.origin.y = r.corner.y +  RES_SCALE(5) + (ship / NUM_PICK_COLS) * RES_SCALE(18) - IF_HD(2);

	s.frame = GetShipIconsFromIndex (ship);
	if (DrawErase)
	{	// draw icon
		DrawStamp (&s);
	}
	else
	{	// erase icon
		Color OldColor;

		OldColor = SetContextForeGroundColor (BLACK_COLOR);
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);
	}
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
	DrawMeleeShipStrings (pMS, pMS->currentShip);
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

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		pMS->buildPickConfirmed = false;
		return FALSE;
	}

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	if (PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		// Confirm selection or cancel.
		pMS->buildPickConfirmed = !PulsedInputState.menu[KEY_MENU_CANCEL];
		return FALSE;
	}
	
	if (PulsedInputState.menu[KEY_MENU_SPECIAL]
			&& (pMS->currentShip != MELEE_NONE))
	{
		// Show ship spin video.
		DoShipSpin (pMS->currentShip, (MUSIC_REF) 0);
		return TRUE;
	}

	{
		MeleeShip newSelectedShip;

		newSelectedShip = pMS->currentShip;

		if (PulsedInputState.menu[KEY_MENU_LEFT])
		{
			if (newSelectedShip % NUM_PICK_COLS == 0)
				newSelectedShip += NUM_PICK_COLS;
			--newSelectedShip;
		}
		else if (PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			++newSelectedShip;
			if (newSelectedShip % NUM_PICK_COLS == 0)
				newSelectedShip -= NUM_PICK_COLS;
		}
		
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (newSelectedShip >= NUM_PICK_COLS)
				newSelectedShip -= NUM_PICK_COLS;
			else
				newSelectedShip += NUM_PICK_COLS * (NUM_PICK_ROWS - 1);
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (newSelectedShip < NUM_PICK_COLS * (NUM_PICK_ROWS - 1))
				newSelectedShip += NUM_PICK_COLS;
			else
				newSelectedShip -= NUM_PICK_COLS * (NUM_PICK_ROWS - 1);
		}

		if (newSelectedShip != pMS->currentShip)
		{
			// A new ship has been selected.
			DrawPickIcon (pMS->currentShip, true);
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

