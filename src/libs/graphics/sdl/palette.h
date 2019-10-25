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

#ifndef PALETTE_H_INCL__
#define PALETTE_H_INCL__

#include "port.h"
#include SDL_INCLUDE(SDL.h)
#include "libs/graphics/cmap.h"

struct NativePalette
{
	SDL_Color colors[NUMBER_OF_PLUTVALS];
};

static inline Color
NativeToColor (SDL_Color native)
{
	Color color;
	color.r = native.r;
	color.g = native.g;
	color.b = native.b;
	color.a = 0xff; // fully opaque
	return color;
}

static inline SDL_Color
ColorToNative (Color color)
{
	SDL_Color native;
	native.r = color.r;
	native.g = color.g;
	native.b = color.b;
#if SDL_MAJOR_VERSION == 1
	native.unused = 0;
#else
	native.a = color.a;
#endif
	return native;
}

#endif /* PALETTE_H_INCL__ */
