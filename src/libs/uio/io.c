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
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "iointrn.h"
#include "ioaux.h"
#include "mount.h"
#include "fstypes.h"
#include "mounttree.h"
#include "physical.h"
#include "paths.h"
#include "mem.h"
#include "uioutils.h"
#include "uioport.h"
#include "../log.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

#if 0
static int uio_accessDir(uio_DirHandle *dirHandle, const char *path,
		int mode);
#endif
static int uio_statDir(uio_DirHandle *dirHandle, const char *path,
		struct stat *statBuf);
static int uio_statOneDir(uio_PDirHandle *pDirHandle, struct stat *statBuf);

static void uio_PDirHandles_delete(uio_PDirHandle *pDirHandles[],
		int numPDirHandles);

static inline uio_PDirHandle *uio_PDirHandle_alloc(void);
static inline void uio_PDirHandle_free(uio_PDirHandle *pDirHandle);
static inline uio_PFileHandle *uio_PFileHandle_alloc(void);
static inline void uio_PFileHandle_free(uio_PFileHandle *pFileHandle);

static uio_DirHandle *uio_DirHandle_new(uio_Repository *repository, char *path,
		char *rootEnd);
static inline uio_DirHandle *uio_DirHandle_alloc(void);
static inline void uio_DirHandle_free(uio_DirHandle *dirHandle);

static inline uio_Handle *uio_Handle_alloc(void);
static inline void uio_Handle_free(uio_Handle *handle);

static uio_MountHandle *uio_MountHandle_new(uio_Repository *repository,
		uio_MountInfo *mountInfo);
static inline void uio_MountHandle_delete(uio_MountHandle *mountHandle);
static inline uio_MountHandle *uio_MountHandle_alloc(void);
static inline void uio_MountHandle_free(uio_MountHandle *mountHandle);



void
uio_init(void) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_init();
#endif
	uio_registerDefaultFileSystems();
}

void
uio_unInit(void) {
	uio_unRegisterDefaultFileSystems();
#ifdef uio_MEM_DEBUG
#	ifdef DEBUG
	uio_MemDebug_printPointers(stderr);
	fflush(stderr);
#	endif
	uio_MemDebug_unInit();
#endif
}

uio_Repository *
uio_openRepository(int flags) {
	return uio_Repository_new(flags);
}

void
uio_closeRepository(uio_Repository *repository) {
	uio_unmountAllDirs(repository);
	uio_Repository_unref(repository);
}

/*
 * Function name: uio_mountDir
 * Description:   Grafts a directory from inside a physical fileSystem
 *                into the locical filesystem, at a specified directory.
 * Arguments:     destRep - the repository where the newly mounted dir
 *                		is to be grafted.
 *                mountPoint - the path to the directory where the dir
 *                		is to be grafted.
 *                fsType - the file system type of physical fileSystem
 *                		pointed to by sourcePath.
 *                sourceDir - the directory to which 'sourcePath' is to
 *                		be taken relative.
 *                sourcePath - a path relative to sourceDir, which contains
 *                		the file/directory to be mounted.
 *                		If sourceDir and sourcePath are NULL, the file
 *                		system of the operating system will be used.
 *                inPath - the location relative to the root of the newly
 *                		mounted fileSystem, pointing to the directory
 *                      that is to be grafted.
 *                      Note: If fsType is uio_FSTYPE_STDIO, inPath is
 *                      relative to the root of the filesystem, NOT to
 *                      the current working dir.
 *                autoMount - array of automount options in function
 *                		in this mountPoint.
 *                flags - one of uio_MOUNT_TOP, uio_MOUNT_BOTTOM,
 *                      uio_MOUNT_BELOW, uio_MOUNT_ABOVE, specifying
 *                      the precedence of this mount, OR'ed with
 *                      one or more of the following flags:
 *                      uio_MOUNT_RDONLY (no writing is allowed)
 *                relative - If 'flags' includes uio_MOUNT_BELOW or
 *                           uio_MOUNT_ABOVE, this is the mount handle
 *                           where the new mount is relative to.
 *                           Otherwise, it should be NULL.
 * Returns:       a handle suitable for uio_unmountDir()
 *                NULL if an error occured. In this case 'errno' is set.
 */
uio_MountHandle *
uio_mountDir(uio_Repository *destRep, const char *mountPoint,
		uio_FileSystemID fsType,
		uio_DirHandle *sourceDir, const char *sourcePath,
		const char *inPath, uio_AutoMount **autoMount, int flags,
		uio_MountHandle *relative) {
	uio_PRoot *pRoot;
	uio_Handle *handle;
	uio_FileSystemHandler *handler;
	uio_MountInfo *relativeInfo;

	switch (flags & uio_MOUNT_LOCATION_MASK) {
		case uio_MOUNT_TOP:
		case uio_MOUNT_BOTTOM:
			if (relative != NULL) {
				errno = EINVAL;
				return NULL;
			}
			relativeInfo = NULL;
			break;
		case uio_MOUNT_BELOW:
		case uio_MOUNT_ABOVE:
			if (relative == NULL) {
				errno = EINVAL;
				return NULL;
			}
			relativeInfo = relative->mountInfo;
			break;
		default:
			abort();
	}

	if (mountPoint[0] == '/')
		mountPoint++;
	if (!validPathName(mountPoint, strlen(mountPoint))) {
		errno = EINVAL;
		return NULL;
	}
	
	// TODO: check if the filesystem is already mounted, and if so, reuse it.
	// A RO filedescriptor will need to be replaced though if the
	// filesystem needs to be remounted RW now.
	if (sourceDir == NULL) {
		if (sourcePath != NULL) {
			// bad: sourceDir is NULL, but sourcePath isn't
			errno = EINVAL;
			return NULL;
		}
		handle = NULL;
	} else {
		if (sourcePath == NULL) {
			// bad: sourcePath is NULL, but sourceDir isn't
			errno = EINVAL;
			return NULL;
		}
		log_add(log_Info, "uio_open %s", sourcePath);
		handle = uio_open(sourceDir, sourcePath,
				((flags & uio_MOUNT_RDONLY) == uio_MOUNT_RDONLY ?
				O_RDONLY : O_RDWR)
#ifdef WIN32
				| O_BINARY
#endif
				, 0);
		if (handle == NULL) {
			log_add(log_Info, "uio_open failed for %s", sourcePath);
			// errno is set
			return NULL;
		}
	}

	handler = uio_getFileSystemHandler(fsType);
	log_add(log_Info, "uio_getFileSystemHandler %p", handler);
	if (handler == NULL) {
		if (handle)
			uio_close(handle);
		errno = ENODEV;
		return NULL;
	}

	assert(handler->mount != NULL);
	pRoot = (handler->mount)(handle, flags);
	if (pRoot == NULL) {
		int savedErrno;

		savedErrno = errno;
		if (handle)
			uio_close(handle);
		errno = savedErrno;
		return NULL;
	}

	if (handle) {
		// Close this reference to handle.
		// The physical layer may store the link in pRoot, in which it
		// will be cleaned up from uio_unmount().
		uio_close(handle);
	}

	// The new file system is ready, now we need to find the specified
	// dir inside it and put it in its place in the mountTree.
	{
		uio_PDirHandle *endDirHandle;
		const char *endInPath;
		char *dirName;
		uio_MountInfo *mountInfo;
		uio_MountTree *mountTree;
		uio_PDirHandle *pRootHandle;
#ifdef BACKSLASH_IS_PATH_SEPARATOR
		char *unixPath;
		
		unixPath = dosToUnixPath(inPath);
		inPath = unixPath;
#endif  /* BACKSLASH_IS_PATH_SEPARATOR */

		if (inPath[0] == '/')
			inPath++;
		pRootHandle = uio_PRoot_getRootDirHandle(pRoot);
		uio_walkPhysicalPath(pRootHandle, inPath, strlen(inPath),
				&endDirHandle, &endInPath);
		if (*endInPath != '\0') {
			// Path inside the filesystem to mount does not exist.
#ifdef BACKSLASH_IS_PATH_SEPARATOR
			uio_free(unixPath);
#endif  /* BACKSLASH_IS_PATH_SEPARATOR */
			uio_PDirHandle_unref(endDirHandle);
			uio_PRoot_unrefMount(pRoot);
			errno = ENOENT;
			return NULL;
		}
	
		dirName = uio_malloc(endInPath - inPath + 1);
		memcpy(dirName, inPath, endInPath - inPath);
		dirName[endInPath - inPath] = '\0';
#ifdef BACKSLASH_IS_PATH_SEPARATOR
		// InPath is a copy with the paths fixed.
		uio_free(unixPath);
#endif  /* BACKSLASH_IS_PATH_SEPARATOR */
		mountInfo = uio_MountInfo_new(fsType, NULL, endDirHandle, dirName,
				autoMount, NULL, flags);
		uio_repositoryAddMount(destRep, mountInfo,
				flags & uio_MOUNT_LOCATION_MASK, relativeInfo);
		mountTree = uio_mountTreeAddMountInfo(destRep, destRep->mountTree,
				mountInfo, mountPoint, flags & uio_MOUNT_LOCATION_MASK,
				relativeInfo);
		// mountTree is the node in destRep->mountTree where mountInfo
		// leads to.
		mountInfo->mountTree = mountTree;
		mountInfo->mountHandle = uio_MountHandle_new(destRep, mountInfo);
		return mountInfo->mountHandle;
	}
}

// Mount a repository directory into same repository at a different location
// From fossil.
uio_MountHandle *
uio_transplantDir(const char *mountPoint, uio_DirHandle *sourceDir, int flags,
		uio_MountHandle *relative) {
	uio_MountInfo *relativeInfo;
	int numPDirHandles;
	uio_PDirHandle **pDirHandles;
	uio_MountTreeItem **treeItems;
	int i;
	uio_MountHandle *handle = NULL;

	if ((flags & uio_MOUNT_RDONLY) != uio_MOUNT_RDONLY) {
		// Only read-only transplants supported atm
		errno = ENOSYS;
		return NULL;
	}

	switch (flags & uio_MOUNT_LOCATION_MASK) {
		case uio_MOUNT_TOP:
		case uio_MOUNT_BOTTOM:
			if (relative != NULL) {
				errno = EINVAL;
				return NULL;
			}
			relativeInfo = NULL;
			break;
		case uio_MOUNT_BELOW:
		case uio_MOUNT_ABOVE:
			if (relative == NULL) {
				errno = EINVAL;
				return NULL;
			}
			relativeInfo = relative->mountInfo;
			break;
		default:
			abort();
	}

	if (mountPoint[0] == '/')
		mountPoint++;
	if (!validPathName(mountPoint, strlen(mountPoint))) {
		errno = EINVAL;
		return NULL;
	}

	if (uio_getPathPhysicalDirs(sourceDir, "", 0,
			&pDirHandles, &numPDirHandles, &treeItems) == -1) {
		// errno is set
		return NULL;
	}
	if (numPDirHandles == 0) {
		errno = ENOENT;
		return NULL;
	}
	
	// TODO: We only transplant the first read-only physical dir that we find
	//    Maybe transplant all of them? We would then have several
	//    uio_MountHandles to return.
	for (i = 0; i < numPDirHandles; ++i) {
		uio_PDirHandle *pDirHandle = pDirHandles[i];
		uio_MountInfo *oldMountInfo = treeItems[i]->mountInfo;
		uio_Repository *rep = oldMountInfo->mountHandle->repository;
		uio_MountInfo *mountInfo;
		uio_MountTree *mountTree;

		// Only interested in read-only dirs in this incarnation
		if (!uio_mountInfoIsReadOnly(oldMountInfo))
			continue;
	
		mountInfo = uio_MountInfo_new(oldMountInfo->fsID, NULL, pDirHandle,
				uio_strdup(""), oldMountInfo->autoMount, NULL, flags);
		// New mount references the same handles
		uio_PDirHandle_ref(pDirHandle);
		uio_PRoot_refMount(pDirHandle->pRoot);

		uio_repositoryAddMount(rep, mountInfo,
				flags & uio_MOUNT_LOCATION_MASK, relativeInfo);
		mountTree = uio_mountTreeAddMountInfo(rep, rep->mountTree,
				mountInfo, mountPoint, flags & uio_MOUNT_LOCATION_MASK,
				relativeInfo);
		// mountTree is the node in rep->mountTree where mountInfo leads to
		mountInfo->mountTree = mountTree;
		mountInfo->mountHandle = uio_MountHandle_new(rep, mountInfo);
		handle = mountInfo->mountHandle;
		break;
	}

	uio_PDirHandles_delete(pDirHandles, numPDirHandles);
	uio_free(treeItems);
	
	if (handle == NULL)
		errno = ENOENT;

	return handle;
}

int
uio_unmountDir(uio_MountHandle *mountHandle) {
	uio_PRoot *pRoot;

	pRoot = mountHandle->mountInfo->pDirHandle->pRoot;
	
	// check if it's in use
#ifdef DEBUG
	if (pRoot->mountRef == 1 && pRoot->handleRef > 0) {
		fprintf(stderr, "Warning: File system to be unmounted still "
				"has file descriptors open. The file system will not "
				"be deallocated until these are all closed.\n");
	}
#endif

	// TODO: lock (and furtheron unlock) repository

	// remove from mount tree
	uio_mountTreeRemoveMountInfo(mountHandle->repository,
			mountHandle->mountInfo->mountTree,
			mountHandle->mountInfo);
	
	// remove from mount list.
	uio_repositoryRemoveMount(mountHandle->repository,
			mountHandle->mountInfo);

	uio_MountInfo_delete(mountHandle->mountInfo);

	uio_MountHandle_delete(mountHandle);
	uio_PRoot_unrefMount(pRoot);
	return 0;
}

int
uio_unmountAllDirs(uio_Repository *repository) {
	int i;

	i = repository->numMounts;
	while (i--)
		uio_unmountDir(repository->mounts[i]->mountHandle);
	return 0;
}

uio_FileSystemID
uio_getMountFileSystemType(uio_MountHandle *mountHandle) {
	return mountHandle->mountInfo->fsID;
}

int
uio_close(uio_Handle *handle) {
	uio_Handle_unref(handle);
	return 0;
}

int
uio_rename(uio_DirHandle *oldDir, const char *oldPath,
		uio_DirHandle *newDir, const char *newPath) {
	uio_PDirHandle *oldPReadDir, *newPReadDir, *newPWriteDir;
	uio_MountInfo *oldReadMountInfo, *newReadMountInfo, *newWriteMountInfo;
	char *oldName, *newName;
	int retVal;

	if (uio_getPhysicalAccess(oldDir, oldPath, O_RDONLY, 0,
			&oldReadMountInfo, &oldPReadDir, NULL,
			NULL, NULL, NULL, &oldName) == -1) {
		// errno is set
		return -1;
	}

	if (uio_getPhysicalAccess(newDir, newPath, O_WRONLY | O_CREAT | O_EXCL,
			uio_GPA_NOWRITE, &newReadMountInfo, &newPReadDir, NULL,
			&newWriteMountInfo, &newPWriteDir, NULL, &newName) == -1) {
		int savedErrno = errno;
		uio_PDirHandle_unref(oldPReadDir);
		uio_free(oldName);
		errno = savedErrno;
		return -1;
	}

	if (oldReadMountInfo != newWriteMountInfo) {
		uio_PDirHandle_unref(oldPReadDir);
		uio_PDirHandle_unref(newPReadDir);
		uio_PDirHandle_unref(newPWriteDir);
		uio_free(oldName);
		uio_free(newName);
		errno = EXDEV;
		return -1;
	}

	if (uio_mountInfoIsReadOnly(oldReadMountInfo)) {
		// XXX: Doesn't uio_getPhysicalAccess already handle this?
		//      It doesn't return EROFS though; perhaps it should.
		uio_PDirHandle_unref(oldPReadDir);
		uio_PDirHandle_unref(newPReadDir);
		uio_PDirHandle_unref(newPWriteDir);
		uio_free(oldName);
		uio_free(newName);
		errno = EROFS;
		return -1;
	}
	
	if (oldReadMountInfo->pDirHandle->pRoot->handler->rename == NULL) {
		uio_PDirHandle_unref(oldPReadDir);
		uio_PDirHandle_unref(newPReadDir);
		uio_PDirHandle_unref(newPWriteDir);
		uio_free(oldName);
		uio_free(newName);
		errno = ENOSYS;
		return -1;
	}
	retVal = (oldReadMountInfo->pDirHandle->pRoot->handler->rename)(
			oldPReadDir, oldName, newPWriteDir, newName);
	if (retVal == -1) {
		int savedErrno = errno;
		uio_PDirHandle_unref(oldPReadDir);
		uio_PDirHandle_unref(newPReadDir);
		uio_PDirHandle_unref(newPWriteDir);
		uio_free(oldName);
		uio_free(newName);
		errno = savedErrno;
		return -1;
	}

	uio_PDirHandle_unref(oldPReadDir);
	uio_PDirHandle_unref(newPReadDir);
	uio_PDirHandle_unref(newPWriteDir);
	uio_free(oldName);
	uio_free(newName);
	return 0;
}

int
uio_access(uio_DirHandle *dir, const char *path, int mode) {
	(void) dir;
	(void) path;
	(void) mode;
	errno = ENOSYS;  // Not implemented.
	return -1;

#if 0
	uio_PDirHandle *pReadDir;
	uio_MountInfo *readMountInfo;
	char *name;
	int result;

	if (uio_getPhysicalAccess(dir, path, O_RDONLY, 0,
			&readMountInfo, &pReadDir, NULL,
			NULL, NULL, NULL, &name) == -1) {
		// XXX: I copied this part from uio_stat(). Is this what I need?
		if (uio_accessDir(dir, path, statBuf) == -1) {
			// errno is set
			return -1;
		}
		return 0;
	}

	if (pReadDir->pRoot->handler->access == NULL) {
		uio_PDirHandle_unref(pReadDir);
		uio_free(name);
		errno = ENOSYS;
		return -1;
	}

	result = (pReadDir->pRoot->handler->access)(pReadDir, name, mode);
	if (result == -1) {
		int savedErrno = errno;
		uio_PDirHandle_unref(pReadDir);
		uio_free(name);
		errno = savedErrno;
		return -1;
	}

	uio_PDirHandle_unref(pReadDir);
	uio_free(name);
	return result;
#endif
}

#if 0
// auxiliary function to uio_access
static int
uio_accessDir(uio_DirHandle *dirHandle, const char *path, int mode) {
	int numPDirHandles;
	uio_PDirHandle **pDirHandles;

	if (mode & R_OK)
	{
		// Read permission is always granted. Nothing to check here.
	}

	if (uio_getPathPhysicalDirs(dirHandle, path, strlen(path),
				&pDirHandles, &numPDirHandles, NULL) == -1) {
		// errno is set
		return -1;
	}

	if (numPDirHandles == 0) {
		errno = ENOENT;
		return -1;
	}

	if (mode & F_OK)
	{
		// We need to check whether each of the directories is complete

		// WORK
	}

	if (mode & W_OK) {
		// If there is any directory where writing is allowed, then
		// we can write.

		// WORK
		errno = ENOENT;
		return -1;

#if 0
		if (uio_statOneDir(pDirHandles[0], statBuf) == -1) {
			int savedErrno = errno;
			uio_PDirHandles_delete(pDirHandles, numPDirHandles);
			errno = savedErrno;
			return -1;
		}
		// TODO: atm, fstat'ing a dir will show the info for the topmost
		//       dir. Maybe it would make sense of merging the bits. (How?)

#if 0
		for (i = 1; i < numPDirHandles; i++) {
			struct stat statOne;
			uio_PDirHandle *pDirHandle;

			if (statOneDir(pDirHandles[i], &statOne) == -1) {
				// errno is set
				int savedErrno = errno;
				uio_PDirHandles_delete(pDirHandles, numPDirHandles);
				errno = savedErrno;
				return -1;
			}

			// Merge dirs:


		}
#endif
#endif
	}

	if (mode & X_OK) {
		// XXX: Not implemented.
		uio_PDirHandles_delete(pDirHandles, numPDirHandles);
		errno = ENOSYS;
		return -1;
	}

	uio_PDirHandles_delete(pDirHandles, numPDirHandles);
	return 0;
}
#endif

int
uio_fstat(uio_Handle *handle, struct stat *statBuf) {
	if (handle->root->handler->fstat == NULL) {
		errno = ENOSYS;
		return -1;
	}
	return (handle->root->handler->fstat)(handle, statBuf);
}

int
uio_stat(uio_DirHandle *dir, const char *path, struct stat *statBuf) {
	uio_PDirHandle *pReadDir;
	uio_MountInfo *readMountInfo;
	char *name;
	int result;

	if (uio_getPhysicalAccess(dir, path, O_RDONLY, 0,
			&readMountInfo, &pReadDir, NULL,
			NULL, NULL, NULL, &name) == -1) {
		if (uio_statDir(dir, path, statBuf) == -1) {
			// errno is set
			return -1;
		}
		return 0;
	}

	if (pReadDir->pRoot->handler->stat == NULL) {
		uio_PDirHandle_unref(pReadDir);
		uio_free(name);
		errno = ENOSYS;
		return -1;
	}

	result = (pReadDir->pRoot->handler->stat)(pReadDir, name, statBuf);
	if (result == -1) {
		int savedErrno = errno;
		uio_PDirHandle_unref(pReadDir);
		uio_free(name);
		errno = savedErrno;
		return -1;
	}

	uio_PDirHandle_unref(pReadDir);
	uio_free(name);
	return result;
}

// auxiliary function to uio_stat
static int
uio_statDir(uio_DirHandle *dirHandle, const char *path,
		struct stat *statBuf) {
	int numPDirHandles;
	uio_PDirHandle **pDirHandles;

	if (uio_getPathPhysicalDirs(dirHandle, path, strlen(path),
				&pDirHandles, &numPDirHandles, NULL) == -1) {
		// errno is set
		return -1;
	}

	if (numPDirHandles == 0) {
		errno = ENOENT;
		return -1;
	}

	if (uio_statOneDir(pDirHandles[0], statBuf) == -1) {
		int savedErrno = errno;
		uio_PDirHandles_delete(pDirHandles, numPDirHandles);
		errno = savedErrno;
		return -1;
	}
	// TODO: atm, fstat'ing a dir will show the info for the topmost
	//       dir. Maybe it would make sense of merging the bits. (How?)

#if 0
	for (i = 1; i < numPDirHandles; i++) {
		struct stat statOne;
		uio_PDirHandle *pDirHandle;

		if (statOneDir(pDirHandles[i], &statOne) == -1) {
			// errno is set
			int savedErrno = errno;
			uio_PDirHandles_delete(pDirHandles, numPDirHandles);
			errno = savedErrno;
			return -1;
		}

		// Merge dirs:


	}
#endif

	uio_PDirHandles_delete(pDirHandles, numPDirHandles);
	return 0;
}

static int
uio_statOneDir(uio_PDirHandle *pDirHandle, struct stat *statBuf) {
	if (pDirHandle->pRoot->handler->stat == NULL) {
		errno = ENOSYS;
		return -1;
	}
	return (pDirHandle->pRoot->handler->stat)(pDirHandle, ".", statBuf);
			// sets errno on error
}

int
uio_mkdir(uio_DirHandle *dir, const char *path, mode_t mode) {
	uio_PDirHandle *pReadDir, *pWriteDir;
	uio_MountInfo *readMountInfo, *writeMountInfo;
	char *name;
	uio_PDirHandle *newDirHandle;

	if (uio_getPhysicalAccess(dir, path, O_WRONLY | O_CREAT | O_EXCL, 0,
			&readMountInfo, &pReadDir, NULL,
			&writeMountInfo, &pWriteDir, NULL, &name) == -1) {
		// errno is set
		if (errno == EISDIR)
			errno = EEXIST;
		return -1;
	}
	uio_PDirHandle_unref(pReadDir);
	
	if (pWriteDir->pRoot->handler->mkdir == NULL) {
		uio_free(name);
		uio_PDirHandle_unref(pWriteDir);
		errno = ENOSYS;
		return -1;
	}

	newDirHandle = (pWriteDir->pRoot->handler->mkdir)(pWriteDir, name, mode);
	if (newDirHandle == NULL) {
		int savedErrno = errno;
		uio_free(name);
		uio_PDirHandle_unref(pWriteDir);
		errno = savedErrno;
		return -1;
	}

	uio_PDirHandle_unref(pWriteDir);
	uio_PDirHandle_unref(newDirHandle);
	uio_free(name);
	return 0;
}

uio_Handle *
uio_open(uio_DirHandle *dir, const char *path, int flags, mode_t mode) {
	uio_PDirHandle *readPDirHandle, *writePDirHandle, *pDirHandle;
	uio_MountInfo *readMountInfo, *writeMountInfo;
	char *name;
	uio_Handle *handle;
	
	if (uio_getPhysicalAccess(dir, path, flags, 0,
			&readMountInfo, &readPDirHandle, NULL,
			&writeMountInfo, &writePDirHandle, NULL, &name) == -1) {
		// errno is set
		log_add(log_Info, "uio_open: uio_getPhysicalAccess  failed for '%s'", path);
		return NULL;
	}
	
	if ((flags & O_ACCMODE) == O_RDONLY) {
		// WritePDirHandle is not filled in.
		pDirHandle = readPDirHandle;
	} else if (readPDirHandle == writePDirHandle) {
		// In general, the dirs can be the same even when the handles are
		// not the same. But here it works, because uio_getPhysicalAccess
		// guarantees it.
		uio_PDirHandle_unref(writePDirHandle);
		pDirHandle = readPDirHandle;
	} else {
		// need to write
		uio_PDirEntryHandle *entry;
		
		entry = uio_getPDirEntryHandle(readPDirHandle, name);
		if (entry != NULL) {
			// file already exists
			uio_PDirEntryHandle_unref(entry);
			if ((flags & O_CREAT) == O_CREAT &&
					(flags & O_EXCL) == O_EXCL) {
				uio_free(name);
				uio_PDirHandle_unref(readPDirHandle);
				uio_PDirHandle_unref(writePDirHandle);
				errno = EEXIST;
				log_add(log_Info, "uio_open: O_CREAT | O_EXCL: file already exists '%s'", name);
				return NULL;
			}
			if ((flags & O_TRUNC) == O_TRUNC) {
				// No use copying the file to the writable dir.
				// As it doesn't exists there, O_TRUNC needs to be turned off
				// though.
				flags &= ~O_TRUNC;
			} else {
				// file needs to be copied
				if (uio_copyFilePhysical(readPDirHandle, name, writePDirHandle,
							name) == -1) {
					int savedErrno = errno;
					uio_free(name);
					uio_PDirHandle_unref(readPDirHandle);
					uio_PDirHandle_unref(writePDirHandle);
					errno = savedErrno;
					log_add(log_Info, "uio_open: uio_copyFilePhysical failed '%s'", name);
					return NULL;
				}
			}
		} else {
			// file does not exist
			if (((flags & O_ACCMODE) == O_RDONLY) ||
					(flags & O_CREAT) != O_CREAT) {
				uio_free(name);
				uio_PDirHandle_unref(readPDirHandle);
				uio_PDirHandle_unref(writePDirHandle);
				errno = ENOENT;
				return NULL;
			}
		}
		uio_PDirHandle_unref(readPDirHandle);
		pDirHandle = writePDirHandle;
	}

	handle = (pDirHandle->pRoot->handler->open)(pDirHandle, name, flags, mode);
			// Also adds a new entry to the physical dir if appropriate.
	if (handle == NULL) {
		int savedErrno = errno;
		log_add(log_Info, "uio_open: open file failed '%s'", name);
		uio_free(name);
		uio_PDirHandle_unref(pDirHandle);
		errno = savedErrno;
		return NULL;
	}

	uio_free(name);
	uio_PDirHandle_unref(pDirHandle);
	return handle;
}

uio_DirHandle *
uio_openDir(uio_Repository *repository, const char *path, int flags) {
	uio_DirHandle *dirHandle;
	const char * const rootStr = "";

	dirHandle = uio_DirHandle_new(repository,
			unconst(rootStr), unconst(rootStr));
			// dirHandle->path will be replaced before uio_openDir()
			// exits()
	if (uio_verifyPath(dirHandle, path, &dirHandle->path) == -1) {
		int savedErrno = errno;
		uio_DirHandle_free(dirHandle);
		errno = savedErrno;
		return NULL;
	}
	// dirHandle->path is no longer equal to 'path' at this point.
	// TODO: increase ref in repository?
	dirHandle->rootEnd = dirHandle->path;
	if (flags & uio_OD_ROOT)
		dirHandle->rootEnd += strlen(dirHandle->path);
	return dirHandle;
}

uio_DirHandle *
uio_openDirRelative(uio_DirHandle *base, const char *path, int flags) {
	uio_DirHandle *dirHandle;
	char *newPath;

	if (uio_verifyPath(base, path, &newPath) == -1) {
		// errno is set
		return NULL;
	}
	if (flags & uio_OD_ROOT) {
		dirHandle = uio_DirHandle_new(base->repository,
				newPath, newPath + strlen(newPath));
		// TODO: increase ref in base->repository?
	} else {
		// use the root of the base dir
		dirHandle = uio_DirHandle_new(base->repository,
				newPath, newPath + (base->rootEnd - base->path));
	}
	return dirHandle;
}

int
uio_closeDir(uio_DirHandle *dirHandle) {
	uio_DirHandle_unref(dirHandle);
	return 0;
}

ssize_t
uio_read(uio_Handle *handle, void *buf, size_t count) {
	return (handle->root->handler->read)(handle, buf, count);
}

int
uio_rmdir(uio_DirHandle *dirHandle, const char *path) {
	int numPDirHandles;
	uio_PDirHandle *pDirHandle, **pDirHandles;
	const char *pathEnd, *name;
	uio_PDirEntryHandle *entry;
	uio_MountTreeItem **items;
	int i;
	int numDeleted;

	pathEnd = strrchr(path, '/');
	if (pathEnd == NULL) {
		pathEnd = path;
		name = path;
	} else
		name = pathEnd + 1;

	if (uio_getPathPhysicalDirs(dirHandle, path, pathEnd - path,
				&pDirHandles, &numPDirHandles, &items) == -1) {
		// errno is set
		return -1;
	}

	entry = NULL;
			// Should be set before a possible goto.

	if (name[0] == '\0') {
		// path was of the form "foo/bar/" or "/foo/bar/"
		// These are intentionally not accepted.
		// I see this as a path and not as a directory identifier.
		errno = ENOENT;
		goto err;
	}

	numDeleted = 0;
	for (i = 0; i < numPDirHandles; i++) {
		pDirHandle = pDirHandles[i];
		entry = uio_getPDirEntryHandle(pDirHandle, name);

		if (entry == NULL)
			continue;

		if (!uio_PDirEntryHandle_isDir(entry)) {
			errno = ENOTDIR;
			goto err;
		}
	
		if (uio_mountInfoIsReadOnly(items[i]->mountInfo)) {
			errno = EROFS;
			goto err;
		}

		if (pDirHandle->pRoot->handler->rmdir == NULL) {
			errno = ENOSYS;
			goto err;
		}

		if ((pDirHandle->pRoot->handler->rmdir)(pDirHandle, name) == -1) {
			// errno is set
			goto err;
		}
		numDeleted++;
		uio_PDirEntryHandle_unref(entry);
	}
	entry = NULL;

	if (numDeleted == 0) {
		errno = ENOENT;
		goto err;
	}

	uio_PDirHandles_delete(pDirHandles, numPDirHandles);
	uio_free(items);
	return 0;

err:
	{
		int savedErrno = errno;
		uio_PDirHandles_delete(pDirHandles, numPDirHandles);
		uio_free(items);
		if (entry != NULL)
			uio_PDirEntryHandle_unref(entry);
		errno = savedErrno;
		return -1;
	}
}

static void
uio_PDirHandles_delete(uio_PDirHandle *pDirHandles[], int numPDirHandles) {
	while (numPDirHandles--)
		uio_PDirHandle_unref(pDirHandles[numPDirHandles]);
	uio_free(pDirHandles);
}

int
uio_lseek(uio_Handle *handle, off_t offset, int whence) {
	if (handle->root->handler->seek == NULL) {
		errno = ENOSYS;
		return -1;
	}
	return (handle->root->handler->seek)(handle, offset, whence);
}

ssize_t
uio_write(uio_Handle *handle, const void *buf, size_t count) {
	if (handle->root->handler->write == NULL) {
		errno = ENOSYS;
		return -1;
	}
	return (handle->root->handler->write)(handle, buf, count);
}

int
uio_unlink(uio_DirHandle *dirHandle, const char *path) {
	int numPDirHandles;
	uio_PDirHandle *pDirHandle, **pDirHandles;
	const char *pathEnd, *name;
	uio_PDirEntryHandle *entry;
	uio_MountTreeItem **items;
	int i;
	int numDeleted;

	pathEnd = strrchr(path, '/');
	if (pathEnd == NULL) {
		pathEnd = path;
		name = path;
	} else
		name = pathEnd + 1;

	if (uio_getPathPhysicalDirs(dirHandle, path, pathEnd - path,
				&pDirHandles, &numPDirHandles, &items) == -1) {
		// errno is set
		return -1;
	}

	entry = NULL;
			// Should be set before a possible goto.

	if (name[0] == '\0') {
		// path was of the form "foo/bar/" or "/foo/bar/"
		errno = ENOENT;
		goto err;
	}

	numDeleted = 0;
	for (i = 0; i < numPDirHandles; i++) {
		pDirHandle = pDirHandles[i];
		entry = uio_getPDirEntryHandle(pDirHandle, name);

		if (entry == NULL)
			continue;

		if (uio_PDirEntryHandle_isDir(entry)) {
			errno = EISDIR;
			goto err;
		}
		
		if (uio_mountInfoIsReadOnly(items[i]->mountInfo)) {
			errno = EROFS;
			goto err;
		}

		if (pDirHandle->pRoot->handler->unlink == NULL) {
			errno = ENOSYS;
			goto err;
		}

		if ((pDirHandle->pRoot->handler->unlink)(pDirHandle, name) == -1) {
			// errno is set
			goto err;
		}
		numDeleted++;
		uio_PDirEntryHandle_unref(entry);
	}
	entry = NULL;

	if (numDeleted == 0) {
		errno = ENOENT;
		goto err;
	}

	uio_PDirHandles_delete(pDirHandles, numPDirHandles);
	uio_free(items);
	return 0;

err:
	{
		int savedErrno = errno;
		uio_PDirHandles_delete(pDirHandles, numPDirHandles);
		uio_free(items);
		if (entry != NULL)
			uio_PDirEntryHandle_unref(entry);
		errno = savedErrno;
		return -1;
	}
}

// inPath and *outPath may point to the same location
int
uio_getFileLocation(uio_DirHandle *dir, const char *inPath,
		int flags, uio_MountHandle **mountHandle, char **outPath) {
	uio_PDirHandle *readPDirHandle, *writePDirHandle;
	uio_MountInfo *readMountInfo, *writeMountInfo, *mountInfo;
	char *name;
	char *readPRootPath, *writePRootPath, *pRootPath;
	
	if (uio_getPhysicalAccess(dir, inPath, flags, 0,
			&readMountInfo, &readPDirHandle, &readPRootPath,
			&writeMountInfo, &writePDirHandle, &writePRootPath,
			&name) == -1) {
		// errno is set
		return -1;
	}

	// TODO: This code is partly the same as the code in uio_open().
	//       probably some code could be put in a seperate function.
	if ((flags & O_ACCMODE) == O_RDONLY) {
		// WritePDirHandle is not filled in.
		uio_PDirHandle_unref(readPDirHandle);
		pRootPath = readPRootPath;
		mountInfo = readMountInfo;
	} else if (readPDirHandle == writePDirHandle) {
		// In general, the dirs can be the same even when the handles are
		// not the same. But here it works, because uio_getPhysicalAccess
		// guarantees it.
		uio_PDirHandle_unref(readPDirHandle);
		uio_PDirHandle_unref(writePDirHandle);
		pRootPath = readPRootPath;
		mountInfo = readMountInfo;
		uio_free(writePRootPath);
	} else {
		// need to write
		uio_PDirEntryHandle *entry;
		
		entry = uio_getPDirEntryHandle(readPDirHandle, name);
		if (entry != NULL) {
			// file already exists
			uio_PDirEntryHandle_unref(entry);
			if ((flags & O_CREAT) == O_CREAT &&
					(flags & O_EXCL) == O_EXCL) {
				uio_free(name);
				uio_PDirHandle_unref(readPDirHandle);
				uio_free(readPRootPath);
				uio_PDirHandle_unref(writePDirHandle);
				uio_free(writePRootPath);
				errno = EEXIST;
				return -1;
			}
			if ((flags & O_TRUNC) == O_TRUNC) {
				// No use copying the file to the writable dir.
				// As it doesn't exists there, O_TRUNC needs to be turned off
				// though.
				flags &= ~O_TRUNC;
			} else {
				// file needs to be copied
				if (uio_copyFilePhysical(readPDirHandle, name, writePDirHandle,
							name) == -1) {
					int savedErrno = errno;
					uio_free(name);
					uio_PDirHandle_unref(readPDirHandle);
					uio_free(readPRootPath);
					uio_PDirHandle_unref(writePDirHandle);
					uio_free(writePRootPath);
					errno = savedErrno;
					return -1;
				}
			}
		} else {
			// file does not exist
			if (((flags & O_ACCMODE) == O_RDONLY) ||
					(flags & O_CREAT) != O_CREAT) {
				uio_free(name);
				uio_PDirHandle_unref(readPDirHandle);
				uio_free(readPRootPath);
				uio_PDirHandle_unref(writePDirHandle);
				uio_free(writePRootPath);
				errno = ENOENT;
				return -1;
			}
		}
		uio_PDirHandle_unref(readPDirHandle);
		uio_PDirHandle_unref(writePDirHandle);
		pRootPath = writePRootPath;
		mountInfo = writeMountInfo;
		uio_free(readPRootPath);
	}
	
	uio_free(name);

	*mountHandle = mountInfo->mountHandle;
	*outPath = pRootPath;
	return 0;
}


// *** begin dirList stuff *** //

#define uio_DIR_BUFFER_SIZE 2048
		// Large values will give significantly better speed for large
		// directories. What is stored in the buffer is file names
		// plus one pointer per file name, so with an average size of 16
		// characters per file (including \0), a buffer of size 2048 will
		// store approximately 100 files.
		// It should be at least big enough to store one entry (NAME_MAX is
		// 255 on POSIX systems).
		// TODO: add a compile-time check for this

typedef struct uio_DirBufferLink {
	char *buffer;
	int numEntries;
	struct uio_DirBufferLink *next;
} uio_DirBufferLink;

static int strPtrCmp(const char * const *ptr1, const char * const *ptr2);
static void uio_DirBufferLink_free(uio_DirBufferLink *sdbl);
static void uio_DirBufferChain_free(uio_DirBufferLink *dirBufferLink);
static uio_DirList *uio_getDirListMulti(uio_PDirHandle **pDirHandles,
		int numPDirHandles, const char *pattern, match_MatchType matchType);
static uio_DirList *uio_makeDirList(const char **newNames,
		const char * const *names, int numNames);
static uio_DirList *uio_DirList_new(const char **names, int numNames,
		char *buffer);
static void uio_collectDirEntries(uio_PDirHandle *pDirHandle,
		uio_DirBufferLink **linkPtr, int *numEntries);
static inline uio_DirList *uio_DirList_alloc(void);
static void uio_filterNames(const char * const *names, int numNames,
		const char **newNames, int *numNewNames,
		match_MatchContext *matchContext);

static uio_EntriesContext *uio_openEntriesPhysical(uio_PDirHandle *dirHandle);
static int uio_readEntriesPhysical(uio_EntriesContext *iterator, char *buf,
		size_t len);
static void uio_closeEntriesPhysical(uio_EntriesContext *iterator);
static uio_EntriesContext *uio_EntriesContext_new(uio_PRoot *pRoot,
		uio_NativeEntriesContext *native);
static inline uio_EntriesContext *uio_EntriesContext_alloc(void);
static inline void uio_EntriesContext_delete(uio_EntriesContext *entriesContext);
static inline void uio_EntriesContext_free(uio_EntriesContext
		*entriesContext);

// The caller may modify the elements of the .names field of the result, but
// .names itself, and the rest of the elements of dirList should be left
// alone, so that they will be freed by uio_DirList_free().
uio_DirList *
uio_getDirList(uio_DirHandle *dirHandle, const char *path, const char *pattern,
		match_MatchType matchType) {
	int numPDirHandles;
	uio_PDirHandle **pDirHandles;
	uio_DirList *result;

	if (uio_getPathPhysicalDirs(dirHandle, path, strlen(path),
				&pDirHandles, &numPDirHandles, NULL) == -1) {
		// errno is set
		return NULL;
	}

	if (numPDirHandles == 0) {
		assert(pDirHandles == NULL);
				// nothing to free
		return uio_DirList_new(NULL, 0, NULL);
	}
	
	result = uio_getDirListMulti(pDirHandles, numPDirHandles, pattern,
			matchType);

	{
		int savedErrno;
		savedErrno = errno;
		uio_PDirHandles_delete(pDirHandles, numPDirHandles);
		errno = savedErrno;
	}
	return result;
}

// Names and newNames may point to the same location.
// numNewNames may point to &numNames.
static void
uio_filterDoubleNames(const char * const *names, int numNames,
		const char **newNames, int *numNewNames) {
	const char * const *endNames;
	const char *prevName;
	int newNum;

	if (numNames == 0) {
		*numNewNames = 0;
		return;
	}

	endNames = names + numNames;
	prevName = *names;
	*newNames = *names;
	newNames++;
	names++;
	newNum = 1;
	while (names < endNames) {
		if (strcmp(prevName, *names) != 0) {
			*newNames = *names;
			newNum++;
			prevName = *names;
			newNames++;
		}
		names++;
	}
	*numNewNames = newNum;
}

static uio_DirList *
uio_getDirListMulti(uio_PDirHandle **pDirHandles,
		int numPDirHandles, const char *pattern, match_MatchType matchType) {
	int pDirI;  // physical dir iterator
	uio_DirBufferLink **links;  // array of bufferLinks for each physical dir
	uio_DirBufferLink *linkPtr;
	int *numNames;  // number of entries in each physical dir
	int totalNumNames;
	const char **bigNameBuffer;  // buffer where all names will end up together
	const char **destPtr;
	uio_DirList *result;
	match_Result matchResult;
	match_MatchContext *matchContext;

	matchResult = match_prepareContext(pattern, &matchContext, matchType);
	if (matchResult != match_OK) {
#ifdef DEBUG
		fprintf(stderr, "Error compiling match function: %s.\n",
				match_errorString(matchContext, matchResult));
#endif
		match_freeContext(matchContext);
		errno = EIO;
				// I actually want to signal an internal error.
				// EIO comes closes
		return NULL;
	}

	// first get the directory listings for all seperate relevant dirs.
	totalNumNames = 0;
	links = uio_malloc(numPDirHandles * sizeof (uio_DirBufferLink *));
	numNames = uio_malloc(numPDirHandles * sizeof (int));
	for (pDirI = 0; pDirI < numPDirHandles; pDirI++) {
		uio_collectDirEntries(pDirHandles[pDirI], &links[pDirI],
				&numNames[pDirI]);
		totalNumNames += numNames[pDirI];
	}

	bigNameBuffer = uio_malloc(totalNumNames * sizeof (uio_DirBufferLink *));	

	// Fill the bigNameBuffer with all the names from all the DirBufferLinks
	// of all the physical dirs.
	destPtr = bigNameBuffer;
	totalNumNames = 0;
	for (pDirI = 0; pDirI < numPDirHandles; pDirI++) {
		for (linkPtr = links[pDirI]; linkPtr != NULL;
				linkPtr = linkPtr->next) {
			int numNewNames;
			uio_filterNames((const char * const *) linkPtr->buffer,
					linkPtr->numEntries, destPtr,
					&numNewNames, matchContext);
			totalNumNames += numNewNames;
			destPtr += numNewNames;
		}
	}

	match_freeContext(matchContext);

	// Sort the bigNameBuffer
	// Necessary for removing doubles.
	// Not really necessary if the big list was the result of only one
	// physical dir, but let's output a sorted list anyhow.
	qsort((void *) bigNameBuffer, totalNumNames, sizeof (char *),
			(int (*)(const void *, const void *)) strPtrCmp);

	// remove doubles
	// (unnecessary if the big list was the result of only one physical dir)
	if (numPDirHandles > 1) {
		uio_filterDoubleNames(bigNameBuffer, totalNumNames,
				bigNameBuffer, &totalNumNames);
	}

	// resize the bigNameBuffer
	bigNameBuffer = uio_realloc((void *) bigNameBuffer,
			totalNumNames * sizeof (char *));

	// put the lot in a DirList, copying the strings themselves
	result = uio_makeDirList(bigNameBuffer, bigNameBuffer,
			totalNumNames);

	// free the old junk
	for (pDirI = 0; pDirI < numPDirHandles; pDirI++)
		uio_DirBufferChain_free(links[pDirI]);
	uio_free(links);
	uio_free(numNames);

	return result;
}

// 'buffer' and 'names' may be the same dir
// 'names' contains an array of 'numNames' pointers.
// 'newNames', if non-NULL, will be used as the array of new pointers
// (to a copy of the strings) in the DirList.
static uio_DirList *
uio_makeDirList(const char **newNames, const char * const *names,
		int numNames) {
	int i;
	size_t len, totLen;
	char *bufPtr;
	uio_DirList *result;

	if (newNames == NULL)
		newNames = uio_malloc(numNames * sizeof (char *));

	totLen = 0;
	for (i = 0; i < numNames; i++)
		totLen += strlen(names[i]);
	totLen += numNames;
			// for the \0's

	result = uio_DirList_new(newNames, numNames, uio_malloc(totLen));

	bufPtr = result->buffer;
	for (i = 0; i < numNames; i++) {
		len = strlen(names[i]) + 1;
		memcpy(bufPtr, names[i], len);
		newNames[i] = bufPtr;
		bufPtr += len;
	}
	return result;
}

static void
uio_collectDirEntries(uio_PDirHandle *pDirHandle, uio_DirBufferLink **linkPtr,
		int *numEntries) {
	uio_EntriesContext *entriesContext;
	uio_DirBufferLink **linkEndPtr;  // where to attach the next link
	int numRead;
	int totalEntries;
	char *buffer;

	entriesContext = uio_openEntriesPhysical(pDirHandle);
	if (entriesContext == NULL) {
#ifdef DEBUG
		fprintf(stderr, "Error: uio_openEntriesPhysical() failed: %s\n",
				strerror(errno));
#endif
		*linkPtr = NULL;
		*numEntries = 0;
		return;
	}
	linkEndPtr = linkPtr;
	totalEntries = 0;
	while (1) {
		*linkEndPtr = uio_malloc(sizeof (uio_DirBufferLink));
		buffer = uio_malloc(uio_DIR_BUFFER_SIZE);
		(*linkEndPtr)->buffer = buffer;
		numRead = uio_readEntriesPhysical(entriesContext, buffer,
				uio_DIR_BUFFER_SIZE);
		if (numRead == 0) {
			fprintf(stderr, "Warning: uio_DIR_BUFFER_SIZE is too small to "
					"hold a certain large entry on its own!\n");
			uio_DirBufferLink_free(*linkEndPtr);
			break;
		}
		totalEntries += numRead;
		(*linkEndPtr)->numEntries = numRead;
		if (((char **) buffer)[numRead - 1] == NULL) {
			// The entry being NULL means this is the last buffer
			// Decrement the amount of queries to get the real number.
			(*linkEndPtr)->numEntries--;
			totalEntries--;
			linkEndPtr = &(*linkEndPtr)->next;
			break;
		}
		linkEndPtr = &(*linkEndPtr)->next;
	}
	*linkEndPtr = NULL;
	uio_closeEntriesPhysical(entriesContext);
	*numEntries = totalEntries;
}

static void
uio_filterNames(const char * const *names, int numNames,
		const char **newNames, int *numNewNames,
		match_MatchContext *matchContext) {
	int newNum;
	const char * const *namesEnd;
	match_Result matchResult;

	newNum = 0;
	namesEnd = names + numNames;
	while (names < namesEnd) {
		matchResult = match_matchPattern(matchContext, *names);
		if (matchResult == match_MATCH) {
			*newNames = *names;
			newNames++;
			newNum++;
		} else if (matchResult != match_NOMATCH) {
			fprintf(stderr, "Error trying to match pattern: %s.\n",
					match_errorString(matchContext, matchResult));
		}
		names++;
	}
	*numNewNames = newNum;
}

static int
strPtrCmp(const char * const *ptr1, const char * const *ptr2) {
	return strcmp(*ptr1, *ptr2);
}

static uio_EntriesContext *
uio_openEntriesPhysical(uio_PDirHandle *dirHandle) {
	uio_NativeEntriesContext *native;
	uio_PRoot *pRoot;

	pRoot = dirHandle->pRoot;

	assert(pRoot->handler->openEntries != NULL);
	native = pRoot->handler->openEntries(dirHandle);
	if (native == NULL)
		return NULL;
	uio_PRoot_refHandle(pRoot);
	return uio_EntriesContext_new(pRoot, native);
}

static int
uio_readEntriesPhysical(uio_EntriesContext *iterator, char *buf, size_t len) {
	assert(iterator->pRoot->handler->readEntries != NULL);
	return iterator->pRoot->handler->readEntries(&iterator->native, buf, len);
}

static void
uio_closeEntriesPhysical(uio_EntriesContext *iterator) {
	assert(iterator->pRoot->handler->closeEntries != NULL);
	iterator->pRoot->handler->closeEntries(iterator->native);
	uio_PRoot_unrefHandle(iterator->pRoot);
	uio_EntriesContext_delete(iterator);
}

static uio_EntriesContext *
uio_EntriesContext_new(uio_PRoot *pRoot, uio_NativeEntriesContext *native) {
	uio_EntriesContext *result;
	result = uio_EntriesContext_alloc();
	result->pRoot = pRoot;
	result->native = native;
	return result;
}

static inline uio_EntriesContext *
uio_EntriesContext_alloc(void) {
	return uio_malloc(sizeof (uio_EntriesContext));
}

static inline void
uio_EntriesContext_delete(uio_EntriesContext *entriesContext) {
	uio_EntriesContext_free(entriesContext);
}

static inline void
uio_EntriesContext_free(uio_EntriesContext *entriesContext) {
	uio_free(entriesContext);
}

static void
uio_DirBufferLink_free(uio_DirBufferLink *dirBufferLink) {
	uio_free(dirBufferLink->buffer);
	uio_free(dirBufferLink);
}

static void
uio_DirBufferChain_free(uio_DirBufferLink *dirBufferLink) {
	uio_DirBufferLink *next;
	
	while (dirBufferLink != NULL) {
		next = dirBufferLink->next;
		uio_DirBufferLink_free(dirBufferLink);
		dirBufferLink = next;
	}
}

static uio_DirList *
uio_DirList_new(const char **names, int numNames, char *buffer) {
	uio_DirList *result;
	
	result = uio_DirList_alloc();
	result->names = names;
	result->numNames = numNames;
	result->buffer = buffer;
	return result;
}

static uio_DirList *
uio_DirList_alloc(void) {
	return uio_malloc(sizeof (uio_DirList));
}

void
uio_DirList_free(uio_DirList *dirList) {
	if (dirList->buffer)
		uio_free(dirList->buffer);
	if (dirList->names)
		uio_free((void *) dirList->names);
	uio_free(dirList);
}

// *** end DirList stuff *** //


// *** PDirEntryHandle stuff *** //

uio_PDirEntryHandle *
uio_getPDirEntryHandle(const uio_PDirHandle *dirHandle,
		const char *name) {
	assert(dirHandle->pRoot->handler != NULL);
	return dirHandle->pRoot->handler->getPDirEntryHandle(dirHandle, name);
}

void
uio_PDirHandle_deletePDirHandleExtra(uio_PDirHandle *pDirHandle) {
	if (pDirHandle->extra == NULL)
		return;
	assert(pDirHandle->pRoot->handler->deletePDirHandleExtra != NULL);
	pDirHandle->pRoot->handler->deletePDirHandleExtra(pDirHandle->extra);
}

void
uio_PFileHandle_deletePFileHandleExtra(uio_PFileHandle *pFileHandle) {
	if (pFileHandle->extra == NULL)
		return;
	assert(pFileHandle->pRoot->handler->deletePFileHandleExtra != NULL);
	pFileHandle->pRoot->handler->deletePFileHandleExtra(pFileHandle->extra);
}

uio_PDirHandle *
uio_PDirHandle_new(uio_PRoot *pRoot, uio_PDirHandleExtra extra) {
	uio_PDirHandle *result;

	result = uio_PDirHandle_alloc();
	result->flags = uio_PDirEntryHandle_TYPE_DIR;
	result->ref = 1;
	result->pRoot = pRoot;
	result->extra = extra;
	return result;
}

void
uio_PDirEntryHandle_delete(uio_PDirEntryHandle *pDirEntryHandle) {
	if (uio_PDirEntryHandle_isDir(pDirEntryHandle)) {
		uio_PDirHandle_delete((uio_PDirHandle *) pDirEntryHandle);
	} else {
		uio_PFileHandle_delete((uio_PFileHandle *) pDirEntryHandle);
	}
}

void
uio_PDirHandle_delete(uio_PDirHandle *pDirHandle) {
	uio_PDirHandle_deletePDirHandleExtra(pDirHandle);
	uio_PDirHandle_free(pDirHandle);
}

static inline uio_PDirHandle *
uio_PDirHandle_alloc(void) {
	uio_PDirHandle *result = uio_malloc(sizeof (uio_PDirHandle));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_PDirHandle, (void *) result);
#endif
	return result;
}

static inline void
uio_PDirHandle_free(uio_PDirHandle *pDirHandle) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_PDirHandle, (void *) pDirHandle);
#endif
	uio_free(pDirHandle);
}

uio_PFileHandle *
uio_PFileHandle_new(uio_PRoot *pRoot, uio_PFileHandleExtra extra) {
	uio_PFileHandle *result;

	result = uio_PFileHandle_alloc();
	result->flags = uio_PDirEntryHandle_TYPE_REG;
	result->ref = 1;
	result->pRoot = pRoot;
	result->extra = extra;
	return result;
}

void
uio_PFileHandle_delete(uio_PFileHandle *pFileHandle) {
	uio_PFileHandle_deletePFileHandleExtra(pFileHandle);
	uio_PFileHandle_free(pFileHandle);
}

static inline uio_PFileHandle *
uio_PFileHandle_alloc(void) {
	uio_PFileHandle *result = uio_malloc(sizeof (uio_PFileHandle));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_PFileHandle, (void *) result);
#endif
	return result;
}

static inline void
uio_PFileHandle_free(uio_PFileHandle *pFileHandle) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_PFileHandle, (void *) pFileHandle);
#endif
	uio_free(pFileHandle);
}

// *** PDirEntryHandle stuff *** //


// ref count set to 1
uio_Handle *
uio_Handle_new(uio_PRoot *root, uio_NativeHandle native, int openFlags) {
	uio_Handle *handle;
	
	handle = uio_Handle_alloc();
	handle->ref = 1;
	uio_PRoot_refHandle(root);
	handle->root = root;
	handle->native = native;
	handle->openFlags = openFlags;
	return handle;
}

void
uio_Handle_delete(uio_Handle *handle) {
	(handle->root->handler->close)(handle);
	uio_PRoot_unrefHandle(handle->root);
	uio_Handle_free(handle);
}

static inline uio_Handle *
uio_Handle_alloc(void) {
	uio_Handle *result = uio_malloc(sizeof (uio_Handle));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_Handle, (void *) result);
#endif
	return result;
}

static inline void
uio_Handle_free(uio_Handle *handle) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_Handle, (void *) handle);
#endif
	uio_free(handle);
}

// ref count set to 1
static uio_DirHandle *
uio_DirHandle_new(uio_Repository *repository, char *path, char *rootEnd) {
	uio_DirHandle *result;
	
	result = uio_DirHandle_alloc();
	result->ref = 1;
	result->repository = repository;
	result->path = path;
	result->rootEnd = rootEnd;
	return result;
}

void
uio_DirHandle_delete(uio_DirHandle *dirHandle) {
	uio_free(dirHandle->path);
	uio_DirHandle_free(dirHandle);
}

static inline uio_DirHandle *
uio_DirHandle_alloc(void) {
	uio_DirHandle *result = uio_malloc(sizeof (uio_DirHandle));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_DirHandle, (void *) result);
#endif
	return result;
}

static inline void
uio_DirHandle_free(uio_DirHandle *dirHandle) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_DirHandle, (void *) dirHandle);
#endif
	uio_free(dirHandle);
}

void
uio_DirHandle_print(const uio_DirHandle *dirHandle, FILE *out) {
	fprintf(out, "[");
	fwrite(dirHandle->path, dirHandle->path - dirHandle->rootEnd, 1, out);
	fprintf(out, "]%s", dirHandle->rootEnd);
}

static uio_MountHandle *
uio_MountHandle_new(uio_Repository *repository, uio_MountInfo *mountInfo) {
	uio_MountHandle *result;
	
	result = uio_MountHandle_alloc();
	result->repository = repository;
	result->mountInfo = mountInfo;
	return result;
}

static inline void
uio_MountHandle_delete(uio_MountHandle *mountHandle) {
	uio_MountHandle_free(mountHandle);
}

static inline uio_MountHandle *
uio_MountHandle_alloc(void) {
	uio_MountHandle *result = uio_malloc(sizeof (uio_MountHandle));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_MountHandle, (void *) result);
#endif
	return result;
}

static inline void
uio_MountHandle_free(uio_MountHandle *mountHandle) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_MountHandle, (void *) mountHandle);
#endif
	uio_free(mountHandle);
}



