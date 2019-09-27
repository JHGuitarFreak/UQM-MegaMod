//Copyright Paul Reiche, Fred Ford. 1992-2002

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

#ifndef LIBS_COMPILER_H_
#define LIBS_COMPILER_H_

#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint8             BYTE;
typedef uint8             UBYTE;
typedef sint8             SBYTE;
typedef uint16            UWORD;
typedef sint16            SWORD;
typedef uint32            DWORD;
typedef sint32           SDWORD;
typedef uint64            QWORD;
typedef sint64            FWORD;

typedef UWORD             COUNT;
typedef SWORD              SIZE;

typedef char            UNICODE;


typedef enum
{
	FALSE = 0,
	TRUE
} BOOLEAN;

typedef void     (*PVOIDFUNC) (void);
typedef BOOLEAN  (*PBOOLFUNC) (void);
typedef BYTE     (*PBYTEFUNC) (void);
typedef UWORD    (*PUWORDFUNC) (void);
typedef SWORD    (*PSWORDFUNC) (void);
typedef DWORD    (*PDWORDFUNC) (void);

#define MAKE_BYTE(lo, hi)   ((BYTE) (((BYTE) (hi) << (BYTE) 4) | (BYTE) (lo)))
#define LONIBBLE(x)  ((BYTE) ((BYTE) (x) & (BYTE) 0x0F))
#define HINIBBLE(x)  ((BYTE) ((BYTE) (x) >> (BYTE) 4))
#define MAKE_WORD(lo, hi)   ((UWORD) ((BYTE) (hi) << 8) | (BYTE) (lo))
#define LOBYTE(x)    ((BYTE) ((UWORD) (x)))
#define HIBYTE(x)    ((BYTE) ((UWORD) (x) >> 8))
#define MAKE_DWORD(lo, hi)  (((DWORD) (hi) << 16) | (UWORD) (lo))
#define LOWORD(x)    ((UWORD) ((DWORD) (x)))
#define HIWORD(x)    ((UWORD) ((DWORD) (x) >> 16))


// To be moved to port.h:
// _ALIGNED_ANY specifies an alignment suitable for any type
// _ALIGNED_ON specifies a caller-supplied alignment (should be a power of 2)
#if defined(__GNUC__)
#	define _PACKED __attribute__((packed))
#	define _ALIGNED_ANY __attribute__((aligned))
#	define _ALIGNED_ON(bytes) __attribute__((aligned(bytes)))
#elif defined(_MSC_VER)
#	define _ALIGNED_ANY
//#	define _ALIGNED_ON(bytes) __declspec(align(bytes))
			// __declspec(align(bytes)) expects a constant. 'sizeof (type)'
			// will not do. This is something that needs some attention,
			// once we find someone with a 64 bits Windows machine.
			// Leaving it alone for now.
#	define _PACKED
#	define _ALIGNED_ON(bytes)
#elif defined(__ARMCC__)
#	define _PACKED __attribute__((packed))
#	define _ALIGNED_ANY __attribute__((aligned))
#	define _ALIGNED_ON(bytes) __attribute__((aligned(bytes)))
#elif defined(__WINSCW__)
#	define _PACKED
#	define _ALIGNED_ANY
#	define _ALIGNED_ON(bytes)
#endif

#if defined(__cplusplus)
}
#endif

#endif /* LIBS_COMPILER_H_ */

