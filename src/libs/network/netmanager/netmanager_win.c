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

#define NETMANAGER_INTERNAL
#define NETDESCRIPTOR_INTERNAL
#define SOCKET_INTERNAL
#include "netmanager_win.h"
#include "../socket/socket.h"

#include "ndesc.h"
#include "types.h"
#include "libs/misc.h"
#include "libs/log.h"

#include <assert.h>
#include <winsock2.h>

#include "netmanager_common.ci"

int closeWSAEvent(WSAEVENT event);


// The elements of the following arrays with the same index belong to
// eachother.
#define MAX_SOCKETS WSA_MAXIMUM_WAIT_EVENTS
		// We cannot have more sockets than we can have events, as
		// all may need an event at some point.
static NetDescriptor *netDescriptors[MAX_SOCKETS];
static WSAEVENT events[WSA_MAXIMUM_WAIT_EVENTS];
static size_t numSockets;
static size_t numActiveEvents;
// For each NetDescriptor registered with the NetManager, an event
// is created. Only the first numActiveEvents will be processed though.
// Inv: numActiveEvents <= numSockets
// Inv: for all i: 0 <= i < numActiveEvents: events[i] is active
//      (events[i] being active also means netDescriptor[i]->smd->eventMask
//      != 0)
// Inv: for all i: 0 <= i < numSockets: netDescriptor[i]->smd->index == i

void
NetManager_init(void) {
	numActiveEvents = 0;
}

void
NetManager_uninit(void) {
	assert(numActiveEvents == 0);
}

static inline SocketManagementDataWin *
SocketManagementData_alloc(void) {
	return malloc(sizeof (SocketManagementDataWin));
}

static inline void
SocketManagementData_free(SocketManagementDataWin *smd) {
	free(smd);
}

// XXX: This function should be moved to some file with generic network
// functions.
int
closeWSAEvent(WSAEVENT event) {
	for (;;) {
		int error;

		if (WSACloseEvent(event))
			break;

		error = WSAGetLastError();
		if (error != WSAEINPROGRESS) {
			log_add(log_Error,
					"WSACloseEvent() failed with error code %d.", error);
			errno = winsockErrorToErrno(error);
			return -1;
		}
	}
	return 0;
}

// Register the NetDescriptor with the NetManager.
int
NetManager_addDesc(NetDescriptor *nd) {
	long eventMask = 0;
	WSAEVENT event;

	if (numSockets >= WSA_MAXIMUM_WAIT_EVENTS) {
		errno = EMFILE;
		return -1;
	}

	if (nd->readCallback != NULL)
		eventMask |= FD_READ | FD_ACCEPT;

	if (nd->writeCallback != NULL)
		eventMask |= FD_WRITE /* | FD_CONNECT */;

	if (nd->exceptionCallback != NULL)
		eventMask |= FD_OOB;

	eventMask |= FD_CLOSE;

	event = WSACreateEvent();
	if (event == WSA_INVALID_EVENT) {
		errno = getWinsockErrno();
		return -1;
	}

	nd->smd = SocketManagementData_alloc();
	if (eventMask != 0) {
			// XXX: This guard is now always true, because of FD_CLOSE.
			//      This means that numActiveEvents will always be equal to
			//      numEvents.
			//      Once I'm convinced this is the right way to go,
			//      I can remove some unnecessary code.
		if (WSAEventSelect(nd->socket->sock, event, eventMask) ==
				SOCKET_ERROR) {
			int savedErrno = getWinsockErrno();
			int closeStatus = closeWSAEvent(event);
			if (closeStatus == -1) {
				log_add(log_Fatal, "closeWSAEvent() failed: %s.",
						strerror(errno));
				explode();
			}
			SocketManagementData_free(nd->smd);
			errno = savedErrno;
			return -1;
		}

		// Move existing socket for which there exists no event, so
		// so that all sockets for which there exists an event are at
		// the front of the array of netdescriptors.
		if (numActiveEvents < numSockets) {
			netDescriptors[numSockets] = netDescriptors[numActiveEvents];
			netDescriptors[numSockets]->smd->index = numSockets;
		}

		nd->smd->index = numActiveEvents;
		numActiveEvents++;
	} else {
		nd->smd->index = numSockets;
	}
	nd->smd->eventMask = eventMask;

	netDescriptors[nd->smd->index] = nd;
	events[nd->smd->index] = event;
	numSockets++;

	return 0;
}

void
NetManager_removeDesc(NetDescriptor *nd) {
	assert(nd->smd != NULL);
	assert(nd->smd->index < numSockets);
	assert(nd == netDescriptors[nd->smd->index]);

	{
		int closeStatus = closeWSAEvent(events[nd->smd->index]);
		if (closeStatus == -1)
			explode();
	}

	if (nd->smd->index < numActiveEvents) {
		size_t index = nd->smd->index;
		if (index + 1 != numActiveEvents) {
			// Keep the list of active events consecutive by filling
			// the new hole with the last active event.
			events[index] = events[numActiveEvents - 1];
			netDescriptors[index] = netDescriptors[numActiveEvents - 1];
			netDescriptors[index]->smd->index = index;
		}
		numActiveEvents--;
	}

	SocketManagementData_free(nd->smd);
	nd->smd = NULL;

	numSockets--;
}

static void
swapSockets(int index1, int index2) {
	NetDescriptor *tempNd;
	WSAEVENT tempEvent;

	tempNd = netDescriptors[index2];
	tempEvent = events[index2];

	netDescriptors[index2] = netDescriptors[index1];
	events[index2] = events[index1];
	netDescriptors[index2]->smd->index = index2;

	netDescriptors[index1] = tempNd;
	events[index1] = tempEvent;
	netDescriptors[index1]->smd->index = index1;
}

static int
NetManager_updateEvent(NetDescriptor *nd) {
	assert(nd == netDescriptors[nd->smd->index]);

	if (WSAEventSelect(nd->socket->sock,
			events[nd->smd->index], nd->smd->eventMask) == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		int closeStatus = closeWSAEvent(events[nd->smd->index]);
		if (closeStatus == -1) {
			log_add(log_Fatal, "closeWSAEvent() failed: %s.",
					strerror(errno));
			explode();
		}
		errno = savedErrno;
		return -1;
	}

	if (nd->smd->eventMask != 0) {
		// There are some events that we are interested in.
		if (nd->smd->index >= numActiveEvents) {
			// Event was not yet active.
			if (nd->smd->index != numActiveEvents) {
				// Need to keep the active nds and events in the front of
				// their arrays.
				swapSockets(nd->smd->index, numActiveEvents);
			}
			numActiveEvents++;
		}
	} else {
		// There are no events that we are interested in.
		if (nd->smd->index < numActiveEvents) {
			// Event was active.
			if (nd->smd->index != numActiveEvents - 1) {
				// Need to keep the active nds and events in the front of
				// their arrays.
				swapSockets(nd->smd->index, numActiveEvents - 1);
			}
		}
		numActiveEvents--;
	}

	return 0;	
}

static void
activateSomeCallback(NetDescriptor *nd, long eventMask) {
	nd->smd->eventMask |= eventMask;
	{
		int status = NetManager_updateEvent(nd);
		if (status == -1) {
			log_add(log_Fatal, "NetManager_updateEvent() failed: %s.",
					strerror(errno));
			explode();
			// TODO: better error handling.
		}
	}
}

static void
deactivateSomeCallback(NetDescriptor *nd, long eventMask) {
	nd->smd->eventMask &= ~eventMask;
	{
		int status = NetManager_updateEvent(nd);
		if (status == -1) {
			log_add(log_Fatal, "NetManager_updateEvent() failed: %s.",
					strerror(errno));
			explode();
			// TODO: better error handling
		}
	}
}

void
NetManager_activateReadCallback(NetDescriptor *nd) {
	activateSomeCallback(nd, FD_READ | FD_ACCEPT);
}

void
NetManager_deactivateReadCallback(NetDescriptor *nd) {
	deactivateSomeCallback(nd, FD_READ | FD_ACCEPT);
}

void
NetManager_activateWriteCallback(NetDescriptor *nd) {
	activateSomeCallback(nd, FD_WRITE /* | FD_CONNECT */);
}

void
NetManager_deactivateWriteCallback(NetDescriptor *nd) {
	deactivateSomeCallback(nd, FD_WRITE /* | FD_CONNECT */);
}

void
NetManager_activateExceptionCallback(NetDescriptor *nd) {
	activateSomeCallback(nd, FD_OOB);
}

void
NetManager_deactivateExceptionCallback(NetDescriptor *nd) {
	deactivateSomeCallback(nd, FD_OOB);
}

static inline int
NetManager_processEvent(size_t index) {
	WSANETWORKEVENTS networkEvents;
	int enumRes;

	enumRes = WSAEnumNetworkEvents(netDescriptors[index]->socket->sock,
			events[index], &networkEvents);
	if (enumRes == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	if (networkEvents.lNetworkEvents & FD_READ) {
		bool closed;
		if (networkEvents.iErrorCode[FD_READ_BIT] != 0) {
			// No special handling is required; the callback function
			// will try to do a recv() and will get the error then.
		}

		closed = NetManager_doReadCallback(netDescriptors[index]);
		if (closed)
			goto closed;
	}
	if (networkEvents.lNetworkEvents & FD_WRITE) {
		bool closed;
		if (networkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
			// No special handling is required; the callback function
			// will try to do a send() and will get the error then.
		}

		closed = NetManager_doWriteCallback(netDescriptors[index]);
		if (closed)
			goto closed;
	}
	if (networkEvents.lNetworkEvents & FD_OOB) {
		bool closed;
		if (networkEvents.iErrorCode[FD_OOB_BIT] != 0) {
			// No special handling is required; the callback function
			// will get the error then when it tries to do a recv().
		}

		closed = NetManager_doExceptionCallback(netDescriptors[index]);
		if (closed)
			goto closed;
	}
	if (networkEvents.lNetworkEvents & FD_ACCEPT) {
		// There is no specific accept callback (because the BSD sockets
		// don't work with specific notification for accept); we use
		// the read callback instead.
		bool closed;
		if (networkEvents.iErrorCode[FD_READ_BIT] != 0) {
			// No special handling is required; the callback function
			// will try to do an accept() and will get the error then.
		}

		closed = NetManager_doReadCallback(netDescriptors[index]);
		if (closed)
			goto closed;
	}
#if 0
	// No need for this. Windows also sets FD_WRITE in this case, and
	// writability is what we check for anyhow.
	if (networkEvents.lNetworkEvents & FD_CONNECT) {
		// There is no specific connect callback (because the BSD sockets
		// don't work with specific notification for connect); we use
		// the write callback instead.
		bool closed;
		if (networkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
			// No special handling is required; the callback function
			// should do getsockopt() with SO_ERROR to get the error.
		}

		closed = NetManager_doWriteCallback(netDescriptors[index]);
		if (closed)
			goto closed;
	}
#endif
	if (networkEvents.lNetworkEvents & FD_CLOSE) {
		// The close event is handled last, in case there was still
		// data in the buffers which could be processed.
		NetDescriptor_close(netDescriptors[index]);
		goto closed;
	}

closed:  /* No special actions required for now. */

	return 0;
}

// This function may be called again from inside a callback function
// triggered by this function. BUG: This may result in callbacks being
// called multiple times.
// This function should however not be called from multiple threads at once.
int
NetManager_process(uint32 *timeoutMs) {
	DWORD timeoutTemp;
	DWORD waitResult;
	DWORD startEvent;

	timeoutTemp = (DWORD) *timeoutMs;

	// WSAWaitForMultipleEvents only reports events for one socket at a
	// time. In order to have each socket checked once, we call it
	// again after it has reported an event, but passing only the
	// events not yet processed. The second time, the timeout will be set
	// to 0, so it won't wait.
	startEvent = 0;
	while (startEvent < numActiveEvents) {
		waitResult = WSAWaitForMultipleEvents(numActiveEvents - startEvent,
				&events[startEvent], FALSE, timeoutTemp, FALSE);
		
		if (waitResult == WSA_WAIT_IO_COMPLETION)
			continue;

		if (waitResult == WSA_WAIT_TIMEOUT) {
			// No events waiting.
			*timeoutMs = 0;
			return 0;
		}

		if (waitResult == WSA_WAIT_FAILED) {
			errno = getWinsockErrno();
			*timeoutMs = timeoutTemp;
			return -1;
		}
		
		{
			DWORD eventIndex = waitResult - WSA_WAIT_EVENT_0;
			if (NetManager_processEvent((size_t) eventIndex) == -1) {
				// errno is set
				*timeoutMs = timeoutTemp;
				return -1;
			}
		
			// Check the rest of the sockets, but don't wait anymore.
			startEvent += eventIndex + 1;
			timeoutTemp = 0;
		}
	}

	*timeoutMs = timeoutTemp;
	return 0;
}


