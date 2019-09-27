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

#ifndef UQM_INIT_H_
#define UQM_INIT_H_

#include "libs/gfxlib.h"
#include "libs/reslib.h"
#include "units.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define NUM_PLAYERS 2
#define NUM_SIDES 2

extern FRAME stars_in_space;
extern FRAME StarPoints;
extern FRAME stars_in_quasispace; // JMS_GFX
extern FRAME crew_dots[NUM_VIEWS]; // JMS_GFX
extern FRAME ion_trails[NUM_VIEWS]; // JMS_GFX

extern BOOLEAN InitSpace (void);
extern void UninitSpace (void);

extern SIZE InitShips (void);
extern void UninitShips (void);

extern BOOLEAN load_animation (FRAME *pixarray, RESOURCE big_res,
		RESOURCE med_res, RESOURCE sml_res);
extern BOOLEAN free_image (FRAME *pixarray);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_INIT_H_ */

