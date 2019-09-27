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

#include "netplay.h"
#include "packet.h"

#include "uqmversion.h"

#include "netrcv.h"
#include "packethandlers.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


#define DEFINE_PACKETDATA(name) \
	{ \
		/* .len = */      sizeof (Packet_##name), \
		/* .handler = */  (PacketHandler) PacketHandler_##name, \
		/* .name = */     #name, \
	}
PacketTypeData packetTypeData[PACKET_NUM] = {
	DEFINE_PACKETDATA(Init),
	DEFINE_PACKETDATA(Ping),
	DEFINE_PACKETDATA(Ack),
	DEFINE_PACKETDATA(Ready),
	DEFINE_PACKETDATA(Fleet),
	DEFINE_PACKETDATA(TeamName),
	DEFINE_PACKETDATA(Handshake0),
	DEFINE_PACKETDATA(Handshake1),
	DEFINE_PACKETDATA(HandshakeCancel),
	DEFINE_PACKETDATA(HandshakeCancelAck),
	DEFINE_PACKETDATA(SeedRandom),
	DEFINE_PACKETDATA(InputDelay),
	DEFINE_PACKETDATA(SelectShip),
	DEFINE_PACKETDATA(BattleInput),
	DEFINE_PACKETDATA(FrameCount),
	DEFINE_PACKETDATA(Checksum),
	DEFINE_PACKETDATA(Abort),
	DEFINE_PACKETDATA(Reset),
};

static inline void *
Packet_alloc(size_t size) {
	return malloc(size);
}

static Packet *
Packet_create(PacketType type, size_t extraSize) {
	Packet *result;
	size_t len;

	// Alignment requirement.
	assert(extraSize % 4 == 0);

	len = packetTypeData[type].len + extraSize;
	result = Packet_alloc(len);
	result->header.len = hton16((uint16) len);
	result->header.type = hton16((uint16) type);
	return result;
}

void
Packet_delete(Packet *packet) {
	free(packet);
}

Packet_Init *
Packet_Init_create(void) {
	Packet_Init *packet = (Packet_Init *) Packet_create(PACKET_INIT, 0);

	packet->protoVersion.major = NETPLAY_PROTOCOL_VERSION_MAJOR;
	packet->protoVersion.minor = NETPLAY_PROTOCOL_VERSION_MINOR;
	packet->padding0 = 0;
	packet->uqmVersion.major = UQM_MAJOR_VERSION;
	packet->uqmVersion.minor = UQM_MINOR_VERSION;
	packet->uqmVersion.patch = UQM_PATCH_VERSION;
	packet->padding1 = 0;
	return packet;
}

Packet_Ping *
Packet_Ping_create(uint32 id) {
	Packet_Ping *packet = (Packet_Ping *) Packet_create(PACKET_PING, 0);

	packet->id = hton32(id);
	return packet;
}

Packet_Ack *
Packet_Ack_create(uint32 id) {
	Packet_Ack *packet = (Packet_Ack *) Packet_create(PACKET_ACK, 0);

	packet->id = hton32(id);
	return packet;
}

Packet_Ready *
Packet_Ready_create(void) {
	Packet_Ready *packet = (Packet_Ready *) Packet_create(PACKET_READY, 0);

	return packet;
}

// The fleet itself still needs to be filled by the caller.
// This function takes care of the necessary padding; it is allocated,
// and filled with zero chars.
Packet_Fleet *
Packet_Fleet_create(NetplaySide side, size_t numShips) {
	Packet_Fleet *packet;
	size_t fleetSize;
	size_t extraSize;
	
	fleetSize = numShips * sizeof (FleetEntry);
	extraSize = (fleetSize + 3) & ~0x03;
	packet = (Packet_Fleet *) Packet_create(PACKET_FLEET, extraSize);
	packet->side = (uint8) side;
	packet->padding = 0;
	packet->numShips = hton16((uint16) numShips);
	memset((char *) packet + sizeof (Packet_Fleet) + fleetSize,
			'\0', extraSize - fleetSize);

	return packet;
}

// 'size' is the number of bytes (not characters) in 'name', excluding
// a possible terminating '\0'. A '\0' will be included in the packet though.
// This function takes care of the required padding.
Packet_TeamName *
Packet_TeamName_create(NetplaySide side, const char *name, size_t size) {
	Packet_TeamName *packet;
	size_t extraSize;
	
	extraSize = ((size + 1) + 3) & ~0x03;
			// The +1 is for the '\0'.
	packet = (Packet_TeamName *) Packet_create(PACKET_TEAMNAME, extraSize);
	packet->side = (uint8) side;
	packet->padding = 0;
	memcpy(packet->name, name, size);
	memset((char *) packet + sizeof (Packet_TeamName) + size, '\0',
			extraSize - size);
			// This takes care of the terminating '\0', as well as the
			// padding.

	return packet;
}

Packet_Handshake0 *
Packet_Handshake0_create(void) {
	Packet_Handshake0 *packet =
			(Packet_Handshake0 *) Packet_create(PACKET_HANDSHAKE0, 0);
	return packet;
}

Packet_Handshake1 *
Packet_Handshake1_create(void) {
	Packet_Handshake1 *packet =
			(Packet_Handshake1 *) Packet_create(PACKET_HANDSHAKE1, 0);
	return packet;
}

Packet_HandshakeCancel *
Packet_HandshakeCancel_create(void) {
	Packet_HandshakeCancel *packet =
			(Packet_HandshakeCancel *) Packet_create(
			PACKET_HANDSHAKECANCEL, 0);
	return packet;
}

Packet_HandshakeCancelAck *
Packet_HandshakeCancelAck_create(void) {
	Packet_HandshakeCancelAck *packet =
			(Packet_HandshakeCancelAck *) Packet_create(
			PACKET_HANDSHAKECANCELACK, 0);
	return packet;
}

Packet_SeedRandom *
Packet_SeedRandom_create(uint32 seed) {
	Packet_SeedRandom *packet =
			(Packet_SeedRandom *) Packet_create(PACKET_SEEDRANDOM, 0);

	packet->seed = hton32(seed);
	return packet;
}

Packet_InputDelay *
Packet_InputDelay_create(uint32 delay) {
	Packet_InputDelay *packet =
			(Packet_InputDelay *) Packet_create(PACKET_INPUTDELAY, 0);

	packet->delay = hton32(delay);
	return packet;
}

Packet_SelectShip *
Packet_SelectShip_create(uint16 ship) {
	Packet_SelectShip *packet =
			(Packet_SelectShip *) Packet_create(PACKET_SELECTSHIP, 0);
	packet->ship = hton16(ship);
	packet->padding = 0;
	return packet;
}

Packet_BattleInput *
Packet_BattleInput_create(uint8 state) {
	Packet_BattleInput *packet =
			(Packet_BattleInput *) Packet_create(PACKET_BATTLEINPUT, 0);
	packet->state = (uint8) state;
	packet->padding0 = 0;
	packet->padding1 = 0;
	return packet;
}

Packet_FrameCount *
Packet_FrameCount_create(uint32 frameCount) {
	Packet_FrameCount *packet =
			(Packet_FrameCount *) Packet_create(PACKET_FRAMECOUNT, 0);
	packet->frameCount = hton32(frameCount);
	return packet;
}

Packet_Checksum *
Packet_Checksum_create(uint32 frameNr, uint32 checksum) {
	Packet_Checksum *packet =
			(Packet_Checksum *) Packet_create(PACKET_CHECKSUM, 0);
	packet->frameNr = hton32(frameNr);
	packet->checksum = hton32(checksum);
	return packet;
}

Packet_Abort *
Packet_Abort_create(uint16 reason) {
	Packet_Abort *packet = (Packet_Abort *) Packet_create(PACKET_ABORT, 0);
	packet->reason = hton16(reason);
	return packet;
}

Packet_Reset *
Packet_Reset_create(uint16 reason) {
	Packet_Reset *packet = (Packet_Reset *) Packet_create(PACKET_RESET, 0);
	packet->reason = hton16(reason);
	return packet;
}



