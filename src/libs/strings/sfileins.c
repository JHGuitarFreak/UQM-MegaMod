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

#include "port.h"
#include "strintrn.h"
#include "libs/uio.h"
#include "libs/reslib.h"


STRING_TABLE
LoadStringTableFile (uio_DirHandle *dir, const char *fileName)
{
	uio_Stream *fp;

	// FIXME: this theoretically needs a mechanism to prevent races
	if (_cur_resfile_name)
		// something else is loading resources atm
		return 0;

	fp = res_OpenResFile (dir, fileName, "rb");
	if (fp)
	{
		STRING_TABLE data;

		_cur_resfile_name = fileName;
		data = (STRING_TABLE) _GetStringData (fp, LengthResFile (fp));
		_cur_resfile_name = 0;
		res_CloseResFile (fp);

		return data;
	}

	return (0);
}

