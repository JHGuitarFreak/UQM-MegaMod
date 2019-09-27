/*
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef TRACKINT_H
#define TRACKINT_H

#include "libs/callback.h"

struct tfb_soundchunk
{
	TFB_SoundDecoder *decoder;  // decoder for this chunk
	float start_time;           // relative time from track start
	int tag_me;                 // set for chunks with subtitles
	uint32 track_num;           // logical track #, comm code needs this
	UNICODE *text;              // subtitle text
	CallbackFunction callback;  // comm callback, executed on chunk start
	struct tfb_soundchunk *next;
};

typedef struct tfb_soundchunk TFB_SoundChunk;

TFB_SoundChunk *create_SoundChunk (TFB_SoundDecoder *decoder, float start_time);
void destroy_SoundChunk_list (TFB_SoundChunk *chain);
TFB_SoundChunk *find_next_page (TFB_SoundChunk *cur);
TFB_SoundChunk *find_prev_page (TFB_SoundChunk *cur);


#endif // TRACKINT_H
