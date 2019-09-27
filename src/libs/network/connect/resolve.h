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

#ifndef LIBS_NETWORK_CONNECT_RESOLVE_H_
#define LIBS_NETWORK_CONNECT_RESOLVE_H_


typedef struct ResolveFlags ResolveFlags;
typedef struct ResolveError ResolveError;
typedef enum {
	Resolve_closed,
	Resolve_resolving
} ResolveStateState;
typedef struct ResolveState ResolveState;

#include "port.h"
#include "../netport.h"

// For addrinfo.
#ifdef USE_WINSOCK
// Not including <winsock2.h> because of possible conflicts with files
// including this file.
struct addrinfo;
#else
#	include <netdb.h>
#endif

#include "libs/callback.h"
#include "../netmanager/netmanager.h"


struct ResolveFlags {
	// Nothing yet.

	int dummy;  // empty struct declarations are not allowed by C'99.
};

struct ResolveError {
	int gaiRes;
	int err;
			// errno value. Only relevant if gaiRes == EAI_SYSTEM.
};

typedef void *ResolveCallbackArg;
typedef void (*ResolveCallback)(ResolveState *resolveState,
		struct addrinfo *result);
		// The receiver of the callback is owner of 'result' and
		// should call freeaddrinfo().
typedef void (*ResolveErrorCallback)(ResolveState *resolveState,
		const ResolveError *error);

#ifdef RESOLVE_INTERNAL
#ifdef USE_WINSOCK
#	include <winsock2.h>
#	include <ws2tcpip.h>
#	include "../wspiapiwrap.h"
#else  /* !defined(USE_WINSOCK) */
#	include <netdb.h>
#endif /* !defined(USE_WINSOCK) */

struct ResolveState {
	RefCount refCount;

	ResolveStateState state;
	ResolveFlags flags;

	ResolveCallback callback;
	ResolveErrorCallback errorCallback;
	void *extra;

	CallbackID callbackID;
	ResolveError error;
	struct addrinfo *result;
};
#endif  /* RESOLVE_INTERNAL */


void ResolveState_incRef(ResolveState *resolveState);
bool ResolveState_decRef(ResolveState *resolveState);
void ResolveState_setExtra(ResolveState *resolveState, void *extra);
void *ResolveState_getExtra(ResolveState *resolveState);
ResolveState *getaddrinfoAsync(const char *node, const char *service,
		const struct addrinfo *hints, ResolveFlags *flags,
		ResolveCallback callback, ResolveErrorCallback errorCallback,
		ResolveCallbackArg extra);
void Resolve_close(ResolveState *resolveState);

void splitAddrInfoOnFamily(struct addrinfo *info, int family,
		struct addrinfo **selected, struct addrinfo ***selectedEnd,
		struct addrinfo **rest, struct addrinfo ***restEnd);


#endif  /* LIBS_NETWORK_CONNECT_RESOLVE_H_ */

