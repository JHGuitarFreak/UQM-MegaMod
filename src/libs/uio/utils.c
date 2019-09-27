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

#include <errno.h>
#include <time.h>
#include <stdio.h>
#ifdef _MSC_VER
#	include <stdarg.h>
#endif  /* _MSC_VER */

#include "iointrn.h"
#include "ioaux.h"
#include "utils.h"

static int uio_copyError(uio_Handle *srcHandle, uio_Handle *dstHandle,
		uio_DirHandle *unlinkHandle, const char *unlinkPath, uio_uint8 *buf);

struct uio_StdioAccessHandle {
	uio_DirHandle *tempRoot;
	char *tempDirName;
	uio_DirHandle *tempDir;
	char *fileName;
	char *stdioPath;
};

static inline uio_StdioAccessHandle *uio_StdioAccessHandle_new(
		uio_DirHandle *tempRoot, char *tempDirName,
		uio_DirHandle *tempDir, char *fileName,
		char *stdioPath);
static inline void uio_StdioAccessHandle_delete(
		uio_StdioAccessHandle *handle);
static inline uio_StdioAccessHandle *uio_StdioAccessHandle_alloc(void);
static inline void uio_StdioAccessHandle_free(uio_StdioAccessHandle *handle);

/*
 * Copy a file with path srcName to a file with name newName.
 * If the destination already exists, the operation fails.
 * Links are followed.
 * Special files (fifos, char devices, block devices, etc) will be
 * read as long as there is data available and the destination will be
 * a regular file with that data.
 * The new file will have the same permissions as the old.
 * If an error occurs during copying, an attempt will be made to
 * remove the copy.
 */
int
uio_copyFile(uio_DirHandle *srcDir, const char *srcName,
		uio_DirHandle *dstDir, const char *newName) {
	uio_Handle *src, *dst;
	struct stat sb;
#define BUFSIZE 65536
	uio_uint8 *buf, *bufPtr;
	ssize_t numInBuf, numWritten;
	
	src = uio_open(srcDir, srcName, O_RDONLY
#ifdef WIN32
			| O_BINARY
#endif
			, 0);
	if (src == NULL)
		return -1;
	
	if (uio_fstat(src, &sb) == -1)
		return uio_copyError(src, NULL, NULL, NULL, NULL);
	
	dst = uio_open(dstDir, newName, O_WRONLY | O_CREAT | O_EXCL
#ifdef WIN32
			| O_BINARY
#endif
			, sb.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
	if (dst == NULL)
		return uio_copyError(src, NULL, NULL, NULL, NULL);
	
	buf = uio_malloc(BUFSIZE);
			// This was originally a statically allocated buffer,
			// but as this function might be run from a thread with
			// a small Stack, this is better.
	while (1) {
		numInBuf = uio_read(src, buf, BUFSIZE);
		if (numInBuf == -1) {
			if (errno == EINTR)
				continue;
			return uio_copyError(src, dst, dstDir, newName, buf);
		}
		if (numInBuf == 0)
			break;
		
		bufPtr = buf;
		do
		{
			numWritten = uio_write(dst, bufPtr, numInBuf);
			if (numWritten == -1) {
				if (errno == EINTR)
					continue;
				return uio_copyError(src, dst, dstDir, newName, buf);
			}
			numInBuf -= numWritten;
			bufPtr += numWritten;
		} while (numInBuf > 0);
	}
	
	uio_free(buf);
	uio_close(src);
	uio_close(dst);
	errno = 0;
	return 0;
}

/*
 * Closes srcHandle if it's not -1.
 * Closes dstHandle if it's not -1.
 * Removes unlinkpath from the unlinkHandle dir if it's not NULL.
 * Frees 'buf' if not NULL.
 * Always returns -1.
 * errno is what was before the call.
 */
static int
uio_copyError(uio_Handle *srcHandle, uio_Handle *dstHandle,
		uio_DirHandle *unlinkHandle, const char *unlinkPath, uio_uint8 *buf) {
	int savedErrno;

	savedErrno = errno;

#ifdef DEBUG
	fprintf(stderr, "Error while copying: %s\n", strerror(errno));
#endif

	if (srcHandle != NULL)
		uio_close(srcHandle);
	
	if (dstHandle != NULL)
		uio_close(dstHandle);
	
	if (unlinkPath != NULL)
		uio_unlink(unlinkHandle, unlinkPath);
	
	if (buf != NULL)
		uio_free(buf);
	
	errno = savedErrno;
	return -1;
}

#define NUM_TEMP_RETRIES 16
		// Retry this many times to create a temporary dir, before giving
		// up. If undefined, keep trying indefinately.

uio_StdioAccessHandle *
uio_getStdioAccess(uio_DirHandle *dir, const char *path, int flags,
		uio_DirHandle *tempDir) {
	int res;
	uio_MountHandle *mountHandle;
	const char *name;
	char *newPath;
	char *tempDirName;
	uio_DirHandle *newDir;
	uio_FileSystemID fsID;

	res = uio_getFileLocation(dir, path, flags, &mountHandle, &newPath);
	if (res == -1) {
		// errno is set
		return NULL;
	}
	
	fsID = uio_getMountFileSystemType(mountHandle);
	if (fsID == uio_FSTYPE_STDIO) {
		// Current location is usable.
		return uio_StdioAccessHandle_new(NULL, NULL, NULL, NULL, newPath);
	}
	uio_free(newPath);

	{
		uio_uint32 dirNum;
		int i;

		// Current location is not usable. Create a directory with a
		// generated name, as a temporary location to store a copy of
		// the file.
		dirNum = (uio_uint32) time(NULL);
		tempDirName = uio_malloc(sizeof "01234567");
		for (i = 0; ; i++) {
#ifdef NUM_TEMP_RETRIES
			if (i >= NUM_TEMP_RETRIES) {
				// Using ENOSPC to report that we couldn't create a
				// temporary dir, getting EEXIST.
				uio_free(tempDirName);
				errno = ENOSPC;
				return NULL;
			}
#endif
			
			sprintf(tempDirName, "%08lx", (unsigned long) dirNum + i);
			
			res = uio_mkdir(tempDir, tempDirName, 0700);
			if (res == -1) {
				int savedErrno;
				if (errno == EEXIST)
					continue;
				savedErrno = errno;
#ifdef DEBUG
				fprintf(stderr, "Error: Could not create temporary dir: %s\n",
						strerror(errno));
#endif
				uio_free(tempDirName);
				errno = savedErrno;
				return NULL;
			}
			break;
		}

		newDir = uio_openDirRelative(tempDir, tempDirName, 0);
		if (newDir == NULL) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not open temporary dir: %s\n",
					strerror(errno));
#endif
			res = uio_rmdir(tempDir, tempDirName);
#ifdef DEBUG
			if (res == -1)
				fprintf(stderr, "Warning: Could not remove temporary dir: "
						"%s.\n", strerror(errno));
#endif
			uio_free(tempDirName);
			errno = EIO;
			return NULL;
		}

		// Get the last component of path. This should be the file to
		// access.
		name = strrchr(path, '/');
		if (name == NULL)
			name = path;

		// Copy the file
		res = uio_copyFile(dir, path, newDir, name);
		if (res == -1) {
			int savedErrno = errno;
#ifdef DEBUG
			fprintf(stderr, "Error: Could not copy file to temporary dir: "
					"%s\n", strerror(errno));
#endif
			uio_closeDir(newDir);
			uio_free(tempDirName);
			errno = savedErrno;
			return NULL;
		}
	}

	res = uio_getFileLocation(newDir, name, flags, &mountHandle, &newPath);
	if (res == -1) {
		int savedErrno = errno;
		fprintf(stderr, "Error: uio_getStdioAccess: Could not get location "
				"of temporary dir: %s.\n", strerror(errno));
		uio_closeDir(newDir);
		uio_free(tempDirName);
		errno = savedErrno;
		return NULL;
	}
	
	fsID = uio_getMountFileSystemType(mountHandle);
	if (fsID != uio_FSTYPE_STDIO) {
		// Temp dir isn't on a stdio fs either.
		fprintf(stderr, "Error: uio_getStdioAccess: Temporary file location "
				"isn't on a stdio filesystem.\n");
		uio_closeDir(newDir);
		uio_free(tempDirName);
		uio_free(newPath);
//		errno = EXDEV;
		errno = EINVAL;
		return NULL;
	}

	uio_DirHandle_ref(tempDir);
	return uio_StdioAccessHandle_new(tempDir, tempDirName, newDir, 
			uio_strdup(name), newPath);
}

void
uio_releaseStdioAccess(uio_StdioAccessHandle *handle) {
	if (handle->tempDir != NULL) {
		if (uio_unlink(handle->tempDir, handle->fileName) == -1) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not remove temporary file: "
					"%s\n", strerror(errno));
#endif
		}

		// Need to free this handle in advance. There should be no handles
		// to a dir left when removing it.
		uio_DirHandle_unref(handle->tempDir);
		handle->tempDir = NULL;

		if (uio_rmdir(handle->tempRoot, handle->tempDirName) == -1) {
#ifdef DEBUG
			fprintf(stderr, "Error: Could not remove temporary directory: "
					"%s\n", strerror(errno));
#endif
		}
	}

	uio_StdioAccessHandle_delete(handle);
}

const char *
uio_StdioAccessHandle_getPath(uio_StdioAccessHandle *handle) {
	return (const char *) handle->stdioPath;
}

// references to tempRoot and tempDir are not increased.
// no copies of arguments are made.
// By calling this function control of the values is transfered to
// the handle.
static inline uio_StdioAccessHandle *
uio_StdioAccessHandle_new(
		uio_DirHandle *tempRoot, char *tempDirName,
		uio_DirHandle *tempDir, char *fileName, char *stdioPath) {
	uio_StdioAccessHandle *result;

	result = uio_StdioAccessHandle_alloc();
	result->tempRoot = tempRoot;
	result->tempDirName = tempDirName;
	result->tempDir = tempDir;
	result->fileName = fileName;
	result->stdioPath = stdioPath;

	return result;
}

static inline void
uio_StdioAccessHandle_delete(uio_StdioAccessHandle *handle) {
	if (handle->tempDir != NULL)
		uio_DirHandle_unref(handle->tempDir);
	if (handle->fileName != NULL)
		uio_free(handle->fileName);
	if (handle->tempRoot != NULL)
		uio_DirHandle_unref(handle->tempRoot);
	if (handle->tempDirName != NULL)
		uio_free(handle->tempDirName);
	uio_free(handle->stdioPath);
	uio_StdioAccessHandle_free(handle);
}

static inline uio_StdioAccessHandle *
uio_StdioAccessHandle_alloc(void) {
	return uio_malloc(sizeof (uio_StdioAccessHandle));
}

static inline void
uio_StdioAccessHandle_free(uio_StdioAccessHandle *handle) {
	uio_free(handle);
}

#ifdef _MSC_VER
#	include <stdarg.h>
#if 0  /* Unneeded for now */
// MSVC does not have snprintf(). It does have a _snprintf(), but it does
// not \0-terminate a truncated string as the C standard prescribes.
static inline int
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
#endif


#if (_MSC_VER <= 1800)
// MSVC does not have vsnprintf(). It does have a _vsnprintf(), but it does
// not \0-terminate a truncated string as the C standard prescribes.
static inline int
vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int result = _vsnprintf (str, size, format, args);
	if (str != NULL && size != 0)
		str[size - 1] = '\0';
	return result;
}
#endif
#endif  /* _MSC_VER */

// The result should be freed using uio_free().
// NB. POSIX allows errno to be set for vsprintf(), but does not require it:
// "The value of errno may be set to nonzero by a library function call
// whether or not there is an error, provided the use of errno is not
// documented in the description of the function in this International
// Standard." The latter is the case for vsprintf().
char *
uio_vasprintf(const char *format, va_list args) {
	// TODO: If there is a system vasprintf, use that.
	// XXX:  That would mean that the allocation would always go through
	//       malloc() or so, instead of uio_malloc(),  which may not be
	//       desirable.

	char *buf;
	size_t bufSize = 128;
			// Start with enough for one screen line, and a power of 2,
			// which might give faster result with allocations.

	buf = uio_malloc(bufSize);
	if (buf == NULL) {
		// errno is set.
		return NULL;
	}

	for (;;) {
		int printResult = vsnprintf(buf, bufSize, format, args);
		if (printResult < 0) {
			// This means the buffer was not large enough, but vsnprintf()
			// does not give us any clue on how large it should be.
			// Note that this does not happen with a C'99 compliant
			// vsnprintf(), but it will happen on MS Windows, and on
			// glibc before version 2.1.
			bufSize *= 2;
		} else if ((unsigned int) printResult >= bufSize) {
			// The buffer was too small, but printResult contains the size
			// that the buffer needs to be (excluding the '\0' character).
			bufSize = printResult + 1;
		} else {
			// Success.
			if ((unsigned int) printResult + 1 != bufSize) {
				// Shorten the resulting buffer to the size that was
				// actually needed.
				char *newBuf = uio_realloc(buf, printResult + 1);
				if (newBuf == NULL) {
					// We could have returned the (overly large) original
					// buffer, but the unused memory might not be
					// acceptable, and the program would be likely to run
					// into problems sooner or later anyhow.
					int savedErrno = errno;
					uio_free(buf);
					errno = savedErrno;
					return NULL;
				}
				return newBuf;
			}

			return buf;
		}

		{
			char *newBuf = uio_realloc(buf, bufSize);
			if (newBuf == NULL)
			{
				int savedErrno = errno;
				uio_free(buf);
				errno = savedErrno;
				return NULL;
			}
			buf = newBuf;
		}
	}
}

// As uio_vasprintf(), but with an argument list.
char *
uio_asprintf(const char *format, ...) {
	// TODO: If there is a system asprintf, use that.
	// XXX:  That would mean that the allocation would always go through
	//       malloc() or so, instead of uio_malloc(),  which may not be
	//       desirable.

	va_list args;
	char *result;
	int savedErrno;

	va_start(args, format);
	result = uio_vasprintf(format, args);
	savedErrno = errno;
	va_end(args);

	errno = savedErrno;
	return result;
}


