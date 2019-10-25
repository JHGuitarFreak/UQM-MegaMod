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

#include "sdluio.h"

#include "port.h"
#include "libs/uio.h"
#include SDL_INCLUDE(SDL.h)
#include SDL_INCLUDE(SDL_error.h)
#include SDL_INCLUDE(SDL_rwops.h)
#include "libs/memlib.h"
#include "png2sdl.h"
#include <errno.h>
#include <string.h>


static SDL_RWops *sdluio_makeRWops (uio_Stream *stream);

#if 0
// For use for initialisation, using structure assignment.
static SDL_RWops sdluio_templateRWops =
{
	.seek = sdluio_seek,
	.read = sdluio_read,
	.write = sdluio_write,
	.close = sdluio_close,
};
#endif

SDL_Surface *
sdluio_loadImage (uio_DirHandle *dir, const char *fileName) {
	uio_Stream *stream;
	SDL_RWops *rwops;
	SDL_Surface *result = NULL;

	stream = uio_fopen (dir, fileName, "rb");
	if (stream == NULL)
	{
		SDL_SetError ("Couldn't open '%s': %s", fileName,
				strerror(errno));
		return NULL;
	}
	rwops = sdluio_makeRWops (stream);
	if (rwops) {
		result = TFB_png_to_sdl (rwops);
		SDL_RWclose (rwops);
	}
	return result;
}
	
#if SDL_MAJOR_VERSION == 1
int
sdluio_seek (SDL_RWops *context, int offset, int whence)
#else
Sint64
sdluio_seek (SDL_RWops *context, Sint64 offset, int whence)
#endif
{
	if (uio_fseek ((uio_Stream *) context->hidden.unknown.data1, offset,
				whence) == -1)
	{
		SDL_SetError ("Error seeking in uio_Stream: %s",
				strerror(errno));
		return -1;
	}
	return uio_ftell ((uio_Stream *) context->hidden.unknown.data1);
}

#if SDL_MAJOR_VERSION == 1
int
sdluio_read (SDL_RWops *context, void *ptr, int size, int maxnum)
#else
size_t
sdluio_read (SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
#endif
{
	size_t numRead;
	
	numRead = uio_fread (ptr, (size_t) size, (size_t) maxnum,
			(uio_Stream *) context->hidden.unknown.data1);
	if (numRead == 0 && uio_ferror ((uio_Stream *)
				context->hidden.unknown.data1))
	{
		SDL_SetError ("Error reading from uio_Stream: %s",
				strerror(errno));
		return 0;
	}
	return (int) numRead;
}

#if SDL_MAJOR_VERSION == 1
int
sdluio_write (SDL_RWops *context, const void *ptr, int size, int num)
#else
size_t
sdluio_write (SDL_RWops *context, const void *ptr, size_t size, size_t num)
#endif
{
	size_t numWritten;

	numWritten = uio_fwrite (ptr, (size_t) size, (size_t) num,
			(uio_Stream *) context->hidden.unknown.data1);
	if (numWritten == 0 && uio_ferror ((uio_Stream *)
				context->hidden.unknown.data1))
	{
		SDL_SetError ("Error writing to uio_Stream: %s",
				strerror(errno));
		return 0;
	}
	return (size_t) numWritten;
}

int
sdluio_close (SDL_RWops *context) {
	int result;
	
	result = uio_fclose ((uio_Stream *) context->hidden.unknown.data1);
	HFree (context);
	return result;
}

static SDL_RWops *
sdluio_makeRWops (uio_Stream *stream) {
	SDL_RWops *result;
	
	result = HMalloc (sizeof (SDL_RWops));
#if 0
	*(struct SDL_RWops *) result = sdluio_templateRWops;
			// structure assignment
#endif
	result->seek = sdluio_seek;
	result->read = sdluio_read;
	result->write = sdluio_write;
	result->close = sdluio_close;
	result->hidden.unknown.data1 = stream;
	return result;
}



