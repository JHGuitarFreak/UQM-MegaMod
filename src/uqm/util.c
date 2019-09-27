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
#include "util.h"
#include "setup.h"
#include "units.h"
#include "settings.h"
#include "libs/inplib.h"
#include "libs/sound/trackplayer.h"
#include "libs/mathlib.h"
#include "libs/log.h"
#include "hyper.h"

void
DrawStarConBox (RECT *pRect, SIZE BorderWidth, Color TopLeftColor,
		Color BottomRightColor, BOOLEAN FillInterior, Color InteriorColor)
{
	RECT locRect;

	if (BorderWidth == 0)
		BorderWidth = 2;
	else
	{
		SetContextForeGroundColor (TopLeftColor);
		locRect.corner = pRect->corner;
		locRect.extent.width = pRect->extent.width;
		locRect.extent.height = 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.width -= 2;
			DrawFilledRectangle (&locRect);
		}

		locRect.corner = pRect->corner;
		locRect.extent.width = 1;
		locRect.extent.height = pRect->extent.height;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.height -= 2;
			DrawFilledRectangle (&locRect);
		}

		SetContextForeGroundColor (BottomRightColor);
		locRect.corner.x = pRect->corner.x + pRect->extent.width - 1;
		locRect.corner.y = pRect->corner.y + 1;
		locRect.extent.height = pRect->extent.height - 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			--locRect.corner.x;
			++locRect.corner.y;
			locRect.extent.height -= 2;
			DrawFilledRectangle (&locRect);
		}

		locRect.corner.x = pRect->corner.x;
		locRect.extent.width = pRect->extent.width;
		locRect.corner.y = pRect->corner.y + pRect->extent.height - 1;
		locRect.extent.height = 1;
		DrawFilledRectangle (&locRect);
		if (BorderWidth == 2)
		{
			++locRect.corner.x;
			--locRect.corner.y;
			locRect.extent.width -= 2;
			DrawFilledRectangle (&locRect);
		}
	}

	if (FillInterior)
	{
		SetContextForeGroundColor (InteriorColor);
		locRect.corner.x = pRect->corner.x + BorderWidth;
		locRect.corner.y = pRect->corner.y + BorderWidth;
		locRect.extent.width = pRect->extent.width - (BorderWidth << 1);
		locRect.extent.height = pRect->extent.height - (BorderWidth << 1);
		DrawFilledRectangle (&locRect);
	}
}

DWORD
SeedRandomNumbers (void)
{
	DWORD cur_time;

	cur_time = GetTimeCounter ();
	TFB_SeedRandom (cur_time);

	return (cur_time);
}

STAMP
SaveContextFrame (const RECT *saveRect)
{
	STAMP s;

	if (saveRect)
	{	// a portion of the context
		s.origin = saveRect->corner;
	}
	else
	{	// the entire context
		s.origin.x = 0;
		s.origin.y = 0;
	}

	s.frame = CaptureDrawable (CopyContextRect (saveRect));

	return s;
}

BOOLEAN
PauseGame (void)
{
	RECT r;
	STAMP s;
	CONTEXT OldContext;
	STAMP saveStamp;
	RECT ctxRect;
	POINT oldOrigin;
	RECT OldRect;

	if (ActivityFrame == 0
			|| (GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_PAUSE))
			|| (LastActivity & (CHECK_LOAD | CHECK_RESTART)))
		return (FALSE);
		
	GLOBAL (CurrentActivity) |= CHECK_PAUSE;

	if (PlayingTrack ())
		PauseTrack ();

	OldContext = SetContext (ScreenContext);
	oldOrigin = SetContextOrigin (MAKE_POINT (0, 0));
	GetContextClipRect (&OldRect);
	SetContextClipRect (NULL);

	GetContextClipRect (&ctxRect);
	GetFrameRect (ActivityFrame, &r);
	r.corner.x = (ctxRect.extent.width - r.extent.width) >> 1;
	r.corner.y = (ctxRect.extent.height - r.extent.height) >> 1;
	saveStamp = SaveContextFrame (&r);

	// TODO: This should draw a localizable text message instead
	s.origin = r.corner;
	s.frame = ActivityFrame;
	SetSystemRect (&r);
	DrawStamp (&s);

	FlushGraphics ();

	while (ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	while (!ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	while (ImmediateInputState.menu[KEY_PAUSE] && GamePaused)
	{
		BeginInputFrame ();
		TaskSwitch ();
	}

	GamePaused = FALSE;

	DrawStamp (&saveStamp);
	DestroyDrawable (ReleaseDrawable (saveStamp.frame));
	ClearSystemRect ();

	SetContextClipRect (&OldRect);
	SetContextOrigin (oldOrigin);
	SetContext (OldContext);

	WaitForNoInput (ONE_SECOND / 4, TRUE);

	if (PlayingTrack ())
		ResumeTrack ();


	TaskSwitch ();
	GLOBAL (CurrentActivity) &= ~CHECK_PAUSE;
	return (TRUE);
}

// Waits for a button to be pressed
// Returns TRUE if the wait succeeded (found input)
//    FALSE if timed out or game aborted
BOOLEAN
WaitForAnyButtonUntil (BOOLEAN newButton, TimeCount timeOut,
		BOOLEAN resetInput)
{
	BOOLEAN buttonPressed;

	if (newButton && !WaitForNoInputUntil (timeOut, FALSE))
		return FALSE;

	buttonPressed = AnyButtonPress (TRUE);
	while (!buttonPressed
			&& (timeOut == WAIT_INFINITE || GetTimeCounter () < timeOut)
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT)
			&& !QuitPosted)
	{
		SleepThread (ONE_SECOND / 40);
		buttonPressed = AnyButtonPress (TRUE);
	} 

	if (resetInput)
		FlushInput ();

	return buttonPressed;
}

BOOLEAN
WaitForAnyButton (BOOLEAN newButton, TimePeriod duration, BOOLEAN resetInput)
{
	TimeCount timeOut = duration;
	if (duration != WAIT_INFINITE)
		timeOut += GetTimeCounter ();
	return WaitForAnyButtonUntil (newButton, timeOut, resetInput);
}

// Returns TRUE if the wait succeeded (found no input)
//    FALSE if timed out or game aborted
BOOLEAN
WaitForNoInputUntil (TimeCount timeOut, BOOLEAN resetInput)
{
	BOOLEAN buttonPressed;

	buttonPressed = AnyButtonPress (TRUE);
	while (buttonPressed
			&& (timeOut == WAIT_INFINITE || GetTimeCounter () < timeOut)
			&& !(GLOBAL (CurrentActivity) & CHECK_ABORT)
			&& !QuitPosted)
	{
		SleepThread (ONE_SECOND / 40);
		buttonPressed = AnyButtonPress (TRUE);
	} 

	if (resetInput)
		FlushInput ();

	return !buttonPressed;
}

BOOLEAN
WaitForNoInput (TimePeriod duration, BOOLEAN resetInput)
{
	TimeCount timeOut = duration;
	if (duration != WAIT_INFINITE)
		timeOut += GetTimeCounter ();
	return WaitForNoInputUntil (timeOut, resetInput);
}

// Stops game clock and music thread and minimizes interrupts/cycles
//  based on value of global GameActive variable
// See similar sleep state for main thread in uqm.c:main()
void
SleepGame (void)
{
	if (QuitPosted)
		return; // Do not sleep the game when already asked to quit

	log_add (log_Debug, "Game is going to sleep");

	if (PlayingTrack ())
		PauseTrack ();
	PauseMusic ();


	while (!GameActive && !QuitPosted)
		SleepThread (ONE_SECOND / 2);

	log_add (log_Debug, "Game is waking up");

	WaitForNoInput (ONE_SECOND / 10, TRUE);

	ResumeMusic ();

	if (PlayingTrack ())
		ResumeTrack ();


	TaskSwitch ();
}

/* Returns the fuel requirement to get to Sol (in fuel units * 100)
 */
DWORD
get_fuel_to_sol (void)
{
	POINT pt;
	DWORD f;

	pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	
	pt.x -= SOL_X;
	pt.y -= SOL_Y;

	f = (DWORD)((long)pt.x * pt.x + (long)pt.y * pt.y);
	if (f == 0 || GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
		return 0;
	else
		return (square_root (f) + (FUEL_TANK_SCALE / 20));
}