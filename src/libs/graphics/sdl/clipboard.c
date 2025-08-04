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

#include "libs/log/uqmlog.h"
#include <SDL.h>

#ifdef _WIN32

#include <windows.h>

static HBITMAP
sdl_surface_to_bitmap (SDL_Surface *surface)
{
	SDL_PixelFormat format;
	SDL_Surface *convPtr;
	BITMAPINFOHEADER BIH;
	HDC hdc;
	HBITMAP hbitmap;

	// Convert to 32-bit BGRA (required for Windows DIB)
#if SDL_MAJOR_VERSION > 1
	format.format = SDL_PIXELFORMAT_BGRA32;
#endif /* SDL_MAJOR_VERSION > 1 */
	format.palette = NULL;
	format.BitsPerPixel = 32;
	format.BytesPerPixel = 4;
	format.Rmask = 0x00FF0000;
	format.Gmask = 0x0000FF00;
	format.Bmask = 0x000000FF;
	format.Amask = 0xFF000000;

	convPtr = SDL_ConvertSurface (surface, &format, 0);
	if (!convPtr)
		return NULL;

	// Create DIB header
	BIH.biSize = sizeof (BITMAPINFOHEADER);
	BIH.biWidth = convPtr->w;
	BIH.biHeight = -convPtr->h;
	BIH.biPlanes = 1;
	BIH.biBitCount = 32;
	BIH.biCompression = BI_RGB;

	// Create DIB section
	hdc = GetDC (NULL);
	hbitmap = CreateDIBitmap (hdc, &BIH, CBM_INIT, convPtr->pixels,
		(BITMAPINFO *)&BIH, DIB_RGB_COLORS);

	ReleaseDC (NULL, hdc);
	SDL_FreeSurface (convPtr);

	return hbitmap;
}

static int
CopySurfaceToClipboard_Win32 (SDL_Surface *surface)
{
	HBITMAP hbitmap;

	if (!surface)
		return -1;

	// Convert surface to Windows bitmap
	hbitmap = sdl_surface_to_bitmap (surface);
	if (!hbitmap)
		return -1;

	// Clipboard operations
	if (!OpenClipboard (NULL))
	{
		DeleteObject (hbitmap);
		return -1;
	}

	EmptyClipboard ();
	SetClipboardData (CF_BITMAP, hbitmap);
	CloseClipboard ();

	return 0;
}

#endif /* _WIN32 */

int
CopySurfaceToClipboard (SDL_Surface *surface)
{
#ifdef _WIN32
	return CopySurfaceToClipboard_Win32 (surface);
#else
	log_add (log_Warning, "Clipboard functionality is not implemented on "
			"this platform.");
	return FALSE;
#endif
}