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

#include "defaultfs.h"
#include "uioport.h"

extern uio_FileSystemHandler stdio_fileSystemHandler;
#ifdef HAVE_ZIP
extern uio_FileSystemHandler zip_fileSystemHandler;
#endif

const uio_DefaultFileSystemSetup defaultFileSystems[] = {
	{ uio_FSTYPE_STDIO, "stdio", &stdio_fileSystemHandler },
#ifdef HAVE_ZIP
	{ uio_FSTYPE_ZIP, "zip", &zip_fileSystemHandler },
#endif
};

int
uio_numDefaultFileSystems(void) {
	return sizeof defaultFileSystems / sizeof defaultFileSystems[0];
}


