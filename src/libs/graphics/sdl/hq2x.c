//hq2x filter
//--------------------------------------------------------------------------
//Copyright (C) 2003 MaxSt ( maxst@hiend3d.com ) - Original version
//
//Portions Copyright (C) 2005 Alex Volkov ( codepro@usa.net )
//   Modified Oct-2-2005

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

// Core algorithm of the HQ screen scaler
//	adapted from hq2x -- www.hiend3d.com/hq2x.html
//  Template
//    When this file is built standalone is produces a plain C version
//    Also #included by 2xscalers_mmx.c for an MMX version

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"


// Pixel blending/manipulation instructions
#define PIXEL00_0     dst_p[0] = pix[5];
#define PIXEL00_10    dst_p[0] = Scale_Blend_31(pix[5], pix[1]);
#define PIXEL00_11    dst_p[0] = Scale_Blend_31(pix[5], pix[4]);
#define PIXEL00_12    dst_p[0] = Scale_Blend_31(pix[5], pix[2]);
#define PIXEL00_20    dst_p[0] = Scale_Blend_211(pix[5], pix[4], pix[2]);
#define PIXEL00_21    dst_p[0] = Scale_Blend_211(pix[5], pix[1], pix[2]);
#define PIXEL00_22    dst_p[0] = Scale_Blend_211(pix[5], pix[1], pix[4]);
#define PIXEL00_60    dst_p[0] = Scale_Blend_521(pix[5], pix[2], pix[4]);
#define PIXEL00_61    dst_p[0] = Scale_Blend_521(pix[5], pix[4], pix[2]);
#define PIXEL00_70    dst_p[0] = Scale_Blend_611(pix[5], pix[4], pix[2]);
#define PIXEL00_90    dst_p[0] = Scale_Blend_233(pix[5], pix[4], pix[2]);
#define PIXEL00_100   dst_p[0] = Scale_Blend_e11(pix[5], pix[4], pix[2]);
#define PIXEL01_0     dst_p[1] = pix[5];
#define PIXEL01_10    dst_p[1] = Scale_Blend_31(pix[5], pix[3]);
#define PIXEL01_11    dst_p[1] = Scale_Blend_31(pix[5], pix[2]);
#define PIXEL01_12    dst_p[1] = Scale_Blend_31(pix[5], pix[6]);
#define PIXEL01_20    dst_p[1] = Scale_Blend_211(pix[5], pix[2], pix[6]);
#define PIXEL01_21    dst_p[1] = Scale_Blend_211(pix[5], pix[3], pix[6]);
#define PIXEL01_22    dst_p[1] = Scale_Blend_211(pix[5], pix[3], pix[2]);
#define PIXEL01_60    dst_p[1] = Scale_Blend_521(pix[5], pix[6], pix[2]);
#define PIXEL01_61    dst_p[1] = Scale_Blend_521(pix[5], pix[2], pix[6]);
#define PIXEL01_70    dst_p[1] = Scale_Blend_611(pix[5], pix[2], pix[6]);
#define PIXEL01_90    dst_p[1] = Scale_Blend_233(pix[5], pix[2], pix[6]);
#define PIXEL01_100   dst_p[1] = Scale_Blend_e11(pix[5], pix[2], pix[6]);
#define PIXEL10_0     dst_p[dlen] = pix[5];
#define PIXEL10_10    dst_p[dlen] = Scale_Blend_31(pix[5], pix[7]);
#define PIXEL10_11    dst_p[dlen] = Scale_Blend_31(pix[5], pix[8]);
#define PIXEL10_12    dst_p[dlen] = Scale_Blend_31(pix[5], pix[4]);
#define PIXEL10_20    dst_p[dlen] = Scale_Blend_211(pix[5], pix[8], pix[4]);
#define PIXEL10_21    dst_p[dlen] = Scale_Blend_211(pix[5], pix[7], pix[4]);
#define PIXEL10_22    dst_p[dlen] = Scale_Blend_211(pix[5], pix[7], pix[8]);
#define PIXEL10_60    dst_p[dlen] = Scale_Blend_521(pix[5], pix[4], pix[8]);
#define PIXEL10_61    dst_p[dlen] = Scale_Blend_521(pix[5], pix[8], pix[4]);
#define PIXEL10_70    dst_p[dlen] = Scale_Blend_611(pix[5], pix[8], pix[4]);
#define PIXEL10_90    dst_p[dlen] = Scale_Blend_233(pix[5], pix[8], pix[4]);
#define PIXEL10_100   dst_p[dlen] = Scale_Blend_e11(pix[5], pix[8], pix[4]);
#define PIXEL11_0     dst_p[dlen + 1] = pix[5];
#define PIXEL11_10    dst_p[dlen + 1] = Scale_Blend_31(pix[5], pix[9]);
#define PIXEL11_11    dst_p[dlen + 1] = Scale_Blend_31(pix[5], pix[6]);
#define PIXEL11_12    dst_p[dlen + 1] = Scale_Blend_31(pix[5], pix[8]);
#define PIXEL11_20    dst_p[dlen + 1] = Scale_Blend_211(pix[5], pix[6], pix[8]);
#define PIXEL11_21    dst_p[dlen + 1] = Scale_Blend_211(pix[5], pix[9], pix[8]);
#define PIXEL11_22    dst_p[dlen + 1] = Scale_Blend_211(pix[5], pix[9], pix[6]);
#define PIXEL11_60    dst_p[dlen + 1] = Scale_Blend_521(pix[5], pix[8], pix[6]);
#define PIXEL11_61    dst_p[dlen + 1] = Scale_Blend_521(pix[5], pix[6], pix[8]);
#define PIXEL11_70    dst_p[dlen + 1] = Scale_Blend_611(pix[5], pix[6], pix[8]);
#define PIXEL11_90    dst_p[dlen + 1] = Scale_Blend_233(pix[5], pix[6], pix[8]);
#define PIXEL11_100   dst_p[dlen + 1] = Scale_Blend_e11(pix[5], pix[6], pix[8]);


// HQ scaling to 2x
// The name expands to
//		Scale_HqFilter (for plain C)
//		Scale_MMX_HqFilter (for MMX)
//		[others when platforms are added]
void
SCALE_(HqFilter) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
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

	int prevline, nextline;
	Uint32 pix[10];
	Uint32 yuv[10];
//   +----+----+----+
//   |    |    |    |
//   | p1 | p2 | p3 |
//   +----+----+----+
//   |    |    |    |
//   | p4 | p5 | p6 |
//   +----+----+----+
//   |    |    |    |
//   | p7 | p8 | p9 |
//   +----+----+----+

	// MMX code runs faster w/o branching
	#define HQXX_DIFFYUV(p1, p2) \
			SCALE_DIFFYUV (p1, p2)

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
		pix[3] = src_p[prevline];
		pix[6] = src_p[0];
		pix[9] = src_p[nextline];

		yuv[3] = SCALE_TOYUV (pix[3]);
		yuv[6] = SCALE_TOYUV (pix[6]);
		yuv[9] = SCALE_TOYUV (pix[9]);

		if (region->x > 0)
		{
			pix[2] = src_p[prevline - 1];
			pix[5] = src_p[-1];
			pix[8] = src_p[nextline - 1];

			yuv[2] = SCALE_TOYUV (pix[2]);
			yuv[5] = SCALE_TOYUV (pix[5]);
			yuv[8] = SCALE_TOYUV (pix[8]);
		}
		else
		{
			pix[2] = pix[3];
			pix[5] = pix[6];
			pix[8] = pix[9];

			yuv[2] = yuv[3];
			yuv[5] = yuv[6];
			yuv[8] = yuv[9];
		}

		for (x = region->x; x < xend; ++x, ++src_p, dst_p += 2)
		{
			int pattern = 0;

			// slide the window
			pix[1] = pix[2];
			pix[4] = pix[5];
			pix[7] = pix[8];

			yuv[1] = yuv[2];
			yuv[4] = yuv[5];
			yuv[7] = yuv[8];

			pix[2] = pix[3];
			pix[5] = pix[6];
			pix[8] = pix[9];

			yuv[2] = yuv[3];
			yuv[5] = yuv[6];
			yuv[8] = yuv[9];

			if (x < w - 1)
			{
				pix[3] = src_p[prevline + 1];
				pix[6] = src_p[1];
				pix[9] = src_p[nextline + 1];

				yuv[3] = SCALE_TOYUV (pix[3]);
				yuv[6] = SCALE_TOYUV (pix[6]);
				yuv[9] = SCALE_TOYUV (pix[9]);
			}
			else
			{
				pix[3] = pix[2];
				pix[6] = pix[5];
				pix[9] = pix[8];

				yuv[3] = yuv[2];
				yuv[6] = yuv[5];
				yuv[9] = yuv[8];
			}

			// this runs much faster with branching removed
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[1]) & 0x0001;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[2]) & 0x0002;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[3]) & 0x0004;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[4]) & 0x0008;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[6]) & 0x0010;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[7]) & 0x0020;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[8]) & 0x0040;
			pattern |= HQXX_DIFFYUV (yuv[5], yuv[9]) & 0x0080;

			switch (pattern)
			{
				case 0:
				case 1:
				case 4:
				case 32:
				case 128:
				case 5:
				case 132:
				case 160:
				case 33:
				case 129:
				case 36:
				case 133:
				case 164:
				case 161:
				case 37:
				case 165:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 2:
				case 34:
				case 130:
				case 162:
				{
				  PIXEL00_22
				  PIXEL01_21
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 16:
				case 17:
				case 48:
				case 49:
				{
				  PIXEL00_20
				  PIXEL01_22
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 64:
				case 65:
				case 68:
				case 69:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_21
				  PIXEL11_22
				  break;
				}
				case 8:
				case 12:
				case 136:
				case 140:
				{
				  PIXEL00_21
				  PIXEL01_20
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 3:
				case 35:
				case 131:
				case 163:
				{
				  PIXEL00_11
				  PIXEL01_21
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 6:
				case 38:
				case 134:
				case 166:
				{
				  PIXEL00_22
				  PIXEL01_12
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 20:
				case 21:
				case 52:
				case 53:
				{
				  PIXEL00_20
				  PIXEL01_11
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 144:
				case 145:
				case 176:
				case 177:
				{
				  PIXEL00_20
				  PIXEL01_22
				  PIXEL10_20
				  PIXEL11_12
				  break;
				}
				case 192:
				case 193:
				case 196:
				case 197:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_21
				  PIXEL11_11
				  break;
				}
				case 96:
				case 97:
				case 100:
				case 101:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_12
				  PIXEL11_22
				  break;
				}
				case 40:
				case 44:
				case 168:
				case 172:
				{
				  PIXEL00_21
				  PIXEL01_20
				  PIXEL10_11
				  PIXEL11_20
				  break;
				}
				case 9:
				case 13:
				case 137:
				case 141:
				{
				  PIXEL00_12
				  PIXEL01_20
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 18:
				case 50:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 80:
				case 81:
				{
				  PIXEL00_20
				  PIXEL01_22
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 72:
				case 76:
				{
				  PIXEL00_21
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 10:
				case 138:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 66:
				{
				  PIXEL00_22
				  PIXEL01_21
				  PIXEL10_21
				  PIXEL11_22
				  break;
				}
				case 24:
				{
				  PIXEL00_21
				  PIXEL01_22
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 7:
				case 39:
				case 135:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 148:
				case 149:
				case 180:
				{
				  PIXEL00_20
				  PIXEL01_11
				  PIXEL10_20
				  PIXEL11_12
				  break;
				}
				case 224:
				case 228:
				case 225:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 41:
				case 169:
				case 45:
				{
				  PIXEL00_12
				  PIXEL01_20
				  PIXEL10_11
				  PIXEL11_20
				  break;
				}
				case 22:
				case 54:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 208:
				case 209:
				{
				  PIXEL00_20
				  PIXEL01_22
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 104:
				case 108:
				{
				  PIXEL00_21
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 11:
				case 139:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 19:
				case 51:
				{
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL00_11
					PIXEL01_10
				  }
				  else
				  {
					PIXEL00_60
					PIXEL01_90
				  }
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 146:
				case 178:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
					PIXEL11_12
				  }
				  else
				  {
					PIXEL01_90
					PIXEL11_61
				  }
				  PIXEL10_20
				  break;
				}
				case 84:
				case 85:
				{
				  PIXEL00_20
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL01_11
					PIXEL11_10
				  }
				  else
				  {
					PIXEL01_60
					PIXEL11_90
				  }
				  PIXEL10_21
				  break;
				}
				case 112:
				case 113:
				{
				  PIXEL00_20
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL10_12
					PIXEL11_10
				  }
				  else
				  {
					PIXEL10_61
					PIXEL11_90
				  }
				  break;
				}
				case 200:
				case 204:
				{
				  PIXEL00_21
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
					PIXEL11_11
				  }
				  else
				  {
					PIXEL10_90
					PIXEL11_60
				  }
				  break;
				}
				case 73:
				case 77:
				{
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL00_12
					PIXEL10_10
				  }
				  else
				  {
					PIXEL00_61
					PIXEL10_90
				  }
				  PIXEL01_20
				  PIXEL11_22
				  break;
				}
				case 42:
				case 170:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
					PIXEL10_11
				  }
				  else
				  {
					PIXEL00_90
					PIXEL10_60
				  }
				  PIXEL01_21
				  PIXEL11_20
				  break;
				}
				case 14:
				case 142:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
					PIXEL01_12
				  }
				  else
				  {
					PIXEL00_90
					PIXEL01_61
				  }
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 67:
				{
				  PIXEL00_11
				  PIXEL01_21
				  PIXEL10_21
				  PIXEL11_22
				  break;
				}
				case 70:
				{
				  PIXEL00_22
				  PIXEL01_12
				  PIXEL10_21
				  PIXEL11_22
				  break;
				}
				case 28:
				{
				  PIXEL00_21
				  PIXEL01_11
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 152:
				{
				  PIXEL00_21
				  PIXEL01_22
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 194:
				{
				  PIXEL00_22
				  PIXEL01_21
				  PIXEL10_21
				  PIXEL11_11
				  break;
				}
				case 98:
				{
				  PIXEL00_22
				  PIXEL01_21
				  PIXEL10_12
				  PIXEL11_22
				  break;
				}
				case 56:
				{
				  PIXEL00_21
				  PIXEL01_22
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 25:
				{
				  PIXEL00_12
				  PIXEL01_22
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 26:
				case 31:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 82:
				case 214:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 88:
				case 248:
				{
				  PIXEL00_21
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 74:
				case 107:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 27:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_10
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 86:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_21
				  PIXEL11_10
				  break;
				}
				case 216:
				{
				  PIXEL00_21
				  PIXEL01_22
				  PIXEL10_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 106:
				{
				  PIXEL00_10
				  PIXEL01_21
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 30:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 210:
				{
				  PIXEL00_22
				  PIXEL01_10
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 120:
				{
				  PIXEL00_21
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_10
				  break;
				}
				case 75:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  PIXEL10_10
				  PIXEL11_22
				  break;
				}
				case 29:
				{
				  PIXEL00_12
				  PIXEL01_11
				  PIXEL10_22
				  PIXEL11_21
				  break;
				}
				case 198:
				{
				  PIXEL00_22
				  PIXEL01_12
				  PIXEL10_21
				  PIXEL11_11
				  break;
				}
				case 184:
				{
				  PIXEL00_21
				  PIXEL01_22
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 99:
				{
				  PIXEL00_11
				  PIXEL01_21
				  PIXEL10_12
				  PIXEL11_22
				  break;
				}
				case 57:
				{
				  PIXEL00_12
				  PIXEL01_22
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 71:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_21
				  PIXEL11_22
				  break;
				}
				case 156:
				{
				  PIXEL00_21
				  PIXEL01_11
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 226:
				{
				  PIXEL00_22
				  PIXEL01_21
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 60:
				{
				  PIXEL00_21
				  PIXEL01_11
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 195:
				{
				  PIXEL00_11
				  PIXEL01_21
				  PIXEL10_21
				  PIXEL11_11
				  break;
				}
				case 102:
				{
				  PIXEL00_22
				  PIXEL01_12
				  PIXEL10_12
				  PIXEL11_22
				  break;
				}
				case 153:
				{
				  PIXEL00_12
				  PIXEL01_22
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 58:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 83:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 92:
				{
				  PIXEL00_21
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 202:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  PIXEL01_21
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  PIXEL11_11
				  break;
				}
				case 78:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  PIXEL11_22
				  break;
				}
				case 154:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 114:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 89:
				{
				  PIXEL00_12
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 90:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 55:
				case 23:
				{
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL00_11
					PIXEL01_0
				  }
				  else
				  {
					PIXEL00_60
					PIXEL01_90
				  }
				  PIXEL10_20
				  PIXEL11_21
				  break;
				}
				case 182:
				case 150:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
					PIXEL11_12
				  }
				  else
				  {
					PIXEL01_90
					PIXEL11_61
				  }
				  PIXEL10_20
				  break;
				}
				case 213:
				case 212:
				{
				  PIXEL00_20
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL01_11
					PIXEL11_0
				  }
				  else
				  {
					PIXEL01_60
					PIXEL11_90
				  }
				  PIXEL10_21
				  break;
				}
				case 241:
				case 240:
				{
				  PIXEL00_20
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL10_12
					PIXEL11_0
				  }
				  else
				  {
					PIXEL10_61
					PIXEL11_90
				  }
				  break;
				}
				case 236:
				case 232:
				{
				  PIXEL00_21
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
					PIXEL11_11
				  }
				  else
				  {
					PIXEL10_90
					PIXEL11_60
				  }
				  break;
				}
				case 109:
				case 105:
				{
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL00_12
					PIXEL10_0
				  }
				  else
				  {
					PIXEL00_61
					PIXEL10_90
				  }
				  PIXEL01_20
				  PIXEL11_22
				  break;
				}
				case 171:
				case 43:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
					PIXEL10_11
				  }
				  else
				  {
					PIXEL00_90
					PIXEL10_60
				  }
				  PIXEL01_21
				  PIXEL11_20
				  break;
				}
				case 143:
				case 15:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
					PIXEL01_12
				  }
				  else
				  {
					PIXEL00_90
					PIXEL01_61
				  }
				  PIXEL10_22
				  PIXEL11_20
				  break;
				}
				case 124:
				{
				  PIXEL00_21
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_10
				  break;
				}
				case 203:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  PIXEL10_10
				  PIXEL11_11
				  break;
				}
				case 62:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 211:
				{
				  PIXEL00_11
				  PIXEL01_10
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 118:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_12
				  PIXEL11_10
				  break;
				}
				case 217:
				{
				  PIXEL00_12
				  PIXEL01_22
				  PIXEL10_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 110:
				{
				  PIXEL00_10
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 155:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_10
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 188:
				{
				  PIXEL00_21
				  PIXEL01_11
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 185:
				{
				  PIXEL00_12
				  PIXEL01_22
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 61:
				{
				  PIXEL00_12
				  PIXEL01_11
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 157:
				{
				  PIXEL00_12
				  PIXEL01_11
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 103:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_12
				  PIXEL11_22
				  break;
				}
				case 227:
				{
				  PIXEL00_11
				  PIXEL01_21
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 230:
				{
				  PIXEL00_22
				  PIXEL01_12
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 199:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_21
				  PIXEL11_11
				  break;
				}
				case 220:
				{
				  PIXEL00_21
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 158:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 234:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  PIXEL01_21
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_11
				  break;
				}
				case 242:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 59:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 121:
				{
				  PIXEL00_12
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 87:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 79:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  PIXEL11_22
				  break;
				}
				case 122:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 94:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 218:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 91:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 229:
				{
				  PIXEL00_20
				  PIXEL01_20
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 167:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_20
				  PIXEL11_20
				  break;
				}
				case 173:
				{
				  PIXEL00_12
				  PIXEL01_20
				  PIXEL10_11
				  PIXEL11_20
				  break;
				}
				case 181:
				{
				  PIXEL00_20
				  PIXEL01_11
				  PIXEL10_20
				  PIXEL11_12
				  break;
				}
				case 186:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 115:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 93:
				{
				  PIXEL00_12
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 206:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  PIXEL11_11
				  break;
				}
				case 205:
				case 201:
				{
				  PIXEL00_12
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_10
				  }
				  else
				  {
					PIXEL10_70
				  }
				  PIXEL11_11
				  break;
				}
				case 174:
				case 46:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_10
				  }
				  else
				  {
					PIXEL00_70
				  }
				  PIXEL01_12
				  PIXEL10_11
				  PIXEL11_20
				  break;
				}
				case 179:
				case 147:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_10
				  }
				  else
				  {
					PIXEL01_70
				  }
				  PIXEL10_20
				  PIXEL11_12
				  break;
				}
				case 117:
				case 116:
				{
				  PIXEL00_20
				  PIXEL01_11
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_10
				  }
				  else
				  {
					PIXEL11_70
				  }
				  break;
				}
				case 189:
				{
				  PIXEL00_12
				  PIXEL01_11
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 231:
				{
				  PIXEL00_11
				  PIXEL01_12
				  PIXEL10_12
				  PIXEL11_11
				  break;
				}
				case 126:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_10
				  break;
				}
				case 219:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_10
				  PIXEL10_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 125:
				{
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL00_12
					PIXEL10_0
				  }
				  else
				  {
					PIXEL00_61
					PIXEL10_90
				  }
				  PIXEL01_11
				  PIXEL11_10
				  break;
				}
				case 221:
				{
				  PIXEL00_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL01_11
					PIXEL11_0
				  }
				  else
				  {
					PIXEL01_60
					PIXEL11_90
				  }
				  PIXEL10_10
				  break;
				}
				case 207:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
					PIXEL01_12
				  }
				  else
				  {
					PIXEL00_90
					PIXEL01_61
				  }
				  PIXEL10_10
				  PIXEL11_11
				  break;
				}
				case 238:
				{
				  PIXEL00_10
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
					PIXEL11_11
				  }
				  else
				  {
					PIXEL10_90
					PIXEL11_60
				  }
				  break;
				}
				case 190:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
					PIXEL11_12
				  }
				  else
				  {
					PIXEL01_90
					PIXEL11_61
				  }
				  PIXEL10_11
				  break;
				}
				case 187:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
					PIXEL10_11
				  }
				  else
				  {
					PIXEL00_90
					PIXEL10_60
				  }
				  PIXEL01_10
				  PIXEL11_12
				  break;
				}
				case 243:
				{
				  PIXEL00_11
				  PIXEL01_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL10_12
					PIXEL11_0
				  }
				  else
				  {
					PIXEL10_61
					PIXEL11_90
				  }
				  break;
				}
				case 119:
				{
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL00_11
					PIXEL01_0
				  }
				  else
				  {
					PIXEL00_60
					PIXEL01_90
				  }
				  PIXEL10_12
				  PIXEL11_10
				  break;
				}
				case 237:
				case 233:
				{
				  PIXEL00_12
				  PIXEL01_20
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  PIXEL11_11
				  break;
				}
				case 175:
				case 47:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  PIXEL01_12
				  PIXEL10_11
				  PIXEL11_20
				  break;
				}
				case 183:
				case 151:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_20
				  PIXEL11_12
				  break;
				}
				case 245:
				case 244:
				{
				  PIXEL00_20
				  PIXEL01_11
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 250:
				{
				  PIXEL00_10
				  PIXEL01_10
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 123:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_10
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_10
				  break;
				}
				case 95:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_10
				  PIXEL11_10
				  break;
				}
				case 222:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 252:
				{
				  PIXEL00_21
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 249:
				{
				  PIXEL00_12
				  PIXEL01_22
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 235:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_21
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  PIXEL11_11
				  break;
				}
				case 111:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_22
				  break;
				}
				case 63:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_11
				  PIXEL11_21
				  break;
				}
				case 159:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_22
				  PIXEL11_12
				  break;
				}
				case 215:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_21
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 246:
				{
				  PIXEL00_22
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 254:
				{
				  PIXEL00_10
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 253:
				{
				  PIXEL00_12
				  PIXEL01_11
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 251:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  PIXEL01_10
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 239:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  PIXEL01_12
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  PIXEL11_11
				  break;
				}
				case 127:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_20
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_20
				  }
				  PIXEL11_10
				  break;
				}
				case 191:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_11
				  PIXEL11_12
				  break;
				}
				case 223:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_20
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_10
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_20
				  }
				  break;
				}
				case 247:
				{
				  PIXEL00_11
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  PIXEL10_12
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
				case 255:
				{
				  if (HQXX_DIFFYUV (yuv[4], yuv[2]))
				  {
					PIXEL00_0
				  }
				  else
				  {
					PIXEL00_100
				  }
				  if (HQXX_DIFFYUV (yuv[2], yuv[6]))
				  {
					PIXEL01_0
				  }
				  else
				  {
					PIXEL01_100
				  }
				  if (HQXX_DIFFYUV (yuv[8], yuv[4]))
				  {
					PIXEL10_0
				  }
				  else
				  {
					PIXEL10_100
				  }
				  if (HQXX_DIFFYUV (yuv[6], yuv[8]))
				  {
					PIXEL11_0
				  }
				  else
				  {
					PIXEL11_100
				  }
				  break;
				}
			}
		}
	}

	SCALE_(PlatDone) ();
}

