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


#define COL_MULTIPLIER (isPC (optSuperPC) ? 7 : 6)
#define NUM_CELL_COLS (UQM_MAP_WIDTH / COL_MULTIPLIER)
#define NUM_CELL_ROWS (SC2_MAP_HEIGHT / 6)
#define MAX_CELL_COLS 40 // Why is this is never used???

extern FRAME SpaceJunkFrame;

static void
ClearReportArea (COORD startx)
{
	COUNT x, y;
	RECT r;
	STAMP s;

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

	s.origin.y = RES_SCALE (1);
	for (y = 0; y < NUM_CELL_ROWS; ++y)
	{
		s.origin.x = startx;
		for (x = 0; x < NUM_CELL_COLS; ++x)
		{
			if (optWhichFonts == OPT_PC)
				DrawStamp (&s);
			else
				DrawFilledStamp (&s);

			s.origin.x += r.extent.width + RES_SCALE (1);
		}
		s.origin.y += r.extent.height + RES_SCALE (1);
	}

	UnbatchGraphics ();
}

static void
MakeReport (SOUND ReadOutSounds, UNICODE *pStr, COUNT StrLen)
{
	int end_page_len;
	UNICODE end_page_buf[200];
	UniChar last_c = 0;
	COUNT row_cells;
	BOOLEAN Sleepy;
	RECT r, contextRect;
	TEXT t;
	Color fgcolor;
	COORD startx;

	SIZE total_lines = -1;
	SIZE curr_line = -1;
	COUNT first_line_length = 0;

	sprintf (end_page_buf, "%s\n",
			GAME_STRING (SCAN_STRING_BASE + NUM_SCAN_TYPES));
	end_page_len = utf8StringCount (end_page_buf);

	GetFrameRect (SetAbsFrameIndex (SpaceJunkFrame, 18), &r);
	GetContextClipRect (&contextRect);

	t.align = ALIGN_LEFT;
	t.CharCount = 1;
	t.pStr = pStr;
	Sleepy = TRUE;

	FlushInput ();

	t.baseline.y = r.extent.height + RES_SCALE (1);
			// Text vertical alignment
	row_cells = 0;
	fgcolor = BUILD_COLOR (MAKE_RGB15 (0x00, 0x1F, 0x00), 0xFF);

	if (contextRect.extent.width < SCALED_MAP_WIDTH)
		startx = RES_SCALE (1); // Special case if we're in space
	else
	{	// In DOS version first cell is 3p away from the edge of the
		// context, and 2 in UQM
		startx = RES_SCALE (RES_DESCALE (r.extent.width) >> 1)
			+ (isPC (optSuperPC) ? RES_SCALE (1) : 0);
	}

	if (StrLen)
	{
		ClearReportArea (startx);
		SetContextForeGroundColor (fgcolor);

		if (is3DO (optSuperPC))
		{
			const UNICODE* pCurrStr;
			COUNT length;

			pCurrStr = t.pStr;
			total_lines = 0;
			curr_line = 0;

			for (length = StrLen; length > 0; length--)
			{
				if (getCharFromString (&pCurrStr) == '\n')
					total_lines++;

				if (total_lines == 0)
					first_line_length++;
			}
		}
	}

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

			// We're adding lines - compensate for it
			if (curr_line >= 0)
				curr_line--;
		}
		if (total_lines >= 0)
		{
			if (curr_line == 0)
				col_cells = (NUM_CELL_COLS - first_line_length) >> 1;

			if (curr_line == total_lines)
				col_cells = (NUM_CELL_COLS - StrLen - 3) >> 1;
		}

		t.baseline.x = startx + (col_cells *
				(r.extent.width + RES_SCALE (1)));

		do
		{
			COUNT word_chars;
			const UNICODE *pStr;
			UniChar c;
 
			pStr = t.pStr;
			pNextStr = t.pStr;
			while (UniChar_isGraph (getCharFromString (&pNextStr)))
				pStr = pNextStr;

			word_chars = (COUNT)utf8StringCountN (t.pStr, pStr);
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
					
					if (GLOBAL (CurrentActivity) & CHECK_ABORT)
						return;
					if (!Sleepy)
						font_DrawText (&t);
					else
					{
						BYTE scale = 0;
						font_DrawText (&t);

						if (CurrentInputState.menu[KEY_MENU_RIGHT])
							scale = 2;

						PlaySound (ReadOutSounds, NotPositional (), NULL,
								GAME_SOUND_PRIORITY);

						switch (c)
						{
							case ',':
								TimeOut += ONE_SECOND / (4 << scale);
								break;
							case '.':
								TimeOut += ONE_SECOND / (2 << scale);
								break;
							case '!':
								if (last_c != '!' && last_c != ' ')
									TimeOut += ONE_SECOND / (2 << scale);
								else
									TimeOut += ONE_SECOND / (20 << scale);
								break;
							case '?':
								if (last_c != '?' && last_c != ' ')
									TimeOut += ONE_SECOND / (2 << scale);
								else
									TimeOut += ONE_SECOND / (20 << scale);
								break;
							default:
								TimeOut += ONE_SECOND / (20 << scale);
								break;
						}

						last_c = c;

						if (word_chars == 0)
							TimeOut += ONE_SECOND / (20 << scale);

						if (WaitForActButtonUntil (TRUE, TimeOut, FALSE))
						{
							Sleepy = FALSE;
							// We draw the whole thing at once after this
							BatchGraphics ();
						}
					}
					t.pStr = pNextStr;
					t.baseline.x += r.extent.width + RES_SCALE (1);
							// Text spacing
				}

				++col_cells;
				last_c = getCharFromString (&t.pStr);
				t.baseline.x += r.extent.width + RES_SCALE (1);
						// Space spacing

				if (curr_line >= 0 && last_c == '\n')
					curr_line++;
			}
		} while (col_cells <= NUM_CELL_COLS && last_c != '\n' && StrLen);

		t.baseline.y += r.extent.height + RES_SCALE (1);
				// Text vertical spacing
		if (++row_cells == NUM_CELL_ROWS || StrLen == 0)
		{
			t.pStr = pLastStr;
			if (!Sleepy)
			{
				UnbatchGraphics ();
			}

			if (!WaitForActButton (TRUE, WAIT_INFINITE, FALSE))
				break;

			t.baseline.y = r.extent.height + RES_SCALE (1);
					// Text vertical alignment
			row_cells = 0;
			if (StrLen)
			{
				if (!Sleepy)
					BatchGraphics ();
				ClearReportArea (startx);
				SetContextForeGroundColor (fgcolor);
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
				(UNICODE *)GetStringAddress (
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString),
				GetStringLength (
					pSolarSysState->SysInfo.PlanetInfo.DiscoveryString));
		
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