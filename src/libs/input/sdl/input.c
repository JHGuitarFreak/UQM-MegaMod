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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include "input.h"
#include "../inpintrn.h"
#include "libs/threadlib.h"
#include "libs/input/sdl/vcontrol.h"
#include "libs/input/sdl/keynames.h"
#include "libs/memlib.h"
#include "libs/file.h"
#include "libs/log.h"
#include "libs/reslib.h"
#include "options.h"


#define KBDBUFSIZE (1 << 8)
static int kbdhead=0, kbdtail=0;
static UniChar kbdbuf[KBDBUFSIZE];
static UniChar lastchar;
static int num_keys = 0;
static int *kbdstate = NULL;
		// Holds all SDL keys +1 for holding invalid values

static volatile int *menu_vec;
static int num_menu;
// The last vector element is the character repeat "key"
#define KEY_MENU_ANY  (num_menu - 1)
static volatile int *flight_vec;
static int num_templ;
static int num_flight;

static BOOLEAN InputInitialized = FALSE;

static BOOLEAN in_character_mode = FALSE;

static const char *menu_res_names[] = {
	"pause",
	"exit",
	"abort",
	"debug",
	"fullscreen",
	"up",
	"down",
	"left",
	"right",
	"select",
	"cancel",
	"special",
	"pageup",
	"pagedown",
	"home",
	"end",
	"zoomin",
	"zoomout",
	"delete",
	"backspace",
	"editcancel",
	"search",
	"next",
	"togglemap", // JMS: For showing SC1-era starmap.
	"debug_2", // JMS: Secondary debug key.
	"debug_3", // JMS: Tertiary debug key.
	"debug_4", // JMS: Quaternary debug key.
	NULL
};

static const char *flight_res_names[] = {
	"up",
	"down",
	"left",
	"right",
	"weapon",
	"special",
	"escape",
#if defined(ANDROID) || defined(__ANDROID__)
	"thrust",
#endif
	NULL
};

static void
register_menu_controls (int index)
{
	int i;
	char buf[40];
	buf[39] = '\0';
	
	i = 1;
	while (TRUE)
	{
		VCONTROL_GESTURE g;
		snprintf (buf, 39, "menu.%s.%d", menu_res_names[index], i);
		if (!res_IsString (buf))
			break;
		VControl_ParseGesture (&g, res_GetString (buf));
		VControl_AddGestureBinding (&g, (int *)&menu_vec[index]);
		i++;
	}
}


static VCONTROL_GESTURE *controls;
#define CONTROL_PTR(i, j, k) \
		(controls + ((i) * num_flight + (j)) * MAX_FLIGHT_ALTERNATES + (k))

static void
register_flight_controls (void)
{
	int i, j, k;
	char buf[40];

	buf[39] = '\0';

	for (i = 0; i < num_templ; i++)
	{
		/* Copy in name */
		snprintf (buf, 39, "keys.%d.name", i+1);
		if (res_IsString (buf))
		{
			strncpy (input_templates[i].name, res_GetString (buf), 29);
			input_templates[i].name[29] = '\0';
		}
		else
		{
			input_templates[i].name[0] = '\0';
		}
		for (j = 0; j < num_flight; j++)
		{
			for (k = 0; k < MAX_FLIGHT_ALTERNATES; k++)
			{
				VCONTROL_GESTURE *g = CONTROL_PTR(i, j, k);
				snprintf (buf, 39, "keys.%d.%s.%d", i+1, flight_res_names[j], k+1);
				if (!res_IsString (buf))
				{
					g->type = VCONTROL_NONE;
					continue;
				}
				VControl_ParseGesture (g, res_GetString (buf));
				VControl_AddGestureBinding (g, (int *)(flight_vec + i * num_flight + j));
			}
		}
	}
}

static void
initKeyConfig (void)
{
	int i;

	if (!menu_vec || !flight_vec)
	{
		log_add (log_Fatal, "initKeyConfig(): invalid input vectors");
		exit (EXIT_FAILURE);
	}

	controls = HCalloc (sizeof (*controls) * num_templ * num_flight
			* MAX_FLIGHT_ALTERNATES);

	/* First, load in the menu keys */
	LoadResourceIndex (contentDir, "menu.key", "menu.");
	LoadResourceIndex (configDir, "override.cfg", "menu.");
	for (i = 0; i < num_menu; i++)
	{
		if (!menu_res_names[i])
			break;
		register_menu_controls (i);
	}
	
	LoadResourceIndex (configDir, "flight.cfg", "keys.");
	if (!res_HasKey ("keys.1.name"))
	{
		/* Either flight.cfg doesn't exist, or we're using an old version
		   of flight.cfg, and thus we wound up loading untyped values into
		   'keys.keys.1.name' and such.  Load the defaults from the content
		   directory. */
		LoadResourceIndex (contentDir, "uqm.key", "keys.");
	}

	register_flight_controls ();

	return;
}

static void
resetKeyboardState (void)
{
	memset (kbdstate, 0, sizeof (int) * num_keys);
	menu_vec[KEY_MENU_ANY] = 0;
}

void
TFB_SetInputVectors (volatile int menu[], int num_menu_, volatile int flight[],
		int num_templ_, int num_flight_)
{
	if (num_menu_ < 0 || num_templ_ < 0 || num_flight_ < 0)
	{
		log_add (log_Fatal, "TFB_SetInputVectors(): invalid vector size");
		exit (EXIT_FAILURE);
	}
	menu_vec = menu;
	num_menu = num_menu_;
	flight_vec = flight;
	num_templ = num_templ_;
	num_flight = num_flight_;
}

#ifdef HAVE_JOYSTICK

static void
initJoystick (void)
{
	int nJoysticks;

	if ((SDL_InitSubSystem(SDL_INIT_JOYSTICK)) == -1)
	{
		log_add (log_Fatal, "Couldn't initialize joystick subsystem: %s",
				SDL_GetError());
		exit (EXIT_FAILURE);
	}

	log_add (log_Info, "%i joysticks were found.", SDL_NumJoysticks ());
	
	nJoysticks = SDL_NumJoysticks ();
	if (nJoysticks > 0)
	{
		int i;

		log_add (log_Info, "The names of the joysticks are:");
		for (i = 0; i < nJoysticks; i++)
		{
			log_add (log_Info, "    %s", SDL_JoystickName (i));
		}
		SDL_JoystickEventState (SDL_ENABLE);
	}
}

#endif /* HAVE_JOYSTICK */

int 
TFB_InitInput (int driver, int flags)
{
	int signed_num_keys; // JMS: New variable to silence warnings
	(void)driver;
	(void)flags;

	SDL_EnableUNICODE(1);
	(void)SDL_GetKeyState (&signed_num_keys);
	num_keys = (unsigned int) signed_num_keys;
	kbdstate = (int *)HMalloc (sizeof (int) * (num_keys + 1));
	

#ifdef HAVE_JOYSTICK
	initJoystick ();
#endif /* HAVE_JOYSTICK */

	in_character_mode = FALSE;
	resetKeyboardState ();

	/* Prepare the Virtual Controller system. */
	VControl_Init ();

	initKeyConfig ();
	
	VControl_ResetInput ();
	InputInitialized = TRUE;

	return 0;
}

void
TFB_UninitInput (void)
{
	VControl_Uninit ();
	HFree (controls);
	HFree (kbdstate);
}

void
EnterCharacterMode (void)
{
	kbdhead = kbdtail = 0;
	lastchar = 0;
	in_character_mode = TRUE;
	VControl_ResetInput ();
}

void
ExitCharacterMode (void)
{
	VControl_ResetInput ();
	in_character_mode = FALSE;
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

UniChar
GetNextCharacter (void)
{
	UniChar result;
	if (kbdhead == kbdtail)
		return 0;
	result = kbdbuf[kbdhead];
	kbdhead = (kbdhead + 1) & (KBDBUFSIZE - 1);
	return result;
}	

UniChar
GetLastCharacter (void)
{
	return lastchar;
}	

volatile int MouseButtonDown = 0;

static void
ProcessMouseEvent (const SDL_Event *e)
{
	switch (e->type)
	{
	case SDL_MOUSEBUTTONDOWN:
		MouseButtonDown = 1;
		break;
	case SDL_MOUSEBUTTONUP:
		MouseButtonDown = 0;
		break;
	default:
		break;
	}
}

static inline int
is_numpad_char_event (const SDL_Event *Event)
{
	return in_character_mode &&
			(Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP) &&
			(Event->key.keysym.mod & KMOD_NUM) &&  /* NumLock is ON */
			Event->key.keysym.unicode > 0 &&       /* Printable char */
			Event->key.keysym.sym >= SDLK_KP0 &&   /* Keypad key */
			Event->key.keysym.sym <= SDLK_KP_PLUS;
}

void
ProcessInputEvent (const SDL_Event *Event)
{
	if (!InputInitialized)
		return;
	
	ProcessMouseEvent (Event);

	// In character mode with NumLock on, numpad chars bypass VControl
	// so that menu arrow events are not produced
	if (!is_numpad_char_event (Event))
		VControl_HandleEvent (Event);

	if (Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP)
	{	// process character input event, if any
		// keysym.sym is an SDLKey type which is an enum and can be signed
		// or unsigned on different platforms; we'll use a guaranteed type
		int k = Event->key.keysym.sym;
		UniChar map_key = Event->key.keysym.unicode;

		if (k < 0 || k > num_keys)
			k = num_keys; // for unknown keys

		if (Event->type == SDL_KEYDOWN)
		{
			int newtail;

			// dont care about the non-printable, non-char
			if (!map_key)
				return;

			kbdstate[k]++;
			
			newtail = (kbdtail + 1) & (KBDBUFSIZE - 1);
			// ignore the char if the buffer is full
			if (newtail != kbdhead)
			{
				kbdbuf[kbdtail] = map_key;
				kbdtail = newtail;
				lastchar = map_key;
				menu_vec[KEY_MENU_ANY]++;
			}
		}
		else if (Event->type == SDL_KEYUP)
		{
			if (kbdstate[k] == 0)
			{	// something is fishy -- better to reset the
				// repeatable state to avoid big problems
				menu_vec[KEY_MENU_ANY] = 0;
			}
			else
			{
				kbdstate[k]--;
				if (menu_vec[KEY_MENU_ANY] > 0)
					menu_vec[KEY_MENU_ANY]--;
			}
		}
	}
}

void
TFB_ResetControls (void)
{
	VControl_ResetInput ();
	resetKeyboardState ();
	// flush character buffer
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

void
InterrogateInputState (int templat, int control, int index, char *buffer, int maxlen)
{
	VCONTROL_GESTURE *g = CONTROL_PTR(templat, control, index);

	if (templat >= num_templ || control >= num_flight
			|| index >= MAX_FLIGHT_ALTERNATES)
	{
		log_add (log_Warning, "InterrogateInputState(): invalid control index");
		buffer[0] = 0;
		return;
	}

	switch (g->type)
	{
	case VCONTROL_KEY:
		snprintf (buffer, maxlen, "%s", VControl_code2name (g->gesture.key));
		buffer[maxlen-1] = 0;
		break;
	case VCONTROL_JOYBUTTON:
		snprintf (buffer, maxlen, "[J%d B%d]", g->gesture.button.port, g->gesture.button.index + 1);
		buffer[maxlen-1] = 0;
		break;
	case VCONTROL_JOYAXIS:
		snprintf (buffer, maxlen, "[J%d A%d %c]", g->gesture.axis.port, g->gesture.axis.index, g->gesture.axis.polarity > 0 ? '+' : '-');
		break;
	case VCONTROL_JOYHAT:
		snprintf (buffer, maxlen, "[J%d H%d %d]", g->gesture.hat.port, g->gesture.hat.index, g->gesture.hat.dir);
		break;
	default:
		/* Something we don't handle yet */
		buffer[0] = 0;
		break;
	}
	return;
}

void
RemoveInputState (int templat, int control, int index)
{
	VCONTROL_GESTURE *g = CONTROL_PTR(templat, control, index);
	char keybuf[40];
	keybuf[39] = '\0';

	if (templat >= num_templ || control >= num_flight
			|| index >= MAX_FLIGHT_ALTERNATES)
	{
		log_add (log_Warning, "RemoveInputState(): invalid control index");
		return;
	}

	VControl_RemoveGestureBinding (g,
			(int *)(flight_vec + templat * num_flight + control));
	g->type = VCONTROL_NONE;

	snprintf (keybuf, 39, "keys.%d.%s.%d", templat+1, flight_res_names[control], index+1);
	res_Remove (keybuf);

	return;
}

void
RebindInputState (int templat, int control, int index)
{
	VCONTROL_GESTURE g;
	char keybuf[40], valbuf[40];
	keybuf[39] = valbuf[39] = '\0';

	if (templat >= num_templ || control >= num_flight
			|| index >= MAX_FLIGHT_ALTERNATES)
	{
		log_add (log_Warning, "RebindInputState(): invalid control index");
		return;
	}

	/* Remove the old binding on this spot */
	RemoveInputState (templat, control, index);

	/* Wait for the next interesting bit of user input */
	VControl_ClearGesture ();
	while (!VControl_GetLastGesture (&g))
	{
		TaskSwitch ();
	}

	/* And now, add the new binding. */
	VControl_AddGestureBinding (&g,
			(int *)(flight_vec + templat * num_flight + control));
	*CONTROL_PTR(templat, control, index) = g;
	snprintf (keybuf, 39, "keys.%d.%s.%d", templat+1, flight_res_names[control], index+1);
	VControl_DumpGesture (valbuf, 39, &g);
	res_PutString (keybuf, valbuf);
}

void
SaveKeyConfiguration (uio_DirHandle *path, const char *fname)
{
	SaveResourceIndex (path, fname, "keys.", TRUE);
}

void
BeginInputFrame (void)
{
	VControl_BeginFrame ();
}

