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

#define uio_INTERNAL_FILEBLOCK

#include "iointrn.h"
#include "fileblock.h"
#include "uioport.h"

#include <errno.h>

static uio_FileBlock *uio_FileBlock_new(uio_Handle *handle, int flags,
		off_t offset, size_t blockSize, char *buffer, size_t bufSize,
		off_t bufOffset, size_t bufFill, size_t readAheadBufSize);
static inline uio_FileBlock *uio_FileBlock_alloc(void);
static void uio_FileBlock_free(uio_FileBlock *block);

// caller should uio_Handle_ref(handle) (unless it doesn't need it's own
// reference anymore).
static uio_FileBlock *
uio_FileBlock_new(uio_Handle *handle, int flags, off_t offset,
		size_t blockSize, char *buffer, size_t bufSize, off_t bufOffset,
		size_t bufFill, size_t readAheadBufSize) {
	uio_FileBlock *result;
	
	result = uio_FileBlock_alloc();
	result->handle = handle;
	result->flags = flags;
	result->offset = offset;
	result->blockSize = blockSize;
	result->buffer = buffer;
	result->bufSize = bufSize;
	result->bufOffset = bufOffset;
	result->bufFill = bufFill;
	result->readAheadBufSize = readAheadBufSize;
	return result;
}

static inline uio_FileBlock *
uio_FileBlock_alloc(void) {
	return uio_malloc(sizeof (uio_FileBlock));
}

static inline void
uio_FileBlock_free(uio_FileBlock *block) {
	uio_free(block);
}

uio_FileBlock *
uio_openFileBlock(uio_Handle *handle) {
	// TODO: if mmap support is available, and it is available natively
	//       for the filesystem (make some sort of sysctl for filesystems
	//       to check this?), use mmap.
	//       mmap the entire file if it's small enough.
	//       N.B. Keep in mind streams of which the size is not known in
	//       advance.
	struct stat statBuf;
	if (uio_fstat(handle, &statBuf) == -1) {
		// errno is set
		return NULL;
	}
	uio_Handle_ref(handle);
	return uio_FileBlock_new(handle, 0, 0, statBuf.st_size, NULL, 0, 0, 0, 0);
}

uio_FileBlock *
uio_openFileBlock2(uio_Handle *handle, off_t offset, size_t size) {
	// TODO: mmap (see uio_openFileBlock)

	// TODO: check if offset and size are acceptable.
	//       Need to handle streams of which the size is unknown.
#if 0
	if (uio_stat(handle, &statBuf) == -1) {
		// errno is set
		return NULL;
	}
	if (statBuf.st_size > offset || (statBuf.st_size - offset > size)) {
		// NOT: 'if (statBuf.st_size > offset + size)', to protect
		// against overflow.

	}
#endif
	uio_Handle_ref(handle);
	return uio_FileBlock_new(handle, 0, offset, size, NULL, 0, 0, 0, 0);
}

static inline ssize_t
uio_accessFileBlockMmap(uio_FileBlock *block, off_t offset, size_t length,
		char **buffer) {
	// TODO
	errno = ENOSYS;
	(void) block;
	(void) offset;
	(void) length;
	(void) buffer;
	return -1;
}

static inline ssize_t
uio_accessFileBlockNoMmap(uio_FileBlock *block, off_t offset, size_t length,
		char **buffer) {
	ssize_t numRead;
	off_t start;
	off_t end;
	size_t bufSize;
	char *oldBuffer;
	//size_t oldBufSize;

	// Don't go beyond the end of the block.
	if (offset > (off_t) block->blockSize) {
		*buffer = block->buffer;
		return 0;
	}
	if (length > block->blockSize - offset)
		length = block->blockSize - offset;

	if (block->buffer != NULL) {
		// Check whether the requested data is already in the buffer.
		if (offset >= block->bufOffset &&
				(offset - block->bufOffset) + length < block->bufFill) {
			*buffer = block->buffer + (offset - block->bufOffset);
			return length;
		}
	}

	if (length < block->readAheadBufSize &&
			(block->flags & uio_FB_USAGE_MASK) != 0) {
		// We can buffer more data.
		switch (block->flags & uio_FB_USAGE_MASK) {
			case uio_FB_USAGE_FORWARD:
				// Read extra data after the requested data.
				start = offset;
				end = (block->readAheadBufSize > block->blockSize - offset) ?
						block->blockSize : offset + block->readAheadBufSize;
				break;
			case uio_FB_USAGE_BACKWARD:
				// Read extra data before the requested data.
				end = offset + length;
				start = (end <= (off_t) block->blockSize) ?
						0 : end - block->bufSize;
				break;
			case uio_FB_USAGE_FORWARD | uio_FB_USAGE_BACKWARD: {
				// Read extra data both before and after the requested data.
				off_t extraBefore = (block->readAheadBufSize - length) / 2;
				start = (offset < extraBefore) ? 0 : offset - extraBefore;

				end = (block->readAheadBufSize > block->blockSize - start) ?
						block->blockSize : start + block->readAheadBufSize;
				break;
			}
		}
	} else {
		start = offset;
		end = offset + length;
	}
	bufSize = (length > block->readAheadBufSize) ?
			length : block->readAheadBufSize;

	// Start contains the start index in the block of the data we're going
	// to read.
	// End contains the end index.
	// bufSize contains the size of the buffer. bufSize >= end - start.

	oldBuffer = block->buffer;
	//oldBufSize = block->bufSize;
	if (block->buffer != NULL || block->bufSize != bufSize) {
		// We don't have a buffer, or we have one, but of the wrong size.
		block->buffer = uio_malloc(bufSize);
		block->bufSize = bufSize;
	}

	if (oldBuffer != NULL) {
		// TODO: If we have part of the data still in the old buffer, we
		// can keep that.
		// memmove(...)

		if (oldBuffer != block->buffer)
			uio_free(oldBuffer);
	}
	block->bufFill = 0;
	block->bufOffset = start;

	// TODO: lock handle
	if (uio_lseek(block->handle, block->offset + start, SEEK_SET) ==
			(off_t) -1) {
		// errno is set
		return -1;
	}
	
	numRead = uio_read(block->handle, block->buffer, end - start);
	if (numRead == -1) {
		// errno is set
		// TODO: unlock handle
		return -1;
	}
	// TODO: unlock handle

	block->bufFill = numRead;
	*buffer = block->buffer + (offset - block->bufOffset);
	if (numRead <= (offset - block->bufOffset))
		return 0;
	if ((size_t) numRead >= length)
		return length;
	return numRead - offset;
}

// block remains usable until the next call to uio_accessFileBlock
// with the same block as argument, or until uio_closeFileBlock with
// that block as argument.
// The 'offset' parameter is wrt. the start of the block.
// Requesting access to data beyond the file block is not an error. The
// FileBlock is meant to be used as a replacement of seek() and read(), and
// as with those functions, trying to go beyond the end of a file just
// goes to the end. The return value is the number of bytes in the buffer.
ssize_t
uio_accessFileBlock(uio_FileBlock *block, off_t offset, size_t length,
		char **buffer) {
	if (block->flags & uio_FB_USE_MMAP) {
		return uio_accessFileBlockMmap(block, offset, length, buffer);
	} else {
		return uio_accessFileBlockNoMmap(block, offset, length, buffer);
	}
}

int
uio_copyFileBlock(uio_FileBlock *block, off_t offset, char *buffer,
		size_t length) {
	if (block->flags & uio_FB_USE_MMAP) {
		// TODO
		errno = ENOSYS;
		return -1;
	} else {
		ssize_t numCopied = 0;
		ssize_t readResult;

		// Don't go beyond the end of the block.
		if (offset > (off_t) block->blockSize)
			return 0;
		if (length > block->blockSize - offset)
			length = block->blockSize - offset;
		
		// Check whether (part of) the requested data is already in our
		// own buffer.
		if (block->buffer != NULL && offset >= block->bufOffset
				&& offset < block->bufOffset + (off_t) block->bufFill) {
			size_t toCopy = block->bufFill - offset;
			if (toCopy > length)
				toCopy = length;
			memcpy(buffer, block->buffer + (offset - block->bufOffset),
					toCopy);
			numCopied += toCopy;
			length -= toCopy;
			if (length == 0)
				return numCopied;
			buffer += toCopy;
			offset += toCopy;
		}

		// TODO: lock handle
		if (uio_lseek(block->handle, block->offset + offset, SEEK_SET) ==
				(off_t) -1) {
			// errno is set
			return -1;
		}
		
		readResult = uio_read(block->handle, buffer, length);
		// TODO: unlock handle
		if (readResult == -1) {
			// errno is set
			return -1;
		}
		numCopied += readResult;

		return numCopied;
	}
}

int
uio_closeFileBlock(uio_FileBlock *block) {
	if (block->flags & uio_FB_USE_MMAP) {
#if 0
		if (block->buffer != NULL)
			uio_mmunmap(block->buffer);
#endif
	} else {
		if (block->buffer != NULL)
			uio_free(block->buffer);
	}
	uio_Handle_unref(block->handle);
	uio_FileBlock_free(block);
	return 0;
}

// Usage is the or'ed value of zero or more of uio_FB_USAGE_FORWARD,
// and uio_FB_USAGE_BACKWARD.
void
uio_setFileBlockUsageHint(uio_FileBlock *block, int usage,
		size_t readAheadBufSize) {
	block->flags = (block->flags & ~uio_FB_USAGE_MASK) |
			(usage & uio_FB_USAGE_MASK);
	block->readAheadBufSize = readAheadBufSize;
}

// Call if you want the memory used by the fileblock to be released, but
// still want to use the fileblock later. If you don't need the fileblock,
// call uio_closeFileBlock() instead.
void
uio_clearFileBlockBuffers(uio_FileBlock *block) {
	if (block->buffer != NULL) {
		uio_free(block->buffer);
		block->buffer = NULL;
	}
}


