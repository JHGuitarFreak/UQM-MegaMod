/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
 *
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

#ifndef UQM_SUPERMELEE_NETPLAY_NETMISC_H_
#define UQM_SUPERMELEE_NETPLAY_NETMISC_H_

typedef struct BattleStateData BattleStateData;

#include "netconnection.h"
#include "netstate.h"
#include "types.h"

#include "../../battle.h"
		// for BattleFrameCounter, BATTLE_FRAME_RATE

#if defined(__cplusplus)
extern "C" {
#endif

struct BattleStateData {
	NETCONNECTION_STATE_DATA_COMMON

	struct melee_state *meleeState;
	struct battlestate_struct *battleState;
	struct getmelee_struct *getMeleeState;
	BattleFrameCounter endFrameCount;
};


void NetMelee_connectCallback(NetConnection *conn);
void NetMelee_closeCallback(NetConnection *conn);
void NetMelee_errorCallback(NetConnection *conn,
		const NetConnectionError *error);

void NetMelee_reenterState_inSetup(NetConnection *conn);


// Returns true iff the connection is in a state where the confirmation
// handshake is meaningful. Right now this is only when we're in the
// pre-game setup menu.
static inline bool
handshakeMeaningful(NetState state) {
	return state == NetState_inSetup;
}

static inline bool
readyFlagsMeaningful(NetState state) {
	return state == NetState_init ||
			state == NetState_preBattle ||
			state == NetState_selectShip ||
			state == NetState_interBattle ||
			state == NetState_inBattle ||
			state == NetState_endingBattle ||
			state == NetState_endingBattle2;
}


#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NETMISC_H_ */


