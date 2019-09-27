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

#ifndef LIBS_SOUND_OPENAL_AUDIODRV_OPENAL_H_
#define LIBS_SOUND_OPENAL_AUDIODRV_OPENAL_H_

#include "config.h"
#include "libs/sound/sound.h"
#include "endian_uqm.h"

#if defined (__APPLE__)
#	include <OpenAL/al.h>
#	include <OpenAL/alc.h>
#else
#	include <AL/al.h>
#	include <AL/alc.h>
#	ifdef _MSC_VER
#		pragma comment (lib, "OpenAL32.lib")
#	endif
#endif

/* This is just a simple endianness setup for decoders */
#ifdef WORDS_BIGENDIAN
#	define MIX_IS_BIG_ENDIAN   true
#	define MIX_WANT_BIG_ENDIAN true
#else
#	define MIX_IS_BIG_ENDIAN   false
#	define MIX_WANT_BIG_ENDIAN false
#endif


/* General */
sint32 openAL_Init (audio_Driver *driver, sint32 flags);
void openAL_Uninit (void);
sint32 openAL_GetError (void);

/* Sources */
void openAL_GenSources (uint32 n, audio_Object *psrcobj);
void openAL_DeleteSources (uint32 n, audio_Object *psrcobj);
bool openAL_IsSource (audio_Object srcobj);
void openAL_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value);
void openAL_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value);
void openAL_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void openAL_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value);
void openAL_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void openAL_SourceRewind (audio_Object srcobj);
void openAL_SourcePlay (audio_Object srcobj);
void openAL_SourcePause (audio_Object srcobj);
void openAL_SourceStop (audio_Object srcobj);
void openAL_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);
void openAL_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);

/* Buffers */
void openAL_GenBuffers (uint32 n, audio_Object *pbufobj);
void openAL_DeleteBuffers (uint32 n, audio_Object *pbufobj);
bool openAL_IsBuffer (audio_Object bufobj);
void openAL_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value);
void openAL_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);


#endif /* LIBS_SOUND_OPENAL_AUDIODRV_OPENAL_H_ */
