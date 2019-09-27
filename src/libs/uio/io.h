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

#ifndef LIBS_UIO_IO_H_
#define LIBS_UIO_IO_H_

#include <assert.h>
#include <sys/stat.h>

typedef struct uio_Handle uio_Handle;
typedef struct uio_DirHandle uio_DirHandle;
typedef struct uio_DirList uio_DirList;
typedef struct uio_MountHandle uio_MountHandle;

typedef enum {
	uio_MOUNT_BOTTOM = (0 << 2),
	uio_MOUNT_TOP =    (1 << 2),
	uio_MOUNT_BELOW = (2 << 2),
	uio_MOUNT_ABOVE =  (3 << 2)
} uio_MountLocation;

#include "match.h"
#include "fstypes.h"
#include "mount.h"
#include "mounttree.h"
#include "uiostream.h"
#include "getint.h"
#include "debug.h"

struct uio_AutoMount {
	const char *pattern;
	match_MatchType matchType;
	uio_FileSystemID fileSystemID;
	int mountFlags;
//	uio_AutoMount **autoMount;
		// automount rules to apply to file systems automounted
		// because of this automount rule.
};

#ifndef uio_INTERNAL
struct uio_DirList {
	const char **names;
	int numNames;
	// The rest of the fields are not visible from the outside
};
#endif


// Initialise the resource system
void uio_init(void);

// Uninitialise the resource system
void uio_unInit(void);

// Open a repository.
uio_Repository *uio_openRepository(int flags);

// Close a repository opened by uio_openRepository().
void uio_closeRepository(uio_Repository *repository);

// Mount a directory into a repository
uio_MountHandle *uio_mountDir(uio_Repository *destRep, const char *mountPoint,
		uio_FileSystemID fsType,
		uio_DirHandle *sourceDir, const char *sourcePath,
		const char *inPath, uio_AutoMount **autoMount, int flags,
		uio_MountHandle *relative);

// Mount a repository directory into same repository at a different
// location.
// From fossil.
uio_MountHandle *uio_transplantDir(const char *mountPoint,
		uio_DirHandle *sourceDir, int flags, uio_MountHandle *relative);

// Unmount a previously mounted dir.
int uio_unmountDir(uio_MountHandle *mountHandle);

// Unmount all previously mounted dirs.
int uio_unmountAllDirs(uio_Repository *repository);

// Get the filesystem identifier for a mounted directory.
uio_FileSystemID uio_getMountFileSystemType(uio_MountHandle *mountHandle);

// Open a file
uio_Handle *uio_open(uio_DirHandle *dir, const char *file, int flags,
		mode_t mode);

// Close a file descriptor for a file opened by uio_open
int uio_close(uio_Handle *handle);

// Rename or move a file or directory.
int uio_rename(uio_DirHandle *oldDir, const char *oldPath,
		uio_DirHandle *newDir, const char *newPath);

// Test permissions on a file or directory.
int uio_access(uio_DirHandle *dir, const char *path, int mode);

// Fstat a file descriptor
int uio_fstat(uio_Handle *handle, struct stat *statBuf);

int uio_stat(uio_DirHandle *dir, const char *path, struct stat *statBuf);

int uio_mkdir(uio_DirHandle *dir, const char *name, mode_t mode);

ssize_t uio_read(uio_Handle *handle, void *buf, size_t count);

int uio_rmdir(uio_DirHandle *dirHandle, const char *path);

int uio_lseek(uio_Handle *handle, off_t offset, int whence);

ssize_t uio_write(uio_Handle *handle, const void *buf, size_t count);

int uio_unlink(uio_DirHandle *dirHandle, const char *path);

int uio_getFileLocation(uio_DirHandle *dir, const char *inPath,
		int flags, uio_MountHandle **mountHandle, char **outPath);

// Get a directory handle.
uio_DirHandle *uio_openDir(uio_Repository *repository, const char *path,
		int flags);
#define uio_OD_ROOT 1

// Get a directory handle using a path relative to another handle.
uio_DirHandle *uio_openDirRelative(uio_DirHandle *base, const char *path,
		int flags);

// Release a directory handle
int uio_closeDir(uio_DirHandle *dirHandle);

uio_DirList *uio_getDirList(uio_DirHandle *dirHandle, const char *path,
		const char *pattern, match_MatchType matchType);
void uio_DirList_free(uio_DirList *dirList);

// For debugging purposes
void uio_DirHandle_print(const uio_DirHandle *dirHandle, FILE *out);


#ifdef DEBUG
#	define uio_DEBUG
#endif

#endif  /* LIBS_UIO_IO_H_ */

