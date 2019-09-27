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

// Contains file handling code

#ifndef LIBS_FILE_H_
#define LIBS_FILE_H_

#include "port.h"
#include "libs/uio.h"

// for bool
#include "types.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if 0
// from temp.h
void initTempDir (void);
void unInitTempDir (void);
char *tempFilePath (const char *filename);
extern uio_DirHandle *tempDir;
#endif


// from dirs.h
int mkdirhier (const char *path);
const char *getHomeDir (void);
int createDirectory (const char *dir, int mode);

int expandPath (char *dest, size_t len, const char *src, int what);
// values for 'what':
#define EP_HOME      1
		// Expand '~' for home dirs.
#define EP_ABSOLUTE  2
		// Make paths absolute
#define EP_ENVVARS   4
		// Expand environment variables.
#define EP_DOTS      8
		// Process ".." and "."
#define EP_SLASHES   16
		// Consider backslashes as path component separators.
		// They will be replaced by slashes. Windows UNC paths will always
		// start with "\\server\share", with backslashes.
#define EP_SINGLESEP 32
		// Replace multiple consecutive path separators by a single one.
#define EP_ALL (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS | EP_SLASHES \
		EP_SINGLESEP)
		// Everything
// Everything except Windows style backslashes on Unix Systems:
#ifdef WIN32
#	define EP_ALL_SYSTEM (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS | \
		EP_SLASHES | EP_SINGLESEP)
#else
#	define EP_ALL_SYSTEM (EP_HOME | EP_ENVVARS | EP_ABSOLUTE | EP_DOTS | \
		EP_SINGLESEP)
#endif

// from files.h
int copyFile (uio_DirHandle *srcDir, const char *srcName,
		uio_DirHandle *dstDir, const char *newName);
bool fileExists (const char *name);
bool fileExists2(uio_DirHandle *dir, const char *fileName);
#ifdef HAVE_UNC_PATHS
size_t skipUNCServerShare(const char *inPath);
#endif  /* HAVE_UNC_PATHS */

#ifdef HAVE_DRIVE_LETTERS
static inline int isDriveLetter(int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
#endif  /* HAVE_DRIVE_LETTERS */

#if defined(__cplusplus)
}
#endif

#endif  /* LIBS_FILE_H_ */

