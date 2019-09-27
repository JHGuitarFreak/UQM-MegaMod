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

#ifndef LIBS_NETWORK_NETPORT_H_
#define LIBS_NETWORK_NETPORT_H_

#include "port.h"

#ifdef USE_WINSOCK
int winsockErrorToErrno(int winsockError);
int getWinsockErrno(void);
#	define EAI_SYSTEM 0x02000001
		// Any value will do that doesn't conflict with an existing value.

#ifdef __MINGW32__
// MinGW does not have a working gai_strerror() yet.
static inline const char *
gai_strerror(int err) {
	(void) err;
	return "[gai_strerror() is not available on MinGW]";
}
#endif  /* defined(__MINGW32__) */

#endif

#endif  /* LIBS_NETWORK_NETPORT_H_ */


