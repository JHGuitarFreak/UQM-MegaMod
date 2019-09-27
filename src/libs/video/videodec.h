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

#ifndef LIBS_VIDEO_VIDEODEC_H_
#define LIBS_VIDEO_VIDEODEC_H_

#include "libs/vidlib.h"
#include "libs/video/video.h"
#include "libs/reslib.h"

// forward-declare
typedef struct tfb_videodecoder TFB_VideoDecoder;

#define THIS_PTR TFB_VideoDecoder*

typedef struct tfb_videodecoderfunc
{
	const char* (* GetName) (void);
	bool (* InitModule) (int flags);
	void (* TermModule) (void);
	uint32 (* GetStructSize) (void);
	int (* GetError) (THIS_PTR);
	bool (* Init) (THIS_PTR, TFB_PixelFormat* fmt);
	void (* Term) (THIS_PTR);
	bool (* Open) (THIS_PTR, uio_DirHandle *dir, const char *filename);
	void (* Close) (THIS_PTR);
	int (* DecodeNext) (THIS_PTR);
	uint32 (* SeekFrame) (THIS_PTR, uint32 frame);
	float (* SeekTime) (THIS_PTR, float time);
	uint32 (* GetFrame) (THIS_PTR);
	float (* GetTime) (THIS_PTR);

} TFB_VideoDecoderFuncs;

// decoder will call these to get info
// from the player
typedef struct tfb_videocallbacks
{
	// any decoder calls these
	void (* BeginFrame) (THIS_PTR);
	void (* EndFrame) (THIS_PTR);
	void* (* GetCanvasLine) (THIS_PTR, uint32 line);
	// non-audio-driven decoders call this to figure out
	// when the next frame should be drawn
	uint32 (* GetTicks) (THIS_PTR);
	// non-audio-driven decoders call this to inform
	// the player when the next frame should be drawn
	bool (* SetTimer) (THIS_PTR, uint32 msecs);

} TFB_VideoCallbacks;

#undef THIS_PTR

struct tfb_videodecoder
{
	// decoder virtual funcs - R/O
	const TFB_VideoDecoderFuncs *funcs;
	// video formats - R/O
	const TFB_PixelFormat *format;
	// decoder-set data - R/O
	uint32 w, h;
	float length; // total length in seconds
	uint32 frame_count;
	uint32 interframe_wait; // nominal interframe delay in msecs
	bool audio_synced;
	// decoder callbacks
	TFB_VideoCallbacks callbacks;

	// other - public
	bool looping;
	void* data; // user-defined data
	// info - public R/O
	sint32 error;
	float pos; // position in seconds
	uint32 cur_frame;

	// semi-private
	uio_DirHandle *dir;
	char *filename;

};

// return values
enum
{
	VIDEODECODER_OK,
	VIDEODECODER_ERROR,
	VIDEODECODER_EOF,
};

typedef struct TFB_RegVideoDecoder TFB_RegVideoDecoder;

TFB_RegVideoDecoder* VideoDecoder_Register (const char* fileext,
		TFB_VideoDecoderFuncs* decvtbl);
void VideoDecoder_Unregister (TFB_RegVideoDecoder* regdec);
const TFB_VideoDecoderFuncs* VideoDecoder_Lookup (const char* fileext);

bool VideoDecoder_Init (int flags, int depth, uint32 Rmask, uint32 Gmask,
		uint32 Bmask, uint32 Amask);
void VideoDecoder_Uninit (void);
TFB_VideoDecoder* VideoDecoder_Load (uio_DirHandle *dir,
		const char *filename);
int VideoDecoder_Decode (TFB_VideoDecoder *decoder);
float VideoDecoder_Seek (TFB_VideoDecoder *decoder, float time_pos);
uint32 VideoDecoder_SeekFrame (TFB_VideoDecoder *decoder, uint32 frame_pos);
void VideoDecoder_Rewind (TFB_VideoDecoder *decoder);
void VideoDecoder_Free (TFB_VideoDecoder *decoder);
const char* VideoDecoder_GetName (TFB_VideoDecoder *decoder);


#endif // LIBS_VIDEO_VIDEODEC_H_
