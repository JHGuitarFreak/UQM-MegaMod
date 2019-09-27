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

#include <ctype.h>
#include <stdlib.h>


#define CONFIRM_WIN_WIDTH RES_SCALE(80) // JMS_GFX
#define CONFIRM_WIN_HEIGHT RES_SCALE(22) // JMS_GFX

BOOLEAN WarpFromMenu = FALSE;

static void
DrawConfirmationWindow (BOOLEAN answer)
{
	Color oldfg = SetContextForeGroundColor (MENU_TEXT_COLOR);
	FONT  oldfont = SetContextFont (StarConFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	RECT r;
	TEXT t;

	BatchGraphics ();
	r.corner.x = (SCREEN_WIDTH - CONFIRM_WIN_WIDTH) >> 1;
	r.corner.y = (SCREEN_HEIGHT - CONFIRM_WIN_HEIGHT) >> 1;
	r.extent.width = CONFIRM_WIN_WIDTH;
#if defined(ANDROID) || defined(__ANDROID__)
	if (GLOBAL(CurrentActivity) & IN_BATTLE && RunAwayAllowed()) {
		r.corner.x -= RES_BOOL(40, 0);
		r.extent.width += RES_BOOL(40, 0);
	}
#endif
	r.extent.height = CONFIRM_WIN_HEIGHT;
	DrawShadowedBox (&r, SHADOWBOX_BACKGROUND_COLOR, 
			SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + RES_SCALE(8); // JMS_GFX
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE); // "Really Quit?"
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);
	t.baseline.y += RES_SCALE(10); // JMS_GFX
	t.baseline.x = r.corner.x + (r.extent.width >> 2);
#if defined(ANDROID) || defined(__ANDROID__)
	if (GLOBAL(CurrentActivity) & IN_BATTLE && RunAwayAllowed())
		t.baseline.x -= RES_BOOL(5, 0);
#endif
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 1); // "Yes"
	SetContextForeGroundColor (answer ? MENU_HIGHLIGHT_COLOR : MENU_TEXT_COLOR);
	font_DrawText (&t);
	t.baseline.x += (r.extent.width >> 1);
	t.pStr = GAME_STRING (QUITMENU_STRING_BASE + 2); // "No"
#if defined(ANDROID) || defined(__ANDROID__)
	if (GLOBAL(CurrentActivity) & IN_BATTLE && RunAwayAllowed()) {
		t.baseline.x -= RES_BOOL(10, 20);
		t.pStr = GAME_STRING(QUITMENU_STRING_BASE + 3); // "Escape Unit"
	}
#endif
	SetContextForeGroundColor (answer ? MENU_TEXT_COLOR : MENU_HIGHLIGHT_COLOR);	
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

	if (PlayingTrack ())
		PauseTrack ();

	PauseFlash ();

	{
		RECT r;
		STAMP s;
		RECT ctxRect;
		CONTEXT oldContext;
		RECT oldRect;
		BOOLEAN response = FALSE, done;

		oldContext = SetContext (ScreenContext);
		GetContextClipRect (&oldRect);
		SetContextClipRect (NULL);

		GetContextClipRect (&ctxRect);
		r.extent.width = CONFIRM_WIN_WIDTH + 4;
		r.extent.height = CONFIRM_WIN_HEIGHT + 4;
		r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
		r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
		s = SaveContextFrame (&r);
		SetSystemRect (&r);

		DrawConfirmationWindow (response);
		FlushGraphics ();

		FlushInput ();
		done = FALSE;

#if defined(ANDROID) || defined(__ANDROID__)
		if (!(GLOBAL(CurrentActivity) & IN_BATTLE)) {
			/* Abort immediately */
			response = TRUE;
			done = TRUE;
		}
#endif
		
		while (!done) {
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
			else if (PulsedInputState.menu[KEY_MENU_LEFT] || PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				response = !response;
				DrawConfirmationWindow (response);
				PlayMenuSound (MENU_SOUND_MOVE);
			}
			SleepThread (ONE_SECOND / 30);
		};

		// Restore the screen under the confirmation window
		DrawStamp (&s);
		DestroyDrawable (ReleaseDrawable (s.frame));
		ClearSystemRect ();
		if (response || (GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			result = TRUE;
			GLOBAL (CurrentActivity) |= CHECK_ABORT;
		}		
		else
		{
#if defined(ANDROID) || defined(__ANDROID__)
			if (GLOBAL(CurrentActivity) & IN_BATTLE && RunAwayAllowed())
				WarpFromMenu = TRUE;
#endif
			result = FALSE;
		}
		ExitRequested = FALSE;
		GamePaused = FALSE;
		FlushInput ();
		SetContextClipRect (&oldRect);
		SetContext (oldContext);
	}

	ContinueFlash ();

	if (PlayingTrack ())
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

	if (!bank)
	{
		log_add (log_Fatal, "FATAL: Memory exhaustion when preparing popup window");
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

	PauseFlash ();

	oldContext = SetContext (ScreenContext);
	GetContextClipRect (&oldRect);
	SetContextClipRect (NULL);

	// TODO: Maybe DrawLabelAsWindow() should return a saved STAMP?
	//   We do not know the dimensions here, and so save the whole context
	s = SaveContextFrame (NULL);

	Widget_SetFont (StarConFont);
	Widget_SetWindowColors (SHADOWBOX_BACKGROUND_COLOR, SHADOWBOX_DARK_COLOR,
			SHADOWBOX_MEDIUM_COLOR);
	DrawLabelAsWindow (&label, &windowRect);
	SetSystemRect (&windowRect);

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
	ContinueFlash ();
	SetMenuSounds (s0, s1);
	StringBank_Free (bank);
}

