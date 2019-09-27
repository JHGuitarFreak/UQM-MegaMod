/*
 *  Copyright 2009 Alex Volkov <codepro@usa.net>
 *
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

#include "palette.h"
#include "libs/memlib.h"
#include "libs/log.h"

NativePalette *
AllocNativePalette (void)
{
	return HCalloc (sizeof (NativePalette));
}

void
FreeNativePalette (NativePalette *palette)
{
	HFree (palette);
}

void
SetNativePaletteColor (NativePalette *palette, int index, Color color)
{
	assert (index < NUMBER_OF_PLUTVALS);
	palette->colors[index] = ColorToNative (color);
}

Color
GetNativePaletteColor (NativePalette *palette, int index)
{
	assert (index < NUMBER_OF_PLUTVALS);
	return NativeToColor (palette->colors[index]);
}
