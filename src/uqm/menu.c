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

#include "menustat.h"

#include "colors.h"
#include "controls.h"
#include "units.h"
#include "options.h"
#include "setup.h"
#include "gamestr.h"
#include "libs/graphics/gfx_common.h"
#include "libs/tasklib.h"
#include "libs/log.h"

static BYTE GetEndMenuState (BYTE BaseState);
static BYTE GetBeginMenuState (BYTE BaseState);
static BYTE FixMenuState (BYTE BadState);
static BYTE NextMenuState (BYTE BaseState, BYTE CurState);
static BYTE PreviousMenuState (BYTE BaseState, BYTE CurState);
static BOOLEAN GetAlternateMenu (BYTE *BaseState, BYTE *CurState);
static BYTE ConvertAlternateMenu (BYTE BaseState, BYTE NewState);


/* Draw the blue background for PC Menu Text, with a border around it.
 * The specified rectangle includes the border. */
static void
DrawPCMenuFrame (RECT *r)
{
	// Draw the top and left of the outer border.
	// This actually draws a rectangle, but the bottom and right parts
	// are drawn over in the next step.
	SetContextForeGroundColor (PCMENU_TOP_LEFT_BORDER_COLOR);
	DrawRectangle (r);

	// Draw the right and bottom of the outer border.
	// This actually draws a rectangle, with the top and left segments just
	// within the total area, but these segments are drawn over in the next
	// step.
	r->corner.x++;
	r->corner.y++;
	r->extent.height--;
	r->extent.width--;
	SetContextForeGroundColor (PCMENU_BOTTOM_RIGHT_BORDER_COLOR);
	DrawRectangle (r);

	// Draw the background.
	r->extent.height--;
	r->extent.width--;
	SetContextForeGroundColor (PCMENU_BACKGROUND_COLOR);
	DrawFilledRectangle (r);
}

#define ALT_MANIFEST 0x80
#define ALT_EXIT_MANIFEST 0x81

static UNICODE pm_crew_str[128];
static UNICODE pm_fuel_str[128];

/* Actually display the menu text */
static void
DrawPCMenu (BYTE beg_index, BYTE end_index, BYTE NewState, BYTE hilite, RECT *r)
{
#define PC_MENU_HEIGHT (RES_STAT_SCALE(8)) // JMS_GFX
	BYTE pos;
	COUNT i;
	int num_items;
	FONT OldFont;
	TEXT t;
	UNICODE buf[256];

	pos = beg_index + NewState;
	num_items = 1 + end_index - beg_index;
	r->corner.x -= 1;
	r->extent.width += 1;
	DrawFilledRectangle (r);
	if (num_items * PC_MENU_HEIGHT > r->extent.height)
		log_add (log_Error, "Warning, no room for all menu items!");
	else
		r->corner.y += (r->extent.height - num_items * PC_MENU_HEIGHT) / 2;
	r->extent.height = num_items * PC_MENU_HEIGHT + 4;
	DrawPCMenuFrame (r);	
	DrawBorder(18, FALSE);
	OldFont = SetContextFont (StarConFont);
	t.align = ALIGN_LEFT;
	t.baseline.x = r->corner.x + 2;
	t.baseline.y = r->corner.y + PC_MENU_HEIGHT - RES_SCALE(1);// - RESOLUTION_FACTOR; // JMS_GFX
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	r->corner.x++;
	r->extent.width -= 2;
	for (i = beg_index; i <= end_index; i++)
	{
		utf8StringCopy (buf, sizeof buf,
						(i == PM_FUEL) ? pm_fuel_str :
						(i == PM_CREW) ? pm_crew_str :
						GAME_STRING (MAINMENU_STRING_BASE + i));
		if (hilite && pos == i)
		{
			// Currently selected menu option.
			
			// Draw the background of the selection.
			SetContextForeGroundColor ((optCustomBorder ? SHADOWBOX_MEDIUM_COLOR : PCMENU_SELECTION_BACKGROUND_COLOR));
			r->corner.y = t.baseline.y - PC_MENU_HEIGHT + RES_STAT_SCALE(2); // + RESOLUTION_FACTOR; // JMS_GFX
			r->extent.height = PC_MENU_HEIGHT - 1;
			DrawFilledRectangle (r);

			// Draw the text of the selected item.
			SetContextForeGroundColor ((optCustomBorder ? NORMAL_ILLUMINATED_COLOR : PCMENU_SELECTION_TEXT_COLOR));
			font_DrawText (&t);
		}
		else
		{
			// Draw the text of an unselected item.
			SetContextForeGroundColor ((optCustomBorder ? SHADOWBOX_BACKGROUND_COLOR : PCMENU_TEXT_COLOR));
			font_DrawText (&t);
		}
		t.baseline.y += PC_MENU_HEIGHT;
	}
	SetContextFont (OldFont);
}

/* Determine the last text item to display */
static BYTE
GetEndMenuState (BYTE BaseState)
{
	switch (BaseState)
	{
		case PM_SCAN:
		case PM_STARMAP:
			return PM_NAVIGATE;
			break;
		case PM_MIN_SCAN:
			return PM_LAUNCH_LANDER;
			break;
		case PM_SAVE_GAME:
			return PM_EXIT_GAME_MENU;
			break;
		case PM_CONVERSE:
			return PM_ENCOUNTER_GAME_MENU;
			break;
		case PM_FUEL:
			return PM_EXIT_OUTFIT;
			break;
		case PM_CREW:
			return PM_EXIT_SHIPYARD;
			break;
		case PM_SOUND_ON:
			return PM_EXIT_SETTINGS;
			break;
		case PM_ALT_SCAN:
		case PM_ALT_STARMAP:
			return PM_ALT_NAVIGATE;
			break;
		case PM_ALT_CARGO:
			return PM_ALT_EXIT_MANIFEST;
			break;
		case PM_ALT_MSCAN:
			return PM_ALT_EXIT_SCAN;
			break;
	}
	return BaseState;
}

static BYTE
GetBeginMenuState (BYTE BaseState)
{
	return BaseState;
}

/* Correct Menu State for cases where the Menu shouldn't move */
static BYTE
FixMenuState (BYTE BadState)
{
	switch (BadState)
	{
		case PM_SOUND_ON:
			if (GLOBAL (glob_flags) & SOUND_DISABLED)
				return PM_SOUND_OFF;
			else
				return PM_SOUND_ON;
		case PM_MUSIC_ON:
			if (GLOBAL (glob_flags) & MUSIC_DISABLED)
				return PM_MUSIC_OFF;
			else
				return PM_MUSIC_ON;
		case PM_CYBORG_OFF:
			return (PM_CYBORG_OFF +
				((BYTE)(GLOBAL (glob_flags) & COMBAT_SPEED_MASK) >>
				COMBAT_SPEED_SHIFT));
	}
	return BadState;
}

/* Choose the next menu to hilight in the 'forward' direction */ 
static BYTE
NextMenuState (BYTE BaseState, BYTE CurState)
{
	BYTE NextState;
	BYTE AdjBase = BaseState;

	if (BaseState == PM_STARMAP)
		AdjBase--;

	switch (AdjBase + CurState)
	{
		case PM_SOUND_ON:
		case PM_SOUND_OFF:
			NextState = PM_MUSIC_ON;
			break;
		case PM_MUSIC_ON:
		case PM_MUSIC_OFF:
			NextState = PM_CYBORG_OFF;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			NextState = PM_CHANGE_CAPTAIN;
			break;
		default:
			NextState = AdjBase + CurState + 1;
	}	
	if (NextState > GetEndMenuState (BaseState))
		NextState = GetBeginMenuState (BaseState);
	return (FixMenuState (NextState) - AdjBase);
}

/* Choose the next menu to hilight in the 'back' direction */ 
BYTE
PreviousMenuState (BYTE BaseState, BYTE CurState)
{
	SWORD NextState;
	BYTE AdjBase = BaseState;

	if (BaseState == PM_STARMAP)
		AdjBase--;

	switch (AdjBase + CurState)
	{
		case PM_SOUND_OFF:
			NextState = PM_EXIT_SETTINGS;
			break;
		case PM_MUSIC_ON:
		case PM_MUSIC_OFF:
			NextState = PM_SOUND_ON;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			NextState = PM_MUSIC_ON;
			break;
		case PM_CHANGE_CAPTAIN:
			NextState = PM_CYBORG_OFF;
			break;
		default:
			NextState = AdjBase + CurState - 1;
	}	
	if (NextState < GetBeginMenuState (BaseState))
		NextState = GetEndMenuState (BaseState);
	return (FixMenuState ((BYTE)NextState) - AdjBase);
}


/* When using PC hierarchy, convert 3do->PC */
static BOOLEAN
GetAlternateMenu (BYTE *BaseState, BYTE *CurState)
{
	BYTE AdjBase = *BaseState;
	BYTE adj = 0;
	if (*BaseState == PM_STARMAP)
	{
		AdjBase--;
		adj = 1;
	}
	if (*CurState & 0x80)
	{
		switch (*CurState)
		{
			case ALT_MANIFEST:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_MANIFEST - PM_ALT_SCAN - adj;
				return TRUE;
			case ALT_EXIT_MANIFEST:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_EXIT_MANIFEST - PM_ALT_CARGO;
				return TRUE;
		}
		log_add (log_Error, "Unknown state combination: %d, %d",
				*BaseState, *CurState);
		return FALSE;
	}
	else
	{
		switch (AdjBase + *CurState)
		{
			case PM_SCAN:
				*BaseState = PM_ALT_SCAN;
				*CurState = PM_ALT_SCAN - PM_ALT_SCAN;
				return TRUE;
			case PM_STARMAP:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_STARMAP - PM_ALT_SCAN - adj;
				return TRUE;
			case PM_DEVICES:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_DEVICES - PM_ALT_CARGO;
				return TRUE;
			case PM_CARGO:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_CARGO - PM_ALT_CARGO;
				return TRUE;
			case PM_ROSTER:
				*BaseState = PM_ALT_CARGO;
				*CurState = PM_ALT_ROSTER - PM_ALT_CARGO;
				return TRUE;
			case PM_GAME_MENU:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_GAME_MENU - PM_ALT_SCAN - adj;
				return TRUE;
			case PM_NAVIGATE:
				*BaseState = PM_ALT_SCAN + adj;
				*CurState = PM_ALT_NAVIGATE - PM_ALT_SCAN - adj;
				return TRUE;
			case PM_MIN_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_MSCAN - PM_ALT_MSCAN;
				return TRUE;
			case PM_ENE_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_ESCAN - PM_ALT_MSCAN;
				return TRUE;
			case PM_BIO_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_BSCAN - PM_ALT_MSCAN;
				return TRUE;
			case PM_EXIT_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_EXIT_SCAN - PM_ALT_MSCAN;
				return TRUE;
			case PM_AUTO_SCAN:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_ASCAN - PM_ALT_MSCAN;
				return TRUE;
			case PM_LAUNCH_LANDER:
				*BaseState = PM_ALT_MSCAN;
				*CurState = PM_ALT_DISPATCH - PM_ALT_MSCAN;
				return TRUE;
		}
		return FALSE;
	}
}

/* When using PC hierarchy, convert PC->3DO */
static BYTE
ConvertAlternateMenu (BYTE BaseState, BYTE NewState)
{
	switch (BaseState + NewState)
	{
		case PM_ALT_SCAN:
			return (PM_SCAN - PM_SCAN);
		case PM_ALT_STARMAP:
			return (PM_STARMAP - PM_SCAN);
		case PM_ALT_MANIFEST:
			return (ALT_MANIFEST);
		case PM_ALT_GAME_MENU:
			return (PM_GAME_MENU - PM_SCAN);
		case PM_ALT_NAVIGATE:
			return (PM_NAVIGATE - PM_SCAN);
		case PM_ALT_CARGO:
			return (PM_CARGO - PM_SCAN);
		case PM_ALT_DEVICES:
			return (PM_DEVICES - PM_SCAN);
		case PM_ALT_ROSTER:
			return (PM_ROSTER - PM_SCAN);
		case PM_ALT_EXIT_MANIFEST:
			return (ALT_EXIT_MANIFEST);
		case PM_ALT_MSCAN:
			return (PM_MIN_SCAN - PM_MIN_SCAN);
		case PM_ALT_ESCAN:
			return (PM_ENE_SCAN - PM_MIN_SCAN);
		case PM_ALT_BSCAN:
			return (PM_BIO_SCAN - PM_MIN_SCAN);
		case PM_ALT_ASCAN:
			return (PM_AUTO_SCAN - PM_MIN_SCAN);
		case PM_ALT_DISPATCH:
			return (PM_LAUNCH_LANDER - PM_MIN_SCAN);
		case PM_ALT_EXIT_SCAN:
			return (PM_EXIT_SCAN - PM_MIN_SCAN);
	}
	return (NewState);
}

BOOLEAN
DoMenuChooser (MENU_STATE *pMS, BYTE BaseState)
{
	BYTE NewState = pMS->CurState;
	BYTE OrigBase = BaseState;
	BOOLEAN useAltMenu = FALSE;
	if (optWhichMenu == OPT_PC)
		useAltMenu = GetAlternateMenu (&BaseState, &NewState);
	if (PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_UP])
		NewState = PreviousMenuState (BaseState, NewState);
	else if (PulsedInputState.menu[KEY_MENU_RIGHT] ||
			PulsedInputState.menu[KEY_MENU_DOWN])
		NewState = NextMenuState (BaseState, NewState);
	else if (useAltMenu && PulsedInputState.menu[KEY_MENU_SELECT])
	{
		NewState = ConvertAlternateMenu (BaseState, NewState);
		if (NewState == ALT_MANIFEST)
		{
			DrawMenuStateStrings (PM_ALT_CARGO, 0);
			pMS->CurState = PM_CARGO - PM_SCAN;
			return TRUE;
		}
		if (NewState == ALT_EXIT_MANIFEST)
		{
			if (OrigBase == PM_SCAN)
				DrawMenuStateStrings (PM_ALT_SCAN,
						PM_ALT_MANIFEST - PM_ALT_SCAN);
			else
				DrawMenuStateStrings (PM_ALT_STARMAP,
						PM_ALT_MANIFEST - PM_ALT_STARMAP);
			pMS->CurState = ALT_MANIFEST;
			return TRUE;
		}
		return FALSE;
	}
	else if ((optWhichMenu == OPT_PC) &&
			PulsedInputState.menu[KEY_MENU_CANCEL] && 
			(BaseState == PM_ALT_CARGO))
	{
		if (OrigBase == PM_SCAN)
			DrawMenuStateStrings (PM_ALT_SCAN,
					PM_ALT_MANIFEST - PM_ALT_SCAN);
		else
			DrawMenuStateStrings (PM_ALT_STARMAP,
					PM_ALT_MANIFEST - PM_ALT_STARMAP);
		pMS->CurState = ALT_MANIFEST;
		return TRUE;
	}
	else
		return FALSE;

	DrawMenuStateStrings (BaseState, NewState);
	if (useAltMenu)
		NewState = ConvertAlternateMenu (BaseState, NewState);
	pMS->CurState = NewState;
	SleepThread (ONE_SECOND / 20);
	return TRUE;
}

void
DrawMenuStateStrings (BYTE beg_index, SWORD NewState)
{
	BYTE end_index;
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	BYTE hilite = 1;
	extern FRAME PlayFrame;

	if (NewState < 0)
	{
		NewState = -NewState;
		hilite = 0;
	}

	if (optWhichMenu == OPT_PC)
	{
		BYTE tmpState = (BYTE)NewState;
		GetAlternateMenu (&beg_index, &tmpState);
		NewState = tmpState;
	}

	if (beg_index == PM_STARMAP)
		NewState--;
	end_index = GetEndMenuState (beg_index);

	s.frame = 0;
	if (NewState <= end_index - beg_index)
		s.frame = SetAbsFrameIndex (PlayFrame, beg_index + NewState);

	PreUpdateFlashRect ();
	OldContext = SetContext (StatusContext);
	GetContextClipRect (&r);
	s.origin.x = RADAR_X - r.corner.x;
	s.origin.y = RADAR_Y - r.corner.y;
	r.corner.x = s.origin.x - 1;
	r.corner.y = s.origin.y - RES_SCALE(11); // JMS_GFX
	r.extent.width = RADAR_WIDTH + 2;
	BatchGraphics ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x0A, 0x0A, 0x0A), 0x08));
	if (s.frame && optWhichMenu == OPT_PC)
	{
		if (beg_index == PM_CREW)
			snprintf (pm_crew_str, sizeof pm_crew_str, "%s(%d)",
					 GAME_STRING (MAINMENU_STRING_BASE + PM_CREW),
					 GLOBAL (CrewCost));
		if (beg_index == PM_FUEL)
			snprintf (pm_fuel_str, sizeof pm_fuel_str, "%s(%d)",
					 GAME_STRING (MAINMENU_STRING_BASE + PM_FUEL),
					 GLOBAL (FuelCost));
		if (beg_index == PM_SOUND_ON)
		{
			end_index = beg_index + 5;
			switch (beg_index + NewState)
			{
				case PM_SOUND_ON:
				case PM_SOUND_OFF:
					NewState = 0;
					break;
				case PM_MUSIC_ON:
				case PM_MUSIC_OFF:
					NewState = 1;
					break;
				case PM_CYBORG_OFF:
				case PM_CYBORG_NORMAL:
				case PM_CYBORG_DOUBLE:
				case PM_CYBORG_SUPER:
					NewState = 2;
					break;
				case PM_CHANGE_CAPTAIN:
					NewState = 3;
					break;
				case PM_CHANGE_SHIP:
					NewState = 4;
					break;
				case PM_EXIT_SETTINGS:
					NewState = 5;
					break;
			}
		}
		r.extent.height = RADAR_HEIGHT + RES_SCALE(11); // JMS_GFX

		DrawPCMenu (beg_index, end_index, (BYTE)NewState, hilite, &r);
		s.frame = 0;
	}
	else
	{
		if (optWhichMenu == OPT_PC)
		{
			r.corner.x -= 1;
			r.extent.width += 1;
			r.extent.height = RADAR_HEIGHT + RES_SCALE(11); // JMS_GFX
		}
		else
			r.extent.height = RES_SCALE(11);
		DrawFilledRectangle (&r);
		DrawBorder(8, FALSE);
	}
	if (s.frame)
	{
		DrawStamp (&s);
		switch (beg_index + NewState)
		{
			TEXT t;
			UNICODE buf[20];

			case PM_CREW:
				t.baseline.x = s.origin.x + RADAR_WIDTH - 2;
				t.baseline.y = s.origin.y + RADAR_HEIGHT - 2;
				t.align = ALIGN_RIGHT;
				t.CharCount = (COUNT)~0;
				t.pStr = buf;
				snprintf (buf, sizeof buf, "%u", GLOBAL (CrewCost));
				SetContextFont (TinyFont);
				SetContextForeGroundColor (THREEDOMENU_TEXT_COLOR);
				font_DrawText (&t);
				break;
			case PM_FUEL:
				t.baseline.x = s.origin.x + RADAR_WIDTH - 2;
				t.baseline.y = s.origin.y + RADAR_HEIGHT - 2;
				t.align = ALIGN_RIGHT;
				t.CharCount = (COUNT)~0;
				t.pStr = buf;
				snprintf (buf, sizeof buf, "%u", GLOBAL (FuelCost));
				SetContextFont (TinyFont);
				SetContextForeGroundColor (THREEDOMENU_TEXT_COLOR);
				font_DrawText (&t);
				break;
		}
	}
	UnbatchGraphics ();
	SetContext (OldContext);
	PostUpdateFlashRect ();
}

void
DrawSubmenu (BYTE Visible)
{
	STAMP s;
	CONTEXT OldContext;
	
	OldContext = SetContext (ScreenContext);

	s.origin.x = 0;
	s.origin.y = 0;

	s.frame = SetAbsFrameIndex (SubmenuFrame, Visible);

	DrawStamp (&s);
	
	SetContext (OldContext);
}

void
DrawBorder (BYTE Visible, BOOLEAN InBattle)
{
	STAMP s;
	CONTEXT OldContext;

	if (!InBattle)
		OldContext = SetContext (ScreenContext);

	s.origin.x = 0;
	s.origin.y = 0;

	s.frame = SetAbsFrameIndex (BorderFrame, Visible);

	if (optCustomBorder || InBattle)
		DrawStamp (&s);
	
	if (!InBattle)
		SetContext (OldContext);
}