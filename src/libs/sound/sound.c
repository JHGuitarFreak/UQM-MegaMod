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

#include "sound.h"
#include "sndintrn.h"
#include "libs/compiler.h"
#include "libs/inplib.h"
#include "libs/memlib.h"


int musicVolume = NORMAL_VOLUME;
float musicVolumeScale;
float sfxVolumeScale;
float speechVolumeScale;
TFB_SoundSource soundSource[NUM_SOUNDSOURCES];


void
StopSound (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		StopSource (i);
	}
}

void
CleanSource (int iSource)
{
#define MAX_STACK_BUFFERS 64	
	audio_IntVal processed;

	soundSource[iSource].positional_object = NULL;
	audio_GetSourcei (soundSource[iSource].handle,
			audio_BUFFERS_PROCESSED, &processed);
	if (processed != 0)
	{
		audio_Object stack_bufs[MAX_STACK_BUFFERS];
		audio_Object *bufs;

		if (processed > MAX_STACK_BUFFERS)
			bufs = (audio_Object *) HMalloc (
					sizeof (audio_Object) * processed);
		else
			bufs = stack_bufs;

		audio_SourceUnqueueBuffers (soundSource[iSource].handle,
				processed, bufs);
		
		if (processed > MAX_STACK_BUFFERS)
			HFree (bufs);
	}
	// set the source state to 'initial'
	audio_SourceRewind (soundSource[iSource].handle);
}

void
StopSource (int iSource)
{
	audio_SourceStop (soundSource[iSource].handle);
	CleanSource (iSource);
}

BOOLEAN
SoundPlaying (void)
{
	int i;

	for (i = 0; i < NUM_SOUNDSOURCES; ++i)
	{
		TFB_SoundSample *sample;
		sample = soundSource[i].sample;
		if (sample && sample->decoder)
		{
			BOOLEAN result;
			LockMutex (soundSource[i].stream_mutex);
			result = PlayingStream (i);
			UnlockMutex (soundSource[i].stream_mutex);
			if (result)
				return TRUE;
		}
		else
		{
			audio_IntVal state;
			audio_GetSourcei (soundSource[i].handle, audio_SOURCE_STATE, &state);
			if (state == audio_PLAYING)
				return TRUE;
		}
	}

	return FALSE;
}

// for now just spin in a sleep() loop
// perhaps later change to condvar implementation
void
WaitForSoundEnd (COUNT Channel)
{
	while (Channel == TFBSOUND_WAIT_ALL ?
			SoundPlaying () : ChannelPlaying (Channel))
	{
		SleepThread (ONE_SECOND / 20);
		if (QuitPosted) // Don't make users wait for sounds to end
			break;
	}
}


// Status: Ignored
BOOLEAN
InitSound (int argc, char* argv[])
{
	/* Quell compiler warnings */
	(void)argc;
	(void)argv;
	return TRUE;
}

// Status: Ignored
void
UninitSound (void)
{
}

void
SetSFXVolume (float volume)
{
	int i;
	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		audio_Sourcef (soundSource[i].handle, audio_GAIN, volume);
	}	
}

void
SetSpeechVolume (float volume)
{
	audio_Sourcef (soundSource[SPEECH_SOURCE].handle, audio_GAIN, volume);
}

DWORD
FadeMusic (BYTE end_vol, SIZE TimeInterval)
{
	if (QuitPosted) // Don't make users wait for fades
		TimeInterval = 0;

	if (TimeInterval < 0)
		TimeInterval = 0;

	if (!SetMusicStreamFade (TimeInterval, end_vol))
	{	// fade rejected, maybe due to TimeInterval==0
		SetMusicVolume (end_vol);
		return GetTimeCounter ();
	}
	else
	{
		return GetTimeCounter () + TimeInterval + 1;
	}
}


