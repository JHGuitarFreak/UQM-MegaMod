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

#include "battlecontrols.h"

#include "intel.h"
		// For computer_intelligence()
#include "tactrans.h"
		// For battleEndReady*
#include "init.h"
		// For NUM_PLAYERS
#include "libs/memlib.h"
		// For HMalloc(), HFree()
#ifdef NETPLAY
#	include "supermelee/netplay/netmelee.h"
#endif  /* NETPLAY */

InputContext *PlayerInput[NUM_PLAYERS];

BattleInputHandlers ComputerInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction) computer_intelligence,
	/* .selectShip     = */ (SelectShipFunction) selectShipComputer,
	/* .battleEndReady = */ (BattleEndReadyFunction) battleEndReadyComputer,
	/* .deleteContext  = */ InputContext_delete,
};

BattleInputHandlers HumanInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction) frameInputHuman,
	/* .selectShip     = */ (SelectShipFunction) selectShipHuman,
	/* .battleEndReady = */ (BattleEndReadyFunction) battleEndReadyHuman,
	/* .deleteContext  = */ InputContext_delete,
};

#ifdef NETPLAY
BattleInputHandlers NetworkInputHandlers = {
	/* .frameInput     = */ (BattleFrameInputFunction) networkBattleInput,
	/* .selectShip     = */ (SelectShipFunction) selectShipNetwork,
	/* .battleEndReady = */ (BattleEndReadyFunction) battleEndReadyNetwork,
	/* .deleteContext  = */ InputContext_delete,
};
#endif


void
InputContext_init (InputContext *context, BattleInputHandlers *handlers,
		COUNT playerNr)
{
	context->handlers = handlers;
	context->playerNr = playerNr;
}

void
InputContext_delete (InputContext *context)
{
	HFree (context);
}

ComputerInputContext *
ComputerInputContext_new (COUNT playerNr)
{
	ComputerInputContext *result = HMalloc (sizeof (ComputerInputContext));
	InputContext_init ((InputContext *) result,
			&ComputerInputHandlers, playerNr);
	return result;
}

HumanInputContext *
HumanInputContext_new (COUNT playerNr)
{
	HumanInputContext *result = HMalloc (sizeof (HumanInputContext));
	InputContext_init ((InputContext *) result,
			&HumanInputHandlers, playerNr);
	return result;
}

#ifdef NETPLAY
NetworkInputContext *
NetworkInputContext_new (COUNT playerNr)
{
	NetworkInputContext *result = HMalloc (sizeof (NetworkInputContext));
	InputContext_init ((InputContext *) result,
			&NetworkInputHandlers, playerNr);
	return result;
}
#endif


