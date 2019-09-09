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

#include "pure.h"
#include "libs/graphics/bbox.h"
#include "libs/log.h"
#include "scalers.h"
#include "uqmversion.h"

#if SDL_MAJOR_VERSION > 1

typedef struct tfb_sdl2_screeninfo_s {
	SDL_Surface *scaled;
	SDL_Texture *texture;
	BOOLEAN dirty, active;
	SDL_Rect updated;
} TFB_SDL2_SCREENINFO;

static TFB_SDL2_SCREENINFO SDL2_Screens[TFB_GFX_NUMSCREENS];

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static int ScreenFilterMode;

static TFB_ScaleFunc scaler = NULL;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define A_MASK 0xff000000
#define B_MASK 0x00ff0000
#define G_MASK 0x0000ff00
#define R_MASK 0x000000ff
#else
#define A_MASK 0x000000ff
#define B_MASK 0x0000ff00
#define G_MASK 0x00ff0000
#define R_MASK 0xff000000
#endif

static void TFB_SDL2_Preprocess (int force_full_redraw, int transition_amount, int fade_amount);
static void TFB_SDL2_Postprocess (void);
static void TFB_SDL2_UploadTransitionScreen (void);
static void TFB_SDL2_Scaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect);
static void TFB_SDL2_Unscaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect);
static void TFB_SDL2_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect);

static TFB_GRAPHICS_BACKEND sdl2_scaled_backend = {
        TFB_SDL2_Preprocess,
        TFB_SDL2_Postprocess,
	TFB_SDL2_UploadTransitionScreen,
        TFB_SDL2_Scaled_ScreenLayer,
        TFB_SDL2_ColorLayer };

static TFB_GRAPHICS_BACKEND sdl2_unscaled_backend = {
        TFB_SDL2_Preprocess,
        TFB_SDL2_Postprocess,
	TFB_SDL2_UploadTransitionScreen,
        TFB_SDL2_Unscaled_ScreenLayer,
        TFB_SDL2_ColorLayer };

static SDL_Surface *
Create_Screen (int w, int h)
{
        SDL_Surface *newsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h,
                        32, R_MASK, G_MASK, B_MASK, 0);
        if (newsurf == 0)
	{
                log_add (log_Error, "Couldn't create screen buffers: %s",
                                SDL_GetError());
        }
        return newsurf;
}

static int
ReInit_Screen (SDL_Surface **screen, int w, int h)
{
        if (*screen)
                SDL_FreeSurface (*screen);
        *screen = Create_Screen (w, h);
        
        return *screen == 0 ? -1 : 0;
}

int
TFB_Pure_ConfigureVideo (int driver, int flags, int width, int height, int togglefullscreen)
{
	int i;
	GraphicsDriver = driver;
	(void) togglefullscreen;
	if (window == NULL)
	{
		SDL_RendererInfo info;
		char caption[200];
		int flags2 = 0;

		if (flags & TFB_GFXFLAGS_FULLSCREEN)
		{
			flags2 = SDL_WINDOW_FULLSCREEN_DESKTOP;
		}
		sprintf (caption, "The Ur-Quan Masters v%d.%d.%d%s",
				UQM_MAJOR_VERSION, UQM_MINOR_VERSION,
				UQM_PATCH_VERSION, UQM_EXTRA_VERSION);
		window = SDL_CreateWindow (caption,
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				width, height, flags2);
		if (!window) 
		{
			return -1;
		}
		renderer = SDL_CreateRenderer (window, -1, 0);
		if (!renderer)
		{
			return -1;
		}
		if (SDL_GetRendererInfo (renderer, &info))
		{
			log_add (log_Info, "SDL2 renderer '%s' selected.\n", info.name);
		}
		else
		{
			log_add (log_Info, "SDL2 renderer had no name.");
		}
		SDL_RenderSetLogicalSize (renderer, ScreenWidth, ScreenHeight);
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			SDL2_Screens[i].scaled = NULL;
			SDL2_Screens[i].texture = NULL;
			SDL2_Screens[i].dirty = TRUE;
			SDL2_Screens[i].active = TRUE;
			if (0 != ReInit_Screen (&SDL_Screens[i], ScreenWidth, ScreenHeight))
			{
				return -1;
			}
		}
		SDL2_Screens[1].active = FALSE;
		SDL_Screen = SDL_Screens[0];
		TransitionScreen = SDL_Screens[2];
		format_conv_surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 0, 0,
				32, R_MASK, G_MASK, B_MASK, A_MASK);
		if (!format_conv_surf)
		{
			return -1;
		}
	}
	else
	{
		if (flags & TFB_GFXFLAGS_FULLSCREEN)
		{
			SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		else
		{
			SDL_SetWindowFullscreen (window, 0);
			SDL_SetWindowSize (window, width, height);
		}
	}

	if (GfxFlags & TFB_GFXFLAGS_SCALE_SOFT_ONLY)
	{
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			if (!SDL2_Screens[i].active)
			{
				continue;
			}
			if (0 != ReInit_Screen(&SDL2_Screens[i].scaled,
					ScreenWidth * 2, ScreenHeight * 2))
			{
				return -1;
			}
			if (SDL2_Screens[i].texture)
			{
				SDL_DestroyTexture (SDL2_Screens[i].texture);
				SDL2_Screens[i].texture = NULL;
			}
			SDL2_Screens[i].texture = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, ScreenWidth * 2, ScreenHeight * 2);
			SDL_LockSurface (SDL2_Screens[i].scaled);
			SDL_UpdateTexture (SDL2_Screens[i].texture, NULL, SDL2_Screens[i].scaled->pixels, SDL2_Screens[i].scaled->pitch);
			SDL_UnlockSurface (SDL2_Screens[i].scaled);
		}
		scaler = Scale_PrepPlatform (flags, SDL2_Screens[0].scaled->format);
		graphics_backend = &sdl2_scaled_backend;
	}
	else
	{
		for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
		{
			if (SDL2_Screens[i].scaled)
			{
				SDL_FreeSurface (SDL2_Screens[i].scaled);
				SDL2_Screens[i].scaled = NULL;
			}
			if (SDL2_Screens[i].texture)
			{
				SDL_DestroyTexture (SDL2_Screens[i].texture);
				SDL2_Screens[i].texture = NULL;
			}
			SDL2_Screens[i].texture = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING, ScreenWidth, ScreenHeight);
			SDL_LockSurface (SDL_Screens[i]);
			SDL_UpdateTexture (SDL2_Screens[i].texture, NULL, SDL_Screens[i]->pixels, SDL_Screens[i]->pitch);
			SDL_UnlockSurface (SDL_Screens[i]);
		}
		scaler = NULL;
		graphics_backend = &sdl2_unscaled_backend;
	}

	/* TODO: Set bilinear vs nearest-neighbor filtering based on
	 *       GfxFlags & TFB_GFXFLAGS_SCALE_ANY */
	return 0;
}

int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height)
{
	log_add (log_Info, "Initializing SDL.");
	log_add (log_Info, "SDL initialized.");
	log_add (log_Info, "Initializing Screen.");

	ScreenWidth = 320;
	ScreenHeight = 240;

	if (TFB_Pure_ConfigureVideo (driver, flags, width, height, 0))
	{
		log_add (log_Fatal, "Could not initialize video: %s",
				SDL_GetError ());
		exit (EXIT_FAILURE);
	}

	/* Initialize scalers (let them precompute whatever) */
	Scale_Init ();

	return 0;
}

static void
TFB_SDL2_UploadTransitionScreen (void)
{
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.x = 0;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.y = 0;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.w = ScreenWidth;
	SDL2_Screens[TFB_SCREEN_TRANSITION].updated.h = ScreenHeight;
	SDL2_Screens[TFB_SCREEN_TRANSITION].dirty = TRUE;
}

static void
TFB_SDL2_UpdateTexture (SDL_Texture *dest, SDL_Surface *src, SDL_Rect *rect)
{
	void *pixels = NULL;
	int pitch = 0;
	SDL_LockSurface (src);
	if (!SDL_LockTexture (dest, rect, &pixels, &pitch))
	{
		char *srcBytes, *destBytes;
		int row, rowsize;
		SDL_Rect backup, *r = rect;
		if (!r)
		{
			/* No rectangle specified, use entire surface */
			backup.x = backup.y = 0;
			backup.w = src->w;
			backup.h = src->h;
			r = &backup;
		}
		/* SDL2 screen surfaces are always 32bpp */
		srcBytes = src->pixels + (src->pitch * r->y) + (r->x * 4);
		destBytes = pixels;
		rowsize = r->w * 4;
		for (row = 0; row < r->h; ++row)
		{
			memcpy (destBytes, srcBytes, rowsize);
			destBytes += pitch;
			srcBytes += src->pitch;
		}
		SDL_UnlockTexture (dest);
	}
	SDL_UnlockSurface (src);
}

static void
TFB_SDL2_ScanLines (void)
{
	/* TODO */
}

static void
TFB_SDL2_Preprocess (int force_full_redraw, int transition_amount, int fade_amount)
{
	(void) transition_amount;
	(void) fade_amount;

	if (force_full_redraw == TFB_REDRAW_YES)
	{
		SDL2_Screens[TFB_SCREEN_MAIN].updated.x = 0;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.y = 0;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.w = ScreenWidth;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.h = ScreenHeight;
		SDL2_Screens[TFB_SCREEN_MAIN].dirty = TRUE;
	}
	else if (TFB_BBox.valid)
	{
		SDL2_Screens[TFB_SCREEN_MAIN].updated.x = TFB_BBox.region.corner.x;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.y = TFB_BBox.region.corner.y;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.w = TFB_BBox.region.extent.width;
		SDL2_Screens[TFB_SCREEN_MAIN].updated.h = TFB_BBox.region.extent.height;
		SDL2_Screens[TFB_SCREEN_MAIN].dirty = TRUE;
	}

	SDL_SetRenderDrawBlendMode (renderer, SDL_BLENDMODE_NONE);
	SDL_SetRenderDrawColor (renderer, 0, 0, 0, 255);
	SDL_RenderClear (renderer);
}

static void
TFB_SDL2_Unscaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect)
{
	SDL_Texture *texture = SDL2_Screens[screen].texture;
	if (SDL2_Screens[screen].dirty)
	{
		TFB_SDL2_UpdateTexture (texture, SDL_Screens[screen], &SDL2_Screens[screen].updated);
	}
	if (a == 255)
	{
		SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_NONE);
	}
	else
	{
		SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod (texture, a);
	}
	SDL_RenderCopy (renderer, texture, rect, rect);
}

static void
TFB_SDL2_Scaled_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect)
{
	SDL_Texture *texture = SDL2_Screens[screen].texture;
	SDL_Rect srcRect, *pSrcRect = NULL;
	if (SDL2_Screens[screen].dirty)
	{
		SDL_Surface *src = SDL2_Screens[screen].scaled;
		SDL_Rect scaled_update = SDL2_Screens[screen].updated;
		scaler (SDL_Screens[screen], src, &SDL2_Screens[screen].updated);
		scaled_update.x *= 2;
		scaled_update.y *= 2;
		scaled_update.w *= 2;
		scaled_update.h *= 2;
		TFB_SDL2_UpdateTexture (texture, src, &scaled_update);
	}
	if (a == 255)
	{
		SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_NONE);
	}
	else
	{
		SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_BLEND);
		SDL_SetTextureAlphaMod (texture, a);
	}
	/* The texture has twice the resolution when scaled, but the
	 * screen's logical resolution has not changed, so the clip
	 * rectangle does not need to be scaled. The *source* clip
	 * rect, however, must be scaled to match. */
	if (rect)
	{
		srcRect = *rect;
		srcRect.x *= 2;
		srcRect.y *= 2;
		srcRect.w *= 2;
		srcRect.h *= 2;
		pSrcRect = &srcRect;
	}
	SDL_RenderCopy (renderer, texture, pSrcRect, rect);
}

static void
TFB_SDL2_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect)
{
	SDL_SetRenderDrawBlendMode (renderer, a == 255 ? SDL_BLENDMODE_NONE 
			: SDL_BLENDMODE_BLEND);
	SDL_SetRenderDrawColor (renderer, r, g, b, a);
	SDL_RenderFillRect (renderer, rect);
}

static void
TFB_SDL2_Postprocess (void)
{
	if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
		TFB_SDL2_ScanLines ();

	SDL_RenderPresent (renderer);
}

#endif
