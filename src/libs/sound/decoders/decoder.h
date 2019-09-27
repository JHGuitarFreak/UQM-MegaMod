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

/* Sound file decoder for .wav, .mod, .ogg
 * API is heavily influenced by SDL_sound.
 */

#ifndef DECODER_H
#define DECODER_H

#include "port.h"
#include "types.h"
#include "libs/uio.h"

#ifndef OVCODEC_NONE
#	ifdef _MSC_VER
#		pragma comment (lib, "vorbisfile.lib")
#	endif  /* _MSC_VER */
#endif  /* OVCODEC_NONE */

typedef struct tfb_decoderformats
{
	bool big_endian;
	bool want_big_endian;
	uint32 mono8;
	uint32 stereo8;
	uint32 mono16;
	uint32 stereo16;
} TFB_DecoderFormats;

// forward-declare
typedef struct tfb_sounddecoder TFB_SoundDecoder;

#define THIS_PTR TFB_SoundDecoder*

typedef struct tfb_sounddecoderfunc
{
	const char* (* GetName) (void);
	bool (* InitModule) (int flags, const TFB_DecoderFormats*);
	void (* TermModule) (void);
	uint32 (* GetStructSize) (void);
	int (* GetError) (THIS_PTR);
	bool (* Init) (THIS_PTR);
	void (* Term) (THIS_PTR);
	bool (* Open) (THIS_PTR, uio_DirHandle *dir, const char *filename);
	void (* Close) (THIS_PTR);
	int (* Decode) (THIS_PTR, void* buf, sint32 bufsize);
			// returns <0 on error, ==0 when no more data, >0 bytes returned
	uint32 (* Seek) (THIS_PTR, uint32 pcm_pos);
			// returns the pcm position set
	uint32 (* GetFrame) (THIS_PTR);

} TFB_SoundDecoderFuncs;

#undef THIS_PTR

struct tfb_sounddecoder
{
	// decoder virtual funcs - R/O
	const TFB_SoundDecoderFuncs *funcs;

	// public R/O, set by decoder
	uint32 format;
	uint32 frequency;
	float length; // total length in seconds
	bool is_null;
	bool need_swap;

	// public R/O, set by wrapper
	void *buffer;
	uint32 buffer_size;
	sint32 error;
	uint32 bytes_per_samp;

	// public R/W
	bool looping;

	// semi-private
	uio_DirHandle *dir;
	char *filename;
	uint32 pos;
	uint32 start_sample;
	uint32 end_sample;

};

// return values
enum
{
	SOUNDDECODER_OK,
	SOUNDDECODER_ERROR,
	SOUNDDECODER_EOF,
};

typedef struct TFB_RegSoundDecoder TFB_RegSoundDecoder;

TFB_RegSoundDecoder* SoundDecoder_Register (const char* fileext,
		TFB_SoundDecoderFuncs* decvtbl);
void SoundDecoder_Unregister (TFB_RegSoundDecoder* regdec);
const TFB_SoundDecoderFuncs* SoundDecoder_Lookup (const char* fileext);

void SoundDecoder_SwapWords (uint16* data, uint32 size);
sint32 SoundDecoder_Init (int flags, TFB_DecoderFormats* formats);
void SoundDecoder_Uninit (void);
TFB_SoundDecoder* SoundDecoder_Load (uio_DirHandle *dir,
		char *filename, uint32 buffer_size, uint32 startTime, sint32 runTime);
uint32 SoundDecoder_Decode (TFB_SoundDecoder *decoder);
uint32 SoundDecoder_DecodeAll (TFB_SoundDecoder *decoder);
float SoundDecoder_GetTime (TFB_SoundDecoder *decoder);
uint32 SoundDecoder_GetFrame (TFB_SoundDecoder *decoder);
void SoundDecoder_Seek (TFB_SoundDecoder *decoder, uint32 msecs);
void SoundDecoder_Rewind (TFB_SoundDecoder *decoder);
void SoundDecoder_Free (TFB_SoundDecoder *decoder);
const char* SoundDecoder_GetName (TFB_SoundDecoder *decoder);

#endif
