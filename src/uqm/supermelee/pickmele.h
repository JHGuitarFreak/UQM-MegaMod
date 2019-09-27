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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UQM_SUPERMELEE_PICKMELE_H_
#define UQM_SUPERMELEE_PICKMELE_H_

typedef struct getmelee_struct GETMELEE_STATE;

#include "../races.h"
#include "../battlecontrols.h"
#include "meleesetup.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

void MeleeShipDeath (STARSHIP *);
void BuildPickMeleeFrame (void);
void DestroyPickMeleeFrame (void);
void FillPickMeleeFrame (MeleeSetup *setup);
void MeleeGameOver (void);
BOOLEAN GetInitialMeleeStarShips (HSTARSHIP *result);
BOOLEAN GetNextMeleeStarShip (COUNT which_player, HSTARSHIP *result);

bool updateMeleeSelection (GETMELEE_STATE *gms, COUNT player, COUNT ship);

BOOLEAN selectShipHuman (HumanInputContext *context, GETMELEE_STATE *gms);
BOOLEAN selectShipComputer (ComputerInputContext *context,
		GETMELEE_STATE *gms);
#ifdef NETPLAY
BOOLEAN selectShipNetwork (NetworkInputContext *context, GETMELEE_STATE *gms);
#endif  /* NETPLAY */

#if defined(__cplusplus)
}
#endif

#ifdef PICKMELE_INTERNAL

#include "../flash.h"
#include "libs/timelib.h"
#include "../init.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct getmelee_struct {
	BOOLEAN (*InputFunc) (struct getmelee_struct *pInputState);

	BOOLEAN Initialized;
	
	struct {
		TimeCount timeIn;
		HSTARSHIP hBattleShip;
				// Chosen ship.
		COUNT choice;
				// Index of chosen ship, or (COUNT) ~0 for random choice.

		COUNT row;
		COUNT col;
		COUNT ships_left;
				// Number of ships still available.
		COUNT randomIndex;
				// Pre-generated random number.
		BOOLEAN selecting;
				// Is this player selecting a ship?
		BOOLEAN done;
				// Has a selection been made for this player?
		FlashContext *flashContext;
				// Context for controlling the flash rectangle.
#ifdef NETPLAY
		BOOLEAN remoteSelected;
#endif
	} player[NUM_PLAYERS];
};

bool setShipSelected(GETMELEE_STATE *gms, COUNT playerI, COUNT choice,
		bool reportNetwork);

#if defined(__cplusplus)
}
#endif

#endif  /* PICKMELE_INTERNAL */

#endif  /* UQM_SUPERMELEE_PICKMELE_H_ */

