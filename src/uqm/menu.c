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
#include "util.h"
#include "planets/planets.h"
#include "shipcont.h"
#include "nameref.h"
#include "ifontres.h"

static BYTE GetEndMenuState (BYTE BaseState);
static BYTE GetBeginMenuState (BYTE BaseState);
static BYTE FixMenuState (BYTE BadState);
static BYTE NextMenuState (BYTE BaseState, BYTE CurState);
static BYTE PreviousMenuState (BYTE BaseState, BYTE CurState);
static BOOLEAN GetAlternateMenu (BYTE *BaseState, BYTE *CurState);
static BYTE ConvertAlternateMenu (BYTE BaseState, BYTE NewState);
void FunkyMenu (BYTE m, STAMP stmp);

/* Draw the blue background for PC Menu Text, with a border around it.
 * The specified rectangle includes the border. */
static void
DrawPCMenuFrame (RECT *r)
{

	if (IS_HD || optCustomBorder)
	{
		DrawRenderedBox (r, TRUE, PCMENU_BACKGROUND_COLOR,
				optCustomBorder ? SPECIAL_BEVEL : THIN_INNER_BEVEL);
	}
	else
	{
		DrawStarConBox (r, RES_SCALE (1), PCMENU_TOP_LEFT_BORDER_COLOR,
				PCMENU_BOTTOM_RIGHT_BORDER_COLOR, TRUE,
				PCMENU_BACKGROUND_COLOR, FALSE, NULL_COLOR);
	}

	// Draw the right and bottom of the outer border.
	// This actually draws a rectangle, with the top and left segments just
	// within the total area, but these segments are drawn over in the next
	// step.
	r->corner.x += RES_SCALE (1);
	r->corner.y += RES_SCALE (1);
	r->extent.height -= RES_SCALE (2);
	r->extent.width -= RES_SCALE (2);
}

#define ALT_MANIFEST 0x80
#define ALT_EXIT_MANIFEST 0x81

#define PC_MENU_HEIGHT (RES_SCALE (8))

static UNICODE pm_crew_str[128];
static UNICODE pm_fuel_str[128];

/* Actually display the menu text */
static void
DrawPCMenu (BYTE beg_index, BYTE end_index, BYTE NewState, BYTE hilite, RECT *r)
{
	BYTE pos;
	COUNT i, j;
	static COUNT rd = 0;
	int num_items;
	FONT OldFont;
	TEXT t;
	UNICODE buf[256];
	RECT rt;

	pos = beg_index + NewState;
	num_items = 1 + end_index - beg_index;
	r->extent.width -= RES_SCALE (1);

	// Gray rectangle behind PC menu
	rt = *r;
	rt.corner.x -= RES_SCALE (1);
	rt.corner.y += PC_MENU_HEIGHT - DOS_BOOL_SCL (12, 3);
	rt.extent.width += RES_SCALE (2);
	rt.extent.height += DOS_BOOL_SCL (2, -6);
	if (!optCustomBorder)
		DrawFilledRectangle (&rt);

	DrawBorder (SIS_RADAR_FRAME);

	if (num_items * PC_MENU_HEIGHT > r->extent.height)
		log_add (log_Error, "Warning, no room for all menu items!");
	else
		r->corner.y += (r->extent.height - (num_items * PC_MENU_HEIGHT)) / 2;
	r->extent.height = num_items * PC_MENU_HEIGHT + RES_SCALE (3);
	
	r->corner.y += DOS_NUM_SCL (1);
	r->corner.y -= SAFE_NUM_SCL (4);

	DrawPCMenuFrame (r);

	OldFont = SetContextFont (StarConFont);
	t.align = ALIGN_LEFT;
	t.baseline.x = r->corner.x + RES_SCALE (2);
	t.baseline.y = r->corner.y + PC_MENU_HEIGHT - RES_SCALE (1);
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	r->corner.x += RES_SCALE (1);
	r->extent.width -= RES_SCALE (2);
	for (i = beg_index; i <= end_index; i++)
	{
		// To compensate for the read speed menu entries
		j = i > PM_EXIT_SETTINGS ?
				(i - (PM_CHANGE_CAPTAIN - PM_READ_VERY_SLOW)) : i;

		utf8StringCopy (buf, sizeof buf,
						(i == PM_FUEL) ? pm_fuel_str :
						(i == PM_CREW) ? pm_crew_str :
						GAME_STRING (MAINMENU_STRING_BASE + j));
		if (hilite && pos == i)
		{
			// Currently selected menu option.
			if (pos != rd)
			{	// Draw the background of the selection.
				SetContextForeGroundColor (
						PCMENU_SELECTION_BACKGROUND_COLOR);
				r->corner.y = t.baseline.y - PC_MENU_HEIGHT
						+ RES_SCALE (2);
				r->extent.height = PC_MENU_HEIGHT - RES_SCALE (1);
				if (optCustomBorder)
				{
					STAMP s;

					s.origin = r->corner;

					s.frame = SetAbsFrameIndex (CustBevelFrame,
							DOS_MENU_HILITE);
					DrawStamp (&s);
				}
				else
					DrawFilledRectangle (r);

				rd = pos;
			}
			else
				rd = 0;

			// Draw the text of the selected item.
			SetContextForeGroundColor (PCMENU_SELECTION_TEXT_COLOR);
			font_DrawText (&t);
		}
		else
		{
			// Draw the text of an unselected item.
			SetContextForeGroundColor (PCMENU_TEXT_COLOR);
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
		case PM_READ_VERY_SLOW:
			return (PM_READ_VERY_SLOW +
				(BYTE)(GLOBAL (glob_flags) & READ_SPEED_MASK));
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
			if ((isPC (optSmoothScroll) && !usingSpeech) || isPC (optWhichMenu))
				NextState = PM_READ_VERY_SLOW;
			else
				NextState = PM_CHANGE_CAPTAIN;
			break;
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
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
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
			NextState = PM_CYBORG_OFF;
			break;
		case PM_CHANGE_CAPTAIN:
			if ((isPC (optSmoothScroll) && !usingSpeech) || isPC (optWhichMenu))
				NextState = PM_READ_VERY_SLOW;
			else
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

	return TRUE;
}

static UNICODE *
IndexToText (int Index)
{
	int i = -1;
	UNICODE temp_str[255];

	if (Index < PM_EXIT_GAME_MENU)
		i = Index;

	switch (Index)
	{
		case PM_ENCOUNTER_GAME_MENU:
		case PM_OUTFIT_GAME_MENU:
		case PM_SHIPYARD_GAME_MENU:
			i = GAMESTR_GAME_MENU;
			break;
		case PM_EXIT_GAME_MENU:
		case PM_EXIT_OUTFIT:
		case PM_EXIT_SHIPYARD:
		case PM_EXIT_SETTINGS:
			i = GAMESTR_EXIT_MENU;
			break;
		case PM_CONVERSE:
			i = GAMESTR_CONVERSE;
			break;
		case PM_ATTACK:
			i = GAMESTR_ATTACK;
			break;
		case PM_FUEL:
			i = GAMESTR_FUEL;
			break;
		case PM_MODULE:
			i = GAMESTR_MODULE;
			break;
		case PM_CREW:
			i = GAMESTR_CREW;
			break;
		case PM_SOUND_ON:
			i = GAMESTR_SND_ON;
			break;
		case PM_SOUND_OFF:
			i = GAMESTR_SND_OFF;
			break;
		case PM_MUSIC_ON:
			i = GAMESTR_MUS_ON;
			break;
		case PM_MUSIC_OFF:
			i = GAMESTR_MUS_OFF;
			break;
		case PM_CYBORG_OFF:
		case PM_CYBORG_NORMAL:
		case PM_CYBORG_DOUBLE:
		case PM_CYBORG_SUPER:
			i = GAMESTR_COMBAT;
			break;
		case PM_READ_VERY_SLOW:
		case PM_READ_SLOW:
		case PM_READ_MODERATE:
		case PM_READ_FAST:
		case PM_READ_VERY_FAST:
			i = GAMESTR_READING;
			break;
		case PM_CHANGE_CAPTAIN:
			i = GAMESTR_CHANGE_CAP;
			break;
		case PM_CHANGE_SHIP:
			i = GAMESTR_CHANGE_SIS;
			break;
		default:
			break;
	}

	if (i == -1 || !strlen (GAME_STRING (PLAYMENU_STRING_BASE + i)))
		return NULL;

	return GAME_STRING (PLAYMENU_STRING_BASE + i);
}

static void
Draw3DOMenuText (RECT *r, int Index)
{
	TEXT text;
	SIZE leading;
	RECT block;
	FONT OldFont;
	Color OldColor;

	if (IndexToText (Index) == NULL)
		return;

	OldFont = SetContextFont (LoadFont (PLAYMENU_FONT));
	OldColor = SetContextForeGroundColor (PM_RECT_COLOR);

	GetContextFontLeading (&leading);

	if (!optCustomBorder)
	{
		block = *r;
		block.corner.y += DOS_NUM_SCL (2);
		block.extent.height = leading;
		DrawFilledRectangle (&block); // with PM_RECT_COLOR
	}
	else
		DrawBorder (TEXT_LABEL_FRAME);

	text.align = ALIGN_CENTER;
	text.baseline.x = r->corner.x + (r->extent.width >> 1);
	text.baseline.y = r->corner.y + leading - NDOS_NUM_SCL (2);
	text.pStr = AlignText (IndexToText (Index), &text.baseline.x);
	text.CharCount = (COUNT)~0;

	font_DrawShadowedText (&text, NORTH_WEST_SHADOW, PM_TEXT_COLOR,
			PM_SHADOW_COLOR);

	SetContextFont (OldFont);
	SetContextForeGroundColor (OldColor);
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
	r.corner.x = s.origin.x - RES_SCALE (1);
	if (optWhichMenu == OPT_PC)
		r.corner.y = s.origin.y - PC_MENU_HEIGHT - IF_HD (2);
	else
		r.corner.y = s.origin.y - RES_SCALE (11) + DOS_NUM_SCL (3);
	r.extent.width = RADAR_WIDTH + RES_SCALE (3);
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
			end_index = beg_index + 6;

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
				case PM_READ_VERY_SLOW:
				case PM_READ_SLOW:
				case PM_READ_MODERATE:
				case PM_READ_FAST:
				case PM_READ_VERY_FAST:
					NewState = 3;
					break;
				case PM_CHANGE_CAPTAIN:
					NewState = 4;
					break;
				case PM_CHANGE_SHIP:
					NewState = 5;
					break;
				case PM_EXIT_SETTINGS:
					NewState = 6;
					break;
			}
		}
		r.extent.height = RADAR_HEIGHT + RES_SCALE (12);

		DrawPCMenu (beg_index, end_index, (BYTE)NewState, hilite, &r);
		s.frame = 0;

		if (optSubmenu)
			FunkyMenu (beg_index + (BYTE)NewState, s);
	}
	else
	{
		// Gray rectangle behind Lander and HyperSpace radar
		r.corner.x -= RES_SCALE (1);
		r.corner.y += DOS_NUM_SCL (5);
		r.extent.width += RES_SCALE (1);
		r.extent.height = RADAR_HEIGHT
				+ RES_SCALE (isPC (optWhichMenu) ? 9 : 12);
		r.extent.height -= DOS_NUM_SCL (6);

		if (!optCustomBorder)
			DrawFilledRectangle (&r);
		else
			DrawBorder (SIS_RADAR_FRAME);
	}
	if (s.frame)
	{
		if (optSubmenu)
			FunkyMenu (beg_index + (BYTE)NewState, s);
		else
			DrawStamp (&s);

		Draw3DOMenuText (&r, beg_index + NewState);

		switch (beg_index + NewState)
		{
			TEXT t;
			UNICODE buf[20];

			case PM_CREW:
				t.baseline.x = s.origin.x + RADAR_WIDTH - RES_SCALE (2);
				t.baseline.y = s.origin.y + RADAR_HEIGHT - RES_SCALE (2);
				t.align = ALIGN_RIGHT;
				t.CharCount = (COUNT)~0;
				t.pStr = buf;
				snprintf (buf, sizeof buf, "%u", GLOBAL (CrewCost));
				if (isPC (optWhichFonts))
					SetContextFont (TinyFont);
				else
					SetContextFont (TinyFontBold);
				SetContextForeGroundColor (THREEDOMENU_TEXT_COLOR);
				font_DrawText (&t);
				break;
			case PM_FUEL:
				t.baseline.x = s.origin.x + RADAR_WIDTH - RES_SCALE (2);
				t.baseline.y = s.origin.y + RADAR_HEIGHT - RES_SCALE (2);
				t.align = ALIGN_RIGHT;
				t.CharCount = (COUNT)~0;
				t.pStr = buf;
				snprintf (buf, sizeof buf, "%u", GLOBAL (FuelCost));
				if (optWhichFonts == OPT_3DO && !optWholeFuel)
					SetContextFont (TinyFontBold);
				else
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
DrawMineralHelpers (void)
{
	CONTEXT OldContext;
	COUNT i, digit;
	STAMP s;
	TEXT t;
	UNICODE buf[40];
	RECT r;
	SIZE leading;

	OldContext = SetContext (StatusContext);

	BatchGraphics ();

	DrawFlagStatDisplay (GAME_STRING (STATUS_STRING_BASE + 6));

	SetContextFont (TinyFont);

	GetContextFontLeading (&leading);
	leading -= RES_SCALE (1);

#define ELEMENT_ORG_X      RES_SCALE (22)
#define ELEMENT_ORG_Y      RES_SCALE (33)
#define ELEMENT_SPACING_Y  RES_SCALE (9)
#define ELEMENT_SPACING_X  RES_SCALE (32)
#define HD_ALIGN_DOTS IF_HD (2)

	// setup element icons
	s.frame = SetAbsFrameIndex (MiscDataFrame,
			(NUM_SCANDOT_TRANSITIONS << 1) + 3);
	s.origin.x = ELEMENT_ORG_X + HD_ALIGN_DOTS;
	s.origin.y = ELEMENT_ORG_Y + HD_ALIGN_DOTS;
	// setup element worths
	t.baseline.x = ELEMENT_ORG_X + RES_SCALE (12);
	t.baseline.y = ELEMENT_ORG_Y + RES_SCALE (3);
	t.align = ALIGN_RIGHT;
	t.pStr = buf;

	// draw element icons and worths
	for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i)
	{
		// draw element icon
		DrawStamp (&s);
		s.frame = SetRelFrameIndex (s.frame, 5);
		s.origin.y += ELEMENT_SPACING_Y;

		// print x'es
		SetContextForeGroundColor (MODULE_PRICE_COLOR);
		snprintf (buf, sizeof buf, "%s", "x");
		font_DrawText (&t);

		// print element worth
		SetContextForeGroundColor (MODULE_NAME_COLOR);
		snprintf (buf, sizeof buf, "%u", GLOBAL (ElementWorth[i]));
		digit = RES_SCALE (11);
		t.baseline.x += digit;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.x -= digit;
		t.baseline.y += ELEMENT_SPACING_Y;
	}

	r.corner.x = RES_SCALE (4);
	r.corner.y = t.baseline.y - RES_SCALE (7);
	r.extent.width = FIELD_WIDTH - RES_SCALE (3);
	r.extent.height = RES_SCALE (1);
	SetContextForeGroundColor (CARGO_SELECTED_BACK_COLOR);
	DrawFilledRectangle (&r);

	SetContextForeGroundColor (MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 8); // FUEL:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 9); // CREW:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 10); // LAND.:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 11); // CARGO:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	SetContextForeGroundColor (MODULE_PRICE_COLOR);
	t.align = ALIGN_RIGHT;
	t.baseline.x = r.extent.width + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = WholeFuelValue ();
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;
	snprintf (buf, sizeof (buf), "%u", GLOBAL_SIS (CrewEnlisted));
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;
	snprintf (buf, sizeof (buf), "%u", GLOBAL_SIS (NumLanders));
	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	t.baseline.y += leading;

	if (GetStorageBayCapacity ())
	{
		float totalCapacity = (float)GLOBAL_SIS (TotalElementMass)
				/ (float)GetStorageBayCapacity () * 100;

		if (totalCapacity == 100)
			snprintf (buf, sizeof (buf), "%.0f%%", totalCapacity);
		else if (totalCapacity > 9)
			snprintf (buf, sizeof (buf), "%.1f%%", totalCapacity);
		else
			snprintf (buf, sizeof (buf), "%.2f%%", totalCapacity);
	}
	else
	{
		snprintf (buf, sizeof (buf), "---");
	}

	t.pStr = buf;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContext (OldContext);
}

void
DrawBorder (BYTE Visible)
{
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (ScreenContext);

	s.origin.x = 0;
	s.origin.y = 0;

	if (optCustomBorder)
	{
		s.frame = SetAbsFrameIndex (BorderFrame, Visible);
		DrawStamp (&s);
	}
	else if (IS_HD)
	{
		s.frame = SetAbsFrameIndex (HDBorderFrame, Visible);
		DrawStamp (&s);
	}
	
	SetContext (OldContext);
}

void
FunkyMenu (BYTE m, STAMP stmp)
{
	static BOOLEAN subMenuFlag;

	if ((!stmp.frame && m >= PM_ALT_MSCAN
			&& m <= PM_ALT_EXIT_SCAN)
			|| m == PM_LAUNCH_LANDER)
	{
		if (stmp.frame)
			DrawStamp (&stmp);

		DrawMineralHelpers ();

		subMenuFlag = TRUE;

		if (stmp.frame)
			return;
	}
	else if (subMenuFlag == TRUE)
	{
		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);
		subMenuFlag = FALSE;
	}

	if (stmp.frame)
		DrawStamp (&stmp);
}
