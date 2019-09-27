/*
 * Portions Copyright (C) 2003-2005  Alex Volkov (codepro@usa.net)
 *
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

// Core algorithm of the Advanced BiAdaptive screen scaler
//  Template
//    When this file is built standalone is produces a plain C version
//    Also #included by 2xscalers_mmx.c for an MMX version

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"


// Advanced biadapt scaling to 2x
// The name expands to either
//		Scale_BiAdaptAdvFilter (for plain C) or
//		Scale_MMX_BiAdaptAdvFilter (for MMX)
//		[others when platforms are added]
void
SCALE_(BiAdaptAdvFilter) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
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
	// for clarity purposes, the 'pixels' array here is transposed
	Uint32 pixels[4][4];
	static int resolve_coord[][2] = 
	{
		{0, -1}, {1, -1}, { 2, 0}, { 2, 1},
		{1,  2}, {0,  2}, {-1, 1}, {-1, 0},
		{100, 100}	// term
	};
	Uint32 *src_p = (Uint32 *)src->pixels;
	Uint32 *dst_p = (Uint32 *)dst->pixels;

	// these macros are for clarity; they make the current pixel (0,0)
	// and allow to access pixels in all directions
	#define PIX(x, y)   (pixels[1 + (x)][1 + (y)])
	#define SRC(x, y)   (src_p + (x) + ((y) * slen))
	// commonly used operations, for clarity also
	// others are defined at their respective bpp levels
	#define BIADAPT_RGBHIGH   8000
	#define BIADAPT_YUVLOW      30
	#define BIADAPT_YUVMED      70
	#define BIADAPT_YUVHIGH    130

	// high tolerance pixel comparison
	#define BIADAPT_CMPRGB_HIGH(p1, p2) \
			(p1 == p2 || SCALE_CMPRGB (p1, p2) <= BIADAPT_RGBHIGH)

	// low tolerance pixel comparison
	#define BIADAPT_CMPYUV_LOW(p1, p2) \
			(p1 == p2 || SCALE_CMPYUV (p1, p2, BIADAPT_YUVLOW))
	// medium tolerance pixel comparison
	#define BIADAPT_CMPYUV_MED(p1, p2) \
			(p1 == p2 || SCALE_CMPYUV (p1, p2, BIADAPT_YUVMED))
	// high tolerance pixel comparison
	#define BIADAPT_CMPYUV_HIGH(p1, p2) \
			(p1 == p2 || SCALE_CMPYUV (p1, p2, BIADAPT_YUVHIGH))

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

	#define SCALE_GETPIX(p)        ( *(Uint32 *)(p) )
	#define SCALE_SETPIX(p, c)     ( *(Uint32 *)(p) = (c) )

	// move ptrs to the first updated pixel
	src_p += slen * region->y + region->x;
	dst_p += (dlen * region->y + region->x) * 2;

	for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
	{
		for (x = region->x; x < xend; ++x, ++src_p, ++dst_p)
		{
			// pixel equality counter
			int cmatch;

			// most pixels will fall into 'all 4 equal'
			// pattern, so we check it first
			cmatch = 0;

			PIX (0, 0) = SCALE_GETPIX (SRC (0, 0));
			
			SCALE_SETPIX (dst_p, PIX (0, 0));

			if (y + 1 < h)
			{
				// check pixel below the current one
				PIX (0, 1) = SCALE_GETPIX (SRC (0, 1));

				if (PIX (0, 0) == PIX (0, 1))
				{
					SCALE_SETPIX (dst_p + dlen, PIX (0, 0));
					cmatch |= 1;
				}
			}
			else
			{
				// last pixel in column - propagate
				PIX (0, 1) = PIX (0, 0);
				SCALE_SETPIX (dst_p + dlen, PIX (0, 0));
				cmatch |= 1;
				
			}

			if (x + 1 < w)
			{
				// check pixel to the right from the current one
				PIX (1, 0) = SCALE_GETPIX (SRC (1, 0));

				if (PIX (0, 0) == PIX (1, 0))
				{
					SCALE_SETPIX (dst_p + 1, PIX (0, 0));
					cmatch |= 2;
				}
			}
			else
			{
				// last pixel in row - propagate
				PIX (1, 0) = PIX (0, 0);
				SCALE_SETPIX (dst_p + 1, PIX (0, 0));
				cmatch |= 2;
			}

			if (cmatch == 3)
			{
				if (y + 1 >= h || x + 1 >= w)
				{
					// last pixel in row/column and nearest
					// neighboor is identical
					dst_p++;
					SCALE_SETPIX (dst_p + dlen, PIX (0, 0));
					continue;
				}

				// check pixel to the bottom-right
				PIX (1, 1) = SCALE_GETPIX (SRC (1, 1));

				if (PIX (0, 0) == PIX (1, 1))
				{
					// all 4 are equal - propagate
					dst_p++;
					SCALE_SETPIX (dst_p + dlen, PIX (0, 0));
					continue;
				}
			}

			// some neighboors are different, lets check them

			if (x > 0)
				PIX (-1, 0) = SCALE_GETPIX (SRC (-1, 0));
			else
				PIX (-1, 0) = PIX (0, 0);

			if (x + 2 < w)
				PIX (2, 0) = SCALE_GETPIX (SRC (2, 0));
			else
				PIX (2, 0) = PIX (1, 0);
			
			if (y + 1 < h)
			{
				if (x > 0)
					PIX (-1, 1) = SCALE_GETPIX (SRC (-1, 1));
				else
					PIX (-1, 1) = PIX (0, 1);

				if (x + 2 < w)
				{
					PIX (1, 1) = SCALE_GETPIX (SRC (1, 1));
					PIX (2, 1) = SCALE_GETPIX (SRC (2, 1));
				}
				else if (x + 1 < w)
				{
					PIX (1, 1) = SCALE_GETPIX (SRC (1, 1));
					PIX (2, 1) = PIX (1, 1);
				}
				else
				{
					PIX (1, 1) = PIX (0, 1);
					PIX (2, 1) = PIX (0, 1);
				}
			}
			else
			{
				// last pixel in column
				PIX (-1, 1) = PIX (-1, 0);
				PIX (1, 1) = PIX (1, 0);
				PIX (2, 1) = PIX (2, 0);
			}

			if (y + 2 < h)
			{
				PIX (0, 2) = SCALE_GETPIX (SRC (0, 2));

				if (x > 0)
					PIX (-1, 2) = SCALE_GETPIX (SRC (-1, 2));
				else
					PIX (-1, 2) = PIX (0, 2);

				if (x + 2 < w)
				{
					PIX (1, 2) = SCALE_GETPIX (SRC (1, 2));
					PIX (2, 2) = SCALE_GETPIX (SRC (2, 2));
				}
				else if (x + 1 < w)
				{
					PIX (1, 2) = SCALE_GETPIX (SRC (1, 2));
					PIX (2, 2) = PIX (1, 2);
				}
				else
				{
					PIX (1, 2) = PIX (0, 2);
					PIX (2, 2) = PIX (0, 2);
				}
			}
			else
			{
				// last pixel in column
				PIX (-1, 2) = PIX (-1, 1);
				PIX (0, 2) = PIX (0, 1);
				PIX (1, 2) = PIX (1, 1);
				PIX (2, 2) = PIX (2, 1);
			}

			if (y > 0)
			{
				PIX (0, -1) = SCALE_GETPIX (SRC (0, -1));

				if (x > 0)
					PIX (-1, -1) = SCALE_GETPIX (SRC (-1, -1));
				else
					PIX (-1, -1) = PIX (0, -1);

				if (x + 2 < w)
				{
					PIX (1, -1) = SCALE_GETPIX (SRC (1, -1));
					PIX (2, -1) = SCALE_GETPIX (SRC (2, -1));
				}
				else if (x + 1 < w)
				{
					PIX (1, -1) = SCALE_GETPIX (SRC (1, -1));
					PIX (2, -1) = PIX (1, -1);
				}
				else
				{
					PIX (1, -1) = PIX (0, -1);
					PIX (2, -1) = PIX (0, -1);
				}
			}
			else
			{
				PIX (-1, -1) = PIX (-1, 0);
				PIX (0, -1) = PIX (0, 0);
				PIX (1, -1) = PIX (1, 0);
				PIX (2, -1) = PIX (2, 0);
			}

			// check pixel below the current one
			if (!(cmatch & 1))
			{
				if (SCALE_CMPYUV (PIX (0, 0), PIX (0, 1), BIADAPT_YUVLOW))
				{
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							PIX (0, 0), PIX (0, 1))
							);
					cmatch |= 1;
				}
				// detect a 2:1 line going across the current pixel
				else if ( (PIX (0, 0) == PIX (-1, 0)
						&& PIX (0, 0) == PIX (1, 1)
						&& PIX (0, 0) == PIX (2, 1) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 2))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) ||

						(PIX (0, 0) == PIX (1, 0)
						&& PIX (0, 0) == PIX (-1, 1)
						&& PIX (0, 0) == PIX (2, -1) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 2))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 0))))) )
				{
					SCALE_SETPIX (dst_p + dlen, PIX (0, 0));
				}
				// detect a 2:1 line going across the pixel below current
				else if ( (PIX (0, 1) == PIX (-1, 0)
						&& PIX (0, 1) == PIX (1, 1)
						&& PIX (0, 1) == PIX (2, 2) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 2))))) ||

						(PIX (0, 1) == PIX (1, 0)
						&& PIX (0, 1) == PIX (-1, 1)
						&& PIX (0, 1) == PIX (2, 0) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, -1))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (-1, 2))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (0, 2))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 1), PIX (2, 1))))) )
				{
					SCALE_SETPIX (dst_p + dlen, PIX (0, 1));
				}
				else
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							PIX (0, 0), PIX (0, 1))
							);
			}

			dst_p++;

			// check pixel to the right from the current one
			if (!(cmatch & 2))
			{
				if (SCALE_CMPYUV (PIX (0, 0), PIX (1, 0), BIADAPT_YUVLOW))
				{
					SCALE_SETPIX (dst_p, Scale_Blend_11 (
							PIX (0, 0), PIX (1, 0))
							);
					cmatch |= 2;
				}
				// detect a 1:2 line going across the current pixel
				else if ( (PIX (0, 0) == PIX (1, -1)
						&& PIX (0, 0) == PIX (0, 1)
						&& PIX (0, 0) == PIX (-1, 2) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 1))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))))) ||

						(PIX (0, 0) == PIX (0, -1)
						&& PIX (0, 0) == PIX (1, 1)
						&& PIX (0, 0) == PIX (1, 2) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (-1, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (0, 2))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (0, 0), PIX (2, 2))))) )

				{
					SCALE_SETPIX (dst_p, PIX (0, 0));
				}
				// detect a 1:2 line going across the pixel to the right
				else if ( (PIX (1, 0) == PIX (1, -1)
						&& PIX (1, 0) == PIX (0, 1)
						&& PIX (1, 0) == PIX (0, 2) &&

						((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, 2))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))))) ||
						
						(PIX (1, 0) == PIX (0, -1)
						&& PIX (1, 0) == PIX (1, 1)
						&& PIX (1, 0) == PIX (2, 2) &&
						
						((!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (-1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (0, 1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, 2))) ||
						(!BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (1, -1))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 0))
						&& !BIADAPT_CMPRGB_HIGH (PIX (1, 0), PIX (2, 1))))) )
				{
					SCALE_SETPIX (dst_p, PIX (1, 0));
				}
				else
					SCALE_SETPIX (dst_p, Scale_Blend_11 (
							PIX (0, 0), PIX (1, 0))
							);
			}

			if (PIX (0, 0) == PIX (1, 1) && PIX (1, 0) == PIX (0, 1))
			{
				// diagonals are equal
				int *coord;
				int cl, cr;
				Uint32 clr;

				// both pairs are equal, have to resolve the pixel
				// race; we try detecting which color is
				// the background by looking for a line or an edge
				// examine 8 pixels surrounding the current quad

				cl = cr = 2;
				for (coord = resolve_coord[0]; *coord < 100; coord += 2)
				{
					clr = PIX (coord[0], coord[1]);

					if (BIADAPT_CMPYUV_MED (clr, PIX (0, 0)))
						cl++;
					else if (BIADAPT_CMPYUV_MED (clr, PIX (1, 0)))
						cr++;
				}

				// least count wins
				if (cl > cr)
					clr = PIX (1, 0);
				else if (cr > cl)
					clr = PIX (0, 0);
				else
					clr = Scale_Blend_11 (PIX (0, 0), PIX (1, 0));

				SCALE_SETPIX (dst_p + dlen, clr);
				continue;
			}

			if (cmatch == 3
					|| (BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (0, 1))
					&& BIADAPT_CMPYUV_LOW (PIX (1, 0), PIX (1, 1))))
			{
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
						PIX (0, 1), PIX (1, 0))
						);
				continue;
			}
			else if (cmatch && BIADAPT_CMPYUV_LOW (PIX (0, 0), PIX (1, 1)))
			{
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
						PIX (0, 0), PIX (1, 1))
						);
				continue;
			}

			// check pixel to the bottom-right
			if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1))
					&& BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
			{
				if (SCALE_GETY (PIX (0, 0)) > SCALE_GETY (PIX (1, 0)))
				{
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							PIX (0, 0), PIX (1, 1))
							);
				}
				else
				{
					SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
							PIX (1, 0), PIX (0, 1))
							);
				}
			}
			else if (BIADAPT_CMPYUV_HIGH (PIX (0, 0), PIX (1, 1)))
			{
				// main diagonal is same color
				// use its value
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
						PIX (0, 0), PIX (1, 1))
						);
			}
			else if (BIADAPT_CMPYUV_HIGH (PIX (1, 0), PIX (0, 1)))
			{
				// 2nd diagonal is same color
				// use its value
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_11 (
						PIX (1, 0), PIX (0, 1))
						);
			}
			else
			{
				// blend all 4
				SCALE_SETPIX (dst_p + dlen, Scale_Blend_1111 (
						PIX (0, 0), PIX (0, 1),
						PIX (1, 0), PIX (1, 1)
						));
			}
		}
	}

	SCALE_(PlatDone) ();
}

