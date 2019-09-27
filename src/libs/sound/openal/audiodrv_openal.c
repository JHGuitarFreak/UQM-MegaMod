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

/* OpenAL audio driver
 */

#ifdef HAVE_OPENAL


#include "audiodrv_openal.h"
#include "../sndintrn.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include <stdlib.h>


ALCcontext *alcContext = NULL;
ALCdevice *alcDevice = NULL;
ALfloat defaultPos[] = {0.0f, 0.0f, -1.0f};
ALfloat listenerPos[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerVel[] = {0.0f, 0.0f, 0.0f};
ALfloat listenerOri[] = {0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f};

static const audio_Driver openAL_Driver =
{
	openAL_Uninit,
	openAL_GetError,
	audio_DRIVER_OPENAL,
	{
		/* Errors */
		AL_FALSE,
		AL_INVALID_NAME,
		AL_INVALID_ENUM,
		AL_INVALID_VALUE,
		AL_INVALID_OPERATION,
		AL_OUT_OF_MEMORY,
		audio_DRIVER_FAILURE,

		/* Source properties */
		AL_POSITION,
		AL_LOOPING,
		AL_BUFFER,
		AL_GAIN,
		AL_SOURCE_STATE,
		AL_BUFFERS_QUEUED,
		AL_BUFFERS_PROCESSED,

		/* Source state information */
		AL_INITIAL,
		AL_STOPPED,
		AL_PLAYING,
		AL_PAUSED,

		/* Sound buffer properties */ 
		AL_FREQUENCY,
		AL_BITS,
		AL_CHANNELS,
		AL_SIZE,
		AL_FORMAT_MONO16,
		AL_FORMAT_STEREO16,
		AL_FORMAT_MONO8,
		AL_FORMAT_STEREO8
	},

	/* Sources */
	openAL_GenSources,
	openAL_DeleteSources,
	openAL_IsSource,
	openAL_Sourcei,
	openAL_Sourcef,
	openAL_Sourcefv,
	openAL_GetSourcei,
	openAL_GetSourcef,
	openAL_SourceRewind,
	openAL_SourcePlay,
	openAL_SourcePause,
	openAL_SourceStop,
	openAL_SourceQueueBuffers,
	openAL_SourceUnqueueBuffers,

	/* Buffers */
	openAL_GenBuffers,
	openAL_DeleteBuffers,
	openAL_IsBuffer,
	openAL_GetBufferi,
	openAL_BufferData
};


/*
 * Initialization
 */

sint32
openAL_Init (audio_Driver *driver, sint32 flags)
{
	int i;
	TFB_DecoderFormats formats =
	{
		MIX_IS_BIG_ENDIAN, MIX_WANT_BIG_ENDIAN,
		audio_FORMAT_MONO8, audio_FORMAT_STEREO8,
		audio_FORMAT_MONO16, audio_FORMAT_STEREO16
	};
	
	log_add (log_Info, "Initializing OpenAL.");
	alcDevice = alcOpenDevice (NULL);

	if (!alcDevice)
	{
		log_add (log_Error, "Couldn't initialize OpenAL: %d",
				alcGetError (NULL));
		return -1;
	}

	*driver = openAL_Driver;

	alcContext = alcCreateContext (alcDevice, NULL);
	if (!alcContext)
	{
		log_add (log_Error, "Couldn't create OpenAL context: %d",
				alcGetError (alcDevice));
		alcCloseDevice (alcDevice);
		alcDevice = NULL;
		return -1;
	}

	alcMakeContextCurrent (alcContext);

	log_add (log_Info, "OpenAL initialized.\n"
			"    version:     %s\n"
			"    vendor:      %s\n"
			"    renderer:    %s\n"
			"    device:      %s",
			alGetString (AL_VERSION), alGetString (AL_VENDOR),
			alGetString (AL_RENDERER),
			alcGetString (alcDevice, ALC_DEFAULT_DEVICE_SPECIFIER));
    //log_add (log_Info, "    extensions:  %s", alGetString (AL_EXTENSIONS));
		
	log_add (log_Info, "Initializing sound decoders.");
	if (SoundDecoder_Init (flags, &formats))
	{
		log_add (log_Error, "Sound decoders initialization failed.");
		alcMakeContextCurrent (NULL);
		alcDestroyContext (alcContext);
		alcContext = NULL;
		alcCloseDevice (alcDevice);
		alcDevice = NULL;
		return -1;
	}
	log_add (log_Error, "Sound decoders initialized.");

	alListenerfv (AL_POSITION, listenerPos);
	alListenerfv (AL_VELOCITY, listenerVel);
	alListenerfv (AL_ORIENTATION, listenerOri);

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		float zero[3] = {0.0f, 0.0f, 0.0f};
		
		alGenSources (1, &soundSource[i].handle);
		alSourcei (soundSource[i].handle, AL_LOOPING, AL_FALSE);
		alSourcefv (soundSource[i].handle, AL_POSITION, defaultPos);
		alSourcefv (soundSource[i].handle, AL_VELOCITY, zero);
		alSourcefv (soundSource[i].handle, AL_DIRECTION, zero);
		
		soundSource[i].stream_mutex = CreateMutex ("OpenAL stream mutex", SYNC_CLASS_AUDIO);
	}

	if (InitStreamDecoder ())
	{
		log_add (log_Error, "Stream decoder initialization failed.");
		// TODO: cleanup source mutexes [or is it "muti"? :) ]
		SoundDecoder_Uninit ();
		alcMakeContextCurrent (NULL);
		alcDestroyContext (alcContext);
		alcContext = NULL;
		alcCloseDevice (alcDevice);
		alcDevice = NULL;
		return -1;
	}
	
	alDistanceModel (AL_INVERSE_DISTANCE);

	(void) driver; // eat compiler warning

	return 0;
}

void
openAL_Uninit (void)
{
	int i;

	UninitStreamDecoder ();

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		if (soundSource[i].sample && soundSource[i].sample->decoder)
		{
			StopStream (i);
		}
		if (soundSource[i].sbuffer)
		{
			void *sbuffer = soundSource[i].sbuffer;
			soundSource[i].sbuffer = NULL;
			HFree (sbuffer);
		}
		DestroyMutex (soundSource[i].stream_mutex);
	}

	alcMakeContextCurrent (NULL);
	alcDestroyContext (alcContext);
	alcContext = NULL;
	alcCloseDevice (alcDevice);
	alcDevice = NULL;

	SoundDecoder_Uninit ();
}


/*
 * General
 */

sint32
openAL_GetError (void)
{
	ALint value = alGetError ();
	switch (value)
	{
		case AL_FALSE:
			return audio_NO_ERROR;
		case AL_INVALID_NAME:
			return audio_INVALID_NAME;
		case AL_INVALID_ENUM:
			return audio_INVALID_ENUM;
		case AL_INVALID_VALUE:
			return audio_INVALID_VALUE;
		case AL_INVALID_OPERATION:
			return audio_INVALID_OPERATION;
		case AL_OUT_OF_MEMORY:
			return audio_OUT_OF_MEMORY;
		default:
			log_add (log_Debug, "openAL_GetError: unknown value %x", value);
			return audio_DRIVER_FAILURE;
			break;
	}
}


/*
 * Sources
 */

void
openAL_GenSources (uint32 n, audio_Object *psrcobj)
{
	alGenSources ((ALsizei) n, (ALuint *) psrcobj);
}

void
openAL_DeleteSources (uint32 n, audio_Object *psrcobj)
{
	alDeleteSources ((ALsizei) n, (ALuint *) psrcobj);
}

bool
openAL_IsSource (audio_Object srcobj)
{
	return alIsSource ((ALuint) srcobj);
}

void
openAL_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value)

{
	alSourcei ((ALuint) srcobj, (ALenum) pname, (ALint) value);
}

void
openAL_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value)
{
	alSourcef ((ALuint) srcobj, (ALenum) pname, (ALfloat) value);
}

void
openAL_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	alSourcefv ((ALuint) srcobj, (ALenum) pname, (ALfloat *) value);
}

void
openAL_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value)
{
	alGetSourcei ((ALuint) srcobj, (ALenum) pname, (ALint *) value);
	if (pname == AL_SOURCE_STATE)
	{
		switch (*value)
		{
			case AL_INITIAL:
				*value = audio_INITIAL;
				break;
			case AL_STOPPED:
				*value = audio_STOPPED;
				break;
			case AL_PLAYING:
				*value = audio_PLAYING;
				break;
			case AL_PAUSED:
				*value = audio_PAUSED;
				break;
			default:
				log_add (log_Debug, "openAL_GetSourcei(): unknown value %x",
						*value);
				*value = audio_DRIVER_FAILURE;
		}
	}
}

void
openAL_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	alGetSourcef ((ALuint) srcobj, (ALenum) pname, (ALfloat *) value);
}

void
openAL_SourceRewind (audio_Object srcobj)
{
	alSourceRewind ((ALuint) srcobj);
}

void
openAL_SourcePlay (audio_Object srcobj)
{
	alSourcePlay ((ALuint) srcobj);
}

void
openAL_SourcePause (audio_Object srcobj)
{
	alSourcePause ((ALuint) srcobj);
}

void
openAL_SourceStop (audio_Object srcobj)
{
	alSourceStop ((ALuint) srcobj);
}

void
openAL_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	alSourceQueueBuffers ((ALuint) srcobj, (ALsizei) n, (ALuint *) pbufobj);
}

void
openAL_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	alSourceUnqueueBuffers ((ALuint) srcobj, (ALsizei) n, (ALuint *) pbufobj);
}


/*
 * Buffers
 */

void
openAL_GenBuffers (uint32 n, audio_Object *pbufobj)
{
	alGenBuffers ((ALsizei) n, (ALuint *) pbufobj);
}

void
openAL_DeleteBuffers (uint32 n, audio_Object *pbufobj)
{
	alDeleteBuffers ((ALsizei) n, (ALuint *) pbufobj);
}

bool
openAL_IsBuffer (audio_Object bufobj)
{
	return alIsBuffer ((ALuint) bufobj);
}

void
openAL_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value)
{
	alGetBufferi ((ALuint) bufobj, (ALenum) pname, (ALint *) value);
}

void
openAL_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	alBufferData ((ALuint) bufobj, (ALenum) format, (ALvoid *) data,
			(ALsizei) size, (ALsizei) freq);
}

#endif
