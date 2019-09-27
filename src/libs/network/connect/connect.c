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

#define CONNECT_INTERNAL
#define SOCKET_INTERNAL
#include "connect.h"

#include "resolve.h"
#include "libs/alarm.h"
#include "../socket/socket.h"
#include "libs/misc.h"
#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#ifdef USE_WINSOCK
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include "../wspiapiwrap.h"
#else
#	include <netdb.h>
#endif

#define DEBUG_CONNECT_REF
#ifdef DEBUG_CONNECT_REF
#	include "types.h"
#endif


static void connectHostNext(ConnectState *connectState);
static void doConnectCallback(ConnectState *connectState, NetDescriptor *nd,
		const struct sockaddr *addr, socklen_t addrLen);
static void doConnectErrorCallback(ConnectState *connectState,
		const ConnectError *error);


static ConnectState *
ConnectState_alloc(void) {
	return (ConnectState *) malloc(sizeof (ConnectState));
}

static void
ConnectState_free(ConnectState *connectState) {
	free(connectState);
}

static void
ConnectState_delete(ConnectState *connectState) {
	assert(connectState->nd == NULL);
	assert(connectState->alarm == NULL);
	assert(connectState->info == NULL);
	assert(connectState->infoPtr == NULL);
	ConnectState_free(connectState);
}

void
ConnectState_incRef(ConnectState *connectState) {
	assert(connectState->refCount < REFCOUNT_MAX);
	connectState->refCount++;
#ifdef DEBUG_CONNECT_REF
	log_add(log_Debug, "ConnectState %08" PRIxPTR ": ref++ (%d)",
			(uintptr_t) connectState, connectState->refCount);
#endif
}

bool
ConnectState_decRef(ConnectState *connectState) {
	assert(connectState->refCount > 0);
	connectState->refCount--;
#ifdef DEBUG_CONNECT_REF
	log_add(log_Debug, "ConnectState %08" PRIxPTR ": ref-- (%d)",
			(uintptr_t) connectState, connectState->refCount);
#endif
	if (connectState->refCount == 0) {
		ConnectState_delete(connectState);
		return true;
	}
	return false;
}

// decrements ref count by 1
void
ConnectState_close(ConnectState *connectState) {
	if (connectState->resolveState != NULL) {
		Resolve_close(connectState->resolveState);
		connectState->resolveState = NULL;
	}
	if (connectState->alarm != NULL) {
		Alarm_remove(connectState->alarm);
		connectState->alarm = NULL;
	}
	if (connectState->nd != NULL) {
		NetDescriptor_close(connectState->nd);
		connectState->nd = NULL;
	}
	if (connectState->info != NULL) {
		freeaddrinfo(connectState->info);
		connectState->info = NULL;
		connectState->infoPtr = NULL;
	}
	connectState->state = Connect_closed;
	ConnectState_decRef(connectState);
}

void
ConnectState_setExtra(ConnectState *connectState, void *extra) {
	connectState->extra = extra;
}

void *
ConnectState_getExtra(ConnectState *connectState) {
	return connectState->extra;
}

static void
connectCallback(NetDescriptor *nd) {
	// Called by the NetManager when a connection has been established.
	ConnectState *connectState =
			(ConnectState *) NetDescriptor_getExtra(nd);
	int err;

	if (connectState->alarm != NULL) {
		Alarm_remove(connectState->alarm);
		connectState->alarm = NULL;
	}

	if (connectState->state == Connect_closed) {
		// The connection attempt has been aborted.
#ifdef DEBUG
		log_add(log_Debug, "Connection attempt was aborted.");
#endif
		ConnectState_decRef(connectState);
		return;
	}

	if (Socket_getError(NetDescriptor_getSocket(nd), &err) == -1) {
		log_add(log_Fatal, "Socket_getError() failed: %s.",
				strerror(errno));
		explode();
	}
	if (err != 0) {
#ifdef DEBUG
		log_add(log_Debug, "connect() failed: %s.", strerror(err));
#endif
		NetDescriptor_close(nd);
		connectState->nd = NULL;
		connectState->infoPtr = connectState->infoPtr->ai_next;
		connectHostNext(connectState);
		return;
	}

#ifdef DEBUG
	log_add(log_Debug, "Connection established.");
#endif

	// Notify the higher layer.
	connectState->nd = NULL;
			// The callback function takes over ownership of the
			// NetDescriptor.
	NetDescriptor_setWriteCallback(nd, NULL);
	// Note that connectState->info and connectState->infoPtr are cleaned up
	// when ConnectState_close() is called by the callback function.
	
	ConnectState_incRef(connectState);
	doConnectCallback(connectState, nd, connectState->infoPtr->ai_addr,
			connectState->infoPtr->ai_addrlen);
	{
		// The callback called should release the last reference to
		// the connectState, by calling ConnectState_close().
		bool released = ConnectState_decRef(connectState);
		assert(released);
		(void) released;  // In case assert() evaluates to nothing.
	}
}

static void
connectTimeoutCallback(ConnectState *connectState) {
	connectState->alarm = NULL;

	NetDescriptor_close(connectState->nd);
	connectState->nd = NULL;

	connectState->infoPtr = connectState->infoPtr->ai_next;
	connectHostNext(connectState);
}

static void
setConnectTimeout(ConnectState *connectState) {
	assert(connectState->alarm == NULL);

	connectState->alarm =
			Alarm_addRelativeMs(connectState->flags.timeout,
			(AlarmCallback) connectTimeoutCallback, connectState);
}

// Try connecting to the next address.
static Socket *
tryConnectHostNext(ConnectState *connectState) {
	struct addrinfo *info;
	Socket *sock;
	int connectResult;

	assert(connectState->nd == NULL);

	info = connectState->infoPtr;

	sock = Socket_openNative(info->ai_family, info->ai_socktype,
			info->ai_protocol);
	if (sock == Socket_noSocket) {
		int savedErrno = errno;
		log_add(log_Error, "socket() failed: %s.", strerror(errno));
		errno = savedErrno;
		return Socket_noSocket;
	}
	
	if (Socket_setNonBlocking(sock) == -1) {
		int savedErrno = errno;
		log_add(log_Error, "Could not make socket non-blocking: %s.",
				strerror(errno));
		errno = savedErrno;
		return Socket_noSocket;
	}

	(void) Socket_setReuseAddr(sock);
			// Ignore errors; it's not a big deal.
	(void) Socket_setInlineOOB(sock);
			// Ignore errors; it's not a big deal as the other party is not
			// not supposed to send any OOB data.
	(void) Socket_setKeepAlive(sock);
			// Ignore errors; it's not a big deal.

	connectResult = Socket_connect(sock, info->ai_addr, info->ai_addrlen);
	if (connectResult == 0) {
		// Connection has already succeeded.
		// We just wait for the writability callback anyhow, so that
		// we can use one code path.
		return sock;
	}

	switch (errno) {
		case EINPROGRESS:
			// Connection in progress; wait for the write callback.
			return sock;
	}

	// Connection failed immediately. This is just for one of the addresses,
	// so this does not have to be final.
	// Note that as the socket is non-blocking, most failed connection
	// errors will usually not be reported immediately.
	{
		int savedErrno = errno;
		Socket_close(sock);
#ifdef DEBUG
		log_add(log_Debug, "connect() immediately failed for one address: "
				"%s.", strerror(errno));
				// TODO: add the address in the status message.
#endif
		errno = savedErrno;
	}
	return Socket_noSocket;
}

static void
connectRetryCallback(ConnectState *connectState) {
	connectState->alarm = NULL;

	connectState->infoPtr = connectState->info;
	connectHostNext(connectState);
}

static void
setConnectRetryAlarm(ConnectState *connectState) {
	assert(connectState->alarm == NULL);
	assert(connectState->flags.retryDelayMs != Connect_noRetry);

	connectState->alarm =
			Alarm_addRelativeMs(connectState->flags.retryDelayMs,
			(AlarmCallback) connectRetryCallback, connectState);
}

static void
connectHostReportAllFailed(ConnectState *connectState) {
	// Could not connect to any host.
	ConnectError error;
	freeaddrinfo(connectState->info);
	connectState->info = NULL;
	connectState->infoPtr = NULL;
	connectState->state = Connect_closed;
	error.state = Connect_connecting;
	error.err = ETIMEDOUT;
			// No errno code is exactly suitable. We have been unable
			// to connect to any host, but the reasons may vary
			// (unreachable, refused, ...).
			// ETIMEDOUT is the least specific portable errno code that
			// seems appropriate.
	doConnectErrorCallback(connectState, &error);
}

static void
connectHostNext(ConnectState *connectState) {
	Socket *sock;

	while (connectState->infoPtr != NULL) {
		sock = tryConnectHostNext(connectState);

		if (sock != Socket_noSocket) {
			// Connection succeeded or connection in progress
			connectState->nd =
					NetDescriptor_new(sock, (void *) connectState);
			if (connectState->nd == NULL) {
				ConnectError error;
				int savedErrno = errno;

				log_add(log_Error, "NetDescriptor_new() failed: %s.",
						strerror(errno));
				Socket_close(sock);
				freeaddrinfo(connectState->info);
				connectState->info = NULL;
				connectState->infoPtr = NULL;
				connectState->state = Connect_closed;
				error.state = Connect_connecting;
				error.err = savedErrno;
				doConnectErrorCallback(connectState, &error);
				return;
			}

			NetDescriptor_setWriteCallback(connectState->nd, connectCallback);
			setConnectTimeout(connectState);
			return;
		}

		connectState->infoPtr = connectState->infoPtr->ai_next;
	}

	// Connect failed to all addresses.

	if (connectState->flags.retryDelayMs == Connect_noRetry) {
		connectHostReportAllFailed(connectState);
		return;
	}

	setConnectRetryAlarm(connectState);
}

static void
connectHostResolveCallback(ResolveState *resolveState,
		struct addrinfo *info) {
	ConnectState *connectState =
			(ConnectState *) ResolveState_getExtra(resolveState);

	connectState->state = Connect_connecting;

	Resolve_close(resolveState);
	connectState->resolveState = NULL;

	if (connectState->flags.familyPrefer != PF_UNSPEC) {
		// Reorganise the 'info' list to put the structures of the
		// prefered family in front.
		struct addrinfo *preferred;
		struct addrinfo **preferredEnd;
		struct addrinfo *rest;
		struct addrinfo **restEnd;
		splitAddrInfoOnFamily(info, connectState->flags.familyPrefer,
				&preferred, &preferredEnd, &rest, &restEnd);
		info = preferred;
		*preferredEnd = rest;
	}

	connectState->info = info;
	connectState->infoPtr = info;

	connectHostNext(connectState);
}

static void
connectHostResolveErrorCallback(ResolveState *resolveState,
		const ResolveError *resolveError) {
	ConnectState *connectState =
			(ConnectState *) ResolveState_getExtra(resolveState);
	ConnectError connectError;

	assert(resolveError->gaiRes != 0);

	Resolve_close(resolveState);
	connectState->resolveState = NULL;

	connectError.state = Connect_resolving;
	connectError.resolveError = resolveError;
	connectError.err = resolveError->err;
	doConnectErrorCallback(connectState, &connectError);
}

ConnectState *
connectHostByName(const char *host, const char *service, Protocol proto,
		const ConnectFlags *flags, ConnectConnectCallback connectCallback,
		ConnectErrorCallback errorCallback, void *extra) {
	struct addrinfo	hints;
	ConnectState *connectState;
	ResolveFlags resolveFlags;
			// Structure is empty (for now).

	assert(flags->familyDemand == PF_inet ||
			flags->familyDemand == PF_inet6 ||
			flags->familyDemand == PF_unspec);
	assert(flags->familyPrefer == PF_inet ||
			flags->familyPrefer == PF_inet6 ||
			flags->familyPrefer == PF_unspec);
	assert(proto == IPProto_tcp || proto == IPProto_udp);

	memset(&hints, '\0', sizeof hints);
	hints.ai_family = protocolFamilyTranslation[flags->familyDemand];
	hints.ai_protocol = protocolTranslation[proto];

	if (proto == IPProto_tcp) {
		hints.ai_socktype = SOCK_STREAM;
	} else {
		assert(proto == IPProto_udp);
		hints.ai_socktype = SOCK_DGRAM;
	}
	hints.ai_flags = 0;

	connectState = ConnectState_alloc();
	connectState->refCount = 1;
#ifdef DEBUG_CONNECT_REF
	log_add(log_Debug, "ConnectState %08" PRIxPTR ": ref=1 (%d)",
			(uintptr_t) connectState, connectState->refCount);
#endif
	connectState->state = Connect_resolving;
	connectState->flags = *flags;
	connectState->connectCallback = connectCallback;
	connectState->errorCallback = errorCallback;
	connectState->extra = extra;
	connectState->info = NULL;
	connectState->infoPtr = NULL;
	connectState->nd = NULL;
	connectState->alarm = NULL;
	
	connectState->resolveState = getaddrinfoAsync(
			host, service, &hints, &resolveFlags,
			(ResolveCallback) connectHostResolveCallback,
			(ResolveErrorCallback) connectHostResolveErrorCallback,
			(ResolveCallbackArg) connectState);

	return connectState;
}

// NB: The callback function becomes the owner of nd
static void
doConnectCallback(ConnectState *connectState, NetDescriptor *nd,
		const struct sockaddr *addr, socklen_t addrLen) {
	assert(connectState->connectCallback != NULL);

	ConnectState_incRef(connectState);
	// No need to increment nd as the callback function takes over ownership.
	(*connectState->connectCallback)(connectState, nd, addr, addrLen);
	ConnectState_decRef(connectState);
}

static void
doConnectErrorCallback(ConnectState *connectState,
		const ConnectError *error) {
	assert(connectState->errorCallback != NULL);

	ConnectState_incRef(connectState);
	(*connectState->errorCallback)(connectState, error);
	ConnectState_decRef(connectState);
}



