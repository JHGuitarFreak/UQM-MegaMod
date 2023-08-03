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
#include "png2sdl.h"

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
static const char* rendererBackend = NULL;

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
static void TFB_SDL2_Postprocess (bool hd);
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

static int
ReInit_FPS_Screen (SDL_Surface **screen, int w, int h)
{
	if (*screen)
		SDL_FreeSurface (*screen);
	*screen = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32, R_MASK, G_MASK, B_MASK, A_MASK);
	if (*screen == 0)
	{
		log_add(log_Error, "Couldn't create screen buffers: %s",
			SDL_GetError());
	}

	return *screen == 0 ? -1 : 0;
}

static int
FindBestRenderDriver (void)
{
	int i, n;
	if (!rendererBackend) {
		/* If the user has no preference, just let SDL2 choose */
		return -1;
	}
	n = SDL_GetNumRenderDrivers ();
	log_add (log_Info, "Searching for render driver \"%s\".", rendererBackend);

	for (i = 0; i < n; i++) {
		SDL_RendererInfo info;
		if (SDL_GetRenderDriverInfo (i, &info) < 0) {
			continue;
		}
		if (!strcmp(info.name, rendererBackend)) {
			return i;
		}
		log_add (log_Info, "Skipping render driver \"%s\"", info.name);
	}
	/* We did not find any accelerated drivers that weren't D3D9.
	 * Return -1 to ask SDL2 to do its best. */
	log_add (log_Info, "Render driver \"%s\" not available, using system default", rendererBackend);
	return -1;
}

int
TFB_Pure_ConfigureVideo (int driver, int flags, int width, int height,
		int togglefullscreen, int resFactor)
{
	int i;
	char buf[50];

	GraphicsDriver = driver;
	(void) togglefullscreen;

	snprintf (buf, sizeof (buf), "The Ur-Quan Masters v%d.%d.%d %s",
			UQM_MAJOR_VERSION, UQM_MINOR_VERSION, UQM_PATCH_VERSION,
			(resFactor ? "HD " UQM_EXTRA_VERSION : UQM_EXTRA_VERSION));

	if (window == NULL)
	{
		SDL_RendererInfo info;

		window = SDL_CreateWindow ("",
				SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
				width, height, 0);
		if (flags & TFB_GFXFLAGS_FULLSCREEN)
		{
			/* If we create the window fullscreen, it will have
			 * no icon if and when it becomes windowed. */
			SDL_SetWindowFullscreen (window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		}
		if (!window) 
		{
			return -1;
		}
		renderer = SDL_CreateRenderer (window, FindBestRenderDriver (), 0);
		if (!renderer)
		{
			return -1;
		}
		if (SDL_GetRendererInfo (renderer, &info) == 0)
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
		if (flags & TFB_GFXFLAGS_SHOWFPS)
		{
			if (0 != ReInit_FPS_Screen (&SDL_Screen_fps, ScreenWidth, ScreenHeight))
				return -1;
		}
		else
			UnInit_Screen (&SDL_Screen_fps);
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
		int LastScreenWidth, LastScreenHeight;
		SDL_RenderGetLogicalSize (renderer, &LastScreenWidth, &LastScreenHeight);
		if (LastScreenWidth != ScreenWidth || LastScreenHeight != ScreenHeight)
		{
			SDL_RenderSetLogicalSize (renderer, ScreenWidth, ScreenHeight);
			for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
			{
				SDL2_Screens[i].dirty = TRUE;
				if (0 != ReInit_Screen (&SDL_Screens[i], ScreenWidth, ScreenHeight))
				{
					return -1;
				}
			}
			SDL_Screen = SDL_Screens[0];
			TransitionScreen = SDL_Screens[2];
		}
		if (flags & TFB_GFXFLAGS_SHOWFPS)
		{
			if (0 != ReInit_FPS_Screen (&SDL_Screen_fps, ScreenWidth, ScreenHeight))
				return -1;
		}
		else
			UnInit_Screen (&SDL_Screen_fps);
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

	SDL_SetWindowTitle (window, buf);

	if (GfxFlags & TFB_GFXFLAGS_SCALE_ANY)
	{
		/* Linear scaling */
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
	}
	else
	{
		/* Nearest-neighbor scaling */
		SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
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
		if (flags & TFB_GFXFLAGS_SHOWFPS)
		{
			if (0 != ReInit_FPS_Screen (&SDL_Screen_fps, ScreenWidth * 2, ScreenHeight * 2))
				return -1;
		}
		else
			UnInit_Screen (&SDL_Screen_fps);
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

	/* We succeeded, so alter the screen size to our new sizes */
	ScreenWidthActual = width;
	ScreenHeightActual = height;

	(void) resFactor; /* satisfy compiler (unused parameter) */
	return 0;
}

int
TFB_Pure_InitGraphics (int driver, int flags, const char* renderer, 
		int width, int height, int resFactor)
{
	log_add (log_Info, "Initializing SDL.");
	log_add (log_Info, "SDL initialized.");
	log_add (log_Info, "Initializing Screen.");

	ScreenWidth = (320 << resFactor);
	ScreenHeight = (240 << resFactor);
	rendererBackend = renderer;

	if (TFB_Pure_ConfigureVideo (driver, flags, width, height, 0, resFactor))
	{
		log_add (log_Fatal, "Could not initialize video: %s",
				SDL_GetError ());
		exit (EXIT_FAILURE);
	}

	/* Initialize scalers (let them precompute whatever) */
	Scale_Init ();

	return 0;
}

void
TFB_Pure_UninitGraphics (void)
{
	if (renderer) {
		SDL_DestroyRenderer (renderer);
	}
	if (window) {
		SDL_DestroyWindow (window);
	}
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
	char *srcBytes;
	SDL_LockSurface (src);
	srcBytes = src->pixels;
	if (rect)
	{
		/* SDL2 screen surfaces are always 32bpp */
		srcBytes += (src->pitch * rect->y) + (rect->x * 4);
	}
	/* 2020-08-02: At time of writing, the documentation for
	 * SDL_UpdateTexture states this: "If the texture is intended to be
	 * updated often, it is preferred to create the texture as streaming
	 * and use [SDL_LockTexture and SDL_UnlockTexture]." Unfortunately,
	 * SDL_LockTexture will corrupt driver-space memory in the 32-bit
	 * Direct3D 9 driver on Intel Integrated graphics chips, resulting
	 * in an immediate crash with no detectable errors from the API up
	 * to that point.
	 *
	 * We also cannot simply forbid the Direct3D driver outright, because
	 * pre-Windows 10 machines appear to fail to initialize D3D11 even
	 * while claiming to support it.
	 *
	 * These bugs may be fixed in the future, but in the meantime we
	 * rely on this allegedly slower but definitely more reliable
	 * function. */
	SDL_UpdateTexture (dest, rect, srcBytes, src->pitch);
	SDL_UnlockSurface (src);
}

static void
TFB_SDL2_ScanLines (bool hd)
{
	int y;
	SDL_SetRenderDrawColor (renderer, 0, 0, 0, 64);
	SDL_SetRenderDrawBlendMode (renderer, SDL_BLENDMODE_BLEND);
	if (!hd)
	{		
		SDL_RenderSetLogicalSize (renderer, ScreenWidth << 1, ScreenHeight << 1);
		for (y = 0; y < (ScreenHeight << 1); y += 2)
		{
			SDL_RenderDrawLine(renderer, 0, y, (ScreenWidth << 1) - 1, y);
		}
		SDL_RenderSetLogicalSize (renderer, ScreenWidth, ScreenHeight);
	}
	else
	{
		for (y = 0; y < ScreenHeight; y++)
		{
			if (y & 2)
				continue;

			SDL_RenderDrawLine (renderer, 0, y, ScreenWidth - 1, y);
		}
	}	
}

static void
TFB_SDL2_FPS (void)
{
	SDL_Texture *texture;
	SDL_Rect r;
	r.x = r.y = 0;
	r.w = ScreenWidth >> 4;
	r.h = ScreenHeight >> 5;
	texture = SDL_CreateTextureFromSurface (renderer, SDL_Screen_fps);
	SDL_SetTextureBlendMode (texture, SDL_BLENDMODE_BLEND);
	SDL_RenderCopy (renderer, texture, &r, &r);
	SDL_DestroyTexture (texture);
	texture = NULL;
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
	if (r == 0 || r == 255)
	{
		SDL_SetRenderDrawBlendMode (renderer, a == 255 ? SDL_BLENDMODE_NONE
				: SDL_BLENDMODE_BLEND);

		SDL_SetRenderDrawColor (renderer, r, g, b, a);
		SDL_RenderFillRect (renderer, rect);
	}
	else
	{
		if (g == 0)
		{
			SDL_SetRenderDrawBlendMode (renderer,
					SDL_ComposeCustomBlendMode (SDL_BLENDFACTOR_ONE,
						SDL_BLENDFACTOR_ONE, r, SDL_BLENDFACTOR_ONE,
						SDL_BLENDFACTOR_ONE,
						SDL_BLENDOPERATION_MAXIMUM));

			SDL_SetRenderDrawColor (renderer, a, a, a, 255);
			SDL_RenderFillRect (renderer, rect);
		}
		else
		{
			Uint8 c = a > 175 ? 175 : a;
			SDL_SetRenderDrawBlendMode (renderer,
					SDL_ComposeCustomBlendMode (SDL_BLENDFACTOR_ONE,
						SDL_BLENDFACTOR_ONE, SDL_BLENDOPERATION_ADD,
						SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE,
						SDL_BLENDOPERATION_MAXIMUM));
			SDL_SetRenderDrawColor (renderer, c, c, c, 255);
			SDL_RenderFillRect (renderer, rect);

			SDL_SetRenderDrawBlendMode (renderer,
					SDL_ComposeCustomBlendMode (SDL_BLENDFACTOR_ONE,
						SDL_BLENDFACTOR_ONE,
						SDL_BLENDOPERATION_REV_SUBTRACT,
						SDL_BLENDFACTOR_ONE, SDL_BLENDFACTOR_ONE,
						SDL_BLENDOPERATION_MAXIMUM));
			SDL_SetRenderDrawColor (renderer, a, a, a, 255);
			SDL_RenderFillRect (renderer, rect);
		}
	}
}

static void
TFB_SDL2_Postprocess (bool hd)
{
	if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
		TFB_SDL2_ScanLines (hd);

	if (GfxFlags & TFB_GFXFLAGS_SHOWFPS)
		TFB_SDL2_FPS ();

	SDL_RenderPresent (renderer);
}

bool
TFB_SDL2_GammaCorrection (float gamma)
{
	return (SDL_SetWindowBrightness(window, gamma) == 0);
}

BOOLEAN
TFB_SDL_ScreenShot (const char *path)
{
	SDL_Surface *tmp = SDL_GetWindowSurface (window);
	BOOLEAN successful = FALSE;

	if (GfxFlags & TFB_GFXFLAGS_FULLSCREEN)
	{
		float width, height;
		width = (float)tmp->w / 320;
		height = (float)tmp->h / 240;

		if (width > height)
			tmp->w = width * 320;
		else if (height > width)
			tmp->h = width * 240;
	}

	tmp = SDL_CreateRGBSurfaceWithFormat (0, tmp->w, tmp->h, 32,
			SDL_PIXELFORMAT_RGBA32);

	SDL_LockSurface (tmp);
	SDL_RenderReadPixels (renderer, NULL, tmp->format->format,
		tmp->pixels, tmp->pitch);
	if (SDL_SavePNG (tmp, path) == 0)
		successful = TRUE;
	SDL_UnlockSurface (tmp);
	SDL_FreeSurface (tmp);

	return successful;
}

#endif
