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

/* By Serge van den Boom
 */

#include "sdltime.h"
#include "libs/timelib.h"

Uint32
SDLWrapper_GetTimeCounter (void)
{
	Uint32 ticks = SDL_GetTicks ();
	return (ticks / 1000) * ONE_SECOND + ((ticks % 1000) * ONE_SECOND / 1000);
	// Use the following instead when confirming "random" lockup bugs (see #668)
	//return ticks * ONE_SECOND / 1000;
}
