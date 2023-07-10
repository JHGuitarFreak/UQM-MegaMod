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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "settings.h"
#include "sounds.h"
#include "units.h"

SOUND MenuSounds;
SOUND GameSounds;

#define MAX_SOUNDS 8
static BYTE num_sounds = 0;
static SOUND sound_buf[MAX_SOUNDS];
static ELEMENT *sound_posobj[MAX_SOUNDS];

// For music resume option
MUSIC_POSITION MainMenuMusic;
MUSIC_POSITION MeleeMenuMusic;
MUSIC_POSITION StarBaseMusic;
MUSIC_POSITION OutfitMusic;
MUSIC_POSITION ShipyardMusic;
MUSIC_POSITION CommMusic[NUM_CONVERSATIONS];
MUSIC_POSITION IPMusic[NUM_SPECIES_ID];
MUSIC_POSITION PlanetMusic[NUM_ORBIT_THEMES];
MUSIC_POSITION BattleRefMusic[3];

void
PlaySound (SOUND S, SoundPosition Pos, ELEMENT *PositionalObject,
		BYTE Priority)
{
	BYTE chan, c;
	static BYTE lru_channel[NUM_FX_CHANNELS] = {0, 1, 2, 3};
	static SOUND channel[NUM_FX_CHANNELS] = {0, 0, 0, 0};

	if (S == 0)
		return;

	for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
	{
		if (S == channel[chan])
			break;
	}

	if (chan == NUM_FX_CHANNELS)
	{
		for (chan = 0; chan < NUM_FX_CHANNELS; ++chan)
		{
			if (!ChannelPlaying (chan + MIN_FX_CHANNEL))
				break;
		}

		if (chan == NUM_FX_CHANNELS)
			chan = lru_channel[0];
	}

	channel[chan] = S;

	for (c = 0; c < NUM_FX_CHANNELS - 1; ++c)
	{
		if (lru_channel[c] == chan)
		{
			memmove (&lru_channel[c], &lru_channel[c + 1],
					(NUM_FX_CHANNELS - 1) - c);
			break;
		}
	}
	lru_channel[NUM_FX_CHANNELS - 1] = chan;

	PlaySoundEffect (S, chan + MIN_FX_CHANNEL, Pos, PositionalObject, Priority);
}

void
PlayMenuSound (MENU_SOUND_EFFECT S)
{
	PlaySoundEffect (SetAbsSoundIndex (MenuSounds, S),
			0, NotPositional (), NULL,
			GAME_SOUND_PRIORITY);
}

void
ProcessSound (SOUND Sound, ELEMENT *PositionalObject)
{
	if (Sound == (SOUND)~0)
	{
		memset (sound_buf, 0, sizeof (sound_buf));
		memset (sound_posobj, 0, sizeof (sound_posobj));
		num_sounds = MAX_SOUNDS;
	}
	else if (num_sounds < MAX_SOUNDS)
	{
		sound_buf[num_sounds] = Sound;
		sound_posobj[num_sounds++] = PositionalObject;
	}
}

SoundPosition
CalcSoundPosition (ELEMENT *ElementPtr)
{
	SoundPosition pos;

	if (ElementPtr == NULL)
	{
		pos.x = pos.y = 0;
		pos.positional = FALSE;
	}
	else
	{
		GRAPHICS_PRIM objtype;
		
		objtype = GetPrimType (&DisplayArray[ElementPtr->PrimIndex]);
		if (objtype == LINE_PRIM)
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Line.first.y;
		}
		else
		{
			pos.x = DisplayArray[ElementPtr->PrimIndex].Object.Point.x;
			pos.y = DisplayArray[ElementPtr->PrimIndex].Object.Point.y;
		}

		pos.x -= (SPACE_WIDTH >> 1);
		pos.y -= (SPACE_HEIGHT >> 1);
		pos.x = RES_DESCALE (pos.x);
		pos.y = RES_DESCALE (pos.y);
		pos.positional = TRUE;
	}

	return pos;
}

SoundPosition
NotPositional (void)
{
	return CalcSoundPosition (NULL);
}

/* Updates positional sound effects */
void
UpdateSoundPositions (void)
{
	COUNT i;

	for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
	{
		ELEMENT *posobj;
		if (!ChannelPlaying(i))
			continue;

		posobj = GetPositionalObject (i);
		if (posobj != NULL)
		{
			SoundPosition pos;
			pos = CalcSoundPosition (posobj);
			if (pos.positional)
				UpdateSoundPosition (i, pos);
		}
	}
}

void
FlushSounds (void)
{
	if (num_sounds > 0)
	{
		SOUND *pSound;
		ELEMENT **pSoundPosObj;

		pSound = sound_buf;
		pSoundPosObj = sound_posobj;
		do
		{
			SoundPosition pos = CalcSoundPosition (*pSoundPosObj);
			PlaySound (*pSound++, pos, *pSoundPosObj++,
					GAME_SOUND_PRIORITY);
		} while (--num_sounds);
	}
}

void
RemoveSoundsForObject (ELEMENT *PosObj)
{
	int i;

	for (i = FIRST_SFX_CHANNEL; i <= LAST_SFX_CHANNEL; ++i)
	{
		if (GetPositionalObject (i) == PosObj)
			SetPositionalObject (i, NULL);
	}

	for (i = 0; i < num_sounds; ++i)
	{
		if (sound_posobj[i] == PosObj)
			sound_posobj[i] = NULL;
	}
}

static MUSIC_POSITION resumeMusicArray[PATH_MAX];

void
InitializeMusicArray (void)
{
	int i;

	for (i = 0; i < PATH_MAX; ++i)
	{
		utf8StringCopy (&resumeMusicArray[i].filename,
				sizeof resumeMusicArray[i].filename, STR_NULL);
	}
}

static void
SetGlobalMusicPosition (void)
{
	MUSIC_POSITION temp;
	int i, j;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
	if (!optMusicResume)
		return;

	utf8StringCopy (temp.filename, sizeof temp.filename,PLRGetFilename ());
	temp.position = PLRGetPos ();
	temp.last_played = GetTimeCounter ();

	if (temp.filename == NULL)
		return;
	if (&resumeMusicArray[0].filename == NULL)
		return;

	for (i = 0; i < PATH_MAX; ++i)
	{
		if (strcmp (&resumeMusicArray[i].filename, STR_NULL) == 0)
			break;
	}

	for (j = 0; j < i; ++j)
	{
		if (strcmp (&resumeMusicArray[j].filename, temp.filename) == 0)
			break;
	}

	if (j < PATH_MAX &&
			strcmp (&resumeMusicArray[j].filename, temp.filename) == 0)
	{
		resumeMusicArray[j].position = temp.position;
		resumeMusicArray[j].last_played = temp.last_played;
		print_mp (resumeMusicArray[j]);
	}
	else
	{
		resumeMusicArray[i] = temp;
		print_mp (resumeMusicArray[i]);
	}
}

void
SetMusicPosition (void)
{
	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return;
	if (!optMusicResume)
		return;
	if (!PLRPlaying ((MUSIC_REF)~0))
		return;

	SetGlobalMusicPosition ();
}

DWORD
GetMusicPosition (void)
{
	UNICODE *filename;
	int i;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return 0;
	if (!optMusicResume)
		return 0;

	filename = PLRGetFilename ();

	for (i = 0; i < PATH_MAX; ++i)
	{
		if (strcmp (&resumeMusicArray[i].filename, filename) == 0)
			break;
	}

	return resumeMusicArray[i].position;
}

#define FIVE_MINUTES (1000 * 300)

BOOLEAN
OkayToResume (void)
{
	TimeCount TimeIn, difference;
	UNICODE *filename;
	int i;

	if (GLOBAL (CurrentActivity) & CHECK_ABORT)
		return FALSE;
	if (!optMusicResume)
		return FALSE;

	filename = PLRGetFilename ();

	for (i = 0; i < PATH_MAX; ++i)
	{
		if (strcmp (&resumeMusicArray[i].filename, filename) == 0)
			break;
	}

	if (i == PATH_MAX)
		return FALSE;

	print_mp (resumeMusicArray[i]);

	if (!resumeMusicArray[i].last_played
			|| !resumeMusicArray[i].position)
		return FALSE;

	TimeIn = GetTimeCounter ();
	difference = TimeIn - resumeMusicArray[i].last_played;

	if (difference < FIVE_MINUTES)
		return TRUE;

	return FALSE;
}
