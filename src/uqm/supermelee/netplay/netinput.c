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
#include "netinput.h"

#include "../../intel.h"
		// for NETWORK_CONTROL
#include "../../setup.h"
		// For PlayerControl
#include "libs/log.h"

#include <errno.h>
#include <stdlib.h>


static BattleInputBuffer battleInputBuffers[NUM_PLAYERS];
static size_t BattleInput_inputDelay;

// Call before initBattleInputBuffers()
void
setBattleInputDelay(size_t delay) {
	BattleInput_inputDelay = delay;
}

size_t
getBattleInputDelay(void) {
	return BattleInput_inputDelay;
}

static void
BattleInputBuffer_init(BattleInputBuffer *bib, size_t bufSize) {
	bib->buf = malloc(bufSize * sizeof (BATTLE_INPUT_STATE));
	bib->maxSize = bufSize;
	bib->first = 0;
	bib->size = 0;
}

static void
BattleInputBuffer_uninit(BattleInputBuffer *bib) {
	if (bib->buf != NULL) {
		free(bib->buf);
		bib->buf = NULL;
	}
	bib->maxSize = 0;
	bib->first = 0;
	bib->size = 0;
}

void
initBattleInputBuffers(void) {
	size_t player;
	int bufSize = BattleInput_inputDelay * 2 + 2;
	
	// The input of frame n will be processed in frame 'n + delay'.
	//
	// In the worst case, side 1 processes frames 'n' through 'n + delay - 1',
	// then blocks in frame 'n + delay'.
	// Then side 2 receives all this input, and progresses to 'delay' frames
	// after the last frame the received input originated from, and blocks
	// in the frame after it.
	// So it sent input for frames 'n' through 'n + delay + delay + 1'.
	// The input for these '2*delay + 2' frames are still
	// unhandled by side 1, so it needs buffer space for this.
	//
	// Initially the buffer is filled with inputDelay zeroes,
	// so that a party can process at least that much frames.

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		BattleInputBuffer *bib = &battleInputBuffers[player];
		BattleInputBuffer_init(bib, bufSize);

		{
			// Initially a party must be able to process at least inputDelay
			// frames, so we fill the buffer with inputDelay zeros.
			size_t i;
			for (i = 0; i < BattleInput_inputDelay; i++)
				BattleInputBuffer_push(bib, (BATTLE_INPUT_STATE) 0);
		}
	}
}

void
uninitBattleInputBuffers(void)
{
	size_t player;

	for (player = 0; player < NUM_PLAYERS; player++)
	{
		BattleInputBuffer *bib;

		bib = &battleInputBuffers[player];
		BattleInputBuffer_uninit(bib);
	}
}

// On error, returns false and sets errno.
bool
BattleInputBuffer_push(BattleInputBuffer *bib, BATTLE_INPUT_STATE input)
{
	size_t next;

	if (bib->size == bib->maxSize) {
		// No more space.
		log_add(log_Error, "NETPLAY:     battleInputBuffer full.\n");
		errno = ENOBUFS;
		return false;
	}

	next = (bib->first + bib->size) % bib->maxSize;
	bib->buf[next] = input;
	bib->size++;
	return true;
}

// On error, returns false and sets errno, and *input remains unchanged.
bool
BattleInputBuffer_pop(BattleInputBuffer *bib, BATTLE_INPUT_STATE *input)
{
	if (bib->size == 0)
	{
		// Buffer is empty.
		errno = EAGAIN;
		return false;
	}

	*input = bib->buf[bib->first];
	bib->first = (bib->first + 1) % bib->maxSize;
	bib->size--;
	return true;
}

BattleInputBuffer *
getBattleInputBuffer(size_t player) {
	return &battleInputBuffers[player];
}


