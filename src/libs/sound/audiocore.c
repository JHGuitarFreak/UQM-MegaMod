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

/* Audio Core API (derived from OpenAL)
 */

#include <stdio.h>
#include <stdlib.h>
#include "audiocore.h"
#include "sound.h"
#include "libs/log.h"

static audio_Driver audiodrv;

/* The globals that control the sound drivers. */
int snddriver, soundflags;

volatile bool audio_inited = false;

/*
 * Declarations for driver init funcs
 */

#ifdef HAVE_OPENAL
sint32 openAL_Init (audio_Driver *driver, sint32 flags);
#endif
sint32 mixSDL_Init (audio_Driver *driver, sint32 flags);
sint32 noSound_Init (audio_Driver *driver, sint32 flags);


/*
 * Initialization
 */

sint32
initAudio (sint32 driver, sint32 flags)
{
	sint32 ret;

#ifdef HAVE_OPENAL
	if (driver == audio_DRIVER_MIXSDL)
		ret = mixSDL_Init (&audiodrv, flags);
	else if (driver == audio_DRIVER_OPENAL)
		ret = openAL_Init (&audiodrv, flags);
	else
		ret = noSound_Init (&audiodrv, flags);
#else
	if (driver == audio_DRIVER_OPENAL)
	{
		log_add (log_Warning, "OpenAL driver not compiled in, so using MixSDL");
		driver = audio_DRIVER_MIXSDL;
	}
	if (driver == audio_DRIVER_MIXSDL)
		ret = mixSDL_Init (&audiodrv, flags);
	else
		ret = noSound_Init (&audiodrv, flags);
#endif

	if (ret != 0)
	{
		log_add (log_Fatal, "Sound driver initialization failed.\n"
				"This may happen when a soundcard is "
				"not present or not available.\n"
				"NOTICE: Try running UQM with '--sound=none' option");
		exit (EXIT_FAILURE);
	}

	SetSFXVolume (sfxVolumeScale);
	SetSpeechVolume (speechVolumeScale);
	SetMusicVolume (musicVolume);
	
	audio_inited = true;
	
	return ret;
}

void
unInitAudio (void)
{
	if (!audio_inited)
		return;

	audio_inited = false;
	audiodrv.Uninitialize ();
}


/*
 * General
 */

sint32
audio_GetError (void)
{
	return audiodrv.GetError ();
}


/*
 * Sources
 */

void
audio_GenSources (uint32 n, audio_Object *psrcobj)
{
	audiodrv.GenSources (n, psrcobj);
}

void
audio_DeleteSources (uint32 n, audio_Object *psrcobj)
{
	audiodrv.DeleteSources (n, psrcobj);
}

bool
audio_IsSource (audio_Object srcobj)
{
	return audiodrv.IsSource (srcobj);
}

void
audio_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value)

{
	audiodrv.Sourcei (srcobj, audiodrv.EnumLookup[pname], value);
}

void
audio_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value)
{
	audiodrv.Sourcef (srcobj, audiodrv.EnumLookup[pname], value);
}

void
audio_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	audiodrv.Sourcefv (srcobj, audiodrv.EnumLookup[pname], value);
}

void
audio_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value)
{
	audiodrv.GetSourcei (srcobj, audiodrv.EnumLookup[pname], value);
}

void
audio_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value)
{
	audiodrv.GetSourcef (srcobj, audiodrv.EnumLookup[pname], value);
}

void
audio_SourceRewind (audio_Object srcobj)
{
	audiodrv.SourceRewind (srcobj);
}

void
audio_SourcePlay (audio_Object srcobj)
{
	audiodrv.SourcePlay (srcobj);
}

void
audio_SourcePause (audio_Object srcobj)
{
	audiodrv.SourcePause (srcobj);
}

void
audio_SourceStop (audio_Object srcobj)
{
	audiodrv.SourceStop (srcobj);
}

void
audio_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	audiodrv.SourceQueueBuffers (srcobj, n, pbufobj);
}

void
audio_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj)
{
	audiodrv.SourceUnqueueBuffers (srcobj, n, pbufobj);
}


/*
 * Buffers
 */

void
audio_GenBuffers (uint32 n, audio_Object *pbufobj)
{
	audiodrv.GenBuffers (n, pbufobj);
}

void
audio_DeleteBuffers (uint32 n, audio_Object *pbufobj)
{
	audiodrv.DeleteBuffers (n, pbufobj);
}

bool
audio_IsBuffer (audio_Object bufobj)
{
	return audiodrv.IsBuffer (bufobj);
}

void
audio_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value)
{
	audiodrv.GetBufferi (bufobj, audiodrv.EnumLookup[pname], value);
}

void
audio_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq)
{
	audiodrv.BufferData (bufobj, audiodrv.EnumLookup[format], data, size,
			freq);
}

bool
audio_GetFormatInfo (uint32 format, int *channels, int *sample_size)
{
	switch (format)
	{
	case audio_FORMAT_MONO8:
		*channels = 1;
		*sample_size = sizeof (uint8);
		return true;

	case audio_FORMAT_STEREO8:
		*channels = 2;
		*sample_size = sizeof (uint8);
		return true;
	
	case audio_FORMAT_MONO16:
		*channels = 1;
		*sample_size = sizeof (sint16);
		return true;

	case audio_FORMAT_STEREO16:
		*channels = 2;
		*sample_size = sizeof (sint16);
		return true;
	}
	return false;
}
