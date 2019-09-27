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
#include "options.h"
#include "libs/reslib.h"
#include <string.h>


SOUND_REF
LoadSoundFile (const char *pStr)
{
	uio_Stream *fp;

	// FIXME: this theoretically needs a mechanism to prevent races
	if (_cur_resfile_name)
		// something else is loading resources atm
		return 0;

	fp = res_OpenResFile (contentDir, pStr, "rb");
	if (fp)
	{
		SOUND_REF hData;

		_cur_resfile_name = pStr;
		hData = (SOUND_REF)_GetSoundBankData (fp, LengthResFile (fp));
		_cur_resfile_name = 0;

		res_CloseResFile (fp);

		return hData;
	}

	return NULL;
}

MUSIC_REF
LoadMusicFile (const char *pStr)
{
	uio_Stream *fp;
	char filename[256];

	// FIXME: this theoretically needs a mechanism to prevent races
	if (_cur_resfile_name)
		// something else is loading resources atm
		return 0;

	strncpy (filename, pStr, sizeof(filename) - 1);
	filename[sizeof(filename) - 1] = '\0';
	CheckMusicResName (filename);

	// Opening the res file is not technically necessary right now
	// since _GetMusicData() completely ignores the arguments
	// But just for the sake of correctness
	fp = res_OpenResFile (contentDir, filename, "rb");
	if (fp)
	{
		MUSIC_REF hData;

		_cur_resfile_name = filename;
		hData = (MUSIC_REF)_GetMusicData (fp, LengthResFile (fp));
		_cur_resfile_name = 0;

		res_CloseResFile (fp);

		return hData;
	}

	return (0);
}

