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

#ifndef LIBS_NETWORK_SOCKET_SOCKET_H_
#define LIBS_NETWORK_SOCKET_SOCKET_H_

typedef struct Socket Socket;
#define Socket_noSocket ((Socket *) NULL)

#include "port.h"

#ifdef USE_WINSOCK
#	include "socket_win.h"
#else
#	include "socket_bsd.h"
#endif


////////////////////////////////////////////////////////////////////////////


// Defining our own types for protocol families and protocols instead of
// using the system defines, so that the layer using this API does not have
// to have anything to do with the system layer.

typedef enum {
	PF_unspec,
	PF_inet,
	PF_inet6,
} ProtocolFamily;
typedef ProtocolFamily AddressFamily;

typedef enum {
	IPProto_tcp,
	IPProto_udp,
} Protocol;

typedef enum {
	Sock_stream,
	Sock_dgram,
} SocketType;

#ifdef SOCKET_INTERNAL
extern const int protocolFamilyTranslation[];
#define addressFamilyTranslation protocolFamilyTranslation;
extern const int protocolTranslation[];
extern const int socketTypeTranslation[];
#endif


////////////////////////////////////////////////////////////////////////////


Socket *Socket_open(ProtocolFamily domain, SocketType type,
		Protocol protocol);
#ifdef SOCKET_INTERNAL
Socket *Socket_openNative(int domain, int type, int protocol);
#endif
int Socket_close(Socket *sock);

int Socket_connect(Socket *sock, const struct sockaddr *addr,
		socklen_t addrLen);
int Socket_bind(Socket *sock, const struct sockaddr *addr,
		socklen_t addrLen);
int Socket_listen(Socket *sock, int backlog);
Socket *Socket_accept(Socket *sock, struct sockaddr *addr, socklen_t *addrLen);
ssize_t Socket_send(Socket *sock, const void *buf, size_t len, int flags);
ssize_t Socket_sendto(Socket *sock, const void *buf, size_t len, int flags,
		const struct sockaddr *addr, socklen_t addrLen);
ssize_t Socket_recv(Socket *sock, void *buf, size_t len, int flags);
ssize_t Socket_recvfrom(Socket *sock, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromLen);

int Socket_setNonBlocking(Socket *sock);
int Socket_setReuseAddr(Socket *sock);
int Socket_setNodelay(Socket *sock);
int Socket_setTOS(Socket *sock, int tos);
int Socket_setInteractive(Socket *sock);
int Socket_setInlineOOB(Socket *sock);
int Socket_setKeepAlive(Socket *sock);
int Socket_getError(Socket *sock, int *err);

#endif  /* LIBS_NETWORK_SOCKET_SOCKET_H_ */

