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

#ifndef LIBS_TIMELIB_H_
#define LIBS_TIMELIB_H_

#define TIMELIB SDL

#include "libs/compiler.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* ONE_SECOND is the LCM of all the fractions of a second the game uses.
 * Battle is 24 FPS, Landers are 35 FPS, most UI-level things are 15 FPS,
 * The Interplanetary flight is 30 FPS, Comm ambient animation is 40 FPS,
 * (also Comm Oscilloscope is 32 FPS, but it does not require a stable
 * timer and currently runs within the Comm ambient anim paradigm anyway)
 * Thus, the minimum value for ONE_SECOND is 840. */
#if TIMELIB == SDL
#	define ONE_SECOND 840
#endif

typedef DWORD TimeCount;
typedef DWORD TimePeriod;

extern void InitTimeSystem (void);
extern void UnInitTimeSystem (void);
extern TimeCount GetTimeCounter (void);

#if defined(__cplusplus)
}
#endif

#endif  /* LIBS_TIMELIB_H_ */

