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

#include "gfxintrn.h"

static void
GetCelFileData (const char *pathname, RESOURCE_DATA *resdata)
{
	resdata->ptr = LoadResourceFromPath (pathname, _GetCelData);
}

static void
GetFontFileData (const char *pathname, RESOURCE_DATA *resdata)
{
	resdata->ptr = LoadResourceFromPath (pathname, _GetFontData);
}


BOOLEAN
InstallGraphicResTypes (void)
{
	InstallResTypeVectors ("GFXRES", GetCelFileData, _ReleaseCelData, NULL);
	InstallResTypeVectors ("FONTRES", GetFontFileData, _ReleaseFontData, NULL);
	return (TRUE);
}

/* Needs to be void * because it could be either a DRAWABLE or a FONT. */
void *
LoadGraphicInstance (RESOURCE res)
{
	void *hData;

	hData = res_GetResource (res);
	if (hData)
		res_DetachResource (res);

	return (hData);
}

