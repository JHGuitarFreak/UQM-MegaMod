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

#define uio_INTERNAL_PHYSICAL
#define uio_INTERNAL_GPHYS
typedef void *uio_NativeHandle;
typedef void *uio_GPRootExtra;
typedef void *uio_GPDirExtra;
typedef void *uio_GPFileExtra;

#include <errno.h>
#include <stdio.h>

#include "gphys.h"
#include "paths.h"
#include "uioport.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

static void uio_GPDir_deepPersistentUnref(uio_GPDir *gPDir);
static uio_GPRoot *uio_GPRoot_alloc(void);

static void uio_GPRoot_free(uio_GPRoot *gPRoot);

static inline uio_GPDir *uio_GPDir_alloc(void);
void uio_GPDir_delete(uio_GPDir *gPDir);
static inline void uio_GPDir_free(uio_GPDir *gPDir);

static inline uio_GPFile *uio_GPFile_alloc(void);
void uio_GPFile_delete(uio_GPFile *gPFile);
static inline void uio_GPFile_free(uio_GPFile *gPFile);

// Call this when you need to edit a file 'dirName' in the GPDir 'gPDir'.
// a new entry is created when necessary.
// uio_gPDirCommitSubDir should be called when you're done with it.
//
// a copy of dirName is made if needed; the caller remains responsible for
// freeing the original
// Allocates a new dir if necessary.
uio_GPDir *
uio_GPDir_prepareSubDir(uio_GPDir *gPDir, const char *dirName) {
	uio_GPDir *subDir;
	uio_GPDirEntry *entry;
	
	entry = uio_GPDirEntries_find(gPDir->entries, dirName);
	if (entry != NULL) {
		if (uio_GPDirEntry_isDir(entry)) {
			// Return existing subdir.
			uio_GPDir_ref((uio_GPDir *) entry);
			return (uio_GPDir *) entry;
		} else {
			// There already exists a file with the same name.
			// This should not happen within one file system.
			fprintf(stderr, "Directory %s shadows file with the same name "
					"from the same filesystem.\n", dirName);
		}
	}

	// return new subdir
	subDir = uio_GPDir_new(gPDir->pRoot, NULL, uio_GPDir_DETACHED);
	// subDir->ref is initialised at 1
	return subDir;
}

// call this when you're done with a dir acquired by a call to
// uio_gPDirPrepareSubDir
void
uio_GPDir_commitSubDir(uio_GPDir *gPDir, const char *dirName,
		uio_GPDir *subDir) {
	if (subDir->flags & uio_GPDir_DETACHED) {
		// New dir.
		// reference to the subDir is passed along to the upDir,
		// so subDir->ref should not be changed.
		uio_GPDirEntries_add(gPDir->entries, dirName, subDir);
		subDir->flags &= ~uio_GPDir_DETACHED;
		if (!(subDir->flags & uio_GPDir_PERSISTENT)) {
			// Persistent dirs have an extra reference.
			uio_GPDir_unref(subDir);
		}
	} else {
		uio_GPDir_unref(subDir);
	}
}

// a copy of fileName is made if needed; the caller remains responsible for
// freeing the original
void
uio_GPDir_addFile(uio_GPDir *gPDir, const char *fileName, uio_GPFile *file) {
	// A file will never already exist in a dir. There can only be
	// one entry in a physical dir with the same name.
	uio_GPDirEntries_add(gPDir->entries, fileName, (uio_GPDirEntry *) file);
}

// Pre: 'fileName' exists in 'gPDir' and is a dir.
void
uio_GPDir_removeFile(uio_GPDir *gPDir, const char *fileName) {
	uio_GPDirEntry *entry;
	uio_GPFile *file;
	uio_bool retVal;

	entry = uio_GPDirEntries_find(gPDir->entries, fileName);
	if (entry == NULL) {
		// This means the file has no associated GPFile.
		// This can happen when the GPFile structure is only used for caching.
		return;
	}

	assert(!uio_GPDirEntry_isDir(entry));
	file = (uio_GPFile *) entry;
	uio_GPFile_unref(file);
	retVal = uio_GPDirEntries_remove(gPDir->entries, fileName);
	assert(retVal);
}

// Pre: 'dirName' exists in 'gPDir' and is a dir.
void
uio_GPDir_removeSubDir(uio_GPDir *gPDir, const char *dirName) {
	uio_GPDirEntry *entry;
	uio_GPDir *subDir;
	uio_bool retVal;

	entry = uio_GPDirEntries_find(gPDir->entries, dirName);
	if (entry == NULL) {
		// This means the directory has no associated GPDir.
		// This can happen when the GPDir structure is only used for caching.
		return;
	}

	assert(uio_GPDirEntry_isDir(entry));
	subDir = (uio_GPDir *) entry;
	if (subDir->flags & uio_GPDir_PERSISTENT) {
		// Persistent dirs have an extra reference.
		uio_GPDir_unref(subDir);
	}
	retVal = uio_GPDirEntries_remove(gPDir->entries, dirName);
	assert(retVal);
}

void
uio_GPDir_setComplete(uio_GPDir *gPDir, uio_bool flag) {
	if (flag) {
		gPDir->flags |= uio_GPDir_COMPLETE;
	} else
		gPDir->flags &= ~uio_GPDir_COMPLETE;
}

int
uio_GPDir_entryCount(const uio_GPDir *gPDir) {
	return uio_GPDirEntries_count(gPDir->entries);
}

static void
uio_GPDir_access(uio_GPDir *gPDir) {
	if (!(gPDir->flags & uio_GPDir_COMPLETE))
		uio_GPDir_fill(gPDir);
}

// The ref counter for the dir entry is not incremented.
uio_GPDirEntry *
uio_GPDir_getGPDirEntry(uio_GPDir *gPDir, const char *name) {
	uio_GPDir_access(gPDir);
	return uio_GPDirEntries_find(gPDir->entries, name);
}

// The ref counter for the dir entry is not incremented.
uio_PDirEntryHandle *
uio_GPDir_getPDirEntryHandle(const uio_PDirHandle *pDirHandle,
		const char *name) {
	uio_GPDirEntry *gPDirEntry;

	gPDirEntry = uio_GPDir_getGPDirEntry(pDirHandle->extra, name);
	if (gPDirEntry == NULL)
		return NULL;
	uio_GPDirEntry_ref(gPDirEntry);
	if (uio_GPDirEntry_isDir(gPDirEntry)) {
		return (uio_PDirEntryHandle *) uio_PDirHandle_new(pDirHandle->pRoot,
				(uio_GPDir *) gPDirEntry);
	} else {
		return (uio_PDirEntryHandle *) uio_PFileHandle_new(pDirHandle->pRoot,
				(uio_GPFile *) gPDirEntry);
	}
}

/*
 * Follow a path starting from a specified physical dir as long as possible.
 * When you can get no further, 'endGPDir' will be filled in with the dir
 * where you ended up, and 'pathRest' will point into the original path. to
 * the beginning of the part that was not matched.
 * It is allowed to have endGPDir point to gPDir and/or restPath
 * point to path when calling this function.
 * returns: 0 if the complete path was matched
 *          ENOENT if some component (the next one) didn't exists
 *          ENODIR if a component (the next one) exists but isn't a dir
 * See also uio_walkPhysicalPath. The difference is that this one works
 * directly on the GPDirs and is faster because of that.
 */
int
uio_walkGPPath(uio_GPDir *startGPDir, const char *path,
		size_t pathLen, uio_GPDir **endGPDir, const char **pathRest) {
	const char *pathEnd;
	const char *partStart, *partEnd;
	char *tempBuf;
	uio_GPDir *gPDir;
	uio_GPDirEntry *entry;
	int retVal;

	gPDir = startGPDir;
	tempBuf = uio_malloc(strlen(path) + 1);
			// XXX: Use a dynamically allocated array when moving to C99.
	pathEnd = path + pathLen;
	getFirstPathComponent(path, pathEnd, &partStart, &partEnd);
	while (1) {
		if (partStart == pathEnd) {
			retVal = 0;
			break;
		}
		memcpy(tempBuf, partStart, partEnd - partStart);
		tempBuf[partEnd - partStart] = '\0';

		entry = uio_GPDir_getGPDirEntry(gPDir, tempBuf);
		if (entry == NULL) {
			retVal = ENOENT;
			break;
		}
		if (!uio_GPDirEntry_isDir(entry)) {
			retVal = ENOTDIR;
			break;
		}
		gPDir = (uio_GPDir *) entry;
		getNextPathComponent(pathEnd, &partStart, &partEnd);
	}

	uio_free(tempBuf);
	*pathRest = partStart;
	*endGPDir = gPDir;
	return retVal;
}

uio_GPDirEntries_Iterator *
uio_GPDir_openEntries(uio_PDirHandle *pDirHandle) {
	uio_GPDir_access(pDirHandle->extra);
	return uio_GPDirEntries_getIterator(pDirHandle->extra->entries);
}

// the start of 'buf' will be filled with pointers to strings
// those strings are stored elsewhere in buf.
// The function returns the number of strings passed along, or -1 for error.
// If there are no more entries, the last pointer will be NULL.
// (this pointer counts towards the return value)
int
uio_GPDir_readEntries(uio_GPDirEntries_Iterator **iterator,
		char *buf, size_t len) {
	char *end;
	char **start;
	int num;
	const char *name;
	size_t nameLen;

	// buf will be filled like this:
	// The start of buf will contain pointers to char *,
	// the end will contain the actual char[] that those pointers point to.
	// The start and the end will grow towards eachother.
	start = (char **) buf;
	end = buf + len;
	num = 0;
	while (!uio_GPDirEntries_iteratorDone(*iterator)) {
		name = uio_GPDirEntries_iteratorName(*iterator);
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
		*iterator = uio_GPDirEntries_iteratorNext(*iterator);
	}
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
uio_GPDir_closeEntries(uio_GPDirEntries_Iterator *iterator) {
	uio_GPDirEntries_freeIterator(iterator);
}

void
uio_GPDir_fill(uio_GPDir *gPDir) {
	if ((gPDir->flags & uio_GPDir_COMPLETE) &&
			!(gPDir->flags & uio_GPDir_NOCACHE))
		return;
	assert(gPDir->pRoot->extra->ops->fillGPDir != NULL);
	gPDir->pRoot->extra->ops->fillGPDir(gPDir);
}

void
uio_GPRoot_deleteGPRootExtra(uio_GPRoot *gPRoot) {
	if (gPRoot->extra == NULL)
		return;
	assert(gPRoot->ops->deleteGPRootExtra != NULL);
	gPRoot->ops->deleteGPRootExtra(gPRoot->extra);
}

void
uio_GPDir_deleteGPDirExtra(uio_GPDir *gPDir) {
	if (gPDir->extra == NULL)
		return;
	assert(gPDir->pRoot->extra->ops->deleteGPDirExtra != NULL);
	gPDir->pRoot->extra->ops->deleteGPDirExtra(gPDir->extra);
}

void
uio_GPFile_deleteGPFileExtra(uio_GPFile *gPFile) {
	if (gPFile->extra == NULL)
		return;
	assert(gPFile->pRoot->extra->ops->deleteGPFileExtra != NULL);
	gPFile->pRoot->extra->ops->deleteGPFileExtra(gPFile->extra);
}

int
uio_gPDirFlagsFromPRootFlags(int flags) {
	int newFlags;
	
	newFlags = 0;
	if (flags & uio_PRoot_NOCACHE)
		newFlags |= uio_GPDir_NOCACHE;
	
	return newFlags;
}

int
uio_gPFileFlagsFromPRootFlags(int flags) {
	int newFlags;
	
	newFlags = 0;
	if (flags & uio_PRoot_NOCACHE)
		newFlags |= uio_GPFile_NOCACHE;
	
	return newFlags;
}

// This function is to be called from the physical layer.
// uio_GPDirHandle is the extra information for an uio_PDirHandle.
// This is in practice a pointer to the uio_GPDir.
void
uio_GPDirHandle_delete(uio_GPDirHandle *gPDirHandle) {
	uio_GPDir_unref((uio_GPDir *) gPDirHandle);
	(void) gPDirHandle;
}

// This function is to be called from the physical layer.
// uio_GPFileHandle is the extra information for an uio_PFileHandle.
// This is in practice a pointer to the uio_GPFile.
void
uio_GPFileHandle_delete(uio_GPFileHandle *gPFileHandle) {
	uio_GPFile_unref((uio_GPFile *) gPFileHandle);
	(void) gPFileHandle;
}

void
uio_GPDirEntry_delete(uio_GPDirEntry *gPDirEntry) {
	if (uio_GPDirEntry_isDir(gPDirEntry)) {
		uio_GPDir_delete((uio_GPDir *) gPDirEntry);
	} else {
		uio_GPFile_delete((uio_GPFile *) gPDirEntry);
	}
}

// note: sets ref count to 1
uio_GPDir *
uio_GPDir_new(uio_PRoot *pRoot, uio_GPDirExtra extra, int flags) {
	uio_GPDir *gPDir;

	gPDir = uio_GPDir_alloc();
	gPDir->ref = 1;
	gPDir->pRoot = pRoot;
	gPDir->entries = uio_GPDirEntries_new();
	gPDir->extra = extra;
	flags |= uio_gPDirFlagsFromPRootFlags(gPDir->pRoot->flags);
	if (pRoot->extra->flags & uio_GPRoot_PERSISTENT)
		flags |= uio_GPDir_PERSISTENT;
	gPDir->flags = flags | uio_GPDirEntry_TYPE_DIR;
	return gPDir;
}

// pre: There are no more references to within the gPDir, and
//      gPDir is the last reference to the gPDir itself.
void
uio_GPDir_delete(uio_GPDir *gPDir) {
#if 0
	uio_GPDirEntry *entry;

	uio_GPDirEntries_Iterator *iterator;

	iterator = uio_GPDirEntries_getIterator(gPDir->entries);
	while (!uio_GPDirEntries_iteratorDone(iterator)) {
		entry = uio_GPDirEntries_iteratorItem(iterator);
		assert(entry->ref == 0);
		if (uio_GPDirEntry_isDir(entry)) {
			uio_GPDir_delete((uio_GPDir *) entry);
		} else {
			uio_GPFile_delete((uio_GPFile *) entry);
		}
		iterator = uio_GPDirEntries_iteratorNext(iterator);
	}
#endif

	assert(gPDir->ref == 0);
	uio_GPDirEntries_deleteHashTable(gPDir->entries);
	uio_GPDir_deleteGPDirExtra(gPDir);
	uio_GPDir_free(gPDir);
}

static void
uio_GPDir_deepPersistentUnref(uio_GPDir *gPDir) {
	uio_GPDirEntry *entry;
	uio_GPDirEntries_Iterator *iterator;

	iterator = uio_GPDirEntries_getIterator(gPDir->entries);
	while (!uio_GPDirEntries_iteratorDone(iterator)) {
		entry = uio_GPDirEntries_iteratorItem(iterator);
		if (uio_GPDirEntry_isDir(entry)) {
			uio_GPDir_deepPersistentUnref((uio_GPDir *) entry);
		} else {
			uio_GPFile_unref((uio_GPFile *) entry);
		}
		iterator = uio_GPDirEntries_iteratorNext(iterator);
	}
	uio_GPDirEntries_freeIterator(iterator);
	if (gPDir->flags & uio_GPDir_PERSISTENT) {
		uio_GPDir_unref(gPDir);
	} else {
		gPDir->flags &= ~uio_GPDir_COMPLETE;
	}
}

static inline uio_GPDir *
uio_GPDir_alloc(void) {
	uio_GPDir *result = uio_malloc(sizeof (uio_GPDir));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_GPDir, (void *) result);
#endif
	return result;
}

static inline void
uio_GPDir_free(uio_GPDir *gPDir) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_GPDir, (void *) gPDir);
#endif
	uio_free(gPDir);
}

// note: sets ref count to 1
uio_GPFile *
uio_GPFile_new(uio_PRoot *pRoot, uio_GPFileExtra extra, int flags) {
	uio_GPFile *gPFile;
	
	gPFile = uio_GPFile_alloc();
	gPFile->ref = 1;
	gPFile->pRoot = pRoot;
	gPFile->extra = extra;
	gPFile->flags = flags;
	return gPFile;
}

void
uio_GPFile_delete(uio_GPFile *gPFile) {
	assert(gPFile->ref == 0);
	uio_GPFile_deleteGPFileExtra(gPFile);
	uio_GPFile_free(gPFile);
}

static inline uio_GPFile *
uio_GPFile_alloc(void) {
	uio_GPFile *result = uio_malloc(sizeof (uio_GPFile));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_GPFile, (void *) result);
#endif
	return result;
}

static inline void
uio_GPFile_free(uio_GPFile *gPFile) {
	uio_free(gPFile);
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_GPFile, (void *) gPFile);
#endif
}

// The ref counter to 'handle' is not incremented.
uio_PRoot *
uio_GPRoot_makePRoot(uio_FileSystemHandler *handler, int pRootFlags,
		uio_GPRoot_Operations *ops, uio_GPRootExtra gPRootExtra, int gPRootFlags,
		uio_Handle *handle, uio_GPDirExtra gPDirExtra, int gPDirFlags) {
	uio_PRoot *result;
	uio_GPDir *gPTopDir;
	uio_GPRoot *gPRoot;

	gPRoot = uio_GPRoot_new(ops, gPRootExtra, gPRootFlags);
	result = uio_PRoot_new(NULL, handler, handle, gPRoot, pRootFlags);

	gPTopDir = uio_GPDir_new(result, gPDirExtra, gPDirFlags);
	if (gPRoot->flags & uio_GPRoot_PERSISTENT)
		uio_GPDir_ref(gPTopDir);
	result->rootDir = uio_GPDir_makePDirHandle(gPTopDir);
	
	return result;	
}

// Pre: there are no more references to PRoot or anything inside it.
int
uio_GPRoot_umount(uio_PRoot *pRoot) {
	uio_PDirHandle *topDirHandle;
	uio_GPDir *topDir;

	topDirHandle = uio_PRoot_getRootDirHandle(pRoot);
	topDir = topDirHandle->extra;
	if (pRoot->extra->flags & uio_GPRoot_PERSISTENT)
		uio_GPDir_deepPersistentUnref(topDir);
	uio_PDirHandle_unref(topDirHandle);
	(void) pRoot;
	return 0;
}

uio_GPRoot *
uio_GPRoot_new(uio_GPRoot_Operations *ops, uio_GPRootExtra extra, int flags) {
	uio_GPRoot *result;
	
	result = uio_GPRoot_alloc();
	result->ops = ops;
	result->extra = extra;
	result->flags = flags;
	return result;
}

void
uio_GPRoot_delete(uio_GPRoot *gPRoot) {
	uio_GPRoot_deleteGPRootExtra(gPRoot);
	uio_GPRoot_free(gPRoot);
}

static uio_GPRoot *
uio_GPRoot_alloc(void) {
	uio_GPRoot *result = uio_malloc(sizeof (uio_GPRoot));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_GPRoot, (void *) result);
#endif
	return result;
}

static void
uio_GPRoot_free(uio_GPRoot *gPRoot) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_GPRoot, (void *) gPRoot);
#endif
	uio_free(gPRoot);
}

// The ref counter to the gPDir is not inremented.
uio_PDirHandle *
uio_GPDir_makePDirHandle(uio_GPDir *gPDir) {
	return uio_PDirHandle_new(gPDir->pRoot, gPDir);
}

#ifdef DEBUG
void
uio_GPDirEntry_print(FILE *outStream, uio_GPDirEntry *gPDirEntry) {
	if (uio_GPDirEntry_isDir(gPDirEntry)) {
		uio_GPDir_print(outStream, (uio_GPDir *) gPDirEntry);
	} else {
		uio_GPFile_print(outStream, (uio_GPFile *) gPDirEntry);
	}
}

void
uio_GPDir_print(FILE *outStream, uio_GPDir *gPDir) {
	(void) outStream;
	(void) gPDir;
}

void
uio_GPFile_print(FILE *outStream, uio_GPFile *gPFile) {
	(void) outStream;
	(void) gPFile;
}
#endif


