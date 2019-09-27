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
#include "npconfirm.h"

#include "types.h"
#include "../netmisc.h"
#include "../packetsenders.h"

#include <assert.h>
#include <errno.h>

int
Netplay_confirm(NetConnection *conn) {
	assert(handshakeMeaningful(NetConnection_getState(conn)));

	if (conn->stateFlags.handshake.localOk) {
		// Already confirmed
		errno = EINVAL;
		return -1;
	}
	
	conn->stateFlags.handshake.localOk = true;

	if (conn->stateFlags.handshake.canceling) {
		// If a previous confirmation was cancelled, but the cancel
		// is not acknowledged yet, we don't have to send anything yet.
		// The handshake0 packet will be sent when the acknowledgement
		// arrives.
	} else if (conn->stateFlags.handshake.remoteOk) {
		// A Handshake0 is implied by the following Handshake1.
		sendHandshake1(conn);
	} else {
		sendHandshake0(conn);
	}
	
	return 0;
}

int
Netplay_cancelConfirmation(NetConnection *conn) {
	assert(handshakeMeaningful(NetConnection_getState(conn)));

	if (!conn->stateFlags.handshake.localOk) {
		// Not confirmed, or already canceling.
		errno = EINVAL;
		return -1;
	}

	conn->stateFlags.handshake.localOk = false;
	if (conn->stateFlags.handshake.canceling) {
		// If previous cancellation is still waiting to be acknowledged,
		// the confirmation we are cancelling here, has not actually been
		// sent yet. By setting the localOk flag to false, it is
		// cancelled, without the need for any packets to be sent.
	} else {
		conn->stateFlags.handshake.canceling = true;
		sendHandshakeCancel(conn);
	}

	return 0;
}


