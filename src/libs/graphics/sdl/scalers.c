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

#include "types.h"
#include "libs/graphics/sdl/sdl_common.h"
#include "libs/platform.h"
#include "libs/log.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"
#ifdef USE_PLATFORM_ACCEL
#	ifndef __APPLE__
	// MacOS X framework has no SDL_cpuinfo.h for some reason
#		include SDL_INCLUDE(SDL_cpuinfo.h)
#	endif
#	ifdef MMX_ASM
#		include "2xscalers_mmx.h"
#	endif /* MMX_ASM */
#endif /* USE_PLATFORM_ACCEL */

#if SDL_MAJOR_VERSION == 1
#define SDL_HasMMX SDL_HasMMXExt
#endif

typedef enum
{
	SCALEPLAT_NULL    = PLATFORM_NULL,
	SCALEPLAT_C       = PLATFORM_C,
	SCALEPLAT_MMX     = PLATFORM_MMX,
	SCALEPLAT_SSE     = PLATFORM_SSE,
	SCALEPLAT_3DNOW   = PLATFORM_3DNOW,
	SCALEPLAT_ALTIVEC = PLATFORM_ALTIVEC,
		
	SCALEPLAT_C_RGBA,
	SCALEPLAT_C_BGRA,
	SCALEPLAT_C_ARGB,
	SCALEPLAT_C_ABGR,
	
} Scale_PlatType_t;


// RGB -> YUV transformation
// the RGB vector is multiplied by the transformation matrix
// to get the YUV vector
#if 0
// original table -- not used
const int YUV_matrix[3][3] =
{
	/*         Y        U        V    */
	/* R */ {0.2989, -0.1687,  0.5000},
	/* G */ {0.5867, -0.3312, -0.4183},
	/* B */ {0.1144,  0.5000, -0.0816}
};
#else
// scaled up by a 2^14 factor, with Y doubled
const int YUV_matrix[3][3] =
{
	/*         Y      U      V    */
	/* R */ { 9794, -2764,  8192},
	/* G */ {19224, -5428, -6853},
	/* B */ { 3749,  8192, -1339}
};
#endif

// pre-computed transformations for 8 bits per channel
int RGB_to_YUV[/*RGB*/ 3][/*YUV*/ 3][ /*mult-res*/ 256];
sint16 dRGB_to_dYUV[/*RGB*/ 3][/*YUV*/ 3][ /*mult-res*/ 512];

// pre-computed transformations for RGB555
YUV_VECTOR RGB15_to_YUV[0x8000];

PLATFORM_TYPE force_platform = PLATFORM_NULL;
Scale_PlatType_t Scale_Platform = SCALEPLAT_NULL;


// pre-compute the RGB->YUV transformations
void
Scale_Init (void)
{
	int i1, i2, i3;

	for (i1 = 0; i1 < 3; i1++) // enum R,G,B
	for (i2 = 0; i2 < 3; i2++) // enum Y,U,V
	for (i3 = 0; i3 < 256; i3++) // enum possible channel vals
	{
		RGB_to_YUV[i1][i2][i3] =
				(YUV_matrix[i1][i2] * i3) >> 14;
	}

	for (i1 = 0; i1 < 3; i1++) // enum R,G,B
	for (i2 = 0; i2 < 3; i2++) // enum Y,U,V
	for (i3 = -255; i3 < 256; i3++) // enum possible channel delta vals
	{
		dRGB_to_dYUV[i1][i2][i3 + 255] =
				(YUV_matrix[i1][i2] * i3) >> 14;
	}

	for (i1 = 0; i1 < 32; ++i1)
	for (i2 = 0; i2 < 32; ++i2)
	for (i3 = 0; i3 < 32; ++i3)
	{
		int y, u, v;
		// adding upper bits halved for error correction
		int r = (i1 << 3) | (i1 >> 3);
		int g = (i2 << 3) | (i2 >> 3);
		int b = (i3 << 3) | (i3 >> 3);

		y =    (  r * YUV_matrix[YUV_XFORM_R][YUV_XFORM_Y]
				+ g * YUV_matrix[YUV_XFORM_G][YUV_XFORM_Y]
				+ b * YUV_matrix[YUV_XFORM_B][YUV_XFORM_Y]
				) >> 15; // we dont need Y doubled, need Y to fit 8 bits

		// U and V are half the importance of Y
		u = 64+(( r * YUV_matrix[YUV_XFORM_R][YUV_XFORM_U]
				+ g * YUV_matrix[YUV_XFORM_G][YUV_XFORM_U]
				+ b * YUV_matrix[YUV_XFORM_B][YUV_XFORM_U]
				) >> 15); // halved

		v = 64+(( r * YUV_matrix[YUV_XFORM_R][YUV_XFORM_V]
				+ g * YUV_matrix[YUV_XFORM_G][YUV_XFORM_V]
				+ b * YUV_matrix[YUV_XFORM_B][YUV_XFORM_V]
				) >> 15); // halved

		RGB15_to_YUV[(i1 << 10) | (i2 << 5) | i3] = (y << 16) | (u << 8) | v;
	}
}


// expands the given rectangle in all directions by 'expansion'
// guarded by 'limits'
void
Scale_ExpandRect (SDL_Rect* rect, int expansion, const SDL_Rect* limits)
{
	if (rect->x - expansion >= limits->x)
	{
		rect->w += expansion;
		rect->x -= expansion;
	}
	else
	{
		rect->w += rect->x - limits->x;
		rect->x = limits->x;
	}

	if (rect->y - expansion >= limits->y)
	{
		rect->h += expansion;
		rect->y -= expansion;
	}
	else
	{
		rect->h += rect->y - limits->y;
		rect->y = limits->y;
	}

	if (rect->x + rect->w + expansion <= limits->w)
		rect->w += expansion;
	else
		rect->w = limits->w - rect->x;

	if (rect->y + rect->h + expansion <= limits->h)
		rect->h += expansion;
	else
		rect->h = limits->h - rect->y;
}


// Platform+Scaler function lookups

typedef struct
{
	Scale_PlatType_t platform;
	const Scale_FuncDef_t* funcdefs;
} Scale_PlatDef_t;


static const Scale_PlatDef_t
Scale_PlatDefs[] =
{
#if defined(MMX_ASM)
	{SCALEPLAT_SSE,     Scale_SSE_Functions},
	{SCALEPLAT_3DNOW,   Scale_3DNow_Functions},
	{SCALEPLAT_MMX,     Scale_MMX_Functions},
#endif /* MMX_ASM */
	// Default
	{SCALEPLAT_NULL,    Scale_C_Functions}
};


TFB_ScaleFunc
Scale_PrepPlatform (int flags, const SDL_PixelFormat* fmt)
{
	const Scale_PlatDef_t* pdef;
	const Scale_FuncDef_t* fdef;

	(void)flags;

	Scale_Platform = SCALEPLAT_NULL;

	// first match wins
	// add better platform techs to the top
#ifdef MMX_ASM
	if ( (!force_platform && (SDL_HasSSE () || SDL_HasMMX ()))
			|| force_platform == PLATFORM_SSE)
	{
		log_add (log_Info, "Screen scalers are using SSE/MMX-Ext/MMX code");
		Scale_Platform = SCALEPLAT_SSE;
		
		Scale_SSE_PrepPlatform (fmt);
	}
	else
	if ( (!force_platform && SDL_HasAltiVec ())
			|| force_platform == PLATFORM_ALTIVEC)
	{
		log_add (log_Info, "Screen scalers would use AltiVec code "
				"if someone actually wrote it");
		//Scale_Platform = SCALEPLAT_ALTIVEC;
	}
	else
	if ( (!force_platform && SDL_Has3DNow ())
			|| force_platform == PLATFORM_3DNOW)
	{
		log_add (log_Info, "Screen scalers are using 3DNow/MMX code");
		Scale_Platform = SCALEPLAT_3DNOW;
		
		Scale_3DNow_PrepPlatform (fmt);
	}
	else
	if ( (!force_platform && SDL_HasMMX ())
			|| force_platform == PLATFORM_MMX)
	{
		log_add (log_Info, "Screen scalers are using MMX code");
		Scale_Platform = SCALEPLAT_MMX;
		
		Scale_MMX_PrepPlatform (fmt);
	}
#endif
	
	if (Scale_Platform == SCALEPLAT_NULL)
	{	// Plain C versions
		if (fmt->Rmask == 0xff000000 && fmt->Bmask == 0x0000ff00)
			Scale_Platform = SCALEPLAT_C_RGBA;
		else if (fmt->Rmask == 0x00ff0000 && fmt->Bmask == 0x000000ff)
			Scale_Platform = SCALEPLAT_C_ARGB;
		else if (fmt->Rmask == 0x0000ff00 && fmt->Bmask == 0xff000000)
			Scale_Platform = SCALEPLAT_C_BGRA;
		else if (fmt->Rmask == 0x000000ff && fmt->Bmask == 0x00ff0000)
			Scale_Platform = SCALEPLAT_C_ABGR;
		else
		{	// use slowest default
			log_add (log_Warning, "Scale_PrepPlatform(): unknown masks "
					"(Red %08x, Blue %08x)", fmt->Rmask, fmt->Bmask);
			Scale_Platform = SCALEPLAT_C;
		}

		if (Scale_Platform == SCALEPLAT_C)
			log_add (log_Info, "Screen scalers are using slow generic C code");
		else
			log_add (log_Info, "Screen scalers are using optimized C code");
	}

	// Lookup the scaling function
	// First find the right platform
	for (pdef = Scale_PlatDefs;
			pdef->platform != Scale_Platform && pdef->platform != SCALEPLAT_NULL;
			++pdef)
		;
	// Next find the right function
	for (fdef = pdef->funcdefs;
			(flags & fdef->flag) != fdef->flag;
			++fdef)
		;

	return fdef->func;
}

