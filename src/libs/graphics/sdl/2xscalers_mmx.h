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

#ifndef LIBS_GRAPHICS_SDL_2XSCALERS_MMX_H_
#define LIBS_GRAPHICS_SDL_2XSCALERS_MMX_H_

// MMX versions
void Scale_MMX_PrepPlatform (const SDL_PixelFormat* fmt);

void Scale_MMX_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_MMX_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_MMX_BiAdaptAdvFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_MMX_TriScanFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_MMX_HqFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);

extern const Scale_FuncDef_t Scale_MMX_Functions[];


// SSE (Intel)/MMX Ext (Athlon) versions
void Scale_SSE_PrepPlatform (const SDL_PixelFormat* fmt);

void Scale_SSE_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_SSE_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_SSE_BiAdaptAdvFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_SSE_TriScanFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_SSE_HqFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);

extern const Scale_FuncDef_t Scale_SSE_Functions[];


// 3DNow (AMD K6/Athlon) versions
void Scale_3DNow_PrepPlatform (const SDL_PixelFormat* fmt);

void Scale_3DNow_Nearest (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_3DNow_BilinearFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_3DNow_BiAdaptAdvFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_3DNow_TriScanFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);
void Scale_3DNow_HqFilter (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r);

extern const Scale_FuncDef_t Scale_3DNow_Functions[];


#endif /* LIBS_GRAPHICS_SDL_2XSCALERS_MMX_H_ */
