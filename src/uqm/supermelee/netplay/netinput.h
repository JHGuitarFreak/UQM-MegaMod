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

#ifndef UQM_SUPERMELEE_NETPLAY_NETINPUT_H_
#define UQM_SUPERMELEE_NETPLAY_NETINPUT_H_

#include "../../controls.h"
		// for BATTLE_INPUT_STATE
#include "../../init.h"

#if defined(__cplusplus)
extern "C" {
#endif
		// for NUM_PLAYERS

typedef struct BattleInputBuffer {
	BATTLE_INPUT_STATE *buf;
			// Cyclic buffer. if 'size' > 0, then 'first' is an index to
			// the first used entry, 'size' is the number of used
			// entries in the buffer, and (first + size) % maxSize is the
			// index to just past the end of the buffer.
	size_t maxSize;
	size_t first;
	size_t size;
} BattleInputBuffer;

void setBattleInputDelay(size_t delay);
size_t getBattleInputDelay(void);
void initBattleInputBuffers(void);
void uninitBattleInputBuffers(void);
bool BattleInputBuffer_push(BattleInputBuffer *bib,
		BATTLE_INPUT_STATE input);
bool BattleInputBuffer_pop(BattleInputBuffer *bib,
		BATTLE_INPUT_STATE *input);

BattleInputBuffer *getBattleInputBuffer(size_t player);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_SUPERMELEE_NETPLAY_NETINPUT_H_ */

