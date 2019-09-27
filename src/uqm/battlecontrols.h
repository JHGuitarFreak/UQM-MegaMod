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

#ifndef UQM_BATTLECONTROLS_H_
#define UQM_BATTLECONTROLS_H_

typedef struct BattleInputHandlers BattleInputHandlers;
typedef struct InputContext InputContext;
typedef struct ComputerInputContext ComputerInputContext;
typedef struct HumanInputContext HumanInputContext;
#ifdef NETPLAY
typedef struct NetworkInputContext NetworkInputContext;
#endif  /* NETPLAY */

#include "controls.h"
#include "supermelee/pickmele.h"
#include "races.h"
#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef BATTLE_INPUT_STATE (*BattleFrameInputFunction) (
		InputContext *context, STARSHIP *StarShipPtr);
typedef BOOLEAN (*SelectShipFunction) (InputContext *context,
		GETMELEE_STATE *gms);
typedef bool (*BattleEndReadyFunction) (InputContext *context);
typedef void (*DeleteInputContextFunction) (InputContext *context);


struct BattleInputHandlers {
	BattleFrameInputFunction frameInput;
	SelectShipFunction selectShip;
	BattleEndReadyFunction battleEndReady;
	DeleteInputContextFunction deleteContext;
};

#define INPUT_CONTEXT_COMMON \
		BattleInputHandlers *handlers; \
		COUNT playerNr;

// Base "class" for all ...InputContext structures
struct InputContext {
	INPUT_CONTEXT_COMMON
};

struct ComputerInputContext {
	INPUT_CONTEXT_COMMON
	// TODO: Put RNG Context used for the AI here.
};

struct HumanInputContext {
	INPUT_CONTEXT_COMMON
};

#ifdef NETPLAY
struct NetworkInputContext {
	INPUT_CONTEXT_COMMON
	// TODO: put NetworkConnection for this player here.
};
#endif  /* NETPLAY */

ComputerInputContext *ComputerInputContext_new (COUNT playerNr);
HumanInputContext *HumanInputContext_new (COUNT playerNr);
#ifdef NETPLAY
NetworkInputContext *NetworkInputContext_new (COUNT playerNr);
#endif  /* NETPLAY */

extern InputContext *PlayerInput[];


BATTLE_INPUT_STATE frameInputHuman (HumanInputContext *context,
		STARSHIP *StarShipPtr);
void InputContext_init(InputContext *context, BattleInputHandlers *handlers,
		COUNT playerNr);
void InputContext_delete (InputContext *context);
		// Do not call directly, only from the FreeInputContextFunction.
		// Call InputContext->handlers->freeContext() to release an
		// InputContext.

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_BATTLECONTROLS_H_ */


