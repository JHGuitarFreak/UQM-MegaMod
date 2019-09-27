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

#define RESOLVE_INTERNAL
#include "resolve.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#define DEBUG_RESOLVE_REF
#ifdef DEBUG_RESOLVE_REF
#	include "types.h"
#	include "libs/log.h"
#	include <string.h>
#endif

static ResolveState *
ResolveState_new(void) {
	return (ResolveState *) malloc(sizeof (ResolveState));
}

static void
ResolveState_free(ResolveState *resolveState) {
	free(resolveState);
}

static void
ResolveState_delete(ResolveState *resolveState) {
	assert(resolveState->callbackID == CallbackID_invalid);
	ResolveState_free(resolveState);
}

void
ResolveState_incRef(ResolveState *resolveState) {
	assert(resolveState->refCount < REFCOUNT_MAX);
	resolveState->refCount++;
#ifdef DEBUG_RESOLVE_REF
	log_add(log_Debug, "ResolveState %08" PRIxPTR ": ref++ (%d)",
			(uintptr_t) resolveState, resolveState->refCount);
#endif
}

bool
ResolveState_decRef(ResolveState *resolveState) {
	assert(resolveState->refCount > 0);
	resolveState->refCount--;
#ifdef DEBUG_RESOLVE_REF
	log_add(log_Debug, "ResolveState %08" PRIxPTR ": ref-- (%d)",
			(uintptr_t) resolveState, resolveState->refCount);
#endif
	if (resolveState->refCount == 0) {
		ResolveState_delete(resolveState);
		return true;
	}
	return false;
}

void
ResolveState_setExtra(ResolveState *resolveState, void *extra) {
	resolveState->extra = extra;
}

void *
ResolveState_getExtra(ResolveState *resolveState) {
	return resolveState->extra;
}

static void
doResolveCallback(ResolveState *resolveState) {
	ResolveState_incRef(resolveState);
	(*resolveState->callback)(resolveState, resolveState->result);
	{
		bool released = ResolveState_decRef(resolveState);
		assert(released);
		(void) released;  // In case assert() evaluates to nothing.
	}
}

static void
doResolveErrorCallback(ResolveState *resolveState) {
	ResolveState_incRef(resolveState);
	resolveState->errorCallback(resolveState, &resolveState->error);
	{
		bool released = ResolveState_decRef(resolveState);
		assert(released);
		(void) released;  // In case assert() evaluates to nothing.
	}
}

static void
resolveCallback(ResolveState *resolveState) {
	resolveState->callbackID = CallbackID_invalid;
	if (resolveState->error.gaiRes == 0) {
		// Successful lookup.
		doResolveCallback(resolveState);
	} else {
		// Lookup failed.
		doResolveErrorCallback(resolveState);
	}
}

// Function that does getaddrinfo() and calls the callback function when
// the result is available.
// TODO: actually make this function asynchronous. Right now it just calls
// getaddrinfo() (which blocks) and then schedules the callback.
ResolveState *
getaddrinfoAsync(const char *node, const char *service,
		const struct addrinfo *hints, ResolveFlags *flags,
		ResolveCallback callback, ResolveErrorCallback errorCallback,
		ResolveCallbackArg extra) {
	ResolveState *resolveState;

	resolveState = ResolveState_new();
	resolveState->refCount = 1;
#ifdef DEBUG_RESOLVE_REF
	log_add(log_Debug, "ResolveState %08" PRIxPTR ": ref=1 (%d)",
			(uintptr_t) resolveState, resolveState->refCount);
#endif
	resolveState->state = Resolve_resolving;
	resolveState->flags = *flags;
	resolveState->callback = callback;
	resolveState->errorCallback = errorCallback;
	resolveState->extra = extra;
	resolveState->result = NULL;
	
	resolveState->error.gaiRes =
			getaddrinfo(node, service, hints, &resolveState->result);
	resolveState->error.err = errno;

	resolveState->callbackID = Callback_add(
			(CallbackFunction) resolveCallback, (CallbackArg) resolveState);

	return resolveState;
}

void
Resolve_close(ResolveState *resolveState) {
	if (resolveState->callbackID != CallbackID_invalid) {
		Callback_remove(resolveState->callbackID);
		resolveState->callbackID = CallbackID_invalid;
	}
	resolveState->state = Resolve_closed;
	ResolveState_decRef(resolveState);
}

// Split an addrinfo list into two separate lists, one with structures with
// the specified value for the ai_family field, the other with the other
// structures. The order of entries in the resulting lists will remain
// the same as in the original list.
// info - the original list
// family - the family for the first list
// selected - pointer to where the list of selected structures should be
//            stored
// selectedEnd - pointer to where the end of 'selected' should be stored
// rest - pointer to where the list of not-selected structures should be
//        stored
// restEnd - pointer to where the end of 'rest' should be stored
//
// Yes, it is allowed to modify the ai_next field of addrinfo structures.
// Or at least, it's not disallowed by RFC 3493, and the following
// text from that RFC seems to suggest it should be possible:
// ] The freeaddrinfo() function must support the freeing of arbitrary
// ] sublists of an addrinfo list originally returned by getaddrinfo().
void
splitAddrInfoOnFamily(struct addrinfo *info, int family,
		struct addrinfo **selected, struct addrinfo ***selectedEnd,
		struct addrinfo **rest, struct addrinfo ***restEnd) {
	struct addrinfo *selectedFirst;
	struct addrinfo **selectedNext;
	struct addrinfo *restFirst;
	struct addrinfo **restNext;

	selectedNext = &selectedFirst;
	restNext = &restFirst;
	while (info != NULL) {
		if (info->ai_family == family) {
			*selectedNext = info;
			selectedNext = &(*selectedNext)->ai_next;
		} else {
			*restNext = info;
			restNext = &(*restNext)->ai_next;
		}
		info = info->ai_next;
	}
	*selectedNext = NULL;
	*restNext = NULL;

	// Fill in the result parameters.
	*selected = selectedFirst;
	*selectedEnd = selectedNext;
	*rest = restFirst;
	*restEnd = restNext;
}


