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

#ifndef UQM_SUPERMELEE_NETPLAY_PACKET_H_
#define UQM_SUPERMELEE_NETPLAY_PACKET_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct Packet Packet;

typedef enum PacketType {
	PACKET_INIT,
	PACKET_PING,
	PACKET_ACK,
	PACKET_READY,
	PACKET_FLEET,
	PACKET_TEAMNAME,
	PACKET_HANDSHAKE0,
	PACKET_HANDSHAKE1,
	PACKET_HANDSHAKECANCEL,
	PACKET_HANDSHAKECANCELACK,
	PACKET_SEEDRANDOM,
	PACKET_INPUTDELAY,
	PACKET_SELECTSHIP,
	PACKET_BATTLEINPUT,
	PACKET_FRAMECOUNT,
	PACKET_CHECKSUM,
	PACKET_ABORT,
	PACKET_RESET,

	PACKET_NUM, /* Number of packet types */
} PacketType;

// Sent before aborting the connection.
typedef enum NetplayAbortReason {
	AbortReason_unspecified,
	AbortReason_versionMismatch,
	AbortReason_invalidHash,
	AbortReason_protocolError,
			// Network is in an inconsistent state.
} NetplayAbortReason;

// Sent before resetting the connection. A game in progress is terminated.
typedef enum NetplayResetReason {
	ResetReason_unspecified,
	ResetReason_syncLoss,
	ResetReason_manualReset,
} NetplayResetReason;

#if defined(__cplusplus)
}
#endif

#ifndef PACKET_H_STANDALONE
#include "netconnection.h"

#include "types.h"
#include "libs/network/bytesex.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* NB: These handlers are expected not to modify the state if an
 *     error occurs.
 * When a handler is called, it has already been validated that the
 * a complete packet has arrived.
 */
typedef int (*PacketHandler)(NetConnection *conn, const void *packet);

typedef struct {
	size_t len;  /* Minimal length of a packet of this type */
	PacketHandler handler;
	const char *name;
} PacketTypeData;

extern PacketTypeData packetTypeData[];

#if defined(__cplusplus)
}
#endif

#endif

#if defined(__cplusplus)
extern "C" {
#endif

// When adding new packets, be sure to have all the fields properly aligned,
// and that the size of a packet is a multiple of 4 bytes in length.
// Fields should be no longer than 4 bytes in themselves, as larger
// fields may require a larger alignment.

typedef struct {
	uint16 len;
	uint16 type;  /* Actually of type PacketType */
} PacketHeader;

// "Base class" for all packets.
struct Packet {
	PacketHeader header;
};

static inline size_t
packetLength(const Packet *packet) {
	return (size_t) ntoh16(packet->header.len);
}

static inline PacketType
packetType(const Packet *packet) {
	return (PacketType) (int) ntoh16(packet->header.type);
}

static inline bool
validPacketType(PacketType type) {
	return type < PACKET_NUM;
}

typedef struct {
	PacketHeader header;
	struct {
		uint8 major;
		uint8 minor;
	} protoVersion; /* Protocol version */
	uint16 padding0;  /* Set to 0 */
	struct {
		uint8 major;
		uint8 minor;
		uint8 patch;
	} uqmVersion; /* Protocol version */
	uint8 padding1;  /* Set to 0 */
} Packet_Init;

typedef struct {
	PacketHeader header;
	uint32 id;
} Packet_Ping;

// Acknowledgement of a Ping.
typedef struct {
	PacketHeader header;
	uint32 id;
} Packet_Ack;

// Used to signal that a party is ready to continue.
typedef struct {
	PacketHeader header;
	// No contents.
} Packet_Ready;

typedef struct {
	PacketHeader header;
	uint32 seed;
} Packet_SeedRandom;

typedef struct {
	PacketHeader header;
	uint32 delay;
} Packet_InputDelay;

// This enum is used to indicate that a packet containing it relates to
// either the local or the remote player, from the perspective of the
// sender of the message;
typedef enum {
	NetplaySide_local,
	NetplaySide_remote
} NetplaySide;

typedef struct {
	uint8 index;  /* Position in the fleet */
	uint8 ship;   /* Ship type index; actually MeleeShip */
} FleetEntry;
// Structure describing an update to a player's fleet.
// TODO: use strings as ship identifiers, instead of numbers,
// so that adding of new ships doesn't break this.
typedef struct {
	PacketHeader header;
	uint8 side;
	uint8 padding;
	uint16 numShips;
	FleetEntry ships[];
	// Be sure to add padding to this structure to make it a multiple of
	// 4 bytes in length.
} Packet_Fleet;

typedef struct {
	PacketHeader header;
	uint8 side;
	uint8 padding;
	uint8 name[];
			// '\0' terminated.
	// Be sure to add padding to this structure to make it a multiple of
	// 4 bytes in length.
} Packet_TeamName;

typedef struct {
	PacketHeader header;
	// No contents.
} Packet_Handshake0;

typedef struct {
	PacketHeader header;
	// No contents.
} Packet_Handshake1;

typedef struct {
	PacketHeader header;
	// No contents.
} Packet_HandshakeCancel;

typedef struct {
	PacketHeader header;
	// No contents.
} Packet_HandshakeCancelAck;

typedef struct {
	PacketHeader header;
	uint16 ship;
			// The value '(uint16) ~0' indicates random selection.
	uint16 padding;
} Packet_SelectShip;

typedef struct {
	PacketHeader header;
	uint8 state;  /* Actually BATTLE_INPUT_STATE */
	uint8 padding0;
	uint16 padding1;
} Packet_BattleInput;

typedef struct {
	PacketHeader header;
	uint32 frameCount;  /* Actually BattleFrameCounter */
} Packet_FrameCount;

typedef struct {
	PacketHeader header;
	uint32 frameNr;  /* Actually BattleFrameCounter */
	uint32 checksum;  /* Actually Checksum */
} Packet_Checksum;

typedef struct {
	PacketHeader header;
	uint16 reason;  /* Actually NetplayAbortReason */
	uint16 padding0;
} Packet_Abort;

typedef struct {
	PacketHeader header;
	uint16 reason;  /* Actually NetplayResetReason */
	uint16 padding0;
} Packet_Reset;


#ifndef PACKET_H_STANDALONE
void Packet_delete(Packet *packet);
Packet_Init *Packet_Init_create(void);
Packet_Ping *Packet_Ping_create(uint32 id);
Packet_Ack *Packet_Ack_create(uint32 id);
Packet_Ready *Packet_Ready_create(void);
Packet_Handshake0 *Packet_Handshake0_create(void);
Packet_Handshake1 *Packet_Handshake1_create(void);
Packet_HandshakeCancel *Packet_HandshakeCancel_create(void);
Packet_HandshakeCancelAck *Packet_HandshakeCancelAck_create(void);
Packet_SeedRandom *Packet_SeedRandom_create(uint32 seed);
Packet_InputDelay *Packet_InputDelay_create(uint32 delay);
Packet_Fleet *Packet_Fleet_create(NetplaySide side, size_t numShips);
Packet_TeamName *Packet_TeamName_create(NetplaySide side, const char *name,
		size_t size);
Packet_SelectShip *Packet_SelectShip_create(uint16 ship);
Packet_BattleInput *Packet_BattleInput_create(uint8 state);
Packet_FrameCount *Packet_FrameCount_create(uint32 frameCount);
Packet_Checksum *Packet_Checksum_create(uint32 frameNr, uint32 checksum);
Packet_Abort *Packet_Abort_create(uint16 reason);
Packet_Reset *Packet_Reset_create(uint16 reason);
#endif

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_PACKET_H_ */

