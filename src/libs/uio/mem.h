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

#ifndef LIBS_UIO_MEM_H_
#define LIBS_UIO_MEM_H_

#include <stdlib.h>
#include <string.h>
#include "uioport.h"

#define uio_malloc malloc
#define uio_realloc realloc
#define uio_free free
#define uio_calloc calloc

#ifdef uio_MEM_DEBUG
// When uio_strdup is defined to the libc strdup, there's no opportunity
// to intercept the alloc. Hence this function here.
static inline char *
uio_strdup(const char *s) {
	char *result;
	size_t size;

	size = strlen(s) + 1;
	result = uio_malloc(size);
	memcpy(result, s, size);
	return result;
}
#else
#	define uio_strdup strdup
#endif

// Allocates new memory, copies 'len' characters from 'src', and adds a '\0'.
static inline char *
uio_memdup0(const char *src, size_t len) {
	char *dst = uio_malloc(len + 1);
	memcpy(dst, src, len);
	dst[len] = '\0';
	return dst;
}

#endif


