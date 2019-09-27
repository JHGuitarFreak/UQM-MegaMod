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

#ifndef UQM_PICKSHIP_H_INCL_
#define UQM_PICKSHIP_H_INCL_

#include "libs/compiler.h"
#include "races.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern HSTARSHIP GetEncounterStarShip (STARSHIP *LastStarShipPtr,
		COUNT which_player);
extern void DrawArmadaPickShip (BOOLEAN draw_salvage_frame, RECT *pPickRect);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_PICKSHIP_H_INCL_ */
