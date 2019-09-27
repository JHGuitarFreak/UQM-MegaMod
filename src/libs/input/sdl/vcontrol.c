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

#include "port.h"
#include SDL_INCLUDE(SDL.h)
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "vcontrol.h"
#include "libs/memlib.h"
#include "keynames.h"
#include "libs/log.h"
#include "libs/reslib.h"

/* How many binding slots are allocated at once. */
#define POOL_CHUNK_SIZE 64

typedef struct vcontrol_keybinding {
	int *target;
	struct vcontrol_keypool *parent;
	struct vcontrol_keybinding *next;
} keybinding;

typedef struct vcontrol_keypool {
	keybinding pool[POOL_CHUNK_SIZE];
	int remaining;
	struct vcontrol_keypool *next;
} keypool;


#ifdef HAVE_JOYSTICK

typedef struct vcontrol_joystick_axis {
	keybinding *neg, *pos;
	int polarity;
#if defined(ANDROID) || defined(__ANDROID__)
	int value;
#endif
} axis_type;

typedef struct vcontrol_joystick_hat {
	keybinding *left, *right, *up, *down;
	Uint8 last;
} hat_type;

typedef struct vcontrol_joystick {
	SDL_Joystick *stick;
	int numaxes, numbuttons, numhats;
	int threshold;
	axis_type *axes;
	keybinding **buttons;
	hat_type *hats;
} joystick;

static joystick *joysticks;

#endif /* HAVE_JOYSTICK */

#if defined(ANDROID) || defined(__ANDROID__)
static unsigned int joycount = 0;
#else
static unsigned int joycount;
#endif

static unsigned int num_sdl_keys = 0;
static keybinding **bindings = NULL;

static keypool *pool;

/* Last interesting event */
static int event_ready;
static SDL_Event last_interesting;

static keypool *
allocate_key_chunk (void)
{
	keypool *x = HMalloc (sizeof (keypool));
	if (x)
	{
		int i;
		x->remaining = POOL_CHUNK_SIZE;
		x->next = NULL;
		for (i = 0; i < POOL_CHUNK_SIZE; i++)
		{
			x->pool[i].target = NULL;
			x->pool[i].next = NULL;
			x->pool[i].parent = x;
		}
	}
	return x;
}

static void
free_key_pool (keypool *x)
{
	if (x)
	{
		free_key_pool (x->next);
		HFree (x);
	}
}

#ifdef HAVE_JOYSTICK

static void
create_joystick (int index)
{
	SDL_Joystick *stick;
	int axes, buttons, hats;
	if ((unsigned int) index >= joycount)
	{
		log_add (log_Warning, "VControl warning: Tried to open a non-existent joystick!");
		return;
	}
	if (joysticks[index].stick)
	{
		// Joystick is already created.  Return.
		return;
	}
	stick = SDL_JoystickOpen (index);
	if (stick)
	{
		joystick *x = &joysticks[index];
		int j;
		log_add (log_Info, "VControl opened joystick: %s", SDL_JoystickName (index));
		axes = SDL_JoystickNumAxes (stick);
		buttons = SDL_JoystickNumButtons (stick);
		hats = SDL_JoystickNumHats (stick);
		log_add (log_Info, "%d axes, %d buttons, %d hats.", axes, buttons, hats);
		x->numaxes = axes;
		x->numbuttons = buttons;
		x->numhats = hats;
		x->axes = HMalloc (sizeof (axis_type) * axes);
		x->buttons = HMalloc (sizeof (keybinding *) * buttons);
		x->hats = HMalloc (sizeof (hat_type) * hats);
		for (j = 0; j < axes; j++)
		{
			x->axes[j].neg = x->axes[j].pos = NULL;
#if defined(ANDROID) || defined(__ANDROID__)
			x->axes[j].polarity = x->axes[j].value = 0;
#endif
		}
		for (j = 0; j < hats; j++)
		{
			x->hats[j].left = x->hats[j].right = NULL;
			x->hats[j].up = x->hats[j].down = NULL;
			x->hats[j].last = SDL_HAT_CENTERED;
		}
		for (j = 0; j < buttons; j++)
		{
			x->buttons[j] = NULL;
		}
		x->stick = stick;
	}
	else
	{
		log_add (log_Warning, "VControl: Could not initialize joystick #%d", index);
	}
}
			
static void
destroy_joystick (int index)
{
	SDL_Joystick *stick = joysticks[index].stick;
	if (stick)
	{
		SDL_JoystickClose (stick);
		joysticks[index].stick = NULL;
		HFree (joysticks[index].axes);
		HFree (joysticks[index].buttons);
		HFree (joysticks[index].hats);
		joysticks[index].numaxes = joysticks[index].numbuttons = 0;
		joysticks[index].axes = NULL;
		joysticks[index].buttons = NULL;
		joysticks[index].hats = NULL;
	}
}

#endif /* HAVE_JOYSTICK */

static void
key_init (void)
{
	unsigned int i;
	int signed_num_sdl_keys; // JMS: New variable to silence warnings
	// int num_keys; // Temp to match type of param for SDL_GetKeyState().

	pool = allocate_key_chunk ();
	(void)SDL_GetKeyState (&signed_num_sdl_keys); // JMS: was num_sdl_keys
	num_sdl_keys = (unsigned int) signed_num_sdl_keys; // JMS: new line
	bindings = (keybinding **) HMalloc (sizeof (keybinding *) * num_sdl_keys);
	for (i = 0; i < num_sdl_keys; i++)
		bindings[i] = NULL;

#ifdef HAVE_JOYSTICK
	/* Prepare for possible joystick controls.  We don't actually
	   GRAB joysticks unless we're asked to make a joystick
	   binding, though. */
	joycount = SDL_NumJoysticks ();
	if (joycount)
	{
		joysticks = HMalloc (sizeof (joystick) * joycount);
		for (i = 0; i < joycount; i++)
		{
			joysticks[i].stick = NULL;	
			joysticks[i].numaxes = joysticks[i].numbuttons = 0;
			joysticks[i].axes = NULL;
			joysticks[i].buttons = NULL;
			joysticks[i].threshold = 10000;
		}
	}
	else
	{
		joysticks = NULL;
	}
#else
	joycount = 0;
#endif /* HAVE_JOYSTICK */
}

static void
key_uninit (void)
{
	unsigned int i;
	free_key_pool (pool);
	for (i = 0; i < num_sdl_keys; i++)
		bindings[i] = NULL;
	HFree (bindings);
	bindings = NULL;
	pool = NULL;

#ifdef HAVE_JOYSTICK
	for (i = 0; i < joycount; i++)
		destroy_joystick (i);
	HFree (joysticks);
#endif /* HAVE_JOYSTICK */
}

void
VControl_Init (void)
{
	key_init ();
}

void
VControl_Uninit (void)
{
	key_uninit ();
}

int
VControl_SetJoyThreshold (int port, int threshold)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joysticks[port].threshold = threshold;
		return 0;
	}
	else
#else
	(void) port;
	(void) threshold;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Warning, "VControl_SetJoyThreshold passed illegal port %d", port);
		return -1;
	}
}


static void
add_binding (keybinding **newptr, int *target)
{
	keybinding *newbinding;
	keypool *searchbase;
	int i;

	/* Acquire a pointer to the keybinding * that we'll be
	 * overwriting.  Along the way, ensure we haven't already
	 * bound this symbol to this target.  If we have, return.*/
	while (*newptr != NULL)
	{
		if ((*newptr)->target == target)
		{
			return;
		}
		newptr = &((*newptr)->next);
	}

	/* Now hunt through the binding pool for a free binding. */

	/* First, find a chunk with free spots in it */

	searchbase = pool;
	while (searchbase->remaining == 0)
	{
		/* If we're completely full, allocate a new chunk */
		if (searchbase->next == NULL)
		{
			searchbase->next = allocate_key_chunk ();
		}
		searchbase = searchbase->next;
	}

	/* Now find a free binding within it */

	newbinding = NULL;
	for (i = 0; i < POOL_CHUNK_SIZE; i++)
	{
		if (searchbase->pool[i].target == NULL)
		{
			newbinding = &searchbase->pool[i];
			break;
		}
	}

	/* Sanity check. */
	if (!newbinding)
	{
		log_add (log_Warning, "add_binding failed to find a free binding slot!");
		return;
	}

	newbinding->target = target;
	newbinding->next = NULL;
	*newptr = newbinding;
	searchbase->remaining--;
}

static void
remove_binding (keybinding **ptr, int *target)
{
	if (!(*ptr))
	{
		/* Nothing bound to symbol; return. */
		return;
	}
	else if ((*ptr)->target == target)
	{
		keybinding *todel = *ptr;
		*ptr = todel->next;
		todel->target = NULL;
		todel->next = NULL;
		todel->parent->remaining++;
	}
	else
	{
		keybinding *prev = *ptr;
		while (prev && prev->next != NULL)
		{
			if (prev->next->target == target)
			{
				keybinding *todel = prev->next;
				prev->next = todel->next;
				todel->target = NULL;
				todel->next = NULL;
				todel->parent->remaining++;
			}
			prev = prev->next;
		}
	}
}

static void
activate (keybinding *i)
{
	while (i != NULL)
	{
		*(i->target) = (*(i->target)+1) | VCONTROL_STARTBIT;
		i = i->next;
	}
}

static void
deactivate (keybinding *i)
{
	while (i != NULL)
	{
		int v = *(i->target) & VCONTROL_MASK;
		if (v > 0)
		{
			*(i->target) = (v-1) | (*(i->target) & VCONTROL_STARTBIT);
		}
		i = i->next;
	}
}

static void
event2gesture (SDL_Event *e, VCONTROL_GESTURE *g)
{
	switch (e->type)
	{
	case SDL_KEYDOWN:
		g->type = VCONTROL_KEY;
		g->gesture.key = e->key.keysym.sym;
		break;
	case SDL_JOYAXISMOTION:
		g->type = VCONTROL_JOYAXIS;
		g->gesture.axis.port = e->jaxis.which;
		g->gesture.axis.index = e->jaxis.axis;
		g->gesture.axis.polarity = (e->jaxis.value < 0) ? -1 : 1;
		break;
	case SDL_JOYHATMOTION:
		g->type = VCONTROL_JOYHAT;
		g->gesture.hat.port = e->jhat.which;
		g->gesture.hat.index = e->jhat.hat;
		g->gesture.hat.dir = e->jhat.value;
		break;
	case SDL_JOYBUTTONDOWN:
		g->type = VCONTROL_JOYBUTTON;
		g->gesture.button.port = e->jbutton.which;
		g->gesture.button.index = e->jbutton.button;
		break;

	default:
		g->type = VCONTROL_NONE;
		break;
	}
}

int
VControl_AddGestureBinding (VCONTROL_GESTURE *g, int *target)
{
	int result = -1;
	switch (g->type)
	{
	case VCONTROL_KEY:
		result = VControl_AddKeyBinding (g->gesture.key, target);
		break;

	case VCONTROL_JOYAXIS:
#ifdef HAVE_JOYSTICK
		result = VControl_AddJoyAxisBinding (g->gesture.axis.port, g->gesture.axis.index, (g->gesture.axis.polarity < 0) ? -1 : 1, target);
		break;
#endif
	case VCONTROL_JOYHAT:
#ifdef HAVE_JOYSTICK
		result = VControl_AddJoyHatBinding (g->gesture.hat.port, g->gesture.hat.index, g->gesture.hat.dir, target);
		break;
#endif
	case VCONTROL_JOYBUTTON:
#ifdef HAVE_JOYSTICK
		result = VControl_AddJoyButtonBinding (g->gesture.button.port, g->gesture.button.index, target);
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_NONE:
		/* Do nothing */
		break;

	default:
		log_add (log_Warning, "VControl_AddGestureBinding didn't understand argument gesture");
		result = -1;
		break;
	}
	return result;
}

void
VControl_RemoveGestureBinding (VCONTROL_GESTURE *g, int *target)
{
	switch (g->type)
	{
	case VCONTROL_KEY:
		VControl_RemoveKeyBinding (g->gesture.key, target);
		break;

	case VCONTROL_JOYAXIS:
#ifdef HAVE_JOYSTICK
		VControl_RemoveJoyAxisBinding (g->gesture.axis.port, g->gesture.axis.index, (g->gesture.axis.polarity < 0) ? -1 : 1, target);
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_JOYHAT:
#ifdef HAVE_JOYSTICK
		VControl_RemoveJoyHatBinding (g->gesture.hat.port, g->gesture.hat.index, g->gesture.hat.dir, target);
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_JOYBUTTON:
#ifdef HAVE_JOYSTICK
		VControl_RemoveJoyButtonBinding (g->gesture.button.port, g->gesture.button.index, target);
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_NONE:
		break;
	default:
		log_add (log_Warning, "VControl_RemoveGestureBinding didn't understand argument gesture");
		break;
	}
}

int
VControl_AddKeyBinding (SDLKey symbol, int *target)
{
	if ((unsigned int) symbol >= num_sdl_keys) {
		log_add (log_Warning, "VControl: Illegal key index %d", symbol);
		return -1;
	}
	add_binding(&bindings[symbol], target);
	return 0;
}

void
VControl_RemoveKeyBinding (SDLKey symbol, int *target)
{
	if ((unsigned int) symbol >= num_sdl_keys) {
		log_add (log_Warning, "VControl: Illegal key index %d", symbol);
		return;
	}
	remove_binding (&bindings[symbol], target);
}

int
VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				add_binding(&joysticks[port].axes[axis].neg, target);
			}
			else if (polarity > 0)
			{
				add_binding(&joysticks[port].axes[axis].pos, target);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to bind to polarity zero");
				return -1;
			}
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to illegal axis %d", axis);
			return -1;
		}
	}
	else
#else
	(void) port;
	(void) axis;
	(void) polarity;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Debug, "VControl: Attempted to bind to illegal port %d", port);
		return -1;
	}
	return 0;
}

void
VControl_RemoveJoyAxisBinding (int port, int axis, int polarity, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				remove_binding(&joysticks[port].axes[axis].neg, target);
			}
			else if (polarity > 0)
			{
				remove_binding(&joysticks[port].axes[axis].pos, target);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to unbind from polarity zero");
			}
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from illegal axis %d", axis);
		}
	}
	else
#else
	(void) port;
	(void) axis;
	(void) polarity;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal port %d", port);
	}
}

int
VControl_AddJoyButtonBinding (int port, int button, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			add_binding(&joysticks[port].buttons[button], target);
			return 0;
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to illegal button %d", button);
			return -1;
		}
	}
	else
#else
	(void) port;
	(void) button;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Debug, "VControl: Attempted to bind to illegal port %d", port);
		return -1;
	}
}

void
VControl_RemoveJoyButtonBinding (int port, int button, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			remove_binding (&joysticks[port].buttons[button], target);
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from illegal button %d", button);
		}
	}
	else
#else
	(void) port;
	(void) button;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal port %d", port);
	}
}

int
VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				add_binding(&joysticks[port].hats[which].left, target);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				add_binding(&joysticks[port].hats[which].right, target);
			}
			else if (dir == SDL_HAT_UP)
			{
				add_binding(&joysticks[port].hats[which].up, target);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				add_binding(&joysticks[port].hats[which].down, target);
			}
			else
			{
				// log_add (log_Debug, "VControl: Attempted to bind to illegal direction");
				return -1;
			}
			return 0;
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to illegal hat %d", which);
			return -1;
		}
	}
	else
#else
	(void) port;
	(void) which;
	(void) dir;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Debug, "VControl: Attempted to bind to illegal port %d", port);
		return -1;
	}
}

void
VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				remove_binding(&joysticks[port].hats[which].left, target);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				remove_binding(&joysticks[port].hats[which].right, target);
			}
			else if (dir == SDL_HAT_UP)
			{
				remove_binding(&joysticks[port].hats[which].up, target);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				remove_binding(&joysticks[port].hats[which].down, target);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to unbind from illegal direction");
			}
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from illegal hat %d", which);
		}
	}
	else
#else
	(void) port;
	(void) which;
	(void) dir;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal port %d", port);
	}
}

void
VControl_RemoveAllBindings (void)
{
	key_uninit ();
	key_init ();
}

void
VControl_ProcessKeyDown (SDLKey symbol)
{
	if (symbol >= num_sdl_keys) {
		log_add (log_Warning, "VControl: Got unknown key index %d", symbol);
		return;
	}
	
	activate (bindings[symbol]);
}

void
VControl_ProcessKeyUp (SDLKey symbol)
{
	if (symbol >= num_sdl_keys)
		return;

	deactivate (bindings[symbol]);
}

void
VControl_ProcessJoyButtonDown (int port, int button)
{
#ifdef HAVE_JOYSTICK
	if (!joysticks[port].stick)
		return;
	activate (joysticks[port].buttons[button]);
#else
	(void) port;
	(void) button;
#endif /* HAVE_JOYSTICK */
}

void
VControl_ProcessJoyButtonUp (int port, int button)
{
#ifdef HAVE_JOYSTICK
	if (!joysticks[port].stick)
		return;
	deactivate (joysticks[port].buttons[button]);
#else
	(void) port;
	(void) button;
#endif /* HAVE_JOYSTICK */
}

void
VControl_ProcessJoyAxis (int port, int axis, int value)
{
#ifdef HAVE_JOYSTICK
	int t;
	if (!joysticks[port].stick)
		return;
#if defined(ANDROID) || defined(__ANDROID__)
	joysticks[port].axes[axis].value = value;
#endif
	t = joysticks[port].threshold;
	if (value > t)
	{
		if (joysticks[port].axes[axis].polarity != 1)
		{
			if (joysticks[port].axes[axis].polarity == -1)
			{
				deactivate (joysticks[port].axes[axis].neg);
			}
			joysticks[port].axes[axis].polarity = 1;
			activate (joysticks[port].axes[axis].pos);
		}
#if defined(ANDROID) || defined(__ANDROID__)
		if (port == 2) {
		// Gamepad used - hide on-screen keys
			TFB_SetOnScreenKeyboard_HiddenPermanently();
		}
#endif
	}
	else if (value < -t)
	{
		if (joysticks[port].axes[axis].polarity != -1)
		{
			if (joysticks[port].axes[axis].polarity == 1)
			{
				deactivate (joysticks[port].axes[axis].pos);
			}
			joysticks[port].axes[axis].polarity = -1;
			activate (joysticks[port].axes[axis].neg);
		}
	}
	else
	{
		if (joysticks[port].axes[axis].polarity == -1)
		{
			deactivate (joysticks[port].axes[axis].neg);
		}
		else if (joysticks[port].axes[axis].polarity == 1)
		{
			deactivate (joysticks[port].axes[axis].pos);
		}
		joysticks[port].axes[axis].polarity = 0;
	}
#else
	(void) port;
	(void) axis;
	(void) value;
#endif /* HAVE_JOYSTICK */
}

void
VControl_ProcessJoyHat (int port, int which, Uint8 value)
{
#ifdef HAVE_JOYSTICK
	Uint8 old;
	if (!joysticks[port].stick)
		return;
	old = joysticks[port].hats[which].last;
	if (!(old & SDL_HAT_LEFT) && (value & SDL_HAT_LEFT))
		activate (joysticks[port].hats[which].left);
	if (!(old & SDL_HAT_RIGHT) && (value & SDL_HAT_RIGHT))
		activate (joysticks[port].hats[which].right);
	if (!(old & SDL_HAT_UP) && (value & SDL_HAT_UP))
		activate (joysticks[port].hats[which].up);
	if (!(old & SDL_HAT_DOWN) && (value & SDL_HAT_DOWN))
		activate (joysticks[port].hats[which].down);
	if ((old & SDL_HAT_LEFT) && !(value & SDL_HAT_LEFT))
		deactivate (joysticks[port].hats[which].left);
	if ((old & SDL_HAT_RIGHT) && !(value & SDL_HAT_RIGHT))
		deactivate (joysticks[port].hats[which].right);
	if ((old & SDL_HAT_UP) && !(value & SDL_HAT_UP))
		deactivate (joysticks[port].hats[which].up);
	if ((old & SDL_HAT_DOWN) && !(value & SDL_HAT_DOWN))
		deactivate (joysticks[port].hats[which].down);
	joysticks[port].hats[which].last = value;
#else
	(void) port;
	(void) which;
	(void) value;
#endif /* HAVE_JOYSTICK */
}

#if defined(ANDROID) || defined(__ANDROID__)
int
VControl_GetJoyAxis(int port, int axis) {
#ifdef HAVE_JOYSTICK
	if (joycount <= port)
		return 0;
	if (!joysticks[port].stick || joysticks[port].numaxes <= axis)
		return 0;
	return joysticks[port].axes[axis].value;
#else
	return 0;
#endif /* HAVE_JOYSTICK */
}

int VControl_GetJoysticksAmount() {
	return joycount;
}
#endif

void
VControl_ResetInput (void)
{
	/* Step through every valid entry in the binding pool and zero
	 * them out.  This will probably zero entries multiple times;
	 * oh well, no harm done. */

	keypool *base = pool;
	while (base != NULL)
	{
		int i;
		for (i = 0; i < POOL_CHUNK_SIZE; i++)
		{
			if(base->pool[i].target)
			{
				*(base->pool[i].target) = 0;
			}
		}
		base = base->next;
	}
}

void
VControl_BeginFrame (void)
{
	/* Step through every valid entry in the binding pool and zero
	 * out the frame-start bit.  This will probably zero entries
	 * multiple times; oh well, no harm done. */

	keypool *base = pool;
	while (base != NULL)
	{
		int i;
		for (i = 0; i < POOL_CHUNK_SIZE; i++)
		{
			if(base->pool[i].target)
			{
				*(base->pool[i].target) &= VCONTROL_MASK;
			}
		}
		base = base->next;
	}
}

void
VControl_HandleEvent (const SDL_Event *e)
{
	switch (e->type)
	{
		case SDL_KEYDOWN:
			VControl_ProcessKeyDown (e->key.keysym.sym);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_KEYUP:
			VControl_ProcessKeyUp (e->key.keysym.sym);
			break;

#ifdef HAVE_JOYSTICK
		case SDL_JOYAXISMOTION:
			VControl_ProcessJoyAxis (e->jaxis.which, e->jaxis.axis, e->jaxis.value);
			if ((e->jaxis.value > 15000) || (e->jaxis.value < -15000))
			{
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_JOYHATMOTION:
			VControl_ProcessJoyHat (e->jhat.which, e->jhat.hat, e->jhat.value);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONDOWN:
			VControl_ProcessJoyButtonDown (e->jbutton.which, e->jbutton.button);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONUP:
			VControl_ProcessJoyButtonUp (e->jbutton.which, e->jbutton.button);
			break;
#endif /* HAVE_JOYSTICK */

		default:
			break;
	}
}

/* Tracking the last interesting event */

void
VControl_ClearGesture (void)
{
	event_ready = 0;
}

int
VControl_GetLastGesture (VCONTROL_GESTURE *g)
{
	if (event_ready && g != NULL)
	{
		event2gesture(&last_interesting, g);
	}
	return event_ready;
}		

/* Configuration file grammar is as follows:  One command per line, 
 * hashes introduce comments that persist to end of line.  Blank lines
 * are ignored.
 *
 * Terminals are represented here as quoted strings, e.g. "foo" for 
 * the literal string foo.  These are matched case-insensitively.
 * Special terminals are:
 *
 * KEYNAME:  This names a key, as defined in keynames.c.
 * IDNAME:   This is an arbitrary string of alphanumerics, 
 *           case-insensitive, and ending with a colon.  This
 *           names an application-specific control value.
 * NUM:      This is an unsigned integer.
 * EOF:      End of file
 *
 * Nonterminals (the grammar itself) have the following productions:
 * 
 * configline <- IDNAME binding
 *             | "joystick" NUM "threshold" NUM
 *             | "version" NUM
 *
 * binding    <- "key" KEYNAME
 *             | "joystick" NUM joybinding
 *
 * joybinding <- "axis" NUM polarity
 *             | "button" NUM
 *             | "hat" NUM direction
 *
 * polarity   <- "positive" | "negative"
 *
 * dir        <- "up" | "down" | "left" | "right"
 *
 * This grammar is amenable to simple recursive descent parsing;
 * in fact, it's fully LL(1). */

/* Actual maximum line and token sizes are two less than this, since
 * we need space for the \n\0 at the end */
#define LINE_SIZE 256
#define TOKEN_SIZE 64

typedef struct vcontrol_parse_state {
	char line[LINE_SIZE];
	char token[TOKEN_SIZE];
	int index;
	int error;
	int linenum;
} parse_state;

static void
next_token (parse_state *state)
{
	int index, base;

	state->token[0] = 0;
	/* skip preceding whitespace */
	base = state->index;
	while (state->line[base] && isspace (state->line[base]))
	{
		base++;
	}

	index = 0;
	while (index < (TOKEN_SIZE-1) && state->line[base+index] && !isspace (state->line[base+index]))
	{
		state->token[index] = state->line[base+index];
		index++;
	}
	state->token[index] = 0;

	/* If the token was too long, skip ahead until we get to whitespace */
	while (state->line[base+index] && !isspace (state->line[base+index]))
	{
		index++;
	}

	state->index = base+index;
}

static void
expected_error (parse_state *state, const char *expected)
{
	log_add (log_Warning, "VControl: Expected '%s' on config file line %d",
			expected, state->linenum);
	state->error = 1;
}

static void
consume (parse_state *state, const char *expected)
{
	if (strcasecmp (expected, state->token))
	{
		expected_error (state, expected);
	}
	next_token (state);
}

static int
consume_keyname (parse_state *state)
{
	int keysym = VControl_name2code (state->token);
	if (!keysym)
	{
		log_add (log_Warning, "VControl: Illegal key name '%s' on config file line %d",
				state->token, state->linenum);
		state->error = 1;
	}
	next_token (state);
	return keysym;
}

static int
consume_num (parse_state *state)
{
	char *end;
	int result = strtol (state->token, &end, 10);
	if (*end != '\0')
	{
		log_add (log_Warning, "VControl: Expected integer on config line %d",
				state->linenum);
		state->error = 1;
	}
	next_token (state);
	return result;
}

static int
consume_polarity (parse_state *state)
{
	int result = 0;
	if (!strcasecmp (state->token, "positive"))
	{
		result = 1;
	}
	else if (!strcasecmp (state->token, "negative"))
	{
		result = -1;
	}
	else
	{
		expected_error (state, "positive' or 'negative");
	}
	next_token (state);
	return result;
}

static Uint8
consume_dir (parse_state *state)
{
	Uint8 result = 0;
	if (!strcasecmp (state->token, "left"))
	{
		result = SDL_HAT_LEFT;
	}
	else if (!strcasecmp (state->token, "right"))
	{
		result = SDL_HAT_RIGHT;
	}
	else if (!strcasecmp (state->token, "up"))
	{
		result = SDL_HAT_UP;
	}
	else if (!strcasecmp (state->token, "down"))
	{
		result = SDL_HAT_DOWN;
	}
	else
	{
		expected_error (state, "left', 'right', 'up' or 'down");
	}
	next_token (state);
	return result;
}

static void
parse_joybinding (parse_state *state, VCONTROL_GESTURE *gesture)
{
	int sticknum;
	consume (state, "joystick");
	sticknum = consume_num (state);
	if (!state->error)
	{
		if (!strcasecmp (state->token, "axis"))
		{
			int axisnum;
			consume (state, "axis");
			axisnum = consume_num (state);
			if (!state->error)
			{
				int polarity = consume_polarity (state);
				if (!state->error)
				{
					gesture->type = VCONTROL_JOYAXIS;
					gesture->gesture.axis.port = sticknum;
					gesture->gesture.axis.index = axisnum;
					gesture->gesture.axis.polarity = polarity;
				}
			}
		} 
		else if (!strcasecmp (state->token, "button"))
		{
			int buttonnum;
			consume (state, "button");
			buttonnum = consume_num (state);
			if (!state->error)
			{
				gesture->type = VCONTROL_JOYBUTTON;
				gesture->gesture.button.port = sticknum;
				gesture->gesture.button.index = buttonnum;
			}
		}
		else if (!strcasecmp (state->token, "hat"))
		{
			int hatnum;
			consume (state, "hat");
			hatnum = consume_num (state);
			if (!state->error)
			{
				Uint8 dir = consume_dir (state);
				if (!state->error)
				{
					gesture->type = VCONTROL_JOYHAT;
					gesture->gesture.hat.port = sticknum;
					gesture->gesture.hat.index = hatnum;
					gesture->gesture.hat.dir = dir;
				}
			}
		}
		else
		{
			expected_error (state, "axis', 'button', or 'hat");
		}
	}
}

static void
parse_gesture (parse_state *state, VCONTROL_GESTURE *gesture)
{
	gesture->type = VCONTROL_NONE; /* Default to error */
	if (!strcasecmp (state->token, "key"))
	{
		/* Parse key binding */
		int keysym;
		consume (state, "key");
		keysym = consume_keyname (state);
		if (!state->error)
		{
			gesture->type = VCONTROL_KEY;
			gesture->gesture.key = keysym;
		}
	}
	else if (!strcasecmp (state->token, "joystick"))
	{
		parse_joybinding (state, gesture);
	}
	else
	{
		expected_error (state, "key' or 'joystick");
	}
}

void
VControl_ParseGesture (VCONTROL_GESTURE *g, const char *spec)
{
	parse_state ps;

	strncpy (ps.line, spec, LINE_SIZE);
	ps.line[LINE_SIZE - 1] = '\0';
	ps.index = ps.error = 0;
	ps.linenum = -1;

	next_token (&ps);
	parse_gesture (&ps, g);
	if (ps.error)
		printf ("Error parsing %s\n", spec);
}

int
VControl_DumpGesture (char *buf, int n, VCONTROL_GESTURE *g)
{
	switch (g->type) 
	{
	case VCONTROL_KEY:
		return snprintf (buf, n, "key %s", VControl_code2name (g->gesture.key));
	case VCONTROL_JOYAXIS:
		return snprintf (buf, n, "joystick %d axis %d %s", g->gesture.axis.port, g->gesture.axis.index, 
				(g->gesture.axis.polarity > 0) ? "positive" : "negative");
	case VCONTROL_JOYBUTTON:
		return snprintf (buf, n, "joystick %d button %d", g->gesture.button.port, g->gesture.button.index);
	case VCONTROL_JOYHAT:
		return snprintf (buf, n, "joystick %d hat %d %s", g->gesture.hat.port, g->gesture.hat.index, 
				(g->gesture.hat.dir == SDL_HAT_UP) ? "up" :
				((g->gesture.hat.dir == SDL_HAT_DOWN) ? "down" : 
				((g->gesture.hat.dir == SDL_HAT_LEFT) ? "left" : "right")));
	default:
		buf[0] = '\0';
		return 0;
	}
}
