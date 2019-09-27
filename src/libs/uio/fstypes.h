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

#ifndef LIBS_UIO_FSTYPES_H_
#define LIBS_UIO_FSTYPES_H_

typedef int uio_FileSystemID;
#define uio_FSTYPE_STDIO 1
#define uio_FSTYPE_ZIP   2


#ifdef uio_INTERNAL

#define uio_FS_FIRST_CUSTOM_ID 0x10

#ifndef uio_INTERNAL_PHYSICAL
typedef void *uio_NativeHandle;
#endif

// 'forward' declarations
typedef struct uio_FileSystemHandler uio_FileSystemHandler;
typedef struct uio_FileSystemInfo uio_FileSystemInfo;

#include <sys/stat.h>
#include <sys/types.h>
#include "physical.h"
#include "uioport.h"


/* Structure describing how to access files _in_ a directory of a certain
 * type (not files _of_ a certain type.) Except for mount().
 * in open(), the first arg points to the dir where the file should be
 * in, and the second arg is the name of that file (no path)
 */
struct uio_FileSystemHandler {
	int               (*init)     (void);
	int               (*unInit)   (void);
	void              (*cleanup)  (uio_PRoot *, int);
			// Called to cleanup memory. The second argument specifies
			// how thoroughly.

	struct uio_PRoot * (*mount)    (uio_Handle *, int);
	int               (*umount)   (uio_PRoot *);

	int               (*access)   (uio_PDirHandle *, const char *, int mode);
	void              (*close)    (uio_Handle *);
			// called when the last reference is closed, not
			// necessarilly each time when uio_close() is called
	int               (*fstat)    (uio_Handle *, struct stat *);
	int               (*stat)     (uio_PDirHandle *, const char *,
			struct stat *);
	uio_PDirHandle *  (*mkdir)    (uio_PDirHandle *, const char *, mode_t);
	uio_Handle *      (*open)     (uio_PDirHandle *, const char *, int,
			mode_t);
	ssize_t           (*read)     (uio_Handle *, void *, size_t);
	int               (*rename)   (uio_PDirHandle *, const char *,
			uio_PDirHandle *, const char *);
	int               (*rmdir)    (uio_PDirHandle *, const char *);
	off_t             (*seek)     (uio_Handle *, off_t, int);
	ssize_t           (*write)    (uio_Handle *, const void *, size_t);
	int               (*unlink)   (uio_PDirHandle *, const char *);

	uio_NativeEntriesContext (*openEntries) (uio_PDirHandle *);
	int               (*readEntries) (uio_NativeEntriesContext *, char *,
			size_t);
	void              (*closeEntries) (uio_NativeEntriesContext);

	uio_PDirEntryHandle * (*getPDirEntryHandle) (
			const uio_PDirHandle *, const char *);
	void              (*deletePRootExtra) (uio_PRootExtra pRootExtra);
	void              (*deletePDirHandleExtra) (
			uio_PDirHandleExtra pDirHandleExtra);
	void              (*deletePFileHandleExtra) (
			uio_PFileHandleExtra pFileHandleExtra);
};

struct uio_FileSystemInfo {
	int ref;
	uio_FileSystemID id;
	char *name;  // name of the file system
	uio_FileSystemHandler *handler;
	struct uio_FileSystemInfo *next;
};

void uio_registerDefaultFileSystems(void);
void uio_unRegisterDefaultFileSystems(void);
uio_FileSystemID uio_registerFileSystem(uio_FileSystemID wantedID,
		 const char *name, uio_FileSystemHandler *handler);
int uio_unRegisterFileSystem(uio_FileSystemID id);
uio_FileSystemHandler *uio_getFileSystemHandler(uio_FileSystemID id);
uio_FileSystemInfo *uio_getFileSystemInfo(uio_FileSystemID id);

#endif  /* uio_INTERNAL */

#endif  /* LIBS_UIO_FSTYPES_H_ */

