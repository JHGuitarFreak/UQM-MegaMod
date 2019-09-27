/*
 * Copyright (C) 2003  Serge van den Boom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 * Nota bene: later versions of the GNU General Public License do not apply
 * to this program.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <sys/types.h>
#include <string.h>

#include "uioutils.h"
#include "mem.h"
#include "paths.h"
#include "uioport.h"

/**
 * Concatenate two strings into a newly allocated buffer.
 *
 * @param[in]  first   The first (left) string, '\0' terminated.
 * @param[in]  second  The second (right) string, '\0' terminated.
 *
 * @returns A newly allocated string consisting of the concatenation of
 * 		'first' and 'second', to be freed using uio_free().
 */
char *
strcata(const char *first, const char *second) {
	char *result, *resPtr;
	size_t firstLen, secondLen;
	
	firstLen = strlen(first);
	secondLen = strlen(second);
	result = uio_malloc(firstLen + secondLen + 1);
	resPtr = result;

	memcpy(resPtr, first, firstLen);
	resPtr += firstLen;

	memcpy(resPtr, second, secondLen);
	resPtr += secondLen;

	*resPtr = '\0';
	return result;
}

// returns a copy of a generic array 'array' with 'element' inserted in
// position 'insertPos'
void *
insertArray(const void *array, size_t oldNumElements, int insertPos,
		const void *element, size_t elementSize) {
	void *newArray, *newArrayPtr;
	const void *arrayPtr;
	size_t preInsertSize;

	newArray = uio_malloc((oldNumElements + 1) * elementSize);
	preInsertSize = insertPos * elementSize;
	memcpy(newArray, array, preInsertSize);
	newArrayPtr = (char *) newArray + preInsertSize;
	arrayPtr = (const char *) array + preInsertSize;
	memcpy(newArrayPtr, element, elementSize);
	newArrayPtr = (char *) newArrayPtr + elementSize;
	memcpy(newArrayPtr, arrayPtr,
			(oldNumElements - insertPos) * elementSize);
	return newArray;
}

// returns a copy of a pointer array 'array' with 'element' inserted in
// position 'insertPos'
void **
insertArrayPointer(const void **array, size_t oldNumElements, int insertPos,
		const void *element) {
	void **newArray, **newArrayPtr;
	const void **arrayPtr;
	size_t preInsertSize;

	newArray = uio_malloc((oldNumElements + 1) * sizeof (void *));
	preInsertSize = insertPos * sizeof (void *);
	memcpy(newArray, array, preInsertSize);
	newArrayPtr = newArray + insertPos;
	arrayPtr = array + insertPos;
	*newArrayPtr = unconst(element);
	newArrayPtr++;
	memcpy(newArrayPtr, arrayPtr,
			(oldNumElements - insertPos) * sizeof (void *));
	return newArray;
}

// returns a copy of a generic array 'array' with 'numExclude' elements,
// starting from startpos, removed.
void *
excludeArray(const void *array, size_t oldNumElements, int startPos,
		int numExclude, size_t elementSize) {
	void *newArray, *newArrayPtr;
	const void *arrayPtr;
	size_t preExcludeSize;

	newArray = uio_malloc((oldNumElements - numExclude) * elementSize);
	preExcludeSize = startPos * elementSize;
	memcpy(newArray, array, preExcludeSize);
	newArrayPtr = (char *) newArray + preExcludeSize;
	arrayPtr = (const char *) array +
			(startPos + numExclude) * sizeof (elementSize);
	memcpy(newArrayPtr, arrayPtr,
			(oldNumElements - startPos - numExclude) * elementSize);
	return newArray;
}

// returns a copy of a pointer array 'array' with 'numExclude' elements,
// starting from startpos, removed.
void **
excludeArrayPointer(const void **array, size_t oldNumElements, int startPos,
		int numExclude) {
	void **newArray;

	newArray = uio_malloc((oldNumElements - numExclude) * sizeof (void *));
	memcpy(newArray, array, startPos * sizeof (void *));
	memcpy(&newArray[startPos], &array[startPos + numExclude],
			(oldNumElements - startPos - numExclude) * sizeof (void *));
	return newArray;
}

// If the given DOS date/time is invalid, the result is unspecified,
// but the function won't crash.
time_t
dosToUnixTime(uio_uint16 date, uio_uint16 tm) {
	// DOS date has the following format:
	//   bits 0-4 specify the number of the day in the month (1-31).
	//   bits 5-8 specify the number of the month in the year (1-12).
	//   bits 9-15 specify the year number since 1980 (0-127)
	// DOS time has the fillowing format:
	//   bits 0-4 specify the number of seconds/2 in the minute (0-29)
	//       (only accurate on 2 seconds)
	//   bits 5-10 specify the number of minutes in the hour (0-59)
	//   bits 11-15 specify the number of hours since midnight (0-23)

	int year, month, day;
	int hours, minutes, seconds;
	long result;

	static const int daysUntilMonth[] = {
			0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334,
			334, 334, 334, 334 };
			// The last 4 entries are there so that there's no
			// invalid memory access if the date is invalid.

	year = date >> 9;
	month = ((date >> 5) - 1) & 0x0f;  // Number in [0..15]
	day = (date - 1) & 0x1f;  // Number in [0..31]
	hours = tm >> 11;
	minutes = (tm >> 5) & 0x3f;
	seconds = (tm & 0x1f) * 2; // Even number in [0..62]

	result = year * 365 + daysUntilMonth[month] + day;
			// Count the (non-leap) days in all those years

	// Add a leapday for each 4th year
	if (year % 4 == 0 && month <= 2) {
		// The given date is a leap-year but the leapday hasn't occured yet.
		result += year / 4;
	} else {
		result += 1 + year / 4;
	}
	// result now is the number of days between 1980-01-01 and the given day.

	// Add the days between 1970-01-01 and 1980-01-01
	// (2 leapdays in this period)
	result += 365 * 10 + 2;

	result = (result * 24) + hours;  // days to hours
	result = (result * 60) + minutes;  // hours to minutes
	result = (result * 60) + seconds;  // minutes to seconds
	
	return (time_t) result;	
}

char *
dosToUnixPath(const char *path) {
	const char *srcPtr;
	char *result, *dstPtr;
	size_t skip;

	result = uio_malloc(strlen(path) + 1);
	srcPtr = path;
	dstPtr = result;

	// A UNC path will look like this: "\\server\share/..."; the first two
	// characters will be backslashes, and the separator between the server
	// and the share too. The rest will be slashes.
	// The goal is that at every forward slash, the path should be
	// stat()'able.
	skip = uio_skipUNCServerShare(srcPtr);
	if (skip != 0) {
		char *slash;
		memcpy(dstPtr, srcPtr, skip);

		slash = memchr(srcPtr + 2, '/', skip - 2);
		if (slash != NULL)
			*slash = '\\';

		srcPtr += skip;
		dstPtr += skip;
	}

	while (*srcPtr != '\0') {
		if (*srcPtr == '\\') {
			*dstPtr = '/';
		} else
			*dstPtr = *srcPtr;
		srcPtr++;
		dstPtr++;
	}
	*dstPtr = '\0';
	return result;
}


