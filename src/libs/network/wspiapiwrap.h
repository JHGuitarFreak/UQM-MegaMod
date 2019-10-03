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

#	if defined(__MINGW32__)
FARPROC WINAPI WspiapiLoad(WORD wFunction);
BOOL WINAPI WspiapiParseV4Address(const char* pszAddress, PDWORD pswAddress);
struct addrinfo* WINAPI WspiapiNewAddrInfo(int iSocketType, int iProtocol,
	WORD wPort, DWORD dwAddress);
char* WINAPI WspiapiStrdup(const char* pszString);
int WINAPI WspiapiLookupNode(const char* pszNodeName, int iSocketType,
	int iProtocol, WORD wPort, BOOL bAI_CANONNAME,
	struct addrinfo** pptResult);
int WINAPI WspiapiQueryDNS(const char* pszNodeName, int iSocketType,
	int iProtocol, WORD wPort, char pszAlias[NI_MAXHOST],
	struct addrinfo** pptResult);
int WINAPI WspiapiClone(WORD wPort, struct addrinfo* ptResult);
void WINAPI WspiapiLegacyFreeAddrInfo(struct addrinfo* ptHead);
int WINAPI WspiapiLegacyGetAddrInfo(const char* pszNodeName,
	const char* pszServiceName, const struct addrinfo* ptHints,
	struct addrinfo** pptRresult);
int WINAPI WspiapiLegacyGetNameInfo(
	const struct sockaddr* ptSocketAddress, socklen_t tSocketLength,
	char* pszNodeName, size_t tNodeLength, char* pszServiceName,
	size_t tServiceLength, int iFlags);
#	endif

#endif

#endif  /* LIBS_NETWORK_WSPIAPIWRAP_H_ */

