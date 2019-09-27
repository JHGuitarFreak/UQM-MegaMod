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

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "paths.h"
#include "uioport.h"
#include "mem.h"

static inline uio_PathComp *uio_PathComp_alloc(void);
static inline void uio_PathComp_free(uio_PathComp *pathComp);

// gets the first dir component of a path
// sets '*start' to the start of the first component
// sets '*end' to the end of the first component
// if *start >= dirEnd, then the end has been reached.
void
getFirstPathComponent(const char *dir, const char *dirEnd,
		const char **startComp, const char **endComp) {
	assert(dir <= dirEnd);
	*startComp = dir;
	if (*startComp == dirEnd) {
		*endComp = *startComp;
		return;
	}
	*endComp = memchr(*startComp, '/', dirEnd - *startComp);
	if (*endComp == NULL)
		*endComp = dirEnd;
}

// gets the first dir component of a path
// sets '*start' to the start of the first component
// sets '*end' to the end of the first component
// if **start == '\0', then the end has been reached.
void
getFirstPath0Component(const char *dir,
		const char **startComp, const char **endComp) {
	*startComp = dir;
	if (**startComp == '\0') {
		*endComp = *startComp;
		return;
	}
	*endComp = strchr(*startComp, '/');
	if (*endComp == NULL)
		*endComp = *startComp + strlen(*startComp);
}

// gets the next component of a path
// '*start' should be set to the start of the last component
// '*end' should be set to the end of the last component
// '*start' will be set to the start of the next component
// '*end' will be set to the end of the next component
// if *start >= dirEnd, then the end has been reached.
void
getNextPathComponent(const char *dirEnd,
		const char **startComp, const char **endComp) {
	assert(*endComp <= dirEnd);
	if (*endComp == dirEnd) {
		*startComp = *endComp;
		return;
	}
	assert(**endComp == '/');
	*startComp = *endComp + 1;
	*endComp = memchr(*startComp, '/', dirEnd - *startComp);
	if (*endComp == NULL)
		*endComp = dirEnd;
}

// gets the next component of a path
// '*start' should be set to the start of the last component
// '*end' should be set to the end of the last component
// '*start' will be set to the start of the next component
// '*end' will be set to the end of the next component
// if **start == '\0', then the end has been reached.
void
getNextPath0Component(const char **startComp, const char **endComp) {
	if (**endComp == '\0') {
		*startComp = *endComp;
		return;
	}
	assert(**endComp == '/');
	*startComp = *endComp + 1;
	*endComp = strchr(*startComp, '/');
	if (*endComp == NULL)
		*endComp = *startComp + strlen(*startComp);
}

// if *end == dir, the beginning has been reached
void
getLastPathComponent(const char *dir, const char *endDir,
		const char **startComp, const char **endComp) {
	*endComp = endDir;
//	if (*(*endComp - 1) == '/')
//		(*endComp)--;
	*startComp = *endComp;
	// TODO: use memrchr where available
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// if *end == dir, the beginning has been reached
// pre: dir is \0-terminated
void
getLastPath0Component(const char *dir,
		const char **startComp, const char **endComp) {
	*endComp = dir + strlen(dir);
//	if (*(*endComp - 1) == '/')
//		(*endComp)--;
	*startComp = *endComp;
	// TODO: use memrchr where available
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// if *end == dir, the beginning has been reached
void
getPreviousPathComponent(const char *dir,
		const char **startComp, const char **endComp) {
	if (*startComp == dir) {
		*endComp = *startComp;
		return;
	}
	*endComp = *startComp - 1;
	*startComp = *endComp;
	while ((*startComp) - 1 >= dir && *(*startComp - 1) != '/')
		(*startComp)--;
}

// Combine two parts of a paths into a new path.
// The new path starts with a '/' only when the first part does.
// The first path may (but doesn't have to) end on a '/', or may be empty.
// Pre: the second path doesn't start with a '/'
char *
joinPaths(const char *first, const char *second) {
	char *result, *resPtr;
	size_t firstLen, secondLen;

	if (first[0] == '\0')
		return uio_strdup(second);
	
	firstLen = strlen(first);
	if (first[firstLen - 1] == '/')
		firstLen--;
	secondLen = strlen(second);
	result = uio_malloc(firstLen + secondLen + 2);
	resPtr = result;

	memcpy(resPtr, first, firstLen);
	resPtr += firstLen;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, second, secondLen);
	resPtr += secondLen;

	*resPtr = '\0';
	return result;
}

// Combine two parts of a paths into a new path.
// The new path will always start with a '/'.
// The first path may (but doesn't have to) end on a '/', or may be empty.
// Pre: the second path doesn't start with a '/'
char *
joinPathsAbsolute(const char *first, const char *second) {
	char *result, *resPtr;
	size_t firstLen, secondLen;

	if (first[0] == '\0') {
		secondLen = strlen(second);
		result = uio_malloc(secondLen + 2);
		result[0] = '/';
		memcpy(&result[1], second, secondLen);
		result[secondLen + 1] = '\0';
		return result;
	}

	firstLen = strlen(first);
	if (first[firstLen - 1] == '/')
		firstLen--;
	secondLen = strlen(second);
	result = uio_malloc(firstLen + secondLen + 3);
	resPtr = result;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, first, firstLen);
	resPtr += firstLen;

	*resPtr = '/';
	resPtr++;

	memcpy(resPtr, second, secondLen);
	resPtr += secondLen;

	*resPtr = '\0';
	return result;
}

// Returns 'false' if
// - one of the path components is empty, or
// - one of the path components is ".", or
// - one of the path components is ".."
// and 'true' otherwise.
uio_bool
validPathName(const char *path, size_t len) {
	const char *pathEnd;
	const char *start, *end;

	pathEnd = path + len;
	getFirstPathComponent(path, pathEnd, &start, &end);
	while (start < pathEnd) {
		if (end == start || (end - start == 1 && start[0] == '.') ||
				(end - start == 2 && start[0] == '.' && start[1] == '.'))
			return false;
		getNextPathComponent(pathEnd, &start, &end);
	}
	return true;
}

// returns 0 if the path is not a valid UNC path.
// Does not skip trailing slashes.
size_t
uio_skipUNCServerShare(const char *inPath) {
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

/**
 * Find the server and share part of a Windows "Universal Naming Convention"
 * path (a path of the form '\\server\share\path\file').
 * The path must contain at least a server and share path to be valid.
 * The initial two slashes must be backslashes, the other slashes may each
 * be either a forward slash or a backslash.
 *
 * @param[in]  inPath   The path to parse.
 * @param[out] outPath  Will contain a newly allocated string (to be
 * 		freed using uio_free(), containing the server and share part
 * 		of inPath, separated by a backslash, or NULL if 'inPath' was
 * 		not a valid UNC path.
 * @param[out] outLen   If not NULL on entry, it will contain the string
 * 		length of '*outPath', or 0 if 'inPath' was not a valid UNC path.
 *
 * @returns The number of characters to add to 'inPath' to get to the first
 * 		path component past the server and share part, or 0 if 'inPath'
 * 		was not a valid UNC path.
 */
size_t
uio_getUNCServerShare(const char *inPath, char **outPath, size_t *outLen) {
	const char *ptr;
	const char *server;
	const char *serverEnd;
	const char *share;
	const char *shareEnd;
	char *name;
	char *nameEnd;
	size_t nameLen;

	ptr = inPath;

	if (ptr[0] != '\\' || ptr[1] != '\\')
		goto noMatch;

	// Parse the server part.
	server = ptr + 2;
	serverEnd = server;
	for (;;) {
		if (*serverEnd == '\0')
			goto noMatch;
		if (isPathDelimiter(*serverEnd))
			break;
		serverEnd++;
	}
	if (serverEnd == server)
		goto noMatch;

	// Parse the share part.
	share = serverEnd + 1;
	shareEnd = share;
	while (*shareEnd != '\0') {
		if (isPathDelimiter(*shareEnd))
			break;
		serverEnd++;
	}

	// Skip any trailing path delimiters.
	ptr = shareEnd;
	while (isPathDelimiter(*ptr))
		ptr++;

	// Allocate a new string and fill it.
	nameLen = (serverEnd - server) + (shareEnd - share) + 3;
	name = uio_malloc(nameLen + 1);
	nameEnd = name;
	*(nameEnd++) = '\\';
	*(nameEnd++) = '\\';
	memcpy(nameEnd, server, serverEnd - server);
	*(nameEnd++) = '\\';
	memcpy(nameEnd, share, shareEnd - share);
	*nameEnd = '\0';

	*outPath = name;
	if (outLen != NULL)
		*outLen = nameLen;
	return (size_t) (ptr - inPath);

noMatch:
	*outPath = NULL;
	if (outLen != NULL)
		*outLen = 0;
	return (size_t) 0;
}

// Decomposes a path into its components.
// If isAbsolute is not NULL, *isAbsolute will be set to true
// iff the path is absolute.
// As POSIX considers multiple consecutive slashes to be equivalent to
// a single slash, so will uio (but not in the "\\MACHINE\share" part
// of a Windows UNC path).
int
decomposePath(const char *path, uio_PathComp **pathComp,
		uio_bool *isAbsolute) {
	uio_PathComp *result;
	uio_PathComp *last;
	uio_PathComp **endResult = &result;
	uio_bool absolute = false;
	char *name;
#ifdef HAVE_UNC_PATHS
	size_t nameLen;
#endif  /* HAVE_UNC_PATHS */

	if (path[0] == '\0') {
		errno = ENOENT;
		return -1;
	}

	last = NULL;
#ifdef HAVE_UNC_PATHS
	path += uio_getUNCServerShare(path, &name, &nameLen);
	if (name != NULL) {
		// UNC path
		*endResult = uio_PathComp_new(name, nameLen, last);
		last = *endResult;
		endResult = &last->next;

		absolute = true;
	} else
#endif  /* HAVE_UNC_PATHS */
#ifdef HAVE_DRIVE_LETTERS
	if (isDriveLetter(path[0]) && path[1] == ':') {
		// DOS/Windows drive letter.
		if (path[2] != '\0' && !isPathDelimiter(path[2])) {
			errno = ENOENT;
			return -1;
		}
		name = uio_memdup0(path, 2);
		*endResult = uio_PathComp_new(name, 2, last);
		last = *endResult;
		endResult = &last->next;
		absolute = true;
	} else
#endif  /* HAVE_DRIVE_LETTERS */
	{
		if (isPathDelimiter(*path)) {
			absolute = true;
			do {
				path++;
			} while (isPathDelimiter(*path));
		}
	}

	while (*path != '\0') {
		const char *start = path;
		while (*path != '\0' && !isPathDelimiter(*path))
			path++;
		
		name = uio_memdup0(path, path - start);
		*endResult = uio_PathComp_new(name, path - start, last);
		last = *endResult;
		endResult = &last->next;

		while (isPathDelimiter(*path))
			path++;
	}

	*endResult = NULL;
	*pathComp = result;
	if (isAbsolute != NULL)
		*isAbsolute = absolute;
	return 0;
}

// Pre: pathComp forms a valid path for the platform.
void
composePath(const uio_PathComp *pathComp, uio_bool absolute,
		char **path, size_t *pathLen) {
	size_t len;
	const uio_PathComp *ptr;
	char *result;
	char *pathPtr;

	assert(pathComp != NULL);

	// First determine how much space is required.
	len = 0;
	if (absolute)
		len++;
	ptr = pathComp;
	while (ptr != NULL) {
		len += ptr->nameLen;
		ptr = ptr->next;
	}

	// Allocate the required space.
	result = (char *) uio_malloc(len + 1);

	// Fill the path.
	pathPtr = result;
	ptr = pathComp;
	if (absolute) {
#ifdef HAVE_UNC_PATHS
		if (ptr->name[0] == '\\') {
			// UNC path
			assert(ptr->name[1] == '\\');
			// Nothing to do.
		} else
#endif  /* HAVE_UNC_PATHS */
#ifdef HAVE_DRIVE_LETTERS
		if (ptr->nameLen == 2 && ptr->name[1] == ':'
				&& isDriveLetter(ptr->name[0])) {
			// Nothing to do.
		}
		else
#endif  /* HAVE_DRIVE_LETTERS */
		{
			*(pathPtr++) = '/';
		}
	}

	for (;;) {
		memcpy(pathPtr, ptr->name, ptr->nameLen);
		pathPtr += ptr->nameLen;

		ptr = ptr->next;
		if (ptr == NULL)
			break;
		
		*(pathPtr++) = '/';
	}

	*path = result;
	*pathLen = len;
}


// *** uio_PathComp *** //

static inline uio_PathComp *
uio_PathComp_alloc(void) {
	uio_PathComp *result = uio_malloc(sizeof (uio_PathComp));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_PathComp, (void *) result);
#endif
	return result;
}

static inline void
uio_PathComp_free(uio_PathComp *pathComp) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_PathComp, (void *) pathComp);
#endif
	uio_free(pathComp);
}

// 'name' should be a null terminated string. It is stored in the PathComp,
// no copy is made.
// 'namelen' should be the length of 'name'
uio_PathComp *
uio_PathComp_new(char *name, size_t nameLen, uio_PathComp *upComp) {
	uio_PathComp *result;
	
	result = uio_PathComp_alloc();
	result->name = name;
	result->nameLen = nameLen;
	result->up = upComp;
	return result;
}

void
uio_PathComp_delete(uio_PathComp *pathComp) {
	uio_PathComp *next;

	while (pathComp != NULL) {
		next = pathComp->next;
		uio_free(pathComp->name);
		uio_PathComp_free(pathComp);
		pathComp = next;
	}
}

// Count the number of path components that 'comp' leads to.
int
uio_countPathComps(const uio_PathComp *comp) {
	int count;
	
	count = 0;
	for (; comp != NULL; comp = comp->next)
		count++;
	return count;
}

uio_PathComp *
uio_lastPathComp(uio_PathComp *comp) {
	if (comp == NULL)
		return NULL;

	while (comp->next != NULL)
		comp = comp->next;
	return comp;
}

// make a list of uio_PathComps from a path string
uio_PathComp *
uio_makePathComps(const char *path, uio_PathComp *upComp) {
	const char *start, *end;
	char *str;
	uio_PathComp *result;
	uio_PathComp **compPtr;  // Where to put the next PathComp
	
	compPtr = &result;
	getFirstPath0Component(path, &start, &end);
	while (*start != '\0') {
		str = uio_malloc(end - start + 1);
		memcpy(str, start, end - start);
		str[end - start] = '\0';
		
		*compPtr = uio_PathComp_new(str, end - start, upComp);
		upComp = *compPtr;
		compPtr = &(*compPtr)->next;
		getNextPath0Component(&start, &end);
	}
	*compPtr = NULL;
	return result;
}

void
uio_printPathComp(FILE *outStream, const uio_PathComp *comp) {
	fprintf(outStream, "%s", comp->name);
}

void
uio_printPathToComp(FILE *outStream, const uio_PathComp *comp) {
	if (comp == NULL)
		return;
	uio_printPathToComp(outStream, comp->up);
	fprintf(outStream, "/");
	uio_printPathComp(outStream, comp);
}


