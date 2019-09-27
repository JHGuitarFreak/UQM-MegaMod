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

#ifndef LIBS_NETWORK_CONNECT_LISTEN_H_
#define LIBS_NETWORK_CONNECT_LISTEN_H_

typedef struct ListenFlags ListenFlags;
typedef enum {
	Listen_closed,
	Listen_resolving,
	Listen_listening
} ListenStateState;
typedef struct ListenError ListenError;
typedef struct ListenState ListenState;

#include "port.h"

#ifdef USE_WINSOCK
// I do not want to include winsock2.h, because of possible conflicts with
// code that includes this file.
// Note that listen.c itself includes winsock2.h; SOCKLEN_T is used there
// only where necessary to keep the API consistent.
#	define SOCKLEN_T size_t
struct sockaddr;
#else
#	include <netinet/in.h>
#	define SOCKLEN_T socklen_t
#endif

#include "resolve.h"
#include "../socket/socket.h"

#include "../netmanager/netmanager.h"

// For listenPort()
struct ListenFlags {
	ProtocolFamily familyDemand;
			// Only accept a protocol family of this type.
			// One of PF_inet, PF_inet6, or PF_unspec.
	ProtocolFamily familyPrefer;
			// Prefer a protocol family of this type.
			// One of PF_inet, PF_inet6, or PF_unspec.
	int backlog;
			// As the 2rd parameter to listen();
};

struct ListenError {
	ListenStateState state;
			// State where the error occured.
	int err;
			// errno value. Not relevant if state == resolving unless
			// gaiRes == EAI_SYSTEM.
	const ResolveError *resolveError;
			// Only relevant if state == resolving.
};

typedef void (*ListenConnectCallback)(ListenState *listenState,
		NetDescriptor *listenNd, NetDescriptor *newNd,
		const struct sockaddr *addr, SOCKLEN_T addrLen);
typedef void (*ListenErrorCallback)(ListenState *listenState,
		const ListenError *error);

#ifdef LISTEN_INTERNAL
struct ListenState {
	RefCount refCount;

	ListenStateState state;
	ListenFlags flags;

	ListenConnectCallback connectCallback;
	ListenErrorCallback errorCallback;
	void *extra;

	ResolveState *resolveState;
	NetDescriptor **nds;
	size_t numNd;
	// INV: (numNd == NULL) == (nds == NULL)
};
#endif  /* defined(LISTEN_INTERNAL) */

ListenState *listenPort(const char *service, Protocol proto,
		const ListenFlags *flags, ListenConnectCallback connectCallback,
		ListenErrorCallback errorCallback, void *extra);
void ListenState_close(ListenState *listenState);
void ListenState_incRef(ListenState *listenState);
bool ListenState_decRef(ListenState *listenState);
void ListenState_setExtra(ListenState *listenState, void *extra);
void *ListenState_getExtra(ListenState *listenState);

#endif  /* LIBS_NETWORK_CONNECT_LISTEN_H_ */

