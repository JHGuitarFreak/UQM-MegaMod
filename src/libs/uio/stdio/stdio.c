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

// The GPDir structures and functions are used for caching only.

#ifdef __svr4__
#	define _POSIX_PTHREAD_SEMANTICS
			// For the POSIX variant of readdir_r()
#endif

#include "./stdio.h"

#ifdef WIN32
#	include <io.h>
#else
#	include <sys/stat.h>
#	include <unistd.h>
#	include <dirent.h>
#endif
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>

#include "../uioport.h"
#include "../paths.h"
#include "../mem.h"
#include "../physical.h"
#ifdef uio_MEM_DEBUG
#	include "../memdebug.h"
#endif

static inline uio_GPFile *stdio_addFile(uio_GPDir *gPDir,
		const char *fileName);
static inline uio_GPDir *stdio_addDir(uio_GPDir *gPDir, const char *dirName);
static char *stdio_getPath(uio_GPDir *gPDir);
static stdio_GPDirData *stdio_GPDirData_new(char *name, char *cachedPath,
		uio_GPDir *upDir);
static void stdio_GPDirData_delete(stdio_GPDirData *gPDirData);
static inline stdio_GPDirData *stdio_GPDirData_alloc(void);
static inline void stdio_GPDirData_free(stdio_GPDirData *gPDirData);
static inline stdio_EntriesIterator *stdio_EntriesIterator_alloc(void);
static inline void stdio_EntriesIterator_free(
		stdio_EntriesIterator *iterator);

uio_FileSystemHandler stdio_fileSystemHandler = {
	/* .init    = */  NULL,
	/* .unInit  = */  NULL,
	/* .cleanup = */  NULL,

	/* .mount  = */  stdio_mount,
	/* .umount = */  uio_GPRoot_umount,

	/* .access = */  stdio_access,
	/* .close  = */  stdio_close,
	/* .fstat  = */  stdio_fstat,
	/* .stat   = */  stdio_stat,
	/* .mkdir  = */  stdio_mkdir,
	/* .open   = */  stdio_open,
	/* .read   = */  stdio_read,
	/* .rename = */  stdio_rename,
	/* .rmdir  = */  stdio_rmdir,
	/* .seek   = */  stdio_seek,
	/* .write  = */  stdio_write,
	/* .unlink = */  stdio_unlink,

	/* .openEntries  = */  stdio_openEntries,
	/* .readEntries  = */  stdio_readEntries,
	/* .closeEntries = */  stdio_closeEntries,

	/* .getPDirEntryHandle     = */  stdio_getPDirEntryHandle,
	/* .deletePRootExtra       = */  uio_GPRoot_delete,
	/* .deletePDirHandleExtra  = */  uio_GPDirHandle_delete,
	/* .deletePFileHandleExtra = */  uio_GPFileHandle_delete,
};

uio_GPRoot_Operations stdio_GPRootOperations = {
	/* .fillGPDir         = */  NULL,
	/* .deleteGPRootExtra = */  NULL,
	/* .deleteGPDirExtra  = */  stdio_GPDirData_delete,
	/* .deleteGPFileExtra = */  NULL,
};


void
stdio_close(uio_Handle *handle) {
	int fd;
	int result;
	
	fd = handle->native->fd;
	uio_free(handle->native);
	
	while (1) {
		result = close(fd);
		if (result == 0)
			break;
		if (errno != EINTR) {
			fprintf(stderr, "Warning: Error while closing socket: %s\n",
					strerror(errno));
			break;
		}
	}
}

int
stdio_access(uio_PDirHandle *pDirHandle, const char *name, int mode) {
	char *path;
	int result;
	
	path = joinPaths(stdio_getPath(pDirHandle->extra), name);
	if (path == NULL) {
		// errno is set
		return -1;
	}

	result = access(path, mode);
	if (result == -1) {
		int savedErrno = errno;
		uio_free(path);
		errno = savedErrno;
		return -1;
	}

	uio_free(path);
	return result;
}

int
stdio_fstat(uio_Handle *handle, struct stat *statBuf) {
	return fstat(handle->native->fd, statBuf);
}

int
stdio_stat(uio_PDirHandle *pDirHandle, const char *name,
		struct stat *statBuf) {
	char *path;
	int result;

	path = joinPaths(stdio_getPath(pDirHandle->extra), name);
	if (path == NULL) {
		// errno is set
		return -1;
	}

	result = stat(path, statBuf);
	if (result == -1) {
		int savedErrno = errno;
		uio_free(path);
		errno = savedErrno;
		return -1;
	}

	uio_free(path);
	return result;
}

uio_PDirHandle *
stdio_mkdir(uio_PDirHandle *pDirHandle, const char *name, mode_t mode) {
	char *path;
	uio_GPDir *newGPDir;
	
	path = joinPaths(stdio_getPath(pDirHandle->extra), name);
	if (path == NULL) {
		// errno is set
		return NULL;
	}

	if (MKDIR(path, mode) == -1) {
		int savedErrno = errno;
		uio_free(path);
		errno = savedErrno;
		return NULL;
	}
	uio_free(path);

	newGPDir = stdio_addDir(pDirHandle->extra, name);
	uio_GPDir_ref(newGPDir);
	return uio_PDirHandle_new(pDirHandle->pRoot, newGPDir);
}

/*
 * Function name: stdio_open
 * Description:   open a file from a normal stdio environment
 * Arguments:     gPDir - the dir where to open the file
 *                file - the name of the file to open
 *                flags - flags, as to stdio open()
 *                mode - mode, as to stdio open()
 * Returns:       handle, for use in functions accessing the opened file.
 *                If failed, errno is set and handle is -1.
 */
uio_Handle *
stdio_open(uio_PDirHandle *pDirHandle, const char *file, int flags,
		mode_t mode) {
	stdio_Handle *handle;
	char *path;
	int fd;
	
	path = joinPaths(stdio_getPath(pDirHandle->extra), file);
	if (path == NULL) {
		// errno is set
		return NULL;
	}

	fd = open(path, flags, mode);
	if (fd == -1) {
		int save_errno;
		
		save_errno = errno;
		uio_free(path);
		errno = save_errno;
		return NULL;
	}
	uio_free(path);

#if 0
	if (flags & O_CREAT) {
		if (uio_GPDir_getGPDirEntry(pDirHandle->extra, file) == NULL)
			stdio_addFile(pDirHandle->extra, file);
	}
#endif

	handle = uio_malloc(sizeof (stdio_Handle));
	handle->fd = fd;

	return uio_Handle_new(pDirHandle->pRoot, handle, flags);
}

ssize_t
stdio_read(uio_Handle *handle, void *buf, size_t count) {
	return read(handle->native->fd, buf, count);
}

int
stdio_rename(uio_PDirHandle *oldPDirHandle, const char *oldName,
		uio_PDirHandle *newPDirHandle, const char *newName) {
	char *newPath, *oldPath;
	int result;

	oldPath = joinPaths(stdio_getPath(oldPDirHandle->extra), oldName);
	if (oldPath == NULL) {
		// errno is set
		return -1;
	}
	
	newPath = joinPaths(stdio_getPath(newPDirHandle->extra), newName);
	if (newPath == NULL) {
		// errno is set
		uio_free(oldPath);
		return -1;
	}
	
	result = rename(oldPath, newPath);
	if (result == -1) {
		int savedErrno = errno;
		uio_free(oldPath);
		uio_free(newPath);
		errno = savedErrno;
		return -1;
	}

	uio_free(oldPath);
	uio_free(newPath);

	{
		// update the GPDir structure
		uio_GPDirEntry *entry;

		// TODO: add locking
		entry = uio_GPDir_getGPDirEntry(oldPDirHandle->extra, oldName);
		if (entry != NULL) {
			uio_GPDirEntries_remove(oldPDirHandle->extra->entries, oldName);
			uio_GPDirEntries_add(newPDirHandle->extra->entries, newName,
					entry);
		}
	}
		
	return result;
}

int
stdio_rmdir(uio_PDirHandle *pDirHandle, const char *name) {
	char *path;
	int result;

	path = joinPaths(stdio_getPath(pDirHandle->extra), name);
	if (path == NULL) {
		// errno is set
		return -1;
	}
	
	result = rmdir(path);
	if (result == -1) {
		int savedErrno = errno;
		uio_free(path);
		errno = savedErrno;
		return -1;
	}

	uio_free(path);

	uio_GPDir_removeSubDir(pDirHandle->extra, name);

	return result;
}

off_t
stdio_seek(uio_Handle *handle, off_t offset, int whence) {
	return lseek(handle->native->fd, offset, whence);
}

ssize_t
stdio_write(uio_Handle *handle, const void *buf, size_t count) {
	return write(handle->native->fd, buf, count);
}

int
stdio_unlink(uio_PDirHandle *pDirHandle, const char *name) {
	char *path;
	int result;

	path = joinPaths(stdio_getPath(pDirHandle->extra), name);
	if (path == NULL) {
		// errno is set
		return -1;
	}
	
	result = unlink(path);
	if (result == -1) {
		int savedErrno = errno;
		uio_free(path);
		errno = savedErrno;
		return -1;
	}

	uio_free(path);

	uio_GPDir_removeFile(pDirHandle->extra, name);

	return result;
}

uio_PDirEntryHandle *
stdio_getPDirEntryHandle(const uio_PDirHandle *pDirHandle, const char *name) {
	uio_PDirEntryHandle *result;
	const char *pathUpTo;
	char *path;
	struct stat statBuf;
#ifdef HAVE_DRIVE_LETTERS
	char driveName[3];
#endif  /* HAVE_DRIVE_LETTERS */

#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)
	if (pDirHandle->extra->extra->upDir == NULL) {
		// Top dir. Contains only drive letters and UNC \\server\share
		// parts.
#ifdef HAVE_DRIVE_LETTERS
		if (isDriveLetter(name[0]) && name[1] == ':' && name[2] == '\0') {
			driveName[0] = tolower(name[0]);
			driveName[1] = ':';
			driveName[2] = '\0';
			name = driveName;
		} else
#endif  /* HAVE_DRIVE_LETTERS */
#ifdef HAVE_UNC_PATHS
		{
			size_t uncLen;

			uncLen = uio_skipUNCServerShare(name);
			if (name[uncLen] != '\0') {
				// 'name' contains neither a drive letter, nor the
				// first part of a UNC path.
				return NULL;
			}
		}
#else /* !defined(HAVE_UNC_PATHS) */
		{
			// Make sure that there is an 'else' case if HAVE_DRIVE_LETTERS
			// is defined.
		}
#endif  /* HAVE_UNC_PATHS */
	}
#endif  /* defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS) */
	
	result = uio_GPDir_getPDirEntryHandle(pDirHandle, name);
	if (result != NULL)
		return result;

#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)
	if (pDirHandle->extra->extra->upDir == NULL) {
		// Need to create a 'directory' for the drive letter or UNC
		// "\\server\share" part.
		// It's no problem if we happen to create a dir for a non-existing
		// drive. It should just produce an empty dir.
		uio_GPDir *gPDir;
		
		gPDir = stdio_addDir(pDirHandle->extra, name);
		uio_GPDir_ref(gPDir);
		return (uio_PDirEntryHandle *) uio_PDirHandle_new(
				pDirHandle->pRoot, gPDir);
	}
#endif  /* defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS) */
	
	pathUpTo = stdio_getPath(pDirHandle->extra);
	if (pathUpTo == NULL) {
		// errno is set
		return NULL;
	}
	path = joinPaths(pathUpTo, name);
	if (path == NULL) {
		// errno is set
		return NULL;
	}

	if (stat(path, &statBuf) == -1) {
#ifdef __SYMBIAN32__
		// XXX: HACK: If we don't have access to a directory, we can still
		// have access to the underlying entries. We don't actually know
		// whether the entry is a directory, but I know of no way to find
		// out. We just pretend that it is; worst case, a file which we can't
		// access shows up as a directory which we can't access.
		if (errno == EACCES) {
			statBuf.st_mode = S_IFDIR;
					// Fake a directory; the other fields of the stat
					// structure are unused.
		} else
#endif
		{
			// errno is set.
			int savedErrno = errno;
			uio_free(path);
			errno = savedErrno;
			return NULL;
		}
	}
	uio_free(path);

	if (S_ISREG(statBuf.st_mode)) {
		uio_GPFile *gPFile;
		
		gPFile = stdio_addFile(pDirHandle->extra, name);
		uio_GPFile_ref(gPFile);
		return (uio_PDirEntryHandle *) uio_PFileHandle_new(
				pDirHandle->pRoot, gPFile);
	} else if (S_ISDIR(statBuf.st_mode)) {
		uio_GPDir *gPDir;
		
		gPDir = stdio_addDir(pDirHandle->extra, name);
		uio_GPDir_ref(gPDir);
		return (uio_PDirEntryHandle *) uio_PDirHandle_new(
				pDirHandle->pRoot, gPDir);
	} else {
#ifdef DEBUG
		fprintf(stderr, "Warning: Attempt to access '%s' from '%s', "
				"which is not a regular file, nor a directory.\n", name,
				pathUpTo);
#endif
		return NULL;
	}
}

uio_PRoot *
stdio_mount(uio_Handle *handle, int flags) {
	uio_PRoot *result;
	stdio_GPDirData *extra;
	
	assert (handle == NULL);
	extra = stdio_GPDirData_new(
			uio_strdup("") /* name */,
#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)
			// Full paths start with a drive letter or \\server\share
			uio_strdup("") /* cached path */,
#else
			uio_strdup("/") /* cached path */,
#endif  /* HAVE_DRIVE_LETTERS */
			NULL /* parent dir */);

	result = uio_GPRoot_makePRoot(
			uio_getFileSystemHandler(uio_FSTYPE_STDIO), flags,
			&stdio_GPRootOperations, NULL, uio_GPRoot_PERSISTENT,
			handle, extra, 0);

	uio_GPDir_setComplete(result->rootDir->extra, true);

	return result;
}

#ifdef WIN32
stdio_EntriesIterator *
stdio_openEntries(uio_PDirHandle *pDirHandle) {
	const char *dirPath;
	char path[PATH_MAX];
	char *pathEnd;
	size_t dirPathLen;
	stdio_EntriesIterator *iterator;

//	uio_GPDir_access(pDirHandle->extra);

	dirPath = stdio_getPath(pDirHandle->extra);
	if (dirPath == NULL) {
		// errno is set
		return NULL;
	}

	dirPathLen = strlen(dirPath);
	if (dirPathLen > PATH_MAX - 3) {
		// dirPath ++ '/' ++ '*' ++ '\0'
		errno = ENAMETOOLONG;
		return NULL;
	}
	memcpy(path, dirPath, dirPathLen);
	pathEnd = path + dirPathLen;
	pathEnd[0] = '/';
	pathEnd[1] = '*';
	pathEnd[2] = '\0';
	iterator = stdio_EntriesIterator_new(0);
	iterator->dirHandle = _findfirst(path, &iterator->findData);
	if (iterator->dirHandle == 1) {
		if (errno != ENOENT) {
			stdio_EntriesIterator_delete(iterator);
			return NULL;
		}
		iterator->status = 1;
	} else
		iterator->status = 0;
	return iterator;
}
#endif

#ifndef WIN32
stdio_EntriesIterator *
stdio_openEntries(uio_PDirHandle *pDirHandle) {
	const char *dirPath;
	DIR *dirHandle;
	stdio_EntriesIterator *result;

//	uio_GPDir_access(pDirHandle->extra);

	dirPath = stdio_getPath(pDirHandle->extra);
	if (dirPath == NULL) {
		// errno is set
		return NULL;
	}

	dirHandle = opendir(dirPath);
	if (dirHandle == NULL) {
		// errno is set;
		return NULL;
	}
	
	result = stdio_EntriesIterator_new(dirHandle);
	result->status = readdir_r(dirHandle, result->direntBuffer,
			&result->entry);
#ifndef WIN32
#	ifdef DEBUG
	if (result->status != 0) {
		fprintf(stderr, "Warning: readdir_r() failed: %s\n",
				strerror(result->status));
	}
#	endif
#endif
	return result;
}
#endif

// the start of 'buf' will be filled with pointers to strings
// those strings are stored elsewhere in buf.
// The function returns the number of strings passed along, or -1 for error.
// If there are no more entries, the last pointer will be NULL.
// (this pointer counts towards the return value)
int
stdio_readEntries(stdio_EntriesIterator **iteratorPtr,
		char *buf, size_t len) {
	char *end;
	char **start;
	int num;
	const char *name;
	size_t nameLen;
	stdio_EntriesIterator *iterator;

	iterator = *iteratorPtr;

	// buf will be filled like this:
	// The start of buf will contain pointers to char *,
	// the end will contain the actual char[] that those pointers point to.
	// The start and the end will grow towards eachother.
	start = (char **) buf;
	end = buf + len;
	num = 0;
#ifdef WIN32
	for (; iterator->status == 0;
			iterator->status = _findnext(iterator->dirHandle,
			&iterator->findData))
#else
	for (; iterator->status == 0 && iterator->entry != NULL;
			iterator->status = readdir_r(iterator->dirHandle,
			iterator->direntBuffer, &iterator->entry))
#endif
	{
#ifdef WIN32
		name = iterator->findData.name;
#else
		name = iterator->entry->d_name;
#endif
		if (name[0] == '.' &&
				(name[1] == '\0' ||
				(name[1] == '.' && name[2] == '\0'))) {
			// skip directories "." and ".."
			continue;
		}
		nameLen = strlen(name) + 1;
		
		// Does this work with systems that need memory access to be
		// aligned on a certain number of bytes?
		if ((size_t) (sizeof (char *) + nameLen) >
				(size_t) (end - (char *) start)) {
			// Not enough room to fit the pointer (at the beginning) and
			// the string (at the end).
			return num;
		}
		end -= nameLen;
		memcpy(end, name, nameLen);
		*start = end;
		start++;
		num++;
	}
#ifndef WIN32
#	ifdef DEBUG
	if (iterator->status != 0) {
		fprintf(stderr, "Warning: readdir_r() failed: %s\n",
				strerror(iterator->status));
	}
#	endif
#endif
	if (sizeof (char *) > (size_t) (end - (char *) start)) {
		// not enough room to fit the NULL pointer.
		// It will have to be reported seperately the next time.
		return num;
	}
	*start = NULL;
	num++;
	return num;
}

void
stdio_closeEntries(stdio_EntriesIterator *iterator) {
#ifdef WIN32
	_findclose(iterator->dirHandle);
#else
	closedir(iterator->dirHandle);
#endif
	stdio_EntriesIterator_delete(iterator);
}

#ifdef WIN32
stdio_EntriesIterator *
stdio_EntriesIterator_new(long dirHandle) {
	stdio_EntriesIterator *result;

	result = stdio_EntriesIterator_alloc();
	result->dirHandle = dirHandle;
	return result;
}
#else
stdio_EntriesIterator *
stdio_EntriesIterator_new(DIR *dirHandle) {
	stdio_EntriesIterator *result;
	size_t bufferSize;

	result = stdio_EntriesIterator_alloc();
	result->dirHandle = dirHandle;

	// Linux's and FreeBSD's struct dirent are defined with a 
	// maximum d_name field (NAME_MAX).
	// However, POSIX doesn't require this, and in fact
	// at least QNX defines struct dirent with an empty d_name field.
	// Solaris defineds it with a d_name field of length 1.
	// This should take care of it:
	bufferSize = sizeof (struct dirent)
			- sizeof (((struct dirent *) 0)->d_name) + (NAME_MAX + 1);
			// Take the length of the dirent structure as it is defined,
			// subtract the length of the d_name field, and add the length
			// of the maximum length d_name field (NAME_MAX plus 1 for
			// the '\0').
			// XXX: Could this give problems with weird alignments?
	result->direntBuffer = uio_malloc(bufferSize);
	return result;
}
#endif

void
stdio_EntriesIterator_delete(stdio_EntriesIterator *iterator) {
#ifndef WIN32
	uio_free(iterator->direntBuffer);
#endif
	stdio_EntriesIterator_free(iterator);
}

static inline stdio_EntriesIterator *
stdio_EntriesIterator_alloc(void) {
	return uio_malloc(sizeof (stdio_EntriesIterator));
}

static inline void
stdio_EntriesIterator_free(stdio_EntriesIterator *iterator) {
	uio_free(iterator);
}

static inline uio_GPFile *
stdio_addFile(uio_GPDir *gPDir, const char *fileName) {
	uio_GPFile *file;
	
	file = uio_GPFile_new(gPDir->pRoot, NULL,
			uio_gPFileFlagsFromPRootFlags(gPDir->pRoot->flags));
	uio_GPDir_addFile(gPDir, fileName, file);
	return file;
}

// called by fillGPDir when a subdir is found
static inline uio_GPDir *
stdio_addDir(uio_GPDir *gPDir, const char *dirName) {
	uio_GPDir *subDir;

	subDir = uio_GPDir_prepareSubDir(gPDir, dirName);
	if (subDir->extra == NULL) {
		// It's a new dir, we'll need to add our own data.
		uio_GPDir_ref(gPDir);
		subDir->extra = stdio_GPDirData_new(uio_strdup(dirName),
				NULL, gPDir);
		uio_GPDir_setComplete(subDir, true);
				// fillPDir should not be called.
	}
	uio_GPDir_commitSubDir(gPDir, dirName, subDir);
	return subDir;
}

// returns a pointer to gPDir->extra->cachedPath
// pointer should not be stored, the memory it points to can be freed
// lateron. TODO: not threadsafe.
static char *
stdio_getPath(uio_GPDir *gPDir) {
	if (gPDir->extra->cachedPath == NULL) {
		char *upPath;
		size_t upPathLen, nameLen;
	
		if (gPDir->extra->upDir == NULL) {
#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)
			// Drive letter or UNC \\server\share still needs to follow.
			gPDir->extra->cachedPath = uio_malloc(1);
			gPDir->extra->cachedPath[0] = '\0';
#else
			gPDir->extra->cachedPath = uio_malloc(2);
			gPDir->extra->cachedPath[0] = '/';
			gPDir->extra->cachedPath[1] = '\0';
#endif  /* defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS) */
			return gPDir->extra->cachedPath;
		}
		
		upPath = stdio_getPath(gPDir->extra->upDir);
		if (upPath == NULL) {
			// errno is set
			return NULL;
		}
			
#if defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS)
		if (upPath[0] == '\0') {
			// The up dir is the root dir. Directly below the root dir are
			// only dirs for drive letters and UNC \\share\server parts.
			// No '/' needs to be attached.
			gPDir->extra->cachedPath = uio_strdup(gPDir->extra->name);
			return gPDir->extra->cachedPath;
		}
#endif  /* defined(HAVE_DRIVE_LETTERS) || defined(HAVE_UNC_PATHS) */
		upPathLen = strlen(upPath);
#if !defined(HAVE_DRIVE_LETTERS) && !defined(HAVE_UNC_PATHS)
		if (upPath[upPathLen - 1] == '/') {
			// should only happen for "/"
			upPathLen--;
		}
#endif  /* !defined(HAVE_DRIVE_LETTERS) && !defined(HAVE_UNC_PATHS) */
		nameLen = strlen(gPDir->extra->name);
		if (upPathLen + nameLen + 1 >= PATH_MAX) {
			errno = ENAMETOOLONG;
			return NULL;
		}
		gPDir->extra->cachedPath = uio_malloc(upPathLen + nameLen + 2);
		memcpy(gPDir->extra->cachedPath, upPath, upPathLen);
		gPDir->extra->cachedPath[upPathLen] = '/';
		memcpy(gPDir->extra->cachedPath + upPathLen + 1,
				gPDir->extra->name, nameLen);
		gPDir->extra->cachedPath[upPathLen + nameLen + 1] = '\0';
	}
	return gPDir->extra->cachedPath;
}

static stdio_GPDirData *
stdio_GPDirData_new(char *name, char *cachedPath, uio_GPDir *upDir) {
	stdio_GPDirData *result;
	
	result = stdio_GPDirData_alloc();
	result->name = name;
	result->cachedPath = cachedPath;
	result->upDir = upDir;
	return result;
}

static void
stdio_GPDirData_delete(stdio_GPDirData *gPDirData) {
	if (gPDirData->upDir != NULL)
		uio_GPDir_unref(gPDirData->upDir);
	stdio_GPDirData_free(gPDirData);
}

static inline stdio_GPDirData *
stdio_GPDirData_alloc(void) {
	stdio_GPDirData *result;
	
	result = uio_malloc(sizeof (stdio_GPDirData));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(stdio_GPDirData, (void *) result);
#endif
	return result;
}

static inline void
stdio_GPDirData_free(stdio_GPDirData *gPDirData) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(stdio_GPDirData, (void *) gPDirData);
#endif
	uio_free(gPDirData->name);
	if (gPDirData->cachedPath != NULL)
		uio_free(gPDirData->cachedPath);
	uio_free(gPDirData);
}


