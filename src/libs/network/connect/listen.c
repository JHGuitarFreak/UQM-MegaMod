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
#include "../netport.h"

#define LISTEN_INTERNAL
#define SOCKET_INTERNAL
#include "listen.h"

#include "resolve.h"
#include "../socket/socket.h"
#include "../netmanager/netmanager.h"
#include "libs/misc.h"
#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#ifdef USE_WINSOCK
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include "../wspiapiwrap.h"
#else
#	include <netdb.h>
#endif
#include <stdlib.h>
#include <string.h>

#define DEBUG_LISTEN_REF
#ifdef DEBUG_LISTEN_REF
#	include "types.h"
#endif


static void acceptCallback(NetDescriptor *nd);
static void doListenErrorCallback(ListenState *listenState,
		const ListenError *error);


static ListenState *
ListenState_alloc(void) {
	return (ListenState *) malloc(sizeof (ListenState));
}

static void
ListenState_free(ListenState *listenState) {
	free(listenState);
}

static void
ListenState_delete(ListenState *listenState) {
	assert(listenState->nds == NULL);
	ListenState_free(listenState);
}

void
ListenState_incRef(ListenState *listenState) {
	assert(listenState->refCount < REFCOUNT_MAX);
	listenState->refCount++;
#ifdef DEBUG_LISTEN_REF
	log_add(log_Debug, "ListenState %08" PRIxPTR ": ref++ (%d)",
			(uintptr_t) listenState, listenState->refCount);
#endif
}

bool
ListenState_decRef(ListenState *listenState) {
	assert(listenState->refCount > 0);
	listenState->refCount--;
#ifdef DEBUG_LISTEN_REF
	log_add(log_Debug, "ListenState %08" PRIxPTR ": ref-- (%d)",
			(uintptr_t) listenState, listenState->refCount);
#endif
	if (listenState->refCount == 0) {
		ListenState_delete(listenState);
		return true;
	}
	return false;
}

// Decrements ref count byh 1
void
ListenState_close(ListenState *listenState) {
	if (listenState->resolveState != NULL) {
		Resolve_close(listenState->resolveState);
		listenState->resolveState = NULL;
	}
	if (listenState->nds != NULL) {
		while (listenState->numNd > 0) {
			listenState->numNd--;
			NetDescriptor_close(listenState->nds[listenState->numNd]);
		}
		free(listenState->nds);
		listenState->nds = NULL;
	}
	listenState->state = Listen_closed;
	ListenState_decRef(listenState);
}

void
ListenState_setExtra(ListenState *listenState, void *extra) {
	listenState->extra = extra;
}

void *
ListenState_getExtra(ListenState *listenState) {
	return listenState->extra;
}

static NetDescriptor *
listenPortSingle(struct ListenState *listenState, struct addrinfo *info) {
	Socket *sock;
	int bindResult;
	int listenResult;
	NetDescriptor *nd;

	sock = Socket_openNative(info->ai_family, info->ai_socktype,
			info->ai_protocol);
	if (sock == Socket_noSocket) {
		int savedErrno = errno;
		log_add(log_Error, "socket() failed: %s.", strerror(errno));
		errno = savedErrno;
		return NULL;
	}

	(void) Socket_setReuseAddr(sock);
			// Ignore errors; it's not a big deal.
	if (Socket_setNonBlocking(sock) == -1) {
		int savedErrno = errno;
		// Error message is already printed.
		Socket_close(sock);
		errno = savedErrno;
		return NULL;
	}
	
	bindResult = Socket_bind(sock, info->ai_addr, info->ai_addrlen);
	if (bindResult == -1) {
		int savedErrno = errno;
		if (errno == EADDRINUSE) {
#ifdef DEBUG
			log_add(log_Warning, "bind() failed: %s.", strerror(errno));
#endif
		} else
			log_add(log_Error, "bind() failed: %s.", strerror(errno));
		Socket_close(sock);
		errno = savedErrno;
		return NULL;
	}

	listenResult = Socket_listen(sock, listenState->flags.backlog);
	if (listenResult == -1) {
		int savedErrno = errno;
		log_add(log_Error, "listen() failed: %s.", strerror(errno));
		Socket_close(sock);
		errno = savedErrno;
		return NULL;
	}

	nd = NetDescriptor_new(sock, (void *) listenState);
	if (nd == NULL) {
		int savedErrno = errno;
		log_add(log_Error, "NetDescriptor_new() failed: %s.",
				strerror(errno));
		Socket_close(sock);
		errno = savedErrno;
		return NULL;
	}
		
	NetDescriptor_setReadCallback(nd, acceptCallback);

	return nd;
}

static void
listenPortMulti(struct ListenState *listenState, struct addrinfo *info) {
	struct addrinfo *addrPtr;
	size_t addrCount;
	size_t addrOkCount;
	NetDescriptor **nds;

	// Count how many addresses we've got.
	addrCount = 0;
	for (addrPtr = info; addrPtr != NULL; addrPtr = addrPtr->ai_next)
		addrCount++;

	// This is where we intend to store the file descriptors of the
	// listening sockets.
	nds = malloc(addrCount * sizeof listenState->nds[0]);

	// Bind to each address.
	addrOkCount = 0;
	for (addrPtr = info; addrPtr != NULL; addrPtr = addrPtr->ai_next) {
		NetDescriptor *nd;
		nd = listenPortSingle(listenState, addrPtr);
		if (nd == NULL) {
			// Failed. On to the next.
			// An error message is already printed for serious errors.
			// If the address is already in use, we here also print
			// a message when we are not already listening on one of
			// the other addresses.
			// This is because on some IPv6 capabable systems (like Linux),
			// IPv6 sockets also handle IPv4 connections, which means
			// that a separate IPv4 socket won't be able to bind to the
			// port.
			// BUG: if the IPv4 socket is in the list before the
			// IPv6 socket, it will be the IPv6 which will fail to bind,
			// so only IPv4 connections will be handled, as v4 sockets can't
			// accept v6 connections.
			// In practice, on Linux, I haven't seen it happen, but
			// it's a real possibility.
			if (errno == EADDRINUSE && addrOkCount == 0) {
				log_add(log_Error, "Error while preparing a network socket "
						"for incoming connections: %s", strerror(errno));
			}
			continue;
		}
		
		nds[addrOkCount] = nd;
		addrOkCount++;
	}

	freeaddrinfo(info);

	listenState->nds =
			realloc(nds, addrOkCount * sizeof listenState->nds[0]);
	listenState->numNd = addrOkCount;

	if (addrOkCount == 0) {
		// Could not listen on any port.
		ListenError error;
		error.state = Listen_listening;
		error.err = EIO;
				// Nothing better to offer.
		doListenErrorCallback(listenState, &error);
		return;
	}
}

static void
listenPortResolveCallback(ResolveState *resolveState,
		struct addrinfo *result) {
	ListenState *listenState =
			(ListenState *) ResolveState_getExtra(resolveState);
	Resolve_close(listenState->resolveState);
	listenState->resolveState = NULL;
	listenState->state = Listen_listening;
	listenPortMulti(listenState, result);
}

static void
listenPortResolveErrorCallback(ResolveState *resolveState,
		const ResolveError *resolveError) {
	ListenState *listenState =
			(ListenState *) ResolveState_getExtra(resolveState);
	ListenError listenError;

	assert(resolveError->gaiRes != 0);

	listenError.state = Listen_resolving;
	listenError.resolveError = resolveError;
	listenError.err = resolveError->err;
	doListenErrorCallback(listenState, &listenError);
}

// 'proto' is one of IPProto_tcp or IPProto_udp.
ListenState *
listenPort(const char *service, Protocol proto, const ListenFlags *flags,
		ListenConnectCallback connectCallback,
		ListenErrorCallback errorCallback, void *extra) {
	struct addrinfo	hints;
	ListenState *listenState;
	ResolveFlags resolveFlags;
			// Structure is empty (for now).

	assert(flags->familyDemand == PF_inet ||
			flags->familyDemand == PF_inet6 ||
			flags->familyDemand == PF_unspec);
	assert(flags->familyPrefer == PF_inet ||
			flags->familyPrefer == PF_inet6 ||
			flags->familyPrefer == PF_unspec);
	assert(proto == IPProto_tcp || proto == IPProto_udp);

	// Acquire a list of addresses to bind to.
	memset(&hints, '\0', sizeof hints);
	hints.ai_family = protocolFamilyTranslation[flags->familyDemand];
	hints.ai_protocol = protocolTranslation[proto];

	if (proto == IPProto_tcp) {
		hints.ai_socktype = SOCK_STREAM;
	} else {
		assert(proto == IPProto_udp);
		hints.ai_socktype = SOCK_DGRAM;
	}
	hints.ai_flags = AI_PASSIVE;

	listenState = ListenState_alloc();
	listenState->refCount = 1;
#ifdef DEBUG_LISTEN_REF
	log_add(log_Debug, "ListenState %08" PRIxPTR ": ref=1 (%d)",
			(uintptr_t) listenState, listenState->refCount);
#endif
	listenState->state = Listen_resolving;
	listenState->flags = *flags;
	listenState->connectCallback = connectCallback;
	listenState->errorCallback = errorCallback;
	listenState->extra = extra;
	listenState->nds = NULL;
	listenState->numNd = 0;

	listenState->resolveState = getaddrinfoAsync(NULL, service, &hints,
			&resolveFlags, listenPortResolveCallback,
			listenPortResolveErrorCallback,
			(ResolveCallbackArg) listenState);

	return listenState;
}

// NB: The callback function becomes the owner of newNd.
static void
doListenConnectCallback(ListenState *listenState, NetDescriptor *listenNd,
		NetDescriptor *newNd,
		const struct sockaddr *addr, SOCKLEN_T addrLen) {
	assert(listenState->connectCallback != NULL);

	ListenState_incRef(listenState);
	// No need to increment listenNd, as there's guaranteed to be one
	// reference from listenState. And no need to increment newNd,
	// as the callback function takes over ownership.
	(*listenState->connectCallback)(listenState, listenNd, newNd,
			addr, (socklen_t) addrLen);
	ListenState_decRef(listenState);
}

static void
doListenErrorCallback(ListenState *listenState, const ListenError *error) {
	assert(listenState->errorCallback != NULL);

	ListenState_incRef(listenState);
	(*listenState->errorCallback)(listenState, error);
	ListenState_decRef(listenState);
}

static void
acceptSingleConnection(ListenState *listenState, NetDescriptor *nd) {
	Socket *sock;
	Socket *acceptResult;
	struct sockaddr_storage addr;
	socklen_t addrLen;
	NetDescriptor *newNd;

	sock = NetDescriptor_getSocket(nd);
	addrLen = sizeof (addr);
	acceptResult = Socket_accept(sock, (struct sockaddr *) &addr, &addrLen);
	if (acceptResult == Socket_noSocket) {
		switch (errno) {
			case EWOULDBLOCK:
			case ECONNABORTED:
				// Nothing serious. Keep listening.
				return;
			case EMFILE:
			case ENFILE:
			case ENOBUFS:
			case ENOMEM:
#ifdef ENOSR
			case ENOSR:
#endif
				// Serious problems, but future connections may still
				// be possible.
				log_add(log_Warning, "accept() reported '%s'",
						strerror(errno));
				return;
			default:
				// Should not happen.
				log_add(log_Fatal, "Internal error: accept() reported "
						"'%s'", strerror(errno));
				explode();
		}
	}

	(void) Socket_setReuseAddr(acceptResult);
			// Ignore errors; it's not a big deal.
	if (Socket_setNonBlocking(acceptResult) == -1) {
		int savedErrno = errno;
		log_add(log_Error, "Could not make socket non-blocking: %s.",
				strerror(errno));
		Socket_close(acceptResult);
		errno = savedErrno;
		return;
	}
	(void) Socket_setInlineOOB(acceptResult);
			// Ignore errors; it's not a big deal as the other
			// party is not not supposed to send any OOB data.
	(void) Socket_setKeepAlive(sock);
			// Ignore errors; it's not a big deal.

#ifdef DEBUG
	{
		char hostname[NI_MAXHOST];
		int gniRes;

		gniRes = getnameinfo((struct sockaddr *) &addr, addrLen,
				hostname, sizeof hostname, NULL, 0, 0);
		if (gniRes != 0) {
			log_add(log_Error, "Error while performing hostname "
					"lookup for incoming connection: %s",
					(gniRes == EAI_SYSTEM) ? strerror(errno) :
					gai_strerror(gniRes));
		} else {
			log_add(log_Debug, "Accepted incoming connection from '%s'.",
					hostname);
		}
	}
#endif
	
	newNd = NetDescriptor_new(acceptResult, NULL);
	if (newNd == NULL) {
		int savedErrno = errno;
		log_add(log_Error, "NetDescriptor_new() failed: %s.",
				strerror(errno));
		Socket_close(acceptResult);
		errno = savedErrno;
		return;
	}

	doListenConnectCallback(listenState, nd, newNd,
			(struct sockaddr *) &addr, addrLen);
	// NB: newNd is now handed over to the callback function, and should
	//     no longer be referenced from here.
}

// Called when select() has indicated readability on a listening socket,
// i.e. when a connection is in the queue.
static void
acceptCallback(NetDescriptor *nd) {
	ListenState *listenState = (ListenState *) NetDescriptor_getExtra(nd);

	acceptSingleConnection(listenState, nd);
}


