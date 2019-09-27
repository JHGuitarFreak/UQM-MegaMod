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

#include "options.h"
#include "sound.h"
#include "sndintrn.h"
#include "libs/reslib.h"
#include "libs/log.h"
#include "libs/strlib.h"
		// for GetStringAddress()
#include "libs/strings/strintrn.h"
		// for AllocStringTable(), FreeStringTable()
#include "libs/memlib.h"
#include <math.h>


static void CheckFinishedChannels (void);

static const SoundPosition notPositional = {FALSE, 0, 0};

void
PlayChannel (COUNT channel, SOUND snd, SoundPosition pos,
		void *positional_object, unsigned char priority)
{
	SOUNDPTR snd_ptr = GetSoundAddress (snd);
	TFB_SoundSample *sample;

	StopSource (channel);
	// all finished (stopped) channels can be cleaned up at this point
	// since this is the only func that can initiate an sfx sound
	CheckFinishedChannels ();
	
	if (!snd_ptr)
		return; // nothing to play

	sample = *(TFB_SoundSample**) snd_ptr;

	soundSource[channel].sample = sample;
	soundSource[channel].positional_object = positional_object;
	
	UpdateSoundPosition (channel, optStereoSFX ? pos : notPositional);

	audio_Sourcei (soundSource[channel].handle, audio_BUFFER,
			sample->buffer[0]);
	audio_SourcePlay (soundSource[channel].handle);
	(void) priority;
}

void
StopChannel (COUNT channel, unsigned char Priority)
{
	StopSource (channel);
	(void)Priority; // ignored
}

static void
CheckFinishedChannels (void)
{
	int i;

	for (i = FIRST_SFX_SOURCE; i <= LAST_SFX_SOURCE; ++i)
	{
		audio_IntVal state;

		audio_GetSourcei (soundSource[i].handle, audio_SOURCE_STATE,
				&state);
		if (state == audio_STOPPED)
		{
			CleanSource (i);
			// and if it failed... we still dont care
			audio_GetError();
		}
	}
}

BOOLEAN
ChannelPlaying (COUNT WhichChannel)
{
	audio_IntVal state;
	
	audio_GetSourcei (soundSource[WhichChannel].handle,
			audio_SOURCE_STATE, &state);
	if (state == audio_PLAYING)
		return TRUE;
	return FALSE;
}

void *
GetPositionalObject (COUNT channel)
{
	return soundSource[channel].positional_object;
}

void
SetPositionalObject (COUNT channel, void *positional_object)
{
	soundSource[channel].positional_object = positional_object;
}

void
UpdateSoundPosition (COUNT channel, SoundPosition pos)
{
	const float ATTENUATION = 160.0f;
	const float MIN_DISTANCE = 0.5f;
	float fpos[3];

	if (pos.positional)
	{
		float dist;

		fpos[0] = pos.x / ATTENUATION;
		fpos[1] = 0.0f;
		fpos[2] = pos.y / ATTENUATION;
		dist = (float) sqrt (fpos[0] * fpos[0] + fpos[2] * fpos[2]);
		if (dist < MIN_DISTANCE)
		{	// object is too close to listener
			// move it away along the same vector
			float scale = MIN_DISTANCE / dist;
			fpos[0] *= scale;
			fpos[2] *= scale;
		}

		audio_Sourcefv (soundSource[channel].handle, audio_POSITION, fpos);
		//log_add (log_Debug, "UpdateSoundPosition(): channel %d, pos %d %d, posobj %x",
		//		channel, pos.x, pos.y, (unsigned int)soundSource[channel].positional_object);
	}
	else
	{
		fpos[0] = fpos[1] = 0.0f;
		fpos[2] = -1.0f;
		audio_Sourcefv (soundSource[channel].handle, audio_POSITION, fpos);
	}
}

void
SetChannelVolume (COUNT channel, COUNT volume, BYTE priority)
		// I wonder what this whole priority business is...
		// I can probably ignore it.
{
	audio_Sourcef (soundSource[channel].handle, audio_GAIN, 
		(volume / (float)MAX_VOLUME) * sfxVolumeScale);
	(void)priority; // ignored
}

void *
_GetSoundBankData (uio_Stream *fp, DWORD length)
{
	int snd_ct, n;
	DWORD opos;
	char CurrentLine[1024], filename[1024];
#define MAX_FX 256
	TFB_SoundSample *sndfx[MAX_FX];
	STRING_TABLE Snd;
	STRING str;
	int i;

	(void) length;  // ignored
	opos = uio_ftell (fp);

	{
		char *s1, *s2;

		if (_cur_resfile_name == 0
			|| (((s2 = 0), (s1 = strrchr (_cur_resfile_name, '/')) == 0)
			&& (s2 = strrchr (_cur_resfile_name, '\\')) == 0))
			n = 0;
		else
		{
			if (s2 > s1)
				s1 = s2;
			n = s1 - _cur_resfile_name + 1;
			strncpy (filename, _cur_resfile_name, n);
		}
	}

	snd_ct = 0;
	while (uio_fgets (CurrentLine, sizeof (CurrentLine), fp) &&
			snd_ct < MAX_FX)
	{
		TFB_SoundSample* sample;
		TFB_SoundDecoder* decoder;
		uint32 decoded_bytes;

		if (sscanf (CurrentLine, "%s", &filename[n]) != 1)
		{
			log_add (log_Warning, "_GetSoundBankData: bad line: '%s'",
					CurrentLine);
			continue;
		}

		log_add (log_Info, "_GetSoundBankData(): loading %s", filename);

		decoder = SoundDecoder_Load (contentDir, filename, 4096, 0, 0);
		if (!decoder)
		{
			log_add (log_Warning, "_GetSoundBankData(): couldn't load %s",
					filename);
			continue;
		}

		// SFX samples don't have decoders, everything is pre-decoded below
		sample = TFB_CreateSoundSample (NULL, 1, NULL);

		// Decode everything and stash it in 1 buffer
		decoded_bytes = SoundDecoder_DecodeAll (decoder);
		log_add (log_Info, "_GetSoundBankData(): decoded bytes %d",
				decoded_bytes);
		
		audio_BufferData (sample->buffer[0], decoder->format,
			decoder->buffer, decoded_bytes, decoder->frequency);
		// just for informational purposes
		sample->length = decoder->length;

		SoundDecoder_Free (decoder);

		sndfx[snd_ct] = sample;
		++snd_ct;
	}

	if (!snd_ct)
		return NULL; // no sounds decoded

	Snd = AllocStringTable (snd_ct, 0);
	if (!Snd)
	{	// Oops, have to delete everything now
		while (snd_ct--)
			TFB_DestroySoundSample (sndfx[snd_ct]);
		
		return NULL;
	}

	// Populate the STRING_TABLE with ptrs to sample
	for (i = 0, str = Snd->strings; i < snd_ct; ++i, ++str)
	{
		TFB_SoundSample **target = HMalloc (sizeof (sndfx[0]));
		*target = sndfx[i];
		str->data = (STRINGPTR)target;
		str->length = sizeof (sndfx[0]);
	}

	return Snd;
}

BOOLEAN
_ReleaseSoundBankData (void *Snd)
{
	STRING_TABLE fxTab = Snd;
	int index;
	
	if (!fxTab)
		return FALSE;

	for (index = 0; index < fxTab->size; ++index)
	{
		int i;
		void **sptr = (void**)fxTab->strings[index].data;
		TFB_SoundSample *sample = (TFB_SoundSample*)*sptr;

		// Check all sources and see if we are currently playing this sample
		for (i = 0; i < NUM_SOUNDSOURCES; ++i)
		{
			if (soundSource[i].sample == sample)
			{	// Playing this sample. Have to stop it.
				StopSource (i);
				soundSource[i].sample = NULL;
			}
		}

        if (sample->decoder)
			SoundDecoder_Free (sample->decoder);
		sample->decoder = NULL;
		TFB_DestroySoundSample (sample);
		// sptr will be deleted by FreeStringTable() below
	}
	
	FreeStringTable (fxTab);

	return TRUE;
}

BOOLEAN
DestroySound(SOUND_REF target)
{
	return _ReleaseSoundBankData (target);
}

// The type conversions are implicit and will generate errors
// or warnings if types change imcompatibly
SOUNDPTR
GetSoundAddress (SOUND sound)
{
	return GetStringAddress (sound);
}
