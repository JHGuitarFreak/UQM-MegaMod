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

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"


// Scaler function lookup table
//
const Scale_FuncDef_t
Scale_C_Functions[] =
{
	{TFB_GFXFLAGS_SCALE_BILINEAR,   Scale_BilinearFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPT,    Scale_BiAdaptFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPTADV, Scale_BiAdaptAdvFilter},
	{TFB_GFXFLAGS_SCALE_TRISCAN,    Scale_TriScanFilter},
	{TFB_GFXFLAGS_SCALE_HQXX,       Scale_HqFilter},
	// Default
	{0,                             Scale_Nearest}
};

// See
//	nearest2x.c  -- Nearest Neighboor scaling
//	bilinear2x.c -- Bilinear scaling
//	biadv2x.c    -- Advanced Biadapt scaling
//	triscan2x.c  -- Triscan scaling

// Biadapt scaling to 2x
void
SCALE_(BiAdaptFilter) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h;
	int xend, yend;
	int dsrc, ddst;
	SDL_Rect *region = r;
	SDL_Rect limits;
	SDL_PixelFormat *fmt = dst->format;
	const int sp = src->pitch, dp = dst->pitch;
	const int bpp = fmt->BytesPerPixel;
	const int slen = sp / bpp, dlen = dp / bpp;
	Uint32 *src_p = (Uint32 *)src->pixels;
	Uint32 *dst_p = (Uint32 *)dst->pixels;
	Uint32 pixval_tl, pixval_tr, pixval_bl, pixval_br;

	// these macros are for clarity; they make the current pixel (0,0)
	// and allow to access pixels in all directions
	#define SRC(x, y)   (src_p + (x) + ((y) * slen))

	SCALE_(PlatInit) ();

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = src->w;
	limits.h = src->h;
	Scale_ExpandRect (region, 2, &limits);

	xend = region->x + region->w;
	yend = region->y + region->h;
	dsrc = slen - region->w;
	ddst = (dlen - region->w) * 2;

	// move ptrs to the first updated pixel
	src_p += slen * region->y + region->x;
	dst_p += (dlen * region->y + region->x) * 2;

	for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
	{
		for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
		{
			pixval_tl = SCALE_GETPIX (SRC (0, 0));
			
			SCALE_SETPIX (dst_p, pixval_tl);
			
			if (y + 1 < h)
			{
				// check pixel below the current one
				pixval_bl = SCALE_GETPIX (SRC (0, 1));

				if (pixval_tl == pixval_bl)
					SCALE_SETPIX (dst_p + dlen, pixval_tl);
				else
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							pixval_tl, pixval_bl)
							);
			}
			else
			{
				// last pixel in column - propagate
				SCALE_SETPIX (dst_p + dlen, pixval_tl);
				pixval_bl = pixval_tl;
			}
			++dst_p;

			if (x + 1 >= w)
			{
				// last pixel in row - propagate
				SCALE_SETPIX (dst_p, pixval_tl);

				if (pixval_tl == pixval_bl)
					SCALE_SETPIX (dst_p + dlen, pixval_tl);
				else
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							pixval_tl, pixval_bl)
							);
				continue;
			}
			
			// check pixel to the right from the current one
			pixval_tr = SCALE_GETPIX (SRC (1, 0));

			if (pixval_tl == pixval_tr)
				SCALE_SETPIX (dst_p, pixval_tr);
			else
				SCALE_SETPIX (dst_p, Scale_Blend_11 (
						pixval_tl, pixval_tr)
						);

			if (y + 1 >= h)
			{
				// last pixel in column - propagate
				SCALE_SETPIX (dst_p + dlen, pixval_tl);
				continue;
			}
			
			// check pixel to the bottom-right
			pixval_br = SCALE_GETPIX (SRC (1, 1));

			if (pixval_tl == pixval_br && pixval_tr == pixval_bl)
			{
				int cl, cr;
				Uint32 clr;

				if (pixval_tl == pixval_tr)
				{
					// all 4 are equal - propagate
					SCALE_SETPIX (dst_p + dlen, pixval_tl);
					continue;
				}

				// both pairs are equal, have to resolve the pixel
				// race; we try detecting which color is
				// the background by looking for a line or an edge
				// examine 8 pixels surrounding the current quad

				cl = cr = 1;

				if (x > 0)
				{
					clr = SCALE_GETPIX (SRC (-1, 0));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;

					clr = SCALE_GETPIX (SRC (-1, 1));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;
				}

				if (y > 0)
				{
					clr = SCALE_GETPIX (SRC (0, -1));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;

					clr = SCALE_GETPIX (SRC (1, -1));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;
				}

				if (x + 2 < w)
				{
					clr = SCALE_GETPIX (SRC (2, 0));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;

					clr = SCALE_GETPIX (SRC (2, 1));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;
				}

				if (y + 2 < h)
				{
					clr = SCALE_GETPIX (SRC (0, 2));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;

					clr = SCALE_GETPIX (SRC (1, 2));
					if (clr == pixval_tl)
						cl++;
					else if (clr == pixval_tr)
						cr++;
				}
				
				// least count wins
				if (cl > cr)
					SCALE_SETPIX (dst_p + dlen, pixval_tr);
				else if (cr > cl)
					SCALE_SETPIX (dst_p + dlen, pixval_tl);
				else
					SCALE_SETPIX (dst_p + dlen,
							Scale_Blend_11 (pixval_tl, pixval_tr));
			}
			else if (pixval_tl == pixval_br)
			{
				// main diagonal is same color
				// use its value
				SCALE_SETPIX (dst_p + dlen, pixval_tl);
			}
			else if (pixval_tr == pixval_bl)
			{
				// 2nd diagonal is same color
				// use its value
				SCALE_SETPIX (dst_p + dlen, pixval_tr);
			}
			else
			{
				// blend all 4
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_1111 (
						pixval_tl, pixval_bl, pixval_tr, pixval_br
						));
			}
		}
	}

	SCALE_(PlatDone) ();
}

