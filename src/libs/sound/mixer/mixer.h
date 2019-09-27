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

/* Mixer for low-level sound output drivers
 */

#ifndef LIBS_SOUND_MIXER_MIXER_H_
#define LIBS_SOUND_MIXER_MIXER_H_

#include "config.h"
#include "types.h"
#include "endian_uqm.h"

/**
 * The interface heavily influenced by OpenAL
 * to the point where you should use OpenAL's
 * documentation when programming the mixer.
 * (some source properties are not supported)
 *
 * EXCEPTION: You may not queue the same buffer
 * on more than one source
 */

#ifdef WORDS_BIGENDIAN
#	define MIX_IS_BIG_ENDIAN   true
#	define MIX_WANT_BIG_ENDIAN true
#else
#	define MIX_IS_BIG_ENDIAN   false
#	define MIX_WANT_BIG_ENDIAN false
#endif

/**
 * Mixer errors (see OpenAL errors)
 */
enum
{
	MIX_NO_ERROR = 0,
	MIX_INVALID_NAME = 0xA001U,
	MIX_INVALID_ENUM = 0xA002U,
	MIX_INVALID_VALUE = 0xA003U,
	MIX_INVALID_OPERATION = 0xA004U,
	MIX_OUT_OF_MEMORY = 0xA005U,

	MIX_DRIVER_FAILURE = 0xA101U
};

/**
 * Source properties (see OpenAL)
 */
typedef enum
{
	MIX_POSITION = 0x1004,
	MIX_LOOPING = 0x1007,
	MIX_BUFFER = 0x1009,
	MIX_GAIN = 0x100A,
	MIX_SOURCE_STATE = 0x1010,

	MIX_BUFFERS_QUEUED = 0x1015,
	MIX_BUFFERS_PROCESSED = 0x1016

} mixer_SourceProp;

/**
 * Source state information
 */
typedef enum
{
	MIX_INITIAL = 0,
	MIX_STOPPED,
	MIX_PLAYING,
	MIX_PAUSED,

} mixer_SourceState;

/** 
 * Sound buffer properties
 */
typedef enum
{
	MIX_FREQUENCY = 0x2001,
	MIX_BITS = 0x2002,
	MIX_CHANNELS = 0x2003,
	MIX_SIZE = 0x2004,
	MIX_DATA = 0x2005

} mixer_BufferProp;

/**
 * Buffer states: semi-private
 */
typedef enum
{
	MIX_BUF_INITIAL = 0,
	MIX_BUF_FILLED,
	MIX_BUF_QUEUED,
	MIX_BUF_PLAYING,
	MIX_BUF_PROCESSED

} mixer_BufferState;

/** Sound buffers: format specifier.
 * bits 00..07: bytes per sample
 * bits 08..15: channels
 * bits 15..31: meaningless
 */
#define MIX_FORMAT_DUMMYID     0x00170000
#define MIX_FORMAT_BPC(f)      ((f) & 0xff)
#define MIX_FORMAT_CHANS(f)    (((f) >> 8) & 0xff)
#define MIX_FORMAT_BPC_MAX     2
#define MIX_FORMAT_CHANS_MAX   2
#define MIX_FORMAT_MAKE(b, c) \
		( MIX_FORMAT_DUMMYID | ((b) & 0xff) | (((c) & 0xff) << 8) )

#define MIX_FORMAT_SAMPSIZE(f) \
		( MIX_FORMAT_BPC(f) * MIX_FORMAT_CHANS(f) )

typedef enum
{
	MIX_FORMAT_MONO8 = MIX_FORMAT_MAKE (1, 1),
	MIX_FORMAT_STEREO8 = MIX_FORMAT_MAKE (1, 2),
	MIX_FORMAT_MONO16 = MIX_FORMAT_MAKE (2, 1),
	MIX_FORMAT_STEREO16 = MIX_FORMAT_MAKE (2, 2)

} mixer_Format;

typedef enum
{
	MIX_QUALITY_LOW = 0,
	MIX_QUALITY_MEDIUM,
	MIX_QUALITY_HIGH,
	MIX_QUALITY_DEFAULT = MIX_QUALITY_MEDIUM,
	MIX_QUALITY_COUNT

} mixer_Quality;

typedef enum
{
	MIX_NOFLAGS = 0,
	MIX_FAKE_DATA = 1
} mixer_Flags;

/*************************************************
 *  Interface Types
 */

typedef intptr_t mixer_Object;
typedef intptr_t mixer_IntVal;

typedef struct _mixer_Source mixer_Source;

typedef struct _mixer_Buffer
{
	uint32 magic;
	bool locked;
	mixer_BufferState state;
	uint8 *data;
	uint32 size;
	uint32 sampsize;
	uint32 high;
	uint32 low;
	float (* Resample) (mixer_Source *src, bool left);
	/* original buffer values for OpenAL compat */
	void* orgdata;
	uint32 orgfreq;
	uint32 orgsize;
	uint32 orgchannels;
	uint32 orgchansize;
	/* next buffer in chain */
	struct _mixer_Buffer *next;

} mixer_Buffer;

#define mixer_bufMagic 0x4258494DU /* MIXB in LSB */

struct _mixer_Source
{
	uint32 magic;
	bool locked;
	mixer_SourceState state;
	bool looping;
	float gain;
	uint32 cqueued;
	uint32 cprocessed;
	mixer_Buffer *firstqueued; /* first buf in the queue */
	mixer_Buffer *nextqueued;  /* next to play, or 0 */
	mixer_Buffer *prevqueued;  /* previously played */
	mixer_Buffer *lastqueued;  /* last in queue */
	uint32 pos; /* position in current buffer */
	uint32 count; /* fractional part of pos */

	float samplecache;

};

#define mixer_srcMagic 0x5358494DU /* MIXS in LSB */

/*************************************************
 *  General interface
 */
uint32 mixer_GetError (void);

bool mixer_Init (uint32 frequency, uint32 format, mixer_Quality quality,
		mixer_Flags flags);
void mixer_Uninit (void);
void mixer_MixChannels (void *userdata, uint8 *stream, sint32 len);
void mixer_MixFake (void *userdata, uint8 *stream, sint32 len);

/*************************************************
 *  Sources
 */
void mixer_GenSources (uint32 n, mixer_Object *psrcobj);
void mixer_DeleteSources (uint32 n, mixer_Object *psrcobj);
bool mixer_IsSource (mixer_Object srcobj);
void mixer_Sourcei (mixer_Object srcobj, mixer_SourceProp pname,
		mixer_IntVal value);
void mixer_Sourcef (mixer_Object srcobj, mixer_SourceProp pname,
		float value);
void mixer_Sourcefv (mixer_Object srcobj, mixer_SourceProp pname,
		float *value);
void mixer_GetSourcei (mixer_Object srcobj, mixer_SourceProp pname,
		mixer_IntVal *value);
void mixer_GetSourcef (mixer_Object srcobj, mixer_SourceProp pname,
		float *value);
void mixer_SourceRewind (mixer_Object srcobj);
void mixer_SourcePlay (mixer_Object srcobj);
void mixer_SourcePause (mixer_Object srcobj);
void mixer_SourceStop (mixer_Object srcobj);
void mixer_SourceQueueBuffers (mixer_Object srcobj, uint32 n,
		mixer_Object* pbufobj);
void mixer_SourceUnqueueBuffers (mixer_Object srcobj, uint32 n,
		mixer_Object* pbufobj);

/*************************************************
 *  Buffers
 */
void mixer_GenBuffers (uint32 n, mixer_Object *pbufobj);
void mixer_DeleteBuffers (uint32 n, mixer_Object *pbufobj);
bool mixer_IsBuffer (mixer_Object bufobj);
void mixer_GetBufferi (mixer_Object bufobj, mixer_BufferProp pname,
		mixer_IntVal *value);
void mixer_BufferData (mixer_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);


/* Make sure the prop-value type is of suitable size
 * it must be able to store both int and void*
 * Adapted from SDL
 * This will generate "negative subscript or subscript is too large"
 * error during compile, if the actual size of a type is wrong
 */
#define MIX_COMPILE_TIME_ASSERT(name, x) \
	typedef int mixer_dummy_##name [(x) * 2 - 1]

MIX_COMPILE_TIME_ASSERT (mixer_Object,
		sizeof(mixer_Object) >= sizeof(void*));
MIX_COMPILE_TIME_ASSERT (mixer_IntVal,
		sizeof(mixer_IntVal) >= sizeof(mixer_Object));

#undef MIX_COMPILE_TIME_ASSERT

#endif /* LIBS_SOUND_MIXER_MIXER_H_ */
