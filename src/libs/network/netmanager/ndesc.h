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

#ifndef LIBS_NETWORK_NETMANAGER_NDESC_H_
#define LIBS_NETWORK_NETMANAGER_NDESC_H_

#include "types.h"


typedef struct NetDescriptor NetDescriptor;

typedef void (*NetDescriptor_ReadCallback)(NetDescriptor *nd);
typedef void (*NetDescriptor_WriteCallback)(NetDescriptor *nd);
typedef void (*NetDescriptor_ExceptionCallback)(NetDescriptor *nd);
typedef void (*NetDescriptor_CloseCallback)(NetDescriptor *nd);

typedef uint32 RefCount;
#define REFCOUNT_MAX UINT32_MAX

#include "../socket/socket.h"
#include "netmanager.h"

#ifdef NETDESCRIPTOR_INTERNAL
// All created NetDescriptors are registered to the NetManager.
// They are unregisted when the NetDescriptor is closed.
// On creation the ref count is set to 1. On close it is decremented by 1.
struct NetDescriptor {
	struct {
		bool closed: 1;
	} flags;
	
	RefCount refCount;
	
	NetDescriptor_ReadCallback readCallback;
	NetDescriptor_WriteCallback writeCallback;
	NetDescriptor_ExceptionCallback exceptionCallback;
	NetDescriptor_CloseCallback closeCallback;
	
	Socket *socket;
	SocketManagementData *smd;

	// Extra state-dependant information for the user.
	void *extra;
};
#endif

NetDescriptor *NetDescriptor_new(Socket *socket, void *extra);
void NetDescriptor_close(NetDescriptor *nd);
void NetDescriptor_incRef(NetDescriptor *nd);
bool NetDescriptor_decRef(NetDescriptor *nd);
void NetDescriptor_detach(NetDescriptor *nd);
Socket *NetDescriptor_getSocket(NetDescriptor *nd);
void NetDescriptor_setExtra(NetDescriptor *nd, void *extra);
void *NetDescriptor_getExtra(const NetDescriptor *nd);
void NetDescriptor_setReadCallback(NetDescriptor *nd,
		NetDescriptor_ReadCallback callback);
void NetDescriptor_setWriteCallback(NetDescriptor *nd,
		NetDescriptor_WriteCallback callback);
void NetDescriptor_setExceptionCallback(NetDescriptor *nd,
		NetDescriptor_ExceptionCallback callback);
void NetDescriptor_setCloseCallback(NetDescriptor *nd,
		NetDescriptor_CloseCallback callback);


#endif  /* LIBS_NETWORK_NETMANAGER_NDESC_H_ */


