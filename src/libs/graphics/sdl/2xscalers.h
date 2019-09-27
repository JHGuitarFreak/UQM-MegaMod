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

#ifndef LIBS_GRAPHICS_SDL_2XSCALERS_H_
#define LIBS_GRAPHICS_SDL_2XSCALERS_H_

void Scale_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_BiAdaptFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_BiAdaptAdvFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_TriScanFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_HqFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);

extern const Scale_FuncDef_t Scale_C_Functions[];


#endif /* LIBS_GRAPHICS_SDL_2XSCALERS_H_ */
