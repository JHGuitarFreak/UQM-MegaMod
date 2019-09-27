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

// Core algorithm of the Triscan screen scaler (based on Scale2x)
//     (for scale2x please see http://scale2x.sf.net)
//  Template
//    When this file is built standalone is produces a plain C version
//    Also #included by 2xscalers_mmx.c for an MMX version

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"


// Triscan scaling to 2x
//	derivative of scale2x -- scale2x.sf.net
// The name expands to either
//		Scale_TriScanFilter (for plain C) or
//		Scale_MMX_TriScanFilter (for MMX)
//		[others when platforms are added]
void
SCALE_(TriScanFilter) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
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
	Uint32 pixels[3][3];
	Uint32 *src_p = (Uint32 *)src->pixels;
	Uint32 *dst_p = (Uint32 *)dst->pixels;

	int prevline, nextline;

	// these macros are for clarity; they make the current pixel (0,0)
	// and allow to access pixels in all directions
	#define PIX(x, y)   (pixels[1 + (x)][1 + (y)])

	#define TRISCAN_YUV_MED     100
	// medium tolerance pixel comparison
	#define TRISCAN_CMPYUV(p1, p2) \
			(PIX p1 == PIX p2 || SCALE_CMPYUV (PIX p1, PIX p2, TRISCAN_YUV_MED))


	SCALE_(PlatInit) ();

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = src->w;
	limits.h = src->h;
	Scale_ExpandRect (region, 1, &limits);

	xend = region->x + region->w;
	yend = region->y + region->h;
	dsrc = slen - region->w;
	ddst = (dlen - region->w) * 2;

	// move ptrs to the first updated pixel
	src_p += slen * region->y + region->x;
	dst_p += (dlen * region->y + region->x) * 2;

	for (y = region->y; y < yend; ++y, dst_p += ddst, src_p += dsrc)
	{
		if (y > 0)
			prevline = -slen;
		else
			prevline = 0;

		if (y < h - 1)
			nextline = slen;
		else
			nextline = 0;
	
		// prime the (tiny) sliding-window pixel arrays
		PIX( 1,  0) = src_p[0];

		if (region->x > 0)
			PIX( 0,  0) = src_p[-1];
		else
			PIX( 0,  0) = PIX( 1,  0);

		for (x = region->x; x < xend; ++x, ++src_p, dst_p += 2)
		{
			// slide the window
			PIX(-1,  0) = PIX( 0,  0);

			PIX( 0, -1) = src_p[prevline];
			PIX( 0,  0) = PIX( 1,  0);
			PIX( 0,  1) = src_p[nextline];

			if (x < w - 1)
				PIX( 1,  0) = src_p[1];
			else
				PIX( 1,  0) = PIX( 0,  0);
			
			if (!TRISCAN_CMPYUV (( 0, -1), ( 0,  1)) &&
				!TRISCAN_CMPYUV ((-1,  0), ( 1,  0)))
			{
				if (TRISCAN_CMPYUV ((-1,  0), ( 0, -1)))
					dst_p[0] = Scale_Blend_11 (PIX(-1, 0), PIX(0, -1));
				else
					dst_p[0] = PIX(0, 0);

				if (TRISCAN_CMPYUV (( 1,  0), ( 0, -1)))
					dst_p[1] = Scale_Blend_11 (PIX(1, 0), PIX(0, -1));
				else
					dst_p[1] = PIX(0, 0);

				if (TRISCAN_CMPYUV ((-1,  0), ( 0,  1)))
					dst_p[dlen] = Scale_Blend_11 (PIX(-1, 0), PIX(0, 1));
				else
					dst_p[dlen] = PIX(0, 0);

				if (TRISCAN_CMPYUV (( 1,  0), ( 0,  1)))
					dst_p[dlen+1] = Scale_Blend_11 (PIX(1, 0), PIX(0, 1));
				else
					dst_p[dlen+1] = PIX(0, 0);
			}
			else
			{
				dst_p[0]      = PIX(0, 0);
				dst_p[1]      = PIX(0, 0);
				dst_p[dlen]   = PIX(0, 0);
				dst_p[dlen+1] = PIX(0, 0);
			}
		}
	}

	SCALE_(PlatDone) ();
}

