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

#ifndef LIBS_SOUND_SOUND_H_ // try avoiding collisions on id
#define LIBS_SOUND_SOUND_H_

#include "types.h"
#include "audiocore.h"
#include "decoders/decoder.h"
#include "libs/threadlib.h"
#include "libs/sndlib.h"


#define FIRST_SFX_SOURCE 0
#define LAST_SFX_SOURCE  (FIRST_SFX_SOURCE + NUM_SFX_CHANNELS - 1)
#define MUSIC_SOURCE (LAST_SFX_SOURCE + 1)
#define SPEECH_SOURCE (MUSIC_SOURCE + 1)
#define NUM_SOUNDSOURCES (SPEECH_SOURCE + 1)

typedef struct
{
	int in_use;
	audio_Object buf_name;
	intptr_t data; // user-defined data
} TFB_SoundTag;

typedef struct tfb_soundcallbacks
{
	// return TRUE to continue, FALSE to abort
	bool (* OnStartStream) (TFB_SoundSample*);
	// return TRUE to continue, FALSE to abort
	bool (* OnEndChunk) (TFB_SoundSample*, audio_Object);
	// return TRUE to continue, FALSE to abort
	void (* OnEndStream) (TFB_SoundSample*);
	// tagged buffer callback
	void (* OnTaggedBuffer) (TFB_SoundSample*, TFB_SoundTag*);
	// buffer just queued
	void (* OnQueueBuffer) (TFB_SoundSample*, audio_Object);
} TFB_SoundCallbacks;


extern int musicVolume;
extern float musicVolumeScale;
extern float sfxVolumeScale;
extern float speechVolumeScale;

void StopSource (int iSource);
void CleanSource (int iSource);

void SetSFXVolume (float volume);
void SetSpeechVolume (float volume);

TFB_SoundSample *TFB_CreateSoundSample (TFB_SoundDecoder*, uint32 num_buffers,
		const TFB_SoundCallbacks* /* can be NULL */);
void TFB_DestroySoundSample (TFB_SoundSample*);
void TFB_SetSoundSampleData (TFB_SoundSample*, void* data);
void* TFB_GetSoundSampleData (TFB_SoundSample*);
void TFB_SetSoundSampleCallbacks (TFB_SoundSample*,
		const TFB_SoundCallbacks* /* can be NULL */);
TFB_SoundDecoder* TFB_GetSoundSampleDecoder (TFB_SoundSample*);

TFB_SoundTag* TFB_FindTaggedBuffer (TFB_SoundSample*, audio_Object buffer);
void TFB_ClearBufferTag (TFB_SoundTag*);
bool TFB_TagBuffer (TFB_SoundSample*, audio_Object buffer, intptr_t data);

#include "stream.h"

#endif // LIBS_SOUND_SOUND_H_
