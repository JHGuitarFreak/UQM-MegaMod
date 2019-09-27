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

#include "scriptres.h"

#include <string.h>


BOOLEAN
ReleaseScriptResData (void *data)
{
	HFree (data);
	return TRUE;
}

static void
GetScriptResData (const char *pathName, RESOURCE_DATA *resdata)
{
	// We don't actually load the data here. We determine the file name, and
	// load the data when we need it, directly onto the Lua stack.
	size_t pathNameLen = strlen (pathName);
	resdata->ptr = HMalloc (pathNameLen + 1);
	if (resdata->ptr == NULL)
		return;

	memcpy (resdata->ptr, pathName, pathNameLen + 1);
}

BOOLEAN
InstallScriptResType (void)
{
	InstallResTypeVectors ("SCRIPT", GetScriptResData, ReleaseScriptResData,
			NULL);
	return TRUE;
}

// Actually just returns the file name of the script.
char *
LoadScriptInstance (RESOURCE res)
{
	void *data = res_GetResource (res);
	if (data)
		res_DetachResource (res);

	return (char *) data;
}

