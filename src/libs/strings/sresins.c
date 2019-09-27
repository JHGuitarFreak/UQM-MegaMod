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

static void
GetStringTableFileData (const char *pathname, RESOURCE_DATA *resdata)
{
	resdata->ptr = LoadResourceFromPath (pathname, _GetStringData);
}

static void
GetBinaryTableFileData (const char *pathname, RESOURCE_DATA *resdata)
{
	resdata->ptr = LoadResourceFromPath (pathname, _GetBinaryTableData);
}

BOOLEAN
InstallStringTableResType (void)
{
	InstallResTypeVectors ("STRTAB", GetStringTableFileData, FreeResourceData, NULL);
	InstallResTypeVectors ("BINTAB", GetBinaryTableFileData, FreeResourceData, NULL);
	InstallResTypeVectors ("CONVERSATION", _GetConversationData, FreeResourceData, NULL);
	return TRUE;
}

STRING_TABLE
LoadStringTableInstance (RESOURCE res)
{
	void *data;

	data = res_GetResource (res);
	if (data)
	{
		res_DetachResource (res);
	}

	return (STRING_TABLE)data;
}

