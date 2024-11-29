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

#include "port.h"
#include "sdl_common.h"
#include "primitives.h"
#include "uqm/units.h"


// Pixel drawing routines

static Uint32
getpixel_8(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x;
	return *p;
}

static void
putpixel_8(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 1;
	*p = pixel;
}

static Uint32
getpixel_16(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 2;
	return *(Uint16 *)p;
}

static void
putpixel_16(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 2;
	*(Uint16 *)p = pixel;
}

static Uint32
getpixel_24_be(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	return p[0] << 16 | p[1] << 8 | p[2];
}

static void
putpixel_24_be(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	p[0] = (pixel >> 16) & 0xff;
	p[1] = (pixel >> 8) & 0xff;
	p[2] = pixel & 0xff;
}

static Uint32
getpixel_24_le(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	return p[0] | p[1] << 8 | p[2] << 16;
}

static void
putpixel_24_le(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 3;
	p[0] = pixel & 0xff;
	p[1] = (pixel >> 8) & 0xff;
	p[2] = (pixel >> 16) & 0xff;
}

static Uint32
getpixel_32(SDL_Surface *surface, int x, int y)
{
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	return *(Uint32 *)p;
}

static void
putpixel_32(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * 4;
	*(Uint32 *)p = pixel;
}

GetPixelFn
getpixel_for (SDL_Surface *surface)
{
	int bpp = surface->format->BytesPerPixel;
	switch (bpp) {
	case 1:
		return &getpixel_8;
	case 2:
		return &getpixel_16;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			return &getpixel_24_be;
		} else {
			return &getpixel_24_le;
		}
	case 4:
		return &getpixel_32;
	}
	return NULL;
}

PutPixelFn
putpixel_for (SDL_Surface *surface)
{
	int bpp = surface->format->BytesPerPixel;
	switch (bpp) {
	case 1:
		return &putpixel_8;
	case 2:
		return &putpixel_16;
	case 3:
		if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
			return &putpixel_24_be;
		} else {
			return &putpixel_24_le;
		}
	case 4:
		return &putpixel_32;
	}
	return NULL;
}

static void
renderpixel_replace (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	(void) factor; // ignored
	putpixel_32 (surface, x, y, pixel);
}

static inline Uint8
clip_channel (int c)
{
	if (c < 0)
		c = 0;
	else if (c > 255)
		c = 255;
	return c;
}

static inline Uint8
modulated_sum (Uint8 dc, Uint8 sc, int factor)
{
	// We use >> 8 instead of / 255 because it is faster, but it does
	// not work 100% correctly. It should be safe because this should
	// not be called for factor==255
	int b = dc + ((sc * factor) >> 8);
	return clip_channel(b);
}

static inline Uint8
alpha_blend (Uint8 dc, Uint8 sc, int alpha)
{
	// We use >> 8 instead of / 255 because it is faster, but it does
	// not work 100% correctly. It should be safe because this should
	// not be called for alpha==255
	// No need to clip since we should never get values outside of 0..255
	// range, unless alpha is over 255, which is not supported.
	if (alpha == 0xff)
		return sc;
	else
		return (((sc - dc) * alpha) >> 8) + dc;
		
}

static inline Uint8
multiply_blend (Uint8 dc, Uint8 sc)
{	// Kruzen: custom blend for multiply
	return (sc * dc) >> 8;
}

static inline Uint8
overlay_blend (Uint8 dc, Uint8 sc)
{	// Custom "overlay" blend mode
	// Funky math to compensate for slow division by 127.5
	// Original formula copied from Wikipedia:
	// https://en.wikipedia.org/wiki/Blend_modes#Overlay

	if (dc < 128)
		return ((dc * sc) >> 7);
	else
		return clip_channel (((dc + sc) << 1) - 255 - ((dc * sc) >> 7));
}

static inline Uint8
screen_blend (Uint8 dc, Uint8 sc)
{	// Custom "screen" blend mode
	return (255 - (((255 - sc) * (255 - dc)) >> 8));
}

static inline Uint8
linburn_blend (Uint8 dc, Uint8 sc)
{	// Custom "linear burn" blend mode

	if (dc + sc < 0xff)
		return 0;
	else
		return dc + sc - 0xff;
}

// Assumes 8 bits/channel, a safe assumption for 32bpp surfaces
#define UNPACK_PIXEL_RGB(p, fmt, r, g, b) \
	do { \
		(r) = ((p) >> (fmt)->Rshift) & 0xff; \
		(g) = ((p) >> (fmt)->Gshift) & 0xff; \
		(b) = ((p) >> (fmt)->Bshift) & 0xff; \
	} while (0)

#define UNPACK_PIXEL_RGBA(p, fmt, r, g, b, a) \
	do { \
		(r) = ((p) >> (fmt)->Rshift) & 0xff; \
		(g) = ((p) >> (fmt)->Gshift) & 0xff; \
		(b) = ((p) >> (fmt)->Bshift) & 0xff; \
		(a) = ((p) >> (fmt)->Ashift) & 0xff; \
	} while (0)

// Assumes the channels already clipped to 8 bits
static inline Uint32
PACK_PIXEL_RGB (const SDL_PixelFormat *fmt,
		Uint8 r, Uint8 g, Uint8 b)
{
	return ((Uint32)r << fmt->Rshift) | ((Uint32)g << fmt->Gshift)
			| ((Uint32)b << fmt->Bshift);
}

static inline Uint32
PACK_PIXEL_RGBA (const SDL_PixelFormat *fmt,
		Uint8 r, Uint8 g, Uint8 b, Uint8 a)
{
	return ((Uint32)r << fmt->Rshift) | ((Uint32)g << fmt->Gshift)
			| ((Uint32)b << fmt->Bshift) | ((Uint32)a << fmt->Ashift);
}

static void
renderpixel_additive (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat *fmt = surface->format;
	Uint32 *p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;
	
	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	
	// TODO: We may need a special case for factor == -ADDITIVE_FACTOR_1 too,
	// but it is not important enough right now to care ;)
	if (factor == ADDITIVE_FACTOR_1)
	{	// no need to modulate the 'pixel', and modulation does not
		// work correctly with factor==255 anyway
		sr = clip_channel (sr + r);
		sg = clip_channel (sg + g);
		sb = clip_channel (sb + b);
	}
	else
	{
		sr = modulated_sum (sr, r, factor);
		sg = modulated_sum (sg, g, factor);
		sb = modulated_sum (sb, b, factor);
	}

	*p = PACK_PIXEL_RGB (fmt, sr, sg, sb);
}

static void
renderpixel_alpha (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat *fmt = surface->format;
	Uint32 *p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;
	
	if (factor == FULLY_OPAQUE_ALPHA)
	{	// alpha == 255 is equivalent to 'replace' and blending does not
		// work correctly anyway because we use >> 8 instead of / 255
		putpixel_32 (surface, x, y, pixel);
		return;
	}

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	sr = alpha_blend (sr, r, factor);
	sg = alpha_blend (sg, g, factor);
	sb = alpha_blend (sb, b, factor);
	*p = PACK_PIXEL_RGB (fmt, sr, sg, sb);
}

static void
renderpixel_multiply (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;

	(void) factor;// Doesn't support alpha

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	sr = multiply_blend (sr, r);
	sg = multiply_blend (sg, g);
	sb = multiply_blend (sb, b);
	*p = PACK_PIXEL_RGB (fmt, sr, sg, sb);
}

static void
renderpixel_overlay (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b, a;

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGBA (pixel, fmt, r, g, b, a);
	r = alpha_blend (sr, overlay_blend (sr, r), factor);
	g = alpha_blend (sg, overlay_blend (sg, g), factor);
	b = alpha_blend (sb, overlay_blend (sb, b), factor);
	*p = PACK_PIXEL_RGB (fmt, r, g, b);
}

static void
renderpixel_screen (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;

	(void) factor;// Doesn't support alpha

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	sr = screen_blend (sr, r);
	sg = screen_blend (sg, g);
	sb = screen_blend (sb, b);
	*p = PACK_PIXEL_RGB (fmt, sr, sg, sb);
}

static void
renderpixel_grayscale (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 avr;
	Uint8 sr, sg, sb;

	(void) pixel;

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);

	avr = overlay_blend ((((sr + sg + sb) * 341) >> 10), factor);
	*p = PACK_PIXEL_RGB (fmt, avr, avr, avr);
}

static void
renderpixel_linearburn (SDL_Surface* surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	r = alpha_blend (sr, linburn_blend (sr, r), factor);
	g = alpha_blend (sg, linburn_blend (sg, g), factor);
	b = alpha_blend (sb, linburn_blend (sb, b), factor);
	*p = PACK_PIXEL_RGB (fmt, r, g, b);
}

static void
renderpixel_desaturate (SDL_Surface *surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;
	int luma;

	(void) pixel;

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);

	luma = ((3 * sr + 6 * sg + sb) * 205) >> 11;
	r = clip_channel(sr + ((factor * (luma - sr)) >> 8));
	g = clip_channel(sg + ((factor * (luma - sg)) >> 8));
	b = clip_channel(sb + ((factor * (luma - sb)) >> 8));
	
	*p = PACK_PIXEL_RGB (fmt, r, g, b);
}

/* Kruzen: special instant blend to transform HD hyperspace ambience to quasispace one */
static void
renderpixel_hypertoquasi (SDL_Surface* surface, int x, int y, Uint32 pixel,
		int factor)
{
	const SDL_PixelFormat* fmt = surface->format;
	Uint32* p;
	Uint32 sp;
	Uint8 sr, sg, sb;
	int r, g, b;

	p = (Uint32 *) ((Uint8 *)surface->pixels + y * surface->pitch + x * 4);
	sp = *p;
	UNPACK_PIXEL_RGB (sp, fmt, sr, sg, sb);
	UNPACK_PIXEL_RGB (pixel, fmt, r, g, b);
	g = alpha_blend (sg, ((255 - (((r + g + b) * 341) >> 10)) * 0x78) >> 8, factor);
	r = alpha_blend (sr, 0, factor);
	b = alpha_blend (sb, 0, factor);
	*p = PACK_PIXEL_RGB (fmt, r, g, b);
}

RenderPixelFn
renderpixel_for (SDL_Surface *surface, RenderKind kind, BOOLEAN forMask)
{
	const SDL_PixelFormat *fmt = surface->format;
	// forMask ignores some older conditions

	// The only supported rendering is to 32bpp surfaces
	if (fmt->BytesPerPixel != 4 && !forMask)
		return NULL;

	// Rendering other than REPLACE is not supported on RGBA surfaces
	if (fmt->Amask != 0 && kind != renderReplace && !forMask)
		return NULL;

	switch (kind)
	{
	case renderReplace:
		return &renderpixel_replace;
	case renderAdditive:
		return &renderpixel_additive;
	case renderAlpha:
		return &renderpixel_alpha;
	case renderMultiply:
		return &renderpixel_multiply;
	case renderOverlay:
		return &renderpixel_overlay;
	case renderScreen:
		return &renderpixel_screen;
	case renderGrayscale:
		return &renderpixel_grayscale;
	case renderLinearburn:
		return &renderpixel_linearburn;
	case renderHypToQuas:
		return &renderpixel_hypertoquasi;
	case renderDesatur:
		return &renderpixel_desaturate;
	}
	// should not ever get here
	return NULL;
}

/* Line drawing routine
 * Adapted from Paul Heckbert's implementation of Bresenham's algorithm,
 * 3 Sep 85; taken from Graphics Gems I */

void
line_prim (int x1, int y1, int x2, int y2, Uint32 color, RenderPixelFn plot,
		int factor, SDL_Surface *dst)
{
	int d, x, y, ax, ay, sx, sy, dx, dy;
	SDL_Rect clip_r;

	SDL_GetClipRect (dst, &clip_r);
	if (!clip_line (&x1, &y1, &x2, &y2, &clip_r))
		return; // line is completely outside clipping rectangle

	dx = x2-x1;
	ax = ((dx < 0) ? -dx : dx) << 1;
	sx = (dx < 0) ? -1 : 1;
	dy = y2-y1;
	ay = ((dy < 0) ? -dy : dy) << 1;
	sy = (dy < 0) ? -1 : 1;

	x = x1;
	y = y1;
	if (ax > ay) {
		d = ay - (ax >> 1);
		for (;;) 
		{
			(*plot)(dst, x, y, color, factor);

			if (x == x2)
				return;
			if (d >= 0) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} else {
		d = ax - (ay >> 1);
		for (;;) 
		{
			(*plot)(dst, x, y, color, factor);

			if (y == y2)
				return;
			if (d >= 0) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}

void
line_aa_prim (int x1, int y1, int x2, int y2, Uint32 color, RenderPixelFn plot,
	int factor, SDL_Surface *dst, BYTE thickness)
{
	int d, x, y, ax, ay, sx, sy, dx, dy;
	SDL_Rect clip_r;

	SDL_GetClipRect (dst, &clip_r);
	if (!clip_line (&x1, &y1, &x2, &y2, &clip_r))
		return; // line is completely outside clipping rectangle

	clip_r.w = clip_r.h = thickness;

	dx = x2-x1;
	ax = ((dx < 0) ? -dx : dx) << 1;
	sx = (dx < 0) ? -1 : 1;
	dy = y2-y1;
	ay = ((dy < 0) ? -dy : dy) << 1;
	sy = (dy < 0) ? -1 : 1;

	x = x1;
	y = y1;
	if (ax > ay) 
	{
		d = ay - (ax >> 1);
		for (;;) 
		{
			clip_r.x = x;
			clip_r.y = y;

			fillrect_prim (clip_r, color, plot, factor, dst);
			
			if (x == x2)
				return;
			if (d >= 0) {
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	} 
	else 
	{
		d = ax - (ay >> 1);
		for (;;) 
		{
			clip_r.x = x;
			clip_r.y = y;

			fillrect_prim (clip_r, color, plot, factor, dst);
			
			if (y == y2)
				return;
			if (d >= 0) {
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}


// Clips line against rectangle using Cohen-Sutherland algorithm

enum {C_TOP = 0x1, C_BOTTOM = 0x2, C_RIGHT = 0x4, C_LEFT = 0x8};

static int
compute_code (float x, float y, float xmin, float ymin, float xmax, float ymax)
{
	int c = 0;
	if (y > ymax)
		c |= C_TOP;
	else if (y < ymin)
		c |= C_BOTTOM;
	if (x > xmax)
		c |= C_RIGHT;
	else if (x < xmin)
		c |= C_LEFT;
	return c;
}

int
clip_line (int *lx1, int *ly1, int *lx2, int *ly2, const SDL_Rect *r)
{
	int C0, C1, C;
	float x, y, x0, y0, x1, y1, xmin, ymin, xmax, ymax;

	x0 = (float)*lx1;
	y0 = (float)*ly1;
	x1 = (float)*lx2;
	y1 = (float)*ly2;

	xmin = (float)r->x;
	ymin = (float)r->y;
	xmax = (float)r->x + r->w - 1;
	ymax = (float)r->y + r->h - 1;

	C0 = compute_code (x0, y0, xmin, ymin, xmax, ymax);
	C1 = compute_code (x1, y1, xmin, ymin, xmax, ymax);

	for (;;) {
		/* trivial accept: both ends in rectangle */
		if ((C0 | C1) == 0)
		{
			*lx1 = (int)x0;
			*ly1 = (int)y0;
			*lx2 = (int)x1;
			*ly2 = (int)y1;
			return 1;
		}

		/* trivial reject: both ends on the external side of the rectangle */
		if ((C0 & C1) != 0)
			return 0;

		/* normal case: clip end outside rectangle */
		C = C0 ? C0 : C1;
		if (C & C_TOP)
		{
			x = x0 + (x1 - x0) * (ymax - y0) / (y1 - y0);
			y = ymax;
		}
		else if (C & C_BOTTOM)
		{
			x = x0 + (x1 - x0) * (ymin - y0) / (y1 - y0);
			y = ymin;
		}
		else if (C & C_RIGHT)
		{
			x = xmax;
			y = y0 + (y1 - y0) * (xmax - x0) / (x1 - x0);
		}
		else
		{
			x = xmin;
			y = y0 + (y1 - y0) * (xmin - x0) / (x1 - x0);
		}

		/* set new end point and iterate */
		if (C == C0)
		{
			x0 = x; y0 = y;
			C0 = compute_code (x0, y0, xmin, ymin, xmax, ymax);
		} 
		else
		{
			x1 = x; y1 = y;
			C1 = compute_code (x1, y1, xmin, ymin, xmax, ymax);
		}
	}
}

void
fillrect_prim(SDL_Rect r, Uint32 color, RenderPixelFn plot, int factor,
		SDL_Surface *dst)
{
	int x, y;
	int x1, y1;
	SDL_Rect clip_r;

	SDL_GetClipRect (dst, &clip_r);
	if (!clip_rect (&r, &clip_r))
		return; // rect is completely outside clipping rectangle

	// TODO: calculate destination pointer directly instead of
	//   using the plot(x,y) version
	x1 = r.x + r.w;
	y1 = r.y + r.h;
	for (y = r.y; y < y1; ++y)
	{
		for (x = r.x; x < x1; ++x)
			plot(dst, x, y, color, factor);
	}
}

// clip the rectangle against the clip rectangle
int
clip_rect(SDL_Rect *r, const SDL_Rect *clip_r)
{
	// NOTE: the following clipping code is copied in part
	//   from SDL-1.2.4 sources
	int dx, dy;
	int w = r->w;
	int h = r->h;
			// SDL_Rect.w and .h are unsigned, we need signed

	dx = clip_r->x - r->x;
	if (dx > 0)
	{
		w -= dx;
		r->x += dx;
	}
	dx = r->x + w - clip_r->x - clip_r->w;
	if (dx > 0)
		w -= dx;

	dy = clip_r->y - r->y;
	if (dy > 0)
	{
		h -= dy;
		r->y += dy;
	}
	dy = r->y + h - clip_r->y - clip_r->h;
	if (dy > 0)
		h -= dy;

	if (w <= 0 || h <= 0)
	{
		r->w = 0;
		r->h = 0;
		return 0;
	}

	r->w = w;
	r->h = h;
	return 1;
}

void
blt_prim (SDL_Surface *src, SDL_Rect src_r, RenderPixelFn plot, int factor,
			SDL_Surface *dst, SDL_Rect dst_r)
{
	SDL_PixelFormat *srcfmt = src->format;
	SDL_Palette *srcpal = srcfmt->palette;
	SDL_PixelFormat *dstfmt = dst->format;
	Uint32 mask = 0;
	Uint32 key = ~0;
	GetPixelFn getpix = getpixel_for(src);
	SDL_Rect clip_r;
	int x, y;

	SDL_GetClipRect (dst, &clip_r);
	if (!clip_blt_rects (&src_r, &dst_r, &clip_r))
		return; // rect is completely outside clipping rectangle

	if (src_r.x >= src->w || src_r.y >= src->h)
		return; // rect is completely outside source bounds

	if (src_r.x + src_r.w > src->w)
		src_r.w = src->w - src_r.x;
	if (src_r.y + src_r.h > src->h)
		src_r.h = src->h - src_r.y;

	// use colorkeys where appropriate
	if (srcfmt->Amask)
	{	// alpha transparency
		mask = srcfmt->Amask;
		key = 0;
	}
	else if (TFB_GetColorKey (src, &key) == 0)
	{
		mask = ~0;
	}
	// TODO: calculate the source and destination pointers directly
	//   instead of using the plot(x,y) version
	for (y = 0; y < src_r.h; ++y)
	{
		for (x = 0; x < src_r.w; ++x)
		{
			Uint8 r, g, b, a;
			Uint32 p;
			
			p = getpix(src, src_r.x + x, src_r.y + y);
			if (srcpal)
			{	// source is paletted, colorkey does not use mask
				if (p == key)
					continue; // transparent pixel
			}
			else
			{	// source is RGB(A), colorkey uses mask
				if ((p & mask) == key)
					continue; // transparent pixel
			}

			// convert pixel format to destination
			SDL_GetRGBA (p, srcfmt, &r, &g, &b, &a);
			// TODO: handle source pixel alpha; plot() should probably
			//   get a source alpha parameter
			// Kruzen: Done via TRANSFER_ALPHA flag, although a bit ugly
			p = SDL_MapRGBA (dstfmt, r, g, b, a);

			plot(dst, dst_r.x + x, dst_r.y + y, p, factor == TRANSFER_ALPHA ? a : factor);
		}
	}
}

// Kruzen: Special blits to permanently transform base image. To restore it - unload it from RAM and load again
void
blt_filtered_prim (SDL_Surface *layer, RenderPixelFn plot, int factor,
			SDL_Surface *base, Color *fill)
{
	SDL_PixelFormat *lrfmt = layer->format;
	SDL_PixelFormat *bsfmt = base->format;
	GetPixelFn getpix;
	Uint32 color = 0;
	int x, y;

	// Cannot process surfaces of different formats
	if (lrfmt->BytesPerPixel != bsfmt->BytesPerPixel)
		return;
	else
		getpix = getpixel_for (layer);// For both layer and base

	// Not for paletted yet!
	if (lrfmt->palette)
		return;

	if (fill)
		color = SDL_MapRGB (bsfmt, fill->r, fill->g, fill->b);

	for (y = 0; y < base->h; ++y)
	{
		for (x = 0; x < base->w; ++x)
		{
			Uint8 al, ab;
			Uint32 lp;
			Uint32 *bp;
			
			lp = getpix (layer, x, y);
			bp = (Uint32 *) ((Uint8 *)base->pixels + y * base->pitch + x * 4);
			
			if ((lp & lrfmt->Amask) == 0 || (*bp & bsfmt->Amask) == 0)
				continue; // transparent pixel

			al = (lp >> (lrfmt->Ashift)) & 0xFF;
			ab = (*bp >> (bsfmt->Ashift)) & 0xFF;

			plot (base, x, y, fill ? color : lp, 
					factor == TRANSFER_ALPHA ? al : factor);

			// Reapply alpha to pixel since every plot function nukes it
			*bp &= ~(bsfmt->Amask);
			*bp |= ((Uint32)ab << (bsfmt->Ashift));
		}
	}
}

void
blt_filtered_fill (SDL_Surface *base, RenderPixelFn plot, int factor,
			Color *fill)
{
	SDL_PixelFormat *fmt = base->format;
	int x, y;
	Uint32 color;

	// Not for paletted yet!
	if (fmt->palette)
		return;

	color = SDL_MapRGB (fmt, fill->r, fill->g, fill->b);

	for (y = 0; y < base->h; ++y)
	{
		for (x = 0; x < base->w; ++x)
		{
			Uint8 a;
			Uint32 *p;
			
			p = (Uint32 *) ((Uint8 *)base->pixels + y * base->pitch + x * 4);
			
			if ((*p & fmt->Amask) == 0)
				continue; // transparent pixel

			a = (*p >> (fmt->Ashift)) & 0xFF;			

			plot (base, x, y, color, factor);

			// Reapply alpha to pixel since every plot function nukes it
			*p &= ~(fmt->Amask);
			*p |= ((Uint32)a << (fmt->Ashift));
		}
	}
}

void
blt_filtered_pal (SDL_Surface *layer, SDL_Surface *base, Color *fill)
{
	SDL_PixelFormat *lrfmt = layer->format;
	SDL_PixelFormat *bsfmt = base->format;
	int x, y;	

	// Wrong formats
	if (!lrfmt->palette || bsfmt->BytesPerPixel != 4)
		return;

	for (y = 0; y < base->h; ++y)
	{
		for (x = 0; x < base->w; ++x)
		{
			Uint8 ab;
			Uint8 *lp;
			Uint32 *bp;
			
			bp = (Uint32 *) ((Uint8 *)base->pixels + y * base->pitch + x * 4);
			
			if ((*bp & bsfmt->Amask) == 0)
				continue; // transparent pixel

			lp = (Uint8 *)layer->pixels + y * layer->pitch + x;

			ab = (*bp >> (bsfmt->Ashift)) & 0xFF;

			*bp = PACK_PIXEL_RGBA (bsfmt, fill[*lp].r, fill[*lp].g, fill[*lp].b, ab);
		}
	}
}

// clip the source and destination rectangles against the clip rectangle
int
clip_blt_rects(SDL_Rect *src_r, SDL_Rect *dst_r, const SDL_Rect *clip_r)
{
	// NOTE: the following clipping code is copied in part
	//   from SDL-1.2.4 sources
	int w, h;
	int dx, dy;

	// clip the source rectangle to the source surface
	w = src_r->w;
	if (src_r->x < 0) 
	{
		w += src_r->x;
		dst_r->x -= src_r->x;
		src_r->x = 0;
	}

	h = src_r->h;
	if (src_r->y < 0) 
	{
		h += src_r->y;
		dst_r->y -= src_r->y;
		src_r->y = 0;
	}

	// clip the destination rectangle against the clip rectangle,
	// minding the source rectangle in the process
	dx = clip_r->x - dst_r->x;
	if (dx > 0)
	{
		w -= dx;
		dst_r->x += dx;
		src_r->x += dx;
	}
	dx = dst_r->x + w - clip_r->x - clip_r->w;
	if (dx > 0)
		w -= dx;

	dy = clip_r->y - dst_r->y;
	if (dy > 0)
	{
		h -= dy;
		dst_r->y += dy;
		src_r->y += dy;
	}
	dy = dst_r->y + h - clip_r->y - clip_r->h;
	if (dy > 0)
		h -= dy;

	if (w <= 0 || h <= 0)
	{
		src_r->w = 0;
		src_r->h = 0;
		return 0;
	}

	src_r->w = w;
	src_r->h = h;
	return 1;
}

