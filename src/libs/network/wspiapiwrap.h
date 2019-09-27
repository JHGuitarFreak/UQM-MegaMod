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

#ifndef LIBS_NETWORK_WSPIAPIWRAP_H_
#define LIBS_NETWORK_WSPIAPIWRAP_H_

#if (_MSC_VER >= 1500)
#	include <wspiapi.h>  //DC: replaced lower section with this part to (hopefully) compile.
#endif

#if (_MSC_VER <= 1500 || defined(__MINGW32__))
   // HACK. See wspiapiwrap.c
#	define getaddrinfo WspiapiGetAddrInfo
#	define getnameinfo WspiapiGetNameInfo
#	define freeaddrinfo WspiapiFreeAddrInfo
void WINAPI WspiapiFreeAddrInfo (struct addrinfo *ai);
int WINAPI WspiapiGetAddrInfo(const char *nodename, const char *servname,
		const struct addrinfo *hints, struct addrinfo **res);
int WINAPI WspiapiGetNameInfo (const struct sockaddr *sa, socklen_t salen,
		char *host, size_t hostlen, char *serv, size_t servlen, int flags);
#endif

#endif  /* LIBS_NETWORK_WSPIAPIWRAP_H_ */

