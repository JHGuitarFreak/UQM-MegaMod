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
#include "netport.h"

#ifdef USE_WINSOCK
#	include <winsock2.h>

int
winsockErrorToErrno(int winsockError) {
	switch (winsockError) {
		case WSAEINTR: return EINTR;
		case WSAEACCES: return EACCES;
		case WSAEFAULT: return EFAULT;
		case WSAEINVAL: return EINVAL;
		case WSAEMFILE: return EMFILE;
		case WSAEWOULDBLOCK: return EWOULDBLOCK;
		case WSAEINPROGRESS: return EINPROGRESS;
		case WSAEALREADY: return EALREADY;
		case WSAENOTSOCK: return ENOTSOCK;
		case WSAEDESTADDRREQ: return EDESTADDRREQ;
		case WSAEMSGSIZE: return EMSGSIZE;
		case WSAEPROTOTYPE: return EPROTOTYPE;
		case WSAENOPROTOOPT: return ENOPROTOOPT;
		case WSAEPROTONOSUPPORT: return EPROTONOSUPPORT;
		case WSAESOCKTNOSUPPORT: return ESOCKTNOSUPPORT;
		case WSAEOPNOTSUPP: return EOPNOTSUPP;
		case WSAEPFNOSUPPORT: return EPFNOSUPPORT;
		case WSAEAFNOSUPPORT: return EAFNOSUPPORT;
		case WSAEADDRINUSE: return EADDRINUSE;
		case WSAEADDRNOTAVAIL: return EADDRNOTAVAIL;
		case WSAENETDOWN: return ENETDOWN;
		case WSAENETUNREACH: return ENETUNREACH;
		case WSAENETRESET: return ENETRESET;
		case WSAECONNABORTED: return ECONNABORTED;
		case WSAECONNRESET: return ECONNRESET;
		case WSAENOBUFS: return ENOBUFS;
		case WSAEISCONN: return EISCONN;
		case WSAENOTCONN: return ENOTCONN;
		case WSAESHUTDOWN: return ESHUTDOWN;
		case WSAETIMEDOUT: return ETIMEDOUT;
		case WSAECONNREFUSED: return ECONNREFUSED;
		case WSAEHOSTDOWN: return EHOSTDOWN;
		case WSAEHOSTUNREACH: return EHOSTUNREACH;
		case WSAEPROCLIM: return EPROCLIM;
		case WSASYSNOTREADY: return ENOSYS;
		case WSAVERNOTSUPPORTED: return ENOSYS;
		case WSANOTINITIALISED: return ENOSYS;
		case WSAEDISCON: return ECONNRESET;
		case WSATYPE_NOT_FOUND: return ENODATA;
		case WSAHOST_NOT_FOUND: return ENODATA;
		case WSATRY_AGAIN: return EAGAIN;
		case WSANO_RECOVERY: return EIO;
		case WSANO_DATA: return ENODATA;
		case WSA_INVALID_HANDLE: return EBADF;
		case WSA_INVALID_PARAMETER: return EINVAL;
		case WSA_IO_INCOMPLETE: return EAGAIN;
		case WSA_IO_PENDING: return EINPROGRESS;
		case WSA_NOT_ENOUGH_MEMORY: return ENOMEM;
		case WSA_OPERATION_ABORTED: return EINTR;
		case WSAEINVALIDPROCTABLE: return ENOSYS;
		case WSAEINVALIDPROVIDER: return ENOSYS;
		case WSAEPROVIDERFAILEDINIT: return ENOSYS;
		case WSASYSCALLFAILURE: return EIO;
		default: return EIO;
	}
}

int
getWinsockErrno(void) {
	return winsockErrorToErrno(WSAGetLastError());
}
#endif  /* defined (USE_WINSOCK) */

