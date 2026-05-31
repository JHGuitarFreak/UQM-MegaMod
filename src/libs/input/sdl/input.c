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

static BOOLEAN set_character_mode = FALSE;
		// Records whether the UI thread has caught up with game thread
		// on this setting

volatile int *menu_vec;
static int num_menu;
// The last vector element is the character repeat "key"
// This is only used in SDL1 input but it's mostly harmless everywhere else
#define KEY_MENU_ANY  (num_menu - 1)
static volatile int *flight_vec;
static int num_templ;
static int num_flight;

static BOOLEAN InputInitialized = FALSE;

static BOOLEAN in_character_mode = FALSE;

MENU_BINDINGS curr_bindings[NUM_MENU_KEYS];
MENU_BINDINGS def_bindings[NUM_MENU_KEYS];

const char *menu_res_names[] = {
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
	"togglemap",
	"screenshot",
	"quicksave",
	"quickload",
	"debug_2",
	"debug_3",
	"debug_4",
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
	"thrust",
	NULL
};

static void
register_menu_controls (int index)
{
	int i;
	char buf[40];
	buf[39] = '\0';
	
	i = 1;

	for (i = 1; i <= 6; i++)
	{
		VCONTROL_GESTURE g;

		snprintf (buf, sizeof (buf), "menu.%s.%d", menu_res_names[index], i);

		if (!res_IsString (buf))
			continue;

		VControl_ParseGesture (&g, res_GetString (buf));
		VControl_AddGestureBinding (&g, (int *)&menu_vec[index]);
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
		for (j = 0; j < num_flight; j++)
		{
			for (k = 0; k < MAX_FLIGHT_ALTERNATES; k++)
			{
				VCONTROL_GESTURE *g = CONTROL_PTR(i, j, k);
				snprintf (buf, 39, "keys.%d.%s.%d", i+1,
						flight_res_names[j], k+1);
				if (!res_IsString (buf))
				{
					g->type = VCONTROL_NONE;
					continue;
				}
				VControl_ParseGesture (g, res_GetString (buf));
				VControl_AddGestureBinding (g,
						(int *)(flight_vec + i * num_flight + j));
			}
		}
	}
}

static void
GetMenuBindings (MENU_BINDINGS *bindings)
{
	int i, j;
	char buf[40];

	for (i = 0; menu_res_names[i] != NULL; i++)
	{
		snprintf (bindings[i].action,
			sizeof (bindings[i].action), "%s", menu_res_names[i]);

		for (j = 1; j <= 6; j++)
		{
			snprintf (buf, sizeof (buf), "menu.%s.%d",
				menu_res_names[i], j);

			if (res_IsString (buf))
			{
				VControl_ParseGesture (&bindings[i].binding[j - 1],
					res_GetString (buf));
			}
			else
			{
				bindings[i].binding[j - 1].type = VCONTROL_NONE;
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
	GetMenuBindings (def_bindings);

	LoadResourceIndex (configDir, "override.cfg", "menu.");
	for (i = 0; i < num_menu; i++)
	{
		if (!menu_res_names[i])
			break;
		register_menu_controls (i);
	}
	GetMenuBindings (curr_bindings);

	LoadResourceIndex (configDir, "flight.cfg", "keys.");
	if (!res_HasKey ("keys.version"))
	{
		/* Either flight.cfg doesn't exist, or we're using an old version
		   of flight.cfg, and thus we wound up loading untyped values into
		   'keys.keys.version' and such.  Load the defaults from the
		   content directory. */
		LoadResourceIndex (contentDir, "uqm.key", "keys.");
	}

	register_flight_controls ();

	return;
}

void
TFB_SetInputVectors (volatile int menu[], int num_menu_,
		volatile int flight[], int num_templ_, int num_flight_)
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

	char *mapping_db;
	int len;
	size_t base_len;
	const char *slash;

	if ((SDL_InitSubSystem (SDL_INIT_GAMECONTROLLER)) == -1)
	{
		log_add (log_Fatal, "Couldn't initialize joystick subsystem: %s",
			SDL_GetError ());
		exit (EXIT_FAILURE);
	}

	SDL_GameControllerEventState (SDL_ENABLE);

	base_len = strlen (baseContentPath);
	if (base_len > 0)
	{
		char last_char = baseContentPath[base_len - 1];
		slash = (last_char == '/' || last_char == '\\') ? "" : "/";
	}
	else
		slash = "/";

	len = snprintf (NULL, 0, "%s%sgamecontrollerdb.txt",
			baseContentPath, slash);

	mapping_db = HMalloc (len + 1);

	snprintf (mapping_db, len + 1, "%s%sgamecontrollerdb.txt",
		baseContentPath, slash);

	if (SDL_GameControllerAddMappingsFromFile (mapping_db) == -1)
	{
		log_add (log_Warning, "Could not load controller mappings "
				"from %s: %s", mapping_db, SDL_GetError ());
	}
	else
	{
		log_add (log_Debug, "Loaded controller mappings from %s",
				mapping_db);
	}

	HFree (mapping_db);

	nJoysticks = SDL_NumJoysticks ();
	log_add (log_Info, "%i joysticks were found.", nJoysticks);

	if (nJoysticks > 0)
	{
		int i;
		log_add (log_Info, "The names of the joysticks are:");
		for (i = 0; i < nJoysticks; i++)
		{
			if (SDL_IsGameController (i))
			{
				log_add (log_Info, "    %s (controller)",
					SDL_GameControllerNameForIndex (i));
			}
			else
			{
				log_add (log_Info, "    %s (joystick)",
					SDL_JoystickNameForIndex (i));
			}
		}
		for (int i = 0; i < nJoysticks; i++)
			create_joystick (i);
	}
}

#endif /* HAVE_JOYSTICK */

int
TFB_InitInput (int driver, int flags)
{
	(void)driver;
	(void)flags;

#ifdef HAVE_JOYSTICK
	initJoystick ();
#endif

	in_character_mode = FALSE;

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
	SDL_QuitSubSystem (SDL_INIT_GAMECONTROLLER);
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
	kbdhead = kbdtail = 0;
	lastchar = 0;
	in_character_mode = FALSE;
	VControl_ResetInput();
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

static inline int
is_numpad_char_event (const SDL_Event * Event)
{
	return in_character_mode &&
			(Event->type == SDL_KEYDOWN || Event->type == SDL_KEYUP) &&
			(Event->key.keysym.mod & KMOD_NUM) &&  /* Numlock on */
			Event->key.keysym.sym >= SDLK_KP_1 &&  /* Keypad key */
			Event->key.keysym.sym <= SDLK_KP_PERIOD;
	// Note that in the SDL2 enumeration 0 comes after 9 and before period
}

void
ProcessInputEvent (const SDL_Event *Event)
{
	if (!InputInitialized)
		return;

	if (in_character_mode && !set_character_mode)
	{
		set_character_mode = TRUE;
		SDL_StartTextInput ();
	}

	if (!in_character_mode && set_character_mode)
	{
		set_character_mode = FALSE;
		SDL_StopTextInput ();
	}

	/* "Block" numpad input when NUM_LOCK is on */
	if (!is_numpad_char_event (Event))
	{
		VControl_HandleEvent (Event);
	}

	if (Event->type == SDL_TEXTINPUT)
	{
		int newtail;
		int i = 0;

		while (Event->text.text[i])
		{
			UniChar map_key = Event->text.text[i++];
			map_key &= 0xFF; /* Interpret as unsigned byte */

			/* Decode any UTF-8 keys */
			if (map_key >= 0xC0 && map_key < 0xE0)
			{
				/* 2-byte UTF-8 */
				map_key = (map_key & 0x1f) << 6;
				map_key |= Event->text.text[i++] & 0x3f;
			}
			else if (map_key >= 0xE0 && map_key < 0xF0)
			{
				/* 3-byte UTF-8 */
				map_key = (map_key & 0x0f) << 6;
				map_key |= Event->text.text[i++] & 0x3f;
				map_key <<= 6;
				map_key |= Event->text.text[i++] & 0x3f;
			}
			else if (map_key >= 0xF0)
			{
				/* Out of the BMP, won't fit in a UniChar */
				/* Use the replacement character instead */
				map_key = 0xFFFD;
				while ((UniChar)Event->text.text[i] > 0x7F)
				{
					++i;
				}
			}

			/* dont care about the non-printable, non-char */
			if (!map_key)
				return;

			newtail = (kbdtail + 1) & (KBDBUFSIZE - 1);

			/* ignore the char if the buffer is full */
			if (newtail != kbdhead)
			{
				kbdbuf[kbdtail] = map_key;
				kbdtail = newtail;
				lastchar = map_key;
			}

			/* Loop back in case there are more chars in the
			 * text input buffer */
		}
	}
}

void
TFB_ResetControls (void)
{
	VControl_ResetInput ();
	// flush character buffer
	kbdhead = kbdtail = 0;
	lastchar = 0;
}

const char xbx_buttons[SDL_CONTROLLER_BUTTON_MAX][16] =
{
	"A", "B", "X", "Y", "Back", "Guide", "Start", "LS", "RS",
	"LB", "RB", "Up", "Down", "Left", "Right", "Misc", "Paddle 1",
	"Paddle 3","Paddle 2","Paddle 4","???"
};

const char xbx_axes[SDL_CONTROLLER_AXIS_MAX][16] =
{ "LS H", "LS V", "RS H", "RS V", "LT", "RT" };

const char ds4_buttons[SDL_CONTROLLER_BUTTON_MAX][16] =
{
	STR_CROSS, STR_CIRCLE, STR_SQUARE, STR_TRIANGLE, "Share", "PS",
	"Options", "L3", "R3", "L1", "R1", "Up", "Down", "Left", "Right",
	"Mic","Paddle 1","Paddle 3","Paddle 2","Paddle 4","TouchPad"
};

const char ds4_axes[SDL_CONTROLLER_AXIS_MAX][16] =
{ "LS H", "LS V", "RS H", "RS V", "L2", "R2" };

const char nx_buttons[SDL_CONTROLLER_BUTTON_MAX][16] =
{
	"B", "A", "Y", "X", "Minus", "Home", "Plus", "LS", "RS",
	"L", "R", "Up", "Down", "Left", "Right", "Capture", "Paddle 1",
	"Paddle 3", "Paddle 2", "Paddle 4", "???"
};

const char nx_axes[SDL_CONTROLLER_AXIS_MAX][16] =
{ "LS H", "LS V", "RS H", "RS V", "ZL", "ZR" };

void
InterrogateInputState (int templat, int control, int index, char *buffer,
		int maxlen, VCONTROL_GESTURE *g_override)
{
	VCONTROL_GESTURE *g;

	if (g_override != NULL)
		g = g_override;
	else
	{
		g = CONTROL_PTR (templat, control, index);

		if (templat >= num_templ || control >= num_flight
				|| index >= MAX_FLIGHT_ALTERNATES)
		{
			log_add (log_Warning,
					"InterrogateInputState(): invalid control index");
			buffer[0] = 0;
			return;
		}
	}

	switch (g->type)
	{
	case VCONTROL_KEY:
		snprintf (buffer, maxlen, "%s",
				VControl_code2name (g->gesture.key));
		buffer[maxlen - 1] = 0;
		break;
	case VCONTROL_JOYBUTTON:
			if (optControllerType == 1)
			{
				snprintf (buffer, maxlen, "[J%d %s]",
						g->gesture.button.port,
						xbx_buttons[g->gesture.button.index]);
			}
			else if (optControllerType == 2)
			{
				snprintf (buffer, maxlen, "[J%d %s]",
						g->gesture.button.port,
						ds4_buttons[g->gesture.button.index]);
			}
			else if (optControllerType == 3)
			{
				snprintf (buffer, maxlen, "[J%d %s]",
						g->gesture.button.port,
						nx_buttons[g->gesture.button.index]);
			}
			else
		{
			snprintf (buffer, maxlen, "J%d B%d",
					g->gesture.button.port,
					g->gesture.button.index);
		}
		buffer[maxlen - 1] = 0;
		break;
	case VCONTROL_JOYAXIS:
			if (optControllerType == 1)
			{
				snprintf (buffer, maxlen, "[J%d %s%c]",
						g->gesture.axis.port,
						xbx_axes[g->gesture.axis.index],
						g->gesture.axis.polarity > 0 ? '+' : '-');
			}
			else if (optControllerType == 2)
			{
				snprintf (buffer, maxlen, "[J%d %s%c]",
						g->gesture.axis.port,
						ds4_axes[g->gesture.axis.index],
						g->gesture.axis.polarity > 0 ? '+' : '-');
			}
			else if (optControllerType == 3)
			{
				snprintf (buffer, maxlen, "[J%d %s%c]",
						g->gesture.axis.port,
						nx_axes[g->gesture.axis.index],
						g->gesture.axis.polarity > 0 ? '+' : '-');
			}
			else
		{
			snprintf (buffer, maxlen, "J%d A%d %c",
					g->gesture.axis.port,
					g->gesture.axis.index,
					g->gesture.axis.polarity > 0 ? '+' : '-');
		}
		buffer[maxlen - 1] = 0;
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

	snprintf (keybuf, 39, "keys.%d.%s.%d", templat+1,
			flight_res_names[control], index+1);
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
	snprintf (keybuf, 39, "keys.%d.%s.%d", templat+1,
			flight_res_names[control], index+1);
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

static FRAME
KeyAtlas (int menu_index, FRAME k_atlas)
{
	VCONTROL_GESTURE g = curr_bindings[menu_index].binding[0];
	int i = VControl_code2index (g.gesture.key);
	const char *key = VControl_code2name (g.gesture.key);

	return SetAbsFrameIndex (k_atlas, i);
}

FRAME
ControlAtlas (int menu_index, FRAME atlas_array[])
{
	VCONTROL_GESTURE g;
	int i;

	if (!optControllerType)
		return KeyAtlas (menu_index, atlas_array[0]);

	for (i = 0; i < 6; i++)
	{
		g = curr_bindings[menu_index].binding[i];
		if (g.type == VCONTROL_JOYBUTTON || g.type == VCONTROL_JOYAXIS)
			break;
	}

	switch (g.type)
	{
	case VCONTROL_JOYBUTTON:
	{
		int index = g.gesture.button.index;
		int frame_index = 0;

		if (optControllerType > 1)
			frame_index = NUM_BUTTONS * (optControllerType - 1);

		return SetAbsFrameIndex (atlas_array[2], index + frame_index);
	}
	case VCONTROL_JOYAXIS:
	{
		int atlas_index;
		int index = g.gesture.axis.index;
		BOOLEAN polarity = g.gesture.axis.polarity < 0 ? 0 : 1;
		int frame_index = 0;

		if (optControllerType > 1)
			frame_index = NUM_AXIS * (optControllerType - 1);

		atlas_index = index * 2 + polarity + frame_index;

		return SetAbsFrameIndex (atlas_array[1], atlas_index);
	}
	default:
		return KeyAtlas (menu_index, atlas_array[0]);
	}
}

VCONTROL_GESTURE *
GetBindingForAction (int player, int action, int alt_index)
{
	if (player < 0 || player >= num_templ ||
		action < 0 || action >= num_flight ||
		alt_index < 0 || alt_index >= MAX_FLIGHT_ALTERNATES)
	{
		return NULL;
	}

	return CONTROL_PTR (player, action, alt_index);
}

int GetActionFromEvent (const SDL_Event *event, int player)
{
	int i;
	VCONTROL_GESTURE *g;

	if (!event)
		return -1;

	for (i = 0; i < NUM_KEYS; i++)
	{
		for (int alt = 0; alt < MAX_FLIGHT_ALTERNATES; alt++)
		{
			g = GetBindingForAction (player, i, alt);

			if (g == NULL)
				return -1;

			if ((g->type == VCONTROL_KEY &&
				g->gesture.key == event->key.keysym.sym) ||
					(g->type == VCONTROL_JOYBUTTON &&
					g->gesture.button.index == event->cbutton.button))
			{
				return i;
			}
		}
	}

	return -1;
}