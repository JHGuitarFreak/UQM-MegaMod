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

#include "tactrans.h"

#include "battlecontrols.h"
#include "build.h"
#include "collide.h"
#include "globdata.h"
#include "element.h"
#include "ship.h"
#include "status.h"
#include "battle.h"
#include "init.h"
#include "supermelee/pickmele.h"
#ifdef NETPLAY
#	include "supermelee/netplay/netmelee.h"
#	include "supermelee/netplay/netmisc.h"
#	include "supermelee/netplay/notify.h"
#	include "supermelee/netplay/proto/ready.h"
#	include "supermelee/netplay/packet.h"
#	include "supermelee/netplay/packetq.h"
#endif
#include "races.h"
#include "encount.h"
#include "settings.h"
#include "sounds.h"
#include "libs/mathlib.h"


static void cleanup_dead_ship (ELEMENT *ElementPtr);

static BOOLEAN dittyIsPlaying;
static STARSHIP *winnerStarShip;
		// Indicates which ship is the winner of the current battle.
		// The winner will be last to pick the next ship.


BOOLEAN
OpponentAlive (STARSHIP *TestStarShipPtr)
{
	HELEMENT hElement, hSuccElement;

	for (hElement = GetHeadElement (); hElement; hElement = hSuccElement)
	{
		ELEMENT *ElementPtr;
		STARSHIP *StarShipPtr;

		LockElement (hElement, &ElementPtr);
		hSuccElement = GetSuccElement (ElementPtr);
		GetElementStarShip (ElementPtr, &StarShipPtr);
		UnlockElement (hElement);

		if (StarShipPtr && StarShipPtr != TestStarShipPtr
				&& StarShipPtr->RaceDescPtr->ship_info.crew_level == 0)
			return FALSE;
	}

	return TRUE;
}

static void
PlayDitty (STARSHIP *ship)
{
	PlayMusic (ship->RaceDescPtr->ship_data.victory_ditty, FALSE, 3);
	dittyIsPlaying = TRUE;
}

void
StopDitty (void)
{
	if (dittyIsPlaying)
		StopMusic ();
	dittyIsPlaying = FALSE;
}

static BOOLEAN
DittyPlaying (void)
{
	if (!dittyIsPlaying)
		return FALSE;

	dittyIsPlaying = PLRPlaying ((MUSIC_REF)~0);
	return dittyIsPlaying;
}

void
ResetWinnerStarShip (void)
{
	winnerStarShip = NULL;
}

#ifdef NETPLAY
static void
readyToEnd2Callback (NetConnection *conn, void *arg)
{
	NetConnection_setState (conn, NetState_endingBattle2);
	(void) arg;
}

static void
readyToEndCallback (NetConnection *conn, void *arg)
{
	// This callback function gets called from inside the function that
	// updates the frame counter, but this is not a problem as the
	// ending frame count will at least be 1 greater than the current
	// frame count.

	BattleStateData *battleStateData;
	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);

#ifdef NETPLAY_DEBUG
	fprintf (stderr, "Both sides are ready to end the battle; starting "
			"end-of-battle synchronisation.\n");
#endif
	NetConnection_setState (conn, NetState_endingBattle);
	if (battleFrameCount + 1 > battleStateData->endFrameCount)
		battleStateData->endFrameCount = battleFrameCount + 1;
	Netplay_Notify_frameCount (conn, battleFrameCount + 1);
			// The +1 is to ensure that after the remote side receives the
			// frame count it will still receive one more frame data packet,
			// so it will know in advance when the last frame data packet
			// will come so it won't block. It also ensures that the
			// local frame counter won't go past the sent number, which
			// could happen when the function triggering the call to this
			// function is the frame update function which might update
			// the frame counter one more time.
	flushPacketQueue (conn);
#ifdef NETPLAY_DEBUG
    fprintf (stderr, "NETPLAY: [%d] ==> Sent battleFrameCount %d.\n",
			NetConnection_getPlayerNr(conn), battleFrameCount + 1);
#endif
	Netplay_localReady(conn, readyToEnd2Callback, NULL, false);
	(void) arg;
}

/*
 * When one player's ship dies, there's a delay before the next ship
 * can be chosen. This time depends on the time the ditty is playing
 * and may differ for each side.
 * To synchronise the time, the following protocol is followed:
 * 1. (NetState_inBattle) The Ready protocol is used to let either
 *    party know when they're ready to stop the battle.
 * 2. (NetState_endingBattle) Each party sends the frame number of when
 *    it wants to end the battle, and continues until that point, where
 *    it waits until it has received the frame number of the other party.
 * 3. After a player has both sent and received a frame count, the
 *    simulation continues for each party, until the maximum of both
 *    frame counts has been achieved.
 * 4. The Ready protocol is used to let each side signal that it has
 *    reached the target frame count.
 * 5. The battle ends.
 */
static bool
readyForBattleEndPlayer (NetConnection *conn)
{
	BattleStateData *battleStateData;
	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);

	if (NetConnection_getState (conn) == NetState_interBattle ||
			NetConnection_getState (conn) == NetState_inSetup)
	{
		// This connection is already ready. The entire synchronisation
		// protocol has already been done for this connection.
		return true;
	}

	if (NetConnection_getState (conn) == NetState_inBattle)
	{
		if (Netplay_isLocalReady(conn))
		{
			// We've already sent notice that we are ready, but we're
			// still waiting for the other side to say it's ready too.
			return false;
		}

		// We haven't yet told the other side we're ready. We do so now.
		Netplay_localReady (conn, readyToEndCallback, NULL, true);
				// This may set the state to endingBattle.

		if (NetConnection_getState (conn) == NetState_inBattle)
			return false;
	}

	assert (NetConnection_getState (conn) == NetState_endingBattle ||
			NetConnection_getState (conn) == NetState_endingBattle2);
	
	// Keep the simulation going as long as the target frame count
	// hasn't been reached yet. Note that if the connection state is
	// NetState_endingBattle, then we haven't yet received the
	// remote frame count, so the target frame count may still rise.
	if (battleFrameCount < battleStateData->endFrameCount)
		return false;

	if (NetConnection_getState (conn) == NetState_endingBattle)
	{
		// We have reached the target frame count, but we don't know
		// the remote target frame count yet. So we wait until it has
		// come in.
		waitReady (conn);
		// TODO: check whether all connections are still connected.
		assert (NetConnection_getState (conn) == NetState_endingBattle2);

		// Continue the simulation if the battleFrameCount has gone up.
		if (battleFrameCount < battleStateData->endFrameCount)
			return false;
	}

	// We are ready and wait for the other party to become ready too.
	negotiateReady (conn, true, NetState_interBattle);

	return true;	
}
#endif

bool
battleEndReadyHuman (HumanInputContext *context)
{
	(void) context;
	return true;
}

bool
battleEndReadyComputer (ComputerInputContext *context)
{
	(void) context;
	return true;
}

#ifdef NETPLAY
bool
battleEndReadyNetwork (NetworkInputContext *context)
{
	return readyForBattleEndPlayer (netConnections[context->playerNr]);
}
#endif

// Returns true iff this side is ready to end the battle.
static inline bool
readyForBattleEnd (void)
{
#ifndef NETPLAY
#if DEMO_MODE
	// In Demo mode, the saved journal should be replayed with frame
	// accuracy. PLRPlaying () isn't consistent enough.
	return true;
#else  /* !DEMO_MODE */
	return !DittyPlaying ();
#endif  /* !DEMO_MODE */
#else  /* defined (NETPLAY) */
	int playerI;

	if (DittyPlaying ())
		return false;

	for (playerI = 0; playerI < NUM_PLAYERS; playerI++)
		if (!PlayerInput[playerI]->handlers->battleEndReady (
				PlayerInput[playerI]))
			return false;

	return true;
#endif  /* defined (NETPLAY) */
}

static void
preprocess_dead_ship (ELEMENT *DeadShipPtr)
{
	ProcessSound ((SOUND)~0, NULL);
	(void)DeadShipPtr; // unused argument
}

void
cleanup_dead_ship (ELEMENT *DeadShipPtr)
{
	STARSHIP *DeadStarShipPtr;

	ProcessSound ((SOUND)~0, NULL);

	GetElementStarShip (DeadShipPtr, &DeadStarShipPtr);
	{
		// Ship explosion has finished, or ship has just warped out
		// if DeadStarShipPtr->crew_level != 0
		BOOLEAN MusicStarted;
		HELEMENT hElement, hSuccElement;

		/* Record crew left after the battle */
		DeadStarShipPtr->crew_level =
				DeadStarShipPtr->RaceDescPtr->ship_info.crew_level;

		MusicStarted = FALSE;

		for (hElement = GetHeadElement (); hElement; hElement = hSuccElement)
		{
			ELEMENT *ElementPtr;
			STARSHIP *StarShipPtr;

			LockElement (hElement, &ElementPtr);
			hSuccElement = GetSuccElement (ElementPtr);
			GetElementStarShip (ElementPtr, &StarShipPtr);
					// Get the STARSHIP that this ELEMENT belongs to.

			if (StarShipPtr == DeadStarShipPtr)
			{
				// This element belongs to the dead ship; it may be the
				// ship's own element.
				SetElementStarShip (ElementPtr, 0);

				if (!(ElementPtr->state_flags & CREW_OBJECT)
						|| ElementPtr->preprocess_func != crew_preprocess)
				{
					// Set the element up for deletion.
					SetPrimType (&DisplayArray[ElementPtr->PrimIndex],
							NO_PRIM);
					ElementPtr->life_span = 0;
					ElementPtr->state_flags =
							NONSOLID | DISAPPEARING | FINITE_LIFE;
					ElementPtr->preprocess_func = 0;
					ElementPtr->postprocess_func = 0;
					ElementPtr->death_func = 0;
					ElementPtr->collision_func = 0;
				}
			}

			if (StarShipPtr
					&& (StarShipPtr->cur_status_flags & PLAY_VICTORY_DITTY))
			{
				// StarShipPtr points to the remaining ship.
				MusicStarted = TRUE;
				PlayDitty (StarShipPtr);
				StarShipPtr->cur_status_flags &= ~PLAY_VICTORY_DITTY;
			}

			UnlockElement (hElement);
		}

#define MIN_DITTY_FRAME_COUNT  ((ONE_SECOND * 3) / BATTLE_FRAME_RATE)
		// The ship will be "alive" for at least 2 more frames to make sure
		// the elements it owns (set up for deletion above) expire first.
		// Ditty does NOT play in the following circumstances:
		//  * The winning ship dies before the loser finishes exploding
		//  * At the moment the losing ship dies, the winner has started
		//    the warp out sequence
		DeadShipPtr->life_span = MusicStarted ? MIN_DITTY_FRAME_COUNT : 1;
		if (DeadStarShipPtr == winnerStarShip)
		{	// This ship died but won the battle. We need to keep it alive
			// longer than the dead opponent ship so that the winning player
			// picks last.
			DeadShipPtr->life_span = MIN_DITTY_FRAME_COUNT + 1;
		}
		DeadShipPtr->death_func = new_ship;
		DeadShipPtr->preprocess_func = preprocess_dead_ship;
		DeadShipPtr->state_flags &= ~DISAPPEARING;
		// XXX: this increment was originally done by another piece of code
		//   just below this one. I am almost sure it is not needed, but it
		//   keeps the original framecount.
		++DeadShipPtr->life_span;
		SetElementStarShip (DeadShipPtr, DeadStarShipPtr);
	}
}

static void
setMinShipLifeSpan (ELEMENT *ship, COUNT life_span)
{
	if (ship->death_func == new_ship)
	{	// The ship has finished exploding or warping out, and now
		// we can work with the remaining element
		assert (ship->state_flags & FINITE_LIFE);
		assert (!(ship->state_flags & DISAPPEARING));
		if (ship->life_span < life_span)
			ship->life_span = life_span;
	}
}

static void
setMinStarShipLifeSpan (STARSHIP *starShip, COUNT life_span)
{
	ELEMENT *ship;

	LockElement (starShip->hShip, &ship);
	setMinShipLifeSpan (ship, life_span);
	UnlockElement (starShip->hShip);
}

static void
checkOtherShipLifeSpan (ELEMENT *deadShip)
{
	STARSHIP *deadStarShip;

	GetElementStarShip (deadShip, &deadStarShip);

	if (winnerStarShip != NULL && deadStarShip != winnerStarShip
			&& winnerStarShip->RaceDescPtr->ship_info.crew_level == 0)
	{	// The opponent ship also died but won anyway (e.g. Glory device)
		// We need to keep the opponent ship alive longer so that the
		// winning player picks last.
		setMinStarShipLifeSpan (winnerStarShip, deadShip->life_span + 1);
	}
	else if (winnerStarShip == NULL)
	{	// Both died at the same time, or the loser has already expired
		HELEMENT hElement, hNextElement;

		// Find the other dead ship(s) and keep them alive for at least as
		// long as this ship.
		for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
		{
			ELEMENT *element;
			STARSHIP *starShip;

			LockElement (hElement, &element);
			hNextElement = GetSuccElement (element);
			GetElementStarShip (element, &starShip);
			
			if (starShip != NULL && element != deadShip
					&& starShip->RaceDescPtr->ship_info.crew_level == 0)
			{	// This is another dead ship
				setMinShipLifeSpan (element, deadShip->life_span);
			}

			UnlockElement (hElement);
		}
	}
}

// This function is called when dead ship element's life_span reaches 0
void
new_ship (ELEMENT *DeadShipPtr)
{
	STARSHIP *DeadStarShipPtr;

	GetElementStarShip (DeadShipPtr, &DeadStarShipPtr);

	if (!readyForBattleEnd ())
	{
		DeadShipPtr->state_flags &= ~DISAPPEARING;
		++DeadShipPtr->life_span;

		// Keep the winner alive longer, or in a simultaneous destruction
		// tie, keep the other dead ship alive so that readyForBattleEnd()
		// is called for only one ship at a time.
		// When a ship has been destroyed, each side of a network
		// connection waits until the other side is ready.
		// When two ships die at the same time, this is handled for one
		// ship after the other.
		checkOtherShipLifeSpan (DeadShipPtr);
		return;
	}

	// Once a ship is being picked, we do not care about the winner anymore
	winnerStarShip = NULL;

	{
		BOOLEAN RestartMusic;

		StopDitty ();
		StopMusic ();
		StopSound ();

		SetElementStarShip (DeadShipPtr, 0);
		RestartMusic = OpponentAlive (DeadStarShipPtr);

		free_ship (DeadStarShipPtr->RaceDescPtr, TRUE, TRUE);
		DeadStarShipPtr->RaceDescPtr = 0;
		
		// Graphics are batched while the draw queue is processed,
		// but we are going to draw the ship selection box now
		UnbatchGraphics ();

#ifdef NETPLAY
		initBattleStateDataConnections ();
		{
			bool allOk =
					negotiateReadyConnections (true, NetState_interBattle);
					// We are already in NetState_interBattle, but all
					// sides just need to pass this checkpoint before
					// going on.
			if (!allOk)
			{
				// Some network connection has been reset.
				GLOBAL (CurrentActivity) &= ~IN_BATTLE;
				BatchGraphics ();
				return;
			}
		}
#endif  /* NETPLAY */

		if (!FleetIsInfinite (DeadStarShipPtr->playerNr))
		{	// This may be a dead ship (crew_level == 0) or a warped out ship
			UpdateShipFragCrew (DeadStarShipPtr);
			// Deactivate the ship (cannot be selected)
			DeadStarShipPtr->SpeciesID = NO_ID;
		}

		if (GetNextStarShip (DeadStarShipPtr, DeadStarShipPtr->playerNr))
		{
#ifdef NETPLAY
			{
				bool allOk =
						negotiateReadyConnections (true, NetState_inBattle);
				if (!allOk)
				{
					// Some network connection has been reset.
					GLOBAL (CurrentActivity) &= ~IN_BATTLE;
					BatchGraphics ();
					return;
				}
			}
#endif
			if (RestartMusic)
				BattleSong (TRUE);
		}
		else if (battle_counter[0] == 0 || battle_counter[1] == 0)
		{
			// One player is out of ships. The battle is over.
			GLOBAL (CurrentActivity) &= ~IN_BATTLE;
		}
#ifdef NETPLAY
		else
		{
			// Battle has been aborted.
			GLOBAL (CurrentActivity) |= CHECK_ABORT;
		}
#endif
		BatchGraphics ();
	}
}

static void
explosion_preprocess (ELEMENT *ShipPtr)
{
	BYTE i;

	i = (NUM_EXPLOSION_FRAMES * 3) - ShipPtr->life_span;
	switch (i)
	{
		case 25:
			ShipPtr->preprocess_func = NULL;
		case 0:
		case 1:
		case 2:
		case 20:
		case 21:
		case 22:
		case 23:
		case 24:
			i = 1;
			break;
		case 3:
		case 4:
		case 5:
		case 18:
		case 19:
			i = 2;
			break;
		case 15:
			SetPrimType (&DisplayArray[ShipPtr->PrimIndex], NO_PRIM);
			ShipPtr->state_flags |= CHANGING;
		default:
			i = 3;
			break;
	}

	do
	{
		HELEMENT hElement;

		hElement = AllocElement ();
		if (hElement)
		{
			COUNT angle, dist;
			DWORD rand_val;
			ELEMENT *ElementPtr;
			extern FRAME explosion[];

			PutElement (hElement);
			LockElement (hElement, &ElementPtr);
			ElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
			ElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			ElementPtr->life_span = 9;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
			ElementPtr->current.image.farray = explosion;
			ElementPtr->current.image.frame = explosion[0];
			rand_val = TFB_Random ();
			angle = LOBYTE (HIWORD (rand_val));
			dist = DISPLAY_TO_WORLD (LOBYTE (LOWORD (rand_val)) % 8);
			if (HIBYTE (LOWORD (rand_val)) < 256 * 1 / 3)
				dist += DISPLAY_TO_WORLD (8);
			ElementPtr->current.location.x =
					ShipPtr->current.location.x + COSINE (angle, dist);
			ElementPtr->current.location.y =
					ShipPtr->current.location.y + SINE (angle, dist);
			ElementPtr->preprocess_func = animation_preprocess;
			rand_val = TFB_Random ();
			angle = LOBYTE (LOWORD (rand_val));
			dist = WORLD_TO_VELOCITY (
					DISPLAY_TO_WORLD (HIBYTE (LOWORD (rand_val)) % 5));
			SetVelocityComponents (&ElementPtr->velocity,
					COSINE (angle, dist), SINE (angle, dist));
			UnlockElement (hElement);
		}
	} while (--i);
}

void
StopAllBattleMusic (void)
{
	StopDitty ();
	StopMusic ();
}

STARSHIP *
FindAliveStarShip (ELEMENT *deadShip)
{
	STARSHIP *aliveShip = NULL;
	HELEMENT hElement, hNextElement;

	// Find the remaining ship, if any, and see if it is still alive.
	for (hElement = GetHeadElement (); hElement; hElement = hNextElement)
	{
		ELEMENT *ElementPtr;

		LockElement (hElement, &ElementPtr);
		if ((ElementPtr->state_flags & PLAYER_SHIP)
				&& ElementPtr != deadShip
						/* and not running away */
				&& ElementPtr->mass_points <= MAX_SHIP_MASS + 1)
		{
			GetElementStarShip (ElementPtr, &aliveShip);
			assert (aliveShip != NULL);
			if (aliveShip->RaceDescPtr->ship_info.crew_level == 0
					/* reincarnating Pkunk is not actually dead */
					&& ElementPtr->mass_points != MAX_SHIP_MASS + 1)
			{
				aliveShip = NULL;
			}

			UnlockElement (hElement);
			break;
		}
		hNextElement = GetSuccElement (ElementPtr);
		UnlockElement (hElement);
	}
	
	return aliveShip;
}

STARSHIP *
GetWinnerStarShip (void)
{
	return winnerStarShip;
}

void
SetWinnerStarShip (STARSHIP *winner)
{
	if (winner == NULL)
		return; // nothing to do
	
	winner->cur_status_flags |= PLAY_VICTORY_DITTY;

	// The winner is set once per battle. If both ships die, this function is
	// called twice, once for each ship. We need to preserve the winner
	// determined on the first call.
	if (winnerStarShip == NULL)
		winnerStarShip = winner;
}

void
RecordShipDeath (ELEMENT *deadShip)
{
	STARSHIP *deadStarShip;

	GetElementStarShip (deadShip, &deadStarShip);
	assert (deadStarShip != NULL);

	if (deadShip->mass_points <= MAX_SHIP_MASS)
	{	// Not running away.
		// When a ship tries to run away, it is (dis)counted in DoRunAway(),
		// so when it dies while running away, we will not count it again
		assert (deadStarShip->playerNr >= 0);
		battle_counter[deadStarShip->playerNr]--;
	}

	if (LOBYTE (GLOBAL (CurrentActivity)) == SUPER_MELEE)
		MeleeShipDeath (deadStarShip);
}

void
StartShipExplosion (ELEMENT *ShipPtr, bool playSound)
{
	STARSHIP *StarShipPtr;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	ZeroVelocityComponents (&ShipPtr->velocity);

	DeltaEnergy (ShipPtr,
			-(SIZE)StarShipPtr->RaceDescPtr->ship_info.energy_level);

	ShipPtr->life_span = NUM_EXPLOSION_FRAMES * 3;
	ShipPtr->state_flags &= ~DISAPPEARING;
	ShipPtr->state_flags |= FINITE_LIFE | NONSOLID;
	ShipPtr->preprocess_func = explosion_preprocess;
	ShipPtr->postprocess_func = PostProcessStatus;
	ShipPtr->death_func = cleanup_dead_ship;
	ShipPtr->hTarget = 0;

	if (playSound)
	{
		PlaySound (SetAbsSoundIndex (GameSounds, SHIP_EXPLODES),
				CalcSoundPosition (ShipPtr), ShipPtr, GAME_SOUND_PRIORITY + 1);
	}
}

void
ship_death (ELEMENT *ShipPtr)
{
	STARSHIP *StarShipPtr;
	STARSHIP *winner;

	GetElementStarShip (ShipPtr, &StarShipPtr);

	StopAllBattleMusic ();

	// If the winning ship dies before the ditty starts, do not play it.
	// e.g. a ship can die after the opponent begins exploding but
	// before the explosion is over.
	StarShipPtr->cur_status_flags &= ~PLAY_VICTORY_DITTY;

	StartShipExplosion (ShipPtr, true);

	winner = FindAliveStarShip (ShipPtr);
	SetWinnerStarShip (winner);
	RecordShipDeath (ShipPtr);
}

#define START_ION_COLOR BUILD_COLOR (MAKE_RGB15 (0x1F, 0x15, 0x00), 0x7A)

// Called from the death_func of an element for an ion trail pixel, or a
// ship shadow (when warping in/out).
static void
cycle_ion_trail (ELEMENT *ElementPtr)
{
	static const Color colorTab[] =
	{
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x15, 0x00), 0x7a),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x11, 0x00), 0x7b),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0E, 0x00), 0x7c),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x00), 0x7d),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x07, 0x00), 0x7e),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x03, 0x00), 0x7f),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x00, 0x00), 0x2a),
		BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2b),
		BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2c),
		BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2d),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0F, 0x00, 0x00), 0x2e),
		BUILD_COLOR (MAKE_RGB15_INIT (0x0B, 0x00, 0x00), 0x2f),
	};
	const size_t colorTabCount = sizeof colorTab / sizeof colorTab[0];
			
	assert (!(ElementPtr->state_flags & PLAYER_SHIP));

	ElementPtr->colorCycleIndex++;
	if (ElementPtr->colorCycleIndex != colorTabCount)
	{
		ElementPtr->life_span = ElementPtr->thrust_wait;
				// Reset the life span.
		
		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
				colorTab[ElementPtr->colorCycleIndex]);

		ElementPtr->state_flags &= ~DISAPPEARING;
		ElementPtr->state_flags |= CHANGING;
	} // else, the element disappears.
}

void
spawn_ion_trail (ELEMENT *ElementPtr, SIZE x_offset, SIZE y_offset)
{
	HELEMENT hIonElement;

	assert (ElementPtr->state_flags & PLAYER_SHIP);

	hIonElement = AllocElement ();
	if (hIonElement)
	{
#define ION_LIFE 1
		COUNT angle;
		RECT r;
		ELEMENT *IonElementPtr;
		STARSHIP *StarShipPtr;

		GetElementStarShip (ElementPtr, &StarShipPtr);
		angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing) + HALF_CIRCLE;
		GetFrameRect (StarShipPtr->RaceDescPtr->ship_data.ship[0], &r);
		r.extent.height = DISPLAY_TO_WORLD (r.extent.height + r.corner.y);

		InsertElement (hIonElement, GetHeadElement ());
		LockElement (hIonElement, &IonElementPtr);
		IonElementPtr->playerNr = NEUTRAL_PLAYER_NUM;
		IonElementPtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
		IonElementPtr->thrust_wait = ION_LIFE;
		IonElementPtr->life_span = IonElementPtr->thrust_wait;
				// When the element "dies", in the death_func
				// 'cycle_ion_trail', it is given new life a number of
				// times, by setting life_span to thrust_wait.

		// JMS_GFX
		if (!IS_HD) {
			SetPrimType (&DisplayArray[IonElementPtr->PrimIndex], POINT_PRIM);
			IonElementPtr->current.image.frame = DecFrameIndex (stars_in_space);
			IonElementPtr->current.image.farray = &stars_in_space;
		} else {
			SetPrimType (&DisplayArray[IonElementPtr->PrimIndex], STAMPFILL_PRIM);
			IonElementPtr->current.image.frame = SetAbsFrameIndex (ion_trails[0], 0);
			IonElementPtr->current.image.farray = ion_trails;
		}
		SetPrimColor (&DisplayArray[IonElementPtr->PrimIndex],
				START_ION_COLOR);
		IonElementPtr->colorCycleIndex = 0;
		/*IonElementPtr->current.image.frame =
				DecFrameIndex (stars_in_space);
		IonElementPtr->current.image.farray = &stars_in_space;*/
		IonElementPtr->current.location = ElementPtr->current.location;
		IonElementPtr->current.location.x +=
				(COORD)COSINE (angle, r.extent.height) + x_offset;
		IonElementPtr->current.location.y +=
				(COORD)SINE (angle, r.extent.height) + y_offset;
		IonElementPtr->death_func = cycle_ion_trail;

		if (IS_HD) {
			IonElementPtr->next.image.frame = IonElementPtr->current.image.frame;
			IonElementPtr->next.image.farray = IonElementPtr->current.image.farray;
		}

		SetElementStarShip (IonElementPtr, StarShipPtr);

		{
			/* normally done during preprocess, but because
			 * object is being inserted at head rather than
			 * appended after tail it may never get preprocessed.
			 */
			IonElementPtr->next = IonElementPtr->current;
			--IonElementPtr->life_span;
			IonElementPtr->state_flags |= PRE_PROCESS;
		}

		UnlockElement (hIonElement);
	}
}

// Preprocess function for spawning a ship into or out of battle.
// Used when a new ship warps in, or a ship escapes by warping out, but not
// when a Pkunk ship is reborn.
void
ship_transition (ELEMENT *ElementPtr)
{
	if (ElementPtr->state_flags & PLAYER_SHIP)
	{
		if (ElementPtr->state_flags & APPEARING)
		{
			ElementPtr->life_span = HYPERJUMP_LIFE;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = NULL;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
		}
		else if (ElementPtr->life_span < HYPERJUMP_LIFE)
		{
			if (ElementPtr->life_span == NORMAL_LIFE
					&& ElementPtr->crew_level)
			{
				ElementPtr->current.image.frame =
						ElementPtr->next.image.frame =
						SetEquFrameIndex (
						ElementPtr->current.image.farray[0],
						ElementPtr->current.image.frame);
				SetPrimType (&DisplayArray[ElementPtr->PrimIndex], STAMP_PRIM);
				InitIntersectStartPoint (ElementPtr);
				InitIntersectEndPoint (ElementPtr);
				InitIntersectFrame (ElementPtr);
				ZeroVelocityComponents (&ElementPtr->velocity);
				ElementPtr->state_flags &= ~(NONSOLID | FINITE_LIFE);
				ElementPtr->state_flags |= CHANGING;

				ElementPtr->preprocess_func = ship_preprocess;
				ElementPtr->postprocess_func = ship_postprocess;
			}

			return;
		}
	}

	{
		HELEMENT hShipImage;
		ELEMENT *ShipImagePtr;
		STARSHIP *StarShipPtr;
		
		GetElementStarShip (ElementPtr, &StarShipPtr);
		LockElement (StarShipPtr->hShip, &ShipImagePtr);

		if (!(ShipImagePtr->state_flags & NONSOLID))
		{
			ElementPtr->preprocess_func = NULL;
		}
		else if ((hShipImage = AllocElement ()))
		{
#define TRANSITION_SPEED DISPLAY_TO_WORLD ((RES_SCALE(40))) // JMS_GFX
#define TRANSITION_LIFE 1
			COUNT angle;

			PutElement (hShipImage);

			angle = FACING_TO_ANGLE (StarShipPtr->ShipFacing);

			LockElement (hShipImage, &ShipImagePtr);
			ShipImagePtr->playerNr = NEUTRAL_PLAYER_NUM;
			ShipImagePtr->state_flags = APPEARING | FINITE_LIFE | NONSOLID;
			ShipImagePtr->thrust_wait = TRANSITION_LIFE;
			ShipImagePtr->life_span = ShipImagePtr->thrust_wait;
					// When the element "dies", in the death_func
					// 'cycle_ion_trail', it is given new life a number of
					// times, by setting life_span to thrust_wait.
			SetPrimType (&DisplayArray[ShipImagePtr->PrimIndex],
					STAMPFILL_PRIM);
			SetPrimColor (&DisplayArray[ShipImagePtr->PrimIndex],
					START_ION_COLOR);
			ShipImagePtr->colorCycleIndex = 0;
			ShipImagePtr->current.image = ElementPtr->current.image;
			ShipImagePtr->current.location = ElementPtr->current.location;
			if (!(ElementPtr->state_flags & PLAYER_SHIP))
			{
				ShipImagePtr->current.location.x +=
						COSINE (angle, TRANSITION_SPEED);
				ShipImagePtr->current.location.y +=
						SINE (angle, TRANSITION_SPEED);
				ElementPtr->preprocess_func = NULL;
			}
			else if (ElementPtr->crew_level)
			{
				// JMS_GFX: Circumventing overflows by using temp variables instead of 
				// subtracting straight from the POINT sized ShipImagePtr->current.location.
				SDWORD temp_x = (SDWORD)ShipImagePtr->current.location.x -
					COSINE (angle, TRANSITION_SPEED) * (ElementPtr->life_span - 1);
				SDWORD temp_y = (SDWORD)ShipImagePtr->current.location.y -
					SINE (angle, TRANSITION_SPEED) * (ElementPtr->life_span - 1);
                
				ShipImagePtr->current.location.x = WRAP_X (temp_x);
				ShipImagePtr->current.location.y = WRAP_Y (temp_y);
			}
			ShipImagePtr->preprocess_func = ship_transition;
			ShipImagePtr->death_func = cycle_ion_trail;
			SetElementStarShip (ShipImagePtr, StarShipPtr);

			UnlockElement (hShipImage);
		}

		UnlockElement (StarShipPtr->hShip);
	}
}

void
flee_preprocess (ELEMENT *ElementPtr)
{
	STARSHIP *StarShipPtr;

	if (--ElementPtr->turn_wait == 0)
	{
		static const Color colorTab[] =
		{
			BUILD_COLOR (MAKE_RGB15_INIT (0x0A, 0x00, 0x00), 0x2E),
			BUILD_COLOR (MAKE_RGB15_INIT (0x0E, 0x00, 0x00), 0x2D),
			BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2C),
			BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2A),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x00, 0x00), 0x29),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x04, 0x04), 0x28),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x0A), 0x27),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0F, 0x0F), 0x26),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x13, 0x13), 0x25),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x19, 0x19), 0x24),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x13, 0x13), 0x25),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0F, 0x0F), 0x26),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x0A, 0x0A), 0x27),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x04, 0x04), 0x28),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1F, 0x00, 0x00), 0x29),
			BUILD_COLOR (MAKE_RGB15_INIT (0x1B, 0x00, 0x00), 0x2A),
			BUILD_COLOR (MAKE_RGB15_INIT (0x17, 0x00, 0x00), 0x2B),
			BUILD_COLOR (MAKE_RGB15_INIT (0x13, 0x00, 0x00), 0x2C),
			BUILD_COLOR (MAKE_RGB15_INIT (0x0E, 0x00, 0x00), 0x2D),
		};
		const size_t colorTabCount = sizeof colorTab / sizeof colorTab[0];

		ElementPtr->colorCycleIndex++;
		if (ElementPtr->colorCycleIndex == colorTabCount)
			ElementPtr->colorCycleIndex = 0;

		SetPrimColor (&DisplayArray[ElementPtr->PrimIndex],
				colorTab[ElementPtr->colorCycleIndex]);

		if (ElementPtr->colorCycleIndex == 0)
			--ElementPtr->thrust_wait;

		ElementPtr->turn_wait = ElementPtr->thrust_wait;
		if (ElementPtr->turn_wait)
		{
			ElementPtr->turn_wait = ((ElementPtr->turn_wait - 1) >> 1) + 1;
		}
		else if (ElementPtr->colorCycleIndex != (colorTabCount / 2))
		{
			ElementPtr->turn_wait = 1;
		}
		else
		{
			ElementPtr->death_func = cleanup_dead_ship;
			ElementPtr->crew_level = 0;

			ElementPtr->life_span = HYPERJUMP_LIFE + 1;
			ElementPtr->preprocess_func = ship_transition;
			ElementPtr->postprocess_func = NULL;
			SetPrimType (&DisplayArray[ElementPtr->PrimIndex], NO_PRIM);
			ElementPtr->state_flags |= NONSOLID | FINITE_LIFE | CHANGING;
		}
	}

	GetElementStarShip (ElementPtr, &StarShipPtr);
	StarShipPtr->cur_status_flags &=
			~(LEFT | RIGHT | THRUST | WEAPON | SPECIAL);
			// Ignore control input when fleeing.
	PreProcessStatus (ElementPtr);
}
