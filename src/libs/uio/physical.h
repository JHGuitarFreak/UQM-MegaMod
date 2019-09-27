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

#ifndef LIBS_UIO_PHYSICAL_H_
#define LIBS_UIO_PHYSICAL_H_

#ifndef uio_INTERNAL_PHYSICAL
typedef void *uio_PRootExtra;
typedef void *uio_NativeEntriesContext;
#endif

// 'forward' declarations
typedef struct uio_PRoot uio_PRoot;
typedef struct uio_PRoot_CloseHandler uio_PRoot_CloseHandler;


#include "iointrn.h"
#include "uioport.h"
#include "fstypes.h"


/*
 * Represents the root of a physical directory structure.
 */
struct uio_PRoot {
	int mountRef;
			/* Number of times this structure is referenced from
			 * mount trees. */
	int handleRef;
			/* Number of file or directory handles that point inside the
			 * physical directory strucure of this pRoot.
			 */
	struct uio_PDirHandle *rootDir;
	struct uio_FileSystemHandler *handler;
			/* How to handle files in this PRoot tree */
	int flags;
#		define uio_PRoot_NOCACHE 0x0002
	struct uio_Handle *handle;
			/* The handle through which this PRoot is opened,
			 * this is NULL for the top PRoot */
			// TODO: move this to extra?
#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
	int numCloseHandlers;
	uio_PRoot_CloseHandler *closeHandlers;
#endif
	uio_PRootExtra extra;
			/* extra internal data for some filesystem types */
};

#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
struct uio_PRoot_CloseHandler {
	void (*fun)(void *);
	void *arg;
};
#endif

void uio_PRoot_deletePRootExtra(uio_PRoot *pRoot);
uio_PRoot *uio_PRoot_new(uio_PDirHandle *topDirHandle,
		uio_FileSystemHandler *handler, uio_Handle *handle,
		uio_PRootExtra extra, int flags);
#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
void uio_PRoot_addCloseHandler(uio_PRoot *pRoot, void (*fun)(void *),
		void *arg);
void uio_PRoot_callCloseHandlers(uio_PRoot *pRoot);
void uio_PRoot_removeCloseHandlers(uio_PRoot *pRoot);
#endif
uio_PDirHandle *uio_PRoot_getRootDirHandle(uio_PRoot *pRoot);
void uio_PRoot_refHandle(uio_PRoot *pRoot);
void uio_PRoot_unrefHandle(uio_PRoot *pRoot);
void uio_PRoot_refMount(uio_PRoot *pRoot);
void uio_PRoot_unrefMount(uio_PRoot *pRoot);

#endif  /* LIBS_UIO_PHYSICAL_H_  */


