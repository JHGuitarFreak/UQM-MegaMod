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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef STARMAP_H_INCL_
#define STARMAP_H_INCL_

#include "libs/compiler.h"
#include "planets/planets.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern STAR_DESC *CurStarDescPtr;
extern STAR_DESC *star_array;

#define NUM_SOLAR_SYSTEMS 502

extern STAR_DESC* FindStar (STAR_DESC *pLastStar, POINT *puniverse,
		SIZE xbounds, SIZE ybounds);

extern void GetClusterName (const STAR_DESC *pSD, UNICODE buf[]);

#if defined(__cplusplus)
}
#endif

#endif  /* STARMAP_H_INCL_ */

