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

#ifndef LIBS_UIO_UTILS_H_
#define LIBS_UIO_UTILS_H_

#include <stdarg.h>

#ifdef uio_INTERNAL
typedef struct uio_StdioAccessHandle uio_StdioAccessHandle;
#else
typedef void uio_StdioAccessHandle;
#endif

int uio_copyFile(uio_DirHandle *srcDir, const char *srcName,
		uio_DirHandle *dstDir, const char *newName);
uio_StdioAccessHandle *uio_getStdioAccess(uio_DirHandle *dir,
		const char *path, int flags, uio_DirHandle *tempDir);
const char *uio_StdioAccessHandle_getPath(uio_StdioAccessHandle *handle);
void uio_releaseStdioAccess(uio_StdioAccessHandle *handle);

char *uio_vasprintf(const char *format, va_list args);
char *uio_asprintf(const char *format, ...);

#endif  /* LIBS_UIO_UTILS_H_ */

