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

// This file includes some misc things, previously in SDL_wrapper.h 
// before modularization. -Mika

#ifndef MISC_H
#define MISC_H

#include <sys/types.h>
#include <stdlib.h>
#include "port.h"

#if defined(__cplusplus)
extern "C" {
#endif


extern int TFB_DEBUG_HALT;

static inline void explode (void) _NORETURN;

static inline void explode (void)
{
#ifdef DEBUG
	// give debugger a chance to hook
	abort ();
#else
	exit (EXIT_FAILURE);
#endif
}

/* Sometimes you just have to remove a 'const'.
 * (for instance, when implementing a function like strchr)
 */
static inline void *
unconst(const void *arg) {
	union {
		void *c;
		const void *cc;
	} u;
	u.cc = arg;
	return u.c;
}

#if defined(__cplusplus)
}
#endif

#endif

