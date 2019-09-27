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

#include "lander.h"
#include "scan.h"
#include "planets.h"
#include "../colors.h"
#include "../controls.h"
#include "../gamestr.h"
#include "../setup.h"
#include "../util.h"
#include "../sounds.h"
#include "../uqmdebug.h"
#include "options.h"
#include "libs/inplib.h"
#include "../nameref.h"
#include "../ifontres.h"
#include "../igfxres.h"

#include <ctype.h>
#include <string.h>


#define NUM_CELL_COLS ((MAP_WIDTH / RES_SCALE(6)) + IF_HD(7)) // JMS_GFX 
#define NUM_CELL_ROWS ((MAP_HEIGHT / RES_SCALE(6)) + IF_HD(2)) // JMS_GFX
#define MAX_CELL_COLS 40

extern FRAME SpaceJunkFrame;

static void
ClearReportArea (void)
{
	COUNT x, y;
	RECT r;
	STAMP s;
	COORD startx;

	if (optWhichFonts == OPT_PC)
		s.frame = SetAbsFrameIndex (SpaceJunkFrame, 21);
	else
		s.frame = SetAbsFrameIndex (SpaceJunkFrame, 18);
	GetFrameRect (s.frame, &r);

	BatchGraphics ();

	SetContextBackGroundColor (BLACK_COLOR);
	ClearDrawable ();
	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x00, 0x07, 0x00), 0x57));
	
	startx = (1 + (r.extent.width >> 1) - 1) - IF_HD(8);
	s.origin.y = 1 + IF_HD(7);
	for (y = 0; y < NUM_CELL_ROWS; ++y)
	{
		s.origin.x = startx;
		for (x = 0; x < NUM_CELL_COLS; ++x)
		{
			if (optWhichFonts == OPT_PC)
				DrawStamp (&s);
			else
				DrawFilledStamp (&s);
			
			s.origin.x += r.extent.width + 1;
		}
		s.origin.y += r.extent.height + 1;
	}

	UnbatchGraphics ();
}

static void
MakeReport (SOUND ReadOutSounds, UNICODE *pStr, COUNT StrLen)
{
	BYTE ButtonState;
	int end_page_len;
	UNICODE end_page_buf[200];
	UniChar last_c = 0;
	COUNT row_cells;
	BOOLEAN Sleepy;
	RECT r;
	TEXT t;

	sprintf (end_page_buf, "%s\n", GAME_STRING (SCAN_STRING_BASE + NUM_SCAN_TYPES));
	end_page_len = utf8StringCount (end_page_buf);

	GetFrameRect (SetAbsFrameIndex (SpaceJunkFrame, 18), &r);

	t.align = ALIGN_LEFT;
	t.CharCount = 1;
	t.pStr = pStr;

	Sleepy = TRUE;

	FlushInput ();
	// XXX: this is a pretty ugly goto
	goto InitPageCell;

	while (StrLen)
	{
		COUNT col_cells;
		const UNICODE *pLastStr;
		const UNICODE *pNextStr;
		COUNT lf_pos;

		pLastStr = t.pStr;

		// scan for LFs in the remaining string
		// trailing LF will be ignored
		for (lf_pos = StrLen, pNextStr = t.pStr;
				lf_pos && getCharFromString (&pNextStr) != '\n';
				--lf_pos)
 			;

		col_cells = 0;
		// check if the remaining text fits on current screen
		if (row_cells == NUM_CELL_ROWS - 1
				&& (StrLen > NUM_CELL_COLS || lf_pos > 1))
		{
			col_cells = (NUM_CELL_COLS >> 1) - (end_page_len >> 1);
			t.pStr = end_page_buf;
			StrLen += end_page_len;
		}
		t.baseline.x = 1 + (r.extent.width >> 1) 
			+ (col_cells * (r.extent.width + 1)) 
			- 1 - IF_HD(8); // JMS_GFX
		do
		{
			COUNT word_chars;
			const UNICODE *pStr;
			UniChar c;
 
			pStr = t.pStr;
			pNextStr = t.pStr;
			while (UniChar_isGraph (getCharFromString (&pNextStr)))
				pStr = pNextStr;

			word_chars = utf8StringCountN (t.pStr, pStr);
			if ((col_cells += word_chars) <= NUM_CELL_COLS)
			{
				TimeCount TimeOut;

				if (StrLen -= word_chars)
					--StrLen;
				TimeOut = GetTimeCounter ();
				while (word_chars--)
				{
					pNextStr = t.pStr;
					c = getCharFromString (&pNextStr);
					
					if (!Sleepy || (GLOBAL (CurrentActivity) & CHECK_ABORT))
						font_DrawText (&t);
					else
					{
						font_DrawText (&t);

						PlaySound (ReadOutSounds, NotPositional (), NULL,
								GAME_SOUND_PRIORITY);

						if (last_c && last_c != ' ' && last_c != ',' &&
								last_c != '.' && last_c != '!' && last_c != '?')
						{
							if (c == ',')
								TimeOut += ONE_SECOND / 4;
							if (c == '.' || c == '!' || c == '?')
								TimeOut += ONE_SECOND / 2;
							else
								TimeOut += ONE_SECOND / 20;
						}
						else
							TimeOut += ONE_SECOND / 20;
						last_c = c;

						if (word_chars == 0)
							TimeOut += ONE_SECOND / 20;

						if (WaitForAnyButtonUntil (TRUE, TimeOut, FALSE))
						{
							Sleepy = FALSE;
							// We draw the whole thing at once after this
							BatchGraphics ();
						}
					}
					t.pStr = pNextStr;
					t.baseline.x += r.extent.width + 1;
				}

				++col_cells;
				last_c = getCharFromString (&t.pStr);
				t.baseline.x += r.extent.width + 1;
			}
		} while (col_cells <= NUM_CELL_COLS && last_c != '\n' && StrLen);

		t.baseline.y += r.extent.height + 1;
		if (++row_cells == NUM_CELL_ROWS || StrLen == 0)
		{
			t.pStr = pLastStr;
			if (!Sleepy)
			{
				UnbatchGraphics ();
			}

			if (!WaitForAnyButton (TRUE, WAIT_INFINITE, FALSE))
				break;

InitPageCell:
			ButtonState = 1;
			t.baseline.y = r.extent.height + 1 + IF_HD(12); // JMS_GFX
			row_cells = 0;
			if (StrLen)
			{
				if (!Sleepy)
					BatchGraphics ();
				ClearReportArea();
				SetContextForeGroundColor (
						BUILD_COLOR (MAKE_RGB15 (0x00, 0x1F, 0x00), 0xFF));
			}
		}
	}
}

void
DoDiscoveryReport (SOUND ReadOutSounds)
{
	CONTEXT OldContext;
	CONTEXT context;
	BOOLEAN ownContext;
	STAMP saveStamp;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif

	context = GetScanContext (&ownContext);
	OldContext = SetContext (context);
	saveStamp = SaveContextFrame (NULL);
	{
		FONT OldFont;
		FRAME OldFontEffect;

		OldFont = SetContextFont (
				pSolarSysState->SysInfo.PlanetInfo.LanderFont);
		if (optWhichFonts == OPT_PC)
			OldFontEffect = SetContextFontEffect (
					pSolarSysState->SysInfo.PlanetInfo.LanderFontEff);
		else
			OldFontEffect = SetContextFontEffect (NULL);

		MakeReport (ReadOutSounds,
			(UNICODE *)GetStringAddress (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString),
			GetStringLength (pSolarSysState->SysInfo.PlanetInfo.DiscoveryString));
		
		SetContextFontEffect (OldFontEffect);
		SetContextFont (OldFont);
	}
	// Restore previous screen
	DrawStamp (&saveStamp);
	SetContext (OldContext);
	// TODO: Make CONTEXT ref-counted
	if (ownContext)
		DestroyScanContext ();

	DestroyDrawable (ReleaseDrawable (saveStamp.frame));

	WaitForNoInput (WAIT_INFINITE, TRUE);
}

void
DoSpaceReport(SOUND ReadOutSounds, STRING report)
{
	CONTEXT OldContext;
	CONTEXT context;
	BOOLEAN ownContext;
	STAMP saveStamp;

#ifdef DEBUG
	if (disableInteractivity)
		return;
#endif

	context = GetScanContext(&ownContext);
	OldContext = SetContext(context);
	saveStamp = SaveContextFrame(NULL);
	{
		FONT OldFont;
		FRAME OldFontEffect;

		OldFont = SetContextFont(LoadFont(LANDER_FONT));
		if (optWhichFonts == OPT_PC)
			OldFontEffect = SetContextFontEffect(
				CaptureDrawable(LoadGraphic(LANDER_FONTEFF_PMAP_ANIM)));
		else
			OldFontEffect = SetContextFontEffect(NULL);

		MakeReport(ReadOutSounds, (UNICODE *)GetStringAddress(report), GetStringLength(report));

		SetContextFontEffect(OldFontEffect);
		SetContextFont(OldFont);

		DestroyFont(OldFont);
		OldFont = NULL;
		DestroyDrawable(ReleaseDrawable(OldFontEffect));
		OldFontEffect = NULL;
	}
	// Restore previous screen
	DrawStamp(&saveStamp);
	SetContext(OldContext);
	// TODO: Make CONTEXT ref-counted
	if (ownContext)
		DestroyScanContext();

	DestroyDrawable(ReleaseDrawable(saveStamp.frame));

	WaitForNoInput(WAIT_INFINITE, TRUE);
}