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

// This file contains variants of the random functions in random.c
// that store the state of the RNG in a context, allowing for multiple
// independant RNGs to be used simultaneously.
// The RNG behavior itself is the same.

#include "libs/compiler.h"

#define RANDOM2_INTERNAL
#include "random.h"

#include "libs/memlib.h"

RandomContext *
RandomContext_New (void)
{
	RandomContext *result = (RandomContext *) HMalloc (sizeof (RandomContext));
	result->seed = 12345;
	return result;
}

RandomContext *
RandomContext_Set(DWORD Context)
{
	RandomContext *result = (RandomContext *) HMalloc (sizeof (RandomContext));
	result->seed = Context;
	return result;
}

void
RandomContext_Delete (RandomContext *context)
{
	HFree ((void *) context);
}

RandomContext *
RandomContext_Copy (const RandomContext *source)
{
	RandomContext *result = (RandomContext *) HMalloc (sizeof (RandomContext));
	*result = *source;
	return result;
}

DWORD
RandomContext_Random (RandomContext *context)
{
	context->seed = SeedA * (context->seed % SeedQ) - SeedR * (context->seed / SeedQ);
	if (context->seed > SeedM) {
		context->seed -= SeedM;
	} else if (context->seed == 0)
		context->seed = 1;

	return context->seed;
}

DWORD
RandomContext_SeedRandom (RandomContext *context, DWORD new_seed)
{
	DWORD old_seed;

	/* coerce the seed to be in the range 1..M */
	if (new_seed == 0) /* 0 becomes 1 */
		new_seed = 1;
	else if (new_seed > SeedM) /* and less than M */
		new_seed -= SeedM;

	old_seed = context->seed;
	context->seed = new_seed;
	return old_seed;
}

DWORD
RandomContext_GetSeed (RandomContext *context)
{
	return context->seed;
}
