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

#ifndef SDL_COMMON_H
#define SDL_COMMON_H

#include "port.h"
#include SDL_INCLUDE(SDL.h)

#include "../gfxintrn.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/gfx_common.h"

// The Graphics Backend vtable
typedef struct _tfb_graphics_backend {
	void (*preprocess) (int force_redraw, int transition_amount, int fade_amount);
	void (*postprocess) (bool hd);
	void (*uploadTransitionScreen) (void);
	void (*screen) (SCREEN screen, Uint8 alpha, SDL_Rect *rect);
	void (*color) (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect);
} TFB_GRAPHICS_BACKEND;

extern TFB_GRAPHICS_BACKEND *graphics_backend;

extern SDL_Surface *SDL_Screen;
extern SDL_Surface *TransitionScreen;

extern SDL_Surface *SDL_Screens[TFB_GFX_NUMSCREENS];

extern SDL_Surface *format_conv_surf;

#if SDL_MAJOR_VERSION == 1
extern const SDL_VideoInfo *SDL_screen_info;
#endif

SDL_Surface* TFB_DisplayFormatAlpha (SDL_Surface *surface);
int TFB_HasSurfaceAlphaMod (SDL_Surface *surface);
int TFB_GetSurfaceAlphaMod (SDL_Surface *surface, Uint8 *alpha);
int TFB_SetSurfaceAlphaMod (SDL_Surface *surface, Uint8 alpha);
int TFB_DisableSurfaceAlphaMod (SDL_Surface *surface);
int TFB_HasColorKey (SDL_Surface *surface);
int TFB_GetColorKey (SDL_Surface *surface, Uint32 *key);
int TFB_SetColorKey (SDL_Surface *surface, Uint32 key, int rleaccel);
int TFB_DisableColorKey (SDL_Surface *surface);
int TFB_SetColors (SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors);

void TFB_InitOnScreenKeyboard(void);

void UnInit_Screen (SDL_Surface **screen);

BOOLEAN TFB_SDL_ScreenShot (const char *path);

#endif
