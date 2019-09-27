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

#define SOCKET_INTERNAL
#define NETDESCRIPTOR_INTERNAL
#include "netmanager_bsd.h"
#include "ndesc.h"
#include "../socket/socket.h"

#include "ndesc.h"
#include "types.h"
#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "netmanager_common.ci"
#include "ndindex.ci"


// INV: The following sets only contain sockets present in the netDescriptor
// array.
static fd_set readSet;
static fd_set writeSet;
static fd_set exceptionSet;


void
NetManager_init(void) {
	NDIndex_init();

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_ZERO(&exceptionSet);
}

void
NetManager_uninit(void) {
	NDIndex_uninit();
}

// Register the NetDescriptor with the NetManager.
int
NetManager_addDesc(NetDescriptor *nd) {
	int fd;
	assert(nd->socket != Socket_noSocket);
	assert(!NDIndex_socketRegistered(nd->socket));

	if (NDIndex_registerNDWithSocket(nd->socket, nd) == -1) {
		// errno is set
		return -1;
	}

	fd = nd->socket->fd;
	if (nd->readCallback != NULL)
		FD_SET(fd, &readSet);
	if (nd->writeCallback != NULL)
		FD_SET(fd, &writeSet);
	if (nd->exceptionCallback != NULL)
		FD_SET(fd, &exceptionSet);
	return 0;
}

void
NetManager_removeDesc(NetDescriptor *nd) {
	int fd;
	
	assert(nd->socket != Socket_noSocket);
	assert(NDIndex_getNDForSocket(nd->socket) == nd);

	fd = nd->socket->fd;
	FD_CLR(fd, &readSet);
	FD_CLR(fd, &writeSet);
	FD_CLR(fd, &exceptionSet);

	NDIndex_unregisterNDForSocket(nd->socket);
}

void
NetManager_activateReadCallback(NetDescriptor *nd) {
	FD_SET(nd->socket->fd, &readSet);
}

void
NetManager_deactivateReadCallback(NetDescriptor *nd) {
	FD_CLR(nd->socket->fd, &readSet);
}

void
NetManager_activateWriteCallback(NetDescriptor *nd) {
	FD_SET(nd->socket->fd, &writeSet);
}

void
NetManager_deactivateWriteCallback(NetDescriptor *nd) {
	FD_CLR(nd->socket->fd, &writeSet);
}

void
NetManager_activateExceptionCallback(NetDescriptor *nd) {
	FD_SET(nd->socket->fd, &exceptionSet);
}

void
NetManager_deactivateExceptionCallback(NetDescriptor *nd) {
	FD_CLR(nd->socket->fd, &exceptionSet);
}

// This function may be called again from inside a callback function
// triggered by this function. BUG: This may result in callbacks being
// called multiple times.
// This function should however not be called from multiple threads at once.
int
NetManager_process(uint32 *timeoutMs) {
	struct timeval timeout;
	size_t i;
	int selectResult;
	fd_set newReadSet;
	fd_set newWriteSet;
	fd_set newExceptionSet;
	bool bitSet;

	timeout.tv_sec = *timeoutMs / 1000;
	timeout.tv_usec = (*timeoutMs % 1000) * 1000;

	// Structure assignment:
	newReadSet = readSet;
	newWriteSet = writeSet;
	newExceptionSet = exceptionSet;

	do {
		selectResult = select(NDIndex_getSelectNumND(),
				&newReadSet, &newWriteSet, &newExceptionSet, &timeout);
		// BUG: If select() is restarted because of EINTR, the timeout
		//      may start over. (Linux changes 'timeout' to the time left,
		//      but most other platforms don't.)
	} while (selectResult == -1 && errno == EINTR);
	if (selectResult == -1) {
		int savedErrno = errno;
		log_add(log_Error, "select() failed: %s.", strerror(errno));
		errno = savedErrno;
		*timeoutMs = (timeout.tv_sec * 1000) + (timeout.tv_usec / 1000);
				// XXX: rounding microseconds down. Is that the correct
				// thing to do?
		return -1;
	}

	for (i = 0; i < maxND; i++) {
		NetDescriptor *nd;
	
		if (selectResult == 0) {
			// No more bits set in the fd_sets
			break;
		}

		nd = NDIndex_getNDForSocketFd(i);
		if (nd == NULL)
			continue;

		bitSet = false;
				// Is one of the bits in the fd_sets set?
		
		// A callback may cause a NetDescriptor to be closed. The deletion
		// of the structure will be scheduled, but will still be
		// available at least until this function returns.

		if (FD_ISSET(i, &newExceptionSet))
		{
			bool closed;
			bitSet = true;			
			closed = NetManager_doExceptionCallback(nd);
			if (closed)
				goto next;
		}

		if (FD_ISSET(i, &newWriteSet))
		{
			bool closed;
			bitSet = true;			
			closed = NetManager_doWriteCallback(nd);
			if (closed)
				goto next;
		}

		if (FD_ISSET(i, &newReadSet))
		{
			bool closed;
			bitSet = true;			
			closed = NetManager_doReadCallback(nd);
			if (closed)
				goto next;
		}

next:
		if (bitSet)
			selectResult--;
	}

	*timeoutMs = (timeout.tv_sec * 1000) + (timeout.tv_usec / 1000);
			// XXX: rounding microseconds down. Is that the correct
			// thing to do?
	return 0;
}



