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
#include "libs/platform.h"

#if defined(MMX_ASM)

#include "libs/graphics/sdl/sdl_common.h"
#include "types.h"
#include "scalers.h"
#include "scaleint.h"
#include "2xscalers.h"
#include "2xscalers_mmx.h"

// MMX name for all functions
#undef SCALE_
#define SCALE_(name) Scale ## _MMX_ ## name

// Tell them which opcodes we want to support
#undef USE_MOVNTQ
#undef USE_PREFETCH
#undef USE_PSADBW
// And Bring in inline asm functions
#include "scalemmx.h"


// Scaler function lookup table
//
const Scale_FuncDef_t
Scale_MMX_Functions[] =
{
	{TFB_GFXFLAGS_SCALE_BILINEAR,   Scale_MMX_BilinearFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPT,    Scale_BiAdaptFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPTADV, Scale_MMX_BiAdaptAdvFilter},
	{TFB_GFXFLAGS_SCALE_TRISCAN,    Scale_MMX_TriScanFilter},
	{TFB_GFXFLAGS_SCALE_HQXX,       Scale_MMX_HqFilter},
	// Default
	{0,                             Scale_MMX_Nearest}
};

// MMX transformation multipliers
Uint64 mmx_888to555_mult;
Uint64 mmx_Y_mult;
Uint64 mmx_U_mult;
Uint64 mmx_V_mult;
// Uint64 mmx_YUV_threshold = 0x00300706; original hq2x threshold
//Uint64 mmx_YUV_threshold = 0x0030100e;
Uint64 mmx_YUV_threshold = 0x0040120c;

void
Scale_MMX_PrepPlatform (const SDL_PixelFormat* fmt)
{
	// prepare the channel-shuffle multiplier
	mmx_888to555_mult = ((Uint64)0x0400) << (fmt->Rshift * 2)
	                  | ((Uint64)0x0020) << (fmt->Gshift * 2)
	                  | ((Uint64)0x0001) << (fmt->Bshift * 2);

	// prepare the RGB->YUV multipliers
	mmx_Y_mult  = ((Uint64)(uint16)YUV_matrix[YUV_XFORM_R][YUV_XFORM_Y])
					<< (fmt->Rshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_G][YUV_XFORM_Y])
					<< (fmt->Gshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_B][YUV_XFORM_Y])
					<< (fmt->Bshift * 2);

	mmx_U_mult  = ((Uint64)(uint16)YUV_matrix[YUV_XFORM_R][YUV_XFORM_U])
					<< (fmt->Rshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_G][YUV_XFORM_U])
					<< (fmt->Gshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_B][YUV_XFORM_U])
					<< (fmt->Bshift * 2);

	mmx_V_mult  = ((Uint64)(uint16)YUV_matrix[YUV_XFORM_R][YUV_XFORM_V])
					<< (fmt->Rshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_G][YUV_XFORM_V])
					<< (fmt->Gshift * 2)
	            | ((Uint64)(uint16)YUV_matrix[YUV_XFORM_B][YUV_XFORM_V])
					<< (fmt->Bshift * 2);

	mmx_YUV_threshold = (SCALE_DIFFYUV_TY << 16) | (SCALE_DIFFYUV_TU << 8)
			| SCALE_DIFFYUV_TV;
}


// Nearest Neighbor scaling to 2x
//	void Scale_MMX_Nearest (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "nearest2x.c"


// Bilinear scaling to 2x
//	void Scale_MMX_BilinearFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "bilinear2x.c"


// Advanced Biadapt scaling to 2x
//	void Scale_MMX_BiAdaptAdvFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "biadv2x.c"


// Triscan scaling to 2x
// derivative of 'scale2x' -- scale2x.sf.net
//	void Scale_MMX_TriScanFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "triscan2x.c"

// Hq2x scaling
//		(adapted from 'hq2x' by Maxim Stepin -- www.hiend3d.com/hq2x.html)
//	void Scale_MMX_HqFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "hq2x.c"


#endif /* MMX_ASM */

