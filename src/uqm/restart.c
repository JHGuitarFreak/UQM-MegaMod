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

#include "restart.h"

#include "colors.h"
#include "controls.h"
#include "credits.h"
#include "starmap.h"
#include "fmv.h"
#include "menustat.h"
#include "gamestr.h"
#include "globdata.h"
#include "intel.h"
#include "supermelee/melee.h"
#include "resinst.h"
#include "nameref.h"
#include "save.h"
#include "settings.h"
#include "setup.h"
#include "sounds.h"
#include "setupmenu.h"
#include "util.h"
#include "starcon.h"
#include "uqmversion.h"
#include "libs/graphics/gfx_common.h"
#include "libs/inplib.h"
#include "libs/graphics/sdl/pure.h"
#include "libs/log.h"
#include "options.h"
#include "cons_res.h"
#include "build.h"

#include "libs/resource/stringbank.h"
// for StringBank_Create() & SplitString()

enum
{
	START_NEW_GAME = 0,
	LOAD_SAVED_GAME,
	PLAY_SUPER_MELEE,
	SETUP_GAME,
	QUIT_GAME,
	NUM_MENU_ELEMENTS
};

enum
{
	EASY_DIFF = 0,
	ORIGINAL_DIFF,
	HARD_DIFF
};

#define CHOOSER_X (SCREEN_WIDTH >> 1)
#define CHOOSER_Y ((SCREEN_HEIGHT >> 1) - RES_SCALE (12))

// Kruzen: Having this ref separated gains more control
// We can load and free it whenever we want and not rely on menu volume
MUSIC_REF menuMusic;

void
InitMenuMusic (void)
{
	if (optMainMenuMusic && !(menuMusic))
	{
		FadeMusic (MUTE_VOLUME, 0);
		menuMusic = loadMainMenuMusic (Rando);
		PlayMusic (menuMusic, TRUE, 1);
		
		if (OkayToResume ())
			SeekMusic (GetMusicPosition ());

		FadeMusic (NORMAL_VOLUME + 70, ONE_SECOND * 3);
	}
}

void
UninitMenuMusic (void)
{
	if (menuMusic)
	{
		SleepThreadUntil (FadeMusic (MUTE_VOLUME, ONE_SECOND));

		SetMusicPosition ();
		StopMusic ();
		DestroyMusic (menuMusic);
		menuMusic = 0;

		FadeMusic (NORMAL_VOLUME, 0);
	}
}

void
DrawToolTips (MENU_STATE *pMS, int answer)
{
	COUNT i;
	TEXT t;
	stringbank *bank = StringBank_Create ();
	const char *lines[30];
	int line_count;
	RECT r;
	STAMP s;

	SetContextFont (TinyFont);

	GetFrameRect (SetRelFrameIndex (pMS->CurFrame, 2), &r);
	r.corner.y += CHOOSER_Y + r.extent.height + RES_SCALE (1);

	s.frame = SetRelFrameIndex (pMS->CurFrame, 3);
	r.extent = GetFrameBounds (s.frame);
	r.corner.x = RES_SCALE (
		(RES_DESCALE (CanvasWidth) - RES_DESCALE (r.extent.width)) >> 1);
	s.origin = r.corner;
	DrawStamp (&s);

	SetContextForeGroundColor (BLACK_COLOR);

	t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 66 + answer);
	line_count = SplitString (t.pStr, '\n', 30, lines, bank);

	t.baseline.x = r.corner.x
		+ RES_SCALE (RES_DESCALE (r.extent.width) >> 1);
	t.baseline.y = r.corner.y + RES_SCALE (10)
			+ RES_SCALE (line_count < 2 ? 8 : (line_count > 2 ? 0 : 3));
	for (i = 0; i < line_count; i++)
	{
		t.pStr = lines[i];
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE (8);
	}

	StringBank_Free (bank);
}

static void
DrawDiffChooser (MENU_STATE *pMS, BYTE answer, BOOLEAN confirm)
{
	STAMP s;
	FONT oldFont;
	TEXT t;
	COUNT i;

	s.origin = MAKE_POINT (CHOOSER_X, CHOOSER_Y);
	s.frame = SetRelFrameIndex (pMS->CurFrame, 2);
	DrawStamp (&s);

	DrawToolTips (pMS, answer);

	oldFont = SetContextFont (MicroFont);

	t.align = ALIGN_CENTER;
	t.baseline.x = s.origin.x;
	t.baseline.y = s.origin.y - RES_SCALE (20);

	for (i = 0; i <= 2; i++)
	{
		t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 56
				+ (!i ? 1 : (i > 1 ? 2 : 0)));
		t.CharCount = (COUNT)utf8StringCount (t.pStr);

		SetContextForeGroundColor (
				i == answer ?
				(confirm ? MENU_BACKGROUND_COLOR
				: MENU_HIGHLIGHT_COLOR) : BLACK_COLOR
			);
		font_DrawText (&t);

		t.baseline.y += RES_SCALE (23);
	}

	SetContextFont (oldFont);
}

static BOOLEAN
DoDiffChooser (MENU_STATE *pMS)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	RECT oldRect;
	STAMP s;
	CONTEXT oldContext;
	BOOLEAN response = FALSE;
	BOOLEAN done = FALSE;
	BYTE a = 1;

	InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;
	LastInputTime = GetTimeCounter ();

	oldContext = SetContext (ScreenContext);
	GetContextClipRect (&oldRect);
	s = SaveContextFrame (NULL);

	DrawDiffChooser (pMS, a, FALSE);

	FlushGraphics ();
	FlushInput ();

	while (!done)
	{
		UpdateInputState ();

		if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{
			return FALSE;
		}
		else if (PulsedInputState.menu[KEY_MENU_SELECT])
		{
			done = TRUE;
			response = TRUE;
			DrawDiffChooser (pMS, a, TRUE);
			PlayMenuSound (MENU_SOUND_SUCCESS);
		}
		else if (PulsedInputState.menu[KEY_MENU_CANCEL]
				|| CurrentInputState.menu[KEY_EXIT])
		{
			done = TRUE;
			response = FALSE;
			DrawStamp (&s);
		}
		else if (PulsedInputState.menu[KEY_MENU_UP] ||
				PulsedInputState.menu[KEY_MENU_DOWN] ||
				PulsedInputState.menu[KEY_MENU_LEFT] ||
				PulsedInputState.menu[KEY_MENU_RIGHT])
		{
			BYTE NewState;

			NewState = a;
			if (PulsedInputState.menu[KEY_MENU_UP]
					|| PulsedInputState.menu[KEY_MENU_LEFT])
			{
				if (NewState == EASY_DIFF)
					NewState = HARD_DIFF;
				else
					--NewState;
			}
			else if (PulsedInputState.menu[KEY_MENU_DOWN]
					|| PulsedInputState.menu[KEY_MENU_RIGHT])
			{
				if (NewState == HARD_DIFF)
					NewState = EASY_DIFF;
				else
					++NewState;
			}
			if (NewState != a)
			{
				BatchGraphics ();
				DrawDiffChooser (pMS, NewState, FALSE);
				UnbatchGraphics ();
				a = NewState;
			}

			PlayMenuSound (MENU_SOUND_MOVE);

			LastInputTime = GetTimeCounter ();

		}
		else if (GetTimeCounter () - LastInputTime > InactTimeOut)
		{	// timed out
			GLOBAL (CurrentActivity) = (ACTIVITY)~0;
			done = TRUE;
			response = FALSE;
		}

		SleepThread (ONE_SECOND / 30);
	}

	if (response)
	{
		optDifficulty = (!a ? OPTVAL_EASY :
				(a > 1 ? OPTVAL_HARD : OPTVAL_NORMAL));
	}

	DestroyDrawable (ReleaseDrawable (s.frame));
	FlushInput ();
	
	SetContextClipRect (&oldRect);
	SetContext (oldContext);

	return response;
}

#define MAIN_TEXT_X (SCREEN_WIDTH >> 1)
#define MAIN_TEXT_Y (RES_SCALE (42) - DOS_NUM_SCL (20))

FRAME TextCache[5];

static void
InitPulseText (void)
{
	FRAME frame, OldFrame;
	SIZE leading;
	TEXT t;
	COUNT i;

	if (TextCache[0] != NULL)
		return;

	SetContextFont (SlabFont);
	SetContextBackGroundColor (BLACK_COLOR);
	SetContextForeGroundColor (WHITE_COLOR);
	GetContextFontLeading (&leading);

	t.baseline.x = MAIN_TEXT_X;
	t.baseline.y = MAIN_TEXT_Y;
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 69 + i);

		frame = CaptureDrawable (CreateDrawable (WANT_PIXMAP, SCREEN_WIDTH,
			SCREEN_HEIGHT, 1));
		SetFrameTransparentColor (frame, BLACK_COLOR);
		OldFrame = SetContextFGFrame (frame);
		ClearDrawable ();
		SetContextFGFrame (OldFrame);

		OldFrame = SetContextFGFrame (frame);
		font_DrawText (&t);
		SetContextFGFrame (OldFrame);

		TextCache[i] = frame;

		t.baseline.y += leading;
	}
}

// Draw the full restart menu. Nothing is done with selections.
static void
DrawRestartMenuGraphic (MENU_STATE *pMS)
{
	RECT r;
	STAMP s;
	TEXT t;
	UNICODE buf[64];
	COUNT i;
	SIZE leading;

	s.frame = pMS->CurFrame;
	GetFrameRect (s.frame, &r);
	s.origin.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	s.origin.y = (SCREEN_HEIGHT - r.extent.height) >> 1;
	
	SetContextBackGroundColor (BLACK_COLOR);
	BatchGraphics ();

	ClearDrawable ();
	FlushColorXForms ();
	DrawStamp (&s);

	s.frame = IncFrameIndex (pMS->CurFrame);
	DrawStamp (&s);

	SetContextFont (SlabFont);

	GetContextFontLeading (&leading);

	t.baseline.x = MAIN_TEXT_X;
	t.baseline.y = MAIN_TEXT_Y;
	t.align = ALIGN_CENTER;
	t.CharCount = (COUNT)~0;

	SetContextForeGroundColor (MAIN_MENU_TEXT_COLOR);

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		t.pStr = GAME_STRING (MAINMENU_STRING_BASE + 69 + i);
		font_DrawText (&t);
		t.baseline.y += leading;
	}

	// Put the version number in the bottom right corner.
	SetContextFont (TinyFont);
	SetContextForeGroundColor (WHITE_COLOR);
	snprintf (buf, sizeof (buf), "v%d.%d.%d %s",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
			RES_BOOL (UQM_EXTRA_VERSION, "HD " UQM_EXTRA_VERSION));
	t.pStr = buf;
	t.baseline.x = SCREEN_WIDTH - RES_SCALE (2);
	t.baseline.y = SCREEN_HEIGHT - RES_SCALE (2);
	t.align = ALIGN_RIGHT;
	font_DrawText (&t);

	// Put the main menu music credit in the bottom left corner.
	if (optMainMenuMusic)
	{
		snprintf (buf, sizeof (buf), "%s %s",
				GAME_STRING (MAINMENU_STRING_BASE + 61),
				GAME_STRING (MAINMENU_STRING_BASE + 62 + Rando));
		t.baseline.x = RES_SCALE (2);
		t.baseline.y = SCREEN_HEIGHT - RES_SCALE (2);
		t.align = ALIGN_LEFT;
		font_DrawText (&t);
	}

	UnbatchGraphics ();
}

static void
DrawRestartMenu (MENU_STATE *pMS, BYTE NewState, FRAME f)
{
	POINT origin;
	origin.x = 0;
	origin.y = 0;
	Flash_setOverlay (pMS->flashContext, &origin, TextCache[NewState], FALSE);

	(void)f; // Silence compiler warnings
}

static BOOLEAN
RestartMessage (void)
{
	if (!optRequiresRestart)
		return FALSE;

	SetFlashRect (NULL, FALSE);
	DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 35));
	// Got to restart -message
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	SetTransitionSource (NULL);
	SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
	GLOBAL (CurrentActivity) = CHECK_ABORT;
	restartGame = TRUE;
	return TRUE;
}

static BOOLEAN
DoRestart (MENU_STATE *pMS)
{
	static TimeCount LastInputTime;
	static TimeCount InactTimeOut;
	TimeCount TimeIn = GetTimeCounter ();

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (optWindowType < 2)
		optMeleeToolTips = (OPT_ENABLABLE)FALSE;
	
	if (optSuperMelee && !optLoadGame)
	{
		pMS->CurState = PLAY_SUPER_MELEE;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}
	else if (optLoadGame && !optSuperMelee)
	{
		pMS->CurState = LOAD_SAVED_GAME;
		PulsedInputState.menu[KEY_MENU_SELECT] = 65535;
	}

	if (pMS->Initialized && !(GLOBAL (CurrentActivity) & CHECK_ABORT))
		Flash_process (pMS->flashContext);

	if (!pMS->Initialized)
	{// Kruzen: too much trouble using this one. Better to just turn it off
		pMS->hMusic = 0;

		InitMenuMusic ();
		InitPulseText ();
		ResetMusicResume ();

		InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;

		pMS->flashContext = Flash_createOverlay (ScreenContext,
				NULL, NULL);
		Flash_setMergeFactors (pMS->flashContext, -3, 3, 16);
		Flash_setSpeed (pMS->flashContext, (6 * ONE_SECOND) / 14, 0,
				(6 * ONE_SECOND) / 14, 0);
		Flash_setFrameTime (pMS->flashContext, ONE_SECOND / 16);
		Flash_setState (pMS->flashContext, FlashState_fadeIn,
				(3 * ONE_SECOND) / 16);
		Flash_setPulseBox (pMS->flashContext, FALSE);

		DrawRestartMenu (pMS, pMS->CurState, NULL);
		Flash_start (pMS->flashContext);

		LastInputTime = GetTimeCounter ();
		pMS->Initialized = TRUE;

		SleepThreadUntil (FadeScreen (FadeAllToColor, ONE_SECOND / 2));
	}
	else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
	{
		SleepThreadUntil (
				FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		switch (pMS->CurState)
		{
			case START_NEW_GAME:
				if (optCustomSeed == 404)
				{
					SetFlashRect (NULL, FALSE);
					DoPopupWindow (
							GAME_STRING (MAINMENU_STRING_BASE + 65));
					// Got to restart -message
					SetMenuSounds (
							MENU_SOUND_UP | MENU_SOUND_DOWN,
							MENU_SOUND_SELECT);
					SetTransitionSource (NULL);
					SleepThreadUntil (
							FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
					GLOBAL (CurrentActivity) = CHECK_ABORT;
					restartGame = TRUE;
					break;
				}

				if (optDiffChooser == OPTVAL_IMPO)
				{
					Flash_pause (pMS->flashContext);
					Flash_setState (pMS->flashContext, FlashState_fadeIn,
						 (3 * ONE_SECOND) / 16);
					if (!DoDiffChooser (pMS))
					{
						LastInputTime = GetTimeCounter ();// if we timed out - don't start second credit roll
						if (GLOBAL (CurrentActivity) != (ACTIVITY)~0)// just declined
							Flash_continue (pMS->flashContext);
						return TRUE;
					}
					Flash_continue (pMS->flashContext);
				}
				LastActivity = CHECK_LOAD | CHECK_RESTART;
				GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
				break;
			case LOAD_SAVED_GAME:
				LastActivity = CHECK_LOAD;
				GLOBAL (CurrentActivity) = IN_INTERPLANETARY;
				optLoadGame = FALSE;
				break;
			case PLAY_SUPER_MELEE:
				GLOBAL (CurrentActivity) = SUPER_MELEE;
				optSuperMelee = FALSE;
				break;
			case SETUP_GAME:
				Flash_pause (pMS->flashContext);
				Flash_setState (pMS->flashContext, FlashState_fadeIn,
						(3 * ONE_SECOND) / 16);

				SetupMenu ();

				if (optRequiresReload)
					return FALSE;

				LastInputTime = GetTimeCounter ();
				InactTimeOut = (optMainMenuMusic ? 60 : 20) * ONE_SECOND;

				SetTransitionSource (NULL);
				BatchGraphics ();
				DrawRestartMenuGraphic (pMS);
				ScreenTransition (3, NULL);
				Flash_UpdateOriginal (pMS->flashContext);
				DrawRestartMenu (pMS, pMS->CurState, NULL);
				Flash_continue (pMS->flashContext);
				UnbatchGraphics ();

				RestartMessage ();

				return TRUE;
			case QUIT_GAME:
				SleepThreadUntil (
						FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
				GLOBAL (CurrentActivity) = CHECK_ABORT;
				break;
		}

		Flash_pause (pMS->flashContext);

		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_UP] ||
			PulsedInputState.menu[KEY_MENU_DOWN])
	{
		BYTE NewState;

		NewState = pMS->CurState;
		if (PulsedInputState.menu[KEY_MENU_UP])
		{
			if (NewState == START_NEW_GAME)
				NewState = QUIT_GAME;
			else
				--NewState;
		}
		else if (PulsedInputState.menu[KEY_MENU_DOWN])
		{
			if (NewState == QUIT_GAME)
				NewState = START_NEW_GAME;
			else
				++NewState;
		}
		if (NewState != pMS->CurState)
		{
			BatchGraphics ();
			DrawRestartMenu (pMS, NewState, NULL);
			UnbatchGraphics ();
			pMS->CurState = NewState;
		}

		LastInputTime = GetTimeCounter ();
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT] ||
			PulsedInputState.menu[KEY_MENU_RIGHT])
	{	// Does nothing, but counts as input for timeout purposes
		LastInputTime = GetTimeCounter ();
	}
	//else if (MouseButtonDown)
	//{
	//	Flash_pause (pMS->flashContext);
	//	DoPopupWindow (GAME_STRING (MAINMENU_STRING_BASE + 54));
	//			// Mouse not supported message
	//	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);

	//	SetTransitionSource (NULL);
	//	BatchGraphics ();
	//	DrawRestartMenuGraphic (pMS);
	//	ScreenTransition (3, NULL);
	//	DrawRestartMenu (pMS, pMS->CurState, NULL);
	//	Flash_continue (pMS->flashContext);
	//	UnbatchGraphics ();

	//	LastInputTime = GetTimeCounter ();
	//}
	else
	{	// No input received, check if timed out
		if (GetTimeCounter () - LastInputTime > InactTimeOut)
		{
			GLOBAL (CurrentActivity) = (ACTIVITY)~0;
			return FALSE;
		}
	}

	SleepThreadUntil (TimeIn + ONE_SECOND / 30);

	return TRUE;
}

static BOOLEAN
RestartMenu (MENU_STATE *pMS)
{
	TimeCount TimeOut;
	COUNT i;

	ReinitQueue (&race_q[0]);
	ReinitQueue (&race_q[1]);

	SetContext (ScreenContext);

	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0
			&& GET_GAME_STATE (UTWIG_BOMB_ON_SHIP)
			&& !GET_GAME_STATE (UTWIG_BOMB)
			&& DeathBySuicide)
	{	// player blew himself up with Utwig bomb
		SET_GAME_STATE (UTWIG_BOMB_ON_SHIP, 0);

		SleepThreadUntil (FadeScreen (FadeAllToWhite, ONE_SECOND / 8)
				+ ONE_SECOND / 60);
		SetContextBackGroundColor (WHITE_COLOR);

		ClearDrawable ();
		FlushColorXForms ();
		TimeOut = ONE_SECOND / 8;

		GLOBAL (CurrentActivity) = IN_ENCOUNTER;

		if (optGameOver)
			GameOver (SUICIDE);

		DeathBySuicide = FALSE;

		FreeGameData ();
		GLOBAL (CurrentActivity) = CHECK_ABORT;
	}
	else
	{
		TimeOut = ONE_SECOND / 2;

		if (GLOBAL_SIS (CrewEnlisted) == (COUNT)~0)
		{
			GLOBAL (CurrentActivity) = IN_ENCOUNTER;

			if (DeathByMelee)
			{
				if (optGameOver)
					GameOver (DIED_IN_BATTLE);
				DeathByMelee = FALSE;
			}
			else if (DeathBySurrender)
			{
				if (optGameOver)
					GameOver (SURRENDERED);
				DeathBySurrender = FALSE;
			}
		}

		if (LOBYTE (LastActivity) == WON_LAST_BATTLE)
		{
			GLOBAL (CurrentActivity) = WON_LAST_BATTLE;
			Victory ();
			Credits (TRUE);
		}

		FreeGameData ();
		GLOBAL (CurrentActivity) = CHECK_ABORT;
	}

	LastActivity = 0;
	NextActivity = 0;

	// TODO: This fade is not always necessary, especially after a splash
	//   screen. It only makes a user wait.
	// Kruzen: This fade is needed when going from SUPER-MELEE and LOAD menus
	// and when Skip Intro option is enabled, 3 second pause goes when
	// the player used the Utwig bomb
	SleepThreadUntil (FadeScreen (FadeAllToBlack, TimeOut));
	if (TimeOut == ONE_SECOND / 8)
		SleepThread (ONE_SECOND * 3);

	pMS->CurFrame = CaptureDrawable (LoadGraphic (RESTART_PMAP_ANIM));

	DrawRestartMenuGraphic (pMS);
	GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
	SetMenuSounds (MENU_SOUND_UP | MENU_SOUND_DOWN, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();
	DoInput (pMS, TRUE);
	
	if (!(optRequiresRestart || optRequiresReload))
		UninitMenuMusic ();

	Flash_terminate (pMS->flashContext);
	pMS->flashContext = 0;
	DestroyDrawable (ReleaseDrawable (pMS->CurFrame));
	pMS->CurFrame = 0;

	for (i = START_NEW_GAME; i < NUM_MENU_ELEMENTS; i++)
	{
		DestroyDrawable (ReleaseDrawable (TextCache[i]));
		TextCache[i] = 0;
	}

	if (optRequiresReload)
		Reload ();

	if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		return (FALSE); // timed out

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return (FALSE); // quit

	TimeOut = FadeScreen (FadeAllToBlack, ONE_SECOND / 2);
	
	SleepThreadUntil (TimeOut);
	FlushColorXForms ();

	SeedRandomNumbers ();

	return (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE);
}

static BOOLEAN
TryStartGame (void)
{
	MENU_STATE MenuState;

	LastActivity = GLOBAL (CurrentActivity);
	GLOBAL (CurrentActivity) = 0;

	memset (&MenuState, 0, sizeof (MenuState));
	MenuState.InputFunc = DoRestart;

	while (!RestartMenu (&MenuState))
	{	// spin until a game is started or loaded
		if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE &&
				!(GLOBAL (CurrentActivity) & CHECK_ABORT))
		{
			FreeGameData ();
			Melee ();
			MenuState.Initialized = FALSE;
		}
		else if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
		{	// timed out
			SleepThreadUntil (FadeScreen (FadeAllToBlack, ONE_SECOND / 2));
			return (FALSE);
		}
		else if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		{	// quit
			return (FALSE);
		}
	}

	return TRUE;
}

BOOLEAN
StartGame (void)
{
	do
	{
		while (!TryStartGame ())
		{
			if (GLOBAL (CurrentActivity) == (ACTIVITY)~0)
			{	// timed out
				GLOBAL (CurrentActivity) = 0;

				if (optRequiresRestart || optRequiresReload)
					optRequiresRestart = optRequiresReload = FALSE;
				else
				{
					SplashScreen (0);
					if (optWhichIntro == OPT_3DO)
						Drumall ();
					Credits (FALSE);
				}
			}

			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
				return (FALSE); // quit
		}

		if (LastActivity & CHECK_RESTART)
		{	// starting a new game
			if (!optSkipIntro)
				Introduction ();
		}
	
	} while (GLOBAL (CurrentActivity) & CHECK_ABORT);
	// Be sure to load the seed type from the settings into the state.
	SET_GAME_STATE (SEED_TYPE, optSeedType);
	// Make sure to reset the seed if prime game is called for.
	if (PrimeSeed)
		optCustomSeed = PrimeA;
#ifdef DEBUG_STARSEED
	fprintf(stderr, "StartGame called for %d mode with seed %d.\n",
			optSeedType, optCustomSeed);
#endif
	{
		extern STAR_DESC starmap_array[];
		extern const BYTE element_array[];
		extern const PlanetFrame planet_array[];
		extern POINT constell_array[];

		// We no longer make a global pointer to the static starmap,
		// we make our own global copy in static memory so it behaves
		// the same throughout the code but can be reset as needed.
		//
		// As a reminder, the array has three extra entries beyond
		// NUM_SOLAR_SYSTEMS and NUM_HYPER_VORTICES due to Arilou
		// Quasispace home and the two endpoint dummy systems used by
		// FindStar as a boundary.
		//
		// While the starseed init code should always force a
		// reset of the starmap_array, we will do it here because
		// paranoia is its own reward.
		COUNT i;
#ifdef DEBUG_STARSEED
		fprintf(stderr, "Initializing star_array, just in case...\n");
#endif
		for (i = 0; i < NUM_SOLAR_SYSTEMS + 1 + NUM_HYPER_VORTICES + 1 + 1; i++)
			star_array[i] = starmap_array[i];
		Elements = element_array;
		PlanData = planet_array;
		constel_array = constell_array;
	}
	PlayerControl[0] = HUMAN_CONTROL | STANDARD_RATING;
	PlayerControl[1] = COMPUTER_CONTROL | AWESOME_RATING;

	return (TRUE);
}

