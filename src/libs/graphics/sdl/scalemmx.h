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

#ifndef SCALEMMX_H_
#define SCALEMMX_H_

#if !defined(SCALE_)
#	error Please define SCALE_(name) before including scalemmx.h
#endif

#if !defined(MSVC_ASM) && !defined(GCC_ASM)
#	error Please define target assembler (MSVC_ASM, GCC_ASM) before including scalemmx.h
#endif

// MMX defaults (no Format param)
#undef  SCALE_CMPRGB
#define SCALE_CMPRGB(p1, p2) \
			SCALE_(GetRGBDelta) (p1, p2)

#undef  SCALE_TOYUV
#define SCALE_TOYUV(p) \
			SCALE_(RGBtoYUV) (p)

#undef  SCALE_CMPYUV
#define SCALE_CMPYUV(p1, p2, toler) \
			SCALE_(CmpYUV) (p1, p2, toler)

#undef  SCALE_GETY
#define SCALE_GETY(p) \
			SCALE_(GetPixY) (p)

// MMX transformation multipliers
extern Uint64 mmx_888to555_mult;
extern Uint64 mmx_Y_mult;
extern Uint64 mmx_U_mult;
extern Uint64 mmx_V_mult;
extern Uint64 mmx_YUV_threshold;

#define USE_YUV_LOOKUP

#if defined(MSVC_ASM)
//	MSVC inline assembly versions

#if defined(USE_MOVNTQ)
#	define MOVNTQ(addr, val)   movntq      [addr], val
#else
#	define MOVNTQ(addr, val)   movq        [addr], val
#endif

#if USE_PREFETCH == INTEL_PREFETCH
//	using Intel SSE non-temporal prefetch
#	define PREFETCH(addr)      prefetchnta [addr]
#	define HAVE_PREFETCH
#elif USE_PREFETCH == AMD_PREFETCH
//	using AMD 3DNOW! prefetch
#	define PREFETCH(addr)      prefetch    [addr]
#	define HAVE_PREFETCH
#else
//	no prefetch -- too bad for poor MMX-only souls
#	define PREFETCH(addr)
#	undef  HAVE_PREFETCH
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1300)
#	pragma warning( disable : 4799 )
#endif

static inline void
SCALE_(PlatInit) (void)
{
	__asm
	{
		// mm0 will be kept == 0 throughout
		// 0 is needed for bytes->words unpack instructions
		pxor       mm0, mm0
	}
}

static inline void
SCALE_(PlatDone) (void)
{
	// finish with MMX registers and yield them to FPU
	__asm
	{
		emms
	}
}

#if defined(HAVE_PREFETCH)
static inline void
SCALE_(Prefetch) (const void* p)
{
    __asm
	{
		mov       eax, p
		PREFETCH  (eax)
	}
}

#else /* Not HAVE_PREFETCH */

static inline void
SCALE_(Prefetch) (const void* p)
{
	(void)p; // silence compiler
	/* no-op */
}

#endif /* HAVE_PREFETCH */

// compute the RGB distance squared between 2 pixels
static inline int
SCALE_(GetRGBDelta) (Uint32 pix1, Uint32 pix2)
{
	__asm
	{
		// load pixels
		movd       mm1, pix1
		punpcklbw  mm1, mm0
		movd       mm2, pix2
		punpcklbw  mm2, mm0
		// get the difference between RGBA components
		psubw      mm1, mm2
		// squared and sumed
		pmaddwd    mm1, mm1
		// finish suming the squares
		movq       mm2, mm1
		punpckhdq  mm2, mm0
		paddd      mm1, mm2
		// store result
		movd       eax, mm1
	}
}

// retrieve the Y (intensity) component of pixel's YUV
static inline int
SCALE_(GetPixY) (Uint32 pix)
{
	__asm
	{
		// load pixel
		movd       mm1, pix
		punpcklbw  mm1, mm0
		// process
		pmaddwd    mm1, mmx_Y_mult // RGB * Yvec
		movq       mm2, mm1   // finish suming
		punpckhdq  mm2, mm0   //   ditto
		paddd      mm1, mm2   //   ditto
		// store result
		movd       eax, mm1
		shr        eax, 14
	}
}

#ifdef USE_YUV_LOOKUP

// convert pixel RGB vector into YUV representation vector
static inline YUV_VECTOR
SCALE_(RGBtoYUV) (Uint32 pix)
{
	__asm
	{
		// convert RGB888 to 555
		movd       mm1, pix
		punpcklbw  mm1, mm0
		psrlw      mm1, 3    // 8->5 bit
		pmaddwd    mm1, mmx_888to555_mult // shuffle into the right channel order
		movq       mm2, mm1   // finish shuffling
		punpckhdq  mm2, mm0   //   ditto
		por        mm1, mm2   //   ditto
		
		// lookup the YUV vector
		movd       eax, mm1
		mov        eax, [RGB15_to_YUV + eax * 4]
	}
}

// compare 2 pixels with respect to their YUV representations
// tolerance set by toler arg
// returns true: close; false: distant (-gt toler)
static inline bool
SCALE_(CmpYUV) (Uint32 pix1, Uint32 pix2, int toler)
{
	__asm
	{
		// convert RGB888 to 555
		movd       mm1, pix1
		punpcklbw  mm1, mm0
		psrlw      mm1, 3    // 8->5 bit
		movd       mm3, pix2
		punpcklbw  mm3, mm0
		psrlw      mm3, 3    // 8->5 bit
		pmaddwd    mm1, mmx_888to555_mult  // shuffle into the right channel order
		movq       mm2, mm1   // finish shuffling
		pmaddwd    mm3, mmx_888to555_mult  // shuffle into the right channel order
		movq       mm4, mm3   // finish shuffling
		punpckhdq  mm2, mm0   //   ditto
		por        mm1, mm2   //   ditto
		punpckhdq  mm4, mm0   //   ditto
		por        mm3, mm4   //   ditto
		
		// lookup the YUV vector
		movd       eax, mm1
		movd       edx, mm3
		movd       mm1, [RGB15_to_YUV + eax * 4]
		movq       mm4, mm1
		movd       mm2, [RGB15_to_YUV + edx * 4]

		// get abs difference between YUV components
#ifdef USE_PSADBW
		// we can use PSADBW and save us some grief
		psadbw     mm1, mm2
		movd       edx, mm1
#else
		// no PSADBW -- have to do it the hard way
		psubusb    mm1, mm2
		psubusb    mm2, mm4
		por        mm1, mm2
		
		// sum the differences
		// XXX: technically, this produces a MAX diff of 510
		//  but we do not need anything bigger, currently
		movq       mm2, mm1
		psrlq      mm2, 8
		paddusb    mm1, mm2
		psrlq      mm2, 8
		paddusb    mm1, mm2
		movd       edx, mm1
		and        edx, 0xff
#endif /* USE_PSADBW */
		xor        eax, eax
		shl        edx, 1
		cmp        edx, toler
		// store result
		setle      al
	}
}

#else /* Not USE_YUV_LOOKUP */

// convert pixel RGB vector into YUV representation vector
static inline YUV_VECTOR
SCALE_(RGBtoYUV) (Uint32 pix)
{
	__asm
	{
		movd       mm1, pix
		punpcklbw  mm1, mm0
		
		movq       mm2, mm1

		// Y vector multiply
		pmaddwd    mm1, mmx_Y_mult
		movq       mm4, mm1
		punpckhdq  mm4, mm0
		punpckldq  mm1, mm0 // clear out the high dword
		paddd      mm1, mm4
		psrad      mm1, 15

		movq       mm3, mm2

		// U vector multiply
		pmaddwd    mm2, mmx_U_mult
		psrad      mm2, 10
		
		// V vector multiply
		pmaddwd    mm3, mmx_V_mult
		psrad      mm3, 10
		
		// load (1|1|1|1) into mm4
		pcmpeqw    mm4, mm4
		psrlw      mm4, 15

		packssdw   mm3, mm2
		pmaddwd    mm3, mm4
		psrad      mm3, 5

		// load (64|64) into mm4
		punpcklwd  mm4, mm0
		pslld      mm4, 6
		paddd      mm3, mm4

		packssdw   mm3, mm1
		packuswb   mm3, mm0

		movd       eax, mm3
	}
}

// compare 2 pixels with respect to their YUV representations
// tolerance set by toler arg
// returns true: close; false: distant (-gt toler)
static inline bool
SCALE_(CmpYUV) (Uint32 pix1, Uint32 pix2, int toler)
{
	__asm
	{
		movd       mm1, pix1
		punpcklbw  mm1, mm0
		movd       mm2, pix2
		punpcklbw  mm2, mm0

		psubw      mm1, mm2
		movq       mm2, mm1

		// Y vector multiply
		pmaddwd    mm1, mmx_Y_mult
		movq       mm4, mm1
		punpckhdq  mm4, mm0
		paddd      mm1, mm4
		// abs()
		movq       mm4, mm1
		psrad      mm4, 31
		pxor       mm4, mm1
		psubd      mm1, mm4

		movq       mm3, mm2

		// U vector multiply
		pmaddwd    mm2, mmx_U_mult
		movq       mm4, mm2
		punpckhdq  mm4, mm0
		paddd      mm2, mm4
		// abs()
		movq       mm4, mm2
		psrad      mm4, 31
		pxor       mm4, mm2
		psubd      mm2, mm4

		paddd      mm1, mm2

		// V vector multiply
		pmaddwd    mm3, mmx_V_mult
		movq       mm4, mm3
		punpckhdq  mm3, mm0
		paddd      mm3, mm4
		// abs()
		movq       mm4, mm3
		psrad      mm4, 31
		pxor       mm4, mm3
		psubd      mm3, mm4

		paddd      mm1, mm3

		movd       edx, mm1
		xor        eax, eax
		shr        edx, 14
		cmp        edx, toler
		// store result
		setle      al
	}
}

#endif /* USE_YUV_LOOKUP */

// Check if 2 pixels are different with respect to their
// YUV representations
// returns 0: close; ~0: distant
static inline int
SCALE_(DiffYUV) (Uint32 yuv1, Uint32 yuv2)
{
	__asm
	{
		// load YUV pixels
		movd       mm1, yuv1
		movq       mm4, mm1
		movd       mm2, yuv2
		// abs difference between channels
		psubusb    mm1, mm2
		psubusb    mm2, mm4
		por        mm1, mm2
		// compare to threshold
		psubusb    mm1, mmx_YUV_threshold

		movd       edx, mm1
		// transform eax to 0 or ~0
		xor        eax, eax
		or         edx, edx
		setz       al
		dec        eax
	}
}

// bilinear weighted blend of four pixels
// MSVC asm version
static inline void
SCALE_(Blend_bilinear) (const Uint32* row0, const Uint32* row1,
				Uint32* dst_p, Uint32 dlen)
{
	__asm
	{
		// EL0: setup vars
		mov        ebx, row0 // EL0

		// EL0: load pixels
		movq       mm1, [ebx] // EL0
		movq       mm2, mm1   // EL0: p[1] -> mm2
		PREFETCH   (ebx + 0x80)
		punpckhbw  mm2, mm0   // EL0: p[1] -> mm2
		mov        ebx, row1
		punpcklbw  mm1, mm0   // EL0: p[0] -> mm1
		movq       mm3, [ebx]
		movq       mm4, mm3   // EL0: p[3] -> mm4
		movq       mm6, mm2   // EL1.1: p[1] -> mm6
		PREFETCH   (ebx + 0x80)
		punpcklbw  mm3, mm0   // EL0: p[2] -> mm3
		movq       mm5, mm1   // EL1.1: p[0] -> mm5
		punpckhbw  mm4, mm0   // EL0: p[3] -> mm4

		mov        edi, dst_p // EL0

		// EL1: cache p[0] + 3*(p[1] + p[2]) + p[3] in mm6
		paddw      mm6, mm3   // EL1.2: p[1] + p[2] -> mm6
		// EL1: cache p[0] + p[1] + p[2] + p[3] in mm7
		movq       mm7, mm6   // EL1.3: p[1] + p[2] -> mm7
		// EL1: cache p[1] + 3*(p[0] + p[3]) + p[2] in mm5
		paddw      mm5, mm4   // EL1.2: p[0] + p[3] -> mm5
		psllw      mm6, 1     // EL1.4: 2*(p[1] + p[2]) -> mm6
		paddw      mm7, mm5   // EL1.4: sum(p[]) -> mm7
		psllw      mm5, 1     // EL1.5: 2*(p[0] + p[3]) -> mm5
		paddw      mm6, mm7   // EL1.5: p[0] + 3*(p[1] + p[2]) + p[3] -> mm6
		paddw      mm5, mm7   // EL1.6: p[1] + 3*(p[0] + p[3]) + p[2] -> mm5

		// EL2: pixel 0 math -- (9*p[0] + 3*(p[1] + p[2]) + p[3]) / 16
		psllw      mm1, 3     // EL2.1: 8*p[0] -> mm1
		paddw      mm1, mm6   // EL2.2: 9*p[0] + 3*(p[1] + p[2]) + p[3] -> mm1
		psrlw      mm1, 4     // EL2.3: sum[0]/16 -> mm1

		mov        edx, dlen  // EL0

		// EL3: pixel 1 math -- (9*p[1] + 3*(p[0] + p[3]) + p[2]) / 16
		psllw      mm2, 3     // EL3.1: 8*p[1] -> mm2
		paddw      mm2, mm5   // EL3.2: 9*p[1] + 3*(p[0] + p[3]) + p[2] -> mm2
		psrlw      mm2, 4     // EL3.3: sum[1]/16 -> mm5

		// EL2/3: store pixels 0 & 1
		packuswb   mm1, mm2   // EL2/3: pack into bytes
		MOVNTQ     (edi, mm1) // EL2/3: store 2 pixels

		// EL4: pixel 2 math -- (9*p[2] + 3*(p[0] + p[3]) + p[1]) / 16
		psllw      mm3, 3     // EL4.1: 8*p[2] -> mm3
		paddw      mm3, mm5   // EL4.2: 9*p[2] + 3*(p[0] + p[3]) + p[1] -> mm3
		psrlw      mm3, 4     // EL4.3: sum[2]/16 -> mm3

		// EL5: pixel 3 math -- (9*p[3] + 3*(p[1] + p[2]) + p[0]) / 16
		psllw      mm4, 3     // EL5.1: 8*p[3] -> mm4
		paddw      mm4, mm6   // EL5.2: 9*p[3] + 3*(p[1] + p[2]) + p[0] -> mm4
		psrlw      mm4, 4     // EL5.3: sum[3]/16 -> mm4

		// EL4/5: store pixels 2 & 3
		packuswb   mm3, mm4   // EL4/5: pack into bytes
		MOVNTQ     (edi + edx*4, mm3) // EL4/5: store 2 pixels
	}
}
// End MSVC_ASM

#elif defined(GCC_ASM)
//	GCC inline assembly versions

#if defined(USE_MOVNTQ)
#	define MOVNTQ(val, addr)   "movntq "   #val "," #addr
#else
#	define MOVNTQ(val, addr)   "movq "     #val "," #addr
#endif

#if USE_PREFETCH == INTEL_PREFETCH
//	using Intel SSE non-temporal prefetch
#	define PREFETCH(addr)      "prefetchnta " #addr
#elif USE_PREFETCH == AMD_PREFETCH
//	using AMD 3DNOW! prefetch
#	define PREFETCH(addr)      "prefetch "    #addr
#else
//	no prefetch -- too bad for poor MMX-only souls
#	define PREFETCH(addr)
#endif

#if defined(__x86_64__)
#	define A_REG   "rax"
#	define D_REG   "rdx"
#	define CLR_UPPER32(r)      "xor "  "%%" r "," "%%" r
#else
#	define A_REG   "eax"
#	define D_REG   "edx"
#	define CLR_UPPER32(r)
#endif

static inline void
SCALE_(PlatInit) (void)
{
	__asm__ (
		// mm0 will be kept == 0 throughout
		// 0 is needed for bytes->words unpack instructions
		"pxor       %%mm0, %%mm0 \n\t"

	: /* nothing */
	: /* nothing */
	);
}

static inline void
SCALE_(PlatDone) (void)
{
	// finish with MMX registers and yield them to FPU
	__asm__ (
		"emms \n\t"
	: /* nothing */ : /* nothing */
	);
}

static inline void
SCALE_(Prefetch) (const void* p)
{
    __asm__ __volatile__ ("" PREFETCH (%0) : /*nothing*/ : "m" (p) );
}

// compute the RGB distance squared between 2 pixels
static inline int
SCALE_(GetRGBDelta) (Uint32 pix1, Uint32 pix2)
{
	int res;
	
	__asm__ (
		// load pixels
		"movd       %1, %%mm1    \n\t"
		"punpcklbw  %%mm0, %%mm1 \n\t"
		"movd       %2, %%mm2    \n\t"
		"punpcklbw  %%mm0, %%mm2 \n\t"
		// get the difference between RGBA components
		"psubw      %%mm2, %%mm1 \n\t"
		// squared and sumed
		"pmaddwd    %%mm1, %%mm1 \n\t"
		// finish suming the squares
		"movq       %%mm1, %%mm2 \n\t"
		"punpckhdq  %%mm0, %%mm2 \n\t"
		"paddd      %%mm2, %%mm1 \n\t"
		// store result
		"movd       %%mm1, %0    \n\t"
	
	: /*0*/"=rm" (res)
	: /*1*/"rm" (pix1), /*2*/"rm" (pix2)
	);
	
	return res;
}

// retrieve the Y (intensity) component of pixel's YUV
static inline int
SCALE_(GetPixY) (Uint32 pix)
{
	int ret;
	
	__asm__ (
		// load pixel
		"movd       %1, %%mm1    \n\t"
		"punpcklbw  %%mm0, %%mm1 \n\t"
		// process
		"pmaddwd    %2, %%mm1    \n\t" // R,G,B * Yvec
		"movq       %%mm1, %%mm2 \n\t" // finish suming
		"punpckhdq  %%mm0, %%mm2 \n\t" //   ditto
		"paddd      %%mm2, %%mm1 \n\t" //   ditto
		// store index
		"movd       %%mm1, %0    \n\t"
	
	: /*0*/"=r" (ret)
	: /*1*/"rm" (pix), /*2*/"m" (mmx_Y_mult)
	);
	return ret >> 14;
}

#ifdef USE_YUV_LOOKUP

// convert pixel RGB vector into YUV representation vector
static inline YUV_VECTOR
SCALE_(RGBtoYUV) (Uint32 pix)
{
	int i;

	__asm__ (
		// convert RGB888 to 555
		"movd       %1, %%mm1 \n\t"
		"punpcklbw  %%mm0, %%mm1 \n\t"
		"psrlw      $3, %%mm1    \n\t"   // 8->5 bit
		"pmaddwd    %2, %%mm1    \n\t"  // shuffle into the right channel order
		"movq       %%mm1, %%mm2 \n\t"  // finish shuffling
		"punpckhdq  %%mm0, %%mm2 \n\t"  //   ditto
		"por        %%mm2, %%mm1 \n\t"  //   ditto
		"movd       %%mm1, %0    \n\t"

	: /*0*/"=rm" (i)
	: /*1*/"rm" (pix), /*2*/"m" (mmx_888to555_mult)
	);
	return RGB15_to_YUV[i];
}

// compare 2 pixels with respect to their YUV representations
// tolerance set by toler arg
// returns true: close; false: distant (-gt toler)
static inline bool
SCALE_(CmpYUV) (Uint32 pix1, Uint32 pix2, int toler)
{
	int delta;
	
	__asm__ (
		"movd       %1, %%mm1 \n\t"
		"movd       %2, %%mm3 \n\t"
		
		// convert RGB888 to 555
		// this is somewhat parallelized
		"punpcklbw  %%mm0, %%mm1 \n\t"
		CLR_UPPER32 (A_REG)     "\n\t"
		"psrlw      $3, %%mm1    \n\t"   // 8->5 bit
		"punpcklbw  %%mm0, %%mm3 \n\t"
		"psrlw      $3, %%mm3    \n\t" // 8->5 bit
		"pmaddwd    %4, %%mm1    \n\t" // shuffle into the right channel order
		"movq       %%mm1, %%mm2 \n\t"  // finish shuffling
		"pmaddwd    %4, %%mm3    \n\t" // shuffle into the right channel order
		CLR_UPPER32 (D_REG)     "\n\t"
		"movq       %%mm3, %%mm4 \n\t"  // finish shuffling
		"punpckhdq  %%mm0, %%mm2 \n\t"  //   ditto
		"por        %%mm2, %%mm1 \n\t"  //   ditto
		"punpckhdq  %%mm0, %%mm4 \n\t"  //   ditto
		"por        %%mm4, %%mm3 \n\t"  //   ditto
		
		// lookup the YUV vector
		"movd       %%mm1, %%eax \n\t"
		"movd       %%mm3, %%edx \n\t"
		"movd       (%3, %%" A_REG ", 4), %%mm1  \n\t"
		"movq       %%mm1, %%mm4 \n\t"
		"movd       (%3, %%" D_REG ", 4), %%mm2  \n\t"

		// get abs difference between YUV components
#ifdef USE_PSADBW
		// we can use PSADBW and save us some grief
		"psadbw     %%mm2, %%mm1 \n\t"
		"movd       %%mm1, %0    \n\t"
#else
		// no PSADBW -- have to do it the hard way
		"psubusb    %%mm2, %%mm1 \n\t"
		"psubusb    %%mm4, %%mm2 \n\t"
		"por        %%mm2, %%mm1 \n\t"
		
		// sum the differences
		//  technically, this produces a MAX diff of 510
		//  but we do not need anything bigger, currently
		"movq       %%mm1, %%mm2 \n\t"
		"psrlq      $8, %%mm2    \n\t"
		"paddusb    %%mm2, %%mm1 \n\t"
		"psrlq      $8, %%mm2    \n\t"
		"paddusb    %%mm2, %%mm1 \n\t"
		// store intermediate delta
		"movd       %%mm1, %0    \n\t"
		"andl       $0xff, %0    \n\t"
#endif /* USE_PSADBW */
	: /*0*/"=rm" (delta)
	: /*1*/"rm" (pix1), /*2*/"rm" (pix2),
		/*3*/ "r" (RGB15_to_YUV),
		/*4*/"m" (mmx_888to555_mult)
	: "%" A_REG, "%" D_REG, "cc"
	);
	
	return (delta << 1) <= toler;
}

#endif /* USE_YUV_LOOKUP */

// Check if 2 pixels are different with respect to their
// YUV representations
// returns 0: close; ~0: distant
static inline int
SCALE_(DiffYUV) (Uint32 yuv1, Uint32 yuv2)
{
	sint32 ret;

	__asm__ (
		// load YUV pixels
		"movd       %1, %%mm1    \n\t"
		"movq       %%mm1, %%mm4 \n\t"
		"movd       %2, %%mm2    \n\t"
		// abs difference between channels
		"psubusb    %%mm2, %%mm1 \n\t"
		"psubusb    %%mm4, %%mm2 \n\t"
		CLR_UPPER32(D_REG)      "\n\t"
		"por        %%mm2, %%mm1 \n\t"
		// compare to threshold
		"psubusb    %3, %%mm1    \n\t"

		"movd       %%mm1, %%edx \n\t"
		// transform eax to 0 or ~0
		"xor        %%" A_REG ", %%" A_REG "\n\t"
		"or         %%" D_REG ", %%" D_REG "\n\t"
		"setz       %%al         \n\t"
		"dec        %%" A_REG "  \n\t"
	
	: /*0*/"=a" (ret)
	: /*1*/"rm" (yuv1), /*2*/"rm" (yuv2),
		/*3*/"m" (mmx_YUV_threshold)
	: "%" D_REG, "cc"
	);
	return ret;
}

// Bilinear weighted blend of four pixels
// Function produces 4 blended pixels (in 2x2 matrix) and writes them
// out to the surface
// Last version
static inline void
SCALE_(Blend_bilinear) (const Uint32* row0, const Uint32* row1,
				Uint32* dst_p, Uint32 dlen)
{
	__asm__ (
		// EL0: load pixels
		"movq       %0, %%mm1      \n\t" // EL0
		"movq       %%mm1, %%mm2   \n\t" // EL0: p[1] -> mm2
		 PREFETCH   (0x80%0)      "\n\t"
		"punpckhbw  %%mm0, %%mm2   \n\t" // EL0: p[1] -> mm2
		"punpcklbw  %%mm0, %%mm1   \n\t" // EL0: p[0] -> mm1
		"movq       %1, %%mm3      \n\t"
		"movq       %%mm3, %%mm4   \n\t" // EL0: p[3] -> mm4
		"movq       %%mm2, %%mm6   \n\t" // EL1.1: p[1] -> mm6
		 PREFETCH   (0x80%1)      "\n\t"
		"punpcklbw  %%mm0, %%mm3   \n\t" // EL0: p[2] -> mm3
		"movq       %%mm1, %%mm5   \n\t" // EL1.1: p[0] -> mm5
		"punpckhbw  %%mm0, %%mm4   \n\t" // EL0: p[3] -> mm4

		// EL1: cache p[0] + 3*(p[1] + p[2]) + p[3] in mm6
		"paddw      %%mm3, %%mm6   \n\t" // EL1.2: p[1] + p[2] -> mm6
		// EL1: cache p[0] + p[1] + p[2] + p[3] in mm7
		"movq       %%mm6, %%mm7   \n\t" // EL1.3: p[1] + p[2] -> mm7
		// EL1: cache p[1] + 3*(p[0] + p[3]) + p[2] in mm5
		"paddw      %%mm4, %%mm5   \n\t" // EL1.2: p[0] + p[3] -> mm5
		"psllw      $1, %%mm6      \n\t" // EL1.4: 2*(p[1] + p[2]) -> mm6
		"paddw      %%mm5, %%mm7   \n\t" // EL1.4: sum(p[]) -> mm7
		"psllw      $1, %%mm5      \n\t" // EL1.5: 2*(p[0] + p[3]) -> mm5
		"paddw      %%mm7, %%mm6   \n\t" // EL1.5: p[0] + 3*(p[1] + p[2]) + p[3] -> mm6
		"paddw      %%mm7, %%mm5   \n\t" // EL1.6: p[1] + 3*(p[0] + p[3]) + p[2] -> mm5

		// EL2: pixel 0 math -- (9*p[0] + 3*(p[1] + p[2]) + p[3]) / 16
		"psllw      $3, %%mm1      \n\t" // EL2.1: 8*p[0] -> mm1
		"paddw      %%mm6, %%mm1   \n\t" // EL2.2: 9*p[0] + 3*(p[1] + p[2]) + p[3] -> mm1
		"psrlw      $4, %%mm1      \n\t" // EL2.3: sum[0]/16 -> mm1

		// EL3: pixel 1 math -- (9*p[1] + 3*(p[0] + p[3]) + p[2]) / 16
		"psllw      $3, %%mm2      \n\t" // EL3.1: 8*p[1] -> mm2
		"paddw      %%mm5, %%mm2   \n\t" // EL3.2: 9*p[1] + 3*(p[0] + p[3]) + p[2] -> mm5
		"psrlw      $4, %%mm2      \n\t" // EL3.3: sum[1]/16 -> mm5
		
		// EL2/4: store pixels 0 & 1
		"packuswb   %%mm2, %%mm1   \n\t" // EL2/4: pack into bytes
		 MOVNTQ     (%%mm1, (%2)) "\n\t" // EL2/4: store 2 pixels

		// EL4: pixel 2 math -- (9*p[2] + 3*(p[0] + p[3]) + p[1]) / 16
		"psllw      $3, %%mm3      \n\t" // EL4.1: 8*p[2] -> mm3
		"paddw      %%mm5, %%mm3   \n\t" // EL4.2: 9*p[2] + 3*(p[0] + p[3]) + p[1] -> mm3
		"psrlw      $4, %%mm3      \n\t" // EL4.3: sum[2]/16 -> mm3

		// EL5: pixel 3 math -- (9*p[3] + 3*(p[1] + p[2]) + p[0]) / 16
		"psllw      $3, %%mm4      \n\t" // EL5.1: 8*p[3] -> mm4
		"paddw      %%mm6, %%mm4   \n\t" // EL5.2: 9*p[3] + 3*(p[1] + p[2]) + p[0] -> mm4
		"psrlw      $4, %%mm4      \n\t" // EL5.3: sum[3]/16 -> mm4

		// EL4/5: store pixels 2 & 3
		"packuswb   %%mm4, %%mm3   \n\t" // EL4/5: pack into bytes
		 MOVNTQ     (%%mm3, (%2,%3,4)) "\n\t" // EL4/5: store 2 pixels
	
	: /* nothing */
	: /*0*/"m" (*row0), /*1*/"m" (*row1), /*2*/"r" (dst_p),
			/*3*/"r" ((unsigned long)dlen) /* 'long' is for proper reg alloc on amd64 */
	: "memory"
	);
}

#undef A_REG
#undef D_REG
#undef CLR_UPPER32

#endif // GCC_ASM

#endif /* SCALEMMX_H_ */
