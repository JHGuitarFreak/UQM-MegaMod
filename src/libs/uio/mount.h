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

#ifndef LIBS_UIO_MOUNT_H_
#define LIBS_UIO_MOUNT_H_


typedef struct uio_Repository uio_Repository;
typedef struct uio_AutoMount uio_AutoMount;
#define uio_MOUNT_RDONLY (1 << 1)

/* *** Internal definitions follow */
#ifdef uio_INTERNAL

#define uio_MOUNT_LOCATION_MASK (3 << 2)

#include "uioport.h"
#include "fstypes.h"
#include "mounttree.h"
#include "match.h"

struct uio_Repository {
	int ref;            /* reference counter */
	int flags;
	int numMounts;
	struct uio_MountInfo **mounts;
			// first in the list is considered the top
			// last entry is NULL
	struct uio_MountTree *mountTree;
};

#define lockRepository(repository, prot)
#define unlockRepository(repository)

uio_Repository *uio_Repository_new(int flags);
void uio_Repository_unref(uio_Repository *repository);
void uio_repositoryAddMount(uio_Repository *repository,
		uio_MountInfo *mountInfo, uio_MountLocation location,
		uio_MountInfo *relative);
void uio_repositoryRemoveMount(uio_Repository *repository,
		uio_MountInfo *mountInfo);


#endif  /* uio_INTERNAL */

#endif  /* LIBS_UIO_MOUNT_H_ */

