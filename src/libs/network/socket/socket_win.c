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

// Socket functions for Winsock sockets.

#define PORT_WANT_ERRNO
#include "port.h"
#include "../netport.h"

#define SOCKET_INTERNAL
#include "socket.h"

#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#include <winsock2.h>


Socket *
Socket_alloc(void) {
	return malloc(sizeof (Socket));
}

void
Socket_free(Socket *sock) {
	free(sock);
}

Socket *
Socket_openNative(int domain, int type, int protocol) {
	Socket *result;
	SOCKET sock;

	sock = socket(domain, type, protocol);
	if (sock == INVALID_SOCKET) {
		errno = getWinsockErrno();
		return Socket_noSocket;
	}

	result = Socket_alloc();
	result->sock = sock;
	return result;
}

int
Socket_close(Socket *sock) {
	int closeResult;

	do {
		closeResult = closesocket(sock->sock);
		if (closeResult != SOCKET_ERROR) {
			Socket_free(sock);
			return 0;
		}

		errno = getWinsockErrno();
	} while (errno == EINTR);

	return -1;
}

int
Socket_connect(Socket *sock, const struct sockaddr *addr,
		socklen_t addrLen) {
	int connectResult;

	do {
		connectResult = connect(sock->sock, addr, addrLen);
		if (connectResult == 0)
			return 0;

		errno = getWinsockErrno();
	} while (errno == EINTR);

	if (errno == EWOULDBLOCK) {
		// Windows returns (WSA)EWOULDBLOCK when a connection is being
		// initiated on a non-blocking socket, while other platforms
		// use EINPROGRESS in such cases.
		errno = EINPROGRESS;
	}

	return -1;
}

int
Socket_bind(Socket *sock, const struct sockaddr *addr, socklen_t addrLen) {
	int bindResult;

	bindResult = bind(sock->sock, addr, addrLen);
	if (bindResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return 0;
}

int
Socket_listen(Socket *sock, int backlog) {
	int listenResult;

	listenResult = listen(sock->sock, backlog);
	if (listenResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return 0;
}

Socket *
Socket_accept(Socket *sock, struct sockaddr *addr, socklen_t *addrLen) {
	SOCKET acceptResult;
	socklen_t tempAddrLen;
	
	do {
		tempAddrLen = *addrLen;
		acceptResult = accept(sock->sock, addr, &tempAddrLen);
		if (acceptResult != INVALID_SOCKET) {
			Socket *result = Socket_alloc();
			result->sock = acceptResult;
			*addrLen = tempAddrLen;
			return result;
		}

		errno = getWinsockErrno();
	} while (errno == EINTR);

	// errno is set
	return Socket_noSocket;
}

ssize_t
Socket_send(Socket *sock, const void *buf, size_t len, int flags) {
	int sendResult;

	sendResult = send(sock->sock, buf, len, flags);
	if (sendResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return sendResult;
}

ssize_t
Socket_sendto(Socket *sock, const void *buf, size_t len, int flags,
		const struct sockaddr *addr, socklen_t addrLen) {
	int sendResult;

	sendResult = sendto(sock->sock, buf, len, flags, addr, addrLen);
	if (sendResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return sendResult;
}

ssize_t
Socket_recv(Socket *sock, void *buf, size_t len, int flags) {
	int recvResult;

	recvResult = recv(sock->sock, buf, len, flags);
	if (recvResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return recvResult;
}

ssize_t
Socket_recvfrom(Socket *sock, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromLen) {
	int recvResult;

	recvResult = recvfrom(sock->sock, buf, len, flags, from, fromLen);
	if (recvResult == SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	return recvResult;
}

int
Socket_setNonBlocking(Socket *sock) {
	unsigned long flag = 1;

	if (ioctlsocket(sock->sock, FIONBIO, &flag) == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		log_add(log_Error, "Setting non-block mode on socket failed: %s.",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

int
Socket_setReuseAddr(Socket *sock) {
	BOOL flag = 1;

	if (setsockopt(sock->sock, SOL_SOCKET, SO_REUSEADDR,
				(const char *) &flag, sizeof flag) == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		log_add(log_Error, "Setting socket reuse failed: %s.",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

// Send data as soon as it is available. Do not collect data to send at
// once.
int
Socket_setNodelay(Socket *sock) {
	BOOL flag = 1;

	if (setsockopt(sock->sock, IPPROTO_TCP, TCP_NODELAY,
				(const char *) &flag, sizeof flag) == SOCKET_ERROR) {
#ifdef DEBUG
		int savedErrno = getWinsockErrno();
		log_add(log_Warning, "Disabling Nagle algorithm failed: %s.",
				strerror(errno));
		errno = savedErrno;
#endif
		return -1;
	}
	return 0;
}

// This function setups the socket for optimal configuration for an
// interactive connection.
int
Socket_setInteractive(Socket *sock) {
	if (Socket_setNodelay(sock) == -1) {
		// errno is set
		return -1;
	}
	
#if 0
	if (Socket_setTOS(sock, IPTOS_LOWDELAY) == -1) {
		// errno is set
		return -1;
	}
#endif
	return 0;
}

int
Socket_setInlineOOB(Socket *sock) {
	BOOL flag = 1;

	if (setsockopt(sock->sock, SOL_SOCKET, SO_OOBINLINE, (const char *) &flag,
			sizeof flag) == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		log_add(log_Error, "Setting inline OOB on socket failed: %s",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

int
Socket_setKeepAlive(Socket *sock) {
	BOOL flag = 1;

	if (setsockopt(sock->sock, IPPROTO_TCP, SO_KEEPALIVE,
			(const char *) &flag, sizeof flag) == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		log_add(log_Error, "Setting keep-alive on socket failed: %s",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

int
Socket_getError(Socket *sock, int *err) {
	int errLen = sizeof(*err);

	if (getsockopt(sock->sock, SOL_SOCKET, SO_ERROR, (char *) err, &errLen)
			== SOCKET_ERROR) {
		errno = getWinsockErrno();
		return -1;
	}

	assert(errLen == sizeof(*err));
	// err is set
	return 0;
}


