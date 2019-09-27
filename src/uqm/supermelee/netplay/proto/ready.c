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

#define NETCONNECTION_INTERNAL
#include "../netplay.h"
#include "ready.h"

#include "types.h"
#include "../netmisc.h"
#include "../packetsenders.h"

#include <assert.h>

static void
Netplay_bothReady(NetConnection *conn) {
	NetConnection_ReadyCallback callback;
	void *readyArg;

	assert(conn->readyCallback != NULL);

	callback = conn->readyCallback;
	readyArg = conn->readyCallbackArg;

	NetConnection_setReadyCallback(conn, NULL, NULL);
			// Clear the readyCallback field before performing the callback,
			// so that it can be set again from inside the callback
			// function.

	callback(conn, readyArg);
}

// If notifyRemote is set, a 'Ready' message will be sent to the other side.
// returns true iff both sides are ready.
// Inside the callback function, ready flags may be set for a possible
// next Ready communication.
bool
Netplay_localReady(NetConnection *conn, NetConnection_ReadyCallback callback,
		void *readyArg, bool notifyRemote) {
	assert(readyFlagsMeaningful(NetConnection_getState(conn)));
	assert(!conn->stateFlags.ready.localReady);
	assert(callback != NULL);

	NetConnection_setReadyCallback(conn, callback, readyArg);

	if (notifyRemote)
		sendReady(conn);
	if (!conn->stateFlags.ready.remoteReady) {
		conn->stateFlags.ready.localReady = true;
		return false;
	}

	// Reset ready flags:
	conn->stateFlags.ready.remoteReady = false;

	// Trigger the callback.
	Netplay_bothReady(conn);
	return true;
}

// returns true iff both sides are ready.
bool
Netplay_remoteReady(NetConnection *conn) {
	assert(readyFlagsMeaningful(NetConnection_getState(conn)));
			// This is supposed to be already verified by the calling
			// function.
	assert(!conn->stateFlags.ready.remoteReady);
	
	if (!conn->stateFlags.ready.localReady) {
		conn->stateFlags.ready.remoteReady = true;
		return false;
	}

	// Reset ready flags:
	conn->stateFlags.ready.localReady = false;

	// Trigger the callback.
	Netplay_bothReady(conn);
	return true;
}

bool
Netplay_isLocalReady(const NetConnection *conn) {
	return conn->stateFlags.ready.localReady;
}

bool
Netplay_isRemoteReady(const NetConnection *conn) {
	return conn->stateFlags.ready.remoteReady;
}


