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

// Core algorithm of the BiLinear screen scaler
//  Template
//    When this file is built standalone is produces a plain C version
//    Also #included by 2xscalers_mmx.c for an MMX version

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"


// Bilinear scaling to 2x
// The name expands to either
//		Scale_BilinearFilter (for plain C) or
//		Scale_MMX_BilinearFilter (for MMX)
//		Scale_SSE_BilinearFilter (for SSE)
//		[others when platforms are added]
void
SCALE_(BilinearFilter) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int x, y;
	const int w = src->w, h = src->h;
	int xend, yend;
	int dsrc, ddst;
	SDL_Rect *region = r;
	SDL_Rect limits;
	SDL_PixelFormat *fmt = dst->format;
	const int pitch = src->pitch, dp = dst->pitch;
	const int bpp = fmt->BytesPerPixel;
	const int len = pitch / bpp, dlen = dp / bpp;
	Uint32 p[4];	// influential pixels array
	Uint32 *srow0 = (Uint32 *) src->pixels;
	Uint32 *dst_p = (Uint32 *) dst->pixels;

	SCALE_(PlatInit) ();

	// expand updated region if necessary
	// pixels neighbooring the updated region may
	// change as a result of updates
	limits.x = 0;
	limits.y = 0;
	limits.w = w;
	limits.h = h;
	Scale_ExpandRect (region, 1, &limits);

	xend = region->x + region->w;
	yend = region->y + region->h;
	dsrc = len - region->w;
	ddst = (dlen - region->w) * 2;

	// move ptrs to the first updated pixel
	srow0 += len * region->y + region->x;
	dst_p += (dlen * region->y + region->x) * 2;

	for (y = region->y; y < yend; ++y, dst_p += ddst, srow0 += dsrc)
	{
		Uint32 *srow1;
		
		SCALE_(Prefetch) (srow0 + 16);
		SCALE_(Prefetch) (srow0 + 32);
		
		if (y < h - 1)
			srow1 = srow0 + len;
		else
			srow1 = srow0;

		SCALE_(Prefetch) (srow1 + 16);
		SCALE_(Prefetch) (srow1 + 32);

		for (x = region->x; x < xend; ++x, ++srow0, ++srow1, dst_p += 2)
		{
			if (x < w - 1)
			{	// can blend directly from pixels
				SCALE_BILINEAR_BLEND4 (srow0, srow1, dst_p, dlen);
			}
			else
			{	// need to make temp pixel rows
				p[0] = srow0[0];
				p[1] = p[0];
				p[2] = srow1[0];
				p[3] = p[2];

				SCALE_BILINEAR_BLEND4 (&p[0], &p[2], dst_p, dlen);
			}
		}

		SCALE_(Prefetch) (srow0 + dsrc);
		SCALE_(Prefetch) (srow0 + dsrc + 16);
		SCALE_(Prefetch) (srow1 + dsrc);
		SCALE_(Prefetch) (srow1 + dsrc + 16);
	}

	SCALE_(PlatDone) ();
}

