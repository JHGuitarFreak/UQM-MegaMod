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

#include "port.h"

#define SOCKET_INTERNAL
#include "socket.h"

#ifdef USE_WINSOCK
#	include <winsock2.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#endif


////////////////////////////////////////////////////////////////////////////


const int protocolFamilyTranslation[] = {
	/* .[PF_unspec] = */ PF_UNSPEC,
	/* .[PF_inet]   = */ PF_INET,
	/* .[PF_inet6]  = */ PF_INET6,
};

const int protocolTranslation[] = {
	/* .[IPProto_tcp] = */ IPPROTO_TCP,
	/* .[IPProto_udp] = */ IPPROTO_UDP,
};

const int socketTypeTranslation[] = {
	/* .[Sock_stream] = */ SOCK_STREAM,
	/* .[Sock_dgram]  = */ SOCK_DGRAM,
};


Socket *
Socket_open(ProtocolFamily domain, SocketType type, Protocol protocol) {
	return Socket_openNative(protocolFamilyTranslation[domain],
			socketTypeTranslation[type], protocolTranslation[protocol]);
}


////////////////////////////////////////////////////////////////////////////


