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

#include "../colors.h"
#include "../controls.h"
#include "../gamestr.h"
#include "../shipcont.h"
#include "../setup.h"
#include "../sounds.h"
#include "../util.h"
#include "../sis.h"
		// for ClearSISRect(), DrawStatusMessage()
#include "planets.h"
#include "libs/graphics/drawable.h"
		// for GetFrameBounds()


#define ELEMENT_ORG_Y      RES_SCALE(35) 
#define FREE_ORG_Y         (ELEMENT_ORG_Y + (NUM_ELEMENT_CATEGORIES \
							* ELEMENT_SPACING_Y))
#define BIO_ORG_Y          RES_SCALE(119) 
#define ELEMENT_SPACING_Y  RES_SCALE(9) 

#define ELEMENT_COL_0      RES_SCALE(7) 
#define ELEMENT_COL_1      RES_SCALE(32) 
#define ELEMENT_COL_2      RES_SCALE(58) 

#define ELEMENT_SEL_ORG_X  (ELEMENT_COL_0 + RES_SCALE(7 + 5)) 
#define ELEMENT_SEL_WIDTH  (ELEMENT_COL_2 - ELEMENT_SEL_ORG_X + RES_SCALE(1)) 

#define TEXT_BASELINE      RES_SCALE(6) 


void
ShowRemainingCapacity (void)
{
	RECT r;
	TEXT t;
	CONTEXT OldContext;
	UNICODE buf[40];

	OldContext = SetContext (StatusContext);
	SetContextFont (TinyFont);

	r.corner.x = RES_SCALE(40); 
	r.corner.y = FREE_ORG_Y;

	snprintf (buf, sizeof buf, "%u",
			GetStorageBayCapacity () - GLOBAL_SIS (TotalElementMass));
	t.baseline.x = ELEMENT_COL_2 + RES_SCALE(1); 
	t.baseline.y = r.corner.y + TEXT_BASELINE;
	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.CharCount = (COUNT)~0;

	r.extent.width = t.baseline.x - r.corner.x + RES_SCALE(1); 
	r.extent.height = ELEMENT_SPACING_Y - RES_SCALE(2); 

	BatchGraphics ();
	// erase previous free amount
	SetContextForeGroundColor (CARGO_BACK_COLOR);
	DrawFilledRectangle (&r);
	// print the new free amount
	SetContextForeGroundColor (CARGO_WORTH_COLOR);
	font_DrawText (&t);
	UnbatchGraphics ();
	
	SetContext (OldContext);
}

static void
DrawElementAmount (COUNT element, bool selected)
{
	RECT r;
	TEXT t;
	UNICODE buf[40];

	r.corner.x = ELEMENT_SEL_ORG_X;
	r.extent.width = ELEMENT_SEL_WIDTH;
	r.extent.height = ELEMENT_SPACING_Y - RES_SCALE(2); 

	if (element == NUM_ELEMENT_CATEGORIES)
		r.corner.y = BIO_ORG_Y;
	else
		r.corner.y = ELEMENT_ORG_Y + (element * ELEMENT_SPACING_Y);
	
	// draw line background
	SetContextForeGroundColor (selected ?
			CARGO_SELECTED_BACK_COLOR : CARGO_BACK_COLOR);
	DrawFilledRectangle (&r);

	t.align = ALIGN_RIGHT;
	t.pStr = buf;
	t.baseline.y = r.corner.y + TEXT_BASELINE;

	if (element == NUM_ELEMENT_CATEGORIES)
	{	// Bio
		snprintf (buf, sizeof buf, "%u", GLOBAL_SIS (TotalBioMass));
	}
	else
	{	// Element
		// print element's worth
		SetContextForeGroundColor (selected ?
				CARGO_SELECTED_WORTH_COLOR : CARGO_WORTH_COLOR);
		t.baseline.x = ELEMENT_COL_1;
		snprintf (buf, sizeof buf, "%u", GLOBAL (ElementWorth[element]));
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		
		snprintf (buf, sizeof buf, "%u", GLOBAL_SIS (ElementAmounts[element]));
	}

	// print the element/bio amount
	SetContextForeGroundColor (selected ?
			CARGO_SELECTED_AMOUNT_COLOR : CARGO_AMOUNT_COLOR);
	t.baseline.x = ELEMENT_COL_2;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
}

static void
DrawCargoDisplay (void)
{
	STAMP s;
	TEXT t;
	RECT r;
	COORD cy;
	COUNT i;

	r.corner.x = 2; 
	r.extent.width = FIELD_WIDTH + 1;
	r.corner.y = RES_SCALE(20);
	// XXX: Shouldn't the height be 1 less? This draws the bottom border
	//   1 pixel too low. Or if not, why do we need another box anyway?
	r.extent.height = (RES_SCALE(129) - r.corner.y);
	DrawStarConBox (&r, 1,
			SHADOWBOX_MEDIUM_COLOR, SHADOWBOX_DARK_COLOR,
			TRUE, CARGO_BACK_COLOR);

	DrawBorder(13, FALSE);

	// draw the "CARGO" title
	SetContextFont (StarConFont);
	t.baseline.x = (STATUS_WIDTH >> 1) - RES_SCALE(1); 
	t.baseline.y = RES_SCALE(27); 
	t.align = ALIGN_CENTER;
	t.pStr = GAME_STRING (CARGO_STRING_BASE);
	t.CharCount = (COUNT)~0;
	SetContextForeGroundColor (CARGO_SELECTED_AMOUNT_COLOR);
	font_DrawText (&t);

	SetContextFont (TinyFont);

	s.frame = SetAbsFrameIndex (MiscDataFrame,
			(NUM_SCANDOT_TRANSITIONS * 2) + 3);
	if (IS_HD)
		s.frame = SetRelFrameIndex (s.frame, -1); 

	r.corner.x = ELEMENT_COL_0;
	r.extent = GetFrameBounds (s.frame);
	s.origin.x = r.corner.x + (r.extent.width >> 1);

	cy = ELEMENT_ORG_Y;

	// print element column headings
	t.align = ALIGN_RIGHT;
	t.baseline.y = cy - RES_SCALE(1); 
	t.CharCount = (COUNT)~0;

	SetContextForeGroundColor (CARGO_WORTH_COLOR);
	t.baseline.x = ELEMENT_COL_1;
	t.pStr = "$";
	font_DrawText (&t);

	t.baseline.x = ELEMENT_COL_2;
	t.pStr = "#";
	font_DrawText (&t);

	// draw element icons and print amounts
	for (i = 0; i < NUM_ELEMENT_CATEGORIES; ++i, cy += ELEMENT_SPACING_Y)
	{
		// erase background under an element icon
		SetContextForeGroundColor (CARGO_BACK_COLOR); // Serosis: Was actually supposed to be black
		r.corner.y = cy;
		DrawFilledRectangle (&r);

		// draw an element icon
		s.origin.y = r.corner.y + (r.extent.height >> 1);
		DrawStamp (&s);
		s.frame = SetRelFrameIndex (s.frame, 5);

		DrawElementAmount (i, false);
	}

	// erase background under the Bio icon
	SetContextForeGroundColor (CARGO_BACK_COLOR); // Serosis: Was actually supposed to be black
	r.corner.y = BIO_ORG_Y;
	DrawFilledRectangle (&r);

	// draw the Bio icon
	s.origin.y = r.corner.y + (r.extent.height >> 1);
	s.frame = SetAbsFrameIndex (s.frame, 68);
	DrawStamp (&s);

	// print the Bio amount
	DrawElementAmount (NUM_ELEMENT_CATEGORIES, false);

	// draw the line over the Bio amount
	r.corner.x = RES_SCALE(4); 
	r.corner.y = BIO_ORG_Y - RES_SCALE(2); 
	r.extent.width = FIELD_WIDTH - RES_SCALE(3); 
	r.extent.height = 1;
	SetContextForeGroundColor (CARGO_SELECTED_BACK_COLOR);
	DrawFilledRectangle (&r);

	// print "Free"
	t.baseline.x = RES_SCALE(5); 
	t.baseline.y = FREE_ORG_Y + TEXT_BASELINE;
	t.align = ALIGN_LEFT;
	t.pStr = GAME_STRING (CARGO_STRING_BASE + 1);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	ShowRemainingCapacity ();
}

void
DrawCargoStrings (BYTE OldElement, BYTE NewElement)
{
	CONTEXT OldContext;

	OldContext = SetContext (StatusContext);
	SetContextFont (TinyFont);

	BatchGraphics ();

	if (OldElement > NUM_ELEMENT_CATEGORIES)
	{	// Asked for the initial display
		DrawCargoDisplay ();

		// do not draw unselected again this time
		OldElement = NewElement;
	}

	if (OldElement != NewElement)
	{	// unselect the previous element
		DrawElementAmount (OldElement, false);
	}

	if (NewElement != (BYTE)~0)
	{	// select the new element
		DrawElementAmount (NewElement, true);
	}

	UnbatchGraphics ();
	SetContext (OldContext);
}

static void
DrawElementDescription (COUNT element)
{
	DrawStatusMessage (GAME_STRING (element + (CARGO_STRING_BASE + 2)));
}

static BOOLEAN
DoDiscardCargo (MENU_STATE *pMS)
{
	BYTE NewState;
	BOOLEAN select, cancel, back, forward;
	
	select = PulsedInputState.menu[KEY_MENU_SELECT];
	cancel = PulsedInputState.menu[KEY_MENU_CANCEL];
	back = PulsedInputState.menu[KEY_MENU_UP] || PulsedInputState.menu[KEY_MENU_LEFT];
	forward = PulsedInputState.menu[KEY_MENU_DOWN] || PulsedInputState.menu[KEY_MENU_RIGHT];

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;

	if (cancel)
	{
		return FALSE;
	}
	else if (select)
	{
		if (GLOBAL_SIS (ElementAmounts[pMS->CurState]))
		{
			--GLOBAL_SIS (ElementAmounts[pMS->CurState]);
			DrawCargoStrings (pMS->CurState, pMS->CurState);

			--GLOBAL_SIS (TotalElementMass);
			ShowRemainingCapacity ();
		}
		else
		{	// no element left in cargo hold
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else
	{
		NewState = pMS->CurState;
		if (back)
		{
			if (NewState == 0)
				NewState += NUM_ELEMENT_CATEGORIES;
			--NewState;
		}
		else if (forward)
		{
			++NewState;
			if (NewState == NUM_ELEMENT_CATEGORIES)
				NewState = 0;
		}

		if (NewState != pMS->CurState)
		{
			DrawCargoStrings (pMS->CurState, NewState);
			DrawElementDescription (NewState);
			pMS->CurState = NewState;
		}
	}

	SleepThread (ONE_SECOND / 30);

	return (TRUE);
}

void
CargoMenu (void)
{
	MENU_STATE MenuState;

	memset (&MenuState, 0, sizeof MenuState);

	// draw the initial cargo display
	DrawCargoStrings ((BYTE)~0, MenuState.CurState);
	DrawElementDescription (MenuState.CurState);

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);

	MenuState.InputFunc = DoDiscardCargo;
	DoInput (&MenuState, TRUE);

	// erase the cargo display
	ClearSISRect (DRAW_SIS_DISPLAY);
}

