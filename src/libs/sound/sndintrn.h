//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifndef LIBS_SOUND_SNDINTRN_H_
#define LIBS_SOUND_SNDINTRN_H_

#include <stdio.h>
#include "types.h"
#include "libs/reslib.h"
#include "libs/memlib.h"

#define PAD_SCOPE_BYTES 256

extern void *_GetMusicData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseMusicData (void *handle);

extern void *_GetSoundBankData (uio_Stream *fp, DWORD length);
extern BOOLEAN _ReleaseSoundBankData (void *handle);

#define AllocMusicData HMalloc
#define FreeMusicData  HFree

extern char* CheckMusicResName (char* filename);

// audio data
struct tfb_soundsample
{
	TFB_SoundDecoder *decoder; // decoder to read from
	float length; // total length of decoder chain in seconds
	audio_Object *buffer;
	uint32 num_buffers;
	TFB_SoundTag *buffer_tag;
	sint32 offset; // initial offset
	void* data; // user-defined data
	TFB_SoundCallbacks callbacks; // user-defined callbacks
};

// equivalent to channel in legacy sound code
typedef struct tfb_soundsource
{
	TFB_SoundSample *sample;
	audio_Object handle;
	bool stream_should_be_playing;
	Mutex stream_mutex;
	sint32 start_time;       // for tracks played-time math
	uint32 pause_time;       // keep track for paused tracks
	void *positional_object;

	audio_Object last_q_buf; // for callbacks processing

	// Cyclic waveform buffer for oscilloscope
	void *sbuffer; 
	uint32 sbuf_size;
	uint32 sbuf_tail;
	uint32 sbuf_head;
	uint32 sbuf_lasttime;    // timestamp of the first queued buffer
} TFB_SoundSource;

extern TFB_SoundSource soundSource[];

#endif /* LIBS_SOUND_SNDINTRN_H_ */
