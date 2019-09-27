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

#ifndef UQM_IPDISP_H_INCL_
#define UQM_IPDISP_H_INCL_

#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

extern void NotifyOthers (COUNT which_race, BYTE target_loc);
// Special target locations for NotifyOthers()
#define IPNL_INTERCEPT_PLAYER   0
#define IPNL_ALL_CLEAR          ((BYTE)-1)

extern void DoMissions (void);

#if defined(__cplusplus)
}
#endif

#endif  /* UQM_IPDISP_H_INCL_ */
