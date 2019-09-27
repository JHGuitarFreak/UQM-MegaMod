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

// SSE name for all functions
#undef SCALE_
#define SCALE_(name) Scale ## _SSE_ ## name

// Tell them which opcodes we want to support
#define USE_MOVNTQ
#define USE_PREFETCH  INTEL_PREFETCH
#define USE_PSADBW
// Bring in inline asm functions
#include "scalemmx.h"


// Scaler function lookup table
//
const Scale_FuncDef_t
Scale_SSE_Functions[] =
{
	{TFB_GFXFLAGS_SCALE_BILINEAR,   Scale_SSE_BilinearFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPT,    Scale_BiAdaptFilter},
	{TFB_GFXFLAGS_SCALE_BIADAPTADV, Scale_SSE_BiAdaptAdvFilter},
	{TFB_GFXFLAGS_SCALE_TRISCAN,    Scale_SSE_TriScanFilter},
	{TFB_GFXFLAGS_SCALE_HQXX,       Scale_MMX_HqFilter},
	// Default
	{0,                             Scale_SSE_Nearest}
};


void
Scale_SSE_PrepPlatform (const SDL_PixelFormat* fmt)
{
	Scale_MMX_PrepPlatform (fmt);
}

// Nearest Neighbor scaling to 2x
//	void Scale_SSE_Nearest (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "nearest2x.c"


// Bilinear scaling to 2x
//	void Scale_SSE_BilinearFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "bilinear2x.c"


// Advanced Biadapt scaling to 2x
//	void Scale_SSE_BiAdaptAdvFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "biadv2x.c"


// Triscan scaling to 2x
// derivative of scale2x -- scale2x.sf.net
//	void Scale_SSE_TriScanFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "triscan2x.c"

#if 0 && NO_IMPROVEMENT
// Hq2x scaling
//		(adapted from 'hq2x' by Maxim Stepin -- www.hiend3d.com/hq2x.html)
//	void Scale_SSE_HqFilter (SDL_Surface *src,
//			SDL_Surface *dst, SDL_Rect *r)

#include "hq2x.c"
#endif

#endif /* MMX_ASM */

