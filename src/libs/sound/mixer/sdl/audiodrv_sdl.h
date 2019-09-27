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

/* SDL audio driver
 */

#ifndef LIBS_SOUND_MIXER_SDL_AUDIODRV_SDL_H_
#define LIBS_SOUND_MIXER_SDL_AUDIODRV_SDL_H_

#include "port.h"
#include "libs/sound/sound.h"
#include "libs/sound/mixer/mixer.h"
#include SDL_INCLUDE(SDL.h)

/* General */
sint32 mixSDL_Init (audio_Driver *driver, sint32 flags);
void mixSDL_Uninit (void);
sint32 mixSDL_GetError (void);

/* Sources */
void mixSDL_GenSources (uint32 n, audio_Object *psrcobj);
void mixSDL_DeleteSources (uint32 n, audio_Object *psrcobj);
bool mixSDL_IsSource (audio_Object srcobj);
void mixSDL_Sourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal value);
void mixSDL_Sourcef (audio_Object srcobj, audio_SourceProp pname,
		float value);
void mixSDL_Sourcefv (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void mixSDL_GetSourcei (audio_Object srcobj, audio_SourceProp pname,
		audio_IntVal *value);
void mixSDL_GetSourcef (audio_Object srcobj, audio_SourceProp pname,
		float *value);
void mixSDL_SourceRewind (audio_Object srcobj);
void mixSDL_SourcePlay (audio_Object srcobj);
void mixSDL_SourcePause (audio_Object srcobj);
void mixSDL_SourceStop (audio_Object srcobj);
void mixSDL_SourceQueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);
void mixSDL_SourceUnqueueBuffers (audio_Object srcobj, uint32 n,
		audio_Object* pbufobj);

/* Buffers */
void mixSDL_GenBuffers (uint32 n, audio_Object *pbufobj);
void mixSDL_DeleteBuffers (uint32 n, audio_Object *pbufobj);
bool mixSDL_IsBuffer (audio_Object bufobj);
void mixSDL_GetBufferi (audio_Object bufobj, audio_BufferProp pname,
		audio_IntVal *value);
void mixSDL_BufferData (audio_Object bufobj, uint32 format, void* data,
		uint32 size, uint32 freq);


#endif /* LIBS_SOUND_MIXER_SDL_AUDIODRV_SDL_H_ */
