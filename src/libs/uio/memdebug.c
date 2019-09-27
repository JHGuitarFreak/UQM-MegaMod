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
#include <stdio.h>

#include "memdebug.h"
#include "hashtable.h"
#include "mem.h"
#include "uioutils.h"
#include "types.h"
#include "uioport.h"

// I'm intentionally not including the .h files. It would only make things
// messy, because the second arguments would be other pointers, which
// would require typecasts of the functions in uio_MemDebug_logTypeInfo.
extern void uio_DirHandle_print(FILE *out, const void *);
extern void uio_printMountInfo(FILE *out, const void *);
extern void uio_printMountTreeItem(FILE *out, const void *);
extern void uio_printPathComp(FILE *out, const void *);

// Must be in the same order as in uio_MemDebug_LogTypes
// Change the third field to have debug info printed for specific actions.
// See memdebug.h file the possible values.
const uio_MemDebug_LogTypeInfo uio_MemDebug_logTypeInfo[] = {
	{ "uio_DirHandle",      uio_DirHandle_print,    0 },
	{ "uio_FileSystemInfo", NULL,                   0 },
	{ "uio_GPDir",          NULL,                   0 },
	{ "uio_GPFile",         NULL,                   0 },
	{ "uio_GPRoot",         NULL,                   0 },
	{ "uio_Handle",         NULL,                   0 },
	{ "uio_MountHandle",    NULL,                   0 },
	{ "uio_MountInfo",      uio_printMountInfo,     0 },
	{ "uio_MountTree",      NULL,                   0 },
	{ "uio_MountTreeItem",  uio_printMountTreeItem, 0 },
	{ "uio_PathComp",       uio_printPathComp,      0 },
	{ "uio_PFileHandle",    NULL,                   0 },
	{ "uio_PDirHandle",     NULL,                   0 },
	{ "uio_PRoot",          NULL,                   0 },
	{ "uio_Repository",     NULL,                   0 },
	{ "uio_Stream",         NULL,                   0 },
	{ "stdio_GPDirData",    NULL,                   0 },
#ifdef HAVE_ZIP
	{ "zip_GPFileData",     NULL,                   0 },
	{ "zip_GPDirData",      NULL,                   0 },
#endif
};

HashTable_HashTable **uio_MemDebug_logs;


static uio_uint32 uio_MemDebug_pointerHash(const void *ptr);
static uio_bool uio_MemDebug_pointerCompare(const void *ptr1, const void *ptr2);
static void *uio_MemDebug_pointerCopy(const void *ptr);
static void uio_MemDebug_pointerFree(void *ptr);

static inline uio_MemDebug_PointerInfo *uio_MemDebug_PointerInfo_new(int ref);
static inline void uio_MemDebug_PointerInfo_delete(
		uio_MemDebug_PointerInfo *pointerInfo);
static inline uio_MemDebug_PointerInfo *uio_MemDebug_PointerInfo_alloc(void);
static inline void uio_MemDebug_PointerInfo_free(
		uio_MemDebug_PointerInfo *pointerInfo);

void
uio_MemDebug_init(void) {
	int i;
	
	assert((int) uio_MemDebug_numLogTypes ==
			(sizeof uio_MemDebug_logTypeInfo /
			sizeof uio_MemDebug_logTypeInfo[0]));
	uio_MemDebug_logs = uio_malloc(uio_MemDebug_numLogTypes *
			sizeof (HashTable_HashTable *));

	for (i = 0; i < uio_MemDebug_numLogTypes; i++) {
		uio_MemDebug_logs[i] = HashTable_newHashTable(
				uio_MemDebug_pointerHash,
				uio_MemDebug_pointerCompare,
				uio_MemDebug_pointerCopy,
				uio_MemDebug_pointerFree,
				4, 0.85, 0.90);
	}
}

void
uio_MemDebug_unInit(void) {
	int i;

	for (i = 0; i < uio_MemDebug_numLogTypes; i++)
		HashTable_deleteHashTable(uio_MemDebug_logs[i]);

	uio_free(uio_MemDebug_logs);
}

static uio_uint32
uio_MemDebug_pointerHash(const void *ptr) {
	uio_uintptr ptrInt;

	ptrInt = (uio_uintptr) ptr;
	return (uio_uint32) (ptrInt ^ (ptrInt >> 10) ^ (ptrInt >> 20));
}

static uio_bool
uio_MemDebug_pointerCompare(const void *ptr1, const void *ptr2) {
	return ptr1 == ptr2;
}

static void *
uio_MemDebug_pointerCopy(const void *ptr) {
	return unconst(ptr);
}

static void
uio_MemDebug_pointerFree(void *ptr) {
	(void) ptr;
}

void
uio_MemDebug_logAllocation(uio_MemDebug_LogType type, void *ptr) {
	uio_MemDebug_PointerInfo *pointerInfo;

	if (ptr == NULL) {
		fprintf(stderr, "Fatal: Allocated pointer is (%s *) NULL.\n",
				uio_MemDebug_logTypeInfo[(int) type].name);
		abort();
	}
	if (uio_MemDebug_logTypeInfo[(int) type].flags & uio_MemDebug_PRINT_ALLOC) {
		fprintf(stderr, "Alloc ");
		uio_MemDebug_printPointer(stderr, type, ptr);
		fprintf(stderr, "\n");
	}
	pointerInfo = uio_MemDebug_PointerInfo_new(1);
	HashTable_add(uio_MemDebug_logs[type], ptr, (void *) pointerInfo);
}

void
uio_MemDebug_logDeallocation(uio_MemDebug_LogType type, void *ptr) {
	uio_MemDebug_PointerInfo *pointerInfo;

	if (ptr == NULL) {
		fprintf(stderr, "Fatal: Attempt to free (%s *) NULL pointer.\n",
				uio_MemDebug_logTypeInfo[(int) type].name);
		abort();
	}
	if (uio_MemDebug_logTypeInfo[(int) type].flags & uio_MemDebug_PRINT_FREE) {
		fprintf(stderr, "Free ");
		uio_MemDebug_printPointer(stderr, type, ptr);
		fprintf(stderr, "\n");
	}
	pointerInfo = HashTable_find(uio_MemDebug_logs[type], ptr);
	if (pointerInfo == NULL) {
		fprintf(stderr, "Fatal: Attempt to free unallocated pointer "
				"(%s *) %p.\n",
				uio_MemDebug_logTypeInfo[(int) type].name, ptr);
		abort();
	}
#if 0
	if (pointerInfo->ref != 0) {
		fprintf(stderr, "Fatal: Attempt to free pointer with references "
				"left (%s *) %p.\n",
				uio_MemDebug_logTypeInfo[(int) type].name, ptr);
		abort();
	}
#endif
	uio_MemDebug_PointerInfo_free(pointerInfo);
	HashTable_remove(uio_MemDebug_logs[type], ptr);
}

void
uio_MemDebug_logRef(uio_MemDebug_LogType type, void *ptr) {
	uio_MemDebug_PointerInfo *pointerInfo;

	if (ptr == NULL) {
		fprintf(stderr, "Fatal: Attempt to increment reference to a "
				"(%s *) NULL pointer.\n",
				uio_MemDebug_logTypeInfo[(int) type].name);
		abort();
	}
	pointerInfo = HashTable_find(uio_MemDebug_logs[type], ptr);
	if (pointerInfo == NULL) {
		fprintf(stderr, "Fatal: Attempt to increment reference to "
				"unallocated pointer (%s *) %p.\n",
				uio_MemDebug_logTypeInfo[(int) type].name, ptr);
		abort();
	}
	pointerInfo->pointerRef++;
	if (uio_MemDebug_logTypeInfo[(int) type].flags & uio_MemDebug_PRINT_REF) {
		fprintf(stderr, "Ref++ to %d, ", pointerInfo->pointerRef);
		uio_MemDebug_printPointer(stderr, type, ptr);
		fprintf(stderr, "\n");
	}
}

void
uio_MemDebug_logUnref(uio_MemDebug_LogType type, void *ptr) {
	uio_MemDebug_PointerInfo *pointerInfo;

	if (ptr == NULL) {
		fprintf(stderr, "Fatal: Attempt to decrement reference to a "
				"(%s *) NULL pointer.\n",
				uio_MemDebug_logTypeInfo[(int) type].name);
		abort();
	}
	pointerInfo = HashTable_find(uio_MemDebug_logs[type], ptr);
	if (pointerInfo == NULL) {
		fprintf(stderr, "Fatal: Attempt to decrement reference to "
				"unallocated pointer (%s *) %p.\n",
				uio_MemDebug_logTypeInfo[(int) type].name, ptr);
		abort();
	}
	if (pointerInfo->pointerRef == 0) {
		fprintf(stderr, "Fatal: Attempt to decrement reference below 0 for "
				"pointer (%s *) %p.\n",
				uio_MemDebug_logTypeInfo[(int) type].name, ptr);
		abort();
	}
	pointerInfo->pointerRef--;
	if (uio_MemDebug_logTypeInfo[(int) type].flags & uio_MemDebug_PRINT_UNREF) {
		fprintf(stderr, "Ref-- to %d, ", pointerInfo->pointerRef);
		uio_MemDebug_printPointer(stderr, type, ptr);
		fprintf(stderr, "\n");
	}
}

void
uio_MemDebug_printPointer(FILE *out, uio_MemDebug_LogType type, void *ptr) {
	fprintf(out, "(%s *) %p", uio_MemDebug_logTypeInfo[(int) type].name, ptr);
	if (uio_MemDebug_logTypeInfo[(int) type].printFunction != NULL) {
		fprintf(out, ": ");
		uio_MemDebug_logTypeInfo[(int) type].printFunction(out, ptr);
	}
}

void
uio_MemDebug_printPointersType(FILE *out, uio_MemDebug_LogType type) {
	HashTable_Iterator *iterator;

	for (iterator = HashTable_getIterator(uio_MemDebug_logs[type]);
			!HashTable_iteratorDone(iterator);
			iterator = HashTable_iteratorNext(iterator)) {
		uio_MemDebug_printPointer(out, type, HashTable_iteratorKey(iterator));
		fprintf(out, "\n");
	}
	HashTable_freeIterator(iterator);
}

void
uio_MemDebug_printPointers(FILE *out) {
	int i;

	for (i = 0; i < uio_MemDebug_numLogTypes; i++)
		uio_MemDebug_printPointersType(out, i);
}

static inline uio_MemDebug_PointerInfo *
uio_MemDebug_PointerInfo_new(int ref) {
	uio_MemDebug_PointerInfo *result;
	result = uio_MemDebug_PointerInfo_alloc();
	result->pointerRef = ref;
	return result;
}

static inline void
uio_MemDebug_PointerInfo_delete(uio_MemDebug_PointerInfo *pointerInfo) {
	uio_MemDebug_PointerInfo_free(pointerInfo);
}

static inline uio_MemDebug_PointerInfo *
uio_MemDebug_PointerInfo_alloc(void) {
	return uio_malloc(sizeof (uio_MemDebug_PointerInfo));
}

static inline void
uio_MemDebug_PointerInfo_free(uio_MemDebug_PointerInfo *pointerInfo) {
	uio_free(pointerInfo);
}

