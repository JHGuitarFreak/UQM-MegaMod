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

#include <stdlib.h>
#include "controls.h"
#include "battlecontrols.h"
#include "init.h"
#include "intel.h"
		// For computer_intelligence
#ifdef NETPLAY
#	include "supermelee/netplay/netmelee.h"
#endif
#include "settings.h"
#include "sounds.h"
#include "tactrans.h"
#include "uqmdebug.h"
#include "libs/async.h"
#include "libs/inplib.h"
#include "libs/timelib.h"
#include "libs/threadlib.h"
#include "libs/input/sdl/vcontrol.h"
#include "setup.h"

// MB: Updated menu delay values so it no longer takes an age to (a) fill up your fuel tanks (b) fill up your crew (c) search through your saved games.
#define ACCELERATION_INCREMENT (ONE_SECOND / 28)
#define MENU_REPEAT_DELAY (ONE_SECOND / 3)

typedef struct
{
	BOOLEAN (*InputFunc) (void *pInputState);
} INPUT_STATE_DESC;

/* These static variables are the values that are set by the controllers. */

typedef struct
{
	DWORD key [NUM_TEMPLATES][NUM_KEYS];
	DWORD menu [NUM_MENU_KEYS];
} MENU_ANNOTATIONS;


CONTROL_TEMPLATE PlayerControls[NUM_PLAYERS];
CONTROLLER_INPUT_STATE CurrentInputState, PulsedInputState;
static CONTROLLER_INPUT_STATE CachedInputState, OldInputState;
static MENU_ANNOTATIONS RepeatDelays, Times;
static DWORD GestaltRepeatDelay, GestaltTime;
static BOOLEAN OldGestalt, CachedGestalt;
static DWORD _max_accel, _min_accel, _step_accel;
static BOOLEAN _gestalt_keys;

static MENU_SOUND_FLAGS sound_0, sound_1;

volatile CONTROLLER_INPUT_STATE ImmediateInputState;

volatile BOOLEAN ExitRequested;
volatile BOOLEAN GamePaused;
volatile BOOLEAN OnScreenKeyboardLocked;

static InputFrameCallback *inputCallback;

static void
_clear_menu_state (void)
{
	int i, j;
	for (i = 0; i < NUM_TEMPLATES; i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			PulsedInputState.key[i][j] = 0;
			CachedInputState.key[i][j] = 0;
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		PulsedInputState.menu[i] = 0;
		CachedInputState.menu[i] = 0;
	}		
	CachedGestalt = FALSE;
}

void
ResetKeyRepeat (void)
{
	DWORD initTime = GetTimeCounter ();
	int i, j;
	for (i = 0; i < NUM_TEMPLATES; i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			RepeatDelays.key[i][j] = _max_accel;
			Times.key[i][j] = initTime;
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		RepeatDelays.menu[i] = _max_accel;
		Times.menu[i] = initTime;
	}
	GestaltRepeatDelay = _max_accel;
	GestaltTime = initTime;
}

static void
_check_for_pulse (int *current, int *cached, int *old, DWORD *accel,
		DWORD *newtime, DWORD *oldtime)
{
	if (*cached && *old)
	{
		if (*newtime - *oldtime < *accel)
		{
			*current = 0;
		}
		else
		{
			*current = *cached;
			if (*accel > _min_accel)
				*accel -= _step_accel;
			if (*accel < _min_accel)
				*accel = _min_accel;
			*oldtime = *newtime;
		}
	}
	else
	{
		*current = *cached;
		*oldtime = *newtime;
		*accel = _max_accel;
	}
}

/* BUG: If a key from a currently unused control template is held,
 * this will affect the gestalt repeat rate.  This isn't a problem
 * *yet*, but it will be once the user gets to define control
 * templates on his own --McM */
static void
_check_gestalt (DWORD NewTime)
{
	BOOLEAN CurrentGestalt;
	int i, j;
	OldGestalt = CachedGestalt;

	CachedGestalt = 0;
	CurrentGestalt = 0;
	for (i = 0; i < NUM_TEMPLATES; i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			CachedGestalt |= ImmediateInputState.key[i][j];
			CurrentGestalt |= PulsedInputState.key[i][j];
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		CachedGestalt |= ImmediateInputState.menu[i];
		CurrentGestalt |= PulsedInputState.menu[i];
	}

	if (OldGestalt && CachedGestalt)
	{
		if (NewTime - GestaltTime < GestaltRepeatDelay)
		{
			for (i = 0; i < NUM_TEMPLATES; i++)
			{
				for (j = 0; j < NUM_KEYS; j++)
				{
					PulsedInputState.key[i][j] = 0;
				}
			}
			for (i = 0; i < NUM_MENU_KEYS; i++)
			{
				PulsedInputState.menu[i] = 0;
			}
		}
		else
		{
			for (i = 0; i < NUM_TEMPLATES; i++)
			{
				for (j = 0; j < NUM_KEYS; j++)
				{
					PulsedInputState.key[i][j] = CachedInputState.key[i][j];
				}
			}
			for (i = 0; i < NUM_MENU_KEYS; i++)
			{
				PulsedInputState.menu[i] = CachedInputState.menu[i];
			}
			if (GestaltRepeatDelay > _min_accel)
				GestaltRepeatDelay -= _step_accel;
			if (GestaltRepeatDelay < _min_accel)
				GestaltRepeatDelay = _min_accel;
			GestaltTime = NewTime;
		}
	}
	else
	{
		for (i = 0; i < NUM_TEMPLATES; i++)
		{
			for (j = 0; j < NUM_KEYS; j++)
			{
				PulsedInputState.key[i][j] = CachedInputState.key[i][j];
			}
		}
		for (i = 0; i < NUM_MENU_KEYS; i++)
		{
			PulsedInputState.menu[i] = CachedInputState.menu[i];
		}
		GestaltTime = NewTime;
		GestaltRepeatDelay = _max_accel;
	}
}

void
UpdateInputState (void)
{
	DWORD NewTime;
	/* First, if the game is, in fact, paused, we stall until
	 * unpaused.  Every thread with control over game logic calls
	 * UpdateInputState routinely, so we handle pause and exit
	 * state updates here. */

	// Automatically pause and enter low-activity state while inactive,
	// for example, window minimized.
	if (!GameActive)
		SleepGame ();

	if (GamePaused)
		PauseGame ();

	if (ExitRequested)
		ConfirmExit ();

	CurrentInputState = ImmediateInputState;
	OldInputState = CachedInputState;
	CachedInputState = ImmediateInputState;
	BeginInputFrame ();
	NewTime = GetTimeCounter ();
	if (_gestalt_keys)
	{
		_check_gestalt (NewTime);
	}
	else
	{
		int i, j;
		for (i = 0; i < NUM_TEMPLATES; i++)
		{
			for (j = 0; j < NUM_KEYS; j++)
			{
				_check_for_pulse (&PulsedInputState.key[i][j],
						&CachedInputState.key[i][j], &OldInputState.key[i][j],
						&RepeatDelays.key[i][j], &NewTime, &Times.key[i][j]);
			}
		}
		for (i = 0; i < NUM_MENU_KEYS; i++)
		{
			_check_for_pulse (&PulsedInputState.menu[i],
					&CachedInputState.menu[i], &OldInputState.menu[i],
					&RepeatDelays.menu[i], &NewTime, &Times.menu[i]);
		}
	}

	if (CurrentInputState.menu[KEY_PAUSE])
		GamePaused = TRUE;

	if (CurrentInputState.menu[KEY_EXIT])
		ExitRequested = TRUE;

#if defined(DEBUG) || defined(USE_DEBUG_KEY)
	if (PulsedInputState.menu[KEY_DEBUG])
		debugKeyPressedSynchronous ();
#endif
}

InputFrameCallback *
SetInputCallback (InputFrameCallback *callback)
{
	InputFrameCallback *old = inputCallback;
	
	// Replacing an existing callback with another is not a problem,
	// but currently this should never happen, which is why the assert.
	assert (!old || !callback);
	inputCallback = callback;

	return old;
}

void
SetMenuRepeatDelay (DWORD min, DWORD max, DWORD step, BOOLEAN gestalt)
{
	_min_accel = min;
	_max_accel = max;
	_step_accel = step;
	_gestalt_keys = gestalt;
	//_clear_menu_state ();
	ResetKeyRepeat ();
}

void
SetDefaultMenuRepeatDelay (void)
{
	_min_accel = ACCELERATION_INCREMENT;
	_max_accel = MENU_REPEAT_DELAY;
	_step_accel = ACCELERATION_INCREMENT;
	_gestalt_keys = FALSE;
	//_clear_menu_state ();
	ResetKeyRepeat ();
}

void
FlushInput (void)
{
	TFB_ResetControls ();
	_clear_menu_state ();
}

static MENU_SOUND_FLAGS
MenuKeysToSoundFlags (const CONTROLLER_INPUT_STATE *state)
{
	MENU_SOUND_FLAGS soundFlags;

	soundFlags = MENU_SOUND_NONE;
	if (state->menu[KEY_MENU_UP])
		soundFlags |= MENU_SOUND_UP;
	if (state->menu[KEY_MENU_DOWN])
		soundFlags |= MENU_SOUND_DOWN;
	if (state->menu[KEY_MENU_LEFT])
		soundFlags |= MENU_SOUND_LEFT;
	if (state->menu[KEY_MENU_RIGHT])
		soundFlags |= MENU_SOUND_RIGHT;
	if (state->menu[KEY_MENU_SELECT])
		soundFlags |= MENU_SOUND_SELECT;
	if (state->menu[KEY_MENU_CANCEL])
		soundFlags |= MENU_SOUND_CANCEL;
	if (state->menu[KEY_MENU_SPECIAL])
		soundFlags |= MENU_SOUND_SPECIAL;
	if (state->menu[KEY_MENU_PAGE_UP])
		soundFlags |= MENU_SOUND_PAGEUP;
	if (state->menu[KEY_MENU_PAGE_DOWN])
		soundFlags |= MENU_SOUND_PAGEDOWN;
	if (state->menu[KEY_MENU_DELETE])
		soundFlags |= MENU_SOUND_DELETE;
	if (state->menu[KEY_MENU_BACKSPACE])
		soundFlags |= MENU_SOUND_DELETE;
	
	return soundFlags;
}

void
DoInput (void *pInputState, BOOLEAN resetInput)
{
	if (resetInput)
		FlushInput ();

	do
	{
		MENU_SOUND_FLAGS soundFlags;
		Async_process ();
		TaskSwitch ();

		UpdateInputState ();

#if DEMO_MODE || CREATE_JOURNAL
		if (ArrowInput != DemoInput)
#endif
		{
#if CREATE_JOURNAL
			JournalInput (InputState);
#endif /* CREATE_JOURNAL */
		}

		soundFlags = MenuKeysToSoundFlags (&PulsedInputState);
			
		if (MenuSounds && (soundFlags & (sound_0 | sound_1)))
		{
			SOUND S;

			S = MenuSounds;
			if (soundFlags & sound_1)
				S = SetAbsSoundIndex (S, MENU_SOUND_SUCCESS);

			PlaySoundEffect (S, 0, NotPositional (), NULL, 0);
		}

		if (inputCallback)
			inputCallback ();

	} while (((INPUT_STATE_DESC*)pInputState)->InputFunc (pInputState));

	if (resetInput)
		FlushInput ();
}

void
SetMenuSounds (MENU_SOUND_FLAGS s0, MENU_SOUND_FLAGS s1)
{
	sound_0 = s0;
	sound_1 = s1;
}

void
GetMenuSounds (MENU_SOUND_FLAGS *s0, MENU_SOUND_FLAGS *s1)
{
	*s0 = sound_0;
	*s1 = sound_1;
}


static BATTLE_INPUT_STATE
ControlInputToBattleInput (const int *keyState, COUNT player, int direction)
{
	BATTLE_INPUT_STATE InputState = 0;

	if (keyState[KEY_WEAPON]){
		if (antiCheatAlt()) {
			resetEnergyBattle();
		}
		InputState |= BATTLE_WEAPON;
	}
	if (keyState[KEY_SPECIAL]){
		if (antiCheatAlt()) {
			resetEnergyBattle();
		}
		InputState |= BATTLE_SPECIAL;
	}
	if (keyState[KEY_ESCAPE])
		InputState |= BATTLE_ESCAPE;
	if (keyState[KEY_DOWN])
		InputState |= BATTLE_DOWN;

	if (direction < 0) {
		if (keyState[KEY_UP])
			InputState |= BATTLE_THRUST;
		if (keyState[KEY_LEFT])
			InputState |= BATTLE_LEFT;
		if (keyState[KEY_RIGHT])
			InputState |= BATTLE_RIGHT;
	} 
#if defined(ANDROID) || defined(__ANDROID__)	
	else {
		InputState |= GetDirectionalJoystickInput(direction, player);
	}
#endif

	return InputState;
}

BATTLE_INPUT_STATE
CurrentInputToBattleInput(COUNT player, int direction)
{
	return ControlInputToBattleInput(
		CurrentInputState.key[PlayerControls[player]], player, direction);
}

BATTLE_INPUT_STATE
PulsedInputToBattleInput (COUNT player)
{
	return ControlInputToBattleInput(
		PulsedInputState.key[PlayerControls[player]], player, -1);
}

BOOLEAN
AnyButtonPress (BOOLEAN CheckSpecial)
{
	int i, j;
	(void) CheckSpecial;   // Ignored
	UpdateInputState ();
	for (i = 0; i < NUM_TEMPLATES; i++)
	{
		for (j = 0; j < NUM_KEYS; j++)
		{
			if (CurrentInputState.key[i][j])
				return TRUE;
		}
	}
	for (i = 0; i < NUM_MENU_KEYS; i++)
	{
		if (CurrentInputState.menu[i])
			return TRUE;
	}
	return FALSE;
}

BOOLEAN
ConfirmExit (void)
{
	DWORD old_max_accel, old_min_accel, old_step_accel;
	BOOLEAN old_gestalt_keys, result;

	old_max_accel = _max_accel;
	old_min_accel = _min_accel;
	old_step_accel = _step_accel;
	old_gestalt_keys = _gestalt_keys;
		
	SetDefaultMenuRepeatDelay ();
		
	result = DoConfirmExit ();
	
	SetMenuRepeatDelay (old_min_accel, old_max_accel, old_step_accel,
			old_gestalt_keys);
	return result;
}

#if defined(ANDROID) || defined(__ANDROID__)
// Fast arctan2, returns angle in radians as integer, with fractional part in lower 16 bits
// Stolen from http://www.dspguru.com/dsp/tricks/fixed-point-atan2-with-self-normalization , precision is said to be 0.07 rads

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
enum { atan2i_coeff_1 = ((int)(M_PI*65536.0 / 4)), atan2i_coeff_2 = (3 * atan2i_coeff_1), atan2i_PI = (int)(M_PI * 65536.0), SHIP_DIRECTIONS = 16 };

static inline int 
atan2i(int y, int x) {
	int angle;
	int abs_y = abs(y);

	if (abs_y == 0)
		abs_y = 1;
	if (x >= 0) {
		angle = atan2i_coeff_1 - atan2i_coeff_1 * (x - abs_y) / (x + abs_y);
	} else {
		angle = atan2i_coeff_2 - atan2i_coeff_1 * (x + abs_y) / (abs_y - x);
	}
	if (y < 0)
		return(-angle);     // negate if in quad III or IV
	else
		return(angle);
}

BATTLE_INPUT_STATE 
GetDirectionalJoystickInput(int direction, int player) {
	BATTLE_INPUT_STATE InputState = 0;
	static BOOLEAN JoystickThrust[NUM_PLAYERS] = { FALSE, FALSE };
	static BOOLEAN JoystickTapFlag[NUM_PLAYERS] = { FALSE, FALSE };
	static TimeCount JoystickTapTime[NUM_PLAYERS] = { 0, 0 };
	int axisX, axisY;

	if (CurrentInputState.key[PlayerControls[player]][KEY_THRUST])
		InputState |= BATTLE_THRUST;

	if (VControl_GetJoysticksAmount() <= 0) {
		if (CurrentInputState.key[PlayerControls[player]][KEY_LEFT])
			InputState |= BATTLE_LEFT;
		if (CurrentInputState.key[PlayerControls[player]][KEY_RIGHT])
			InputState |= BATTLE_RIGHT;
		if (CurrentInputState.key[PlayerControls[player]][KEY_UP])
			InputState |= BATTLE_THRUST;
		return InputState;
	}

	axisX = VControl_GetJoyAxis(0, player * 2);
	axisY = VControl_GetJoyAxis(0, player * 2 + 1);

	if (axisX == 0 && axisY == 0) {
		// Some basic gamepad input support
		axisX = VControl_GetJoyAxis(2, player * 2);
		axisY = VControl_GetJoyAxis(2, player * 2 + 1);
		if (abs(axisX) > 5000 || abs(axisY) > 5000) { // Deadspot at the center
			JoystickTapFlag[player] = TRUE;
			JoystickThrust[player] = FALSE;
			// Turning thrust with joystick is uncomfortable
			//if( abs( axisX ) > 25000 || abs( axisY ) > 25000 )
			//	JoystickThrust[player] = TRUE;
		} else {
			axisX = 0;
			axisY = 0;
		}
	}

	if (axisX == 0 && axisY == 0) {
		// Process keyboard input only when joystick is not used
		if (CurrentInputState.key[PlayerControls[player]][KEY_LEFT])
			InputState |= BATTLE_LEFT;
		if (CurrentInputState.key[PlayerControls[player]][KEY_RIGHT])
			InputState |= BATTLE_RIGHT;
		if (CurrentInputState.key[PlayerControls[player]][KEY_UP])
			InputState |= BATTLE_THRUST;
	}

	if (!optDirectionalJoystick) {
		if (player == 1) {
			axisX = -axisX;
			axisY = -axisY;
		}
		if (axisX < -10000)
			InputState |= BATTLE_LEFT;
		if (axisX > 10000)
			InputState |= BATTLE_RIGHT;
		if (axisY < 0)
			InputState |= BATTLE_THRUST;
		return InputState;
	}

	if (axisX != 0 || axisY != 0) {
		int angle = atan2i(axisY, axisX), diff;
		// Convert it to 16 directions used by Melee
		angle += atan2i_PI / SHIP_DIRECTIONS;
		if (angle < 0)
			angle += atan2i_PI * 2;
		if (angle > atan2i_PI * 2)
			angle -= atan2i_PI * 2;
		angle = angle * SHIP_DIRECTIONS / atan2i_PI / 2;

		diff = angle - direction - SHIP_DIRECTIONS / 4;
		while (diff >= SHIP_DIRECTIONS)
			diff -= SHIP_DIRECTIONS;
		while (diff < 0)
			diff += SHIP_DIRECTIONS;

		if (diff < SHIP_DIRECTIONS / 2)
			InputState |= BATTLE_LEFT;
		if (diff > SHIP_DIRECTIONS / 2)
			InputState |= BATTLE_RIGHT;

		if (!JoystickTapFlag[player]) {
			JoystickTapFlag[player] = TRUE;
			if (GetTimeCounter() < JoystickTapTime[player] + ONE_SECOND)
				JoystickThrust[player] = !JoystickThrust[player];
			else
				JoystickThrust[player] = TRUE;
		}
		if (JoystickThrust[player])
			InputState |= BATTLE_THRUST;
	} else {
		if (JoystickTapFlag[player]) {
			JoystickTapFlag[player] = FALSE;
			JoystickTapTime[player] = GetTimeCounter();
		}
	}
	return InputState;
}
#endif