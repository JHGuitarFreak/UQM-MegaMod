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

#if defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif

#include "battle.h"

#include "battlecontrols.h"
#include "controls.h"
#include "init.h"
#include "element.h"
#include "ship.h"
#include "process.h"
#include "tactrans.h"
		// for flee_preprocess()
#include "intel.h"
#ifdef NETPLAY
#	include "supermelee/netplay/netmelee.h"
#	ifdef NETPLAY_CHECKSUM
#		include "supermelee/netplay/checksum.h"
#	endif
#	include "supermelee/netplay/notifyall.h"
#endif
#include "supermelee/pickmele.h"
#include "resinst.h"
#include "nameref.h"
#include "setup.h"
#include "settings.h"
#include "sounds.h"
#include "libs/async.h"
#include "libs/graphics/gfx_common.h"
#include "libs/log.h"
#include "libs/mathlib.h"
#include "globdata.h"
#include "libs/input/sdl/vcontrol.h"


BYTE battle_counter[NUM_SIDES];
		// The number of ships still available for battle to each side.
		// A ship that has warped out is no longer available.
BOOLEAN instantVictory;
size_t battleInputOrder[NUM_SIDES];
		// Indices of the sides in the order their input is processed.
		// Network sides are last so that the sides will never be waiting
		// on eachother, and games with a 0 frame delay are theoretically
		// possible.
#ifdef NETPLAY
BattleFrameCounter battleFrameCount;
		// Used for synchronisation purposes during netplay.
#endif

BOOLEAN
RunAwayAllowed (void)
{
	return ((LOBYTE (GLOBAL (CurrentActivity)) == IN_ENCOUNTER
			|| LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
			&& (NOMAD || GET_GAME_STATE (STARBASE_AVAILABLE))
			&& (DIF_EASY || !GET_GAME_STATE (BOMB_CARRIER)));
}

static void
DoRunAway (STARSHIP *StarShipPtr)
{
	ELEMENT *ElementPtr;

	LockElement (StarShipPtr->hShip, &ElementPtr);
	if (GetPrimType (&DisplayArray[ElementPtr->PrimIndex]) == STAMP_PRIM
			&& ElementPtr->life_span == NORMAL_LIFE
			&& !(ElementPtr->state_flags & FINITE_LIFE)
			&& ElementPtr->mass_points != MAX_SHIP_MASS * 10
			&& !(ElementPtr->state_flags & APPEARING))
	{
		battle_counter[0]--;

		ElementPtr->turn_wait = 3;
		ElementPtr->thrust_wait = 4;
		ElementPtr->colorCycleIndex = 0;
		ElementPtr->preprocess_func = flee_preprocess;
		ElementPtr->mass_points = MAX_SHIP_MASS * 10;
		ZeroVelocityComponents (&ElementPtr->velocity);
		StarShipPtr->cur_status_flags &=
				~(SHIP_AT_MAX_SPEED | SHIP_BEYOND_MAX_SPEED);

		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
				BUILD_COLOR (MAKE_RGB15 (0x0B, 0x00, 0x00), 0x2E));
				// XXX: I think this is supposed to be the same as the
				// first entry of the color cycle table in flee_preeprocess,
				// but it is slightly different (0x0A as red value). - SvdB.
		SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMPFILL_PRIM);
	
		StarShipPtr->ship_input_state = 0;
	}
	UnlockElement (StarShipPtr->hShip);
}

static void
setupBattleInputOrder(void)
{
	size_t i;

#ifndef NETPLAY
	for (i = 0; i < NUM_SIDES; i++)
		battleInputOrder[i] = i;
#else
	int j;

	i = 0;
	// First put the locally controlled players in the array.
	for (j = 0; j < NUM_SIDES; j++) {
		if (!(PlayerControl[j] & NETWORK_CONTROL)) {
			battleInputOrder[i] = j;
			i++;
		}
	}
	
	// Next put the network controlled players in the array.
	for (j = 0; j < NUM_SIDES; j++) {
		if (PlayerControl[j] & NETWORK_CONTROL) {
			battleInputOrder[i] = j;
			i++;
		}
	}
#endif
}

BATTLE_INPUT_STATE
frameInputHuman (HumanInputContext *context, STARSHIP *StarShipPtr)
{
	(void) StarShipPtr;
#if defined(ANDROID) || defined(__ANDROID__)
	return CurrentInputToBattleInput(context->playerNr, StarShipPtr ? StarShipPtr->ShipFacing : -1);
#else
	return CurrentInputToBattleInput(context->playerNr, -1);
#endif
}

static void
ProcessInput (void)
{
	BOOLEAN CanRunAway;
	size_t sideI;

#ifdef NETPLAY
	netInput ();
#endif

	CanRunAway = RunAwayAllowed ();
		
	for (sideI = 0; sideI < NUM_SIDES; sideI++)
	{
		HSTARSHIP hBattleShip, hNextShip;
		size_t cur_player = battleInputOrder[sideI];

		for (hBattleShip = GetHeadLink (&race_q[cur_player]);
				hBattleShip != 0; hBattleShip = hNextShip)
		{
			BATTLE_INPUT_STATE InputState;
			STARSHIP *StarShipPtr;

			StarShipPtr = LockStarShip (&race_q[cur_player], hBattleShip);
			hNextShip = _GetSuccLink (StarShipPtr);

			if (StarShipPtr->hShip)
			{
				// TODO: review and see if we have to do this every frame, or
				//   if we can do this once somewhere
				StarShipPtr->control = PlayerControl[cur_player];
				
				InputState = PlayerInput[cur_player]->handlers->frameInput (
						PlayerInput[cur_player], StarShipPtr);

#if CREATE_JOURNAL
				JournalInput (InputState);
#endif /* CREATE_JOURNAL */
#ifdef NETPLAY
				if (!(PlayerControl[cur_player] & NETWORK_CONTROL))
				{
					BattleInputBuffer *bib = getBattleInputBuffer(cur_player);
					Netplay_NotifyAll_battleInput (InputState);
					flushPacketQueues ();

					BattleInputBuffer_push (bib, InputState);
							// Add this input to the end of the buffer.
					BattleInputBuffer_pop (bib, &InputState);
							// Get the input from the front of the buffer.
				}
#endif

				StarShipPtr->ship_input_state = 0;
				if (StarShipPtr->RaceDescPtr->ship_info.crew_level)
				{
					if (InputState & BATTLE_LEFT)
						StarShipPtr->ship_input_state |= LEFT;
					else if (InputState & BATTLE_RIGHT)
						StarShipPtr->ship_input_state |= RIGHT;
					if (InputState & BATTLE_THRUST)
						StarShipPtr->ship_input_state |= THRUST;
					if (InputState & BATTLE_WEAPON)
						StarShipPtr->ship_input_state |= WEAPON;
					if (InputState & BATTLE_SPECIAL)
						StarShipPtr->ship_input_state |= SPECIAL;

					if (CanRunAway && cur_player == 0 &&
						((InputState & BATTLE_ESCAPE) || WarpFromMenu))
					{
						WarpFromMenu = FALSE;
						DoRunAway(StarShipPtr);
					}
				}
			}

			UnlockStarShip (&race_q[cur_player], hBattleShip);
		}
	}
	
#ifdef NETPLAY
	flushPacketQueues ();
#endif

	if (GLOBAL (CurrentActivity) & (CHECK_LOAD | CHECK_ABORT))
		GLOBAL (CurrentActivity) &= ~IN_BATTLE;
}

#if DEMO_MODE || CREATE_JOURNAL
DWORD BattleSeed;
#endif /* DEMO_MODE */

static MUSIC_REF BattleRef;

void
BattleSong (BOOLEAN DoPlay)
{
	if (BattleRef == 0)
	{
		if (inHyperSpace ())
			BattleRef = LoadMusic (HYPERSPACE_MUSIC);
		else if (inQuasiSpace ())
			BattleRef = LoadMusic (QUASISPACE_MUSIC);
		else {
			if(LOBYTE(GLOBAL(CurrentActivity)) == IN_LAST_BATTLE && !optRemixMusic)
				BattleRef = LoadMusic(BATTLE_MUSIC_SAMATRA);
			else
				BattleRef = LoadMusic(BATTLE_MUSIC);
		}
	}

	if (DoPlay)
		PlayMusic (BattleRef, TRUE, 1);
}

void
FreeBattleSong (void)
{
	DestroyMusic (BattleRef);
	BattleRef = 0;
}

static BOOLEAN
DoBattle (BATTLE_STATE *bs)
{
	extern UWORD nth_frame;
	RECT r;
	BYTE battle_speed;

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);

#if defined (NETPLAY) && defined (NETPLAY_CHECKSUM)
	if (getNumNetConnections() > 0 &&
			battleFrameCount % NETPLAY_CHECKSUM_INTERVAL == 0)
	{
		crc_State state;
		Checksum checksum;

		crc_init(&state);
		crc_processState (&state);
		checksum = (Checksum) crc_finish (&state);

		Netplay_NotifyAll_checksum ((uint32) battleFrameCount,
				(uint32) checksum);
		flushPacketQueues ();
		addLocalChecksum (battleFrameCount, checksum);
	}
#endif
	ProcessInput ();
			// Also calls NetInput()
#if defined (NETPLAY) && defined (NETPLAY_CHECKSUM)
	if (getNumNetConnections() > 0)
	{
		size_t delay = getBattleInputDelay();

		if (battleFrameCount >= delay
				&& (battleFrameCount - delay) % NETPLAY_CHECKSUM_INTERVAL == 0)
		{
			if (!(GLOBAL (CurrentActivity) & CHECK_ABORT))
			{
				if (!verifyChecksums (battleFrameCount - delay)) {
					GLOBAL(CurrentActivity) |= CHECK_ABORT;
					resetConnections (ResetReason_syncLoss);
				}
			}
		}
	}
#endif

	if (bs->first_time)
	{
		r.corner.x = SIS_ORG_X;
		r.corner.y = SIS_ORG_Y;
		r.extent.width = SIS_SCREEN_WIDTH;
		r.extent.height = SIS_SCREEN_HEIGHT;
		SetTransitionSource (&r);
	}
	BatchGraphics ();

	// Call the callback function, if set
	if (bs->frame_cb)
		bs->frame_cb ();

	RedrawQueue (TRUE);

	if (bs->first_time)
	{
		bs->first_time = FALSE;
		ScreenTransition (3, &r);
	}
	UnbatchGraphics ();
	if ((!(GLOBAL (CurrentActivity) & IN_BATTLE)) ||
			(GLOBAL (CurrentActivity) & (CHECK_ABORT | CHECK_LOAD)))
	{
		return FALSE;
	}

	battle_speed = HIBYTE (nth_frame);
	if (battle_speed == (BYTE)~0)
	{	// maximum speed, nothing rendered at all
		Async_process ();
		TaskSwitch ();
	}
	else
	{
		SleepThreadUntil (bs->NextTime
				+ BATTLE_FRAME_RATE / (battle_speed + 1));
		bs->NextTime = GetTimeCounter ();
	}

	if ((GLOBAL (CurrentActivity) & IN_BATTLE) == 0)
		return FALSE;

#ifdef NETPLAY
	battleFrameCount++;
#endif
	return TRUE;
}

#ifdef NETPLAY
COUNT
GetPlayerOrder (COUNT i)
{
	// Iff 'myTurn' is set on a connection, the local party will be
	// processed first.
	// If neither is network controlled, the top player (1) is handled
	// first.
	if (((PlayerControl[0] & NETWORK_CONTROL) &&
			!NetConnection_getDiscriminant (netConnections[0])) ||
			((PlayerControl[1] & NETWORK_CONTROL) &&
			NetConnection_getDiscriminant (netConnections[1])))
		return i;
	else
		return 1 - i;
}
#endif

// Let each player pick his ship.
static BOOLEAN
selectAllShips (SIZE num_ships)
{
	if (num_ships == 1) {
		// HyperSpace in full game.
		return GetNextStarShip (NULL, 0);
	}
	
#ifdef NETPLAY
	if ((PlayerControl[0] & NETWORK_CONTROL) &&
			(PlayerControl[1] & NETWORK_CONTROL))
	{
		log_add (log_Error, "Only one side at a time can be network "
				"controlled.\n");
		return FALSE;
	}
#endif

	return GetInitialStarShips ();
}

BOOLEAN
Battle (BattleFrameCallback *callback)
{
	SIZE num_ships;

#if defined(ANDROID) || defined(__ANDROID__)
	TFB_SetOnScreenKeyboard_Melee();
	if (PlayerControl[1] & HUMAN_CONTROL) {
		TFB_SetOnScreenKeyboard_TwoPlayersMelee();
	}
#endif

#if !(DEMO_MODE || CREATE_JOURNAL)
	if (LOBYTE (GLOBAL (CurrentActivity)) != SUPER_MELEE) {
		// In Supermelee, the RNG is already initialised.
		TFB_SeedRandom (GetTimeCounter ());
	}
#else /* DEMO_MODE */
	if (BattleSeed == 0)
		BattleSeed = TFB_Random ();
	TFB_SeedRandom (BattleSeed);
	BattleSeed = TFB_Random (); /* get next battle seed */
#endif /* DEMO_MODE */

	BattleSong (FALSE);
	
	num_ships = InitShips ();

	if (instantVictory)
	{
		num_ships = 0;
		battle_counter[0] = 1;
		battle_counter[1] = 0;
		instantVictory = FALSE;
	}
	
	if (num_ships)
	{
		BATTLE_STATE bs;

		GLOBAL (CurrentActivity) |= IN_BATTLE;
		battle_counter[0] = CountLinks (&race_q[0]);
		battle_counter[1] = CountLinks (&race_q[1]);

		if (optMeleeScale != TFB_SCALE_STEP)
			SetGraphicScaleMode (optMeleeScale);

		setupBattleInputOrder ();
#ifdef NETPLAY
		initBattleInputBuffers ();
#ifdef NETPLAY_CHECKSUM
		initChecksumBuffers ();
#endif  /* NETPLAY_CHECKSUM */
		battleFrameCount = 0;
		ResetWinnerStarShip ();
		setBattleStateConnections (&bs);
#endif  /* NETPLAY */

		if (!selectAllShips (num_ships)) {
			GLOBAL (CurrentActivity) |= CHECK_ABORT;
			goto AbortBattle;
		}

		BattleSong (TRUE);
		bs.NextTime = 0;
#ifdef NETPLAY
		initBattleStateDataConnections ();
		{
			bool allOk = negotiateReadyConnections (true, NetState_inBattle);
			if (!allOk) {
				GLOBAL (CurrentActivity) |= CHECK_ABORT;
				goto AbortBattle;
			}
		}
#endif  /* NETPLAY */
		bs.InputFunc = DoBattle;
		bs.frame_cb = callback;
		bs.first_time = inHQSpace ();

		DoInput (&bs, FALSE);

AbortBattle:
		if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
		{
			if (GLOBAL (CurrentActivity) & CHECK_ABORT)
			{
				// Do not return to the main menu when a game is aborted,
				// (just to the supermelee menu).
#ifdef NETPLAY
				waitResetConnections(NetState_inSetup);
						// A connection may already be in inSetup (set from
						// GetMeleeStarship). This is not a problem, although
						// it will generate a warning in debug mode.
#endif

				GLOBAL (CurrentActivity) &= ~CHECK_ABORT;
			}
			else
			{
				// Show the result of the battle.
				MeleeGameOver ();
			}
		}

#ifdef NETPLAY
		uninitBattleInputBuffers();
#ifdef NETPLAY_CHECKSUM
		uninitChecksumBuffers ();
#endif  /* NETPLAY_CHECKSUM */
		setBattleStateConnections (NULL);
#endif  /* NETPLAY */

		StopDitty ();
		StopMusic ();
		StopSound ();
	}

	UninitShips ();
	FreeBattleSong ();

#if defined(ANDROID) || defined(__ANDROID__)
	TFB_SetOnScreenKeyboard_Menu();
#endif
	
	return (BOOLEAN) (num_ships < 0);
}

