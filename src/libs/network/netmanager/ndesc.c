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

#define NETDESCRIPTOR_INTERNAL
#include "ndesc.h"

#include "netmanager.h"
#include "libs/callback.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>

#undef DEBUG_NETDESCRIPTOR_REF
#ifdef DEBUG_NETDESCRIPTOR_REF
#	include "libs/log.h"
#	include <inttypes.h>
#endif


static NetDescriptor *
NetDescriptor_alloc(void) {
	return malloc(sizeof (NetDescriptor));
}

static void
NetDescriptor_free(NetDescriptor *nd) {
	free(nd);
}

// Sets the ref count to 1.
NetDescriptor *
NetDescriptor_new(Socket *socket, void *extra) {
	NetDescriptor *nd;

	nd = NetDescriptor_alloc();
	nd->refCount = 1;
#ifdef DEBUG_NETDESCRIPTOR_REF
	log_add(log_Debug, "NetDescriptor %08" PRIxPTR ": ref=1 (%d)",
			(uintptr_t) nd, nd->refCount);
#endif

	nd->flags.closed = false;
	nd->readCallback = NULL;
	nd->writeCallback = NULL;
	nd->exceptionCallback = NULL;
	nd->closeCallback = NULL;
	nd->socket = socket;
	nd->smd = NULL;
	nd->extra = extra;

	if (NetManager_addDesc(nd) == -1) {
		int savedErrno = errno;
		NetDescriptor_free(nd);
		errno = savedErrno;
		return NULL;
	}

	return nd;
}

static void
NetDescriptor_delete(NetDescriptor *nd) {
	assert(nd->socket == Socket_noSocket);
	assert(nd->smd == NULL);

	NetDescriptor_free(nd);
}

// Called from the callback handler.
static void
NetDescriptor_closeCallback(NetDescriptor *nd) {
	if (nd->closeCallback != NULL) {
		// The check is necessary because the close callback may have
		// been removed before it is triggered.
		(*nd->closeCallback)(nd);
	}
	NetDescriptor_decRef(nd);
}

void
NetDescriptor_close(NetDescriptor *nd) {
	assert(!nd->flags.closed);
	assert(nd->socket != Socket_noSocket);

	NetManager_removeDesc(nd);
	(void) Socket_close(nd->socket);
	nd->socket = Socket_noSocket;
	nd->flags.closed = true;
	if (nd->closeCallback != NULL) {
		// Keep one reference around until the close callback has been
		// called.
		(void) Callback_add(
				(CallbackFunction) NetDescriptor_closeCallback,
				(CallbackArg) nd);
	} else
		NetDescriptor_decRef(nd);
}

void
NetDescriptor_incRef(NetDescriptor *nd) {
	assert(nd->refCount < REFCOUNT_MAX);
	nd->refCount++;
#ifdef DEBUG_NETDESCRIPTOR_REF
	log_add(log_Debug, "NetDescriptor %08" PRIxPTR ": ref++ (%d)",
			(uintptr_t) nd, nd->refCount);
#endif
}

// returns true iff the ref counter has reached 0.
bool
NetDescriptor_decRef(NetDescriptor *nd) {
	assert(nd->refCount > 0);
	nd->refCount--;
#ifdef DEBUG_NETDESCRIPTOR_REF
	log_add(log_Debug, "NetDescriptor %08" PRIxPTR ": ref-- (%d)",
			(uintptr_t) nd, nd->refCount);
#endif
	if (nd->refCount == 0) {
		NetDescriptor_delete(nd);
		return true;
	}
	return false;
}

// The socket will no longer be managed by the NetManager.
void
NetDescriptor_detach(NetDescriptor *nd) {
	NetManager_removeDesc(nd);
	nd->socket = Socket_noSocket;
	nd->flags.closed = true;
	NetDescriptor_decRef(nd);
}

Socket *
NetDescriptor_getSocket(NetDescriptor *nd) {
	return nd->socket;
}

void
NetDescriptor_setExtra(NetDescriptor *nd, void *extra) {
	nd->extra = extra;
}

void *
NetDescriptor_getExtra(const NetDescriptor *nd) {
	return nd->extra;
}

void
NetDescriptor_setReadCallback(NetDescriptor *nd,
		NetDescriptor_ReadCallback callback) {
	nd->readCallback = callback;
	if (!nd->flags.closed) {
		if (nd->readCallback != NULL) {
			NetManager_activateReadCallback(nd);
		} else
			NetManager_deactivateReadCallback(nd);
	}
}

void
NetDescriptor_setWriteCallback(NetDescriptor *nd,
		NetDescriptor_WriteCallback callback) {
	nd->writeCallback = callback;
	if (!nd->flags.closed) {
		if (nd->writeCallback != NULL) {
			NetManager_activateWriteCallback(nd);
		} else
			NetManager_deactivateWriteCallback(nd);
	}
}

void
NetDescriptor_setExceptionCallback(NetDescriptor *nd,
		NetDescriptor_ExceptionCallback callback) {
	nd->exceptionCallback = callback;
	if (!nd->flags.closed) {
		if (nd->exceptionCallback != NULL) {
			NetManager_activateExceptionCallback(nd);
		} else
			NetManager_deactivateExceptionCallback(nd);
	}
}

// The close callback is called as a result of a socket being closed, either
// because of a local command or a remote disconnect.
// The close callback will only be scheduled when this happens. The
// callback will not be called until the Callback_process() is called.
void
NetDescriptor_setCloseCallback(NetDescriptor *nd,
		NetDescriptor_CloseCallback callback) {
	nd->closeCallback = callback;
}


