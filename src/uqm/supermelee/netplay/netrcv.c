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

#include "netconnection.h"
#include "netrcv.h"
#include "packet.h"

#include "types.h"
#include "libs/log.h"

#include <errno.h>
#include <string.h>


// Try to get a single packet from a stream of data.
// Returns 0 if the packet was successfully processed, or
// -1 on an error, in which case the state is unchanged.
static ssize_t
dataReceivedSingle(NetConnection *conn, const uint8 *data,
		size_t dataLen) {
	uint32 packetLen;
	PacketType type;
	int result;
	
	if (dataLen < sizeof (PacketHeader)) {
		// Incomplete packet. We'll have to wait for the rest.
		return 0;
	}
	
	packetLen = packetLength((const Packet *) data);
	type = packetType((const Packet *) data);

	if (!validPacketType(type)) {
		log_add(log_Warning, "Packet with invalid type %d received.\n", type);
		errno = EBADMSG;
		return -1;
	}

	if (packetLen < packetTypeData[type].len) {
		// Bad len field of packet.
		log_add(log_Warning, "Packet with bad length field received (type="
				"%s, lenfield=%d.\n", packetTypeData[type].name,
				packetLen);
		errno = EBADMSG;
		return -1;
	}

	if (dataLen < packetLen) {
		// Incomplete packet. We'll have to wait for the rest.
		return 0;
	}

#ifdef NETPLAY_STATISTICS
	NetConnection_getStatistics(conn)->packetsReceived++;
	NetConnection_getStatistics(conn)->packetTypeReceived[type]++;
#endif

#ifdef NETPLAY_DEBUG
	if (type != PACKET_BATTLEINPUT && type != PACKET_CHECKSUM) {
		// Reporting BattleInput and Checksum would get so spammy that it
		// would slow down the battle.
		log_add(log_Debug, "NETPLAY: [%d] <== Received packet of type %s.\n",
				NetConnection_getPlayerNr(conn), packetTypeData[type].name);
	}
#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != NULL) {
		uio_fprintf(conn->debugFile,
				"NETPLAY: [%d] <== Received packet of type %s.\n",
				NetConnection_getPlayerNr(conn), packetTypeData[type].name);
	}
#endif  /* NETPLAY_DEBUG_FILE */
#endif  /* NETPLAY_DEBUG */
	
	result = packetTypeData[type].handler(conn, data);
	if (result == -1) {
		// An error occured. errno is set by the handler.
		return -1;
	}

	return packetLen;
}

// Try to get all the packets from a stream of data.
// Returns the number of bytes processed.
static ssize_t
dataReceivedMulti(NetConnection *conn, const uint8 *data, size_t len) {
	size_t processed;
	
	processed = 0;
	while (len > 0) {
		ssize_t packetLen = dataReceivedSingle(conn, data, len);
		if (packetLen == -1) {
			// Bad packet. Errno is set.
			return -1;
		}

		if (packetLen == 0) {
			// No packet was processed. This means that no complete
			// packet arrived.
			break;
		}

		processed += packetLen;
		data += packetLen;
		len -= packetLen;
	}

	return processed;
}

void
dataReadyCallback(NetDescriptor *nd) {
	NetConnection *conn = (NetConnection *) NetDescriptor_getExtra(nd);
	Socket *socket = NetDescriptor_getSocket(nd);

	for (;;) {
		ssize_t numRead;
		ssize_t numProcessed;

		numRead = Socket_recv(socket, conn->readEnd,
				NETPLAY_READBUFSIZE - (conn->readEnd - conn->readBuf), 0);
		if (numRead == 0) {
			// Other side closed the connection.
			NetDescriptor_close(nd);
			return;
		}

		if (numRead == -1) {
			switch (errno) {
				case EAGAIN:  // No more data for now.
					return;
				case EINTR:  // System call was interrupted. Retry.
					continue;
				default: {
					int savedErrno = errno;
					log_add(log_Error, "recv() failed: %s.\n",
							strerror(errno));
					NetConnection_doErrorCallback(conn, savedErrno);
					NetDescriptor_close(nd);
					return;
				}
			}
		}

		conn->readEnd += numRead;

		numProcessed = dataReceivedMulti(conn, conn->readBuf,
				conn->readEnd - conn->readBuf);
		if (numProcessed == -1) {
			// An error occured during processing.
			// errno is set.
			NetConnection_doErrorCallback(conn, errno);
			NetDescriptor_close(nd);
			return;
		}
		if (numProcessed == 0) {
			// No packets could be processed. This means we need to receive
			// more data first.
			return;
		}
		
		// Some packets have been processed.
		// We more any rest to the front of the buffer, to make room
		// for more data.
		// A cyclic buffer would obviate the need for this move,
		// but it would complicate things a lot.
		memmove(conn->readBuf, conn->readBuf + numProcessed,
				(conn->readEnd - conn->readBuf) - numProcessed);
		conn->readEnd -= numProcessed;
	}
}



