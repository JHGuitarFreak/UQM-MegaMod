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

#ifndef uio_DEFAULTFS_H
#define uio_DEFAULTFS_H

typedef struct uio_DefaultFileSystemSetup uio_DefaultFileSystemSetup;

#include "iointrn.h"
#include "uioport.h"
#include "fstypes.h"

struct uio_DefaultFileSystemSetup {
	uio_FileSystemID id;
	const char *name;
	uio_FileSystemHandler *handler;
};

extern const uio_DefaultFileSystemSetup defaultFileSystems[];

int uio_numDefaultFileSystems(void);

#endif  /* uio_DEFAULTFS_H */

