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

#include "../../../uqmversion.h"

#if !defined(UQM_SUPERMELEE_NETPLAY_NETPLAY_H_) && defined(NETPLAY)
#define UQM_SUPERMELEE_NETPLAY_NETPLAY_H_

// NETPLAY can either be unset (in which case we will never get here)
// NETPLAY_FULL, or NETPLAY_IPV4 (disables IPv6)
#define NETPLAY_IPV4  1
#define NETPLAY_FULL  2

#define NETPLAY_PROTOCOL_VERSION_MAJOR 0
#define NETPLAY_PROTOCOL_VERSION_MINOR 4

#define NETPLAY_MIN_UQM_VERSION_MAJOR UQM_MAJOR_VERSION
#define NETPLAY_MIN_UQM_VERSION_MINOR UQM_MINOR_VERSION
#define NETPLAY_MIN_UQM_VERSION_PATCH UQM_PATCH_VERSION

#undef NETPLAY_DEBUG
		/* Extra debugging for netplay */
#undef NETPLAY_DEBUG_FILE
		/* Dump extra debugging information to file.
		 * Implies NETPLAY_DEBUG.*/
#define NETPLAY_STATISTICS
		/* Keep some statistics */
#define NETPLAY_CHECKSUM
		/* Send/process checksums to verify that both sides of a network
		  * connection are still in sync.
		  * If not enabled, incoming checksum packets will be ignored.
		  * TODO: make compilation of crc.c and checksum.c conditional. */
#define NETPLAY_CHECKSUM_INTERVAL 1
		/* If NETPLAY_CHECKSUM is defined, this define determines
		 * every how many frames a checksum packet is sent. */

#define NETPLAY_READBUFSIZE  2048
#define NETPLAY_CONNECTTIMEOUT  2000
		/* Time to wait for a connect() to succeed. In ms. */
//#define NETPLAY_LISTENTIMEOUT   30000
//		/* Time to wait for a listen() to succeed. In ms. */
#define NETPLAY_RETRYDELAY   2000
		/* Time to wait after all addresses of a host have been tried
		 * before starting retrying them all. In ms. */
#define NETPLAY_LISTEN_BACKLOG 2
		/* Second argument to listen(). */


#ifdef _MSC_VER
#	if _MSC_VER < 1300
		/* NETPLAY_DEBUG_FILE requires the __VA_ARGS__ macro, which is
		 * not available on MSVC 6.0. */
#		undef NETPLAY_DEBUG_FILE
#	endif
#endif

#ifdef NETPLAY_DEBUG_FILE
#	define NETPLAY_DEBUG
#	define DUMP_CRC_OPS
#endif


#endif  /* UQM_SUPERMELEE_NETPLAY_NETPLAY_H_ */

