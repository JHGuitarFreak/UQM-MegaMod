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
 * This file contains definitions that might be included in one system, but
 * omited in another.
 *
 * Created by Serge van den Boom
 */

#include "port.h"

#include <ctype.h>
#include <errno.h>
#ifdef _MSC_VER
#	include <stdarg.h>
#	include <stdio.h>
#endif  /* _MSC_VER */
#include <stdlib.h>
#include <string.h>
#if !defined (_MSC_VER) && !defined (HAVE_READDIR_R)
#	include <dirent.h>
#endif

#ifndef HAVE_STRUPR
char *
strupr (char *str)
{
	char *ptr;
	
	ptr = str;
	while (*ptr)
	{
		*ptr = (char) toupper (*ptr);
		ptr++;
	}
	return str;
}
#endif

#ifndef HAVE_SETENV
int
setenv (const char *name, const char *value, int overwrite)
{
	char *string, *ptr;
	size_t nameLen, valueLen;

	if (!overwrite)
	{
		char *old;

		old = getenv (name);
		if (old != NULL)
			return 0;
	}

	nameLen = strlen (name);	
	valueLen = strlen (value);

	string = malloc (nameLen + valueLen + 2);
			// "NAME=VALUE\0"
			// putenv() does NOT make a copy, but uses the string passed.

	ptr = string;

	strcpy (string, name);
	ptr += nameLen;

	*ptr = '=';
	ptr++;
	
	strcpy (ptr, value);
	
	return putenv (string);
}
#endif

#if !defined (_MSC_VER) && !defined (HAVE_READDIR_R)
// NB. This function calls readdir() directly, and as such has the same
//     reentrance issues as that function. For the purposes of UQM it will
//     do though.
//     Note the POSIX requires that "The pointer returned by readdir()
//     points to data which may be overwritten by another call to
//     readdir( ) on the same directory stream. This data is not
//     overwritten by another call to readdir() on a different directory
//     stream."
// NB. This function makes an extra copy of the dirent and will hence be
//     slower than a direct call to readdir() or readdir_r().
int
readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result) {
	struct dirent *readdir_entry;

	readdir_entry = readdir(dirp);
	if (readdir_entry == NULL) {
		*result = NULL;
		return errno;
	}

	*entry = *readdir_entry;
	*result = entry;
	return 0;
}
#endif

#ifdef _MSC_VER
#if (_MSC_VER <= 1800)
// MSVC does not have snprintf() and vsnprintf(). It does have a _snprintf()
// and _vsnprintf(), but these do not terminate a truncated string as
// the C standard prescribes.
int
snprintf(char *str, size_t size, const char *format, ...)
{
	int result;
	va_list args;
	
	va_start (args, format);
	result = _vsnprintf (str, size, format, args);
	if (str != NULL && size != 0)
		str[size - 1] = '\0';
	va_end (args);

	return result;
}

int
vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int result = _vsnprintf (str, size, format, args);
	if (str != NULL && size != 0)
		str[size - 1] = '\0';
	return result;
}
#endif
#endif  /* _MSC_VER */

