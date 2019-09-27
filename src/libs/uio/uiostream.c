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

#include "uioport.h"
#include "iointrn.h"
#include "uiostream.h"

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>

#include "uioutils.h"
#include "utils.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

#define uio_Stream_BLOCK_SIZE 1024

static inline uio_Stream *uio_Stream_new(uio_Handle *handle, int openFlags);
static inline void uio_Stream_delete(uio_Stream *stream);
static inline uio_Stream *uio_Stream_alloc(void);
static inline void uio_Stream_free(uio_Stream *stream);
#ifdef NDEBUG
#	define uio_assertReadSanity(stream)
#	define uio_assertWriteSanity(stream)
#else
static void uio_assertReadSanity(uio_Stream *stream);
static void uio_assertWriteSanity(uio_Stream *stream);
#endif
static int uio_Stream_fillReadBuffer(uio_Stream *stream);
static int uio_Stream_flushWriteBuffer(uio_Stream *stream);
static void uio_Stream_discardReadBuffer(uio_Stream *stream);


uio_Stream *
uio_fopen(uio_DirHandle *dir, const char *path, const char *mode) {
	int openFlags;
	uio_Handle *handle;
	uio_Stream *stream;
	int i;
	
	switch (*mode) {
		case 'r':
			openFlags = O_RDONLY;
			break;
		case 'w':
			openFlags = O_WRONLY | O_CREAT | O_TRUNC;
			break;
		case 'a':
			openFlags = O_WRONLY| O_CREAT | O_APPEND;
		default:
			errno = EINVAL;
			fprintf(stderr, "Invalid mode string in call to uio_fopen().\n");
			return NULL;
	}
	mode++;

	// C'89 says 'b' may either be the second or the third character.
	// If someone specifies both 'b' and 't', he/she is out of luck.
	i = 2;
	while (i-- && (*mode != '\0')) {
		switch (*mode) {
			case 'b':
#ifdef WIN32
				openFlags |= O_BINARY;
#endif
				break;
			case 't':
#ifdef WIN32
				openFlags |= O_TEXT;
#endif
				break;
			case '+':
				openFlags = (openFlags & ~O_ACCMODE) | O_RDWR;
				break;
			default:
				i = 0;
					// leave the while loop
				break;
		}
		mode++;
	}

	// Any characters in the mode string that might follow are ignored.
	
	handle = uio_open(dir, path, openFlags, S_IRUSR | S_IWUSR |
			S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (handle == NULL) {
		// errno is set
		return NULL;
	}

	stream = uio_Stream_new(handle, openFlags);
	return stream;
}

int
uio_fclose(uio_Stream *stream) {
	if (stream->operation == uio_StreamOperation_write)
		uio_Stream_flushWriteBuffer(stream);
	uio_close(stream->handle);
	uio_Stream_delete(stream);
	return 0;
}

// "The file position indicator for the stream (if defined) is advanced by
// the number of characters successfully read. If an error occurs, the
// resulting value of the file position indicator for the stream is
// indeterminate. If a partial element is read, its value is
// indeterminate." (from POSIX for fread()).
size_t
uio_fread(void *buf, size_t size, size_t nmemb, uio_Stream *stream) {
	size_t bytesToRead;
	size_t bytesRead;

	bytesToRead = size * nmemb;
	bytesRead = 0;

	uio_assertReadSanity(stream);
	stream->operation = uio_StreamOperation_read;

	if (stream->dataEnd > stream->dataStart) {
		// First use what's in the buffer.
		size_t numRead;

		numRead = minu(stream->dataEnd - stream->dataStart, bytesToRead);
		memcpy(buf, stream->dataStart, numRead);
		buf = (void *) ((char *) buf + numRead);
		stream->dataStart += numRead;
		bytesToRead -= numRead;
		bytesRead += numRead;
	}
	if (bytesToRead == 0) {
		// Done already
		return nmemb;
	}

	{
		// Read the rest directly into the caller's buffer.
		ssize_t numRead;
		numRead = uio_read(stream->handle, buf, bytesToRead);
		if (numRead == -1) {
			stream->status = uio_Stream_STATUS_ERROR;
			goto out;
		}
		bytesRead += numRead;
		if ((size_t) numRead < bytesToRead) {
			// End of file
			stream->status = uio_Stream_STATUS_EOF;
			stream->operation = uio_StreamOperation_none;
			goto out;
		}
	}
	
out:
	if (bytesToRead == 0)
		return nmemb;
	return bytesRead / size;
}

char *
uio_fgets(char *s, int size, uio_Stream *stream) {
	int orgSize;
	char *buf;

	uio_assertReadSanity(stream);
	stream->operation = uio_StreamOperation_read;
	
	size--;
	orgSize = size;
	buf = s;
	while (size > 0) {
		size_t maxRead;
		const char *newLinePos;

		// Fill buffer if empty.
		if (stream->dataStart == stream->dataEnd) {
			if (uio_Stream_fillReadBuffer(stream) == -1) {
				// errno is set
				stream->status = uio_Stream_STATUS_ERROR;
				return NULL;
			}
			if (stream->dataStart == stream->dataEnd) {
				// End-of-file
				stream->status = uio_Stream_STATUS_EOF;
				stream->operation = uio_StreamOperation_none;
				if (size == orgSize) {
					// Nothing was read.
					return NULL;
				}
				break;
			}
		}
		
		// Search in buffer
		maxRead = minu(stream->dataEnd - stream->dataStart, size);
		newLinePos = memchr(stream->dataStart, '\n', maxRead);
		if (newLinePos != NULL) {
			// Newline found.
			maxRead = newLinePos + 1 - stream->dataStart;
			memcpy(buf, stream->dataStart, maxRead);
			stream->dataStart += maxRead;
			buf[maxRead] = '\0';
			return buf;
		}
		// No newline present.
		memcpy(buf, stream->dataStart, maxRead);
		stream->dataStart += maxRead;
		buf += maxRead;
		size -= maxRead;
	}

	*buf = '\0';
	return s;	
}

int
uio_fgetc(uio_Stream *stream) {
	int result;

	uio_assertReadSanity(stream);
	stream->operation = uio_StreamOperation_read;

	if (stream->dataStart == stream->dataEnd) {
		// Buffer is empty
		if (uio_Stream_fillReadBuffer(stream) == -1) {
			stream->status = uio_Stream_STATUS_ERROR;
			return (int) EOF;
		}
		if (stream->dataStart == stream->dataEnd) {
			// End-of-file
			stream->status = uio_Stream_STATUS_EOF;
			stream->operation = uio_StreamOperation_none;
			return (int) EOF;
		}
	}

	result = (int) *((unsigned char *) stream->dataStart);
	stream->dataStart++;
	return result;
}

// Only one character pushback is guaranteed, just like with stdio ungetc().
int
uio_ungetc(int c, uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);
	assert(c >= 0 && c <= 255);

	return (int) EOF;
			// not implemented
//	return c;
}

// JMS: The datastream can be stepped back n bytes with this baby.
int
uio_backtrack(int rewinded_bytes, uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);

	stream->operation = uio_StreamOperation_read;
	stream->dataStart -= rewinded_bytes;
	return rewinded_bytes;
}

// NB. POSIX allows errno to be set for vsprintf(), but does not require it:
// "The value of errno may be set to nonzero by a library function call
// whether or not there is an error, provided the use of errno is not
// documented in the description of the function in this International
// Standard." The latter is the case for vsprintf().
int
uio_vfprintf(uio_Stream *stream, const char *format, va_list args) {
	// This could be done faster, but going through snprintf() is easiest,
	// and is fast enough for now.
	char *buf;
	int putResult;
	int savedErrno;

	buf = uio_vasprintf(format, args);
	if (buf == NULL) {
		// errno may or may not be set
		return -1;
	}

	putResult = uio_fputs(buf, stream);
	savedErrno = errno;

	uio_free(buf);

	errno = savedErrno;
	return putResult;
}

int
uio_fprintf(uio_Stream *stream, const char *format, ...) {
	va_list args;
	int result;

	va_start(args, format);
	result = uio_vfprintf(stream, format, args);
	va_end(args);

	return result;
}

int
uio_fputc(int c, uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_RDONLY);
	assert(c >= 0 && c <= 255);

	uio_assertWriteSanity(stream);
	stream->operation = uio_StreamOperation_write;

	if (stream->dataEnd == stream->bufEnd) {
		// The buffer is full. Flush it out.
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			// Error status (for ferror()) is set.
			return EOF;
		}
	}

	*(unsigned char *) stream->dataEnd = (unsigned char) c;
	stream->dataEnd++;
	return c;
}

int
uio_fputs(const char *s, uio_Stream *stream) {
	int result;
	
	result = uio_fwrite(s, strlen(s), 1, stream);
	if (result != 1)
		return EOF;
	return 0;
}

int
uio_fseek(uio_Stream *stream, long offset, int whence) {
	int newPos;

	if (stream->operation == uio_StreamOperation_read) {
		uio_Stream_discardReadBuffer(stream);
	} else if (stream->operation == uio_StreamOperation_write) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return -1;
		}
	}
	assert(stream->dataStart == stream->buf);
	assert(stream->dataEnd == stream->buf);
	stream->operation = uio_StreamOperation_none;

	newPos = uio_lseek(stream->handle, offset, whence);
	if (newPos == -1) {
		// errno is set
		return -1;
	}
	stream->status = uio_Stream_STATUS_OK;
			// Clear error or end-of-file flag.
	
	return 0;
}

long
uio_ftell(uio_Stream *stream) {
	off_t newPos;
	
	newPos = uio_lseek(stream->handle, 0, SEEK_CUR);
	if (newPos == (off_t) -1) {
		// errno is set
		return (long) -1;
	}
	
	if (stream->operation == uio_StreamOperation_write) {
		newPos += stream->dataEnd - stream->dataStart;
	} else if (stream->operation == uio_StreamOperation_read) {
		newPos -= stream->dataEnd - stream->dataStart;
	}

	return (long) newPos;
}

// If less that nmemb elements could be written, or an error occurs, the
// file pointer is undefined. clearerr() followed by fseek() need to be
// called before attempting to read or write again.
// I don't have the C standard myself, but I suspect this is the official
// behaviour for fread() and fwrite().
size_t
uio_fwrite(const void *buf, size_t size, size_t nmemb, uio_Stream *stream) {
	ssize_t bytesToWrite;
	ssize_t bytesWritten;

	uio_assertWriteSanity(stream);
	stream->operation = uio_StreamOperation_write;
	
	// NB. If a file is opened in append mode, the file position indicator
	// is moved to the end of the file before writing.
	// We leave that up to the physical layer.

	bytesToWrite = size * nmemb;
	if (bytesToWrite < stream->bufEnd - stream->dataEnd) {
		// There's enough space in the write buffer to store everything.
		memcpy(stream->dataEnd, buf, bytesToWrite);
		stream->dataEnd += bytesToWrite;
		return nmemb;
	}

	// Not enough space in the write buffer to write everything.
	// Flush what's left in the write buffer first.
	if (uio_Stream_flushWriteBuffer(stream) == -1) {
		// errno is set
		// Error status (for ferror()) is set.
		return 0;
	}
	
	if (bytesToWrite < stream->bufEnd - stream->dataEnd) {
		// The now empty write buffer is large enough to store everything.
		memcpy(stream->dataEnd, buf, bytesToWrite);
		stream->dataEnd += bytesToWrite;
		return nmemb;
	}

	// There is more data to write than fits in the (empty) write buffer.
	// The data is written directly, in its entirety, without going
	// through the write buffer.
	bytesWritten = uio_write(stream->handle, buf, bytesToWrite);
	if (bytesWritten != bytesToWrite) {
		stream->status = uio_Stream_STATUS_ERROR;
		if (bytesWritten == -1)
			return 0;
	}

	if (bytesWritten == bytesToWrite)
		return nmemb;
	return (size_t) bytesWritten / size;
}

// NB: stdio fflush() accepts NULL to flush all streams. uio_flush() does
// not.
int
uio_fflush(uio_Stream *stream) {
	assert(stream != NULL);

	if (stream->operation == uio_StreamOperation_write) {
		if (uio_Stream_flushWriteBuffer(stream) == -1) {
			// errno is set
			return (int) EOF;
		}
		stream->operation = uio_StreamOperation_none;
	}

	return 0;
}

int
uio_feof(uio_Stream *stream) {
	return stream->status == uio_Stream_STATUS_EOF;
}

int
uio_ferror(uio_Stream *stream) {
	return stream->status == uio_Stream_STATUS_ERROR;
}

void
uio_clearerr(uio_Stream *stream) {
	stream->status = uio_Stream_STATUS_OK;
}

// Counterpart of fileno()
uio_Handle *
uio_streamHandle(uio_Stream *stream) {
	return stream->handle;	
}

#ifndef NDEBUG
static void
uio_assertReadSanity(uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_WRONLY);
	
	if (stream->operation == uio_StreamOperation_write) {
		// "[...] output shall not be directly followed by input without an
		// intervening call to the fflush function or to a file positioning
		// function (fseek, fsetpos, or rewind), and input shall not be
		// directly followed by output without an intervening call to a file
		// positioning function, unless the input operation encounters
		// end-of-file." (POSIX, C)
		fprintf(stderr, "Error: Reading on a file directly after writing, "
				"without an intervening call to fflush() or a file "
				"positioning function.\n");
		abort();
	}
}
#endif

#ifndef NDEBUG
static void
uio_assertWriteSanity(uio_Stream *stream) {
	assert((stream->openFlags & O_ACCMODE) != O_RDONLY);
	
	if (stream->operation == uio_StreamOperation_read) {
		// "[...] output shall not be directly followed by input without an
		// intervening call to the fflush function or to a file positioning
		// function (fseek, fsetpos, or rewind), and input shall not be
		// directly followed by output without an intervening call to a file
		// positioning function, unless the input operation encounters
		// end-of-file." (POSIX, C)
		fprintf(stderr, "Error: Writing on a file directly after reading, "
				"without an intervening call to a file positioning "
				"function.\n");
		abort();
	}
	assert(stream->dataStart == stream->buf);
}
#endif

static int
uio_Stream_flushWriteBuffer(uio_Stream *stream) {
	ssize_t bytesWritten;

	assert(stream->operation == uio_StreamOperation_write);

	bytesWritten = uio_write(stream->handle, stream->dataStart,
			stream->dataEnd - stream->dataStart);
	if (bytesWritten != stream->dataEnd - stream->dataStart) {
		stream->status = uio_Stream_STATUS_ERROR;
		return -1;
	}
	assert(stream->dataStart == stream->buf);
	stream->dataEnd = stream->buf;

	return 0;
}
	
static void
uio_Stream_discardReadBuffer(uio_Stream *stream) {
	assert(stream->operation == uio_StreamOperation_read);
	stream->dataStart = stream->buf;
	stream->dataEnd = stream->buf;
	// TODO: when implementing pushback: throw away pushback buffer.
}

static int
uio_Stream_fillReadBuffer(uio_Stream *stream) {
	ssize_t numRead;

	assert(stream->operation == uio_StreamOperation_read);

	numRead = uio_read(stream->handle, stream->buf,
				uio_Stream_BLOCK_SIZE);
	if (numRead == -1)
		return -1;
	stream->dataStart = stream->buf;
	stream->dataEnd = stream->buf + numRead;
	return 0;	
}

static inline uio_Stream *
uio_Stream_new(uio_Handle *handle, int openFlags) {
	uio_Stream *result;

	result = uio_Stream_alloc();
	result->handle = handle;
	result->openFlags = openFlags;
	result->status = uio_Stream_STATUS_OK;
	result->operation = uio_StreamOperation_none;
	result->buf = uio_malloc(uio_Stream_BLOCK_SIZE);
	result->dataStart = result->buf;
	result->dataEnd = result->buf;
	result->bufEnd = result->buf + uio_Stream_BLOCK_SIZE;
	return result;
}

static inline uio_Stream *
uio_Stream_alloc(void) {
	uio_Stream *result = uio_malloc(sizeof (uio_Stream));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_Stream, (void *) result);
#endif
	return result;
}

static inline void
uio_Stream_delete(uio_Stream *stream) {
	uio_free(stream->buf);
	uio_Stream_free(stream);
}

static inline void
uio_Stream_free(uio_Stream *stream) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_Stream, (void *) stream);
#endif
	uio_free(stream);
}

