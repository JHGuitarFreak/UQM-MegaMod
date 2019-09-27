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

#ifndef LIBS_UIO_UIOSTREAM_H_
#define LIBS_UIO_UIOSTREAM_H_


typedef struct uio_Stream uio_Stream;

#include "io.h"

#include <stdarg.h>


uio_Stream *uio_fopen(uio_DirHandle *dir, const char *path, const char *mode);
int uio_fclose(uio_Stream *stream);
size_t uio_fread(void *buf, size_t size, size_t nmemb, uio_Stream *stream);
char *uio_fgets(char *buf, int size, uio_Stream *stream);
int uio_fgetc(uio_Stream *stream);
#define uio_getc uio_fgetc
int uio_ungetc(int c, uio_Stream *stream);
int uio_backtrack(int rewinded_bytes, uio_Stream *stream); // JMS
int uio_vfprintf(uio_Stream *stream, const char *format, va_list args);
int uio_fprintf(uio_Stream *stream, const char *format, ...);
int uio_fputc(int c, uio_Stream *stream);
#define uio_putc uio_fputc
int uio_fputs(const char *s, uio_Stream *stream);
int uio_fseek(uio_Stream *stream, long offset, int whence);
long uio_ftell(uio_Stream *stream);
size_t uio_fwrite(const void *buf, size_t size, size_t nmemb,
		uio_Stream *stream);
int uio_fflush(uio_Stream *stream);
int uio_feof(uio_Stream *stream);
int uio_ferror(uio_Stream *stream);
void uio_clearerr(uio_Stream *stream);
uio_Handle *uio_streamHandle(uio_Stream *stream);


/* *** Internal definitions follow *** */
#ifdef uio_INTERNAL

#include <sys/types.h>
#include <fcntl.h>
#include "iointrn.h"

typedef enum {
	uio_StreamOperation_none,
	uio_StreamOperation_read,
	uio_StreamOperation_write
} uio_StreamOperation;

struct uio_Stream {
	char *buf;
			// Start of the buffer.
	char *dataStart;
			// Start of the part of the buffer that is in use.
	char *dataEnd;
			// Start of the unused part of the buffer.
	char *bufEnd;
			// End of the buffer.
	// INV: buf <= dataStart <= dataEnd <= bufEnd
	// INV: if 'operation == uio_StreamOperation_write' then buf == dataStart

	uio_Handle *handle;
	int status;
#define uio_Stream_STATUS_OK 0
#define uio_Stream_STATUS_EOF 1
#define uio_Stream_STATUS_ERROR 2
	uio_StreamOperation operation;
			// What was the last action (reading or writing). This
			// determines whether the buffer is a read or write buffer.
	int openFlags;
			// Flags used for opening the file.
};


#endif  /* uio_INTERNAL */

#endif  /* LIBS_UIO_UIOSTREAM_H_ */


