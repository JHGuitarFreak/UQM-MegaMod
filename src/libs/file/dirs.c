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

// Contains code handling directories

#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "port.h"
#include "config.h"
#include "filintrn.h"
#include "libs/compiler.h"
#include "libs/memlib.h"
#include "libs/misc.h"
#include "libs/log.h"

#ifdef HAVE_DRIVE_LETTERS
#	include <ctype.h>
			// For tolower()
#endif  /* HAVE_DRIVE_LETTERS */
#ifdef WIN32
#	include <direct.h>
			// For _getdcwd()
#else
#	include <pwd.h>
			// For getpwuid()
#endif

/* Try to find a suitable value for %APPDATA% if it isn't defined on
 * Windows.
 */
#define APPDATA_FALLBACK


static char *expandPathAbsolute (char *dest, size_t destLen, const char *src,
		size_t *skipSrc, int what);
static char *strrchr2(const char *start, int c, const char *end);


int
createDirectory(const char *dir, int mode)
{
	return MKDIR(dir, mode);
}

// make all components of the path if they don't exist already
// returns 0 on success, -1 on failure.
// on failure, some parts may still have been created.
int
mkdirhier (const char *path)
{
	char *buf;              // buffer
	char *ptr;              // end of the string in buf
	const char *pathstart;  // start of a component of path
	const char *pathend;    // first char past the end of a component of path
	size_t len;
	struct stat statbuf;
	
	len = strlen (path);
	buf = HMalloc (len + 2);  // one extra for possibly added '/'

	ptr = buf;
	pathstart = path;

#ifdef HAVE_DRIVE_LETTERS
	if (isDriveLetter(pathstart[0]) && pathstart[1] == ':')
	{
		// Driveletter + semicolon on Windows.
		// Copy as is; don't try to create directories for it.
		*(ptr++) = *(pathstart++);
		*(ptr++) = *(pathstart++);

		ptr[0] = '/';
		ptr[1] = '\0';
		if (stat (buf, &statbuf) == -1)
		{
			log_add (log_Error, "Can't stat \"%s\": %s", buf, strerror (errno));
			goto err;
		}
	}
	else
#endif  /* HAVE_DRIVE_LETTERS */
#ifdef HAVE_UNC_PATHS
	if (pathstart[0] == '\\' && pathstart[1] == '\\')
	{
		// Universal Naming Convention path. (\\server\share\...)
		// Copy the server part as is; don't try to create directories for
		// it, or stat it. Don't create a dir for the share either.
		*(ptr++) = *(pathstart++);
		*(ptr++) = *(pathstart++);

		// Copy the server part
		while (*pathstart != '\0' && *pathstart != '\\' && *pathstart != '/')
			*(ptr++) = *(pathstart++);
		
		if (*pathstart == '\0')
		{
			log_add (log_Error, "Incomplete UNC path \"%s\"", pathstart);
			goto err;
		}

		// Copy the path seperator.
		*(ptr++) = *(pathstart++);
	
		// Copy the share part
		while (*pathstart != '\0' && *pathstart != '\\' && *pathstart != '/')
			*(ptr++) = *(pathstart++);

		ptr[0] = '/';
		ptr[1] = '\0';
		if (stat (buf, &statbuf) == -1)
		{
			log_add (log_Error, "Can't stat \"%s\": %s", buf, strerror (errno));
			goto err;
		}
	}
#else
	{
		// Making sure that there is an 'else' case if HAVE_DRIVE_LETTERS is
		// defined.
	}
#endif  /* HAVE_UNC_PATHS */

	if (*pathstart == '/')
		*(ptr++) = *(pathstart++);

	if (*pathstart == '\0') {
		// path exists completely, nothing more to do
		goto success;
	}

	// walk through the path as long as the components exist
	while (1)
	{
		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		memcpy(ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
		
		if (stat (buf, &statbuf) == -1)
		{
			if (errno == ENOENT)
				break;
#ifdef __SYMBIAN32__
			// XXX: HACK: If we don't have access to a directory, we can
			// still have access to the underlying entries. We don't
			// actually know whether the entry is a directory, but I know of
			// no way to find out. We just pretend that it is; if we were
			// wrong, an error will occur when we try to do something with
			// the directory. That /should/ not be a problem, as any such
			// action should have its own error checking.
			if (errno != EACCES)
#endif			
			{
				log_add (log_Error, "Can't stat \"%s\": %s", buf,
						strerror (errno));
				goto err;
			}
		}
		
		if (*pathend == '\0')
			goto success;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			goto success;
	}
	
	// create all components left
	while (1)
	{
		if (createDirectory (buf, 0777) == -1)
		{
			log_add (log_Error, "Error: Can't create %s: %s", buf,
					strerror (errno));
			goto err;
		}

		if (*pathend == '\0')
			break;

		*ptr = '/';
		ptr++;
		pathstart = pathend + 1;
		while (*pathstart == '/')
			pathstart++;
		// pathstart is the next non-slash character

		if (*pathstart == '\0')
			break;

		pathend = strchr (pathstart, '/');
		if (pathend == NULL)
			pathend = path + len;
		
		memcpy (ptr, pathstart, pathend - pathstart);
		ptr += pathend - pathstart;
		*ptr = '\0';
	}

success:
	HFree (buf);
	return 0;

err:
	{
		int savedErrno = errno;
		HFree (buf);
		errno = savedErrno;
	}
	return -1;
}

// Get the user's home dir
// returns a pointer to a static buffer from either getenv() or getpwuid().
const char *
getHomeDir (void)
{
#ifdef WIN32
	return getenv ("HOME");
#else
	const char *home;
	struct passwd *pw;

	home = getenv ("HOME");
	if (home != NULL)
		return home;

	pw = getpwuid (getuid ());
	if (pw == NULL)
		return NULL;
	// NB: pw points to a static buffer.

	return pw->pw_dir;
#endif
}

// Performs various types of string expansions on a path.
// 'what' is an OR'd compination of the folowing flags, which
// specify what type of exmansions will be performed.
// EP_HOME - Expand '~' for home dirs.
// EP_ABSOLUTE - Make relative paths absolute
// EP_ENVVARS - Expand environment variables
// EP_DOTS - Process ".." and "."
// EP_SLASHES - Consider backslashes as path component separators.
//              They will be replaced by slashes.
// EP_SINGLESEP - Replace multiple consecutive path seperators (which POSIX
//                considers equivalent to a single one) by a single one.
// Additionally, there's EP_ALL, which indicates all of the above,
// and EP_ALL_SYSTEM, which does the same as EP_ALL, with the exception
// of EP_SLASHES, which will only be included if the operating system
// accepts backslashes as path terminators.
// Returns 0 on success.
// Returns -1 on failure, setting errno.
int
expandPath (char *dest, size_t len, const char *src, int what)
{
	char *destptr, *destend;
	char *buf = NULL;
	char *bufptr, *bufend;
	const char *srcend;

#define CHECKLEN(bufname, n) \
		if (bufname##ptr + (n) >= bufname##end) \
		{ \
			errno = ENAMETOOLONG; \
			goto err; \
		} \
		else \
			(void) 0

	destptr = dest;
	destend = dest + len;

	if (what & EP_ENVVARS)
	{
		buf = HMalloc (len);
		bufptr = buf;
		bufend = buf + len;
		while (*src != '\0')
		{
			switch (*src)
			{
#ifdef WIN32
				case '%':
				{
					/* Environment variable substitution in Windows */
					const char *end;     // end of env var name in src
					const char *envVar;
					char *envName;
					size_t envNameLen, envVarLen;
					
					src++;
					end = strchr (src, '%');
					if (end == NULL)
					{
						errno = EINVAL;
						goto err;
					}
					
					envNameLen = end - src;
					envName = HMalloc (envNameLen + 1);
					memcpy (envName, src, envNameLen + 1);
					envName[envNameLen] = '\0';
					envVar = getenv (envName);
					HFree (envName);

					if (envVar == NULL)
					{
#ifdef APPDATA_FALLBACK
						if (strncmp (src, "APPDATA", envNameLen) != 0)
						{
							// Substitute an empty string
							src = end + 1;
							break;
						}

						// fallback for when the APPDATA env var is not set
						// Using SHGetFolderPath or SHGetSpecialFolderPath
						// is problematic (not everywhere available).
						log_add (log_Warning, "Warning: %%APPDATA%% is not set. "
								"Falling back to \"%%USERPROFILE%%\\Application "
								"Data\"");
						envVar = getenv ("USERPROFILE");
						if (envVar != NULL)
						{
#define APPDATA_STRING "\\Application Data"
							envVarLen = strlen (envVar);
							CHECKLEN (buf,
									envVarLen + sizeof (APPDATA_STRING) - 1);
							strcpy (bufptr, envVar);
							bufptr += envVarLen;
							strcpy (bufptr, APPDATA_STRING);
							bufptr += sizeof (APPDATA_STRING) - 1;
							src = end + 1;
							break;
						}
						
						// fallback to "./userdata"
#define APPDATA_FALLBACK_STRING ".\\userdata"
						log_add (log_Warning,
								"Warning: %%USERPROFILE%% is not set. "
								"Falling back to \"%s\" for %%APPDATA%%",
								APPDATA_FALLBACK_STRING);
						CHECKLEN (buf, sizeof (APPDATA_FALLBACK_STRING) - 1);
						strcpy (bufptr, APPDATA_FALLBACK_STRING);
						bufptr += sizeof (APPDATA_FALLBACK_STRING) - 1;
						src = end + 1;
						break;

#else  /* !defined (APPDATA_FALLBACK) */
						// Substitute an empty string
						src = end + 1;
						break;
#endif  /* APPDATA_FALLBACK */
					}

					envVarLen = strlen (envVar);
					CHECKLEN (buf, envVarLen);
					strcpy (bufptr, envVar);
					bufptr += envVarLen;
					src = end + 1;
					break;
				}
#endif
#ifndef WIN32
				case '$':
				{
					const char *end;
					char *envName;
					size_t envNameLen;
					const char *envVar;
					size_t envVarLen;

					src++;
					if (*src == '{')
					{
						src++;
						end = strchr(src, '}');
						if (end == NULL)
						{
							errno = EINVAL;
							goto err;
						}
						envNameLen = end - src;
						end++;  // Skip the '}'
					}
					else
					{
						end = src;
						while ((*end >= 'A' && *end <= 'Z') ||
								(*end >= 'a' && *end <= 'z') ||
								(*end >= '0' && *end <= '9') ||
								*end == '_')
							end++;
						envNameLen = end - src;
					}

					envName = HMalloc (envNameLen + 1);
					memcpy (envName, src, envNameLen + 1);
					envName[envNameLen] = '\0';
					envVar = getenv (envName);
					HFree (envName);

					if (envVar != NULL)
					{
						envVarLen = strlen (envVar);
						CHECKLEN (buf, envVarLen);
						memcpy (bufptr, envVar, envVarLen);
						bufptr += envVarLen;
					}

					src = end;
					break;
				}
#endif
				default:
					CHECKLEN(buf, 1);
					*(bufptr++) = *(src++);
					break;
			}  // switch
		}  // while
		*bufptr = '\0';
		src = buf;
		srcend = bufptr;
	}  // if (what & EP_ENVVARS)
	else
		srcend = src + strlen (src);

	if (what & EP_HOME)
	{
		if (src[0] == '~')
		{
			const char *home;
			size_t homelen;
			
			if (src[1] != '/')
			{
				errno = EINVAL;
				goto err;
			}

			home = getHomeDir ();
			if (home == NULL)
			{
				errno = ENOENT;
				goto err;
			}
			homelen = strlen (home);
		
			if (what & EP_ABSOLUTE) {
				size_t skip;
				destptr = expandPathAbsolute (dest, destend - dest,
						home, &skip, what);
				if (destptr == NULL)
				{
					// errno is set
					goto err;
				}
				home += skip;
				what &= ~EP_ABSOLUTE;
						// The part after the '~' should not be seen
						// as absolute.
			}

			CHECKLEN (dest, homelen);
			memcpy (destptr, home, homelen);
			destptr += homelen;
			src++;  /* skip the ~ */
		}
	}
	
	if (what & EP_ABSOLUTE)
	{
		size_t skip;
		destptr = expandPathAbsolute (destptr, destend - destptr, src,
				&skip, what);
		if (destptr == NULL)
		{
			// errno is set
			goto err;
		}
		src += skip;
	}

	CHECKLEN (dest, srcend - src);
	memcpy (destptr, src, srcend - src + 1);
			// The +1 is for the '\0'. It is already taken into account by
			// CHECKLEN.
	
	if (what & EP_SLASHES)
	{
		/* Replacing backslashes in path by slashes. */
		destptr = dest;
#ifdef HAVE_UNC_PATHS
		{
			// A UNC path should always start with two backslashes
			// and have a backslash in between the server and share part.
			size_t skip = skipUNCServerShare (destptr);
			if (skip != 0)
			{
				char *slash = (char *) memchr (destptr + 2, '/', skip - 2);
				if (slash)
					*slash = '\\';
				destptr += skip;
			}
		}
#endif  /* HAVE_UNC_PATHS */
		while (*destptr != '\0')
		{
			if (*destptr == '\\')
				*destptr = '/';
			destptr++;
		}
	}
	
	if (what & EP_DOTS) {
		// At this point backslashes are already replaced by slashes if they
		// are specified to be path seperators.
		// Note that the path can only get smaller, so no size checks
		// need to be done.
		char *pathStart;
				// Start of the first path component, after any
				// leading slashes or drive letters.
		char *startPart;
		char *endPart;

		pathStart = dest;
#ifdef HAVE_DRIVE_LETTERS
		if (isDriveLetter(pathStart[0]) && (pathStart[1] == ':'))
		{
			pathStart += 2;
		}
		else
#endif  /* HAVE_DRIVE_LETTERS */
#ifdef HAVE_UNC_PATHS
		{
			// Test for a Universal Naming Convention path.
			pathStart += skipUNCServerShare(pathStart);
		}
#else
		{
			// Making sure that there is an 'else' case if HAVE_DRIVE_LETTERS is
			// defined.
		}
#endif  /* HAVE_UNC_PATHS */
		if (pathStart[0] == '/')
			pathStart++;

		startPart = pathStart;
		destptr = pathStart;
		for (;;)
		{
			endPart = strchr(startPart, '/');
			if (endPart == NULL)
				endPart = startPart + strlen(startPart);

			if (endPart - startPart == 1 && startPart[0] == '.')
			{
				// Found "." as path component. Ignore this component.
			}
			else if (endPart - startPart == 2 &&
					startPart[0] == '.' && startPart[1] == '.')
			{
				// Found ".." as path component. Remove the previous
				// component, and ignore this one.
				char *lastSlash;
				lastSlash = strrchr2(pathStart, '/', destptr - 1);
				if (lastSlash == NULL)
				{
					if (destptr == pathStart)
					{
						// We ran out of path components to back out of.
						errno = EINVAL;
						goto err;
					}
					destptr = pathStart;
				}
				else
				{
					destptr = lastSlash;
					if (*endPart == '/')
						destptr++;
				}
			}
			else
			{
				// A normal path component; copy it.
				// Using memmove as source and destination may overlap.
				memmove(destptr, startPart, endPart - startPart);
				destptr += (endPart - startPart);
				if (*endPart == '/')
				{
					*destptr = '/';
					destptr++;
				}
			}
			if (*endPart == '\0')
				break;
			startPart = endPart + 1;
		}
		*destptr = '\0';	
	}
	
	if (what & EP_SINGLESEP)
	{
		char *srcptr;
		srcptr = dest;
		destptr = dest;
		while (*srcptr != '\0')
		{
			char ch = *srcptr;
			*(destptr++) = *(srcptr++);
			if (ch == '/')
			{
				while (*srcptr == '/')
					srcptr++;
			}
		}
		*destptr = '\0';
	}
	
	HFree (buf);
	return 0;

err:
	if (buf != NULL) {
		int savedErrno = errno;
		HFree (buf);
		errno = savedErrno;
	}
	return -1;
}

#if defined(HAVE_DRIVE_LETTERS) && defined(HAVE_CWD_PER_DRIVE)
		// This code is only needed if we have a current working directory
		// per drive.
// letter is 0 based: 0 = A, 1 = B, ...
static bool
driveLetterExists(int letter)
{
	unsigned long drives;

	drives = _getdrives ();

	return ((drives >> letter) & 1) != 0;
}
#endif  /* if defined(HAVE_DRIVE_LETTERS) && defined(HAVE_CWD_PER_DRIVE) */

// helper for expandPath, expanding an absolute path
// returns a pointer to the end of the filled in part of dest.
static char *
expandPathAbsolute (char *dest, size_t destLen, const char *src,
		size_t *skipSrc, int what)
{
	const char *orgSrc;

	if (src[0] == '/' || ((what & EP_SLASHES) && src[0] == '\\'))
	{
		// Path is already absolute; nothing to do
		*skipSrc = 0;
		return dest;
	}

	orgSrc = src;
#ifdef HAVE_DRIVE_LETTERS
	if (isDriveLetter(src[0]) && (src[1] == ':'))
	{
		int letter;

		if (src[2] == '/' || src[2] == '\\')
		{
			// Path is already absolute (of the form "d:/"); nothing to do
			*skipSrc = 0;
			return dest;
		}

		// Path is of the form "d:path", without a (back)slash after the
		// semicolon.

#ifdef REJECT_DRIVE_PATH_WITHOUT_SLASH
		// We reject paths of the form "d:foo/bar".
		errno = EINVAL;
		return NULL;
#elif defined(HAVE_CWD_PER_DRIVE)
		// Paths of the form "d:foo/bar" are treated as "foo/bar" relative
		// to the working directory of d:.
		letter = tolower(src[0]) - 'a';

		// _getdcwd() should only be called on drives that exist.
		// This is weird though, because it means a race condition
		// in between the existance check and the call to _getdcwd()
		// cannot be avoided, unless a drive still exists for Windows
		// when the physical drive is removed.
		if (!driveLetterExists (letter))
		{
			errno = ENOENT;
			return NULL;
		}

		// Get the working directory for a specific drive.
		if (_getdcwd (letter + 1, dest, destLen) == NULL)
		{
			// errno is set
			return NULL;
		}

		src += 2;
#else  /* if !defined(HAVE_CWD_PER_DRIVE) */
		// We treat paths of the form "d:foo/bar" as "d:/foo/bar".
		if (destLen < 3) {
			errno = ERANGE;
			return NULL;
		}
		dest[0] = src[0];
		dest[1] = ':';
		dest[2] = '/';
		*skipSrc = 2;
		dest += 3;
		return dest;
#endif  /* HAVE_CWD_PER_DRIVE */
	}
	else
#endif  /* HAVE_DRIVE_LETTERS */
	{
		// Relative dir
		if (getcwd (dest, destLen) == NULL)
		{
			// errno is set
			return NULL;
		}
	}

	{
		size_t tempLen;
		tempLen = strlen (dest);
		if (tempLen == 0)
		{
			// getcwd() or _getdcwd() returned a 0-length string.
			errno = ENOENT;
			return NULL;
		}
		dest += tempLen;
		destLen -= tempLen;
	}
	if (dest[-1] != '/'
#ifdef BACKSLASH_IS_PATH_SEPARATOR
			&& dest[-1] != '\\'
#endif  /* BACKSLASH_IS_PATH_SEPARATOR */
			)
	{
		// Need to add a slash.
		// There's always space, as we overwrite the '\0' that getcwd()
		// always returns.
		dest[0] = '/';
		dest++;
		destLen--;
	}

	*skipSrc = (size_t) (src - orgSrc);
	return dest;
}

// As strrchr, but starts searching from the indicated end of the string.
static char *
strrchr2(const char *start, int c, const char *end) {
	for (;;) {
		end--;
		if (end < start)
			return (char *) NULL;
		if (*end == c)
			return (char *) unconst(end);
	}
}

#ifdef HAVE_UNC_PATHS
// returns 0 if the path is not a valid UNC path.
// Does not skip trailing slashes.
size_t
skipUNCServerShare(const char *inPath) {
	const char *path = inPath;

	// Skip the initial two backslashes.
	if (path[0] != '\\' || path[1] != '\\')
		return (size_t) 0;
	path += 2;

	// Skip the server part.
	while (*path != '\\' && *path != '/') {
		if (*path == '\0')
			return (size_t) 0;
		path++;
	}

	// Skip the seperator.
	path++;

	// Skip the share part.
	while (*path != '\0' && *path != '\\' && *path != '/')
		path++;
	
	return (size_t) (path - inPath);
}
#endif  /* HAVE_UNC_PATHS */


