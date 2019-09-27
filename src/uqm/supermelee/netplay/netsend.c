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

#define PORT_WANT_ERRNO
#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "port.h"

#include "netsend.h"
#include "netconnection.h"
#include "packet.h"
#include "libs/log.h"
#include "libs/net.h"

#include <assert.h>
#include <errno.h>
#include <string.h>


int
sendPacket(NetConnection *conn, Packet *packet) {
	ssize_t sendResult;
	size_t len;
	Socket *socket;
		
	assert(NetConnection_isConnected(conn));

#ifdef NETPLAY_DEBUG
	//if (packetType(packet) != PACKET_BATTLEINPUT && 
	//		packetType(packet) != PACKET_CHECKSUM) {
	//	// Reporting BattleInput or Checksum would get so spammy that it
	//	// would slow down the battle.
	//	log_add(log_Debug, "NETPLAY: [%d] ==> Sending packet of type %s.\n",
	//			conn->player, packetTypeData[packetType(packet)].name);
	//}
#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != NULL) {
		uio_fprintf(conn->debugFile,
				"NETPLAY: [%d] ==> Sending packet of type %s.\n",
				conn->player, packetTypeData[packetType(packet)].name);
	}
#endif  /* NETPLAY_DEBUG_FILE */
#endif  /* NETPLAY_DEBUG */

	socket = NetDescriptor_getSocket(conn->nd);

	len = packetLength(packet);
	while (len > 0) {
		sendResult = Socket_send(socket, (void *) packet, len, 0);
		if (sendResult >= 0) {
			len -= sendResult;
			continue;
		}

		switch (errno) {
			case EINTR:  // System call interrupted, retry;
				continue;
			case ECONNRESET: {  // Connection reset by peer.
				// keep errno
				return -1;
			}
			default: {
				// Should not happen.
				int savedErrno = errno;
				log_add(log_Error, "send() failed: %s.\n", strerror(errno));
				errno = savedErrno;
				return -1;
			}
		}
	}

#ifdef NETPLAY_STATISTICS
	NetConnection_getStatistics(conn)->packetsSent++;
	NetConnection_getStatistics(conn)->packetTypeSent[packetType(packet)]++;
#endif

	return 0;
}


