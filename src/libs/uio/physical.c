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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "physical.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif
#include "uioport.h"

static inline uio_PRoot *uio_PRoot_alloc(void);
static inline void uio_PRoot_free(uio_PRoot *pRoot);

// NB: ref counter is not incremented
uio_PDirHandle *
uio_PRoot_getRootDirHandle(uio_PRoot *pRoot) {
	return pRoot->rootDir;
}

void
uio_PRoot_deletePRootExtra(uio_PRoot *pRoot) {
	if (pRoot->extra == NULL)
		return;
	assert(pRoot->handler->deletePRootExtra != NULL);
	pRoot->handler->deletePRootExtra(pRoot->extra);
}

// note: sets refMount count to 1
//       set handlerRef count to 0
uio_PRoot *
uio_PRoot_new(uio_PDirHandle *topDirHandle,
		uio_FileSystemHandler *handler, uio_Handle *handle,
		uio_PRootExtra extra, int flags) {
	uio_PRoot *pRoot;
	
	pRoot = uio_PRoot_alloc();
	pRoot->mountRef = 1;
	pRoot->handleRef = 0;
	pRoot->rootDir = topDirHandle;
	pRoot->handler = handler;
	pRoot->handle = handle;
	pRoot->extra = extra;
	pRoot->flags = flags;
#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
	pRoot->numCloseHandlers = 0;
	pRoot->closeHandlers = NULL;
#endif

	return pRoot;
}

#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
// Closehandlers code disabled.
// It was only meant for internal use, but I don't need it any more.
// Keeping it around for a while until I'm confident I won't need it in the
// future.

void
uio_PRoot_addCloseHandler(uio_PRoot *pRoot, void (*fun)(void *), void *arg) {
	pRoot->numCloseHandlers++;
	pRoot->closeHandlers = uio_realloc(pRoot->closeHandlers,
			pRoot->numCloseHandlers * sizeof (uio_PRoot_CloseHandler));
	pRoot->closeHandlers[pRoot->numCloseHandlers - 1].fun = fun;
	pRoot->closeHandlers[pRoot->numCloseHandlers - 1].arg = arg;
}

void
uio_PRoot_callCloseHandlers(uio_PRoot *pRoot) {
	int i;

	i = pRoot->numCloseHandlers;
	while (i--) {
		uio_PRoot_CloseHandler *closeHandler;

		closeHandler = &pRoot->closeHandlers[i];
		(closeHandler->fun)(closeHandler->arg);
	}
}

void
uio_PRoot_removeCloseHandlers(uio_PRoot *pRoot) {
	pRoot->numCloseHandlers = 0;
	if (pRoot->closeHandlers != NULL)
		uio_free(pRoot->closeHandlers);
	pRoot->closeHandlers = NULL;
}
#endif

static inline void
uio_PRoot_delete(uio_PRoot *pRoot) {
#ifdef uio_PROOT_HAVE_CLOSE_HANDLERS
	uio_PRoot_callCloseHandlers(pRoot);
	uio_PRoot_removeCloseHandlers(pRoot);
#endif
	assert(pRoot->handler->umount != NULL);
	pRoot->handler->umount(pRoot);
	if (pRoot->handle)
		uio_Handle_unref(pRoot->handle);
	uio_PRoot_deletePRootExtra(pRoot);
	uio_PRoot_free(pRoot);
}

static inline uio_PRoot *
uio_PRoot_alloc(void) {
	uio_PRoot *result = uio_malloc(sizeof (uio_PRoot));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_PRoot, (void *) result);
#endif
	return result;
}

static inline void
uio_PRoot_free(uio_PRoot *pRoot) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_PRoot, (void *) pRoot);
#endif
	uio_free(pRoot);
}

void
uio_PRoot_refHandle(uio_PRoot *pRoot) {
	pRoot->handleRef++;
}

void
uio_PRoot_unrefHandle(uio_PRoot *pRoot) {
	assert(pRoot->handleRef > 0);
	pRoot->handleRef--;
	if (pRoot->handleRef == 0 && pRoot->mountRef == 0)
		uio_PRoot_delete(pRoot);
}

void
uio_PRoot_refMount(uio_PRoot *pRoot) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugRef(uio_PRoot, (void *) pRoot);
#endif
	pRoot->mountRef++;
}

void
uio_PRoot_unrefMount(uio_PRoot *pRoot) {
	assert(pRoot->mountRef > 0);
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugUnref(uio_PRoot, (void *) pRoot);
#endif
	pRoot->mountRef--;
	if (pRoot->mountRef == 0 && pRoot->handleRef == 0)
		uio_PRoot_delete(pRoot);
}


