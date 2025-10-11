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

#ifndef LIBS_INPUT_SDL_VCONTROL_H_
#define LIBS_INPUT_SDL_VCONTROL_H_

#include "port.h"
#include SDL_INCLUDE(SDL.h)

#if SDL_MAJOR_VERSION == 1
typedef SDLKey sdl_key_t;
#else
typedef SDL_Keycode sdl_key_t;
#endif

/* Initialization routines */
void VControl_Init (void);
void VControl_Uninit (void);

/* Structures for representing actual VControl Inputs.  Returned by
   iterators and used to construct bindings. */

typedef enum {
	VCONTROL_NONE,
	VCONTROL_KEY,
	VCONTROL_JOYAXIS,
	VCONTROL_JOYBUTTON,
	VCONTROL_JOYHAT,
	NUM_VCONTROL_GESTURES
} VCONTROL_GESTURE_TYPE;

typedef struct {
	VCONTROL_GESTURE_TYPE type;
	union {
		sdl_key_t key;
		struct { int port, index, polarity; } axis;
		struct { int port, index; } button;
		struct { int port, index; Uint8 dir; } hat;
	} gesture;
} VCONTROL_GESTURE;			

/* Control of bindings */
int  VControl_AddGestureBinding (VCONTROL_GESTURE *g, int *target);
void VControl_RemoveGestureBinding (VCONTROL_GESTURE *g, int *target);

int  VControl_AddKeyBinding (sdl_key_t symbol, int *target);
void VControl_RemoveKeyBinding (sdl_key_t symbol, int *target);
int  VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target);
void VControl_RemoveJoyAxisBinding (int port, int axis, int polarity, int *target);
int  VControl_SetJoyThreshold (int port, int threshold);
int  VControl_AddJoyButtonBinding (int port, int button, int *target);
void VControl_RemoveJoyButtonBinding (int port, int button, int *target);
int  VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target);
void VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target);

void VControl_RemoveAllBindings (void);

/* Signal to VControl that a frame is about to begin. */
void VControl_BeginFrame (void);

/* The listener.  Routines besides HandleEvent may be used to 'fake' inputs without 
 * fabricating an SDL_Event. 
 */
void VControl_HandleEvent (const SDL_Event *e);
void VControl_ProcessKeyDown (sdl_key_t symbol);
void VControl_ProcessKeyUp (sdl_key_t symbol);
void VControl_ProcessJoyButtonDown (int port, int button);
void VControl_ProcessJoyButtonUp (int port, int button);
void VControl_ProcessJoyAxis (int port, int axis, int value);
void VControl_ProcessJoyHat (int port, int which, Uint8 value);

/* Force the input into the blank state.  For preventing "sticky" keys. */
void VControl_ResetInput (void);

/* Translate between gestures and string representations thereof. */
void VControl_ParseGesture (VCONTROL_GESTURE *g, const char *spec);
int VControl_DumpGesture (char *buf, int n, VCONTROL_GESTURE *g);

/* Tracking the "last interesting gesture."  Used to poll to find new
   control keys. */

void VControl_ClearGesture (void);
int  VControl_GetLastGesture (VCONTROL_GESTURE *g);

/* Constants for handling the "Start bit."  If a gesture is made, and
 * then ends, within a single frame, it will still, for one frame,
 * have a nonzero value.  This is because Bit 16 will be on for the
 * first frame a gesture is struck.  This bit is cleared when
 * VControl_BeginFrame() is called.  These constants are used to mask
 * out results if necessary. */

#define VCONTROL_STARTBIT 0x10000
#define VCONTROL_MASK     0x0FFFF

#endif
