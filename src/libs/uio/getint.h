/*
 * Copyright (C) 2007  Serge van den Boom
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

#ifndef LIBS_UIO_GETINT_H_
#define LIBS_UIO_GETINT_H_

/* All these functions return true on success, or false on failure */

#include "types.h"
#include "uioport.h"

static inline uio_bool
uio_getU8(uio_Stream *stream, uio_uint8 *result) {
	int val = uio_getc(stream);
	if (val == EOF)
		return false;

	*result = (uio_uint8) val;
	return true;
}

static inline uio_bool
uio_getS8(uio_Stream *stream, uio_sint8 *result) {
	int val = uio_getc(stream);
	if (val == EOF)
		return false;

	*result = (uio_sint8) val;
	return true;
}

static inline uio_bool
uio_getU16LE(uio_Stream *stream, uio_uint16 *result) {
	uio_uint8 buf[2];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (buf[1] << 8) | buf[0];
	return true;
}

static inline uio_bool
uio_getU16BE(uio_Stream *stream, uio_uint16 *result) {
	uio_uint8 buf[2];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (buf[0] << 8) | buf[1];
	return true;
}

static inline uio_bool
uio_getS16LE(uio_Stream *stream, uio_sint16 *result) {
	uio_uint8 buf[2];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (uio_sint16) ((buf[1] << 8) | buf[0]);
	return true;
}

static inline uio_bool
uio_getS16BE(uio_Stream *stream, uio_sint16 *result) {
	uio_uint8 buf[2];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (uio_sint16) ((buf[0] << 8) | buf[1]);
	return true;
}

static inline uio_bool
uio_getU32LE(uio_Stream *stream, uio_uint32 *result) {
	uio_uint8 buf[4];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	return true;
}

static inline uio_bool
uio_getU32BE(uio_Stream *stream, uio_uint32 *result) {
	uio_uint8 buf[4];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	return true;
}

static inline uio_bool
uio_getS32LE(uio_Stream *stream, uio_sint32 *result) {
	uio_uint8 buf[4];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (uio_sint32) ((buf[3] << 24) | (buf[2] << 16) |
				(buf[1] << 8) | buf[0]);
	return true;
}

static inline uio_bool
uio_getS32BE(uio_Stream *stream, uio_sint32 *result) {
	uio_uint8 buf[4];

	if (uio_fread(buf, sizeof buf, 1, stream) != 1)
		return false;

	*result = (uio_sint32) ((buf[0] << 24) | (buf[1] << 16) |
				(buf[2] << 8) | buf[3]);
	return true;
}


#endif  /* LIBS_UIO_GETINT_H_ */

