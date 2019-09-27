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
#include "scalers.h"
#include "libs/log.h"
#include "../../../uqm/units.h"

static SDL_Surface *fade_color_surface = NULL;
static SDL_Surface *fade_temp = NULL;
static SDL_Surface *scaled_display = NULL;

static TFB_ScaleFunc scaler = NULL;

static Uint32 fade_color;

static void TFB_Pure_Scaled_Preprocess (int force_full_redraw, int transition_amount, int fade_amount);
static void TFB_Pure_Scaled_Postprocess (void);
static void TFB_Pure_Unscaled_Preprocess (int force_full_redraw, int transition_amount, int fade_amount);
static void TFB_Pure_Unscaled_Postprocess (void);
static void TFB_Pure_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect);
static void TFB_Pure_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect);

static TFB_GRAPHICS_BACKEND pure_scaled_backend = {
	TFB_Pure_Scaled_Preprocess,
	TFB_Pure_Scaled_Postprocess,
	TFB_Pure_ScreenLayer,
	TFB_Pure_ColorLayer };

static TFB_GRAPHICS_BACKEND pure_unscaled_backend = {
	TFB_Pure_Unscaled_Preprocess,
	TFB_Pure_Unscaled_Postprocess,
	TFB_Pure_ScreenLayer,
	TFB_Pure_ColorLayer };

// We cannot rely on SDL_DisplayFormatAlpha() anymore. It can return
// formats that we do not expect (SDL v1.2.14 on Mac OSX). Mac likes
// ARGB surfaces, but SDL_DisplayFormatAlpha thinks that only RGBA are fast.
// This is a generic replacement that gives what we want.
static void
CalcAlphaFormat (const SDL_PixelFormat* video, SDL_PixelFormat* ours)
{
	int valid = 0;

	// We use 32-bit surfaces internally
	ours->BitsPerPixel = 32;

	// Try to get as close to the video format as possible
	if (video->BitsPerPixel == 15 || video->BitsPerPixel == 16)
	{	// At least match the channel order
		ours->Rshift = video->Rshift / 5 * 8;
		ours->Gshift = video->Gshift / 5 * 8;
		ours->Bshift = video->Bshift / 5 * 8;
		valid = 1;
	}
	else if (video->BitsPerPixel == 24 || video->BitsPerPixel == 32)
	{
		// We can only use channels aligned on byte boundary
		if (video->Rshift % 8 == 0 && video->Gshift % 8 == 0
				&& video->Bshift % 8 == 0)
		{	// Match RGB in video
			ours->Rshift = video->Rshift;
			ours->Gshift = video->Gshift;
			ours->Bshift = video->Bshift;
			valid = 1;
		}
	}

	if (valid)
	{	// For alpha, use the unoccupied byte
		ours->Ashift = 48 - (ours->Rshift + ours->Gshift + ours->Bshift);
		// Set channels according to byte positions
		ours->Rmask = 0xff << ours->Rshift;
		ours->Gmask = 0xff << ours->Gshift;
		ours->Bmask = 0xff << ours->Bshift;
		ours->Amask = 0xff << ours->Ashift;
		return;
	}

	// Fallback case. It does not matter what we set, but SDL likes
	// Alpha to be the highest.
	ours->Rmask = 0x000000ff;
	ours->Gmask = 0x0000ff00;
	ours->Bmask = 0x00ff0000;
	ours->Amask = 0xff000000;
}

int
TFB_Pure_ConfigureVideo(int driver, int flags, int width, int height, int togglefullscreen, unsigned int resFactor)
{
	int i, videomode_flags;
	SDL_PixelFormat conv_fmt;
	int BPP = 32;

	GraphicsDriver = driver;

	// must use SDL_SWSURFACE, HWSURFACE doesn't work properly
	// with fades/scaling
	if (width == 320 && height == 240)
	{
		videomode_flags = SDL_SWSURFACE;
		ScreenWidthActual = 320;
		ScreenHeightActual = 240;
		graphics_backend = &pure_unscaled_backend;
	}
	else
	{
		videomode_flags = SDL_SWSURFACE;

		// JMS_GFX: Resolution is calculated with the help of a Resolution factor.
		if (resFactor != HD)
		{
			// Check the sanity of resolution.
			if (width != 640 || height != 480)
			{
				log_add(log_Error, "Screen resolution of %dx%d not supported "
					"under pure SDL, using 640x480", width, height);

				width = 640;
				height = 480;
			}

			ScreenWidthActual = width;
			ScreenHeightActual = height;
			graphics_backend = &pure_scaled_backend;
		}
		else
		{
			ScreenWidthActual = (320 << resFactor);
			ScreenHeightActual = (240 << resFactor);
			graphics_backend = &pure_unscaled_backend;
		}
	}

#if defined(ANDROID) || defined(__ANDROID__)
	videomode_flags = SDL_SWSURFACE;
	//ScreenWidthActual = 1280;
	//ScreenHeightActual = 960;
	graphics_backend = &pure_unscaled_backend;
	BPP = 24;
#endif

	videomode_flags |= SDL_ANYFORMAT;
	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		videomode_flags |= SDL_FULLSCREEN;

	/* We'll ask for a 32bpp frame, but it doesn't really matter, because we've set
	   SDL_ANYFORMAT */
	SDL_Video = SDL_SetVideoMode (ScreenWidthActual, ScreenHeightActual, 
		BPP, videomode_flags);

	if (SDL_Video == NULL)
	{
		log_add (log_Error, "Couldn't set %ix%i video mode: %s",
			ScreenWidthActual, ScreenHeightActual,
			SDL_GetError ());
		return -1;
	}
	else
	{
		const SDL_Surface *video = SDL_GetVideoSurface ();
		const SDL_PixelFormat* fmt = video->format;

		ScreenColorDepth = fmt->BitsPerPixel;
		log_add (log_Info, "Set the resolution to: %ix%ix%i",
				video->w, video->h, ScreenColorDepth);
		log_add (log_Info, "  Video: R %08x, G %08x, B %08x, A %08x",
				fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
		
		if (togglefullscreen)
		{
			// NOTE: We cannot change the format_conv_surf now because we
			//   have already loaded lots of graphics and changing it now
			//   will only lead to chaos.
			// Just check if channel order has changed significantly
			CalcAlphaFormat (fmt, &conv_fmt);
			fmt = format_conv_surf->format;
			if (conv_fmt.Rmask != fmt->Rmask || conv_fmt.Bmask != fmt->Bmask)
				log_add (log_Warning, "Warning: pixel format has changed "
						"significantly. Rendering will be slow.");
			return 0;
		}
	}

	// Create a 32bpp surface in a compatible format which will supply
	// the format information to all other surfaces used in the game
	if (format_conv_surf)
	{
		SDL_FreeSurface (format_conv_surf);
		format_conv_surf = NULL;
	}
	CalcAlphaFormat (SDL_Video->format, &conv_fmt);
	format_conv_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, 0, 0,
			conv_fmt.BitsPerPixel, conv_fmt.Rmask, conv_fmt.Gmask,
			conv_fmt.Bmask, conv_fmt.Amask);
	if (!format_conv_surf)
	{
		log_add (log_Error, "Couldn't create format_conv_surf: %s",
				SDL_GetError());
		return -1;
	}
	else
	{
		const SDL_PixelFormat* fmt = format_conv_surf->format;
		log_add (log_Info, "  Internal: R %08x, G %08x, B %08x, A %08x",
				fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
	}
	
	for (i = 0; i < TFB_GFX_NUMSCREENS; i++)
	{
		if (0 != ReInit_Screen (&SDL_Screens[i], format_conv_surf,
				ScreenWidth, ScreenHeight))
			return -1;
	}

	SDL_Screen = SDL_Screens[0];
	TransitionScreen = SDL_Screens[2];

	if (0 != ReInit_Screen (&fade_color_surface, format_conv_surf,
			ScreenWidth, ScreenHeight))
		return -1;
	fade_color = SDL_MapRGB (fade_color_surface->format, 0, 0, 0);
	SDL_FillRect (fade_color_surface, NULL, fade_color);
	
	if (0 != ReInit_Screen (&fade_temp, format_conv_surf,
			ScreenWidth, ScreenHeight))
		return -1;

	if (ScreenWidthActual > ScreenWidth || ScreenHeightActual > ScreenHeight)
	{
		if (0 != ReInit_Screen (&scaled_display, format_conv_surf,
				ScreenWidthActual, ScreenHeightActual))
			return -1;

		scaler = Scale_PrepPlatform (flags, SDL_Screen->format);
	}
	else
	{	// no need to scale
		scaler = NULL;
	}

	return 0;
}

int
TFB_Pure_InitGraphics (int driver, int flags, int width, int height, unsigned int resFactor)
{
	char VideoName[256];

	log_add (log_Info, "Initializing Pure-SDL graphics.");

	SDL_VideoDriverName (VideoName, sizeof (VideoName));
	log_add (log_Info, "SDL driver used: %s", VideoName);
			// Set the environment variable SDL_VIDEODRIVER to override
			// For Linux: x11 (default), dga, fbcon, directfb, svgalib,
			//            ggi, aalib
			// For Windows: directx (default), windib

	log_add (log_Info, "SDL initialized.");
	log_add (log_Info, "Initializing Screen.");

	ScreenWidth = (320 << resFactor); // 320
	ScreenHeight = (240 << resFactor); // 240

	if (TFB_Pure_ConfigureVideo (driver, flags, width, height, 0, resFactor))
	{
		log_add (log_Fatal, "Could not initialize video: "
				"no fallback at start of program!");
		exit (EXIT_FAILURE);
	}

	// Initialize scalers (let them precompute whatever)
	Scale_Init ();

	return 0;
}

void
TFB_Pure_UninitGraphics (void)
{
	UnInit_Screen (&scaled_display);
	UnInit_Screen (&fade_color_surface);
	UnInit_Screen (&fade_temp);
}

static void
ScanLines (SDL_Surface *dst, SDL_Rect *r)
{
	const int rw = r->w * 2;
	const int rh = r->h * 2;
	SDL_PixelFormat *fmt = dst->format;
	const int pitch = dst->pitch;
	const int len = pitch / fmt->BytesPerPixel;
	int ddst;
	Uint32 *p = (Uint32 *) dst->pixels;
	int x, y;

	p += len * (r->y * 2) + (r->x * 2);
	ddst = len + len - rw;

	for (y = rh; y; y -= 2, p += ddst)
	{
		for (x = rw; x; --x, ++p)
		{
			// we ignore the lower bits as the difference
			// of 1 in 255 is negligible
			*p = ((*p >> 1) & 0x7f7f7f7f) + ((*p >> 2) & 0x3f3f3f3f);
		}
	}
}

static SDL_Surface *backbuffer = NULL, *scalebuffer = NULL;
static SDL_Rect updated;

static void
TFB_Pure_Scaled_Preprocess (int force_full_redraw, int transition_amount, int fade_amount)
{
	if (force_full_redraw != TFB_REDRAW_NO)
	{
		updated.x = updated.y = 0;
		updated.w = ScreenWidth;
		updated.h = ScreenHeight;	
	}
	else
	{
		updated.x = TFB_BBox.region.corner.x;
		updated.y = TFB_BBox.region.corner.y;
		updated.w = TFB_BBox.region.extent.width;
		updated.h = TFB_BBox.region.extent.height;
	}

	if (transition_amount == 255 && fade_amount == 255)
		backbuffer = SDL_Screens[TFB_SCREEN_MAIN];
	else
		backbuffer = fade_temp;

	// we can scale directly onto SDL_Video if video is compatible
	if (SDL_Video->format->BitsPerPixel == SDL_Screen->format->BitsPerPixel
			&& SDL_Video->format->Rmask == SDL_Screen->format->Rmask
			&& SDL_Video->format->Bmask == SDL_Screen->format->Bmask)
		scalebuffer = SDL_Video;
	else
		scalebuffer = scaled_display;

}

static void
TFB_Pure_Unscaled_Preprocess (int force_full_redraw, int transition_amount, int fade_amount)
{
	if (force_full_redraw != TFB_REDRAW_NO)
	{
		updated.x = updated.y = 0;
		updated.w = ScreenWidth;
		updated.h = ScreenHeight;	
	}
	else
	{
		updated.x = TFB_BBox.region.corner.x;
		updated.y = TFB_BBox.region.corner.y;
		updated.w = TFB_BBox.region.extent.width;
		updated.h = TFB_BBox.region.extent.height;
	}

	backbuffer = SDL_Video;
	(void)transition_amount;
	(void)fade_amount;
}

static void
TFB_Pure_Scaled_Postprocess (void)
{
	SDL_LockSurface (scalebuffer);
	SDL_LockSurface (backbuffer);

	if (scaler)
		scaler (backbuffer, scalebuffer, &updated);

	if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
		ScanLines (scalebuffer, &updated);
		
	SDL_UnlockSurface (backbuffer);
	SDL_UnlockSurface (scalebuffer);

	updated.x *= 2;
	updated.y *= 2;
	updated.w *= 2;
	updated.h *= 2;
	if (scalebuffer != SDL_Video)
		SDL_BlitSurface (scalebuffer, &updated, SDL_Video, &updated);

	SDL_UpdateRects (SDL_Video, 1, &updated);
}

static void
TFB_Pure_Unscaled_Postprocess (void)
{
	SDL_UpdateRect (SDL_Video, updated.x, updated.y,
			updated.w, updated.h);
}

static void
TFB_Pure_ScreenLayer (SCREEN screen, Uint8 a, SDL_Rect *rect)
{
	if (SDL_Screens[screen] == backbuffer)
		return;
	SDL_SetAlpha (SDL_Screens[screen], SDL_SRCALPHA, a);
	SDL_BlitSurface (SDL_Screens[screen], rect, backbuffer, rect);
}	

static void
TFB_Pure_ColorLayer (Uint8 r, Uint8 g, Uint8 b, Uint8 a, SDL_Rect *rect)
{
	Uint32 col = SDL_MapRGB (fade_color_surface->format, r, g, b);
	if (col != fade_color)
	{
		fade_color = col;
		SDL_FillRect (fade_color_surface, NULL, fade_color);
	}
	SDL_SetAlpha (fade_color_surface, SDL_SRCALPHA, a);
	SDL_BlitSurface (fade_color_surface, rect, backbuffer, rect);
}

void
Scale_PerfTest (void)
{
	TimeCount TimeStart, TimeIn;
	TimeCount Now = 0;
	SDL_Rect updated = {0, 0, ScreenWidth, ScreenHeight};
	int i;

	if (!scaler)
	{
		log_add (log_Error, "No scaler configured! "
				"Run with larger resolution, please");
		return;
	}
	if (!scaled_display)
	{
		log_add (log_Error, "Run scaler performance tests "
				"in Pure mode, please");
		return;
	}

	SDL_LockSurface (SDL_Screen);
	SDL_LockSurface (scaled_display);

	TimeStart = TimeIn = SDL_GetTicks ();

	for (i = 1; i < 1001; ++i) // run for 1000 frames
	{
		scaler (SDL_Screen, scaled_display, &updated);
		
		if (GfxFlags & TFB_GFXFLAGS_SCANLINES)
			ScanLines (scaled_display, &updated);

		if (i % 100 == 0)
		{
			Now = SDL_GetTicks ();
			log_add (log_Debug, "%03d(%04u) ", 100*1000 / (Now - TimeIn),
					Now - TimeIn);
			TimeIn = Now;
		}
	}

	log_add (log_Debug, "Full frames scaled: %d; over %u ms; %d fps\n",
			(i - 1), Now - TimeStart, i * 1000 / (Now - TimeStart));

	SDL_UnlockSurface (scaled_display);
	SDL_UnlockSurface (SDL_Screen);
}

