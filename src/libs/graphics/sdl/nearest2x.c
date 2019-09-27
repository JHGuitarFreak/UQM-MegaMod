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

// Nearest Neighbor scaling to 2x
// The name expands to
//		Scale_Nearest (for plain C)
//		Scale_MMX_Nearest (for MMX)
//		Scale_SSE_Nearest (for SSE)
//		[others when platforms are added]
void
SCALE_(Nearest) (SDL_Surface *src, SDL_Surface *dst, SDL_Rect *r)
{
	int y;
	const int rw = r->w, rh = r->h;
	const int sp = src->pitch, dp = dst->pitch;
	const int bpp = dst->format->BytesPerPixel;
	const int slen = sp / bpp, dlen = dp / bpp;
	const int dsrc = slen-rw, ddst = (dlen-rw) * 2;

	Uint32 *src_p = (Uint32 *)src->pixels;
	Uint32 *dst_p = (Uint32 *)dst->pixels;

	// guard asm code against such atrocities
	if (rw == 0 || rh == 0)
		return;

	SCALE_(PlatInit) ();

	// move ptrs to the first updated pixel
	src_p += slen * r->y + r->x;
	dst_p += (dlen * r->y + r->x) * 2;

#if defined(MMX_ASM) && defined(MSVC_ASM)
	// Just about everything has to be done in asm for MSVC
	// to actually take advantage of asm here
	// MSVC does not support beautiful GCC-like asm templates

	y = rh;
	__asm
	{
		// setup vars
		mov       esi, src_p
		mov       edi, dst_p

		PREFETCH  (esi + 0x40)
		PREFETCH  (esi + 0x80)
		PREFETCH  (esi + 0xc0)

		mov       edx, dlen
		lea       edx, [edx * 4]
		mov       eax, dsrc
		lea       eax, [eax * 4]
		mov       ebx, ddst
		lea       ebx, [ebx * 4]

		mov       ecx, rw
	loop_y:
		test       ecx, 1
		jz         even_x
	
		// one-pixel transfer
		movd       mm1, [esi]
		punpckldq  mm1, mm1    // pix1 | pix1 -> mm1
		add        esi, 4
		MOVNTQ     (edi, mm1)
		add        edi, 8
		MOVNTQ     (edi - 8 + edx, mm1)

	even_x:
		shr        ecx, 1      // x = rw / 2
		jz         end_x       // rw was 1

	loop_x:
		// two-pixel transfer
		movq       mm1, [esi]
		movq       mm2, mm1
		PREFETCH   (esi + 0x100)
		punpckldq  mm1, mm1    // pix1 | pix1 -> mm1
		add        esi, 8
		MOVNTQ     (edi, mm1)
		punpckhdq  mm2, mm2    // pix2 | pix2 -> mm2
		MOVNTQ     (edi + edx, mm1)
		add        edi, 16
		MOVNTQ     (edi - 8, mm2)
		MOVNTQ     (edi - 8 + edx, mm2)

		dec        ecx
		jnz        loop_x
	
	end_x:
		// try to prefetch as early as possible to have it on time
		PREFETCH  (esi + eax)

		mov       ecx, rw
		add       esi, eax

		PREFETCH  (esi + 0x40)
		PREFETCH  (esi + 0x80)
		PREFETCH  (esi + 0xc0)

		add       edi, ebx

		dec       y
		jnz       loop_y
	}

#elif defined(MMX_ASM) && defined(GCC_ASM)

	SCALE_(Prefetch) (src_p + 16);
	SCALE_(Prefetch) (src_p + 32);
	SCALE_(Prefetch) (src_p + 48);

	for (y = rh; y; --y)
	{
		int x = rw;

		if (x & 1)
		{	// one-pixel transfer
			__asm__ (
				"movd       (%0), %%mm1      \n\t"
				"punpckldq  %%mm1, %%mm1     \n\t"
				 MOVNTQ     (%%mm1, (%1))   "\n\t"
				 MOVNTQ     (%%mm1, (%1,%2)) "\n\t"

			: /* nothing */
			: /*0*/"r" (src_p), /*1*/"r" (dst_p), /*2*/"r" (dlen*sizeof(Uint32))
			);

			++src_p;
			dst_p += 2;
			--x;
		}

		for (x >>= 1; x; --x, src_p += 2, dst_p += 4)
		{	// two-pixel transfer
			__asm__ (
				"movq       (%0), %%mm1       \n\t"
				"movq       %%mm1, %%mm2      \n\t"
				 PREFETCH   (0x100(%0))      "\n\t"
				"punpckldq  %%mm1, %%mm1      \n\t"
				 MOVNTQ     (%%mm1, (%1))    "\n\t"
				 MOVNTQ     (%%mm1, (%1,%2)) "\n\t"
				"punpckhdq  %%mm2, %%mm2      \n\t"
				 MOVNTQ     (%%mm2, 8(%1))    "\n\t"
				 MOVNTQ     (%%mm2, 8(%1,%2)) "\n\t"

			: /* nothing */
			: /*0*/"r" (src_p), /*1*/"r" (dst_p), /*2*/"r" (dlen*sizeof(Uint32))
			);
		}

		src_p += dsrc;
		// try to prefetch as early as possible to have it on time
		SCALE_(Prefetch) (src_p);

		dst_p += ddst;

		SCALE_(Prefetch) (src_p + 16);
		SCALE_(Prefetch) (src_p + 32);
		SCALE_(Prefetch) (src_p + 48);
	}

#else
	// Plain C version
	for (y = 0; y < rh; ++y)
	{
		int x;
		for (x = 0; x < rw; ++x, ++src_p, dst_p += 2)
		{
			Uint32 pix = *src_p;
			dst_p[0] = pix;
			dst_p[1] = pix;
			dst_p[dlen] = pix;
			dst_p[dlen + 1] = pix;
		}
		dst_p += ddst;
		src_p += dsrc;
	}
#endif

	SCALE_(PlatDone) ();
}

