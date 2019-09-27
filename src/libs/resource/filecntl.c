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

#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "port.h"
#include "resintrn.h"
#include "libs/uio.h"
#include "libs/log.h"

uio_Stream *
res_OpenResFile (uio_DirHandle *dir, const char *filename, const char *mode)
{
	uio_Stream *fp;
	struct stat sb;

	if (uio_stat(dir, filename, &sb) == 0 && S_ISDIR(sb.st_mode)) {
		log_add(log_Debug, "res_OpenResFile('%s', '%s') - cannot open dir as file", filename, mode);
		return ((uio_Stream *)~0);
	}

	fp = uio_fopen (dir, filename, mode);

	return (fp);
}

BOOLEAN
res_CloseResFile (uio_Stream *fp)
{
	if (fp)
	{
		if (fp != (uio_Stream *)~0)
			uio_fclose (fp);
		return (TRUE);
	}

	return (FALSE);
}

BOOLEAN
DeleteResFile (uio_DirHandle *dir, const char *filename)
{
	return (uio_unlink (dir, filename) == 0);
}

size_t
ReadResFile (void *lpBuf, size_t size, size_t count, uio_Stream *fp)
{
	int retval;

	retval = uio_fread (lpBuf, size, count, fp);

	return (retval);
}

size_t
WriteResFile (const void *lpBuf, size_t size, size_t count, uio_Stream *fp)
{
	int retval;

	retval = uio_fwrite (lpBuf, size, count, fp);

	return (retval);
}

int
GetResFileChar (uio_Stream *fp)
{
	int retval;

	retval = uio_getc (fp);

	return (retval);
}

int
PutResFileChar (char ch, uio_Stream *fp)
{
	int retval;

	retval = uio_putc (ch, fp);
	return (retval);
}

int
PutResFileNewline (uio_Stream *fp)
{
	int retval;

#ifdef WIN32
	PutResFileChar ('\r', fp);
#endif
	retval = PutResFileChar ('\n', fp);
	return (retval);
}

long
SeekResFile (uio_Stream *fp, long offset, int whence)
{
	long retval;

	retval = uio_fseek (fp, offset, whence);

	return (retval);
}

long
TellResFile (uio_Stream *fp)
{
	long retval;

	retval = uio_ftell (fp);

	return (retval);
}

size_t
LengthResFile (uio_Stream *fp)
{
	struct stat sb;

	if (fp == (uio_Stream *)~0)
		return (1);
	if (uio_fstat(uio_streamHandle(fp), &sb) == -1)
		return 1;
	return sb.st_size;
}


