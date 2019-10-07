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

#ifndef LIBS_GRAPHICS_SDL_SDLUIO_H_
#define LIBS_GRAPHICS_SDL_SDLUIO_H_

#include "port.h"
#include "libs/uio.h"
#include SDL_INCLUDE(SDL.h)
#include SDL_INCLUDE(SDL_rwops.h)

SDL_Surface *sdluio_loadImage (uio_DirHandle *dir, const char *fileName);
#if SDL_MAJOR_VERSION == 1
int sdluio_seek (SDL_RWops *context, int offset, int whence);
int sdluio_read (SDL_RWops *context, void *ptr, int size, int maxnum);
int sdluio_write (SDL_RWops *context, const void *ptr, int size, int num);
#else
Sint64 sdlui_seek (SDL_RWops *context, Sint64 offset, int whence);
size_t sdlui_read (SDL_RWops *context, void *ptr, size_t size, size_t maxnum);
size_t sdlui_write (SDL_RWops *contex, const void *ptr, size_t size, size_t num);
#endif
int sdluio_close (SDL_RWops *context);


#endif  /* LIBS_GRAPHICS_SDL_SDLUIO_H_ */

