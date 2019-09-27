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

#include "netplay.h"
#include "checkbuf.h"
#include "libs/log.h"

#include "../../battle.h"
		// for battleFrameCount


#include <errno.h>
#include <stdlib.h>



static inline BattleFrameCounter
ChecksumBuffer_getCurrentFrameNr(void) {
	return battleFrameCount;
}

void
ChecksumBuffer_init(ChecksumBuffer *cb, size_t delay, size_t interval) {
	// The input buffer lags BattleInput_inputDelay frames behind,
	// but only every interval frames will there be a checksum to be
	// checked.

	// Checksums will be checked when 'frameNr % interval == 0'.
	// (and frameNr is zero-based).
	// The checksum of frame n will be processed in frame 'n + delay'.

	// In the worst case, side 1 processes frames 'n' through 'n + delay - 1',
	// then blocks in frame 'n + delay' (after sending a checksum, if that's
	// pertinent for that frame).
	// Then side 2 receives all this input and these checksums, and
	// progresses to 'delay' frames after the last frame the received input
	// originated from, and blocks in the frame after it (after sending a
	// checksum, if that's pertinent for the frame).
	// So it sent input and checksums for frames 'n' through
	// 'n + delay + delay + 1'.
	// The input and checksums for these '2*delay + 2' frames are still
	// unhandled by side 1, so it needs buffer space for this.
	// With checksums only sent every interval frames, the buffer space
	// needed will be 'roundUp(2*delay + 2)' spaces.

	size_t bufSize = ((2 * delay + 2) + (interval - 1)) / interval;

	{
#ifdef NETPLAY_DEBUG
		size_t i;
#endif

		cb->checksums = malloc(bufSize * sizeof (ChecksumEntry));
		cb->maxSize = bufSize;
		cb->interval = interval;

#ifdef NETPLAY_DEBUG
		for (i = 0; i < bufSize; i++) {
			cb->checksums[i].checksum = 0;
			cb->checksums[i].frameNr = (BattleFrameCounter) -1;
		}
#endif
	}
}

void
ChecksumBuffer_uninit(ChecksumBuffer *cb) {
	if (cb->checksums != NULL) {
		free(cb->checksums);
		cb->checksums = NULL;
	}
}

// Returns the entry that would be used for the checksum for the specified
// frame. Whether the entry is actually valid is not checked.
static ChecksumEntry *
ChecksumBuffer_getChecksumEntry(ChecksumBuffer *cb,
		BattleFrameCounter frameNr) {
	size_t index;
	ChecksumEntry *entry;

	assert(frameNr % cb->interval == 0);
			// We only record checksums exactly every 'interval' frames.

	index = (frameNr / cb->interval) % cb->maxSize;
	entry = &cb->checksums[index];

	return entry;
}

bool
ChecksumBuffer_addChecksum(ChecksumBuffer *cb, BattleFrameCounter frameNr,
		Checksum checksum) {
	ChecksumEntry *entry;
	
	assert(frameNr % cb->interval == 0);

	entry = ChecksumBuffer_getChecksumEntry(cb, frameNr);

#ifdef NETPLAY_DEBUG
	entry->frameNr = frameNr;
#endif
	entry->checksum = checksum;
	return true;
}

// Pre: frameNr is within the range of the checksums stored in cb.
bool
ChecksumBuffer_getChecksum(ChecksumBuffer *cb, BattleFrameCounter frameNr,
		Checksum *result) {
	ChecksumEntry *entry;

	entry = ChecksumBuffer_getChecksumEntry(cb, frameNr);
	
#ifdef NETPLAY_DEBUG
	if (frameNr != entry->frameNr) {
		log_add(log_Error, "Checksum buffer entry for requested frame %u "
				"(still?) contains a checksum for frame %u.\n",
				frameNr, entry->frameNr);
		return false;
	}
#endif

	*result = entry->checksum;
	return true;
}

