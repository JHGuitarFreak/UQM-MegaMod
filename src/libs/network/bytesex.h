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

// Routines for changing the endianness of values.
// I'm not using ntohs() etc. as those would require include files that may
// have conflicting definitions. This is a problem on Windows, where these
// functions are in winsock2.h, which includes windows.h, which includes
// pretty much Microsoft's complete collection of .h files.

#ifndef LIBS_NETWORK_BYTESEX_H_
#define LIBS_NETWORK_BYTESEX_H_

#include "port.h"
		// for inline
#include "endian_uqm.h"
		// for WORDS_BIGENDIAN
#include "types.h"

static inline uint16
swapBytes16(uint16 x) {
	return (x << 8) | (x >> 8);
}

static inline uint32
swapBytes32(uint32 x) {
	return (x << 24)
			| ((x & 0x0000ff00) << 8)
			| ((x & 0x00ff0000) >> 8)
			| (x >> 24);
}

#ifdef WORDS_BIGENDIAN
// Already in network order.

static inline uint16
hton16(uint16 x) {
	return x;
}

static inline uint32
hton32(uint32 x) {
	return x;
}

static inline uint16
ntoh16(uint16 x) {
	return x;
}

static inline uint32
ntoh32(uint32 x) {
	return x;
}

#else  /* !defined(WORDS_BIGENDIAN) */
// Need to swap bytes

static inline uint16
hton16(uint16 x) {
	return swapBytes16(x);
}

static inline uint32
hton32(uint32 x) {
	return swapBytes32(x);
}

static inline uint16
ntoh16(uint16 x) {
	return swapBytes16(x);
}

static inline uint32
ntoh32(uint32 x) {
	return swapBytes32(x);
}

#endif  /* defined(WORDS_BIGENDIAN) */

#endif  /* LIBS_NETWORK_BYTESEX_H_ */

