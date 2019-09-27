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

#ifndef PLATFORM_H_
#define PLATFORM_H_

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(USE_PLATFORM_ACCEL)
#	if defined(__GNUC__) && (defined(i386) || defined(__x86_64__))
#		define MMX_ASM
#		define GCC_ASM
#	elif (_MSC_VER >= 1100) && defined(_M_IX86)
//	All 32bit AMD chips are IX86 equivalents
//	We do not enable MSVC/MMX_ASM for _M_AMD64 as of now
#		define MMX_ASM
#		define MSVC_ASM
#	endif
//	other realistic possibilities for MSVC compiler are
//		_M_AMD64 (AMD x86-64), _M_IA64 (Intel Arch 64)
#endif

typedef enum
{
	PLATFORM_NULL = 0,
	PLATFORM_C,
	PLATFORM_MMX,
	PLATFORM_SSE,
	PLATFORM_3DNOW,
	PLATFORM_ALTIVEC,

	PLATFORM_LAST = PLATFORM_ALTIVEC

} PLATFORM_TYPE;

extern PLATFORM_TYPE force_platform;

#if defined(__cplusplus)
}
#endif

#endif /* PLATFORM_H_ */
