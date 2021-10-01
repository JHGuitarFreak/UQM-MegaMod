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

#include "port.h"

#define UNICODE_INTERNAL
#include "libs/unicode.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "libs/log.h"
#include "libs/misc.h"


// Resynchronise (skip everything starting with 0x10xxxxxx):
static inline void
resyncUTF8(const unsigned char **ptr) {
	while ((**ptr & 0xc0) == 0x80)
		(*ptr)++;
}

// Get one character from a UTF-8 encoded string.
// *ptr will point to the start of the next character.
// Returns 0 if the encoding is bad. This can be distinguished from the
// '\0' character by checking whether **ptr == '\0' before calling this
// function.
UniChar
getCharFromString(const unsigned char **ptr) {
	const unsigned char *origPtr = *ptr;
	UniChar result, errData;

	if (**ptr < 0x80) {
		// 0xxxxxxx, regular ASCII
		result = **ptr;
		(*ptr)++;

		return result;
	}

	if ((**ptr & 0xe0) == 0xc0) {
		// 110xxxxx; 10xxxxxx must follow
		// Value between 0x00000080 and 0x000007ff (inclusive)
		result = **ptr & 0x1f;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00000080) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}

	if ((**ptr & 0xf0) == 0xe0) {
		// 1110xxxx; 10xxxxxx 10xxxxxx must follow
		// Value between 0x00000800 and 0x0000ffff (inclusive)
		result = **ptr & 0x0f;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00000800) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}

	if ((**ptr & 0xf8) == 0xf0) {
		// 11110xxx; 10xxxxxx 10xxxxxx 10xxxxxx must follow
		// Value between 0x00010000 and 0x0010ffff (inclusive)
		result = **ptr & 0x07;
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if ((**ptr & 0xc0) != 0x80)
			goto err;
		result = (result << 6) | ((**ptr) & 0x3f);
		(*ptr)++;
		
		if (result < 0x00010000) {
			// invalid encoding - must reject
			goto err;
		}
		return result;
	}
	
err:
	errData = origPtr[0] * 0x1000000;
	if (origPtr[0] && origPtr[1])
		errData &= origPtr[1] * 0x10000;
	if (origPtr[0] && origPtr[1] && origPtr[2])
		errData &= origPtr[2] * 0x100;
	if (origPtr[0] && origPtr[1] && origPtr[2] && origPtr[3])
		errData &= origPtr[3];
	log_add(log_Warning, "Warning: Invalid UTF8 sequence: result 0x%x last byte 0x%02x str 0x%08x %s", result, (unsigned)(**ptr), errData, origPtr);
	
	// Resynchronise (skip everything starting with 0x10xxxxxx):
	resyncUTF8(ptr);
	
	return 0;
}

UniChar
getCharFromStringN(const unsigned char **ptr, const unsigned char *end) {
	size_t numBytes;

	if (*ptr == end)
		goto err;

	if (**ptr < 0x80) {
		numBytes = 1;
	} else if ((**ptr & 0xe0) == 0xc0) {
		numBytes = 2;
	} else if ((**ptr & 0xf0) == 0xe0) {
		numBytes = 3;
	} else if ((**ptr & 0xf8) == 0xf0) {
		numBytes = 4;
	} else
		goto err;

	if (*ptr + numBytes > end)
		goto err;

	return getCharFromString(ptr);

err:
	*ptr = end;
	return 0;
}

// Get one line from a string.
// A line is terminated with either CRLF (DOS/Windows),
// LF (Unix, MacOS X), or CR (old MacOS).
// The end of the string is reached when **startNext == '\0'.
// NULL is returned if the string is not valid UTF8. In this case
// *end points to the first invalid character (or the character before if
// it was a LF), and *startNext to the start of the next (possibly invalid
// too) character.
unsigned char *
getLineFromString(const unsigned char *start, const unsigned char **end,
		const unsigned char **startNext) {
	const unsigned char *ptr = start;
	const unsigned char *lastPtr;
	UniChar ch;

	// Search for the first newline.
	for (;;) {
		if (*ptr == '\0') {
			*end = ptr;
			*startNext = ptr;
			return (unsigned char *) unconst(start);
		}
		lastPtr = ptr;
		ch = getCharFromString(&ptr);
		if (ch == '\0') {
			// Bad string
			*end = lastPtr;
			*startNext = ptr;
			return NULL;
		}
		if (ch == '\n') {
			*end = lastPtr;
			if (*ptr == '\0'){
				// LF at the end of the string.
				*startNext = ptr;
				return (unsigned char *) unconst(start);
			}
			ch = getCharFromString(&ptr);
			if (ch == '\0') {
				// Bad string
				return NULL;
			}
			if (ch == '\r') {
				// LFCR
				*startNext = ptr;
			} else {
				// LF
				*startNext = *end;
			}
			return (unsigned char *) unconst(start);
		} else if (ch == '\r') {
			*end = lastPtr;
			*startNext = ptr;
			return (unsigned char *) unconst(start);
		} // else: a normal character
	}
}

size_t
utf8StringCount(const unsigned char *start) {
	size_t count = 0;
	UniChar ch;

	for (;;) {
		ch = getCharFromString(&start);
		if (ch == '\0')
			return count;
		count++;
	}
}

size_t
utf8StringCountN(const unsigned char *start, const unsigned char *end) {
	size_t count = 0;
	UniChar ch;

	for (;;) {
		ch = getCharFromStringN(&start, end);
		if (ch == '\0')
			return count;
		count++;
	}
}

// Locates a unicode character (ch) in a UTF-8 string (pStr)
// returns the char positions when found
//  -1 when not found
int
utf8StringPos (const unsigned char *pStr, UniChar ch)
{
	int pos;
 
	for (pos = 0; *pStr != '\0'; ++pos)
	{
		if (getCharFromString (&pStr) == ch)
			return pos;
	}

	if (ch == '\0' && *pStr == '\0')
		return pos;

	return -1;
}

// Safe version of strcpy(), somewhat analogous to strncpy()
// except it guarantees a 0-term when size > 0
// when size == 0, returns NULL
// BUG: this may result in the last character being only partially in the
// buffer
unsigned char *
utf8StringCopy (unsigned char *dst, size_t size, const unsigned char *src)
{
	if (size == 0)
		return 0;

	strncpy ((char *) dst, (const char *) src, size);
	dst[size - 1] = '\0';
	
	return dst;
}

// TODO: this is not implemented with respect to collating order
int
utf8StringCompare (const unsigned char *str1, const unsigned char *str2)
{
#if 0
	// UniChar comparing version
	UniChar ch1;
	UniChar ch2;

	for (;;)
	{
		int cmp;
		
		ch1 = getCharFromString(&str1);
		ch2 = getCharFromString(&str2);
		if (ch1 == '\0' || ch2 == '\0')
			break;

		cmp = utf8CompareChar (ch1, ch2);
		if (cmp != 0)
			return cmp;
	}

	if (ch1 != '\0')
	{
		// ch2 == '\0'
		// str2 ends, str1 continues
		return 1;
	}
	
	if (ch2 != '\0')
	{
		// ch1 == '\0'
		// str1 ends, str2 continues
		return -1;
	}
	
	// ch1 == '\0' && ch2 == '\0'.
	// Strings match completely.
	return 0;
#else
	// this will do for now
	return strcmp ((const char *) str1, (const char *) str2);
#endif
}

unsigned char *
skipUTF8Chars(const unsigned char *ptr, size_t num) {
	UniChar ch;
	const unsigned char *oldPtr;

	while (num--) {
		oldPtr = ptr;
		ch = getCharFromString(&ptr);
		if (ch == '\0')
			return (unsigned char *) unconst(oldPtr);
	}
	return (unsigned char *) unconst(ptr);
}

// Decodes a UTF-8 string (start) into a unicode character string (wstr)
// returns number of chars decoded and stored, not counting 0-term
// any chars that do not fit are truncated
// wide string term 0 is always appended, unless the destination
// buffer is 0 chars long
size_t
getUniCharFromStringN(UniChar *wstr, size_t maxcount,
		const unsigned char *start, const unsigned char *end)
{
	UniChar *next;

	if (maxcount == 0)
		return 0;

	// always leave room for 0-term
	--maxcount;

	for (next = wstr; maxcount > 0; ++next, --maxcount)
	{
		*next = getCharFromStringN(&start, end);
		if (*next == 0)
			break;
	}

	*next = 0; // term

	return next - wstr;
}

// See getStringFromWideN() for functionality
//  the only difference is that the source string (start) length is
//  calculated by searching for 0-term
size_t
getUniCharFromString(UniChar *wstr, size_t maxcount,
		const unsigned char *start)
{
	UniChar *next;

	if (maxcount == 0)
		return 0;

	// always leave room for 0-term
	--maxcount;

	for (next = wstr; maxcount > 0; ++next, --maxcount)
	{
		*next = getCharFromString(&start);
		if (*next == 0)
			break;
	}

	*next = 0; // term

	return next - wstr;
}

// Encode one wide character into UTF-8
// returns number of bytes used in the buffer,
//  0  : invalid or unsupported char
//  <0 : negative of bytes needed if buffer too small
// string term '\0' is *not* appended or counted
int
getStringFromChar(unsigned char *ptr, size_t size, UniChar ch)
{
	int i;
	static const struct range_def
	{
		UniChar lim;
		int marker;
		int mask;
	}
	ranges[] = 
	{
		{0x0000007f, 0x00, 0x7f},
		{0x000007ff, 0xc0, 0x1f},
		{0x0000ffff, 0xe0, 0x0f},
		{0x001fffff, 0xf0, 0x07},
		{0x03ffffff, 0xf8, 0x03},
		{0x7fffffff, 0xfc, 0x01},
		{0x00000000, 0x00, 0x00} // term
	};
	const struct range_def *def;

	// lookup the range
	for (i = 0, def = ranges; ch > def->lim && def->mask != 0; ++i, ++def)
		;
	if (def->mask == 0)
	{	// invalid or unsupported char
		log_add(log_Warning, "Warning: Invalid or unsupported unicode "
				"char (%lu)", (unsigned long) ch);
		return 0;
	}

	if ((size_t)i + 1 > size)
		return -(i + 1);

	// unrolled for speed
	switch (i)
	{
		case 5: ptr[5] = (ch & 0x3f) | 0x80;
				ch >>= 6;
		case 4: ptr[4] = (ch & 0x3f) | 0x80;
				ch >>= 6;
		case 3: ptr[3] = (ch & 0x3f) | 0x80;
				ch >>= 6;
		case 2: ptr[2] = (ch & 0x3f) | 0x80;
				ch >>= 6;
		case 1: ptr[1] = (ch & 0x3f) | 0x80;
				ch >>= 6;
		case 0: ptr[0] = (ch & def->mask) | def->marker;
	}

	return i + 1;
}

// Encode a wide char string (wstr) into a UTF-8 string (ptr)
// returns number of bytes used in the buffer (includes 0-term)
// any chars that do not fit are truncated
// string term '\0' is always appended, unless the destination
// buffer is 0 bytes long
size_t
getStringFromWideN(unsigned char *ptr, size_t size,
		const UniChar *wstr, size_t count)
{
	unsigned char *next;
	int used;

	if (size == 0)
		return 0;

	// always leave room for 0-term
	--size;
	
	for (next = ptr; size > 0 && count > 0;
			size -= used, next += used, --count, ++wstr)
	{
		used = getStringFromChar(next, size, *wstr);
		if (used < 0)
			break; // not enough room
		if (used == 0)
		{	// bad char?
			*next = '?';
			used = 1;
		}
	}

	*next = '\0'; // term

	return next - ptr + 1;
}

// See getStringFromWideN() for functionality
//  the only difference is that the source string (wstr) length is
//  calculated by searching for 0-term
size_t
getStringFromWide(unsigned char *ptr, size_t size, const UniChar *wstr)
{
	const UniChar *end;

	for (end = wstr; *end != 0; ++end)
		;
	
	return getStringFromWideN(ptr, size, wstr, (end - wstr));
}

int
UniChar_isGraph(UniChar ch)
{	// this is not technically sufficient, but close enough for us
	// we'll consider all non-control (CO and C1) chars in 'graph' class
	// except for the "Private Use Area" (0xE000 - 0xF8FF)

	// TODO: The private use area is really only glommed by OS X,
	// and even there, not all of it.  (Delete and Backspace both
	// end up producing characters there -- see bug #942 for the
	// gory details.)
	return (ch > 0xa0 && (ch < 0xE000 || ch > 0xF8FF)) ||
			(ch > 0x20 && ch < 0x7f);
}

int
UniChar_isPrint(UniChar ch)
{	// this is not technically sufficient, but close enough for us
	// chars in 'print' class are 'graph' + 'space' classes
	// the only space we currently have defined is 0x20
	return (ch == 0x20) || UniChar_isGraph(ch);
}

UniChar
UniChar_toUpper(UniChar ch)
{	// this is a very basic Latin-1 implementation
	// just to get things going
	return (ch < 0x100) ? (UniChar) toupper((int) ch) : ch;
}

UniChar
UniChar_toLower(UniChar ch)
{	// this is a very basic Latin-1 implementation
	// just to get things going
	return (ch < 0x100) ? (UniChar) tolower((int) ch) : ch;
}

