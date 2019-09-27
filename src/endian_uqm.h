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

/*
 * Endian swapping, taken from SDL-1.2.5 sources and modified
 * Original copyright (C) Sam Lantinga
 */

#ifndef ENDIAN_UQM_H_
#define ENDIAN_UQM_H_

#include "config.h"
#include "types.h"

#if defined (__APPLE__) && defined (__GNUC__)
// When using the MacOS gcc compiler to build universal binaries,
// each file will be compiled once for each platform.
// This means that checking endianness beforehand from build.sh will not do,
// but fortunately, gcc defines __BIG_ENDIAN__ or __LITTLE_ENDIAN__ on
// this platform.
#	if defined(__BIG_ENDIAN__)
#		undef WORDS_BIGENDIAN
#		define WORDS_BIGENDIAN
#	elif defined(__LITTLE_ENDIAN__)
#		undef WORDS_BIGENDIAN
#	else
		// Neither __BIG_ENDIAN__ nor __LITTLE_ENDIAN__ is defined.
		// Fallback to using the build.sh defined value.
#	endif
#endif  /* __APPLE__ */

#if defined(_MSC_VER) || defined(__BORLANDC__) || \
    defined(__DMC__) || defined(__SC__) || \
    defined(__WATCOMC__) || defined(__LCC__)
#ifndef __inline__
#define __inline__	__inline
#endif
#endif

/* The macros used to swap values */
/* Try to use superfast macros on systems that support them */
#ifdef linux
#include <endian.h>
#ifdef __arch__swab16
#define UQM_Swap16  __arch__swab16
#endif
#ifdef __arch__swab32
#define UQM_Swap32  __arch__swab32
#endif
#endif /* linux */

#if defined(__cplusplus)
extern "C" {
#endif

/* Use inline functions for compilers that support them, and static
   functions for those that do not.  Because these functions become
   static for compilers that do not support inline functions, this
   header should only be included in files that actually use them.
*/
#ifndef UQM_Swap16
static __inline__ uint16 UQM_Swap16(uint16 D)
{
	return((D<<8)|(D>>8));
}
#endif
#ifndef UQM_Swap32
static __inline__ uint32 UQM_Swap32(uint32 D)
{
	return((D<<24)|((D<<8)&0x00FF0000)|((D>>8)&0x0000FF00)|(D>>24));
}
#endif
#ifdef UQM_INT64
#ifndef UQM_Swap64
static __inline__ uint64 UQM_Swap64(uint64 val)
{
	uint32 hi, lo;

	/* Separate into high and low 32-bit values and swap them */
	lo = (uint32)(val&0xFFFFFFFF);
	val >>= 32;
	hi = (uint32)(val&0xFFFFFFFF);
	val = UQM_Swap32(lo);
	val <<= 32;
	val |= UQM_Swap32(hi);
	return(val);
}
#endif
#else
#ifndef UQM_Swap64
/* This is mainly to keep compilers from complaining in SDL code.
   If there is no real 64-bit datatype, then compilers will complain about
   the fake 64-bit datatype that SDL provides when it compiles user code.
*/
#define UQM_Swap64(X)	(X)
#endif
#endif /* UQM_INT64 */


/* Byteswap item from the specified endianness to the native endianness
 * or vice versa.
 */
#ifndef WORDS_BIGENDIAN
#define UQM_SwapLE16(X)	(X)
#define UQM_SwapLE32(X)	(X)
#define UQM_SwapLE64(X)	(X)
#define UQM_SwapBE16(X)	UQM_Swap16(X)
#define UQM_SwapBE32(X)	UQM_Swap32(X)
#define UQM_SwapBE64(X)	UQM_Swap64(X)
#else
#define UQM_SwapLE16(X)	UQM_Swap16(X)
#define UQM_SwapLE32(X)	UQM_Swap32(X)
#define UQM_SwapLE64(X)	UQM_Swap64(X)
#define UQM_SwapBE16(X)	(X)
#define UQM_SwapBE32(X)	(X)
#define UQM_SwapBE64(X)	(X)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* _ENDIAN_H */
