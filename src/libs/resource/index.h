//Copyright Paul Reiche, Fred Ford. 1992-2002

/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef LIBS_RESOURCE_INDEX_H_
#define LIBS_RESOURCE_INDEX_H_

typedef struct resource_handlers ResourceHandlers;
typedef struct resource_desc ResourceDesc;

#include <stdio.h>
#include "libs/reslib.h"
#include "libs/uio/charhashtable.h"

struct resource_handlers
{
	const char *resType;
	ResourceLoadFun *loadFun;
	ResourceFreeFun *freeFun;
	ResourceStringFun *toString;
};

struct resource_desc
{
	RESOURCE res_id;
	char *fname;
	ResourceHandlers *vtable;
	RESOURCE_DATA resdata;
	// refcount is rudimentary as nothing really frees the descriptors
	unsigned refcount;
};

struct resource_index_desc
{
	CharHashTable_HashTable *map;
	size_t numRes;
};

#endif /* LIBS_RESOURCE_INDEX_H_ */

