/*
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
/*
 * CDP dlopen() & Co. WIN32 implementation
 */

#include "windl.h"
#include "port.h"
#define WIN32_LEAN_AND_MEAN
//#include <windows.h>
#include <wtypes.h>
#include <winbase.h>
#include <stdio.h>

static uint32 wdl_last_error = 0;
static char wdl_errstr[128] = "";

void*
dlopen (const char *filename, int flag)
		// all defined flags are not possible on win32
{
	HMODULE hlib;

	if (filename == NULL)
		hlib = GetModuleHandleA(NULL);
	else
		hlib = LoadLibraryA (filename);

	if (!hlib)
		wdl_last_error = GetLastError ();

	return hlib;
}

void*
dlsym (void *handle, const char *symbol)
{
	void* ptr = GetProcAddress (handle, symbol);
	if (!ptr)
		wdl_last_error = GetLastError ();
	return ptr;
}

int
dlclose (void *handle)
{
	return FreeLibrary (handle);
}

char*
dlerror (void)
{
	if (wdl_last_error)
	{
		sprintf (wdl_errstr, "Windows error %u", wdl_last_error);
		wdl_last_error = 0;
		return wdl_errstr;
	}
	else
		return NULL;
}
