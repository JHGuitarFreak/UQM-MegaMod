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

#ifndef LIBS_UIO_FILEBLOCK_H_
#define LIBS_UIO_FILEBLOCK_H_

typedef struct uio_FileBlock uio_FileBlock;

#include "io.h"
#include "uioport.h"

#include <sys/types.h>

#define uio_FB_USAGE_FORWARD  1
#define uio_FB_USAGE_BACKWARD 2
#define uio_FB_USAGE_MASK  (uio_FB_USAGE_FORWARD | uio_FB_USAGE_BACKWARD)

#ifdef uio_INTERNAL_FILEBLOCK

// A fileblock represents a contiguous block of data from a file.
// It's purpose is to avoid needless copying of data, while enabling
// buffering.

struct uio_FileBlock {
	uio_Handle *handle;
	int flags;
			// See above for uio_FB_USAGE_FORWARD, uio_FB_USAGE_BACKWARD.
#define uio_FB_USE_MMAP       4
	off_t offset;
			// Offset to the start of the block in the file.
	size_t blockSize;
			// Size of the block of data represented by this FileBlock.
	char *buffer;
			// either allocated buffer, or buffer to mmap'ed area.
	size_t bufSize;
			// Size of the buffer.
	off_t bufOffset;
			// Offset of the start of the buffer into the block.
	size_t bufFill;
			// Part of 'buffer' which is in use.
	size_t readAheadBufSize;
			// Try to read up to this many bytes at a time, even when less
			// is immediately needed.
};
// INV: The FileBlock represents 'fileData[offset..(offset + blockSize - 1)]'
// where 'fileData' is the contents of the file.
// INV: If buf != NULL then:
//     bufFill <= bufSize
//     bufFill <= blockSize
//     buffer[0..bufFill - 1] == fileData[
//             (offset + bufOffset)..(offset + bufOffset + bufFill - 1)]


#endif  /* uio_INTERNAL_FILEBLOCK */

uio_FileBlock *uio_openFileBlock(uio_Handle *handle);
uio_FileBlock *uio_openFileBlock2(uio_Handle *handle, off_t offset,
		size_t size);
ssize_t uio_accessFileBlock(uio_FileBlock *block, off_t offset, size_t length,
		char **buffer);
int uio_copyFileBlock(uio_FileBlock *block, off_t offset, char *buffer,
		size_t length);
int uio_closeFileBlock(uio_FileBlock *block);
#define uio_FB_READAHEAD_BUFSIZE_MAX ((size_t) -1)
void uio_setFileBlockUsageHint(uio_FileBlock *block, int usage,
		size_t readAheadBufSize);
void uio_clearFileBlockBuffers(uio_FileBlock *block);

#endif  /* LIBS_UIO_FILEBLOCK_H_ */


