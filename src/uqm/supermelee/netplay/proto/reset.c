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
	
// See doc/devel/netplay/protocol

#define NETCONNECTION_INTERNAL
#include "../netplay.h"
#include "reset.h"

#include "types.h"
#include "../packetsenders.h"
#include "../../melee.h"
		// For resetFeedback.

#include <assert.h>

// Reset packets are sent to indicate that a game is to be reset.
// i.e. the game is to return to the SuperMelee fleet setup menu.
// The reset will occur when a reset packet has both been sent and
// received. When a reset packet is received and the local side had not
// sent a reset packet itself, the local side will confirm the reset.
// When both sides initiate a reset simultaneously, the reset packets
// of each side will act as a confirmation for the other side.
//
// When a reset packet has been sent, no further gameplay packets should be
// sent until the game has been reset. Non-gameplay packets such as 'ping'
// are allowed.
// When a reset packet has been received, all further incoming gameplay
// packets are ignored until the game has been reset.
//
// conn->stateFlags.reset.localReset is set when a reset packet is sent.
// conn->stateFlags.reset.remoteReset is set when a reset packet is
// received.
//
// When either localReset or remoteReset gets set and the other flag isn't
// set, Netplay_connectionReset() gets called.
//
// As soon as the following three conditions are met, the reset callback is
// called and the localReset and remoteReset flags are cleared.
// - conn->stateFlags.reset.localReset is set
// - conn->stateFlags.reset.remoteReset is set
// - the reset callback is non-NULL.
//
// Elsewhere in the UQM source:
// When the local side causes a reset, it calls Netplay_localReset().
// When a remote reset packet is received, Netplay_remoteReset() is called
// (which will sent a reset packet back as confirmation, as required).
// At the end of melee, the reset callback is set (for each connection),
// and the game will wait until the reset callback for each connection has
// been called (when the forementioned conditions have become true)
// (or until the connection is terminated).


// This function is called when one side initiates a reset.
static void
Netplay_connectionReset(NetConnection *conn, NetplayResetReason reason,
		bool byRemote) {
	switch (NetConnection_getState(conn)) {
		case NetState_unconnected:
		case NetState_connecting:
		case NetState_init:
		case NetState_inSetup:
			break;
		case NetState_preBattle:
		case NetState_interBattle:
		case NetState_selectShip:
		case NetState_inBattle:
		case NetState_endingBattle:
		case NetState_endingBattle2:
			resetFeedback(conn, reason, byRemote);
			break;
	}
}

static void
Netplay_doConnectionResetCallback(NetConnection *conn) {
	NetConnection_ResetCallback callback;
	void *resetArg;

	callback = conn->resetCallback;
	resetArg = conn->resetCallbackArg;

	NetConnection_setResetCallback(conn, NULL, NULL);
			// Clear the resetCallback field before performing the callback,
			// so that it can be set again from inside the callback
			// function.
	callback(conn, resetArg);
}

static void
Netplay_resetConditionTriggered(NetConnection *conn) {
	if (conn->resetCallback == NULL)
		return;
	
	if (!conn->stateFlags.reset.localReset ||
			!conn->stateFlags.reset.remoteReset)
		return;

	conn->stateFlags.reset.localReset = false;
	conn->stateFlags.reset.remoteReset = false;

	Netplay_doConnectionResetCallback(conn);
}

void
Netplay_setResetCallback(NetConnection *conn,
		NetConnection_ResetCallback callback, void *resetArg) {
	NetConnection_setResetCallback(conn, callback, resetArg);

	Netplay_resetConditionTriggered(conn);
}

void
Netplay_localReset(NetConnection *conn, NetplayResetReason reason) {
	assert(!conn->stateFlags.reset.localReset);

	conn->stateFlags.reset.localReset = true;
	if (conn->stateFlags.reset.remoteReset) {
		// Both sides have initiated/confirmed the reset.
		Netplay_resetConditionTriggered(conn);
	} else {
		sendReset(conn, reason);
		Netplay_connectionReset(conn, reason, false);
	}
}

void
Netplay_remoteReset(NetConnection *conn, NetplayResetReason reason) {
	assert(!conn->stateFlags.reset.remoteReset);
			// Should already be checked when the packet arrives.

	conn->stateFlags.reset.remoteReset = true;
	if (!conn->stateFlags.reset.localReset) {
		sendReset(conn, reason);
		conn->stateFlags.reset.localReset = true;
		Netplay_connectionReset(conn, reason, true);
	}

	Netplay_resetConditionTriggered(conn);
}

bool
Netplay_isLocalReset(const NetConnection *conn) {
	return conn->stateFlags.reset.localReset;
}

bool
Netplay_isRemoteReset(const NetConnection *conn) {
	return conn->stateFlags.reset.remoteReset;
}

