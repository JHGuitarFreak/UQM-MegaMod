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

typedef struct stdio_Handle *uio_NativeHandle;
typedef void *uio_GPRootExtra;
typedef struct stdio_GPDirData *uio_GPDirExtra;
typedef void *uio_GPFileExtra;
typedef struct stdio_EntriesIterator stdio_EntriesIterator;
typedef stdio_EntriesIterator *uio_NativeEntriesContext;


#define uio_INTERNAL_PHYSICAL

#include "../gphys.h"
#include "../iointrn.h"
#include "../uioport.h"
#include "../fstypes.h"
#include "../physical.h"

#include <sys/stat.h>
#ifndef WIN32
#	include <dirent.h>
#endif


typedef struct stdio_GPDirData {
	// The reason that names are stored is that in the system filesystem
	// you need names to refer to files and directories.
	// (you could keep a file descriptor to each one, but that would
	// mean a lot of open file descriptors, and for some it won't even
	// be enough).
	// This is not needed for all filesystems; therefor this info is not
	// in uio_GPDir itself.
	// The reasons for including upDir here are similar.
	char *name;
	char *cachedPath;
	uio_GPDir *upDir;
} stdio_GPDirData;

typedef struct stdio_Handle {
	int fd;
} stdio_Handle;

#ifdef WIN32
struct stdio_EntriesIterator {
	long dirHandle;
	struct _finddata_t findData;
	int status;
};
#endif	

#ifndef WIN32
struct stdio_EntriesIterator {
	DIR *dirHandle;
	struct dirent *entry;
	struct dirent *direntBuffer;
	int status;
};
#endif	


uio_PRoot *stdio_mount(uio_Handle *handle, int flags);
int stdio_umount(uio_PRoot *);
uio_PDirHandle *stdio_mkdir(uio_PDirHandle *pDirHandle, const char *name,
		mode_t mode);
uio_Handle *stdio_open(uio_PDirHandle *pDirHandle, const char *file, int flags,
		mode_t mode);
void stdio_close(uio_Handle *handle);
int zip_access(uio_PDirHandle *pDirHandle, const char *name, int mode);
int stdio_access(uio_PDirHandle *pDirHandle, const char *name, int mode);
int stdio_fstat(uio_Handle *handle, struct stat *statBuf);
int stdio_stat(uio_PDirHandle *pDirHandle, const char *name,
		struct stat *statBuf);
ssize_t stdio_read(uio_Handle *handle, void *buf, size_t count);
int stdio_rename(uio_PDirHandle *oldPDirHandle, const char *oldName,
		uio_PDirHandle *newPDirHandle, const char *newName);
int stdio_rmdir(uio_PDirHandle *pDirHandle, const char *name);
off_t stdio_seek(uio_Handle *handle, off_t offset, int whence);
ssize_t stdio_write(uio_Handle *handle, const void *buf, size_t count);
int stdio_unlink(uio_PDirHandle *pDirHandle, const char *name);

stdio_EntriesIterator *stdio_openEntries(uio_PDirHandle *pDirHandle);
int stdio_readEntries(stdio_EntriesIterator **iterator,
		char *buf, size_t len);
void stdio_closeEntries(stdio_EntriesIterator *iterator);
#ifdef WIN32
stdio_EntriesIterator *stdio_EntriesIterator_new(long dirHandle);
#else
stdio_EntriesIterator *stdio_EntriesIterator_new(DIR *dirHandle);
#endif
void stdio_EntriesIterator_delete(stdio_EntriesIterator *iterator);
uio_PDirEntryHandle *stdio_getPDirEntryHandle(
		const uio_PDirHandle *pDirHandle, const char *name);

