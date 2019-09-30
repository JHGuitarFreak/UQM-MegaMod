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
#include "netplay.h"
#include "netconnection.h"
#include "packetq.h"
#include "netsend.h"
#include "packetsenders.h"
#ifdef NETPLAY_DEBUG
#	include "libs/log.h"
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static inline PacketQueueLink *
PacketQueueLink_alloc(void) {
	// XXX: perhaps keep a pool of links?
	return malloc(sizeof (PacketQueueLink));
}

static inline void
PacketQueueLink_delete(PacketQueueLink *link) {
	free(link);
}

// 'maxSize' should at least be 1
void
PacketQueue_init(PacketQueue *queue) {
	queue->size = 0;
	queue->first = NULL;
	queue->end = &queue->first;
}

static void
PacketQueue_deleteLinks(PacketQueueLink *link) {
	while (link != NULL) {
		PacketQueueLink *next = link->next;
		Packet_delete(link->packet);
		PacketQueueLink_delete(link);
		link = next;
	}
}

void
PacketQueue_uninit(PacketQueue *queue) {
	PacketQueue_deleteLinks(queue->first);
}

void
queuePacket(NetConnection *conn, Packet *packet) {
	PacketQueue *queue;
	PacketQueueLink *link;

	assert(NetConnection_isConnected(conn));
	
	queue = &conn->queue;

	link = PacketQueueLink_alloc();
	link->packet = packet;
	link->next = NULL;
	*queue->end = link;
	queue->end = &link->next;

	queue->size++;
	// XXX: perhaps check that this queue isn't getting too large?

#ifdef NETPLAY_DEBUG
	if (packetType(packet) != PACKET_BATTLEINPUT &&
			packetType(packet) != PACKET_CHECKSUM) {
		// Reporting BattleInput or Checksum would get so spammy that it
		// would slow down the battle.
		log_add(log_Debug, "NETPLAY: [%d] ==> Queueing packet of type %s.\n",
				NetConnection_getPlayerNr(conn),
				packetTypeData[packetType(packet)].name);
	}
#ifdef NETPLAY_DEBUG_FILE
	if (conn->debugFile != NULL) {
		uio_fprintf(conn->debugFile,
				"NETPLAY: [%d] ==> Queueing packet of type %s.\n",
				NetConnection_getPlayerNr(conn),
				packetTypeData[packetType(packet)].name);
	}
#endif  /* NETPLAY_DEBUG_FILE */
#endif  /* NETPLAY_DEBUG */
}

// If an error occurs during sending, we leave the unsent packets in
// the queue, and let the caller decide what to do with them.
// This function may return -1 with errno EAGAIN or EWOULDBLOCK
// if we're waiting for the other party to act first.
static int
flushPacketQueueLinks(NetConnection *conn, PacketQueueLink **first) {
	PacketQueueLink *link;
	PacketQueueLink *next;
	PacketQueue *queue = &conn->queue;
	
	for (link = *first; link != NULL; link = next) {
		if (sendPacket(conn, link->packet) == -1) {
			// Errno is set.
			*first = link;
			return -1;
		}
		
		next = link->next;
		Packet_delete(link->packet);
		PacketQueueLink_delete(link);
		queue->size--;
	}

	*first = link;
	return 0;
}

int
flushPacketQueue(NetConnection *conn) {
	int flushResult;
	PacketQueue *queue = &conn->queue;
	
	assert(NetConnection_isConnected(conn));

	flushResult = flushPacketQueueLinks(conn, &queue->first);
	if (queue->first == NULL)
		queue->end = &queue->first;
	if (flushResult == -1) {
		// errno is set
		return -1;
	}
	
	return 0;
}

