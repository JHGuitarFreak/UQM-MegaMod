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

#include "sdl_common.h"
#include "opengl.h"
#include "pure.h"
#include "primitives.h"
#include "options.h"
#include "uqmversion.h"
#include "libs/graphics/drawcmd.h"
#include "libs/graphics/dcqueue.h"
#include "libs/graphics/cmap.h"
#include "libs/input/sdl/input.h"
		// for ProcessInputEvent()
#include "libs/graphics/bbox.h"
#include "port.h"
#include "libs/uio.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "libs/vidlib.h"

#if SDL_MAJOR_VERSION > 1

void
TFB_PreInit (void)
{
	SDL_version compiled, linked;
	SDL_VERSION(&compiled);
	SDL_GetVersion(&linked);
	log_add (log_Info, "Initializing base SDL functionality.");
	log_add (log_Info, "Using SDL version %d.%d.%d (compiled with "
			"%d.%d.%d)", linked.major, linked.minor, linked.patch,
			compiled.major, compiled.minor, compiled.patch);
#if 0
	if (compiled.major != linked.major || compiled.minor != linked.minor ||
			compiled.patch != linked.patch)
	{
		log_add (log_Warning, "The used SDL library is not the same version "
				"as the one used to compile The Ur-Quan Masters with! "
				"If you experience any crashes, this would be an excellent "
				"suspect.");
	}
#endif

	if ((SDL_Init (SDL_INIT_VIDEO) == -1))
	{
		log_add (log_Fatal, "Could not initialize SDL: %s.", SDL_GetError ());
		exit (EXIT_FAILURE);
	}
}

int
TFB_ReInitGraphics (int driver, int flags, int width, int height)
{
	int result;
	int togglefullscreen = 0;

	if (GfxFlags == (flags ^ TFB_GFXFLAGS_FULLSCREEN) &&
			driver == GraphicsDriver &&
			width == ScreenWidthActual && height == ScreenHeightActual)
	{
		togglefullscreen = 1;
	}

	GfxFlags = flags;

	result = TFB_Pure_ConfigureVideo (TFB_GFXDRIVER_SDL_PURE, flags, 
			width, height, togglefullscreen);

	if (flags & TFB_GFXFLAGS_FULLSCREEN)
		SDL_ShowCursor (SDL_DISABLE);
	else
		SDL_ShowCursor (SDL_ENABLE);

	return result;
}

/* Probably ought to clean this away at some point. */
SDL_Surface *
TFB_DisplayFormatAlpha (SDL_Surface *surface)
{
	SDL_Surface* newsurf;
	SDL_PixelFormat* dstfmt;
	const SDL_PixelFormat* srcfmt = surface->format;
	
	// figure out what format to use (alpha/no alpha)
	if (surface->format->Amask)
		dstfmt = format_conv_surf->format;
	else
		dstfmt = SDL_Screen->format;

	if (srcfmt->BytesPerPixel == dstfmt->BytesPerPixel &&
			srcfmt->Rmask == dstfmt->Rmask &&
			srcfmt->Gmask == dstfmt->Gmask &&
			srcfmt->Bmask == dstfmt->Bmask &&
			srcfmt->Amask == dstfmt->Amask)
		return surface; // no conversion needed

	newsurf = SDL_ConvertSurface (surface, dstfmt, 0);

	return newsurf;
}

void
TFB_BlitSurface (SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst,
		SDL_Rect *dstrect, int blend_numer, int blend_denom)
{
	BOOLEAN has_colorkey;
	int x, y, x1, y1, x2, y2, dst_x2, dst_y2, nr, ng, nb;
	int srcx, srcy, w, h;
	Uint8 sr, sg, sb, dr, dg, db;
	Uint32 src_pixval, dst_pixval, colorkey;
	GetPixelFn src_getpix, dst_getpix;
	PutPixelFn putpix;
	SDL_Rect fulldst;

	if (blend_numer == blend_denom)
	{
		// normal blit: dst = src
		
		// log_add (log_Debug, "normal blit\n");
		SDL_BlitSurface (src, srcrect, dst, dstrect);
		return;
	}
		
	// NOTE: following clipping code is copied from SDL-1.2.4 sources

	// If the destination rectangle is NULL, use the entire dest surface
	if (dstrect == NULL)
	{
		fulldst.x = fulldst.y = 0;
		dstrect = &fulldst;
	}

	// clip the source rectangle to the source surface
	if (srcrect)
	{
		int maxw, maxh;

		srcx = srcrect->x;
		w = srcrect->w;
		if (srcx < 0) 
		{
			w += srcx;
			dstrect->x -= srcx;
			srcx = 0;
		}
		maxw = src->w - srcx;
		if (maxw < w)
			w = maxw;

		srcy = srcrect->y;
		h = srcrect->h;
		if (srcy < 0) 
		{
			h += srcy;
			dstrect->y -= srcy;
			srcy = 0;
		}
		maxh = src->h - srcy;
		if (maxh < h)
			h = maxh;
	}
	else
	{
		srcx = 0;
		srcy = 0;
		w = src->w;
		h = src->h;
	}

	// clip the destination rectangle against the clip rectangle
	{
		SDL_Rect *clip = &dst->clip_rect;
		int dx, dy;

		dx = clip->x - dstrect->x;
		if (dx > 0)
		{
			w -= dx;
			dstrect->x += dx;
			srcx += dx;
		}
		dx = dstrect->x + w - clip->x - clip->w;
		if (dx > 0)
			w -= dx;

		dy = clip->y - dstrect->y;
		if (dy > 0)
		{
			h -= dy;
			dstrect->y += dy;
			srcy += dy;
		}
		dy = dstrect->y + h - clip->y - clip->h;
		if (dy > 0)
			h -= dy;
	}

	dstrect->w = w;
	dstrect->h = h;

	if (w <= 0 || h <= 0)
		return;

	x1 = srcx;
	y1 = srcy;
	x2 = srcx + w;
	y2 = srcy + h;

	if (SDL_GetColorKey (src, &colorkey) < 0)
	{
		has_colorkey = FALSE;
		colorkey = 0;  /* Satisfying compiler */
	}
	else
	{
		has_colorkey = TRUE;
	}

	src_getpix = getpixel_for (src);
	dst_getpix = getpixel_for (dst);
	putpix = putpixel_for (dst);

	if (blend_denom < 0)
	{
		// additive blit: dst = src + dst
#if 0
		log_add (log_Debug, "additive blit %d %d, src %d %d %d %d dst %d %d,"
				" srcbpp %d", blend_numer, blend_denom, x1, y1, x2, y2,
				dstrect->x, dstrect->y, src->format->BitsPerPixel);
#endif		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);
				
				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr + dr;
				ng = sg + dg;
				nb = sb + db;

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else if (blend_numer < 0)
	{
		// subtractive blit: dst = src - dst
#if 0
		log_add (log_Debug, "subtractive blit %d %d, src %d %d %d %d"
				" dst %d %d, srcbpp %d", blend_numer, blend_denom,
					x1, y1, x2, y2, dstrect->x, dstrect->y,
					src->format->BitsPerPixel);
#endif		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;

				dst_pixval = dst_getpix (dst, dst_x2, dst_y2);

				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);
				SDL_GetRGB (dst_pixval, dst->format, &dr, &dg, &db);

				nr = sr - dr;
				ng = sg - dg;
				nb = sb - db;

				if (nr < 0)
					nr = 0;
				if (ng < 0)
					ng = 0;
				if (nb < 0)
					nb = 0;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
	else 
	{
		// modulated blit: dst = src * (blend_numer / blend_denom) 

		float f = blend_numer / (float)blend_denom;
#if 0
		log_add (log_Debug, "modulated blit %d %d, f %f, src %d %d %d %d"
				" dst %d %d, srcbpp %d\n", blend_numer, blend_denom, f,
				x1, y1, x2, y2, dstrect->x, dstrect->y,
				src->format->BitsPerPixel);
#endif		
		for (y = y1; y < y2; ++y)
		{
			dst_y2 = dstrect->y + (y - y1);
			for (x = x1; x < x2; ++x)
			{
				dst_x2 = dstrect->x + (x - x1);
				src_pixval = src_getpix (src, x, y);

				if (has_colorkey && src_pixval == colorkey)
					continue;
				
				SDL_GetRGB (src_pixval, src->format, &sr, &sg, &sb);

				nr = (int)(sr * f);
				ng = (int)(sg * f);
				nb = (int)(sb * f);

				if (nr > 255)
					nr = 255;
				if (ng > 255)
					ng = 255;
				if (nb > 255)
					nb = 255;

				putpix (dst, dst_x2, dst_y2,
						SDL_MapRGB (dst->format, nr, ng, nb));
			}
		}
	}
}

void
TFB_SetGamma (float gamma)
{
	log_add (log_Warning, "Custom gamma correction is not available in the SDL2 engine.");
}

int
TFB_GetSurfaceAlphaMod (SDL_Surface *surface, Uint8 *alpha)
{
	SDL_BlendMode blend_mode;
	if (!surface || !alpha)
	{
		return -1;
	}
	if (SDL_GetSurfaceBlendMode (surface, &blend_mode) == 0)
	{
		return SDL_GetSurfaceAlphaMod (surface, alpha);
	}
	*alpha = 255;
	return 0;
}

int
TFB_SetSurfaceAlphaMod (SDL_Surface *surface, Uint8 alpha)
{
	int result = SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_BLEND);
	if (result == 0)
	{
		result = SDL_SetSurfaceAlphaMod (surface, alpha);
	}
	return result;
}

int
TFB_DisableSurfaceAlphaMod (SDL_Surface *surface)
{
	SDL_SetSurfaceAlphaMod (surface, 255);
	return SDL_SetSurfaceBlendMode (surface, SDL_BLENDMODE_NONE);
}

int
TFB_GetColorKey (SDL_Surface *surface, Uint32 *key)
{
	return SDL_GetColorKey (surface, key);
}

int
TFB_SetColorKey (SDL_Surface *surface, Uint32 key)
{
	return SDL_SetColorKey (surface, SDL_TRUE, key);
}

int
TFB_DisableColorKey (SDL_Surface *surface)
{
	return SDL_SetColorKey (surface, SDL_FALSE, 0);
}

#endif
