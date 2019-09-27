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

#include "strintrn.h"
#include "libs/memlib.h"

STRING_TABLE
AllocStringTable (int num_entries, int flags)
{
	STRING_TABLE strtab = HMalloc (sizeof (STRING_TABLE_DESC));
	int i, multiplier = 1;

	if (flags & HAS_NAMEINDEX)
	{
		multiplier++;
	}
	if (flags & HAS_SOUND_CLIPS)
	{
		multiplier++;
	}
	if (flags & HAS_TIMESTAMP)
	{
		multiplier++;
	}
	strtab->flags = flags;
	strtab->size = num_entries;
	num_entries *= multiplier;
	strtab->strings = HMalloc (sizeof (STRING_TABLE_ENTRY_DESC) * num_entries);
	for (i = 0; i < num_entries; i++)
	{
		strtab->strings[i].data = NULL;
		strtab->strings[i].length = 0;
		strtab->strings[i].parent = strtab;
		strtab->strings[i].index = i;
	}
	strtab->nameIndex = NULL;
	return strtab;
}

void
FreeStringTable (STRING_TABLE strtab)
{
	int i, multiplier = 1;

	if (strtab == NULL)
	{
		return;
	}

	if (strtab->flags & HAS_SOUND_CLIPS)
	{
		multiplier++;
	}
	if (strtab->flags & HAS_TIMESTAMP)
	{
		multiplier++;
	}

	for (i = 0; i < strtab->size * multiplier; i++)
	{
		if (strtab->strings[i].data != NULL)
		{
			HFree (strtab->strings[i].data);
		}
	}

	HFree (strtab->strings);
	HFree (strtab);
}

BOOLEAN
DestroyStringTable (STRING_TABLE StringTable)
{
	FreeStringTable (StringTable);
	return TRUE;
}

STRING
CaptureStringTable (STRING_TABLE StringTable)
{
	if ((StringTable != 0) && (StringTable->size > 0))
	{
		return StringTable->strings;
	}

	return NULL;
}

STRING_TABLE
ReleaseStringTable (STRING String)
{
	STRING_TABLE StringTable;

	StringTable = GetStringTable (String);

	return (StringTable);
}

STRING_TABLE
GetStringTable (STRING String)
{
	if (String && String->parent)
	{
		return String->parent;
	}
	return NULL;
}

COUNT
GetStringTableCount (STRING String)
{
	if (String && String->parent)
	{
		return String->parent->size;
	}
	return 0;
}

COUNT
GetStringTableIndex (STRING String)
{
	if (String)
	{
		return String->index;
	}
	return 0;
}

STRING
SetAbsStringTableIndex (STRING String, COUNT StringTableIndex)
{
	STRING_TABLE StringTablePtr;

	if (!String)
		return NULL;
	
	StringTablePtr = String->parent;
	
	if (StringTablePtr == NULL)
	{
		String = NULL;
	}
	else
	{
		StringTableIndex = StringTableIndex % StringTablePtr->size;
		String = &StringTablePtr->strings[StringTableIndex];
	}

	return (String);
}

STRING
SetRelStringTableIndex (STRING String, SIZE StringTableOffs)
{
	STRING_TABLE StringTablePtr;

	if (!String)
		return NULL;
	
	StringTablePtr = String->parent;
	
	if (StringTablePtr == NULL)
	{
		String = NULL;
	}
	else
	{
		COUNT StringTableIndex;

		while (StringTableOffs < 0)
			StringTableOffs += StringTablePtr->size;
		StringTableIndex = (String->index + StringTableOffs)
				% StringTablePtr->size;

		String = &StringTablePtr->strings[StringTableIndex];
	}

	return (String);
}

COUNT
GetStringLength (STRING String)
{
	if (String == NULL)
	{
		return 0;
	}
	return utf8StringCountN(String->data, String->data + String->length);
}

COUNT
GetStringLengthBin (STRING String)
{
	if (String == NULL)
	{
		return 0;
	}
	return String->length;
}

STRINGPTR
GetStringName (STRING String)
{
	STRING_TABLE StringTablePtr;
	COUNT StringIndex;

	if (String == NULL)
	{
		return NULL;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == NULL)
	{
		return NULL;
	}

	StringIndex = String->index;

	if (!(StringTablePtr->flags & HAS_NAMEINDEX))
	{
		return NULL;
	}
	StringIndex += StringTablePtr->size;

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return NULL;
	}

	return String->data;
}

STRINGPTR
GetStringSoundClip (STRING String)
{
	STRING_TABLE StringTablePtr;
	COUNT StringIndex;

	if (String == NULL)
	{
		return NULL;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == NULL)
	{
		return NULL;
	}

	StringIndex = String->index;
	if (!(StringTablePtr->flags & HAS_SOUND_CLIPS))
	{
		return NULL;
	}
	StringIndex += StringTablePtr->size;

	if (StringTablePtr->flags & HAS_NAMEINDEX)
	{
		StringIndex += StringTablePtr->size;
	}

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return NULL;
	}

	return String->data;
}

STRINGPTR
GetStringTimeStamp (STRING String)
{
	STRING_TABLE StringTablePtr;
	COUNT StringIndex;

	if (String == NULL)
	{
		return NULL;
	}

	StringTablePtr = String->parent;
	if (StringTablePtr == NULL)
	{
		return NULL;
	}

	StringIndex = String->index;
	if (!(StringTablePtr->flags & HAS_TIMESTAMP))
	{
		return NULL;
	}
	StringIndex += StringTablePtr->size;

	if (StringTablePtr->flags & HAS_NAMEINDEX)
	{
		StringIndex += StringTablePtr->size;
	}
	
	if (StringTablePtr->flags & HAS_SOUND_CLIPS)
	{
		StringIndex += StringTablePtr->size;
	}

	String = &StringTablePtr->strings[StringIndex];
	if (String->length == 0)
	{
		return NULL;
	}

	return String->data;
}

STRINGPTR
GetStringAddress (STRING String)
{
	if (String == NULL)
	{
		return NULL;
	}
	return String->data;
}

STRING
GetStringByName (STRING_TABLE StringTable, const char *index)
{
	return (STRING) StringHashTable_find (StringTable->nameIndex, index);
}


