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

#ifndef LIBS_VIDEO_VIDEO_H_
#define LIBS_VIDEO_VIDEO_H_

#include "libs/vidlib.h"
#include "libs/sndlib.h"
#include "libs/graphics/tfb_draw.h"
#include "types.h"
#include "videodec.h"
#include "libs/sound/sound.h"


typedef struct tfb_videoclip
{
	TFB_VideoDecoder *decoder; // decoder to read from
	float length; // total length of clip seconds
	uint32 w, h;

	// video player data
	RECT dst_rect;     // destination screen rect
	RECT src_rect;     // source rect
	MUSIC_REF hAudio;
	uint32 frame_time; // time when next frame should be rendered
	TFB_Image* frame;  // frame preped and optimized for rendering
	uint32 cur_frame;  // index of frame currently displayed
	bool playing;
	bool own_audio;
	uint32 loop_frame; // frame index to loop from
	uint32 loop_to;    // frame index to loop to

	Mutex guard;
	uint32 want_frame; // audio-signaled desired frame index
	int lag_cnt;       // N of frames video is behind or ahead of audio

	void* data; // user-defined data

} TFB_VideoClip;

extern VIDEO_REF _init_video_file(const char *pStr);

#endif // LIBS_VIDEO_VIDEO_H_
