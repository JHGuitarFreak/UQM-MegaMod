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

// 3DNow! name for all functions
#undef SCALE_
#define SCALE_(name) Scale ## _3DNow_ ## name

// Tell them which opcodes we want to support
#undef  USE_MOVNTQ
#define USE_PREFETCH  AMD_PREFETCH
#undef  USE_PSADBW
// Bring in inline asm functions
#include "scalemmx.h"


// Scaler function lookup table
//
const Scale_FuncDef_t
Scale_3DNow_Functions[] =
{
	{TFB_GFXFLAGS_SCALE_BILINEAR,   Scale_3DNow_BilinearFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPT,    Scale_BiAdaptFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPTADV, Scale_MMX_BiAdaptAdvFilter},
	{TFB_GFXFLAGS_SCALE_TRISCAN,    Scale_MMX_TriScanFilter},
	{TFB_GFXFLAGS_SCALE_HQXX,       Scale_MMX_HqFilter},
	// Default
	{0,                             Scale_3DNow_Nearest}
};


void
Scale_3DNow_PrepPlatform (const SDL_PixelFormat* fmt)
{
	Scale_MMX_PrepPlatform (fmt);
}

// Nearest Neighbor scaling to 2x
//	void Scale_3DNow_Nearest (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "nearest2x.c"


// Bilinear scaling to 2x
//	void Scale_3DNow_BilinearFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "bilinear2x.c"


#if 0 && NO_IMPROVEMENT

// Advanced Biadapt scaling to 2x
//	void Scale_3DNow_BiAdaptAdvFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "biadv2x.c"


// Triscan scaling to 2x
// derivative of scale2x -- scale2x.sf.net
//	void Scale_3DNow_TriScanFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "triscan2x.c"

// Hq2x scaling
//		(adapted from 'hq2x' by Maxim Stepin -- www.hiend3d.com/hq2x.html)
//	void Scale_3DNow_HqFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "hq2x.c"

#endif /* NO_IMPROVEMENT */

#endif /* MMX_ASM */

