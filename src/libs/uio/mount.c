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

#include <assert.h>
#include <string.h>

#include "iointrn.h"
#include "uioport.h"
#include "mount.h"
#include "mounttree.h"
#include "mem.h"
#include "uioutils.h"
#ifdef uio_MEM_DEBUG
#	include "memdebug.h"
#endif

static void uio_Repository_delete(uio_Repository *repository);
static uio_Repository *uio_Repository_alloc(void);
static void uio_Repository_free(uio_Repository *repository);


void
uio_repositoryAddMount(uio_Repository *repository,
		uio_MountInfo *mountInfo, uio_MountLocation location,
		uio_MountInfo *relative) {
	lockRepository(repository, uio_LOCK_WRITE);
	switch (location) {
		case uio_MOUNT_TOP: {
			uio_MountInfo **newMounts;

			newMounts = uio_malloc(
					(repository->numMounts + 2) * sizeof (uio_MountInfo *));
			newMounts[0] = mountInfo;
			memcpy(&newMounts[1], repository->mounts,
					(repository->numMounts + 1) * sizeof (uio_MountInfo *));
			uio_free(repository->mounts);
			repository->mounts = newMounts;
			repository->numMounts++;
			break;
		}
		case uio_MOUNT_BOTTOM:
			repository->mounts = uio_realloc(repository->mounts,
					(repository->numMounts + 2) * sizeof (uio_MountInfo *));
			repository->mounts[repository->numMounts] = mountInfo;
			repository->numMounts++;
			break;
		case uio_MOUNT_ABOVE: {
			int i;
			uio_MountInfo **newMounts;

			i = 0;
			while (repository->mounts[i] != relative)
					i++;
			newMounts = (uio_MountInfo **) insertArrayPointer(
					(const void **) repository->mounts,
					repository->numMounts + 1, i, (void *) mountInfo);
			uio_free(repository->mounts);
			repository->mounts = newMounts;
			repository->numMounts++;
			break;
		}
		case uio_MOUNT_BELOW: {
			int i;
			uio_MountInfo **newMounts;

			i = 0;
			while (repository->mounts[i] != relative)
					i++;
			i++;
			newMounts = (uio_MountInfo **) insertArrayPointer(
					(const void **) repository->mounts,
					repository->numMounts + 1, i, (void *) mountInfo);
			uio_free(repository->mounts);
			repository->mounts = newMounts;
			repository->numMounts++;
			break;
		}
		default:
			assert(false);
	}
	unlockRepository(repository);
}

void
uio_repositoryRemoveMount(uio_Repository *repository, uio_MountInfo *mountInfo) {
	int i;
	uio_MountInfo **newMounts;

	lockRepository(repository, uio_LOCK_WRITE);

	i = 0;
	while (repository->mounts[i] != mountInfo)
		i++;
	newMounts = (uio_MountInfo **) excludeArrayPointer(
			(const void **) repository->mounts, repository->numMounts + 1,
			i, 1);
	uio_free(repository->mounts);
	repository->mounts = newMounts;
	repository->numMounts--;
	unlockRepository(repository);
}

// sets ref to 1
uio_Repository *
uio_Repository_new(int flags) {
	uio_Repository *result;

	result = uio_Repository_alloc();
	result->ref = 1;
	result->flags = flags;
	result->numMounts = 0;
	result->mounts = uio_malloc(1 * sizeof (uio_MountInfo *));
	result->mounts[0] = NULL;
	result->mountTree = uio_makeRootMountTree();
	return result;
}

void
uio_Repository_unref(uio_Repository *repository) {
	assert(repository->ref > 0);
	repository->ref--;
	if (repository->ref == 0)
		uio_Repository_delete(repository);
}

static uio_Repository *
uio_Repository_alloc(void) {
	uio_Repository *result = uio_malloc(sizeof (uio_Repository));
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugAlloc(uio_Repository, (void *) result);
#endif
	return result;
}

static void
uio_Repository_delete(uio_Repository *repository) {
	assert(repository->numMounts == 0);
	uio_free(repository->mounts);
	uio_MountTree_delete(repository->mountTree);
	uio_Repository_free(repository);
}

static void
uio_Repository_free(uio_Repository *repository) {
#ifdef uio_MEM_DEBUG
	uio_MemDebug_debugFree(uio_Repository, (void *) repository);
#endif
	uio_free(repository);
}


