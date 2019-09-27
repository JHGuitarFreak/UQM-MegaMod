/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef LIBS_UIO_UIOUTILS_H_
#define LIBS_UIO_UIOUTILS_H_

#include <time.h>

#include "types.h"
#include "uioport.h"

char *strcata(const char *first, const char *second);
void *insertArray(const void *array, size_t oldNumElements, int insertPos,
		const void *element, size_t elementSize);
void **insertArrayPointer(const void **array, size_t oldNumElements,
		int insertPos, const void *element);
void *excludeArray(const void *array, size_t oldNumElements, int startPos,
		int numExclude, size_t elementSize);
void **excludeArrayPointer(const void **array, size_t oldNumElements,
		int startPos, int numExclude);
time_t dosToUnixTime(uio_uint16 date, uio_uint16 tm);
char *dosToUnixPath(const char *path);

/* Sometimes you just have to remove a 'const'.
 * (for instance, when implementing a function like strchr)
 */
static inline void *
unconst(const void *arg) {
	union {
		void *c;
		const void *cc;
	} u;
	u.cc = arg;
	return u.c;
}

// byte1 is the lowest byte, byte4 the highest
static inline uio_uint32
makeUInt32(uio_uint8 byte1, uio_uint8 byte2, uio_uint8 byte3, uio_uint8 byte4) {
	return byte1 | (byte2 << 8) | (byte3 << 16) | (byte4 << 24);
}

static inline uio_uint16
makeUInt16(uio_uint8 byte1, uio_uint8 byte2) {
	return byte1 | (byte2 << 8);
}

static inline uio_sint32
makeSInt32(uio_uint8 byte1, uio_uint8 byte2, uio_uint8 byte3, uio_uint8 byte4) {
	return byte1 | (byte2 << 8) | (byte3 << 16) | (byte4 << 24);
}

static inline uio_sint16
makeSInt16(uio_uint8 byte1, uio_uint8 byte2) {
	return byte1 | (byte2 << 8);
}

static inline uio_bool
isBitSet(uio_uint32 bitField, int bit) {
	return ((bitField >> bit) & 1) == 1;
}

static inline int
mins(int i1, int i2) {
	return i1 <= i2 ? i1 : i2;
}

static inline unsigned int
minu(unsigned int i1, unsigned int i2) {
	return i1 <= i2 ? i1 : i2;
}


#endif  /* LIBS_UIO_UIOUTILS_H_ */

