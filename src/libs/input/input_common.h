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

#ifndef INPUT_COMMON_H
#define INPUT_COMMON_H

// driver for TFB_InitInput
enum
{
	TFB_INPUTDRIVER_SDL
};

// flags for TFB_InitInput
//#define TFB_INPUTFLAGS_ETC (1<<0)

extern int TFB_InitInput (int driver, int flags);
extern void TFB_UninitInput (void);

#if defined(ANDROID) || defined(__ANDROID__)
#define MAX_FLIGHT_ALTERNATES  3
#else
#define MAX_FLIGHT_ALTERNATES 2
#endif

extern void TFB_SetInputVectors (volatile int menu[], int num_menu,
		volatile int flight[], int num_templ, int num_flight);

#endif
