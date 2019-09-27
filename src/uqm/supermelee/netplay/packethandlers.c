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
#include "port.h"

#define NETCONNECTION_INTERNAL
#include "netplay.h"
#include "packethandlers.h"

#include "netinput.h"
#include "netmisc.h"
#include "packetsenders.h"
#include "proto/npconfirm.h"
#include "proto/ready.h"
#include "proto/reset.h"
#include "libs/log.h"

#include "../../controls.h"
		// for BATTLE_INPUT_STATE
#include "../../init.h"
		// for NUM_PLAYERS
#include "../../globdata.h"
		// for GLOBAL
#include "../melee.h"
		// for various update functions.
#include "../meleeship.h"
		// for MeleeShip
#include "../pickmele.h"
		// for various update functions.
#include "libs/mathlib.h"
		// for TFB_SeedRandom

#include <errno.h>


static bool
testNetState(bool condition, PacketType type) {
	if (!condition) {
		log_add(log_Error, "Packet of type '%s' received from wrong "
				"state.\n", packetTypeData[type].name);
		errno = EBADMSG;
	}
	return condition;
}

static int
versionCompare(int major1, int minor1, int patch1,
		int major2, int minor2, int patch2) {
	if (major1 < major2)
		return -1;
	if (major1 > major2)
		return 1;

	if (minor1 < minor2)
		return -1;
	if (minor1 > minor2)
		return 1;

	if (patch1 < patch2)
		return -1;
	if (patch1 > patch2)
		return 1;

	return 0;
}

int
PacketHandler_Init(NetConnection *conn, const Packet_Init *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_init &&
			!conn->stateFlags.ready.remoteReady, PACKET_INIT))
		return -1;  // errno is set
	
	if (packet->protoVersion.major != NETPLAY_PROTOCOL_VERSION_MAJOR ||
			packet->protoVersion.minor != NETPLAY_PROTOCOL_VERSION_MINOR) {
		sendAbort (conn, AbortReason_versionMismatch);
		abortFeedback(conn, AbortReason_versionMismatch);
		log_add(log_Error, "Protocol version %d.%d not supported.\n",
				packet->protoVersion.major, packet->protoVersion.minor);
		errno = ENOSYS;
		return -1;
	}

	if (versionCompare(packet->uqmVersion.major, packet->uqmVersion.minor,
			packet->uqmVersion.patch, NETPLAY_MIN_UQM_VERSION_MAJOR,
			NETPLAY_MIN_UQM_VERSION_MINOR, NETPLAY_MIN_UQM_VERSION_PATCH)
			< 0) {
		sendAbort (conn, AbortReason_versionMismatch);
		abortFeedback(conn, AbortReason_versionMismatch);
		log_add(log_Error, "Remote side is running a version of UQM that "
				"is too old (%d.%d.%g; %d.%d.%g is required).\n",
				packet->uqmVersion.major, packet->uqmVersion.minor,
				packet->uqmVersion.patch, NETPLAY_MIN_UQM_VERSION_MAJOR,
				NETPLAY_MIN_UQM_VERSION_MINOR, NETPLAY_MIN_UQM_VERSION_PATCH);
		errno = ENOSYS;
		return -1;
	}

	Netplay_remoteReady(conn);
	
	return 0;
}

int
PacketHandler_Ping(NetConnection *conn, const Packet_Ping *packet) {
	if (!testNetState(conn->state > NetState_init, PACKET_PING))
		return -1;  // errno is set

	sendAck(conn, packet->id);
	return 0;
}

int
PacketHandler_Ack(NetConnection *conn, const Packet_Ack *packet) {
	if (!testNetState(conn->state > NetState_init, PACKET_ACK))
		return -1;  // errno is set

	(void) conn;
	(void) packet;
	return 0;
}

// Convert the side indication relative to a remote party to
// a local player number.
static inline int
localSide(NetConnection *conn, NetplaySide side) {
	if (side == NetplaySide_local) {
		// "local" relative to the remote party.
		return conn->player;
	}

	return 1 - conn->player;
}

int
PacketHandler_Ready(NetConnection *conn, const Packet_Ready *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(readyFlagsMeaningful(conn->state) &&
			!conn->stateFlags.ready.remoteReady, PACKET_READY))
		return -1;  // errno is set

	Netplay_remoteReady(conn);
	
	(void) packet;
			// Its contents is not interesting.
	
	return 0;
}

int
PacketHandler_Fleet(NetConnection *conn, const Packet_Fleet *packet) {
	uint16 numShips = ntoh16(packet->numShips);
	size_t i;
	size_t len;
	int player;
	BattleStateData *battleStateData;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_inSetup, PACKET_FLEET))
		return -1;  // errno is set

	player = localSide(conn, (NetplaySide) packet->side);

	len = packetLength((const Packet *) packet);
	if (sizeof packet + numShips * sizeof(packet->ships[0]) > len) {
		// There is not enough room in the packet to contain all
		// the ships it says it contains.
		log_add(log_Warning, "Invalid fleet size. Specified size is %d, "
				"actual size = %d\n",
				numShips, (len - sizeof packet) / sizeof(packet->ships[0]));
		errno = EBADMSG;
		return -1;
	}

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);

	if (conn->stateFlags.handshake.localOk) {
		Netplay_cancelConfirmation(conn);
		confirmationCancelled(battleStateData->meleeState, conn->player);
	}

	for (i = 0; i < numShips; i++) {
		MeleeShip ship = (MeleeShip) packet->ships[i].ship;
		FleetShipIndex index = (FleetShipIndex) packet->ships[i].index;
		
		if (!MeleeShip_valid(ship)) {
			log_add (log_Warning, "Invalid ship type number %d (max = %d).\n",
					ship, NUM_MELEE_SHIPS - 1);
			errno = EBADMSG;
			return -1;
		}
	
		if (index >= MELEE_FLEET_SIZE)
		{
			log_add (log_Warning, "Invalid ship position number %d "
					"(max = %d).\n", index, MELEE_FLEET_SIZE - 1);
			errno = EBADMSG;
			return -1;
		}
	
		Melee_RemoteChange_ship (battleStateData->meleeState, conn,
				player, index, ship);
	}

	// Padding data may follow; it is ignored.
	return 0;
}

int
PacketHandler_TeamName(NetConnection *conn, const Packet_TeamName *packet) {
	size_t nameLen;
	int side;
	BattleStateData *battleStateData;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_inSetup, PACKET_FLEET))
		return -1;  // errno is set

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);

	if (conn->stateFlags.handshake.localOk) {
		Netplay_cancelConfirmation(conn);
		confirmationCancelled(battleStateData->meleeState, conn->player);
	}
	
	side = localSide(conn, (NetplaySide) packet->side);
	nameLen = packetLength((const Packet *) packet)
			- sizeof (Packet_TeamName) - 1;
			// The -1 is for not counting the terminating '\0'.

	{
		char buf[MAX_TEAM_CHARS + 1];

		if (nameLen > MAX_TEAM_CHARS)
			nameLen = MAX_TEAM_CHARS;
		memcpy (buf, (const char *) packet->name, nameLen);
		buf[nameLen] = '\0';
		
		Melee_RemoteChange_teamName(battleStateData->meleeState, conn,
				side, buf);
	}

	// Padding data may follow; it is ignored.
	return 0;
}

static void
handshakeComplete(NetConnection *conn) {
	assert(!conn->stateFlags.handshake.localOk);
	assert(!conn->stateFlags.handshake.remoteOk);
	assert(!conn->stateFlags.handshake.canceling);

	assert(conn->state == NetState_inSetup);
	NetConnection_setState(conn, NetState_preBattle);
}

int
PacketHandler_Handshake0(NetConnection *conn,
		const Packet_Handshake0 *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(handshakeMeaningful(conn->state)
			&& !conn->stateFlags.handshake.remoteOk, PACKET_HANDSHAKE0))
		return -1;  // errno is set

	conn->stateFlags.handshake.remoteOk = true;
	if (conn->stateFlags.handshake.localOk &&
			!conn->stateFlags.handshake.canceling)
		sendHandshake1(conn);
	
	(void) packet;
			// Its contents is not interesting.

	return 0;
}

int
PacketHandler_Handshake1(NetConnection *conn,
		const Packet_Handshake1 *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(handshakeMeaningful(conn->state) &&
			(conn->stateFlags.handshake.localOk ||
			conn->stateFlags.handshake.canceling), PACKET_HANDSHAKE1))
		return -1;  // errno is set

	if (conn->stateFlags.handshake.canceling) {
		conn->stateFlags.handshake.remoteOk = true;
	} else {
		bool remoteWasOk = conn->stateFlags.handshake.remoteOk;

		conn->stateFlags.handshake.localOk = false;	
		conn->stateFlags.handshake.remoteOk = false;	
	
		if (!remoteWasOk) {
			// Received Handshake1 without prior Handshake0.
			// A Handshake0 is implied, but we still need to confirm
			// it with a Handshake1.
			sendHandshake1(conn);
		}

		handshakeComplete(conn);
	}
	
	(void) packet;
			// Its contents is not interesting.

	return 0;
}

int
PacketHandler_HandshakeCancel(NetConnection *conn,
		const Packet_HandshakeCancel *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(handshakeMeaningful(conn->state)
			&& conn->stateFlags.handshake.remoteOk, PACKET_HANDSHAKECANCEL))
		return -1;  // errno is set

	conn->stateFlags.handshake.remoteOk = false;
	sendHandshakeCancelAck(conn);
	
	(void) packet;
			// Its contents is not interesting.

	return 0;
}

int
PacketHandler_HandshakeCancelAck(NetConnection *conn,
		const Packet_HandshakeCancelAck *packet) {
	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(handshakeMeaningful(conn->state)
			&& conn->stateFlags.handshake.canceling,
			PACKET_HANDSHAKECANCELACK))
		return -1;  // errno is set

	conn->stateFlags.handshake.canceling = false;
	if (conn->stateFlags.handshake.localOk) {
		if (conn->stateFlags.handshake.remoteOk) {
			sendHandshake1(conn);
		} else
			sendHandshake0(conn);
	}
	
	(void) packet;
			// Its contents is not interesting.

	return 0;
}

int
PacketHandler_SeedRandom(NetConnection *conn,
		const Packet_SeedRandom *packet) {
	BattleStateData *battleStateData;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_preBattle &&
			!conn->stateFlags.discriminant, PACKET_SEEDRANDOM))
		return -1;  // errno is set

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	updateRandomSeed (battleStateData->meleeState, conn->player,
			ntoh32(packet->seed));
	
	conn->stateFlags.agreement.randomSeed = true;
	return 0;
}

int
PacketHandler_InputDelay(NetConnection *conn,
		const Packet_InputDelay *packet) {
	BattleStateData *battleStateData;
	uint32 delay;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_preBattle, PACKET_INPUTDELAY))
		return -1;  // errno is set

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	delay = ntoh32(packet->delay);
	if (delay > BATTLE_FRAME_RATE) {
		log_add(log_Error, "NETPLAY: [%d]     Received absurdly large "
				"input delay value (%d).\n", conn->player, delay);
		return -1;
	}
	conn->stateFlags.inputDelay = delay;
	
	return 0;
}

int
PacketHandler_SelectShip(NetConnection *conn,
		const Packet_SelectShip *packet) {
	bool updateResult;
	BattleStateData *battleStateData;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_selectShip, PACKET_SELECTSHIP))
		return -1;  // errno is set

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	updateResult = updateMeleeSelection(battleStateData->getMeleeState,
			conn->player, ntoh16(packet->ship));
	if (!updateResult)
	{
		errno = EBADMSG;
		return -1;
	}

	return 0;
}

int
PacketHandler_BattleInput(NetConnection *conn,
		const Packet_BattleInput *packet) {
	BATTLE_INPUT_STATE input;
	BattleInputBuffer *bib;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_inBattle ||
			conn->state == NetState_endingBattle ||
			conn->state == NetState_endingBattle2, PACKET_BATTLEINPUT))
		return -1;  // errno is set

	input = (BATTLE_INPUT_STATE) packet->state;
	bib = getBattleInputBuffer(conn->player);
	if (!BattleInputBuffer_push(bib, input)) {
		// errno is set
		return -1;
	}

	return 0;
}

int
PacketHandler_FrameCount(NetConnection *conn,
		const Packet_FrameCount *packet) {
	BattleStateData *battleStateData;
	BattleFrameCounter frameCount;

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(conn->state == NetState_endingBattle,
			PACKET_FRAMECOUNT))
		return -1;  // errno is set
	
	frameCount = (BattleFrameCounter) ntoh32(packet->frameCount);
#ifdef NETPLAY_DEBUG
	log_add(log_Debug, "NETPLAY: [%d] <== Received battleFrameCount %u.\n",
			conn->player, (unsigned int) frameCount);
#endif

	battleStateData = (BattleStateData *) NetConnection_getStateData(conn);
	if (frameCount > battleStateData->endFrameCount)
		battleStateData->endFrameCount = frameCount;
	Netplay_remoteReady(conn);

	return 0;
}

int
PacketHandler_Checksum(NetConnection *conn, const Packet_Checksum *packet) {
#ifdef NETPLAY_CHECKSUM
	uint32 frameNr;
	uint32 checksum;
	size_t delay;
	size_t interval;
#endif

	if (conn->stateFlags.reset.localReset)
		return 0;
	if (conn->stateFlags.reset.remoteReset) {
		errno = EBADMSG;
		return -1;
	}

	if (!testNetState(NetState_battleActive(conn->state), PACKET_CHECKSUM))
		return -1;  // errno is set
	
#ifdef NETPLAY_CHECKSUM
	frameNr = ntoh32(packet->frameNr);
	checksum = ntoh32(packet->checksum);
	interval = NetConnection_getChecksumInterval(conn);
	delay = getBattleInputDelay();

	if (frameNr % interval != 0) {
		log_add(log_Warning, "NETPLAY: [%d] <== Received checksum "
				"for frame %u, while we only expect checksums on frames "
				"divisable by %u -- discarding.\n", conn->player,
				(unsigned int) frameNr, interval);
		return 0;
				// No need to close the connection; checksums are not
				// essential.
	}

	// The checksum is sent at the beginning of a frame.
	// If we're in frame n and have sent our input already,
	// the remote side has got enough input to progress delay + 1 frames from
	// frame n. The next frame is then n + delay + 1, for which we can
	// receive a checksum.
	if (frameNr > battleFrameCount + delay + 1) {
		log_add(log_Warning, "NETPLAY: [%d] <== Received checksum "
				"for a frame too far in the future (frame %u, current "
				"is %u, input delay is %u) -- discarding.\n", conn->player,
				(unsigned int) frameNr, battleFrameCount, delay);
		return 0;
				// No need to close the connection; checksums are not
				// essential.
	}

	// We can progress delay more frames after the last frame for which we
	// received input. If we call that frame n, we can complete frames
	// n through n + delay - 1. While we are waiting for the next input,
	// in frame n + delay,  we will first receive the checksum that the
	// remote side sent at the start of frame n + 1.
	// In this situation frameNr is n + 1, and battleFrameCount is
	// n + delay.
	if (frameNr + delay < battleFrameCount) {
		log_add(log_Warning, "NETPLAY: [%d] <== Received checksum "
				"for a frame too far in the past (frame %u, current "
				"is %u, input delay is %u) -- discarding.", conn->player,
				(unsigned int) frameNr, battleFrameCount, delay);
		return 0;
				// No need to close the connection; checksums are not
				// essential.
	}

	addRemoteChecksum(conn, frameNr, checksum);
#endif

#ifndef NETPLAY_CHECKSUM
	(void) packet;
#endif
	return 0;
}

int
PacketHandler_Abort(NetConnection *conn, const Packet_Abort *packet) {
	abortFeedback(conn, packet->reason);

	return -1;
			// Close connection.
}

int
PacketHandler_Reset(NetConnection *conn, const Packet_Reset *packet) {
	NetplayResetReason reason;

	if (!testNetState(!conn->stateFlags.reset.remoteReset, PACKET_RESET))
		return -1;  // errno is set

	reason = ntoh16(packet->reason);

	Netplay_remoteReset(conn, reason);
	return 0;
}


