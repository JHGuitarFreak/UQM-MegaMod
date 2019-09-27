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

#ifndef LIBS_STRINGS_STRINTRN_H_
#define LIBS_STRINGS_STRINTRN_H_

#include <stdio.h>
#include <string.h>
#include "libs/strlib.h"
#include "libs/reslib.h"
#include "stringhashtable.h"

struct string_table_entry
{
	STRINGPTR data;
	int length;  /* Internal NULs are allowed */
	int index;
	struct string_table *parent;
};

struct string_table
{
	unsigned short flags;
	int size;
	STRING_TABLE_ENTRY_DESC *strings;
	StringHashTable_HashTable *nameIndex;
};

#define HAS_SOUND_CLIPS  (1 << 0)
#define HAS_TIMESTAMP    (1 << 1)
#define HAS_NAMEINDEX    (1 << 2)

STRING_TABLE AllocStringTable (int num_entries, int flags);
void FreeStringTable (STRING_TABLE strtab);

void *_GetStringData (uio_Stream *fp, DWORD length);
void *_GetBinaryTableData (uio_Stream *fp, DWORD length);
void _GetConversationData (const char *path, RESOURCE_DATA *resdata);

#endif /* LIBS_STRINGS_STRINTRN_H_ */

