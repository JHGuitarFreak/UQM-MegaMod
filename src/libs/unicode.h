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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef UNICODE_H
#define UNICODE_H

#include "port.h"
#include "types.h"
		// for uint32
#include <sys/types.h>
		// For size_t

#if defined(__cplusplus)
extern "C" {
#endif

typedef uint32 UniChar;

#ifdef UNICODE_INTERNAL
#	define UNICODE_CHAR  unsigned char
#else
#	define UNICODE_CHAR  char
#endif

UniChar getCharFromString(const UNICODE_CHAR **ptr);
UniChar getCharFromStringN(const UNICODE_CHAR **ptr,
		const UNICODE_CHAR *end);
unsigned char *getLineFromString(const UNICODE_CHAR *start,
		const UNICODE_CHAR **end, const UNICODE_CHAR **startNext);
size_t utf8StringCount(const UNICODE_CHAR *start);
size_t utf8StringCountN(const UNICODE_CHAR *start,
		const UNICODE_CHAR *end);
int utf8StringPos (const UNICODE_CHAR *pStr, UniChar ch);
unsigned char *utf8StringCopy (UNICODE_CHAR *dst, size_t size,
		const UNICODE_CHAR *src);
int utf8StringCompare (const UNICODE_CHAR *str1, const UNICODE_CHAR *str2);
UNICODE_CHAR *skipUTF8Chars(const UNICODE_CHAR *ptr, size_t num);
size_t getUniCharFromString(UniChar *wstr, size_t maxcount,
		const UNICODE_CHAR *start);
size_t getUniCharFromStringN(UniChar *wstr, size_t maxcount,
		const UNICODE_CHAR *start, const UNICODE_CHAR *end);
int getStringFromChar(UNICODE_CHAR *ptr, size_t size, UniChar ch);
size_t getStringFromWideN(UNICODE_CHAR *ptr, size_t size,
		const UniChar *wstr, size_t count);
size_t getStringFromWide(UNICODE_CHAR *ptr, size_t size,
		const UniChar *wstr);
int UniChar_isGraph(UniChar ch);
int UniChar_isPrint(UniChar ch);
UniChar UniChar_toUpper(UniChar ch);
UniChar UniChar_toLower(UniChar ch);

#undef UNICODE_CHAR

#if defined(__cplusplus)
}
#endif

#endif  /* UNICODE_H */

