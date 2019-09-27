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

#include <stdlib.h>

#include "comm.h"
#include "battle.h"
#include "fmv.h"
#include "gameev.h"
#include "types.h"
#include "globdata.h"
#include "resinst.h"
#include "restart.h"
#include "starbase.h"
#include "save.h"
#include "setup.h"
#include "master.h"
#include "controls.h"
#include "starcon.h"
#include "clock.h"
		// for GameClockTick()
#include "hyper.h"
		// for SeedUniverse()
#include "planets/planets.h"
		// for ExploreSolarSys()
#include "uqmdebug.h"
#include "uqm/lua/luastate.h"
#include "libs/tasklib.h"
#include "libs/log.h"
#include "libs/gfxlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/misc.h"
#include "libs/scriptlib.h"
#include "build.h"
#include "uqmversion.h"
#include "options.h"
#include "setupmenu.h"
#include "build.h"
#include "gameopt.h" // JMS: For naming captain and ship at game start.
#include "nameref.h"
#include "settings.h"
#include "cons_res.h"
#include <time.h>//required to use 'srand(time(NULL))'

volatile int MainExited = FALSE;
#ifdef DEBUG_SLEEP
uint32 mainThreadId;
extern uint32 SDL_ThreadID(void);
#endif

// Open or close the periodically occuring QuasiSpace portal.
// It changes the appearant portal size when necessary.
static void
checkArilouGate (void)
{
	BYTE counter;

	counter = GET_GAME_STATE (ARILOU_SPACE_COUNTER);
	if (GET_GAME_STATE (ARILOU_SPACE) == OPENING)
	{	// The portal is opening or fully open
		if (counter < 9)
			++counter;
	}
	else
	{	// The portal is closing or fully closed
		if (counter > 0)
			--counter;
	}
	SET_GAME_STATE (ARILOU_SPACE_COUNTER, counter);
}

// Battle frame callback function.
static void
on_battle_frame (void)
{
	GameClockTick ();
	checkArilouGate ();

	if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))){
		SeedUniverse ();
	}

	DrawAutoPilotMessage (FALSE);
}

static void
BackgroundInitKernel (DWORD TimeOut)
{
	LoadMasterShipList (TaskSwitch);
	TaskSwitch ();
	InitGameKernel ();

	while ((GetTimeCounter () <= TimeOut) &&
	       !(GLOBAL (CurrentActivity) & CHECK_ABORT))
	{
		UpdateInputState ();
		TaskSwitch ();
	}
}

// Executes on the main() thread
void
SignalStopMainThread (void)
{
	GamePaused = FALSE;
	GLOBAL (CurrentActivity) |= CHECK_ABORT;
	TaskSwitch ();
}

// Executes on the main() thread
void
ProcessUtilityKeys (void)
{
	if (ImmediateInputState.menu[KEY_ABORT])
	{
		log_showBox (false, false);
		exit (EXIT_SUCCESS);
	}
	
	if (ImmediateInputState.menu[KEY_FULLSCREEN])
	{
		int flags = GfxFlags ^ TFB_GFXFLAGS_FULLSCREEN;
		// clear ImmediateInputState so we don't repeat this next frame
		FlushInput ();
		TFB_DrawScreen_ReinitVideo (GraphicsDriver, flags, ScreenWidthActual,
				ScreenHeightActual);
	}

#if defined(DEBUG) || defined(USE_DEBUG_KEY)
	{	// Only call the debug func on the rising edge of
		// ImmediateInputState[KEY_DEBUG] so it does not execute repeatedly.
		// This duplicates the PulsedInputState somewhat, but we cannot
		// use PulsedInputState here because it is meant for another thread.
		static int debugKeyState;

		if (ImmediateInputState.menu[KEY_DEBUG] && debugKeyState == 0)
		{
			debugKeyPressed ();
		}
		debugKeyState = ImmediateInputState.menu[KEY_DEBUG];
	}
#endif  /* DEBUG */
}

/* TODO: Remove these declarations once threading is gone. */
extern int snddriver, soundflags;

int
Starcon2Main (void *threadArg)
{
#ifdef DEBUG_SLEEP
	mainThreadId = SDL_ThreadID();
#endif

#if CREATE_JOURNAL
{
int ac = argc;
char **av = argv;

while (--ac > 0)
{
	++av;
	if ((*av)[0] == '-')
	{
		switch ((*av)[1])
		{
#if CREATE_JOURNAL
			case 'j':
				++create_journal;
				break;
#endif //CREATE_JOURNAL
		}
	}
}
}
#endif // CREATE_JOURNAL

	{
		/* TODO: Put initAudio back in main where it belongs once threading
		 *       is gone.
		 */
		extern sint32 initAudio (sint32 driver, sint32 flags);
		initAudio (snddriver, soundflags);
	}

	if (!LoadKernel (0,0, FALSE))
	{
		log_add (log_Fatal, "\n  *** FATAL ERROR: Could not load basic content ***\n\nUQM requires at least the base content pack to run properly.");
		log_add (log_Fatal, "This file is typically called uqm-%d.%d.0.%g-content.uqm.  UQM was expecting", UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION);
		log_add (log_Fatal, "it in the %s/packages directory.", baseContentPath);
		log_add (log_Fatal, "Either your installation did not install the content pack at all, or it\ninstalled it in a different directory.\n\nFix your installation and rerun UQM.\n\n  *******************\n");
		log_showBox (true, true);

		MainExited = TRUE;
		return EXIT_FAILURE;
	}
	log_add (log_Info, "We've loaded the Kernel");

	GLOBAL (CurrentActivity) = 0;
	luaUqm_initState ();
	// show logo then splash and init the kernel in the meantime
	if(!optSkipIntro)
		Logo ();

	{
		srand(time(NULL));
		
		Rando = (rand() % NUM_MM_THEMES);

		// printf("Random Music #: %d\n", Rando);

		FadeMusic(0,0);
		PlayMusic (loadMainMenuMusic(Rando), TRUE, 1);
		
		if (optMainMenuMusic)
			FadeMusic (NORMAL_VOLUME+70, ONE_SECOND * 3);
		comingFromInit = TRUE;
	}

	SplashScreen (BackgroundInitKernel);

#ifdef DEBUG
	printf("Set Seed: %d\n", optCustomSeed);
	printf("Set Difficulty: %s\n", DIF_STR(optDifficulty));
	printf("Set Extended: %s\n", EXT_STR(optExtended));
	printf("Set Nomad: %s\n\n", NOMAD_STR(optNomad));
#endif
	log_add(log_Info, "Set Seed: %d\n", optCustomSeed);
	log_add(log_Info, "Set Difficulty: %s\n", DIF_STR(optDifficulty));
	log_add(log_Info, "Set Extended: %s\n", EXT_STR(optExtended));
	log_add(log_Info, "Set Nomad: %s\n\n", NOMAD_STR(optNomad));

//	OpenJournal ();
	while (StartGame ())
	{
		// Initialise a new game
		if (!SetPlayerInputAll ()) {
			log_add (log_Fatal, "Could not set player input.");
			explode ();  // Does not return;
		}

		luaUqm_reinitState ();
		InitGameStructures ();
		InitGameClock ();
		initEventSystem ();
		AddInitialGameEvents();

		// JMS: Name Captain & Ship at start (not at loading old game).
		if (LastActivity == (CHECK_LOAD | CHECK_RESTART)){
			AskNameForCaptainAndShip();

#ifdef DEBUG
			printf("New Game Seed: %d\n", GLOBAL_SIS(Seed));
			printf("New Game Difficulty: %s\n", DIF_STR(GLOBAL_SIS(Difficulty)));
			printf("New Game Extended: %s\n", EXT_STR(GLOBAL_SIS(Extended)));
			printf("New Game Nomad: %s\n\n", NOMAD_STR(GLOBAL_SIS(Nomad)));
#endif
			log_add(log_Info, "New Game Seed: %d\n", GLOBAL_SIS(Seed));
			log_add(log_Info, "New Game Difficulty: %s\n", DIF_STR(GLOBAL_SIS(Difficulty)));
			log_add(log_Info, "New Game Extended: %s\n", EXT_STR(GLOBAL_SIS(Extended)));
			log_add(log_Info, "New Game Nomad: %s\n\n", NOMAD_STR(GLOBAL_SIS(Nomad)));
		}

#if defined(ANDROID) || defined(__ANDROID__)
		TFB_SetOnScreenKeyboard_Melee();
#endif
		do
		{

			if (debugHook != NULL)
			{
				void (*saveDebugHook) (void);
				saveDebugHook = debugHook;
				debugHook = NULL;
						// No further debugHook calls unless the called
						// function resets debugHook.
				(*saveDebugHook) ();
				continue;
			}

			SetStatusMessageMode (SMM_DEFAULT);

			if (!((GLOBAL (CurrentActivity) | NextActivity) & CHECK_LOAD))
				ZeroVelocityComponents (&GLOBAL (velocity));
					// not going into talking pet conversation
			else if (GLOBAL (CurrentActivity) & CHECK_LOAD)
				GLOBAL (CurrentActivity) = NextActivity;

			if ((GLOBAL (CurrentActivity) & START_ENCOUNTER)
					|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
			{
				if (GET_GAME_STATE (CHMMR_BOMB_STATE) == 2
						&& !GET_GAME_STATE (STARBASE_AVAILABLE))
				{	/* BGD mode */
					InstallBombAtEarth ();
				}
				else if (GET_GAME_STATE (GLOBAL_FLAGS_AND_DATA) == (BYTE)~0
						|| GET_GAME_STATE (CHMMR_BOMB_STATE) == 2)
				{
					GLOBAL (CurrentActivity) |= START_ENCOUNTER;
					VisitStarBase ();
				}
				else
				{
					GLOBAL (CurrentActivity) |= START_ENCOUNTER;
					RaceCommunication ();
				}

				if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
				{
					GLOBAL (CurrentActivity) &= ~START_ENCOUNTER;
					if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
						GLOBAL (CurrentActivity) |= START_INTERPLANETARY;
				}
			}
			else if (GLOBAL (CurrentActivity) & START_INTERPLANETARY)
			{
				GLOBAL (CurrentActivity) = MAKE_WORD (IN_INTERPLANETARY, 0);

				DrawAutoPilotMessage (TRUE);
				SetGameClockRate (INTERPLANETARY_CLOCK_RATE);
				ExploreSolarSys ();
			}
			else
			{
				// Entering HyperSpace or QuasiSpace.
				GLOBAL (CurrentActivity) = MAKE_WORD (IN_HYPERSPACE, 0);

				DrawAutoPilotMessage (TRUE);
				SetGameClockRate (HYPERSPACE_CLOCK_RATE);
				Battle (&on_battle_frame);
			}

			SetFlashRect (NULL);

			LastActivity = GLOBAL (CurrentActivity);

			if (!(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD))
					&& (LOBYTE (GLOBAL (CurrentActivity)) == WON_LAST_BATTLE
							// if died for some reason
					|| GLOBAL_SIS (CrewEnlisted) == (COUNT)~0))
			{
				if (GET_GAME_STATE(KOHR_AH_KILLED_ALL)) {
					InitCommunication(BLACKURQ_CONVERSATION);
					GameOver(DEATH_MARCH);
				}
				else if (GLOBAL (CurrentActivity) & CHECK_RESTART){
					// surrendered to Ur-Quan
					DeathBySurrender = TRUE;
					GLOBAL (CurrentActivity) &= ~CHECK_RESTART;
				}
				break;
			}
		} while (!(GLOBAL (CurrentActivity) & CHECK_ABORT));

		StopSound ();
		uninitEventSystem ();
		UninitGameClock ();
		UninitGameStructures ();
		ClearPlayerInputAll ();
	}
//	CloseJournal ();
	luaUqm_uninitState ();

	UninitGameKernel ();
	FreeMasterShipList ();
	FreeKernel ();

	log_showBox (false, false);
	MainExited = TRUE;

	(void) threadArg;  /* Satisfying compiler (unused parameter) */
	return 0;
}

