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

typedef struct zip_Handle *uio_NativeHandle;
typedef void *uio_GPRootExtra;
typedef struct zip_GPFileData *uio_GPFileExtra;
typedef struct zip_GPFileData *uio_GPDirExtra;
typedef struct uio_GPDirEntries_Iterator *uio_NativeEntriesContext;

#define uio_INTERNAL_PHYSICAL

#include "../gphys.h"
#include "../iointrn.h"
#include "../uioport.h"
#include "../physical.h"
#include "../types.h"
#include "../fileblock.h"

#include <zlib.h>
#include <sys/types.h>
#include <sys/stat.h>

// zip_USE_HEADERS determinines what header for files within a .zip file
// is used when building the directory structure.
// Set to 'zip_USE_CENTRAL_HEADERS' to use the central directory header,
// set to 'zip_USE_LOCAL_HEADERS' to use the local file header.
// Central is highly adviced: it uses much less seeking, and hence is much
// faster.
#define zip_USE_HEADERS zip_USE_CENTRAL_HEADERS
#define zip_USE_CENTRAL_HEADERS 1
#define zip_USE_LOCAL_HEADERS 2

#define zip_INCOMPLETE_STAT
		// Ignored unless zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS.
		// If defined, extra meta-data for files in the .zip archive
		// isn't retrieved from the local file header when zip_stat()
		// is called. The uid, gid, file mode, and file times may be
		// inaccurate. The advantage is that a possibly costly seek and
		// read can be avoided.

typedef struct zip_GPFileData {
	off_t compressedSize;
	off_t uncompressedSize;
	uio_uint16 compressionFlags;
	uio_uint16 compressionMethod;
#if zip_USE_HEADERS == zip_USE_CENTRAL_HEADERS
	off_t headerOffset;  // start of the local header for this file
#endif
	off_t fileOffset;  // start of the compressed data in the .zip file
	uid_t uid;
	gid_t gid;
	mode_t mode;
	time_t atime;  // access time
	time_t mtime;  // modification time
	time_t ctime;  // change time
} zip_GPFileData;

typedef zip_GPFileData zip_GPDirData;
// TODO: some of the fields from zip_GPFileData are not needed for
// directories. A few bytes could be saved here by making a seperate
// structure.

typedef struct zip_Handle {
	uio_GPFile *file;
	z_stream zipStream;
	uio_FileBlock *fileBlock;
	off_t uncompressedOffset;
			// seek location in the uncompressed stream
	off_t compressedOffset;
			// seek location in the compressed stream, from the start
			// of the compressed file
} zip_Handle;


uio_PRoot *zip_mount(uio_Handle *handle, int flags);
int zip_umount(struct uio_PRoot *);
uio_Handle *zip_open(uio_PDirHandle *pDirHandle, const char *file, int flags,
		mode_t mode);
void zip_close(uio_Handle *handle);
int zip_access(uio_PDirHandle *pDirHandle, const char *name, int mode);
int zip_fstat(uio_Handle *handle, struct stat *statBuf);
int zip_stat(uio_PDirHandle *pDirHandle, const char *name,
		struct stat *statBuf);
ssize_t zip_read(uio_Handle *handle, void *buf, size_t count);
off_t zip_seek(uio_Handle *handle, off_t offset, int whence);




