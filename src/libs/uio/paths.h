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

#ifndef LIBS_UIO_PATHS_H_
#define LIBS_UIO_PATHS_H_

typedef struct uio_PathComp uio_PathComp;

#include "types.h"
#include "uioport.h"

#include <stdio.h>

struct uio_PathComp {
	char *name;
			// The name of this path component, 0-terminated
	size_t nameLen;
			// The length of the 'name' field, for fast lookups.
	struct uio_PathComp *next;
			// Next component in the path.
	struct uio_PathComp *up;
			// Previous component in the path.
};

void getFirstPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp);
void getFirstPath0Component(const char *dir, const char **startComp,
		const char **endComp);
void getNextPathComponent(const char *dirEnd,
		const char **startComp, const char **endComp);
void getNextPath0Component(const char **startComp, const char **endComp);
void getLastPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp);
void getLastPath0Component(const char *dir, const char **startComp,
		const char **endComp);
void getPreviousPathComponent(const char *dir, const char **startComp,
		const char **endComp);
#define getPreviousPath0Component getPreviousPathComponent
char *joinPaths(const char *first, const char *second);
char *joinPathsAbsolute(const char *first, const char *second);

uio_bool validPathName(const char *path, size_t len);
size_t uio_skipUNCServerShare(const char *inPath);
size_t uio_getUNCServerShare(const char *inPath, char **outPath,
		size_t *outLen);

#ifdef HAVE_DRIVE_LETTERS
static inline int
isDriveLetter(int c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
#endif  /* HAVE_DRIVE_LETTERS */

static inline int
isPathDelimiter(int c)
{
#ifdef BACKSLASH_IS_PATH_SEPARATOR
	return c == '/' || c == '\\';
#else
	return c == '/';
#endif  /* BACKSLASH_IS_PATH_SEPARATOR */
}

int decomposePath(const char *path, uio_PathComp **pathComp,
		uio_bool *isAbsolute);
void composePath(const uio_PathComp *pathComp, uio_bool absolute,
		char **path, size_t *pathLen);
uio_PathComp *uio_PathComp_new(char *name, size_t nameLen,
		uio_PathComp *upComp);
void uio_PathComp_delete(uio_PathComp *pathComp);
int uio_countPathComps(const uio_PathComp *comp);
uio_PathComp *uio_lastPathComp(uio_PathComp *comp);
uio_PathComp *uio_makePathComps(const char *path, uio_PathComp *upComp);
void uio_printPathComp(FILE *outStream, const uio_PathComp *comp);
void uio_printPathToComp(FILE *outStream, const uio_PathComp *comp);

#endif  /* LIBS_UIO_PATHS_H_ */

