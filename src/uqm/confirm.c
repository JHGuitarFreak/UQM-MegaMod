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

#include "controls.h"
#include "colors.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "gamestr.h"
#include "util.h"
#include "libs/graphics/widgets.h"
#include "libs/sound/trackplayer.h"
#include "libs/log.h"
#include "libs/resource/stringbank.h"
#include "battle.h"
#include "comm.h"

#include <ctype.h>
#include <stdlib.h>


#define CONFIRM_WIN_WIDTH RES_SCALE (84)
#define CONFIRM_WIN_HEIGHT RES_SCALE (26)

BOOLEAN WarpFromMenu = FALSE;

static void
DrawConfirmationWindow (BOOLEAN answer, BOOLEAN confirm)
{
	Color oldfg = SetContextForeGroundColor (SHADOWBOX_DARK_COLOR);
	FONT  oldfont = SetContextFont (StarConFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	RECT r;
	TEXT t;

	BatchGraphics ();
	r.corner.x = (SCREEN_WIDTH - CONFIRM_WIN_WIDTH) >> 1;
	r.corner.y = (SCREEN_HEIGHT - CONFIRM_WIN_HEIGHT) >> 1;
	r.extent.width = CONFIRM_WIN_WIDTH;
	r.extent.height = CONFIRM_WIN_HEIGHT;
	if (!IS_HD)
	{
		DrawShadowedBox (&r, ALT_SHADOWBOX_BACKGROUND_COLOR,
				SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);
	}
	else
	{
		DrawRenderedBox (&r, TRUE, ALT_SHADOWBOX_BACKGROUND_COLOR,
				THICK_OUTER_BEVEL, FALSE);
	}


	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + RES_SCALE (10);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE); // "Really Quit?"
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += RES_SCALE (10);
	t.baseline.x = r.corner.x + (r.extent.width >> 2);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 1); // "Yes"
	SetContextForeGroundColor (
			answer ? (confirm ? WHITE_COLOR : MENU_HIGHLIGHT_COLOR) :
			BLACK_COLOR);
	font_DrawText (&t);
	t.baseline.x += (r.extent.width >> 1);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 2); // "No"
	SetContextForeGroundColor (
			answer ? BLACK_COLOR : MENU_HIGHLIGHT_COLOR);
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContextFontEffect (oldFontEffect);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldfg);
}

BOOLEAN
DoConfirmExit (void)
{
	BOOLEAN result;
	BOOLEAN FlashPaused;

	if (PlayingTrack ())
		PauseTrack ();

	FlashPaused = PauseFlash ();

	{
		RECT r;
		STAMP s;
		RECT ctxRect;
		CONTEXT oldContext;
		RECT oldRect;
		BOOLEAN response = FALSE, done;
		Color OldColor;
		DrawMode mode, oldMode;
		BYTE oldVolume;
		TimeCount deltaT;

		deltaT = GetTimeCounter ();

		oldContext = SetContext (ScreenContext);
		GetContextClipRect (&oldRect);
		SetContextClipRect (NULL);

		GetContextClipRect (&ctxRect);
		r.extent.width = CONFIRM_WIN_WIDTH + RES_SCALE (4);
		r.extent.height = CONFIRM_WIN_HEIGHT + RES_SCALE (4);
		r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
		r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
		s = SaveContextFrame (&ctxRect);

		mode = MAKE_DRAW_MODE (DRAW_DESATURATE, DESAT_AMOUNT);
		oldMode = SetContextDrawMode (mode);
		DrawFilledRectangle (&ctxRect);
		SetContextDrawMode (oldMode);
		OldColor = SetContextForeGroundColor 
				(BUILD_COLOR_RGBA (0x00, 0x00, 0x00, 0x30));
		DrawFilledRectangle (&ctxRect);
		SetContextForeGroundColor (OldColor);

		// There was a SetSystemRect(&r) call which we don't need anymore

		DrawConfirmationWindow (response, FALSE);
		FlushGraphics ();

		FlushInput ();
		done = FALSE;

		oldVolume = GetCurrMusicVol();
		FadeMusic (60, ONE_SECOND / 2);
		
		while (!done){
			// Forbid recursive calls or pausing here!
			ExitRequested = FALSE;
			GamePaused = FALSE;
			UpdateInputState ();
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			{	// something else triggered an exit
				done = TRUE;
				response = TRUE;
			}
			else if (PulsedInputState.menu[KEY_MENU_SELECT])
			{
				done = TRUE;
				PlayMenuSound (MENU_SOUND_SUCCESS);
			}
			else if (PulsedInputState.menu[KEY_MENU_CANCEL])
			{
				done = TRUE;
				response = FALSE;
			}
			else if (PulsedInputState.menu[KEY_MENU_LEFT]
					|| PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				response = !response;
				DrawConfirmationWindow (response, FALSE);
				PlayMenuSound (MENU_SOUND_MOVE);
			}
			SleepThread (ONE_SECOND / 30);
		};

		// Restore the screen under the confirmation window
		if (!response)
		{
			DrawStamp (&s);
			DeltaLastTime (GetTimeCounter () - deltaT);
			FadeMusic (oldVolume, ONE_SECOND / 2);
		}
		else
		{
			DrawConfirmationWindow (TRUE, TRUE);
		}
		DestroyDrawable (ReleaseDrawable (s.frame));
		ClearSystemRect ();
		if (response || (GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			result = TRUE;
			GLOBAL (CurrentActivity) |= CHECK_ABORT;
		}		
		else
		{
			result = FALSE;
		}
		ExitRequested = FALSE;
		GamePaused = FALSE;
		FlushInput ();
		SetContextClipRect (&oldRect);
		SetContext (oldContext);
	}

	if (FlashPaused)
		ContinueFlash ();

	if (PlayingTrack () && !result)
		ResumeTrack ();

	return (result);
}

typedef struct popup_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct popup_state *self);
} POPUP_STATE;

static BOOLEAN
DoPopup (struct popup_state *self)
{
	(void)self;
	SleepThread (ONE_SECOND / 20);
	return !(PulsedInputState.menu[KEY_MENU_SELECT] ||
			PulsedInputState.menu[KEY_MENU_CANCEL] ||
			(GLOBAL (CurrentActivity) & CHECK_ABORT));
}

void
DoPopupWindow (const char *msg)
{
	stringbank *bank = StringBank_Create ();
	const char *lines[30];
	WIDGET_LABEL label;
	STAMP s;
	CONTEXT oldContext;
	RECT oldRect;
	RECT windowRect;
	POPUP_STATE state;
	MENU_SOUND_FLAGS s0, s1;
	InputFrameCallback *oldCallback;
	BOOLEAN FlashPaused;

	if (!bank)
	{
		log_add (log_Fatal, "FATAL: Memory exhaustion when preparing popup"
				" window");
		exit (EXIT_FAILURE);
	}

	label.tag = WIDGET_TYPE_LABEL;
	label.parent = NULL;
 	label.handleEvent = Widget_HandleEventIgnoreAll;
	label.receiveFocus = Widget_ReceiveFocusRefuseFocus;
	label.draw = Widget_DrawLabel;
	label.height = Widget_HeightLabel;
	label.width = Widget_WidthFullScreen;
	label.line_count = SplitString (msg, '\n', 30, lines, bank);
	label.lines = lines;

	FlashPaused = PauseFlash ();

	oldContext = SetContext (ScreenContext);
	GetContextClipRect (&oldRect);
	SetContextClipRect (NULL);

	// TODO: Maybe DrawLabelAsWindow() should return a saved STAMP?
	//   We do not know the dimensions here, and so save the whole context
	s = SaveContextFrame (NULL);

	Widget_SetFont (StarConFont);
	Widget_SetWindowColors (SHADOWBOX_BACKGROUND_COLOR,
			SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);
	DrawLabelAsWindow (&label, &windowRect);
	// There was a SetSystemRect(&windowRect) call which we don't need anymore

	GetMenuSounds (&s0, &s1);
	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	oldCallback = SetInputCallback (NULL);

	state.InputFunc = DoPopup;
	DoInput (&state, TRUE);

	SetInputCallback (oldCallback);
	ClearSystemRect ();
	DrawStamp (&s);
	DestroyDrawable (ReleaseDrawable (s.frame));
	SetContextClipRect (&oldRect);
	SetContext (oldContext);

	if (FlashPaused)
		ContinueFlash ();

	SetMenuSounds (s0, s1);
	StringBank_Free (bank);
}

