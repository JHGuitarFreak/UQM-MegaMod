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

#ifndef UQM_SUPERMELEE_NETPLAY_CHECKBUF_H_
#define UQM_SUPERMELEE_NETPLAY_CHECKBUF_H_

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ChecksumEntry ChecksumEntry;
typedef struct ChecksumBuffer ChecksumBuffer;

#if defined(__cplusplus)
}
#endif

#include "../../battle.h"
		// for BattleFrameCounter
#include "checksum.h"

#if defined(__cplusplus)
extern "C" {
#endif


struct ChecksumEntry {
#ifdef NETPLAY_DEBUG
	BattleFrameCounter frameNr;
			// The number of the frame this checksum originated from.
			// If the checksumming code is working correctly, the checksum
			// can only come from one frame, so this value is not needed
			// for normal operation.
			// Its only use is to detect some cases where checksumming code
			// is *not* working correctly.
#endif
	Checksum checksum;
};

struct ChecksumBuffer {
	ChecksumEntry *checksums;
			// Cyclic buffer. if 'size' > 0, then 'first' is an index to
			// the first used entry, 'size' is the number of used
			// entries in the buffer, and (first + size) % maxSize is the
			// index to just past the end of the buffer.
	size_t maxSize;

	size_t interval;
};

void ChecksumBuffer_init(ChecksumBuffer *cb, size_t delay, size_t interval);
void ChecksumBuffer_uninit(ChecksumBuffer *cb);
bool ChecksumBuffer_addChecksum(ChecksumBuffer *cb,
		BattleFrameCounter frameNr, Checksum checksum);
bool ChecksumBuffer_getChecksum(ChecksumBuffer *cb,
		BattleFrameCounter frameNr, Checksum *result);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_CHECKBUF_H_ */

