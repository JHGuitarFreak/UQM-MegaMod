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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "libs/memlib.h"
#include "libs/log.h"
#include "libs/misc.h"


bool
mem_init (void)
{	// This is a stub
	return true;
}

bool
mem_uninit (void)
{	// This is a stub
	return true;
}

void *
HMalloc (size_t size)
{
	void *p = malloc (size);
	if (p == NULL && size > 0)
	{
		log_add (log_Fatal, "HMalloc() FATAL: out of memory.");
		fflush (stderr);
		explode ();
	}

	return p;
}

void
HFree (void *p)
{
	free (p);
}

void *
HCalloc (size_t size)
{
	void *p;

	p = HMalloc (size);
	memset (p, 0, size);

	return p;
}

void *
HRealloc (void *p, size_t size)
{
	p = realloc (p, size);
	if (p == NULL && size > 0)
	{
		log_add (log_Fatal, "HRealloc() FATAL: out of memory.");
		fflush (stderr);
		explode ();
	}

	return p;
}

