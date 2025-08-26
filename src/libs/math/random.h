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
 
/****************************************************************************
* FILE: random.h
* DESC: definitions and externs for random number generators
*
* HISTORY: Created 6/ 6/1989
* LAST CHANGED:
*
* Copyright (c) 1989, Robert Leyland and Scott Anderson
****************************************************************************/

#ifndef LIBS_MATH_RANDOM_H_
#define LIBS_MATH_RANDOM_H_

#include "../../uqm/globdata.h"
#include "uqm/setupmenu.h"

/* ----------------------------GLOBALS/EXTERNS---------------------------- */

DWORD TFB_SeedRandom (DWORD seed);
DWORD TFB_Random (void);


typedef struct RandomContext RandomContext;

#ifdef RANDOM2_INTERNAL
struct RandomContext {
	DWORD seed;
};
#endif

// The Planet Generation seeding code alters SeedA to function.
// All other modes use proper PrimeA == SeedA == 16807
#define PrimeA 16807
#define MAX_SEED 2147483644
#define MIN_SEED 3
#define SANE_SEED(a) (((a) < MIN_SEED || (a) > MAX_SEED) ? false : true)
#define SeedA (((optSeedType == OPTVAL_PLANET) && \
		(SANE_SEED (GLOBAL_SIS (Seed)))) ? GLOBAL_SIS (Seed) : PrimeA)
// Default SeedA (PrimeA): 16807 - a relatively prime number - also M div Q
#define SeedM (UINT32_MAX / 2) // 0xFFFFFFFF div 2
#define SeedQ (SeedM / SeedA)  // Default: 127773L - M div A
#define SeedR (SeedM % SeedA)  // Default: 2836 - M mod A 
#define PrimeSeed (optSeedType == OPTVAL_PRIME)
#define StarSeed (optSeedType > OPTVAL_PLANET)

static inline UNICODE *
SeedStr (void)
{
	switch (optSeedType)
	{
		case 0:
			return "None";
		case 1:
			return "Planet";
		case 2:
			return "MRQ";
		case 3:
			return "StarSeed";
		default:
			return "???";
	}
}

RandomContext *RandomContext_New (void);
RandomContext *RandomContext_Set(DWORD Context);
void RandomContext_Delete (RandomContext *context);
RandomContext *RandomContext_Copy (const RandomContext *source);
DWORD RandomContext_Random (RandomContext *context);
DWORD RandomContext_SeedRandom (RandomContext *context, DWORD new_seed);
DWORD RandomContext_GetSeed (RandomContext *context);
DWORD RandomContext_FastRandom (DWORD seed);
int RangeMinMax (int min, int max, DWORD rand);


#endif  /* LIBS_MATH_RANDOM_H_ */


