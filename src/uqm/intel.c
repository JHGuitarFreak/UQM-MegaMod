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

#include "intel.h"

#include "battlecontrols.h"
#include "controls.h"
#include "globdata.h"
#include "setup.h"
#include "libs/log.h"

#include <stdio.h>


BATTLE_INPUT_STATE
computer_intelligence (ComputerInputContext *context, STARSHIP *StarShipPtr)
{
	BATTLE_INPUT_STATE InputState;

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_LAST_BATTLE)
		return 0;

	if (StarShipPtr)
	{
		// Selecting the next action for in battle.
		if (StarShipPtr->control & CYBORG_CONTROL)
		{
			InputState = tactical_intelligence (context, StarShipPtr);

			// Allow a player to warp-escape in cyborg mode
			if (StarShipPtr->playerNr == RPG_PLAYER_NUM)
				InputState |= CurrentInputToBattleInput (context->playerNr, -1) & BATTLE_ESCAPE;
		}
		else {
			InputState = CurrentInputToBattleInput (context->playerNr, -1);
		}
	}
	else if (!(PlayerControl[context->playerNr] & PSYTRON_CONTROL))
		InputState = 0;
	else
	{
		switch (LOBYTE (GLOBAL (CurrentActivity)))
		{
			case SUPER_MELEE:
			{
				SleepThread (ONE_SECOND >> 1);
				InputState = BATTLE_WEAPON; /* pick a random ship */
				break;
			}
			default:
				// Should not happen. Satisfying compiler.
				log_add (log_Warning, "Warning: Unexpected state in "
						"computer_intelligence().");
				InputState = 0;
				break;
		}
	}
	return InputState;
}