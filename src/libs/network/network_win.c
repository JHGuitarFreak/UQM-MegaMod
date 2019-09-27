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

#include "netport.h"

#include "network.h"

#include "libs/misc.h"
#include "libs/log.h"

#include <errno.h>
#include <winsock2.h>

void
Network_init(void) {
	WSADATA data;
	int startupResult;
	WORD requestVersion = MAKEWORD(2, 2);

	startupResult = WSAStartup(requestVersion, &data);
	if (startupResult != 0) {
		int savedErrno = winsockErrorToErrno(startupResult);
		log_add(log_Fatal, "WSAStartup failed.");
		errno = savedErrno;
		explode();
	}

#ifdef DEBUG
	log_add(log_Debug, "Winsock version %d.%d found: \"%s\".",
			LOBYTE(data.wHighVersion), HIBYTE(data.wHighVersion),
			data.szDescription);
	log_add(log_Debug, "Requesting to use Winsock version %d.%d, got "
			"version %d.%d.",
			LOBYTE(requestVersion), HIBYTE(requestVersion),
			LOBYTE(data.wVersion), HIBYTE(data.wVersion));
#endif
	if (data.wVersion != requestVersion) {
		log_add(log_Fatal, "Winsock version %d.%d presented, requested "
				"%d.%d.", LOBYTE(data.wVersion), HIBYTE(data.wVersion),
				LOBYTE(requestVersion), HIBYTE(requestVersion));
		(void) WSACleanup();
				// Ignoring errors; we're going to abort anyhow.
		explode();
	}
}

void
Network_uninit(void) {
	int cleanupResult;

	cleanupResult = WSACleanup();
	if (cleanupResult == SOCKET_ERROR) {
		int savedErrno = getWinsockErrno();
		log_add(log_Fatal, "WSACleanup failed.");
		errno = savedErrno;
		explode();
	}
}


