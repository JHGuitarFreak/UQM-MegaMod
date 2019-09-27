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

#ifndef LIBS_UIO_IOAUX_H_
#define LIBS_UIO_IOAUX_H_

#include "iointrn.h"
#include "physical.h"
#include "uioport.h"

int uio_walkPhysicalPath(uio_PDirHandle *startPDirHandle, const char *path,
		size_t pathLen, uio_PDirHandle **endPDirHandle,
		const char **pathRest);
uio_PDirHandle *uio_makePath(uio_PDirHandle *pDirHandle, const char *path,
		size_t pathLen, mode_t mode);
int uio_copyFilePhysical(uio_PDirHandle *fromDir, const char *fromName,
		uio_PDirHandle *toDir, const char *toName);
int uio_getPhysicalAccess(uio_DirHandle *dirHandle, const char *path,
		int flags, int extraFlags,
		uio_MountInfo **mountInfoReadPtr, uio_PDirHandle **readPDirHandlePtr,
		char **readPRootPathPtr,
		uio_MountInfo **mountInfoWritePtr, uio_PDirHandle **writePDirHandlePtr,
		char **writePRootPathPtr,
		char **restPtr);
#define uio_GPA_NOWRITE 1
int uio_getPathPhysicalDirs(uio_DirHandle *dirHandle, const char *path,
		size_t pathLen, uio_PDirHandle ***resPDirHandles,
		int *resNumPDirHandles, uio_MountTreeItem ***resItems);
int uio_verifyPath(uio_DirHandle *dirHandle, const char *path,
		char **resolvedPath);
ssize_t uio_resolvePath(uio_DirHandle *dirHandle, const char *path,
		size_t pathLen, char **destPath);


#endif  /* LIBS_UIO_IOAUX_H_ */

