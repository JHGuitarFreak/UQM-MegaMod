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

#ifndef CONS_RES_H_
#define CONS_RES_H_

#include "libs/gfxlib.h"
#include "libs/sndlib.h"

#if defined(__cplusplus)
extern "C" {
#endif

void load_gravity_well (BYTE selector);
void free_gravity_well (void);

FRAME load_life_form (BYTE selector);

#define NUM_ORBIT_THEMES 5
static MUSIC_REF OrbitMusic[NUM_ORBIT_THEMES];
MUSIC_REF load_orbit_theme (BYTE selector);

#define NUM_MM_THEMES 3
MUSIC_REF loadMainMenuMusic (BYTE selector);

#if defined(__cplusplus)
}
#endif

#endif /* CONS_RES_H_ */
