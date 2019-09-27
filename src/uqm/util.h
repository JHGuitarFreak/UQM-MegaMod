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

#ifndef UQM_UTIL_H_
#define UQM_UTIL_H_

#include "libs/compiler.h"
#include "libs/gfxlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void DrawStarConBox (RECT *pRect, SIZE BorderWidth,
		Color TopLeftColor, Color BottomRightColor, BOOLEAN FillInterior,
		Color InteriorColor);
extern DWORD SeedRandomNumbers (void);

// saveRect can be NULL to save the entire context frame
extern STAMP SaveContextFrame (const RECT *saveRect);

extern DWORD get_fuel_to_sol (void);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_UTIL_H_ */

