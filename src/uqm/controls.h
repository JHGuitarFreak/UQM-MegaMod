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

#ifndef UQM_CONTROLS_H_
#define UQM_CONTROLS_H_

#include "libs/compiler.h"
#include "libs/strlib.h"
#include "libs/timelib.h"

#if defined(__cplusplus)
extern "C" {
#endif

// Enumerated type for controls
enum {
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_WEAPON,
	KEY_SPECIAL,
	KEY_ESCAPE,
#if defined(ANDROID) || defined(__ANDROID__)
	KEY_THRUST,
#endif
	NUM_KEYS
};
enum {
	KEY_PAUSE,
	KEY_EXIT,
	KEY_ABORT,
	KEY_DEBUG,
	KEY_FULLSCREEN,
	KEY_MENU_UP,
	KEY_MENU_DOWN,
	KEY_MENU_LEFT,
	KEY_MENU_RIGHT,
	KEY_MENU_SELECT,
	KEY_MENU_CANCEL,
	KEY_MENU_SPECIAL,
	KEY_MENU_PAGE_UP,
	KEY_MENU_PAGE_DOWN,
	KEY_MENU_HOME,
	KEY_MENU_END,
	KEY_MENU_ZOOM_IN,
	KEY_MENU_ZOOM_OUT,
	KEY_MENU_DELETE,
	KEY_MENU_BACKSPACE,
	KEY_MENU_EDIT_CANCEL,
	KEY_MENU_SEARCH,
	KEY_MENU_NEXT,
	KEY_MENU_TOGGLEMAP, // JMS: For showing SC1-era starmap
	KEY_MENU_ANY, /* abstract char key */
	KEY_DEBUG_2,  // JMS: Secondary debug key
	KEY_DEBUG_3,  // JMS: Tertiary debug key
	KEY_DEBUG_4,  // JMS: Quaternary debug key
	NUM_MENU_KEYS
};

typedef enum {
	CONTROL_TEMPLATE_KB_1,
	CONTROL_TEMPLATE_KB_2,
	CONTROL_TEMPLATE_KB_3,
	CONTROL_TEMPLATE_JOY_1,
	CONTROL_TEMPLATE_JOY_2,
	CONTROL_TEMPLATE_JOY_3,
	NUM_TEMPLATES
} CONTROL_TEMPLATE;

typedef struct _controller_input_state {
	int key[NUM_TEMPLATES][NUM_KEYS];
	int menu[NUM_MENU_KEYS];
} CONTROLLER_INPUT_STATE;

typedef UBYTE BATTLE_INPUT_STATE;
#define BATTLE_LEFT       ((BATTLE_INPUT_STATE)(1 << 0))
#define BATTLE_RIGHT      ((BATTLE_INPUT_STATE)(1 << 1))
#define BATTLE_THRUST     ((BATTLE_INPUT_STATE)(1 << 2))
#define BATTLE_WEAPON     ((BATTLE_INPUT_STATE)(1 << 3))
#define BATTLE_SPECIAL    ((BATTLE_INPUT_STATE)(1 << 4))
#define BATTLE_ESCAPE     ((BATTLE_INPUT_STATE)(1 << 5))
#define BATTLE_DOWN       ((BATTLE_INPUT_STATE)(1 << 6))

BATTLE_INPUT_STATE CurrentInputToBattleInput (COUNT player, int direction); // direction = -1 for no directional input);
BATTLE_INPUT_STATE PulsedInputToBattleInput (COUNT player);

extern CONTROLLER_INPUT_STATE CurrentInputState;
extern CONTROLLER_INPUT_STATE PulsedInputState;
extern volatile CONTROLLER_INPUT_STATE ImmediateInputState;
extern CONTROL_TEMPLATE PlayerControls[];
extern BOOLEAN WarpFromMenu;

void UpdateInputState (void);
extern void FlushInput (void);
void SetMenuRepeatDelay (DWORD min, DWORD max, DWORD step, BOOLEAN gestalt);
void SetDefaultMenuRepeatDelay (void);
void ResetKeyRepeat (void);
BOOLEAN PauseGame (void);
void SleepGame (void);
BOOLEAN DoConfirmExit (void);
BOOLEAN ConfirmExit (void);

#define WAIT_INFINITE ((TimePeriod)-1)
BOOLEAN WaitForAnyButton (BOOLEAN newButton, TimePeriod duration,
		BOOLEAN resetInput);
BOOLEAN WaitForAnyButtonUntil (BOOLEAN newButton, TimeCount timeOut,
		BOOLEAN resetInput);
BOOLEAN WaitForNoInput (TimePeriod duration, BOOLEAN resetInput);
BOOLEAN WaitForNoInputUntil (TimeCount timeOut, BOOLEAN resetInput);

extern BATTLE_INPUT_STATE GetDirectionalJoystickInput(int direction, int player);

void DoPopupWindow(const char *msg);

typedef void (InputFrameCallback) (void);
InputFrameCallback* SetInputCallback (InputFrameCallback *);
// pInputState must point to a struct derived from INPUT_STATE_DESC
void DoInput (void *pInputState, BOOLEAN resetInput);

extern volatile BOOLEAN GamePaused;
extern volatile BOOLEAN ExitRequested;

typedef struct joy_char joy_char_t;

typedef struct textentry_state
{
	// standard state required by DoInput
	BOOLEAN (*InputFunc) (struct textentry_state *pTES);

	// these are semi-private read-only
	BOOLEAN Initialized;
	DWORD NextTime;    // use this for input frame timing
	BOOLEAN Success;   // edit confirmed or canceled
	UNICODE *CacheStr; // cached copy to revert immediate changes
	STRING JoyAlphaString; // joystick alphabet definition
	BOOLEAN JoystickMode;  // TRUE when doing joystick input
	BOOLEAN UpperRegister; // TRUE when entering Caps
	joy_char_t *JoyAlpha;  // joystick alphabet
	int JoyAlphaLength;
	joy_char_t *JoyUpper;  // joystick upper register
	joy_char_t *JoyLower;  // joystick lower register
	int JoyRegLength;
	UNICODE *InsPt;        // set to current pos of insertion point
	// these are public and must be set before calling DoTextEntry
	UNICODE *BaseStr;  // set to string to edit
	int CursorPos;     // set to current cursor pos in chars
	int MaxSize;       // set to max size of edited string

	BOOLEAN (*ChangeCallback) (struct textentry_state *pTES);
			// returns TRUE if last change is OK
	BOOLEAN (*FrameCallback) (struct textentry_state *pTES);
			// called on every input frame; do whatever;
			// returns TRUE to continue processing
	void *CbParam;     // callback parameter, use as you like
	
} TEXTENTRY_STATE;

extern BOOLEAN DoTextEntry (TEXTENTRY_STATE *pTES);

#if defined(__cplusplus)
}
#endif

#endif


