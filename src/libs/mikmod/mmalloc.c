/*	MikMod sound library
	(c) 1998, 1999 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  Dynamic memory routines

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "mikmod_internals.h"

void* MikMod_realloc(void *data, size_t size)
{
	if (data)
		return realloc(data, size);
	else
		return MikMod_malloc(size);
}

/* Same as malloc, but sets error variable _mm_error when fails */
void* MikMod_malloc(size_t size)
{
	void *d;

	if(!(d=calloc(1,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
}

/* Same as calloc, but sets error variable _mm_error when fails */
void* MikMod_calloc(size_t nitems,size_t size)
{
	void *d;
   
	if(!(d=calloc(nitems,size))) {
		_mm_errno = MMERR_OUT_OF_MEMORY;
		if(_mm_errorhandler) _mm_errorhandler();
	}
	return d;
}

void MikMod_free(void *p)
{
	if (p)
		free(p);
}

/* ex:set ts=4: */
