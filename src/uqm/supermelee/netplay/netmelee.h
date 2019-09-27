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

#if !defined(UQM_SUPERMELEE_NETPLAY_NETMELEE_H_) && defined(NETPLAY)
#define UQM_SUPERMELEE_NETPLAY_NETMELEE_H_

#include "netplay.h"
#include "netinput.h"
#include "netconnection.h"
#include "packetsenders.h"

#include "../../battlecontrols.h"
		// for NetworkInputContext
#include "../../controls.h"
		// for BATTLE_INPUT_STATE
#include "../../races.h"
		// for STARSHIP

#if defined(__cplusplus)
extern "C" {
#endif

extern struct NetConnection *netConnections[];


void addNetConnection(NetConnection *conn, int playerNr);
void removeNetConnection(int playerNr);
void closeAllConnections(void);
void closeDisconnectedConnections(void);
size_t getNumNetConnections(void);
typedef bool(*ForEachConnectionCallback)(NetConnection *conn, void *arg);
bool forEachConnectedPlayer(ForEachConnectionCallback callback, void *arg);

struct melee_state *NetMelee_getMeleeState(NetConnection *conn);
struct battlestate_struct *NetMelee_getBattleState(NetConnection *conn);

void netInput(void);
void netInputBlocking(uint32 timeoutMs);
void flushPacketQueues(void);

void confirmConnections(void);
void cancelConfirmations(void);
void connectionsLocalReady(NetConnection_ReadyCallback callback, void *arg);

bool allConnected(void);

void initBattleStateDataConnections(void);
void setBattleStateConnections(struct battlestate_struct *bs);

BATTLE_INPUT_STATE networkBattleInput(NetworkInputContext *context,
		STARSHIP *StarShipPtr);

NetConnection *openPlayerNetworkConnection(COUNT player, void *extra);
void closePlayerNetworkConnection(COUNT player);

bool setupInputDelay(size_t localInputDelay);
bool setStateConnections(NetState state);
bool sendAbortConnections(NetplayAbortReason reason);
bool resetConnections(NetplayResetReason reason);
bool localReadyConnections(NetConnection_ReadyCallback readyCallback,
		void *arg, bool notifyRemote);

bool negotiateReady(NetConnection *conn, bool notifyRemote,
		NetState nextState);
bool negotiateReadyConnections(bool notifyRemote, NetState nextState);
bool waitReady(NetConnection *conn);

bool waitReset(NetConnection *conn, NetState nextState);
bool waitResetConnections(NetState nextState);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NETMELEE_H_ */


