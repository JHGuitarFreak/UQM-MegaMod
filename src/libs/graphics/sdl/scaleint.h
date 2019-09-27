/*
 * Copyright (C) 2005  Alex Volkov (codepro@usa.net)
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

// Scalers Internals

#ifndef SCALEINT_H_
#define SCALEINT_H_

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"


// Plain C names
#define SCALE_(name) Scale ## _ ## name

// These are defaults
#define SCALE_GETPIX(p)        ( *(Uint32 *)(p) )
#define SCALE_SETPIX(p, c)     ( *(Uint32 *)(p) = (c) )

// Plain C defaults
#define SCALE_CMPRGB(p1, p2) \
			SCALE_(GetRGBDelta) (fmt, p1, p2)

#define SCALE_TOYUV(p) \
			SCALE_(RGBtoYUV) (fmt, p)

#define SCALE_CMPYUV(p1, p2, toler) \
			SCALE_(CmpYUV) (fmt, p1, p2, toler)

#define SCALE_DIFFYUV(p1, p2) \
			SCALE_(DiffYUV) (p1, p2)
#define SCALE_DIFFYUV_TY 0x40
#define SCALE_DIFFYUV_TU 0x12
#define SCALE_DIFFYUV_TV 0x0c

#define SCALE_GETY(p) \
			SCALE_(GetPixY) (fmt, p)

#define SCALE_BILINEAR_BLEND4(r0, r1, dst, dlen) \
			SCALE_(Blend_bilinear) (r0, r1, dst, dlen)

#define NO_PREFETCH     0
#define INTEL_PREFETCH  1
#define AMD_PREFETCH    2

typedef enum
{
	YUV_XFORM_R = 0,
	YUV_XFORM_G = 1,
	YUV_XFORM_B = 2,
	YUV_XFORM_Y = 0,
	YUV_XFORM_U = 1,
	YUV_XFORM_V = 2
} RGB_YUV_INDEX;

extern const int YUV_matrix[3][3];

// pre-computed transformations for 8 bits per channel
extern int RGB_to_YUV[/*RGB*/ 3][/*YUV*/ 3][ /*mult-res*/ 256];
extern sint16 dRGB_to_dYUV[/*RGB*/ 3][/*YUV*/ 3][ /*mult-res*/ 512];

typedef Uint32 YUV_VECTOR;
// pre-computed transformations for RGB555
extern YUV_VECTOR RGB15_to_YUV[0x8000];


// Platform+Scaler function lookups
//
typedef struct
{
	int flag;
	TFB_ScaleFunc func;
} Scale_FuncDef_t;


// expands the given rectangle in all directions by 'expansion'
// guarded by 'limits'
extern void Scale_ExpandRect (SDL_Rect* rect, int expansion,
				const SDL_Rect* limits);


// Standard plain C versions of support functions

// Initialize various platform-specific features
static inline void
SCALE_(PlatInit) (void)
{
}

// Finish with various platform-specific features
static inline void
SCALE_(PlatDone) (void)
{
}

#if 0
static inline void
SCALE_(Prefetch) (const void* p)
{
	/* no-op in pure C */
	(void)p;
}
#else
#	define Scale_Prefetch(p)
#endif

// compute the RGB distance squared between 2 pixels
// Plain C version
static inline int
SCALE_(GetRGBDelta) (const SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2)
{
	int c;
	int delta;

	c = ((pix1 >> fmt->Rshift) & 0xff) - ((pix2 >> fmt->Rshift) & 0xff);
	delta = c * c;

	c = ((pix1 >> fmt->Gshift) & 0xff) - ((pix2 >> fmt->Gshift) & 0xff);
	delta += c * c;

	c = ((pix1 >> fmt->Bshift) & 0xff) - ((pix2 >> fmt->Bshift) & 0xff);
	delta += c * c;

	return delta;
}

// retrieve the Y (intensity) component of pixel's YUV
// Plain C version
static inline int
SCALE_(GetPixY) (const SDL_PixelFormat* fmt, Uint32 pix)
{
	Uint32 r, g, b;

	r = (pix >> fmt->Rshift) & 0xff;
	g = (pix >> fmt->Gshift) & 0xff;
	b = (pix >> fmt->Bshift) & 0xff;

	return RGB_to_YUV [YUV_XFORM_R][YUV_XFORM_Y][r]
			+ RGB_to_YUV [YUV_XFORM_G][YUV_XFORM_Y][g]
			+ RGB_to_YUV [YUV_XFORM_B][YUV_XFORM_Y][b];
}

static inline YUV_VECTOR
SCALE_(RGBtoYUV) (const SDL_PixelFormat* fmt, Uint32 pix)
{
	return RGB15_to_YUV[
			(((pix >> (fmt->Rshift + 3)) & 0x1f) << 10) |
			(((pix >> (fmt->Gshift + 3)) & 0x1f) <<  5) |
			(((pix >> (fmt->Bshift + 3)) & 0x1f)      )
			];
}

// compare 2 pixels with respect to their YUV representations
// tolerance set by toler arg
// returns true: close; false: distant (-gt toler)
// Plain C version
static inline bool
SCALE_(CmpYUV) (const SDL_PixelFormat* fmt, Uint32 pix1, Uint32 pix2, int toler)
#if 1
{
	int dr, dg, db;
	int delta;

	dr = ((pix1 >> fmt->Rshift) & 0xff) - ((pix2 >> fmt->Rshift) & 0xff) + 255;
	dg = ((pix1 >> fmt->Gshift) & 0xff) - ((pix2 >> fmt->Gshift) & 0xff) + 255;
	db = ((pix1 >> fmt->Bshift) & 0xff) - ((pix2 >> fmt->Bshift) & 0xff) + 255;
	
	// compute Y delta
	delta = abs (dRGB_to_dYUV [YUV_XFORM_R][YUV_XFORM_Y][dr]
			+ dRGB_to_dYUV [YUV_XFORM_G][YUV_XFORM_Y][dg]
			+ dRGB_to_dYUV [YUV_XFORM_B][YUV_XFORM_Y][db]);
	if (delta > toler)
		return false;

	// compute U delta
	delta += abs (dRGB_to_dYUV [YUV_XFORM_R][YUV_XFORM_U][dr]
			+ dRGB_to_dYUV [YUV_XFORM_G][YUV_XFORM_U][dg]
			+ dRGB_to_dYUV [YUV_XFORM_B][YUV_XFORM_U][db]);
	if (delta > toler)
		return false;
	
	// compute V delta
	delta += abs (dRGB_to_dYUV [YUV_XFORM_R][YUV_XFORM_V][dr]
			+ dRGB_to_dYUV [YUV_XFORM_G][YUV_XFORM_V][dg]
			+ dRGB_to_dYUV [YUV_XFORM_B][YUV_XFORM_V][db]);

	return delta <= toler;
}
#else
{
	int delta;
	Uint32 yuv1, yuv2;

	yuv1 = RGB15_to_YUV[
			(((pix1 >> (fmt->Rshift + 3)) & 0x1f) << 10) |
			(((pix1 >> (fmt->Gshift + 3)) & 0x1f) <<  5) |
			(((pix1 >> (fmt->Bshift + 3)) & 0x1f)      )
			];

	yuv2 = RGB15_to_YUV[
			(((pix2 >> (fmt->Rshift + 3)) & 0x1f) << 10) |
			(((pix2 >> (fmt->Gshift + 3)) & 0x1f) <<  5) |
			(((pix2 >> (fmt->Bshift + 3)) & 0x1f)      )
			];

	// compute Y delta
	delta = abs ((yuv1 & 0xff0000) - (yuv2 & 0xff0000)) >> 16;
	if (delta > toler)
		return false;

	// compute U delta
	delta += abs ((yuv1 & 0x00ff00) - (yuv2 & 0x00ff00)) >> 8;
	if (delta > toler)
		return false;
	
	// compute V delta
	delta += abs ((yuv1 & 0x0000ff) - (yuv2 & 0x0000ff));

	return delta <= toler;
}
#endif

// Check if 2 pixels are different with respect to their
// YUV representations
// returns 0: close; ~0: distant
static inline int
SCALE_(DiffYUV) (Uint32 yuv1, Uint32 yuv2)
{
	// non-branching version -- assumes 2's complement integers
	// delta math only needs 25 bits and we have 32 available;
	// only interested in the sign bits after subtraction
	sint32 delta, ret;

	if (yuv1 == yuv2)
		return 0;

	// compute Y delta
	delta = abs ((yuv1 & 0xff0000) - (yuv2 & 0xff0000));
	ret = (SCALE_DIFFYUV_TY << 16) - delta; // save sign bit
	
	// compute U delta
	delta = abs ((yuv1 & 0x00ff00) - (yuv2 & 0x00ff00));
	ret |= (SCALE_DIFFYUV_TU << 8) - delta; // save sign bit
	
	// compute V delta
	delta = abs ((yuv1 & 0x0000ff) - (yuv2 & 0x0000ff));
	ret |= SCALE_DIFFYUV_TV - delta; // save sign bit

	return (ret >> 31);
}

// blends two pixels with 1:1 ratio
static inline Uint32
SCALE_(Blend_11) (Uint32 pix1, Uint32 pix2)
{
	/* (pix1 + pix2) >> 1 */
	return  
		/*	lower bits can be safely ignored - the error is minimal
			expression that calcs them is left for posterity
			(pix1 & pix2 & low_mask) +
		*/
			((pix1 & 0xfefefefe) >> 1) + ((pix2 & 0xfefefefe) >> 1);
}

// blends four pixels with 1:1:1:1 ratio
static inline Uint32
SCALE_(Blend_1111) (Uint32 pix1, Uint32 pix2,
						Uint32 pix3, Uint32 pix4)
{
	/* (pix1 + pix2 + pix3 + pix4) >> 2 */
	return
		/*	lower bits can be safely ignored - the error is minimal
			expression that calcs them is left for posterity
			((((pix1 & low_mask) + (pix2 & low_mask) +
			   (pix3 & low_mask) + (pix4 & low_mask)
			  ) >> 2) & low_mask) +
		*/
			((pix1 & 0xfcfcfcfc) >> 2) + ((pix2 & 0xfcfcfcfc) >> 2) +
			((pix3 & 0xfcfcfcfc) >> 2) + ((pix4 & 0xfcfcfcfc) >> 2);
}

// blends pixels with 3:1 ratio
static inline Uint32
Scale_Blend_31 (Uint32 pix1, Uint32 pix2)
{
	/* (pix1 * 3 + pix2) / 4 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfefefefe) >> 1) + ((pix1 & 0xfcfcfcfc) >> 2) +
			((pix2 & 0xfcfcfcfc) >> 2);
}

// blends pixels with 2:1:1 ratio
static inline Uint32
Scale_Blend_211 (Uint32 pix1, Uint32 pix2, Uint32 pix3)
{
	/* (pix1 * 2 + pix2 + pix3) / 4 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfefefefe) >> 1) +
			((pix2 & 0xfcfcfcfc) >> 2) +
			((pix3 & 0xfcfcfcfc) >> 2);
}

// blends pixels with 5:2:1 ratio
static inline Uint32
Scale_Blend_521 (Uint32 pix1, Uint32 pix2, Uint32 pix3)
{
	/* (pix1 * 5 + pix2 * 2 + pix3) / 8 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfefefefe) >> 1) + ((pix1 & 0xf8f8f8f8) >> 3) +
			((pix2 & 0xfcfcfcfc) >> 2) +
			((pix3 & 0xf8f8f8f8) >> 3) +
			0x02020202 /* half-error */;
}

// blends pixels with 6:1:1 ratio
static inline Uint32
Scale_Blend_611 (Uint32 pix1, Uint32 pix2, Uint32 pix3)
{
	/* (pix1 * 6 + pix2 + pix3) / 8 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfefefefe) >> 1) + ((pix1 & 0xfcfcfcfc) >> 2) +
			((pix2 & 0xf8f8f8f8) >> 3) +
			((pix3 & 0xf8f8f8f8) >> 3) +
			0x02020202 /* half-error */;
}

// blends pixels with 2:3:3 ratio
static inline Uint32
Scale_Blend_233 (Uint32 pix1, Uint32 pix2, Uint32 pix3)
{
	/* (pix1 * 2 + pix2 * 3 + pix3 * 3) / 8 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfcfcfcfc) >> 2) +
			((pix2 & 0xfcfcfcfc) >> 2) + ((pix2 & 0xf8f8f8f8) >> 3) +
			((pix3 & 0xfcfcfcfc) >> 2) + ((pix3 & 0xf8f8f8f8) >> 3) +
			0x02020202 /* half-error */;
}

// blends pixels with 14:1:1 ratio
static inline Uint32
Scale_Blend_e11 (Uint32 pix1, Uint32 pix2, Uint32 pix3)
{
	/* (pix1 * 14 + pix2 + pix3) >> 4 */
	/*	lower bits can be safely ignored - the error is minimal */
	return  ((pix1 & 0xfefefefe) >> 1) + ((pix1 & 0xfcfcfcfc) >> 2) +
				((pix1 & 0xf8f8f8f8) >> 3) +
			((pix2 & 0xf0f0f0f0) >> 4) +
			((pix3 & 0xf0f0f0f0) >> 4) +
			0x03030303 /* half-error */;
}

// Halfs the pixel's intensity
static inline Uint32
SCALE_(HalfPixel) (Uint32 pix)
{
	return ((pix & 0xfefefefe) >> 1);
}


// Bilinear weighted blend of four pixels
// Function produces 4 blended pixels and writes them
// out to the surface (in 2x2 matrix)
// Pixels are computed using expanded weight matrix like so:
//	('sp' - source pixel, 'dp' - destination pixel)
//	dp[0] = (9*sp[0] + 3*sp[1] + 3*sp[2] + 1*sp[3]) / 16
//	dp[1] = (3*sp[0] + 9*sp[1] + 1*sp[2] + 3*sp[3]) / 16
//	dp[2] = (3*sp[0] + 1*sp[1] + 9*sp[2] + 3*sp[3]) / 16
//	dp[3] = (1*sp[0] + 3*sp[1] + 3*sp[2] + 9*sp[3]) / 16
static inline void
SCALE_(Blend_bilinear) (const Uint32* row0, const Uint32* row1,
					Uint32* dst_p, Uint32 dlen)
{
	// We loose some lower bits here and try to compensate for
	// that by adding half-error values.
	// In general, the error is minimal (+-7)
	// The >>4 reduction is achieved gradually
#	define BL_PACKED_HALF(p) \
			(((p) & 0xfefefefe) >> 1)
#	define BL_SUM(p1, p2) \
			(BL_PACKED_HALF(p1) + BL_PACKED_HALF(p2))
#	define BL_HALF_ERR  0x01010101
#	define BL_SUM_WERR(p1, p2) \
			(BL_PACKED_HALF(p1) + BL_PACKED_HALF(p2) + BL_HALF_ERR)

	Uint32 sum1111, sum1331, sum3113;
	
	// cache p[0] + 3*(p[1] + p[2]) + p[3] in sum1331
	// cache p[1] + 3*(p[0] + p[3]) + p[2] in sum3113
	sum1331 = BL_SUM (row0[1], row1[0]);
	sum3113 = BL_SUM (row0[0], row1[1]);
	
	// cache p[0] + p[1] + p[2] + p[3] in sum1111
	sum1111 = BL_SUM_WERR (sum1331, sum3113);

	sum1331 = BL_SUM_WERR (sum1331, sum1111);
	sum1331 = BL_PACKED_HALF (sum1331);
	sum3113 = BL_SUM_WERR (sum3113, sum1111);
	sum3113 = BL_PACKED_HALF (sum3113);

	// pixel 0 math -- (9*p[0] + 3*(p[1] + p[2]) + p[3]) / 16
	dst_p[0] = BL_PACKED_HALF (row0[0]) + sum1331;

	// pixel 1 math -- (9*p[1] + 3*(p[0] + p[3]) + p[2]) / 16
	dst_p[1] = BL_PACKED_HALF (row0[1]) + sum3113;

	// pixel 2 math -- (9*p[2] + 3*(p[0] + p[3]) + p[1]) / 16
	dst_p[dlen] = BL_PACKED_HALF (row1[0]) + sum3113;

	// pixel 3 math -- (9*p[3] + 3*(p[1] + p[2]) + p[0]) / 16
	dst_p[dlen + 1] = BL_PACKED_HALF (row1[1]) + sum1331;

#	undef BL_PACKED_HALF
#	undef BL_SUM
#	undef BL_HALF_ERR
#	undef BL_SUM_WERR
}

#endif /* SCALEINT_H_ */
