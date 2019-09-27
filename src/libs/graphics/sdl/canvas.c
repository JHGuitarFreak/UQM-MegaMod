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

#include "port.h"
#include <string.h>
		// for memcpy()

#include SDL_INCLUDE(SDL.h)
#include "sdl_common.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/tfb_draw.h"
#include "libs/graphics/cmap.h"
#include "libs/log.h"
#include "libs/memlib.h"
#include "primitives.h"
#include "palette.h"
#include "sdluio.h"
#include "rotozoom.h"
#include "options.h"
#include "types.h"

typedef SDL_Surface *NativeCanvas;

// BYTE x BYTE weight (mult >> 8) table
static Uint8 btable[256][256];

void
TFB_DrawCanvas_Initialize (void)
{
	int i, j;
	for (i = 0; i < 256; ++i)
		for (j = 0; j < 256; ++j)
			btable[j][i] = (j * i + 0x80) >> 8;
				// need error correction here
}

const char *
TFB_DrawCanvas_GetError (void)
{
	const char *err = SDL_GetError ();
	// TODO: Should we call SDL_ClearError() here so that it is not
	//   returned again later?
	return err;
}

static void
checkPrimitiveMode (SDL_Surface *surf, Color *color, DrawMode *mode)
{
	const SDL_PixelFormat *fmt = surf->format;
	// Special case: We support DRAW_ALPHA mode to non-alpha surfaces
	// for primitives via Color.a
	if (mode->kind == DRAW_REPLACE && fmt->Amask == 0 && color->a != 0xff)
	{
		mode->kind = DRAW_ALPHA;
		mode->factor = color->a;
		color->a = 0xff;
	}
}

void
TFB_DrawCanvas_Line (int x1, int y1, int x2, int y2, Color color,
		DrawMode mode, TFB_Canvas target)
{
	SDL_Surface *dst = target;
	SDL_PixelFormat *fmt = dst->format;
	Uint32 sdlColor;
	RenderPixelFn plotFn;

	checkPrimitiveMode (dst, &color, &mode);
	sdlColor = SDL_MapRGBA (fmt, color.r, color.g, color.b, color.a);

	plotFn = renderpixel_for (target, mode.kind);
	if (!plotFn)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_Line "
				"unsupported draw mode (%d)", (int)mode.kind);
		return;
	}

	SDL_LockSurface (dst);
	line_prim (x1, y1, x2, y2, sdlColor, plotFn, mode.factor, dst);
	SDL_UnlockSurface (dst);
}

void
TFB_DrawCanvas_Rect (RECT *rect, Color color, DrawMode mode, TFB_Canvas target)
{
	SDL_Surface *dst = target;
	SDL_PixelFormat *fmt = dst->format;
	Uint32 sdlColor;
	SDL_Rect sr;
	sr.x = rect->corner.x;
	sr.y = rect->corner.y;
	sr.w = rect->extent.width;
	sr.h = rect->extent.height;

	checkPrimitiveMode (dst, &color, &mode);
	sdlColor = SDL_MapRGBA (fmt, color.r, color.g, color.b, color.a);

	if (mode.kind == DRAW_REPLACE)
	{	// Standard SDL fillrect rendering
		if (fmt->Amask && (dst->flags & SDL_SRCCOLORKEY))
		{	// special case -- alpha surface with colorkey
			// colorkey rects are transparent
			if ((sdlColor & ~fmt->Amask) == (fmt->colorkey & ~fmt->Amask))
				sdlColor &= ~fmt->Amask; // make transparent
		}
		SDL_FillRect (dst, &sr, sdlColor);
	}
	else
	{	// Custom fillrect rendering
		RenderPixelFn plotFn = renderpixel_for (target, mode.kind);
		if (!plotFn)
		{
			log_add (log_Warning, "ERROR: TFB_DrawCanvas_Rect "
					"unsupported draw mode (%d)", (int)mode.kind);
			return;
		}

		SDL_LockSurface (dst);
		fillrect_prim (sr, sdlColor, plotFn, mode.factor, dst);
		SDL_UnlockSurface (dst);
	}
}

static void
TFB_DrawCanvas_Blit (SDL_Surface *src, SDL_Rect *src_r,
		SDL_Surface *dst, SDL_Rect *dst_r, DrawMode mode)
{
	SDL_PixelFormat *srcfmt = src->format;

	if (mode.kind == DRAW_REPLACE)
	{	// Standard SDL simple blit
		SDL_BlitSurface (src, src_r, dst, dst_r);
	}
	else if (mode.kind == DRAW_ALPHA && srcfmt->Amask == 0)
	{	// Standard SDL surface-alpha blit
		// Note that surface alpha and per-pixel alpha cannot work
		// at the same time, which is why the Amask test
		assert (!(src->flags & SDL_SRCALPHA));
		// Set surface alpha temporarily
		SDL_SetAlpha (src, SDL_SRCALPHA, mode.factor);
		SDL_BlitSurface (src, src_r, dst, dst_r);
		SDL_SetAlpha (src, 0, 255);
	}
	else
	{	// Custom blit
		SDL_Rect loc_src_r, loc_dst_r;
		RenderPixelFn plotFn = renderpixel_for (dst, mode.kind);
		if (!plotFn)
		{
			log_add (log_Warning, "ERROR: TFB_DrawCanvas_Blit "
					"unsupported draw mode (%d)", (int)mode.kind);
			return;
		}

		if (!src_r)
		{	// blit whole image; generate rect
			loc_src_r.x = 0;
			loc_src_r.y = 0;
			loc_src_r.w = src->w;
			loc_src_r.h = src->h;
			src_r = &loc_src_r;
		}

		if (!dst_r)
		{	// blit to 0,0; generate rect
			loc_dst_r.x = 0;
			loc_dst_r.y = 0;
			loc_dst_r.w = dst->w;
			loc_dst_r.h = dst->h;
			dst_r = &loc_dst_r;
		}

		SDL_LockSurface (dst);
		blt_prim (src, *src_r, plotFn, mode.factor, dst, *dst_r);
		SDL_UnlockSurface (dst);
	}
}

// XXX: If a colormap is passed in, it has to have been acquired via
// TFB_GetColorMap(). We release the colormap at the end.
void
TFB_DrawCanvas_Image (TFB_Image *img, int x, int y, int scale,
		int scaleMode, TFB_ColorMap *cmap, DrawMode mode, TFB_Canvas target)
{
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;
	SDL_Palette *NormalPal;

	if (img == 0)
	{
		log_add (log_Warning,
				"ERROR: TFB_DrawCanvas_Image passed null image ptr");
		return;
	}

	LockMutex (img->mutex);

	NormalPal = ((SDL_Surface *)img->NormalImg)->format->palette;
	// only set the new palette if it changed
	if (NormalPal && cmap && img->colormap_version != cmap->version)
		SDL_SetColors (img->NormalImg, cmap->palette->colors, 0, 256);

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		if (scaleMode == TFB_SCALE_TRILINEAR && img->MipmapImg)
		{
			// only set the new palette if it changed
			if (TFB_DrawCanvas_IsPaletted (img->MipmapImg)
					&& cmap && img->colormap_version != cmap->version)
				SDL_SetColors (img->MipmapImg, cmap->palette->colors, 0, 256);
		}
		else if (scaleMode == TFB_SCALE_TRILINEAR && !img->MipmapImg)
		{	// Do bilinear scaling instead when mipmap is unavailable
			scaleMode = TFB_SCALE_BILINEAR;
		}

		TFB_DrawImage_FixScaling (img, scale, scaleMode);
		surf = img->ScaledImg;
		if (TFB_DrawCanvas_IsPaletted (surf))
		{
			// We may only get a paletted scaled image if the source is
			// paletted. Currently, all scaling targets are truecolor.
			assert (NormalPal && NormalPal->colors);
			SDL_SetColors (surf, NormalPal->colors, 0, NormalPal->ncolors);
		}

		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;

		targetRect.x = x - img->last_scale_hs.x;
		targetRect.y = y - img->last_scale_hs.y;
	}
	else
	{
		surf = img->NormalImg;
		pSrcRect = NULL;

		targetRect.x = x - img->NormalHs.x;
		targetRect.y = y - img->NormalHs.y;
	}
	
	if (cmap)
	{
		img->colormap_version = cmap->version;
		// TODO: Technically, this is not a proper place to release a
		//   colormap. As it stands now, the colormap must have been
		//   addrefed when passed to us.
		TFB_ReturnColorMap (cmap);
	}
	
	TFB_DrawCanvas_Blit (surf, pSrcRect, target, &targetRect, mode);
	UnlockMutex (img->mutex);
}

// Assumes the source and destination surfaces are in the same format
static void
TFB_DrawCanvas_Fill (SDL_Surface *src, Uint32 fillcolor, SDL_Surface *dst)
{
	const SDL_PixelFormat *srcfmt = src->format;
	SDL_PixelFormat *dstfmt = dst->format;
	const int width = src->w;
	const int height = src->h;
	const int bpp = dstfmt->BytesPerPixel;
	const int sp = src->pitch, dp = dst->pitch;
	const int slen = sp / bpp, dlen = dp / bpp;
	const int dsrc = slen - width, ddst = dlen - width;
	Uint32 *src_p;
	Uint32 *dst_p;
	int x, y;
	Uint32 dstkey = 0; // 0 means alpha=0 too
	Uint32 amask = srcfmt->Amask;
	int alpha = (fillcolor & amask) >> srcfmt->Ashift;

	if (srcfmt->BytesPerPixel != 4 || dstfmt->BytesPerPixel != 4)
	{
		log_add (log_Warning, "TFB_DrawCanvas_Fill: Unsupported surface "
				"formats: %d bytes/pixel source, %d bytes/pixel destination",
				(int)srcfmt->BytesPerPixel, (int)dstfmt->BytesPerPixel);
		return;
	}

	// Strip the alpha channel from fillcolor because we process the
	// alpha separately
	fillcolor &= ~amask;

	SDL_LockSurface(src);
	SDL_LockSurface(dst);

	src_p = (Uint32 *)src->pixels;
	dst_p = (Uint32 *)dst->pixels;

	if (dstkey == fillcolor)
	{	// color collision, must switch colorkey
		// new colorkey is grey (1/2,1/2,1/2)
		dstkey = SDL_MapRGBA (dstfmt, 127, 127, 127, 0);
	}

	if (srcfmt->Amask)
	{	// alpha-based fill
		for (y = 0; y < height; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = 0; x < width; ++x, ++src_p, ++dst_p)
			{
				Uint32 p = *src_p & amask;

				if (p == 0)
				{	// fully transparent pixel
					*dst_p = dstkey;
				}
				else if (alpha == 0xff)
				{	// not for DRAW_ALPHA; use alpha chan directly
					*dst_p = p | fillcolor;
				}
				else
				{	// for DRAW_ALPHA; modulate the alpha channel
					p >>= srcfmt->Ashift;
					p = (p * alpha) >> 8;
					p <<= srcfmt->Ashift;
					*dst_p = p | fillcolor;
				}
			}
		}
	}
	else if (src->flags & SDL_SRCCOLORKEY)
	{	// colorkey-based fill
		Uint32 srckey = srcfmt->colorkey;

		for (y = 0; y < height; ++y, dst_p += ddst, src_p += dsrc)
		{
			for (x = 0; x < width; ++x, ++src_p, ++dst_p)
			{
				Uint32 p = *src_p;

				*dst_p = (p == srckey) ? dstkey : fillcolor;
			}
		}
	}
	else
	{
		log_add (log_Warning, "TFB_DrawCanvas_Fill: Unsupported source"
				"surface format\n");
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);

	// save the colorkey (dynamic image -- not using RLE coding here)
	SDL_SetColorKey (dst, SDL_SRCCOLORKEY, dstkey);
			// if the filled surface is RGBA, colorkey will only be used
			// when SDL_SRCALPHA flag is cleared. this allows us to blit
			// the surface in different ways to diff targets
}

void
TFB_DrawCanvas_FilledImage (TFB_Image *img, int x, int y, int scale,
		int scaleMode, Color color, DrawMode mode, TFB_Canvas target)
{
	SDL_Surface *dst = target;
	SDL_Rect srcRect, targetRect, *pSrcRect;
	SDL_Surface *surf;
	SDL_Palette *palette;
	int i;
	bool force_fill = false;

	if (img == 0)
	{
		log_add (log_Warning,
				"ERROR: TFB_DrawCanvas_FilledImage passed null image ptr");
		return;
	}

	checkPrimitiveMode (dst, &color, &mode);

	LockMutex (img->mutex);

	if (scale != 0 && scale != GSCALE_IDENTITY)
	{
		if (scaleMode == TFB_SCALE_TRILINEAR)
			scaleMode = TFB_SCALE_BILINEAR;
					// no point in trilinear for filled images

		if (scale != img->last_scale || scaleMode != img->last_scale_type)
			force_fill = true;

		TFB_DrawImage_FixScaling (img, scale, scaleMode);
		surf = img->ScaledImg;
		srcRect.x = 0;
		srcRect.y = 0;
		srcRect.w = img->extent.width;
		srcRect.h = img->extent.height;
		pSrcRect = &srcRect;

		targetRect.x = x - img->last_scale_hs.x;
		targetRect.y = y - img->last_scale_hs.y;
	}
	else
	{
		if (img->last_scale != 0)
		{
			// Make sure we remember that the last fill was from
			// an unscaled image
			force_fill = true;
			img->last_scale = 0;
		}

		surf = img->NormalImg;
		pSrcRect = NULL;
		
		targetRect.x = x - img->NormalHs.x;
		targetRect.y = y - img->NormalHs.y;
	}

	palette = surf->format->palette;
	if (palette)
	{	// set palette for fill-stamp
		// Calling SDL_SetColors() results in an expensive src -> dst
		// color-mapping operation for an SDL blit, following the call.
		// We want to avoid that as much as possible.
		
		// TODO: generate a 32bpp filled image?

		SDL_Color colors[256];

		colors[0] = ColorToNative (color);
		for (i = 1; i < palette->ncolors; i++)
			colors[i] = colors[0];

		SDL_SetColors (surf, colors, 0, palette->ncolors);
		// reflect the change in *actual* image palette
		img->colormap_version--;
	}
	else
	{	// fill the non-transparent parts of the image with fillcolor
		SDL_Surface *newfill = img->FilledImg;
		SDL_PixelFormat *fillfmt;

		if (newfill && (newfill->w < surf->w || newfill->h < surf->h))
		{
			TFB_DrawCanvas_Delete (newfill);
			newfill = NULL;
		}

		// prepare the filled image
		if (!newfill)
		{
			newfill = SDL_CreateRGBSurface (SDL_SWSURFACE,
						surf->w, surf->h,
						surf->format->BitsPerPixel,
						surf->format->Rmask,
						surf->format->Gmask,
						surf->format->Bmask,
						surf->format->Amask);
			force_fill = true;
		}
		fillfmt = newfill->format;

		if (force_fill || !sameColor (img->last_fill, color))
		{	// image or fillcolor changed - regenerate
			Uint32 fillColor;

			if (mode.kind == DRAW_ALPHA && fillfmt->Amask)
			{	// Per-pixel alpha and surface alpha will not work together
				// We have to handle DRAW_ALPHA differently by modulating
				// the surface alpha channel ourselves.
				color.a = mode.factor;
				mode.kind = DRAW_REPLACE;
			}
			else
			{	// Make sure we do not modulate the alpha channel
				color.a = 0xff;
			}
			fillColor = SDL_MapRGBA (newfill->format, color.r, color.g,
					color.b, color.a);
			TFB_DrawCanvas_Fill (surf, fillColor, newfill);
			// cache filled image if possible
			img->last_fill = color;
		}

		img->FilledImg = newfill;
		surf = newfill;
	}

	TFB_DrawCanvas_Blit (surf, pSrcRect, dst, &targetRect, mode);
	UnlockMutex (img->mutex);
}

void
TFB_DrawCanvas_FontChar (TFB_Char *fontChar, TFB_Image *backing,
		int x, int y, DrawMode mode, TFB_Canvas target)
{
	SDL_Surface *dst = target;
	SDL_Rect srcRect, targetRect;
	SDL_Surface *surf;
	int w, h;

	if (fontChar == 0)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar passed null char ptr");
		return;
	}
	if (backing == 0)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar passed null backing ptr");
		return;
	}

	w = fontChar->extent.width;
	h = fontChar->extent.height;

	LockMutex (backing->mutex);

	surf = backing->NormalImg;
	if (surf->format->BytesPerPixel != 4
			|| surf->w < w || surf->h < h)
	{
		log_add (log_Warning, "ERROR: "
				"TFB_DrawCanvas_FontChar bad backing surface: %dx%dx%d; "
				"char: %dx%d",
				surf->w, surf->h, (int)surf->format->BytesPerPixel, w, h);
		UnlockMutex (backing->mutex);
		return;
	}

	srcRect.x = 0;
	srcRect.y = 0;
	srcRect.w = fontChar->extent.width;
	srcRect.h = fontChar->extent.height;

	targetRect.x = x - fontChar->HotSpot.x;
	targetRect.y = y - fontChar->HotSpot.y;

	// transfer the alpha channel to the backing surface
	SDL_LockSurface (surf);
	{
		int x, y;
		const int sskip = fontChar->pitch - w;
		const int dskip = (surf->pitch / 4) - w;
		const Uint32 dmask = ~surf->format->Amask;
		const int ashift = surf->format->Ashift;
		Uint8 *src_p = fontChar->data;
		Uint32 *dst_p = (Uint32 *)surf->pixels;

		if (mode.kind == DRAW_ALPHA)
		{	// Per-pixel alpha and surface alpha will not work together
			// We have to handle DRAW_ALPHA differently by modulating
			// the backing surface alpha channel ourselves.
			// The existing backing surface alpha channel is ignored.
			int alpha = mode.factor;
			mode.kind = DRAW_REPLACE;

			for (y = 0; y < h; ++y, src_p += sskip, dst_p += dskip)
			{
				for (x = 0; x < w; ++x, ++src_p, ++dst_p)
				{
					Uint32 p = *dst_p & dmask;
					Uint32 a = *src_p;
					
					// we use >> 8 instead of / 255, and it does not handle
					// alpha == 255 correctly
					if (alpha != 0xff)
					{	// modulate the alpha channel
						a = (a * alpha) >> 8;
					}
					*dst_p = p | (a << ashift);
				}
			}
		}
		else /* if (mode.kind != DRAW_ALPHA) */
		{	// Transfer the alpha channel to the backing surface
			// DRAW_REPLACE + Color.a is NOT supported right now
			for (y = 0; y < h; ++y, src_p += sskip, dst_p += dskip)
			{
				for (x = 0; x < w; ++x, ++src_p, ++dst_p)
				{
					*dst_p = (*dst_p & dmask) | ((Uint32)*src_p << ashift);
				}
			}
		}
	}
	SDL_UnlockSurface (surf);

	TFB_DrawCanvas_Blit (surf, &srcRect, dst, &targetRect, mode);
	UnlockMutex (backing->mutex);
}

TFB_Canvas
TFB_DrawCanvas_New_TrueColor (int w, int h, BOOLEAN hasalpha)
{
	SDL_Surface *new_surf;
	SDL_PixelFormat* fmt = format_conv_surf->format;

	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
			fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask,
			hasalpha ? fmt->Amask : 0);
	if (!new_surf)
	{
		log_add (log_Fatal, "INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_ForScreen (int w, int h, BOOLEAN withalpha)
{
	SDL_Surface *new_surf;
	SDL_PixelFormat* fmt = SDL_Screen->format;

	if (fmt->palette)
	{
		log_add (log_Warning, "TFB_DrawCanvas_New_ForScreen() WARNING:"
				"Paletted display format will be slow");

		new_surf = TFB_DrawCanvas_New_TrueColor (w, h, withalpha);
	}
	else
	{
		if (withalpha && fmt->Amask == 0)
			fmt = format_conv_surf->format; // Screen has no alpha and we need it

		new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h,
				fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask,
				withalpha ? fmt->Amask : 0);
	}

	if (!new_surf)
	{
		log_add (log_Fatal, "TFB_DrawCanvas_New_ForScreen() INTERNAL PANIC:"
				"Failed to create TFB_Canvas: %s", SDL_GetError());
		exit (EXIT_FAILURE);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_Paletted (int w, int h, Color palette[256],
		int transparent_index)
{
	SDL_Surface *new_surf;
	new_surf = SDL_CreateRGBSurface (SDL_SWSURFACE, w, h, 8, 0, 0, 0, 0);
	if (!new_surf)
	{
		log_add (log_Fatal, "INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}
	if (palette != NULL)
	{
		TFB_DrawCanvas_SetPalette (new_surf, palette);
	}
	if (transparent_index >= 0)
	{
		SDL_SetColorKey (new_surf, SDL_SRCCOLORKEY, transparent_index);
	}
	else
	{
		SDL_SetColorKey (new_surf, 0, 0);
	}
	return new_surf;
}

TFB_Canvas
TFB_DrawCanvas_New_ScaleTarget (TFB_Canvas canvas, TFB_Canvas oldcanvas, int type, int last_type)
{
	SDL_Surface *src = canvas;
	SDL_Surface *old = oldcanvas;
	SDL_Surface *newsurf = NULL;

	// For the purposes of this function, bilinear == trilinear
	if (type == TFB_SCALE_TRILINEAR)
		type = TFB_SCALE_BILINEAR;
	if (last_type == TFB_SCALE_TRILINEAR)
		last_type = TFB_SCALE_BILINEAR;

	if (old && type != last_type)
	{
		TFB_DrawCanvas_Delete (old);
		old = NULL;
	}
	if (old)
		return old; /* can just reuse the old one */

	if (type == TFB_SCALE_NEAREST)
	{
		newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE, src->w,
					src->h,
					src->format->BitsPerPixel,
					src->format->Rmask,
					src->format->Gmask,
					src->format->Bmask,
					src->format->Amask);
		TFB_DrawCanvas_CopyTransparencyInfo (src, newsurf);
	}
	else
	{
		// The scaled image may in fact be larger by 1 pixel than the source
		// because of hotspot alignment and fractional edge pixels
		if (SDL_Screen->format->BitsPerPixel == 32)
			newsurf = TFB_DrawCanvas_New_ForScreen (src->w + 1, src->h + 1, TRUE);
		else
			newsurf = TFB_DrawCanvas_New_TrueColor (src->w + 1, src->h + 1, TRUE);
	}
		
	return newsurf;
}

TFB_Canvas
TFB_DrawCanvas_New_RotationTarget (TFB_Canvas src_canvas, int angle)
{
	SDL_Surface *src = src_canvas;
	SDL_Surface *newsurf;
	EXTENT size;

	TFB_DrawCanvas_GetRotatedExtent (src_canvas, angle, &size);
	
	newsurf = SDL_CreateRGBSurface (SDL_SWSURFACE,
				size.width, size.height,
				src->format->BitsPerPixel,
				src->format->Rmask,
				src->format->Gmask,
				src->format->Bmask,
				src->format->Amask);
	if (!newsurf)
	{
		log_add (log_Fatal, "TFB_DrawCanvas_New_RotationTarget()"
				" INTERNAL PANIC: Failed to create TFB_Canvas: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}
	TFB_DrawCanvas_CopyTransparencyInfo (src, newsurf);
	
	return newsurf;
}

TFB_Canvas
TFB_DrawCanvas_LoadFromFile (void *dir, const char *fileName)
{
	SDL_Surface *surf = sdluio_loadImage (dir, fileName);
	if (!surf)
		return NULL;
	
	if (surf->format->BitsPerPixel < 8)
	{
		SDL_SetError ("unsupported image format (min 8bpp)");
		SDL_FreeSurface (surf);
		surf = NULL;
	}

	return surf;
}

void
TFB_DrawCanvas_Delete (TFB_Canvas canvas)
{
	if (!canvas)
	{
		log_add (log_Warning, "INTERNAL PANIC: Attempted"
				" to delete a NULL canvas!");
		/* Should we actually die here? */
	}
	else
	{
		SDL_FreeSurface (canvas);
	}
}

BOOLEAN
TFB_DrawCanvas_GetFontCharData (TFB_Canvas canvas, BYTE *outData,
		unsigned dataPitch)
{
	SDL_Surface *surf = canvas;
	int x, y;
	Uint8 r, g, b, a;
	Uint32 p;
	SDL_PixelFormat *fmt = surf->format;
	GetPixelFn getpix;

	if (!surf || !outData)
		return FALSE;

	SDL_LockSurface (surf);

	getpix = getpixel_for (surf);

	// produce an alpha-only image in internal BYTE[] format
	//  from the SDL surface
	for (y = 0; y < surf->h; ++y)
	{
		BYTE *dst = outData + dataPitch * y;

		for (x = 0; x < surf->w; ++x, ++dst)
		{
			p = getpix (surf, x, y);
			SDL_GetRGBA (p, fmt, &r, &g, &b, &a);

			if (!fmt->Amask)
			{	// produce alpha from intensity (Y component)
				// using a fast approximation
				// contributions to Y are: R=2, G=4, B=1
				a = ((r * 2) + (g * 4) + b) / 7;
			}
			
			*dst = a;
		}
	}

	SDL_UnlockSurface (surf);

	return TRUE;
}

Color *
TFB_DrawCanvas_ExtractPalette (TFB_Canvas canvas)
{
	int i;		
	Color *result;
	SDL_Surface *surf = canvas;
	SDL_Palette *palette = surf->format->palette;

	if (!palette)
		return NULL;

	// There may be less colors in the surface than 256. Init to 0 first.
	result = HCalloc (sizeof (Color) * 256);
	assert (palette->ncolors <= 256);
	for (i = 0; i < palette->ncolors; ++i)
		result[i] = NativeToColor (palette->colors[i]);
		
	return result;
}

TFB_Canvas
TFB_DrawCanvas_ToScreenFormat (TFB_Canvas canvas)
{
	SDL_Surface *result = TFB_DisplayFormatAlpha (canvas);
	if (result == NULL)
	{
		log_add (log_Debug, "WARNING: Could not convert"
				" sprite-canvas to display format.");
		return canvas;
	}
	else if (result == canvas)
	{	// no conversion was necessary
		return canvas;
	}
	else
	{	// converted
		TFB_DrawCanvas_Delete (canvas);
		return result;
	}

	return canvas;
}

BOOLEAN
TFB_DrawCanvas_IsPaletted (TFB_Canvas canvas)
{
	return ((SDL_Surface *)canvas)->format->palette != NULL;
}

void
TFB_DrawCanvas_SetPalette (TFB_Canvas target, Color palette[256])
{
	SDL_Color colors[256];
	int i;

	for (i = 0; i < 256; ++i)
		colors[i] = ColorToNative (palette[i]);

	SDL_SetColors (target, colors, 0, 256);
}

int
TFB_DrawCanvas_GetTransparentIndex (TFB_Canvas canvas)
{
	if (((SDL_Surface *)canvas)->flags & SDL_SRCCOLORKEY)
		return ((SDL_Surface *)canvas)->format->colorkey;
	return -1;
}

void
TFB_DrawCanvas_SetTransparentIndex (TFB_Canvas canvas, int index, BOOLEAN rleaccel)
{
	if (index >= 0)
	{
		int flags = SDL_SRCCOLORKEY;
		if (rleaccel)
			flags |= SDL_RLEACCEL;
		SDL_SetColorKey (canvas, flags, index);
		
		if (!TFB_DrawCanvas_IsPaletted (canvas))
		{
			// disables surface alpha so color key transparency actually works
			SDL_SetAlpha (canvas, 0, 255); 
		}
	}
	else
	{
		SDL_SetColorKey (canvas, 0, 0); 
	}		
}

void
TFB_DrawCanvas_CopyTransparencyInfo (TFB_Canvas src_canvas,
		TFB_Canvas dst_canvas)
{
	SDL_Surface* src = src_canvas;

	if (src->format->palette)
	{
		int index;
		index = TFB_DrawCanvas_GetTransparentIndex (src_canvas);
		TFB_DrawCanvas_SetTransparentIndex (dst_canvas, index, FALSE);
	}
	else
	{
		Color color;
		if (TFB_DrawCanvas_GetTransparentColor (src_canvas, &color))
			TFB_DrawCanvas_SetTransparentColor (dst_canvas, color, FALSE);
	}
}

BOOLEAN
TFB_DrawCanvas_GetTransparentColor (TFB_Canvas canvas, Color *color)
{
	if (!TFB_DrawCanvas_IsPaletted (canvas)
			&& (((SDL_Surface *)canvas)->flags & SDL_SRCCOLORKEY) )
	{
		Uint8 ur, ug, ub;
		int colorkey = ((SDL_Surface *)canvas)->format->colorkey;
		SDL_GetRGB (colorkey, ((SDL_Surface *)canvas)->format, &ur, &ug, &ub);
		color->r = ur;
		color->g = ug;
		color->b = ub;
		color->a = 0xff;
		return TRUE;
	}
	return FALSE;
}

void
TFB_DrawCanvas_SetTransparentColor (TFB_Canvas canvas, Color color,
		BOOLEAN rleaccel)
{
	Uint32 sdlColor;
	int flags = SDL_SRCCOLORKEY;
	if (rleaccel)
		flags |= SDL_RLEACCEL;
	sdlColor = SDL_MapRGBA (((SDL_Surface *)canvas)->format,
			color.r, color.g, color.b, 0);
	SDL_SetColorKey (canvas, flags, sdlColor);
	
	if (!TFB_DrawCanvas_IsPaletted (canvas))
	{
		// disables surface alpha so color key transparency actually works
		SDL_SetAlpha (canvas, 0, 255); 
	}
}

void
TFB_DrawCanvas_GetScaledExtent (TFB_Canvas src_canvas, HOT_SPOT* src_hs,
		TFB_Canvas src_mipmap, HOT_SPOT* mm_hs,
		int scale, int type, EXTENT *size, HOT_SPOT *hs)
{
	SDL_Surface *src = src_canvas;
	sint32 x, y, w, h;
	int frac;
	
	if (!src_mipmap)
	{
		w = src->w * scale;
		h = src->h * scale;
		x = src_hs->x * scale;
		y = src_hs->y * scale;
	}
	else
	{
		// interpolates extents between src and mipmap to get smoother
		// transition when surface changes
		SDL_Surface *mipmap = src_mipmap;
		int ratio = scale * 2 - GSCALE_IDENTITY;

		assert (scale >= GSCALE_IDENTITY / 2);

		w = mipmap->w * GSCALE_IDENTITY + (src->w - mipmap->w) * ratio;
		h = mipmap->h * GSCALE_IDENTITY + (src->h - mipmap->h) * ratio;

		// Seems it is better to use mipmap hotspot because some
		// source and mipmap images have the same dimensions!
		x = mm_hs->x * GSCALE_IDENTITY + (src_hs->x - mm_hs->x) * ratio;
		y = mm_hs->y * GSCALE_IDENTITY + (src_hs->y - mm_hs->y) * ratio;
	}

	if (type != TFB_SCALE_NEAREST)
	{
		// align hotspot on an whole pixel
		if (x & (GSCALE_IDENTITY - 1))
		{
			frac = GSCALE_IDENTITY - (x & (GSCALE_IDENTITY - 1));
			x += frac;
			w += frac;
		}
		if (y & (GSCALE_IDENTITY - 1))
		{
			frac = GSCALE_IDENTITY - (y & (GSCALE_IDENTITY - 1));
			y += frac;
			h += frac;
		}
		// pad the extent to accomodate fractional edge pixels
		w += (GSCALE_IDENTITY - 1);
		h += (GSCALE_IDENTITY - 1);
	}

	size->width = w / GSCALE_IDENTITY;
	size->height = h / GSCALE_IDENTITY;
	hs->x = x / GSCALE_IDENTITY;
	hs->y = y / GSCALE_IDENTITY;

	// Scaled image can be larger than the source by 1 pixel
	// because of hotspot alignment and fractional edge pixels
	assert (size->width <= src->w + 1 && size->height <= src->h + 1);

	if (!size->width && src->w)
		size->width = 1;
	if (!size->height && src->h)
		size->height = 1;
}

void
TFB_DrawCanvas_GetExtent (TFB_Canvas canvas, EXTENT *size)
{
	SDL_Surface *src = canvas;

	size->width = src->w;
	size->height = src->h;
}

void
TFB_DrawCanvas_Rescale_Nearest (TFB_Canvas src_canvas, TFB_Canvas dst_canvas,
		int scale, HOT_SPOT* src_hs, EXTENT* size, HOT_SPOT* dst_hs)
{
	SDL_Surface *src = src_canvas;
	SDL_Surface *dst = dst_canvas;
	int x, y;
	int fsx = 0, fsy = 0; // source fractional dx and dy increments
	int ssx = 0, ssy = 0; // source fractional x and y starting points
	int w, h;

	if (scale > 0)
	{
		TFB_DrawCanvas_GetScaledExtent (src, src_hs, NULL, NULL, scale,
				TFB_SCALE_NEAREST, size, dst_hs);

		w = size->width;
		h = size->height;
	}
	else
	{
		// Just go with the dst surface dimensions
		w = dst->w;
		h = dst->h;
	}

	if (w > dst->w || h > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Nearest: Tried to scale"
				" image to size %d %d when dest_canvas has only"
				" dimensions of %d %d! Failing.",
				w, h, dst->w, dst->h);
		return;
	}

	if (w > 1)
		fsx = ((src->w - 1) << 16) / (w - 1);
	if (h > 1)
		fsy = ((src->h - 1) << 16) / (h - 1);
	// We start with a value in 0..0.5 range to shift the bigger
	// jumps towards the center of the image
	ssx = 0x6000;
	ssy = 0x6000;

	SDL_LockSurface (src);
	SDL_LockSurface (dst);

	if (src->format->BytesPerPixel == 1 && dst->format->BytesPerPixel == 1)
	{
		Uint8 *sp, *csp, *dp, *cdp;
		int sx, sy; // source fractional x and y positions

		sp = csp = (Uint8 *) src->pixels;
		dp = cdp = (Uint8 *) dst->pixels;

		for (y = 0, sy = ssy; y < h; ++y)
		{
			csp += (sy >> 16) * src->pitch;
			sp = csp;
			dp = cdp;
			for (x = 0, sx = ssx; x < w; ++x)
			{
				sp += (sx >> 16);
				*dp = *sp;
				sx &= 0xffff;
				sx += fsx;
				++dp;
			}
			sy &= 0xffff;
			sy += fsy;
			cdp += dst->pitch;
		}
	}	
	else if (src->format->BytesPerPixel == 4 && dst->format->BytesPerPixel == 4)
	{
		Uint32 *sp, *csp, *dp, *cdp;
		int sx, sy; // source fractional x and y positions
		int sgap, dgap;

		sgap = src->pitch >> 2;
		dgap = dst->pitch >> 2;
		
		sp = csp = (Uint32 *) src->pixels;
		dp = cdp = (Uint32 *) dst->pixels;

		for (y = 0, sy = ssy; y < h; ++y)
		{
			csp += (sy >> 16) * sgap;
			sp = csp;
			dp = cdp;
			for (x = 0, sx = ssx; x < w; ++x)
			{
				sp += (sx >> 16);
				*dp = *sp;
				sx &= 0xffff;
				sx += fsx;
				++dp;
			}
			sy &= 0xffff;
			sy += fsy;
			cdp += dgap;
		}
	}
	else
	{
		log_add (log_Warning, "Tried to deal with unknown BPP: %d -> %d",
				src->format->BitsPerPixel, dst->format->BitsPerPixel);
	}
	SDL_UnlockSurface (dst);
	SDL_UnlockSurface (src);
}

typedef union
{
	Uint32 value;
	Uint8 chan[4];
	struct
	{
		Uint8 r, g, b, a;
	} c;
} pixel_t;

static inline Uint8
dot_product_8_4 (pixel_t* p, int c, Uint8* v)
{	// math expanded for speed
#if 0	
	return (
			(Uint32)p[0].chan[c] * v[0] + (Uint32)p[1].chan[c] * v[1] +
			(Uint32)p[2].chan[c] * v[2] + (Uint32)p[3].chan[c] * v[3]
		) >> 8;
#else
	// mult-table driven version
	return
			btable[p[0].chan[c]][v[0]] + btable[p[1].chan[c]][v[1]] +
			btable[p[2].chan[c]][v[2]] + btable[p[3].chan[c]][v[3]];
#endif
}

static inline Uint8
weight_product_8_4 (pixel_t* p, int c, Uint8* w)
{	// math expanded for speed
	return (
			(Uint32)p[0].chan[c] * w[0] + (Uint32)p[1].chan[c] * w[1] +
			(Uint32)p[2].chan[c] * w[2] + (Uint32)p[3].chan[c] * w[3]
		) / (w[0] + w[1] + w[2] + w[3]);
}

static inline Uint8
blend_ratio_2 (Uint8 c1, Uint8 c2, int ratio)
{	// blend 2 color values according to ratio (0..256)
	// identical to proper alpha blending
	return (((c1 - c2) * ratio) >> 8) + c2;
}

static inline Uint32
scale_read_pixel (void* ppix, SDL_PixelFormat *fmt, SDL_Color *pal,
				Uint32 mask, Uint32 key)
{
	pixel_t p;

	// start off with non-present pixel
	p.value = 0;

	if (pal)
	{	// paletted pixel; mask not used
		Uint32 c = *(Uint8 *)ppix;
		
		if (c != key)
		{
			p.c.r = pal[c].r;
			p.c.g = pal[c].g;
			p.c.b = pal[c].b;
			p.c.a = SDL_ALPHA_OPAQUE;
		}
	}
	else
	{	// RGB(A) pixel; (pix & mask) != key
		Uint32 c = *(Uint32 *)ppix;

		if ((c & mask) != key)
		{
#if 0
			SDL_GetRGBA (c, fmt, &p.c.r, &p.c.g, &p.c.b, &p.c.a);
#else
			// Assume 8 bits/channel; a safe assumption with 32bpp surfaces
			p.c.r = (c >> fmt->Rshift) & 0xff;
			p.c.g = (c >> fmt->Gshift) & 0xff;
			p.c.b = (c >> fmt->Bshift) & 0xff;
			if (fmt->Amask)
				p.c.a = (c >> fmt->Ashift) & 0xff;
			else
				p.c.a = SDL_ALPHA_OPAQUE;
		}
#endif
	}

	return p.value;
}

static inline Uint32
scale_get_pixel (SDL_Surface *src, Uint32 mask, Uint32 key, int x, int y)
{
	SDL_Color *pal = src->format->palette? src->format->palette->colors : 0;

	if (x < 0 || x >= src->w || y < 0 || y >= src->h)
		return 0;

	return scale_read_pixel ((Uint8*)src->pixels + y * src->pitch +
			x * src->format->BytesPerPixel, src->format, pal, mask, key);
}

void
TFB_DrawCanvas_Rescale_Trilinear (TFB_Canvas src_canvas, TFB_Canvas src_mipmap,
		TFB_Canvas dst_canvas, int scale, HOT_SPOT* src_hs, HOT_SPOT* mm_hs,
		EXTENT* size, HOT_SPOT* dst_hs)
{
	SDL_Surface *src = src_canvas;
	SDL_Surface *dst = dst_canvas;
	SDL_Surface *mm = src_mipmap;
	SDL_PixelFormat *srcfmt = src->format;
	SDL_PixelFormat *mmfmt = mm->format;
	SDL_PixelFormat *dstfmt = dst->format;
	SDL_Color *srcpal = srcfmt->palette? srcfmt->palette->colors : 0;
	const int sbpp = srcfmt->BytesPerPixel;
	const int mmbpp = mmfmt->BytesPerPixel;
	const int slen = src->pitch;
	const int mmlen = mm->pitch;
	const int dst_has_alpha = (dstfmt->Amask != 0);
	const int transparent = (dst->flags & SDL_SRCCOLORKEY) ?
			dstfmt->colorkey : 0;
	const int alpha_threshold = dst_has_alpha ? 0 : 127;
	// src v. mipmap importance factor
	int ratio = scale * 2 - GSCALE_IDENTITY;
	// source masks and keys
	Uint32 mk0 = 0, ck0 = ~0, mk1 = 0, ck1 = ~0;
	// source fractional x and y positions
	int sx0, sy0, sx1, sy1;
	// source fractional dx and dy increments
	int fsx0 = 0, fsy0 = 0, fsx1 = 0, fsy1 = 0;
	// source fractional x and y starting points
	int ssx0 = 0, ssy0 = 0, ssx1 = 0, ssy1 = 0;
	int x, y, w, h;

	if (mmfmt->palette && !srcpal) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Trilinear: "
				"Mipmap is paletted, but source is not! Failing.");
		return;
	}

	if (scale > 0)
	{
		int fw, fh;

		// Use (scale / GSCALE_IDENTITY) sizing factor
		TFB_DrawCanvas_GetScaledExtent (src, src_hs, mm, mm_hs, scale,
				TFB_SCALE_TRILINEAR, size, dst_hs);

		w = size->width;
		h = size->height;

		fw = mm->w * GSCALE_IDENTITY + (src->w - mm->w) * ratio;
		fh = mm->h * GSCALE_IDENTITY + (src->h - mm->h) * ratio;

		// This limits the effective source dimensions to 2048x2048,
		// and we also lose 4 bits of precision out of 16 (no problem)
		fsx0 = (src->w << 20) / fw;
		fsx0 <<= 4;
		fsy0 = (src->h << 20) / fh;
		fsy0 <<= 4;

		fsx1 = (mm->w << 20) / fw;
		fsx1 <<= 4;
		fsy1 = (mm->h << 20) / fh;
		fsy1 <<= 4;

		// position the hotspots directly over each other
		ssx0 = (src_hs->x << 16) - fsx0 * dst_hs->x;
		ssy0 = (src_hs->y << 16) - fsy0 * dst_hs->y;

		ssx1 = (mm_hs->x << 16) - fsx1 * dst_hs->x;
		ssy1 = (mm_hs->y << 16) - fsy1 * dst_hs->y;
	}
	else
	{
		// Just go with the dst surface dimensions
		w = dst->w;
		h = dst->h;

		fsx0 = (src->w << 16) / w;
		fsy0 = (src->h << 16) / h;

		fsx1 = (mm->w << 16) / w;
		fsy1 = (mm->h << 16) / h;

		// give equal importance to both edges
		ssx0 = (((src->w - 1) << 16) - fsx0 * (w - 1)) >> 1;
		ssy0 = (((src->h - 1) << 16) - fsy0 * (h - 1)) >> 1;

		ssx1 = (((mm->w - 1) << 16) - fsx1 * (w - 1)) >> 1;
		ssy1 = (((mm->h - 1) << 16) - fsy1 * (h - 1)) >> 1;
	}

	if (w > dst->w || h > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Trilinear: "
				"Tried to scale image to size %d %d when dest_canvas"
				" has only dimensions of %d %d! Failing.",
				w, h, dst->w, dst->h);
		return;
	}

	if ((srcfmt->BytesPerPixel != 1 && srcfmt->BytesPerPixel != 4) ||
		(mmfmt->BytesPerPixel != 1 && mmfmt->BytesPerPixel != 4) ||
		(dst->format->BytesPerPixel != 4))
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Trilinear: "
				"Tried to deal with unknown BPP: %d -> %d, mipmap %d",
				srcfmt->BitsPerPixel, dst->format->BitsPerPixel,
				mmfmt->BitsPerPixel);
		return;
	}

	// use colorkeys where appropriate
	if (srcfmt->Amask)
	{	// alpha transparency
		mk0 = srcfmt->Amask;
		ck0 = 0;
	}
	else if (src->flags & SDL_SRCCOLORKEY)
	{	// colorkey transparency
		mk0 = ~srcfmt->Amask;
		ck0 = srcfmt->colorkey & mk0;
	}

	if (mmfmt->Amask)
	{	// alpha transparency
		mk1 = mmfmt->Amask;
		ck1 = 0;
	}
	else if (mm->flags & SDL_SRCCOLORKEY)
	{	// colorkey transparency
		mk1 = ~mmfmt->Amask;
		ck1 = mmfmt->colorkey & mk1;
	}

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	SDL_LockSurface(mm);
	
	for (y = 0, sy0 = ssy0, sy1 = ssy1;
			y < h;
			++y, sy0 += fsy0, sy1 += fsy1)
	{
		Uint32 *dst_p = (Uint32 *) ((Uint8*)dst->pixels + y * dst->pitch);
		const int py0 = (sy0 >> 16);
		const int py1 = (sy1 >> 16);
		Uint8 *src_a0 = (Uint8*)src->pixels + py0 * slen;
		Uint8 *src_a1 = (Uint8*)mm->pixels + py1 * mmlen;
		// retrieve the fractional portions of y
		const Uint8 v0 = (sy0 >> 8) & 0xff;
		const Uint8 v1 = (sy1 >> 8) & 0xff;
		Uint8 w0[4], w1[4]; // pixel weight vectors

		for (x = 0, sx0 = ssx0, sx1 = ssx1;
				x < w;
				++x, ++dst_p, sx0 += fsx0, sx1 += fsx1)
		{
			const int px0 = (sx0 >> 16);
			const int px1 = (sx1 >> 16);
			// retrieve the fractional portions of x
			const Uint8 u0 = (sx0 >> 8) & 0xff;
			const Uint8 u1 = (sx1 >> 8) & 0xff;
			// pixels are examined and numbered in pattern
			//  0  1
			//  2  3
			// the ideal pixel (4) is somewhere between these four
			// and is calculated from these using weight vector (w)
			// with a dot product
			pixel_t p0[5], p1[5];
			Uint8 res_a;
			
			w0[0] = btable[255 - u0][255 - v0];
			w0[1] = btable[u0][255 - v0];
			w0[2] = btable[255 - u0][v0];
			w0[3] = btable[u0][v0];
			
			w1[0] = btable[255 - u1][255 - v1];
			w1[1] = btable[u1][255 - v1];
			w1[2] = btable[255 - u1][v1];
			w1[3] = btable[u1][v1];

			// Collect interesting pixels from src image
			// Optimization: speed is criticial on larger images;
			// most pixel reads fall completely inside the image
			if (px0 >= 0 && px0 + 1 < src->w && py0 >= 0 && py0 + 1 < src->h)
			{
				Uint8 *src_p = src_a0 + px0 * sbpp;

				p0[0].value = scale_read_pixel (src_p, srcfmt,
						srcpal, mk0, ck0);
				p0[1].value = scale_read_pixel (src_p + sbpp, srcfmt,
						srcpal, mk0, ck0);
				p0[2].value = scale_read_pixel (src_p + slen, srcfmt,
						srcpal, mk0, ck0);
				p0[3].value = scale_read_pixel (src_p + sbpp + slen, srcfmt,
						srcpal, mk0, ck0);
			}
			else
			{
				p0[0].value = scale_get_pixel (src, mk0, ck0, px0, py0);
				p0[1].value = scale_get_pixel (src, mk0, ck0, px0 + 1, py0);
				p0[2].value = scale_get_pixel (src, mk0, ck0, px0, py0 + 1);
				p0[3].value = scale_get_pixel (src, mk0, ck0,
						px0 + 1, py0 + 1);
			}

			// Collect interesting pixels from mipmap image
			if (px1 >= 0 && px1 + 1 < mm->w && py1 >= 0 && py1 + 1 < mm->h)
			{
				Uint8 *mm_p = src_a1 + px1 * mmbpp;

				p1[0].value = scale_read_pixel (mm_p, mmfmt,
						srcpal, mk1, ck1);
				p1[1].value = scale_read_pixel (mm_p + mmbpp, mmfmt,
						srcpal, mk1, ck1);
				p1[2].value = scale_read_pixel (mm_p + mmlen, mmfmt,
						srcpal, mk1, ck1);
				p1[3].value = scale_read_pixel (mm_p + mmbpp + mmlen, mmfmt,
						srcpal, mk1, ck1);
			}
			else
			{
				p1[0].value = scale_get_pixel (mm, mk1, ck1, px1, py1);
				p1[1].value = scale_get_pixel (mm, mk1, ck1, px1 + 1, py1);
				p1[2].value = scale_get_pixel (mm, mk1, ck1, px1, py1 + 1);
				p1[3].value = scale_get_pixel (mm, mk1, ck1,
						px1 + 1, py1 + 1);
			}

			p0[4].c.a = dot_product_8_4 (p0, 3, w0);
			p1[4].c.a = dot_product_8_4 (p1, 3, w1);
			
			res_a = blend_ratio_2 (p0[4].c.a, p1[4].c.a, ratio);

			if (res_a <= alpha_threshold)
			{
				*dst_p = transparent;
			}
			else if (!dst_has_alpha)
			{	// RGB surface handling
				p0[4].c.r = dot_product_8_4 (p0, 0, w0);
				p0[4].c.g = dot_product_8_4 (p0, 1, w0);
				p0[4].c.b = dot_product_8_4 (p0, 2, w0);

				p1[4].c.r = dot_product_8_4 (p1, 0, w1);
				p1[4].c.g = dot_product_8_4 (p1, 1, w1);
				p1[4].c.b = dot_product_8_4 (p1, 2, w1);

				p0[4].c.r = blend_ratio_2 (p0[4].c.r, p1[4].c.r, ratio);
				p0[4].c.g = blend_ratio_2 (p0[4].c.g, p1[4].c.g, ratio);
				p0[4].c.b = blend_ratio_2 (p0[4].c.b, p1[4].c.b, ratio);

				// TODO: we should handle alpha-blending here, but we do
				//   not know the destination color for blending!

				*dst_p =
					(p0[4].c.r << dstfmt->Rshift) |
					(p0[4].c.g << dstfmt->Gshift) |
					(p0[4].c.b << dstfmt->Bshift);
			}
			else
			{	// RGBA surface handling

				// we do not want to blend with non-present pixels
				// (pixels that have alpha == 0) as these will
				// skew the result and make resulting alpha useless
				if (p0[4].c.a != 0)
				{
					int i;
					for (i = 0; i < 4; ++i)
						if (p0[i].c.a == 0)
							w0[i] = 0;

					p0[4].c.r = weight_product_8_4 (p0, 0, w0);
					p0[4].c.g = weight_product_8_4 (p0, 1, w0);
					p0[4].c.b = weight_product_8_4 (p0, 2, w0);
				}
				if (p1[4].c.a != 0)
				{
					int i;
					for (i = 0; i < 4; ++i)
						if (p1[i].c.a == 0)
							w1[i] = 0;

					p1[4].c.r = weight_product_8_4 (p1, 0, w1);
					p1[4].c.g = weight_product_8_4 (p1, 1, w1);
					p1[4].c.b = weight_product_8_4 (p1, 2, w1);
				}

				if (p0[4].c.a != 0 && p1[4].c.a != 0)
				{	// blend if both present
					p0[4].c.r = blend_ratio_2 (p0[4].c.r, p1[4].c.r, ratio);
					p0[4].c.g = blend_ratio_2 (p0[4].c.g, p1[4].c.g, ratio);
					p0[4].c.b = blend_ratio_2 (p0[4].c.b, p1[4].c.b, ratio);
				}
				else if (p1[4].c.a != 0)
				{	// other pixel is present
					p0[4].value = p1[4].value;
				}

				// error-correct alpha to fully opaque to remove
				// the often unwanted and unnecessary blending
				if (res_a > 0xf8)
					res_a = 0xff;

				*dst_p =
					(p0[4].c.r << dstfmt->Rshift) |
					(p0[4].c.g << dstfmt->Gshift) |
					(p0[4].c.b << dstfmt->Bshift) |
					(res_a << dstfmt->Ashift);
			}
		}
	}

	SDL_UnlockSurface(mm);
	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

void
TFB_DrawCanvas_Rescale_Bilinear (TFB_Canvas src_canvas, TFB_Canvas dst_canvas,
		int scale, HOT_SPOT* src_hs, EXTENT* size, HOT_SPOT* dst_hs)
{
	SDL_Surface *src = src_canvas;
	SDL_Surface *dst = dst_canvas;
	SDL_PixelFormat *srcfmt = src->format;
	SDL_PixelFormat *dstfmt = dst->format;
	SDL_Color *srcpal = srcfmt->palette? srcfmt->palette->colors : 0;
	const int sbpp = srcfmt->BytesPerPixel;
	const int slen = src->pitch;
	const int dst_has_alpha = (dstfmt->Amask != 0);
	const int transparent = (dst->flags & SDL_SRCCOLORKEY) ?
			dstfmt->colorkey : 0;
	const int alpha_threshold = dst_has_alpha ? 0 : 127;
	// source masks and keys
	Uint32 mk = 0, ck = ~0;
	// source fractional x and y positions
	int sx, sy;
	// source fractional dx and dy increments
	int fsx = 0, fsy = 0;
	// source fractional x and y starting points
	int ssx = 0, ssy = 0;
	int x, y, w, h;

	if (scale > 0)
	{
		// Use (scale / GSCALE_IDENTITY) sizing factor
		TFB_DrawCanvas_GetScaledExtent (src, src_hs, NULL, NULL, scale,
				TFB_SCALE_BILINEAR, size, dst_hs);

		w = size->width;
		h = size->height;
		fsx = (GSCALE_IDENTITY << 16) / scale;
		fsy = (GSCALE_IDENTITY << 16) / scale;

		// position the hotspots directly over each other
		ssx = (src_hs->x << 16) - fsx * dst_hs->x;
		ssy = (src_hs->y << 16) - fsy * dst_hs->y;
	}
	else
	{
		// Just go with the dst surface dimensions
		w = dst->w;
		h = dst->h;
		fsx = (src->w << 16) / w;
		fsy = (src->h << 16) / h;

		// give equal importance to both edges
		ssx = (((src->w - 1) << 16) - fsx * (w - 1)) >> 1;
		ssy = (((src->h - 1) << 16) - fsy * (h - 1)) >> 1;
	}

	if (w > dst->w || h > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Bilinear: "
				"Tried to scale image to size %d %d when dest_canvas"
				" has only dimensions of %d %d! Failing.",
				w, h, dst->w, dst->h);
		return;
	}

	if ((srcfmt->BytesPerPixel != 1 && srcfmt->BytesPerPixel != 4) ||
		(dst->format->BytesPerPixel != 4))
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rescale_Bilinear: "
				"Tried to deal with unknown BPP: %d -> %d",
				srcfmt->BitsPerPixel, dst->format->BitsPerPixel);
		return;
	}

	// use colorkeys where appropriate
	if (srcfmt->Amask)
	{	// alpha transparency
		mk = srcfmt->Amask;
		ck = 0;
	}
	else if (src->flags & SDL_SRCCOLORKEY)
	{	// colorkey transparency
		mk = ~srcfmt->Amask;
		ck = srcfmt->colorkey & mk;
	}

	SDL_LockSurface(src);
	SDL_LockSurface(dst);
	
	for (y = 0, sy = ssy; y < h; ++y, sy += fsy)
	{
		Uint32 *dst_p = (Uint32 *) ((Uint8*)dst->pixels + y * dst->pitch);
		const int py = (sy >> 16);
		Uint8 *src_a = (Uint8*)src->pixels + py * slen;
		// retrieve the fractional portions of y
		const Uint8 v = (sy >> 8) & 0xff;
		Uint8 weight[4]; // pixel weight vectors

		for (x = 0, sx = ssx; x < w; ++x, ++dst_p, sx += fsx)
		{
			const int px = (sx >> 16);
			// retrieve the fractional portions of x
			const Uint8 u = (sx >> 8) & 0xff;
			// pixels are examined and numbered in pattern
			//  0  1
			//  2  3
			// the ideal pixel (4) is somewhere between these four
			// and is calculated from these using weight vector (weight)
			// with a dot product
			pixel_t p[5];
			
			weight[0] = btable[255 - u][255 - v];
			weight[1] = btable[u][255 - v];
			weight[2] = btable[255 - u][v];
			weight[3] = btable[u][v];

			// Collect interesting pixels from src image
			// Optimization: speed is criticial on larger images;
			// most pixel reads fall completely inside the image
			if (px >= 0 && px + 1 < src->w && py >= 0 && py + 1 < src->h)
			{
				Uint8 *src_p = src_a + px * sbpp;

				p[0].value = scale_read_pixel (src_p, srcfmt, srcpal, mk, ck);
				p[1].value = scale_read_pixel (src_p + sbpp, srcfmt,
						srcpal, mk, ck);
				p[2].value = scale_read_pixel (src_p + slen, srcfmt,
						srcpal, mk, ck);
				p[3].value = scale_read_pixel (src_p + sbpp + slen, srcfmt,
						srcpal, mk, ck);
			}
			else
			{
				p[0].value = scale_get_pixel (src, mk, ck, px, py);
				p[1].value = scale_get_pixel (src, mk, ck, px + 1, py);
				p[2].value = scale_get_pixel (src, mk, ck, px, py + 1);
				p[3].value = scale_get_pixel (src, mk, ck, px + 1, py + 1);
			}

			p[4].c.a = dot_product_8_4 (p, 3, weight);
			
			if (p[4].c.a <= alpha_threshold)
			{
				*dst_p = transparent;
			}
			else if (!dst_has_alpha)
			{	// RGB surface handling
				p[4].c.r = dot_product_8_4 (p, 0, weight);
				p[4].c.g = dot_product_8_4 (p, 1, weight);
				p[4].c.b = dot_product_8_4 (p, 2, weight);

				// TODO: we should handle alpha-blending here, but we do
				//   not know the destination color for blending!

				*dst_p =
					(p[4].c.r << dstfmt->Rshift) |
					(p[4].c.g << dstfmt->Gshift) |
					(p[4].c.b << dstfmt->Bshift);
			}
			else
			{	// RGBA surface handling

				// we do not want to blend with non-present pixels
				// (pixels that have alpha == 0) as these will
				// skew the result and make resulting alpha useless
				int i;
				for (i = 0; i < 4; ++i)
					if (p[i].c.a == 0)
						weight[i] = 0;

				p[4].c.r = weight_product_8_4 (p, 0, weight);
				p[4].c.g = weight_product_8_4 (p, 1, weight);
				p[4].c.b = weight_product_8_4 (p, 2, weight);

				// error-correct alpha to fully opaque to remove
				// the often unwanted and unnecessary blending
				if (p[4].c.a > 0xf8)
					p[4].c.a = 0xff;

				*dst_p =
					(p[4].c.r << dstfmt->Rshift) |
					(p[4].c.g << dstfmt->Gshift) |
					(p[4].c.b << dstfmt->Bshift) |
					(p[4].c.a << dstfmt->Ashift);
			}
		}
	}

	SDL_UnlockSurface(dst);
	SDL_UnlockSurface(src);
}

void
TFB_DrawCanvas_Lock (TFB_Canvas canvas)
{
	SDL_Surface *surf = canvas;
	SDL_LockSurface (surf);
}

void
TFB_DrawCanvas_Unlock (TFB_Canvas canvas)
{
	SDL_Surface *surf = canvas;
	SDL_UnlockSurface (surf);
}

void
TFB_DrawCanvas_GetScreenFormat (TFB_PixelFormat *fmt)
{
	SDL_PixelFormat *sdl = SDL_Screen->format;
	
	if (sdl->palette)
	{
		log_add (log_Warning, "TFB_DrawCanvas_GetScreenFormat() WARNING:"
				"Paletted display format will be slow");

		fmt->BitsPerPixel = 32;
		fmt->Rmask = 0x000000ff;
		fmt->Gmask = 0x0000ff00;
		fmt->Bmask = 0x00ff0000;
		fmt->Amask = 0xff000000;
	}
	else
	{
		fmt->BitsPerPixel = sdl->BitsPerPixel;
		fmt->Rmask = sdl->Rmask;
		fmt->Gmask = sdl->Gmask;
		fmt->Bmask = sdl->Bmask;
		fmt->Amask = sdl->Amask;
	}
}

int
TFB_DrawCanvas_GetStride (TFB_Canvas canvas)
{
	SDL_Surface *surf = canvas;
	return surf->pitch;
}

void*
TFB_DrawCanvas_GetLine (TFB_Canvas canvas, int line)
{
	SDL_Surface *surf = canvas;
	return (uint8 *)surf->pixels + surf->pitch * line;
}

Color
TFB_DrawCanvas_GetPixel (TFB_Canvas canvas, int x, int y)
{
	SDL_Surface* surf = canvas;
	Uint32 pixel;
	GetPixelFn getpixel;
	Color c = {0, 0, 0, 0};

	if (x < 0 || x >= surf->w || y < 0 || y >= surf->h)
	{	// outside bounds, return 0
		return c;
	}

	SDL_LockSurface (surf);

	getpixel = getpixel_for(surf);
	pixel = (*getpixel)(surf, x, y);
	SDL_GetRGBA (pixel, surf->format, &c.r, &c.g, &c.b, &c.a);

	SDL_UnlockSurface (surf);

	return c;
}

void
TFB_DrawCanvas_Rotate (TFB_Canvas src_canvas, TFB_Canvas dst_canvas,
		int angle, EXTENT size)
{
	SDL_Surface *src = src_canvas;
	SDL_Surface *dst = dst_canvas;
	int ret;
	Color color;

	if (size.width > dst->w || size.height > dst->h) 
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rotate: Tried to rotate"
				" image to size %d %d when dst_canvas has only dimensions"
				" of %d %d! Failing.",
				size.width, size.height, dst->w, dst->h);
		return;
	}

	if (TFB_DrawCanvas_GetTransparentColor (src, &color))
	{
		TFB_DrawCanvas_SetTransparentColor (dst, color, FALSE);
		/* fill destination with transparent color before rotating */
		SDL_FillRect(dst, NULL, SDL_MapRGBA (dst->format,
				color.r, color.g, color.b, 0));
	}

	ret = rotateSurface (src, dst, angle, 0);
	if (ret != 0)
	{
		log_add (log_Warning, "TFB_DrawCanvas_Rotate: WARNING:"
				" actual rotation func returned failure\n");
	}
}

void
TFB_DrawCanvas_GetRotatedExtent (TFB_Canvas src_canvas, int angle, EXTENT *size)
{
	int dstw, dsth;
	SDL_Surface *src = src_canvas;
	
	rotozoomSurfaceSize (src->w, src->h, angle, 1, &dstw, &dsth);
	size->height = dsth;
	size->width = dstw;
}

void
TFB_DrawCanvas_CopyRect (TFB_Canvas source, const RECT *srcRect,
		TFB_Canvas target, POINT dstPt)
{
	SDL_Rect sourceRect, targetRect;

	if (source == 0 || target == 0)
	{
		log_add (log_Warning,
				"ERROR: TFB_DrawCanvas_CopyRect passed null canvas ptr");
		return;
	}

	sourceRect.x = srcRect->corner.x;
	sourceRect.y = srcRect->corner.y;
	sourceRect.w = srcRect->extent.width;
	sourceRect.h = srcRect->extent.height;

	targetRect.x = dstPt.x;
	targetRect.y = dstPt.y;
	// According to SDL docs, width and height are ignored, but
	// we'll set them anyway, just in case.
	targetRect.w = srcRect->extent.width;
	targetRect.h = srcRect->extent.height;
	
	SDL_BlitSurface (source, &sourceRect, target, &targetRect);
}

void
TFB_DrawCanvas_SetClipRect (TFB_Canvas canvas, const RECT *clipRect)
{
	if (canvas == 0)
	{
		log_add (log_Warning,
				"ERROR: TFB_DrawCanvas_SetClipRect passed null canvas ptr");
		return;
	}

	if (!clipRect)
	{	// clipping disabled
		SDL_SetClipRect (canvas, NULL);
	}
	else
	{
		SDL_Rect r;
		r.x = clipRect->corner.x;
		r.y = clipRect->corner.y;
		r.w = clipRect->extent.width;
		r.h = clipRect->extent.height;
		SDL_SetClipRect (canvas, &r);
	}
}

BOOLEAN
TFB_DrawCanvas_Intersect (TFB_Canvas canvas1, POINT c1org,
		TFB_Canvas canvas2, POINT c2org, const RECT *interRect)
{
	BOOLEAN ret = FALSE;
	SDL_Surface *surf1 = canvas1;
	SDL_Surface *surf2 = canvas2;
	int x, y;
	Uint32 s1key, s2key;
	Uint32 s1mask, s2mask;
	GetPixelFn getpixel1, getpixel2;

	SDL_LockSurface (surf1);
	SDL_LockSurface (surf2);

	getpixel1 = getpixel_for (surf1);
	getpixel2 = getpixel_for (surf2);

	if (surf1->format->Amask)
	{	// use alpha transparency info
		s1mask = surf1->format->Amask;
		// consider any not fully transparent pixel collidable
		s1key = 0;
	}
	else
	{	// colorkey transparency
		s1mask = ~surf1->format->Amask;
		s1key = surf1->format->colorkey & s1mask;
	}

	if (surf2->format->Amask)
	{	// use alpha transparency info
		s2mask = surf2->format->Amask;
		// consider any not fully transparent pixel collidable
		s2key = 0;
	}
	else
	{	// colorkey transparency
		s2mask = ~surf2->format->Amask;
		s2key = surf2->format->colorkey & s2mask;
	}

	// convert surface origins to pixel offsets within
	c1org.x = interRect->corner.x - c1org.x;
	c1org.y = interRect->corner.y - c1org.y;
	c2org.x = interRect->corner.x - c2org.x;
	c2org.y = interRect->corner.y - c2org.y;

	for (y = 0; y < interRect->extent.height; ++y)
	{
		for (x = 0; x < interRect->extent.width; ++x)
		{
			Uint32 p1 = getpixel1 (surf1, x + c1org.x, y + c1org.y) & s1mask;
			Uint32 p2 = getpixel2 (surf2, x + c2org.x, y + c2org.y) & s2mask;
			
			if (p1 != s1key && p2 != s2key)
			{	// pixel collision
				ret = TRUE;
				break;
			}
		}
	}

	SDL_UnlockSurface (surf2);
	SDL_UnlockSurface (surf1);

	return ret;
}

// Read/write the canvas pixels in a Color format understood by the core.
// The pixels array is assumed to be at least width * height large.
// The pixels array can be wider/narrower or taller/shorter than the canvas,
// and in that case, only the relevant pixels will be transfered.
static BOOLEAN
TFB_DrawCanvas_TransferColors (TFB_Canvas canvas, BOOLEAN write,
		Color *pixels, int width, int height)
{
	SDL_Surface *surf = canvas;
	SDL_PixelFormat *fmt;
	GetPixelFn getpix;
	PutPixelFn putpix;
	int x, y, w, h;

	if (canvas == 0)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_TransferColors "
				"passed null canvas");
		return FALSE;
	}

	fmt = surf->format;
	getpix = getpixel_for (surf);
	putpix = putpixel_for (surf);

	w = width < surf->w ? width : surf->w;
	h = height < surf->h ? height : surf->h;
	
	SDL_LockSurface (surf);

	// This could be done faster if we assumed 32bpp surfaces
	for (y = 0; y < h; ++y)
	{
		// pixels array pitch is width so as not to violate the interface
		Color *c = pixels + y * width;

		for (x = 0; x < w; ++x, ++c)
		{
			if (write)
			{	// writing from data to surface
				Uint32 p = SDL_MapRGBA (fmt, c->r, c->g, c->b, c->a);
				putpix (surf, x, y, p);
			}
			else
			{	// reading from surface to data
				Uint32 p = getpix (surf, x, y);
				SDL_GetRGBA (p, fmt, &c->r, &c->g, &c->b, &c->a);
			}
		}
	}

	SDL_UnlockSurface (surf);

	return TRUE;
}

// Read the canvas pixels in a Color format understood by the core.
// See TFB_DrawCanvas_TransferColors() for pixels array info
BOOLEAN
TFB_DrawCanvas_GetPixelColors (TFB_Canvas canvas, Color *pixels,
		int width, int height)
{
	return TFB_DrawCanvas_TransferColors (canvas, FALSE, pixels,
			width, height);
}

// Write the canvas pixels from a Color format understood by the core.
// See TFB_DrawCanvas_TransferColors() for pixels array info
BOOLEAN
TFB_DrawCanvas_SetPixelColors (TFB_Canvas canvas, const Color *pixels,
		int width, int height)
{
	// unconst pixels, but it is safe -- it will not be written to
	return TFB_DrawCanvas_TransferColors (canvas, TRUE, (Color *)pixels,
			width, height);
}

// Read/write the indexed canvas pixels as palette indexes.
// The data array is assumed to be at least width * height large.
// The data array can be wider/narrower or taller/shorter than the canvas,
// and in that case, only the relevant pixels will be transfered.
static BOOLEAN
TFB_DrawCanvas_TransferIndexes (TFB_Canvas canvas, BOOLEAN write,
		BYTE *data, int width, int height)
{
	SDL_Surface *surf = canvas;
	const SDL_PixelFormat *fmt;
	int y, w, h;

	if (canvas == 0)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_TransferIndexes "
				"passed null canvas");
		return FALSE;
	}
	fmt = surf->format;
	if (!TFB_DrawCanvas_IsPaletted (canvas) || fmt->BitsPerPixel != 8)
	{
		log_add (log_Warning, "ERROR: TFB_DrawCanvas_TransferIndexes "
				"unimplemeted function: not an 8bpp indexed canvas", fmt->BitsPerPixel);
		return FALSE;
	}

	w = width < surf->w ? width : surf->w;
	h = height < surf->h ? height : surf->h;

	SDL_LockSurface (surf);

	for (y = 0; y < h; ++y)
	{
		Uint8 *surf_p = (Uint8 *)surf->pixels + y * surf->pitch;
		// pixels array pitch is width so as not to violate the interface
		BYTE *data_p = data + y * width;

		if (write)
		{	// writing from data to surface
			memcpy (surf_p, data_p, w * sizeof (BYTE));
		}
		else
		{	// reading from surface to data
			memcpy (data_p, surf_p, w * sizeof (BYTE));
		}
	}

	SDL_UnlockSurface (surf);

	return TRUE;
}

// Read the indexed canvas pixels as palette indexes.
// See TFB_DrawCanvas_TransferIndexes() for data array info.
BOOLEAN
TFB_DrawCanvas_GetPixelIndexes (TFB_Canvas canvas, BYTE *data,
		int width, int height)
{
	return TFB_DrawCanvas_TransferIndexes (canvas, FALSE, data,
			width, height);
}

// Write the indexed canvas pixels as palette indexes.
// See TFB_DrawCanvas_TransferIndexes() for data array info.
BOOLEAN
TFB_DrawCanvas_SetPixelIndexes (TFB_Canvas canvas, const BYTE *data,
		int width, int height)
{
	// unconst data, but it is safe -- it will not be written to
	return TFB_DrawCanvas_TransferIndexes (canvas, TRUE, (BYTE *)data,
			width, height);
}
