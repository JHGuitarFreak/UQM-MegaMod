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

#if !defined(ANDROID) || !defined(__ANDROID__)

#include "msgbox.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>


// Converts a UTF-8 string to Windows WideChar.
// Caller is responsible for free()ing the returned string.
static LPWSTR
toWideChar (const /*UTF-8*/char *str)
{
	int cch;
	LPWSTR wstr;

	cch = MultiByteToWideChar (CP_UTF8, 0, str, -1, NULL, 0);
	if (cch == 0)
		return NULL; // failed, probably no UTF8 converter

	wstr = malloc (cch * sizeof (WCHAR));
	if (!wstr)
		return NULL; // out of memory

	cch = MultiByteToWideChar (CP_UTF8, 0, str, -1, wstr, cch);
	if (cch == 0)
	{	// Failed. It should not fail here if it succeeded just above,
		// but it did. Not much can be done about it.
		free (wstr);
		return NULL;
	}

	return wstr;
}

void
log_displayBox (const /*UTF-8*/char *title, int isError,
		const /*UTF-8*/char *msg)
{
	LPWSTR swTitle = toWideChar (title);
	LPWSTR swMsg = toWideChar (msg);
	UINT uType = isError ? MB_ICONWARNING : MB_ICONINFORMATION;

	if (swTitle && swMsg)
		MessageBoxW (NULL, swMsg, swTitle, uType);
	else // Could not convert; let's try ASCII, though it may look ugly
		MessageBoxA (NULL, msg, title, uType);

	free (swTitle);
	free (swMsg);
}

#endif