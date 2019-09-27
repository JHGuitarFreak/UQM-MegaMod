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

#ifndef LIBS_NETWORK_CONNECT_CONNECT_H_
#define LIBS_NETWORK_CONNECT_CONNECT_H_


typedef struct ConnectFlags ConnectFlags;
typedef struct ConnectError ConnectError;
typedef struct ConnectState ConnectState;

typedef enum {
	Connect_closed,
	Connect_resolving,
	Connect_connecting
} ConnectStateState;


#include "../netmanager/netmanager.h"
#include "../socket/socket.h"
#include "resolve.h"


// For connectHost()
struct ConnectFlags {
	ProtocolFamily familyDemand;
			// Only accept a protocol family of this type.
			// One of PF_inet, PF_inet6, or PF_unspec.
	ProtocolFamily familyPrefer;
			// Prefer a protocol family of this type.
			// One of PF_inet, PF_inet6, or PF_unspec.
	int timeout;
			/* Number of milliseconds before timing out a connection attempt.
			 * Note that if a host has multiple addresses, a connect to that
			 * host will have this timeout *per address*. */
	int retryDelayMs;
			/* Retry connecting this many ms after connecting to the last
			 * address for the specified host fails. Set to Connect_noRetry
			 * to give up after one try. */
#define Connect_noRetry -1
};

struct ConnectError {
	ConnectStateState state;
			// State where the error occured.
	int err;
			// errno value. Not relevant if state == resolving unless
			// gaiRes == EAI_SYSTEM.
	const ResolveError *resolveError;
			// Only relevant if state == resolving.
};

typedef void (*ConnectConnectCallback)(ConnectState *connectState,
		NetDescriptor *nd, const struct sockaddr *addr, socklen_t addrLen);
typedef void (*ConnectErrorCallback)(ConnectState *connectState,
		const ConnectError *error);

#ifdef CONNECT_INTERNAL

#include "libs/alarm.h"

struct ConnectState {
	RefCount refCount;

	ConnectStateState state;
	ConnectFlags flags;

	ConnectConnectCallback connectCallback;
	ConnectErrorCallback errorCallback;
	void *extra;

	struct addrinfo *info;
	struct addrinfo *infoPtr;

	ResolveState *resolveState;

	NetDescriptor *nd;
	Alarm *alarm;
			// Used for both the timeout for a connection attempt
			// and to retry after all addresses have been tried.
};
#endif  /* CONNECT_INTERNAL */

ConnectState *connectHostByName(const char *host, const char *service,
		Protocol proto, const ConnectFlags *flags,
		ConnectConnectCallback connectCallback,
		ConnectErrorCallback errorCallback, void *extra);
void ConnectState_incRef(ConnectState *connectState);
bool ConnectState_decRef(ConnectState *connectState);
void ConnectState_close(ConnectState *connectState);
void ConnectState_setExtra(ConnectState *connectState, void *extra);
void *ConnectState_getExtra(ConnectState *connectState);

#endif  /* LIBS_NETWORK_CONNECT_CONNECT_H_ */


