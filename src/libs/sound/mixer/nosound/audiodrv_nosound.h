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

#ifndef LIBS_SOUND_MIXER_NOSOUND_AUDIODRV_NOSOUND_H_
#define LIBS_SOUND_MIXER_NOSOUND_AUDIODRV_NOSOUND_H_

#include "config.h"
#include "libs/sound/sound.h"
#include "libs/sound/mixer/mixer.h"


/* Playback task */
int PlaybackTaskFunc (void *data);

/* General */
sint32 noSound_Init (audio_Driver *driver, sint32 flags);
void noSound_Uninit (void);
sint32 noSound_GetError (void);

/* Sources */
void noSound_GenSources (uint32 n, audio_Object *psrcobj);
void noSound_DeleteSources (uint32 n, audio_Object *psrcobj);
bool noSound_IsSource (audio_Object srcobj);
void noSound_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value);
void noSound_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value);
void noSound_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void noSound_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value);
void noSound_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void noSound_SourceRewind (audio_Object srcobj);
void noSound_SourcePlay (audio_Object srcobj);
void noSound_SourcePause (audio_Object srcobj);
void noSound_SourceStop (audio_Object srcobj);
void noSound_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);
void noSound_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);

/* Buffers */
void noSound_GenBuffers (uint32 n, audio_Object *pbufobj);
void noSound_DeleteBuffers (uint32 n, audio_Object *pbufobj);
bool noSound_IsBuffer (audio_Object bufobj);
void noSound_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value);
void noSound_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);


#endif /* LIBS_SOUND_MIXER_NOSOUND_AUDIODRV_NOSOUND_H_ */
