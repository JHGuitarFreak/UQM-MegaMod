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

/* Nosound audio driver
 */

#include "audiodrv_nosound.h"
#include "../../sndintrn.h"
#include "libs/tasklib.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include <stdlib.h>


static Task PlaybackTask;
static uint32 nosound_freq = 22050;

static const audio_Driver noSound_Driver =
{
	noSound_Uninit,
	noSound_GetError,
	audio_DRIVER_NOSOUND,
	{
		/* Errors */
		MIX_NO_ERROR,
		MIX_INVALID_NAME,
		MIX_INVALID_ENUM,
		MIX_INVALID_VALUE,
		MIX_INVALID_OPERATION,
		MIX_OUT_OF_MEMORY,
		MIX_DRIVER_FAILURE,

		/* Source properties */
		MIX_POSITION,
		MIX_LOOPING,
		MIX_BUFFER,
		MIX_GAIN,
		MIX_SOURCE_STATE,
		MIX_BUFFERS_QUEUED,
		MIX_BUFFERS_PROCESSED,

		/* Source state information */
		MIX_INITIAL,
		MIX_STOPPED,
		MIX_PLAYING,
		MIX_PAUSED,

		/* Sound buffer properties */ 
		MIX_FREQUENCY,
		MIX_BITS,
		MIX_CHANNELS,
		MIX_SIZE,
		MIX_FORMAT_MONO16,
		MIX_FORMAT_STEREO16,
		MIX_FORMAT_MONO8,
		MIX_FORMAT_STEREO8
	},

	/* Sources */
	noSound_GenSources,
	noSound_DeleteSources,
	noSound_IsSource,
	noSound_Sourcei,
	noSound_Sourcef,
	noSound_Sourcefv,
	noSound_GetSourcei,
	noSound_GetSourcef,
	noSound_SourceRewind,
	noSound_SourcePlay,
	noSound_SourcePause,
	noSound_SourceStop,
	noSound_SourceQueueBuffers,
	noSound_SourceUnqueueBuffers,

	/* Buffers */
	noSound_GenBuffers,
	noSound_DeleteBuffers,
	noSound_IsBuffer,
	noSound_GetBufferi,
	noSound_BufferData
};

/* Adapted from SDL
 * This will generate "negative subscript or subscript is too large"
 * error during compile, if the size of a type is wrong
 */
#define UQM_COMPILE_TIME_ASSERT(name, x) \
	typedef int UQM_dummy_##name [(x) * 2 - 1]

UQM_COMPILE_TIME_ASSERT (mixer_Object_fits_in_audio_Object,
	sizeof (mixer_Object) <= sizeof (audio_Object));

#undef UQM_COMPILE_TIME_ASSERT

// Converts an array of n audio_Objects to an array of mixer_Objects, in place.
static void
noSound_ConvertObjectArrayToMixerObjects (uint32 n, audio_Object *arr)
{
	if (sizeof (audio_Object) == sizeof (mixer_Object))
		return;
	uint32 i;
	for (i = 0; i < n; i++)
	{
		((mixer_Object *) arr)[i] = arr[i];
	}

}
// Converts an array of n mixer_Objects to an array of audio_Objects, in place.
static void
noSound_ConvertObjectArrayFromMixerObjects (uint32 n, audio_Object *arr)
{
	if (sizeof (audio_Object) == sizeof (mixer_Object))
		return;
	uint32 i = n;
	while (i--)
	{
		arr[i] = ((mixer_Object *) arr)[i];
	}
}

/*
 * Initialization
 */

sint32 
noSound_Init (audio_Driver *driver, sint32 flags)
{
	int i;
	TFB_DecoderFormats formats =
	{
		0, 0, 
		audio_FORMAT_MONO8, audio_FORMAT_STEREO8,
		audio_FORMAT_MONO16, audio_FORMAT_STEREO16
	};
	
	log_add (log_Info, "Using nosound audio driver.");
	log_add (log_Info, "Initializing mixer.");

	if (!mixer_Init (nosound_freq, MIX_FORMAT_MAKE (1, 1),
			MIX_QUALITY_LOW, MIX_FAKE_DATA))
	{
		log_add (log_Error, "Mixer initialization failed: %x",
				mixer_GetError ());
		return -1;
	}
	log_add (log_Info, "Mixer initialized.");

	log_add (log_Info, "Initializing sound decoders.");
	if (SoundDecoder_Init (flags, &formats))
	{
		log_add (log_Error, "Sound decoders initialization failed.");
		mixer_Uninit ();
		return -1;
	}
	log_add (log_Info, "Sound decoders initialized.");

	*driver = noSound_Driver;
	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		audio_GenSources (1, &soundSource[i].handle);		
		soundSource[i].stream_mutex = CreateMutex ("Nosound stream mutex", SYNC_CLASS_AUDIO);
	}

	if (InitStreamDecoder ())
	{
		log_add (log_Error, "Stream decoder initialization failed.");
		// TODO: cleanup source mutexes [or is it "muti"? :) ]
		SoundDecoder_Uninit ();
		mixer_Uninit ();
		return -1;
	}

	PlaybackTask = AssignTask (PlaybackTaskFunc, 1024, 
		"nosound audio playback");

	return 0;
}

void
noSound_Uninit (void)
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

		noSound_DeleteSources (1, &soundSource[i].handle);
	}

	if (PlaybackTask)
	{
		ConcludeTask (PlaybackTask);
		PlaybackTask = 0;
	}

	mixer_Uninit ();
	SoundDecoder_Uninit ();
}


/*
 * Playback task
 */

int
PlaybackTaskFunc (void *data)
{
	Task task = (Task)data;
	uint8 *stream;
	uint32 entryTime;
	sint32 period, delay;
	uint32 len = 2048;
	
	stream = (uint8 *) HMalloc (len);
	period = (sint32)((len / (double)nosound_freq) * ONE_SECOND);

	while (!Task_ReadState (task, TASK_EXIT))
	{
		entryTime = GetTimeCounter ();
		mixer_MixFake (NULL, stream, len);
		delay = period - (GetTimeCounter () - entryTime);
		if (delay > 0)
			HibernateThread (delay);
	}

	HFree (stream);
	FinishTask (task);
	return 0;
}


/*
 * General
 */

sint32
noSound_GetError (void)
{
	sint32 value = mixer_GetError ();
	switch (value)
	{
		case MIX_NO_ERROR:
			return audio_NO_ERROR;
		case MIX_INVALID_NAME:
			return audio_INVALID_NAME;
		case MIX_INVALID_ENUM:
			return audio_INVALID_ENUM;
		case MIX_INVALID_VALUE:
			return audio_INVALID_VALUE;
		case MIX_INVALID_OPERATION:
			return audio_INVALID_OPERATION;
		case MIX_OUT_OF_MEMORY:
			return audio_OUT_OF_MEMORY;
		default:
			log_add (log_Debug, "noSound_GetError: unknown value %x",
					value);
			return audio_DRIVER_FAILURE;
			break;
	}
}


/*
 * Sources
 */

void
noSound_GenSources (uint32 n, audio_Object *psrcobj)
{
	mixer_GenSources (n, (mixer_Object *) psrcobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, psrcobj);
}

void
noSound_DeleteSources (uint32 n, audio_Object *psrcobj)
{
	noSound_ConvertObjectArrayToMixerObjects (n, psrcobj);
	mixer_DeleteSources (n, (mixer_Object *) psrcobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, psrcobj);
}

bool
noSound_IsSource (audio_Object srcobj)
{
	return mixer_IsSource ((mixer_Object) srcobj);
}

void
noSound_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value)

{
	mixer_Sourcei ((mixer_Object) srcobj, (mixer_SourceProp) pname,
			(mixer_IntVal) value);
}

void
noSound_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value)
{
	mixer_Sourcef ((mixer_Object) srcobj, (mixer_SourceProp) pname, value);
}

void
noSound_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	mixer_Sourcefv ((mixer_Object) srcobj, (mixer_SourceProp) pname, value);
}

void
noSound_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value)
{
	mixer_IntVal temp = *value;
	mixer_GetSourcei ((mixer_Object) srcobj, (mixer_SourceProp) pname,
			&temp);
	*value = temp;
	if (pname == MIX_SOURCE_STATE)
	{
		switch (*value)
		{
			case MIX_INITIAL:
				*value = audio_INITIAL;
				break;
			case MIX_STOPPED:
				*value = audio_STOPPED;
				break;
			case MIX_PLAYING:
				*value = audio_PLAYING;
				break;
			case MIX_PAUSED:
				*value = audio_PAUSED;
				break;
			default:
				log_add (log_Debug, "noSound_GetSourcei(): unknown value %lx",
						(long int) *value);
				*value = audio_DRIVER_FAILURE;
		}
	}
}

void
noSound_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	mixer_GetSourcef ((mixer_Object) srcobj, (mixer_SourceProp) pname, value);
}

void
noSound_SourceRewind (audio_Object srcobj)
{
	mixer_SourceRewind ((mixer_Object) srcobj);
}

void
noSound_SourcePlay (audio_Object srcobj)
{
	mixer_SourcePlay ((mixer_Object) srcobj);
}

void
noSound_SourcePause (audio_Object srcobj)
{
	mixer_SourcePause ((mixer_Object) srcobj);
}

void
noSound_SourceStop (audio_Object srcobj)
{
	mixer_SourceStop ((mixer_Object) srcobj);
}

void
noSound_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects (n, pbufobj);
	mixer_SourceQueueBuffers ((mixer_Object) srcobj, n,
			(mixer_Object *) pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, pbufobj);
}

void
noSound_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects (n, pbufobj);
	mixer_SourceUnqueueBuffers ((mixer_Object) srcobj, n,
			(mixer_Object *) pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, pbufobj);
}


/*
 * Buffers
 */

void
noSound_GenBuffers (uint32 n, audio_Object *pbufobj)
{
	mixer_GenBuffers (n, (mixer_Object *) pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, pbufobj);
}

void
noSound_DeleteBuffers (uint32 n, audio_Object *pbufobj)
{
	noSound_ConvertObjectArrayToMixerObjects (n, pbufobj);
	mixer_DeleteBuffers (n, (mixer_Object *) pbufobj);
	noSound_ConvertObjectArrayFromMixerObjects (n, pbufobj);
}

bool
noSound_IsBuffer (audio_Object bufobj)
{
	return mixer_IsBuffer ((mixer_Object) bufobj);
}

void
noSound_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value)
{
	mixer_IntVal temp = *value;
	mixer_GetBufferi ((mixer_Object) bufobj, (mixer_BufferProp) pname,
			&temp);
	*value = temp;
}

void
noSound_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	mixer_BufferData ((mixer_Object) bufobj, format, data, size, freq);
}
