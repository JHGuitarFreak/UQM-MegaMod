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

#ifndef LIBS_UIO_GPHYS_H_
#define LIBS_UIO_GPHYS_H_

#include "uioport.h"

#ifndef uio_INTERNAL_PHYSICAL
typedef void *uio_GPRootExtra;
typedef void *uio_GPDirExtra;
typedef void *uio_GPFileExtra;
#endif

typedef struct CharHashTable_HashTable uio_GPDirEntries;

#define uio_GPDirEntries_new() \
		((uio_GPDirEntries *) CharHashTable_newHashTable(NULL, NULL, NULL, \
		NULL, NULL, 0, 0.85, 0.9))
#define uio_GPDirEntries_add(hashTable, name, item) \
		CharHashTable_add((CharHashTable_HashTable *) hashTable, name, \
		(void *) item)
#define uio_GPDirEntries_remove(hashTable, name) \
		CharHashTable_remove((CharHashTable_HashTable *) hashTable, name)
#define uio_GPDirEntries_count(hashTable) \
		CharHashTable_count((CharHashTable_HashTable *) hashTable)
#define uio_GPDirEntries_find(hashTable, name) \
		((uio_GPDirEntry *) CharHashTable_find( \
		(CharHashTable_HashTable *) hashTable, name))
#define uio_GPDirEntries_deleteHashTable(hashTable) \
		CharHashTable_deleteHashTable((CharHashTable_HashTable *) hashTable)
//#define uio_GPDirEntries_clear(hashTable)
//		CharHashTable_clear((CharHashTable_HashTable *) hashTable)
#define uio_GPDirEntries_getIterator(hashTable) \
		((uio_GPDirEntries_Iterator *) CharHashTable_getIterator( \
		(const CharHashTable_HashTable *) hashTable))
#define uio_GPDirEntries_iteratorDone(iterator) \
		CharHashTable_iteratorDone((const CharHashTable_Iterator *) iterator)
#define uio_GPDirEntries_iteratorName(iterator) \
		CharHashTable_iteratorKey((CharHashTable_Iterator *) iterator)
#define uio_GPDirEntries_iteratorItem(iterator) \
		((uio_GPDirEntry *) CharHashTable_iteratorValue( \
		(CharHashTable_Iterator *) iterator))
#define uio_GPDirEntries_iteratorNext(iterator) \
		((uio_GPDirEntries_Iterator *) CharHashTable_iteratorNext( \
		(CharHashTable_Iterator *) iterator))
#define uio_GPDirEntries_freeIterator(iterator) \
		CharHashTable_freeIterator(iterator)

// 'forward' declarations
typedef struct uio_GPDirEntry uio_GPDirEntry;
typedef struct uio_GPDir uio_GPDir;
typedef struct uio_GPFile uio_GPFile;
typedef struct uio_GPRoot_Operations uio_GPRoot_Operations;
typedef struct uio_GPRoot uio_GPRoot;

#include "charhashtable.h"
typedef CharHashTable_Iterator uio_GPDirEntries_Iterator;

#ifdef uio_INTERNAL_GPHYS
typedef uio_GPDirEntries_Iterator *uio_NativeEntriesContext;
#endif
typedef struct uio_GPRoot *uio_PRootExtra;
typedef struct uio_GPDir uio_GPDirHandle;
typedef uio_GPDirHandle *uio_PDirHandleExtra;
typedef struct uio_GPFile uio_GPFileHandle;
typedef uio_GPFileHandle *uio_PFileHandleExtra;


#ifdef DEBUG
#	include <stdio.h>
#endif
#include "iointrn.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

struct uio_GPRoot_Operations {
	void (*fillGPDir)(uio_GPDir *);
	void (*deleteGPRootExtra)(uio_GPRootExtra);
	void (*deleteGPDirExtra)(uio_GPDirExtra);
	void (*deleteGPFileExtra)(uio_GPFileExtra);
};

struct uio_GPRoot {
	int flags;
#define uio_GPRoot_PERSISTENT 0x4000
			/* Set if directories in this file system should not be deleted
			 * as long as the file system is mounted. If this flag is not
			 * set, the GPDir structure is only a cache.
			 */
	uio_GPRoot_Operations *ops;
	uio_GPRootExtra extra;
};

#define uio_GPDirEntry_COMMON \
	int flags; \
	int ref; \
			/* Number of times this structure is referenced from the \
			 * outside (so not counting the references from subdirs \
			 * or files when the entry is a directory) \
			 */

#define uio_GPDirEntry_NOCACHE   uio_PRoot_NOCACHE

/*
 * uio_GPDirEntry
 * super-'class' of uio_GPDir and uio_GPFile
 */
struct uio_GPDirEntry {
	uio_GPDirEntry_COMMON
	void *extra;
};

#define uio_GPDirEntry_TYPE_REG  0x0000
#define uio_GPDirEntry_TYPE_DIR  0x0001
#define uio_GPDirEntry_TYPEMASK  0x0001

/*
 * uio_GPDir
 * Represents a directory in a physical directory structure.
 * sub-'class' of uio_GPDirEntry
 */
struct uio_GPDir {
	uio_GPDirEntry_COMMON
#			define uio_GPDir_NOCACHE  uio_GPDirEntry_NOCACHE
			/* This directory info will not be cached.
			 * PDIR_COMPLETE is irrelevant in this case */
#			define uio_GPDir_COMPLETE 0x1000
			/* Set if fillDir should not be called if an entry does not
			 * exist in a directory. Usually set if the entire dir has been
			 * completely read in.
			 */
#			define uio_GPDir_DETACHED 0x2000
			/* Set if this dir is not linked to from elsewhere in the physical
			 * structure */
#			define uio_GPDir_PERSISTENT 0x4000
			/* Set if this dir should not be deleted as long as the file
			 * system is mounted. If this flag is not set, the GPDir
			 * structure is only a cache.
			 */
	uio_GPDirExtra extra;
			/* extra internal data for some filesystem types */
	uio_PRoot *pRoot;
	uio_GPDirEntries *entries;
};


/*
 * uio_GPFile
 * Represents a file in a physical directory structure.
 * sub-'class' of uio_GPDirEntry
 */
struct uio_GPFile {
	uio_GPDirEntry_COMMON
#			define uio_GPFile_NOCACHE  uio_GPDirEntry_NOCACHE
			/* Info on this file will not be cached. */
	uio_GPFileExtra extra;
			/* extra internal data for some filesystem types */
	uio_PRoot *pRoot;
};


static inline uio_bool
uio_GPDirEntry_isReg(uio_GPDirEntry *gPDirEntry) {
	return (gPDirEntry->flags & uio_GPDirEntry_TYPEMASK) ==
			uio_GPDirEntry_TYPE_REG;
}

static inline uio_bool
uio_GPDirEntry_isDir(uio_GPDirEntry *gPDirEntry) {
	return (gPDirEntry->flags & uio_GPDirEntry_TYPEMASK) ==
			uio_GPDirEntry_TYPE_DIR;
}


#ifdef DEBUG
void uio_GPDirEntry_print(FILE *outStream, uio_GPDirEntry *gPDirEntry);
void uio_GPDir_print(FILE *outStream, uio_GPDir *gPDir);
void uio_GPFile_print(FILE *outStream, uio_GPFile *pFile);
#endif

uio_NativeEntriesContext uio_GPDir_openEntries(uio_PDirHandle *pDirHandle);
int uio_GPDir_readEntries(uio_NativeEntriesContext *iterator,
		char *buf, size_t len);
void uio_GPDir_closeEntries(uio_NativeEntriesContext iterator);
int uio_GPDir_entryCount(const uio_GPDir *gPDir);

int uio_gPDirFlagsFromPRootFlags(int flags);
int uio_gPFileFlagsFromPRootFlags(int flags);
uio_PRoot *uio_GPRoot_makePRoot(uio_FileSystemHandler *handler, int pRootFlags,
		uio_GPRoot_Operations *ops, uio_GPRootExtra gPRootExtra, int gPRootFlags,
		uio_Handle *handle, uio_GPDirExtra gPDirExtra, int gPDirFlags);
int uio_GPRoot_umount(uio_PRoot *pRoot);

uio_GPDir *uio_GPDir_prepareSubDir(uio_GPDir *gPDir, const char *dirName);
void uio_GPDir_commitSubDir(uio_GPDir *gPDir, const char *dirName,
		uio_GPDir *subDir);
void uio_GPDir_addFile(uio_GPDir *gPDir, const char *fileName,
		uio_GPFile *file);
void uio_GPDir_removeFile(uio_GPDir *gPDir, const char *fileName);
void uio_GPDir_removeSubDir(uio_GPDir *gPDir, const char *dirName);
void uio_GPDir_setComplete(uio_GPDir *gPDir, uio_bool flag);
uio_GPDirEntry *uio_GPDir_getGPDirEntry(uio_GPDir *gPDir,
		const char *name);
uio_PDirEntryHandle *uio_GPDir_getPDirEntryHandle(
		const uio_PDirHandle *pDirHandle, const char *name);
int uio_walkGPPath(uio_GPDir *startGPDir, const char *path,
		size_t pathLen, uio_GPDir **endGPDir, const char **pathRest);
uio_PDirHandle *uio_GPDir_makePDirHandle(uio_GPDir *gPDir);

void uio_GPDir_fill(uio_GPDir *gPDir);
void uio_GPRoot_deleteGPRootExtra(uio_GPRoot *gPRoot);
void uio_GPDir_deleteGPDirExtra(uio_GPDir *gPDir);
void uio_GPFile_deleteGPFileExtra(uio_GPFile *gPFile);

void uio_GPDirHandle_delete(uio_GPDirHandle *gPDirHandle);
void uio_GPFileHandle_delete(uio_GPFileHandle *gPFileHandle);
void uio_GPDirEntry_delete(uio_GPDirEntry *gPDirEntry);
uio_GPRoot *uio_GPRoot_new(uio_GPRoot_Operations *ops, uio_GPRootExtra extra,
		int flags);
void uio_GPRoot_delete(uio_GPRoot *gPRoot);
uio_GPDir *uio_GPDir_new(uio_PRoot *pRoot, uio_GPDirExtra extra, int flags);
void uio_GPDir_delete(uio_GPDir *gPDir);
uio_GPFile *uio_GPFile_new(uio_PRoot *pRoot, uio_GPFileExtra extra, int flags);
void uio_GPFile_delete(uio_GPFile *gPFile);


static inline void
uio_GPDirEntry_ref(uio_GPDirEntry *gPDirEntry) {
#ifdef uio_MEM_DEBUG
	if (uio_GPDirEntry_isDir(gPDirEntry)) {
		uio_MemDebug_debugRef(uio_GPDir, (void *) gPDirEntry);
	} else {
		uio_MemDebug_debugRef(uio_GPFile, (void *) gPDirEntry);
	}
#endif
	gPDirEntry->ref++;
}

static inline void
uio_GPDirEntry_unref(uio_GPDirEntry *gPDirEntry) {
	assert(gPDirEntry->ref > 0);
#ifdef uio_MEM_DEBUG
	if (uio_GPDirEntry_isDir(gPDirEntry)) {
		uio_MemDebug_debugUnref(uio_GPDir, (void *) gPDirEntry);
	} else {
		uio_MemDebug_debugUnref(uio_GPFile, (void *) gPDirEntry);
	}
#endif
	gPDirEntry->ref--;
	if (gPDirEntry->ref == 0)
		uio_GPDirEntry_delete(gPDirEntry);
}

static inline void
uio_GPDir_ref(uio_GPDir *gPDir) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugRef(uio_GPDir, (void *) gPDir);
#endif
	gPDir->ref++;
}

static inline void
uio_GPDir_unref(uio_GPDir *gPDir) {
	assert(gPDir->ref > 0);
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugUnref(uio_GPDir, (void *) gPDir);
#endif
	gPDir->ref--;
	if (gPDir->ref == 0)
		uio_GPDir_delete(gPDir);
}

static inline void
uio_GPFile_ref(uio_GPFile *gPFile) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugRef(uio_GPFile, (void *) gPFile);
#endif
	gPFile->ref++;
}

static inline void
uio_GPFile_unref(uio_GPFile *gPFile) {
	assert(gPFile->ref > 0);
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugUnref(uio_GPFile, (void *) gPFile);
#endif
	gPFile->ref--;
	if (gPFile->ref == 0)
		uio_GPFile_delete(gPFile);
}


#endif  /* LIBS_UIO_GPHYS_H_ */

