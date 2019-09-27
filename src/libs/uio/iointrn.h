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

#ifndef LIBS_UIO_IOINTRN_H_
#define LIBS_UIO_IOINTRN_H_

#define uio_INTERNAL

typedef struct uio_PDirEntryHandle uio_PDirEntryHandle;
typedef struct uio_PDirHandle uio_PDirHandle;
typedef struct uio_PFileHandle uio_PFileHandle;
typedef struct uio_EntriesContext uio_EntriesContext;

#ifndef uio_INTERNAL_PHYSICAL
typedef void *uio_PDirHandleExtra;
typedef void *uio_PFileHandleExtra;
#endif

#include "io.h"
#include "uioport.h"
#include "physical.h"
#include "mount.h"
#include "mounttree.h"
#include "match.h"
#include "mem.h"

struct uio_Handle {
	int ref;
	struct uio_PRoot *root;
	uio_NativeHandle native;
	int openFlags;
			// need to know whether the handle is a RO or RW handle.
};

struct uio_DirHandle {
	int ref;
	struct uio_Repository *repository;
	char *path;
			// does not contain any '.' or '..'; does not start or end
			// with a /
	char *rootEnd;
			// points to the end of the part of path that is considered
			// the root dir. (you can't use '..' to get above the root dir)
};

struct uio_MountHandle {
	struct uio_Repository *repository;
	struct uio_MountInfo *mountInfo;
};

struct uio_DirList {
	const char **names;
	int numNames;
	char *buffer;
};


#define uio_PHandle_COMMON \
	int flags; \
	int ref; \
	uio_PRoot *pRoot;

#define uio_PDirEntryHandle_TYPE_REG 0x0000
#define uio_PDirEntryHandle_TYPE_DIR 0x0001
#define uio_PDirEntryHandle_TYPEMASK 0x0001

struct uio_PDirEntryHandle {
	uio_PHandle_COMMON
};

struct uio_PDirHandle {
	uio_PHandle_COMMON
	uio_PDirHandleExtra extra;
};

struct uio_PFileHandle {
	uio_PHandle_COMMON
	uio_PFileHandleExtra extra;
};

struct uio_EntriesContext {
	uio_PRoot *pRoot;
	uio_NativeEntriesContext native;
};


uio_Handle *uio_Handle_new(uio_PRoot *root, uio_NativeHandle native,
		int openFlags);
void uio_Handle_delete(uio_Handle *handle);
void uio_DirHandle_delete(uio_DirHandle *dirHandle);

uio_PDirEntryHandle *uio_getPDirEntryHandle(
		const uio_PDirHandle *dirHandle, const char *name);
void uio_PDirHandle_deletePDirHandleExtra(uio_PDirHandle *pDirHandle);
void uio_PFileHandle_deletePFileHandleExtra(uio_PFileHandle *pFileHandle);

uio_PDirHandle *uio_PDirHandle_new(uio_PRoot *pRoot,
		uio_PDirHandleExtra extra);
uio_PFileHandle *uio_PFileHandle_new(uio_PRoot *pRoot,
		uio_PFileHandleExtra extra);
void uio_PDirEntryHandle_delete(uio_PDirEntryHandle *pDirEntryHandle);
void uio_PDirHandle_delete(uio_PDirHandle *pDirHandle);
void uio_PFileHandle_delete(uio_PFileHandle *pFileHandle);


static inline void
uio_Handle_ref(uio_Handle *handle) {
	handle->ref++;
}

static inline void
uio_Handle_unref(uio_Handle *handle) {
	assert(handle->ref > 0);
	handle->ref--;
	if (handle->ref == 0)
		uio_Handle_delete(handle);
}

static inline void
uio_DirHandle_ref(uio_DirHandle *dirHandle) {
	dirHandle->ref++;
}

static inline void
uio_DirHandle_unref(uio_DirHandle *dirHandle) {
	assert(dirHandle->ref > 0);
	dirHandle->ref--;
	if (dirHandle->ref == 0)
		uio_DirHandle_delete(dirHandle);
}

static inline uio_bool
uio_PDirEntryHandle_isDir(const uio_PDirEntryHandle *handle) {
	return (handle->flags & uio_PDirEntryHandle_TYPEMASK) ==
		uio_PDirEntryHandle_TYPE_DIR;
}

static inline void
uio_PDirEntryHandle_ref(uio_PDirEntryHandle *handle) {
	handle->ref++;
}

static inline void
uio_PDirEntryHandle_unref(uio_PDirEntryHandle *handle) {
	assert(handle->ref > 0);
	handle->ref--;
	if (handle->ref == 0)
		uio_PDirEntryHandle_delete(handle);
}

static inline void
uio_PDirHandle_ref(uio_PDirHandle *handle) {
	handle->ref++;
}

static inline void
uio_PDirHandle_unref(uio_PDirHandle *handle) {
	assert(handle->ref > 0);
	handle->ref--;
	if (handle->ref == 0)
		uio_PDirHandle_delete(handle);
}

static inline void
uio_PFileHandle_ref(uio_PFileHandle *handle) {
	handle->ref++;
}

static inline void
uio_PFileHandle_unref(uio_PFileHandle *handle) {
	assert(handle->ref > 0);
	handle->ref--;
	if (handle->ref == 0)
		uio_PFileHandle_delete(handle);
}


#endif  /* LIBS_UIO_IOINTRN_H_ */


