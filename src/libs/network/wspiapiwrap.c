/*
 *  Copyright 2006  Serge van den Boom <svdb@stack.nl>
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

// Only used for MinGW

// HACK. MinGW misses some functionality, so we're #including
// the actual Windows wspiapi.h file. Because that file includes
// inline functions that aren't static, it can only be included
// once per executable when using gcc (for MSVC this is apparently
// not a problem), so this file is it. The prototypes of these
// functions are added to wspiapiwrap.h

#include "netport.h"

#if defined(USE_WINSOCK) && defined(__MINGW32__)
#	include <ws2tcpip.h>
#	include <wspiapi.h>
#endif

