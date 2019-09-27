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

// Socket functions for BSD sockets.

#define SOCKET_INTERNAL
#include "socket.h"

#include "libs/log.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#	include <netinet/in_systm.h>
#	include <netinet/in.h>
#endif
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>


static Socket *
Socket_alloc(void) {
	return malloc(sizeof (Socket));
}

static void
Socket_free(Socket *sock) {
	free(sock);
}

Socket *
Socket_openNative(int domain, int type, int protocol) {
	Socket *result;
	int fd;
	
	fd = socket(domain, type, protocol);
	if (fd == -1) {
		// errno is set
		return Socket_noSocket;
	}
	
	result = Socket_alloc();
	result->fd = fd;
	return result;
}

int
Socket_close(Socket *sock) {
	int closeResult;

	do {
		closeResult = close(sock->fd);
		if (closeResult == 0) {
			Socket_free(sock);
			return 0;
		}
	} while (errno == EINTR);

	return -1;
}

int
Socket_connect(Socket *sock, const struct sockaddr *addr,
		socklen_t addrLen) {
	int connectResult;

	do {
		connectResult = connect(sock->fd, addr, addrLen);
	} while (connectResult == -1 && errno == EINTR);

	return connectResult;
}

int
Socket_bind(Socket *sock, const struct sockaddr *addr, socklen_t addrLen) {
	return bind(sock->fd, addr, addrLen);
}

int
Socket_listen(Socket *sock, int backlog) {
	return listen(sock->fd, backlog);
}

Socket *
Socket_accept(Socket *sock, struct sockaddr *addr, socklen_t *addrLen) {
	int acceptResult;
	socklen_t tempAddrLen;
	
	do {
		tempAddrLen = *addrLen;
		acceptResult = accept(sock->fd, addr, &tempAddrLen);
		if (acceptResult != -1) {
			Socket *result = Socket_alloc();
			result->fd = acceptResult;
			*addrLen = tempAddrLen;
			return result;
		}

	} while (errno == EINTR);

	// errno is set
	return Socket_noSocket;
}

ssize_t
Socket_send(Socket *sock, const void *buf, size_t len, int flags) {
	return send(sock->fd, buf, len, flags);
}

ssize_t
Socket_sendto(Socket *sock, const void *buf, size_t len, int flags,
		const struct sockaddr *addr, socklen_t addrLen) {
	return sendto(sock->fd, buf, len, flags, addr, addrLen);
}

ssize_t
Socket_recv(Socket *sock, void *buf, size_t len, int flags) {
	return recv(sock->fd, buf, len, flags);
}

ssize_t
Socket_recvfrom(Socket *sock, void *buf, size_t len, int flags,
		struct sockaddr *from, socklen_t *fromLen) {
	return recvfrom(sock->fd, buf, len, flags, from, fromLen);
}

int
Socket_setNonBlocking(Socket *sock) {
	int flags;

	flags = fcntl(sock->fd, F_GETFL);
	if (flags == -1) {
		int savedErrno = errno;
		log_add(log_Error, "Getting file descriptor flags of socket failed: "
				"%s.", strerror(errno));
		errno = savedErrno;
		return -1;
	}

	if (fcntl(sock->fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		int savedErrno = errno;
		log_add(log_Error, "Setting non-blocking mode on socket failed: "
				"%s.", strerror(errno));
		errno = savedErrno;
		return -1;
	}

	return 0;
}

int
Socket_setReuseAddr(Socket *sock) {
	int flag = 1;

	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag)
			== -1) {
		int savedErrno = errno;
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
	int flag = 1;

	if (setsockopt(sock->fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof flag)
			== -1) {
#ifdef DEBUG
		int savedErrno = errno;
		log_add(log_Warning, "Disabling Nagle algorithm failed: %s.",
				strerror(errno));
		errno = savedErrno;
#endif
		return -1;
	}
	return 0;
}

// 'tos' should be IPTOS_LOWDELAY, IPTOS_THROUGHPUT, IPTOS_THROUGHPUT,
// IPTOS_RELIABILITY, or IPTOS_MINCOST.
int
Socket_setTOS(Socket *sock, int tos) {
	if (setsockopt(sock->fd, IPPROTO_IP, IP_TOS, &tos, sizeof tos) == -1) {
#ifdef DEBUG
		int savedErrno = errno;
		log_add(log_Warning, "Setting socket type-of-service failed: %s.",
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
	
	if (Socket_setTOS(sock, IPTOS_LOWDELAY) == -1) {
		// errno is set
		return -1;
	}
	return 0;
}

int
Socket_setInlineOOB(Socket *sock) {
	int flag = 1;

	if (setsockopt(sock->fd, SOL_SOCKET, SO_OOBINLINE, &flag, sizeof flag)
			== -1) {
		int savedErrno = errno;
		log_add(log_Error, "Setting inline OOB on socket failed: %s",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

int
Socket_setKeepAlive(Socket *sock) {
	int flag = 1;

	if (setsockopt(sock->fd, IPPROTO_TCP, SO_KEEPALIVE, &flag, sizeof flag)
			== -1) {
		int savedErrno = errno;
		log_add(log_Error, "Setting keep-alive on socket failed: %s",
				strerror(errno));
		errno = savedErrno;
		return -1;
	}
	return 0;
}

int
Socket_getError(Socket *sock, int *err) {
	socklen_t errLen = sizeof(*err);

	if (getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, err, &errLen) == -1) {
		// errno is set
		return -1;
	}

	assert(errLen == sizeof(*err));
	// err is set
	return 0;
}


