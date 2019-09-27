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

#ifndef UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H_
#define UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H_

#include "netplay.h"
		// for NETPLAY_STATISTICS

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct NetConnection NetConnection;
typedef struct NetConnectionError NetConnectionError;
typedef struct ConnectStateData ConnectStateData;
#ifdef NETPLAY_STATISTICS
typedef struct NetStatistics NetStatistics;
#endif

typedef void (*NetConnection_ConnectCallback)(NetConnection *nd);
typedef void (*NetConnection_CloseCallback)(NetConnection *nd);
typedef void (*NetConnection_ErrorCallback)(NetConnection *nd,
		const NetConnectionError *error);
typedef void (*NetConnection_DeleteCallback)(NetConnection *nd);

typedef void (*NetConnection_ReadyCallback)(NetConnection *conn, void *arg);
typedef void (*NetConnection_ResetCallback)(NetConnection *conn, void *arg);

#if defined(__cplusplus)
}
#endif

#include "netstate.h"
#include "netoptions.h"
#ifdef NETPLAY_CHECKSUM
#	include "checkbuf.h"
#endif
#if defined(NETPLAY_STATISTICS) || defined(NETCONNECTION_INTERNAL)
#	include "packet.h"
#endif
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
#	include "libs/uio.h"
#endif

#if defined(__cplusplus)
extern "C" {
#endif

struct NetConnectionError {
	NetState state;
	int err;
	union {
		const struct ListenError *listenError;
		const struct ConnectError *connectError;
	} extra;
};

#ifdef NETPLAY_STATISTICS
struct NetStatistics {
	size_t packetsReceived;
	size_t packetTypeReceived[PACKET_NUM];
	size_t packetsSent;
	size_t packetTypeSent[PACKET_NUM];
};
#endif

#if defined(__cplusplus)
}
#endif

#ifdef NETCONNECTION_INTERNAL
#include "libs/net.h"
#include "packetq.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct {
	// For actions that require agreement by both parties.
	bool localOk : 1;    /* Action confirmed by us */
	bool remoteOk : 1;   /* Action confirmed by the remote party */
	bool canceling : 1;  /* Awaiting cancel confirmation */
} HandShakeFlags;

typedef struct {
	// For actions that do not require agreement, but for which it
	// only is relevant that both sides are ready.
	bool localReady : 1;
	bool remoteReady : 1;
} ReadyFlags;

typedef struct {
	bool localReset : 1;
	bool remoteReset : 1;
} ResetFlags;

// Which parameters have we both sides of a connection reached agreement on?
typedef struct {
	bool randomSeed : 1;
} Agreement;

typedef struct {
	bool connected;
			/* This NetConnection is connected. */
	bool disconnected;
			/* This NetConnection has been disconnected. This implies
			 * !connected. It is only set if the NetConnection was once
			 * connected, but is no longer. */
	bool discriminant;
			/* If it is true here, it is false on the remote side
			 * of the same connection. It may be used to break ties.
			 * It is guaranteed not to change during a connection. Undefined
			 * while not connected. */
	HandShakeFlags handshake;
	ReadyFlags ready;
	ResetFlags reset;
	Agreement agreement;
	size_t inputDelay;
			/* Used during negotiation of the actual inputDelay. This
			 * field does NOT necessarilly contain the actual input delay,
			 * which is a property of the game, not of any specific
			 * connection. Use getBattleInputDelay() to get at it. */
#ifdef NETPLAY_CHECKSUM
	size_t checksumInterval;
#endif
} NetStateFlags;

struct NetConnection {
	NetDescriptor *nd;
	int player;
			// Number of the player for this connection, as it is
			// known locally. For the other player, it may be
			// differently.
	NetState state;
	NetStateFlags stateFlags;

	NetConnection_ReadyCallback readyCallback;
			// Called when both sides have indicated that they are ready.
			// Set by Netplay_localReady().
	void *readyCallbackArg;
			// Extra argument for readyCallback().
			// XXX: when is this cleaned up if a connection is broken?

	NetConnection_ResetCallback resetCallback;
			// Called when a reset has been signalled and confirmed.
			// Set by Netplay_localReset().
	void *resetCallbackArg;
			// Extra argument for resetCallback().
			// XXX: when is this cleaned up if a connection is broken?

	const NetplayPeerOptions *options;
	PacketQueue queue;
#ifdef NETPLAY_STATISTICS
	NetStatistics statistics;
#endif
#ifdef NETPLAY_CHECKSUM
	ChecksumBuffer checksumBuffer;
#endif
#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
	uio_Stream *debugFile;
#endif

	NetConnection_ConnectCallback connectCallback;
	NetConnection_CloseCallback closeCallback;
			// Called when the NetConnection becomes disconnected.
	NetConnection_ErrorCallback errorCallback;
	NetConnection_DeleteCallback deleteCallback;
			// Called when the NetConnection is destroyed.
	uint8 *readBuf;
	uint8 *readEnd;
	NetConnectionStateData *stateData;
			// State dependant information.
	void *extra;
};

struct ConnectStateData {
	NETCONNECTION_STATE_DATA_COMMON

	bool isServer;
	union {
		struct ConnectState *connectState;
		struct ListenState *listenState;
	} state;
};

#endif  /* NETCONNECTION_INTERNAL */


NetConnection *NetConnection_open(int player,
		const NetplayPeerOptions *options,
		NetConnection_ConnectCallback connectCallback,
		NetConnection_CloseCallback closeCallback,
		NetConnection_ErrorCallback errorCallback,
		NetConnection_DeleteCallback deleteCallback, void *extra);
void NetConnection_close(NetConnection *conn);
bool NetConnection_isConnected(const NetConnection *conn);

void NetConnection_doErrorCallback(NetConnection *nd, int err);

void NetConnection_setStateData(NetConnection *conn,
		NetConnectionStateData *stateData);
NetConnectionStateData *NetConnection_getStateData(const NetConnection *conn);
void NetConnection_setExtra(NetConnection *conn, void *extra);
void *NetConnection_getExtra(const NetConnection *conn);
void NetConnection_setState(NetConnection *conn, NetState state);
NetState NetConnection_getState(const NetConnection *conn);
bool NetConnection_getDiscriminant(const NetConnection *conn);
const NetplayPeerOptions *NetConnection_getPeerOptions(
		const NetConnection *conn);
int NetConnection_getPlayerNr(const NetConnection *conn);
size_t NetConnection_getInputDelay(const NetConnection *conn);
#ifdef NETPLAY_CHECKSUM
ChecksumBuffer *NetConnection_getChecksumBuffer(NetConnection *conn);
size_t NetConnection_getChecksumInterval(const NetConnection *conn);
#endif
#ifdef NETPLAY_STATISTICS
NetStatistics *NetConnection_getStatistics(NetConnection *conn);
#endif

void NetConnection_setReadyCallback(NetConnection *conn,
		NetConnection_ReadyCallback callback, void *arg);
NetConnection_ReadyCallback NetConnection_getReadyCallback(
		const NetConnection *conn);
void *NetConnection_getReadyCallbackArg(const NetConnection *conn);

void NetConnection_setResetCallback(NetConnection *conn,
		NetConnection_ResetCallback callback, void *arg);
NetConnection_ResetCallback NetConnection_getResetCallback(
		const NetConnection *conn);
void *NetConnection_getResetCallbackArg(const NetConnection *conn);


#if defined(NETPLAY_DEBUG) && defined(NETPLAY_DEBUG_FILE)
extern uio_Stream *netplayDebugFile;
#endif

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NETCONNECTION_H_ */


