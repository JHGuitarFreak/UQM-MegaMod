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

/* Total number of key input buckets. SDL1 keys are a simple enum,
 * but SDL2 scatters key symbols through the entire 32-bit space,
 * so we do not rely on being able to declare an array with one
 * entry per key. */
#define KEYBOARD_INPUT_BUCKETS 512

typedef struct vcontrol_keybinding {
	int *target;
	sdl_key_t keycode;
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
} axis_type;

typedef struct vcontrol_joystick_hat {
	keybinding *left, *right, *up, *down;
	Uint8 last;
} hat_type;

typedef struct vcontrol_joystick {
#if SDL_MAJOR_VERSION > 1
	SDL_GameController *stick;
#else
	SDL_Joystick *stick;
#endif
	int numaxes, numbuttons;
	int threshold;
	axis_type *axes;
	keybinding **buttons;
	int numhats;
	hat_type *hats;
} joystick;

static joystick *joysticks;
static unsigned int joycount;

#if SDL_MAJOR_VERSION > 1

typedef struct vcontrol_controller_list
{
	joystick gamepad;
	struct vcontrol_controller_list *next;
	SDL_JoystickID instance_id;
} controller_list;

static controller_list *controller_list_head = NULL;
static int controller_assignments[2] = { -1, -1 };
static int active_controller_count = 0;

typedef struct
{
	int is_axis_binding;
	int axis;
	int polarity;
	int button;
	int *target;
	int logical_port;
} default_binding;

static default_binding *default_bindings = NULL;
static int default_binding_count = 0;
static int default_binding_capacity = 0;

#endif // SDL_MAJOR_VERSION
#endif /* HAVE_JOYSTICK */

static keybinding *bindings[KEYBOARD_INPUT_BUCKETS];

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
			x->pool[i].keycode = SDLK_UNKNOWN;
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
#if SDL_MAJOR_VERSION > 1

static void
store_default_binding (int is_axis_binding, int axis, int polarity,
		int button, int *target, int logical_port)
{
	default_binding *new_bindings, *binding;
	int new_capacity;

	if (default_binding_count >= default_binding_capacity)
	{
		new_capacity = default_binding_capacity == 0
				? 16 : default_binding_capacity * 2;
		new_bindings = HRealloc (default_bindings,
				sizeof (default_binding) *new_capacity);

		if (!new_bindings)
			return;

		default_bindings = new_bindings;
		default_binding_capacity = new_capacity;
	}

	binding = &default_bindings[default_binding_count++];
	binding->is_axis_binding = is_axis_binding;
	binding->axis = axis;
	binding->polarity = polarity;
	binding->button = button;
	binding->target = target;
	binding->logical_port = logical_port;
}

static void
apply_default_bindings_to_controller (SDL_JoystickID instance_id,
		int logical_port)
{
	default_binding *binding;

	if (!default_bindings)
		return;

	for (int i = 0; i < default_binding_count; i++)
	{
		binding = &default_bindings[i];

		if (binding->logical_port != logical_port)
			continue;

		if (binding->is_axis_binding)
		{
			VControl_AddJoyAxisBinding (instance_id, binding->axis,
					binding->polarity, binding->target);
		}
		else
		{
			VControl_AddJoyButtonBinding (instance_id,
					binding->button, binding->target);
		}
	}

	log_add (log_Debug, "Applied %d bindings for logical port %d "
			"(instance %d)", default_binding_count, logical_port,
			instance_id);
}

void
create_joystick (int device_index)
{
	SDL_GameController *gamecontroller;
	SDL_Joystick *gamepad;
	SDL_JoystickID instance_id;
	controller_list *current, *new_controller;
	joystick *x;

	if (!SDL_IsGameController (device_index))
		return;

	gamecontroller = SDL_GameControllerOpen (device_index);
	if (!gamecontroller)
	{
		log_add (log_Warning, "VControl: Could not initialize "
				"controller #%d", device_index);
		return;
	}

	gamepad = SDL_GameControllerGetJoystick (gamecontroller);
	instance_id = SDL_JoystickInstanceID (gamepad);

	current = controller_list_head;
	while (current)
	{
		if (current->instance_id == instance_id)
		{
			log_add (log_Warning, "Controller instance %d already in list",
					instance_id);
			SDL_GameControllerClose (gamecontroller);
			return;
		}
		current = current->next;
	}

	new_controller = HMalloc (sizeof (controller_list));
	if (!new_controller)
	{
		log_add (log_Warning,
				"Failed to allocate memory for new controller");
		SDL_GameControllerClose (gamecontroller);
		return;
	}

	x = &new_controller->gamepad;
	x->numaxes = SDL_CONTROLLER_AXIS_MAX;
	x->numbuttons = SDL_CONTROLLER_BUTTON_MAX;
	x->axes = HMalloc (sizeof (axis_type) *x->numaxes);
	x->buttons = HMalloc (sizeof (keybinding *) *x->numbuttons);

	for (int j = 0; j < x->numaxes; j++)
	{
		x->axes[j].neg = x->axes[j].pos = NULL;
		x->axes[j].polarity = 0;
	}
	for (int j = 0; j < x->numbuttons; j++)
	{
		x->buttons[j] = NULL;
	}
	x->stick = gamecontroller;
	x->threshold = 10000;

	new_controller->instance_id = instance_id;
	new_controller->next = controller_list_head;
	controller_list_head = new_controller;

	int logical_port = -1;
	for (int i = 0; i < 2; i++)
	{
		if (controller_assignments[i] == -1)
		{
			logical_port = i;
			controller_assignments[i] = instance_id;
			active_controller_count++;
			break;
		}
	}

	if (logical_port == -1)
	{
		log_add (log_Info, "Maximum of 2 controllers already connected, "
				"ignoring additional controller");
		SDL_GameControllerClose (gamecontroller);
		HFree (x->axes);
		HFree (x->buttons);
		HFree (new_controller);
		return;
	}

	log_add (log_Info, "VControl opened controller %d (logical port %d): "
			"%s (instance %d)", device_index, logical_port,
			SDL_GameControllerName (gamecontroller), instance_id);

	apply_default_bindings_to_controller (instance_id, logical_port);
}

static void
destroy_joystick (SDL_JoystickID instance_id)
{
	controller_list **prev;
	controller_list *current;

	for (int i = 0; i < 2; i++)
	{
		if (controller_assignments[i] == instance_id)
		{
			controller_assignments[i] = -1;
			active_controller_count--;
			break;
		}
	}

	prev = &controller_list_head;
	current = controller_list_head;

	while (current)
	{
		if (current->instance_id == instance_id)
		{
			joystick *x;

			*prev = current->next;

			x = &current->gamepad;
			if (x->stick)
			{
				SDL_GameControllerClose (x->stick);
				x->stick = NULL;
			}

			HFree (x->axes);
			HFree (x->buttons);
			HFree (current);

			log_add (log_Info,
					"Controller instance %d removed", instance_id);
			return;
		}
		prev = &current->next;
		current = current->next;
	}

	log_add (log_Warning, "Controller instance %d not found for removal",
			instance_id);
}

#else

void
create_joystick (int index)
{
	SDL_Joystick *stick;
	int axes, buttons, hats;
	if ((unsigned int) index >= joycount)
	{
		log_add (log_Warning, "VControl warning: Tried to open a "
				"non-existent joystick!");
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
#if SDL_MAJOR_VERSION == 1
		log_add (log_Info, "VControl opened joystick: %s",
				SDL_JoystickName (index));
#else
		log_add (log_Info, "VControl opened joystick: %s",
				SDL_JoystickName (stick));
#endif
		axes = SDL_JoystickNumAxes (stick);
		buttons = SDL_JoystickNumButtons (stick);
		hats = SDL_JoystickNumHats (stick);
		log_add (log_Info, "%d axes, %d buttons, %d hats.", axes, buttons,
				hats);
		x->numaxes = axes;
		x->numbuttons = buttons;
		x->numhats = hats;
		x->axes = HMalloc (sizeof (axis_type) * axes);
		x->buttons = HMalloc (sizeof (keybinding *) * buttons);
		x->hats = HMalloc (sizeof (hat_type) * hats);
		for (j = 0; j < axes; j++)
		{
			x->axes[j].neg = x->axes[j].pos = NULL;
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
		log_add (log_Warning,
				"VControl: Could not initialize joystick #%d", index);
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

#endif // SDL_MAJOR_VERSION
#endif /* HAVE_JOYSTICK */

static void
key_init (void)
{
	unsigned int i;
	pool = allocate_key_chunk ();
	for (i = 0; i < KEYBOARD_INPUT_BUCKETS; i++)
		bindings[i] = NULL;

#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION > 1
	for (i = 0; i < 2; i++)
	{
		controller_assignments[i] = -1;
	}
	active_controller_count = 0;
	controller_list_head = NULL;
	default_bindings = NULL;
	default_binding_count = 0;
	default_binding_capacity = 0;
#else
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
#endif // SDL_MAJOR_VERSION
#else
# if SDL_MAJOR_VERSION > 1
	active_controller_count = 0;
# else
	joycount = 0;
# endif // SDL_MAJOR_VERSION
#endif /* HAVE_JOYSTICK */
}

static void
key_uninit (void)
{
	unsigned int i;
	free_key_pool (pool);
	for (i = 0; i < KEYBOARD_INPUT_BUCKETS; i++)
		bindings[i] = NULL;
	pool = NULL;

#ifdef HAVE_JOYSTICK
# if SDL_MAJOR_VERSION > 1
	controller_list *current = controller_list_head;
	controller_list *next;
	
	while (current)
	{
		next = current->next;
		destroy_joystick (current->instance_id);
		current = next;
	}
	controller_list_head = NULL;
	active_controller_count = 0;

	// Free binding tracking
	HFree (default_bindings);
	default_bindings = NULL;
	default_binding_count = 0;
	default_binding_capacity = 0;
# else
	// SDL1: Original array-based cleanup
	for (i = 0; i < joycount; i++)
		destroy_joystick (i);
	HFree (joysticks);
# endif // SDL_MAJOR_VERSION
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
#if SDL_MAJOR_VERSION == 1
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joysticks[port].threshold = threshold;
		return 0;
	}
#else
	if (port >= 0)
	{
		controller_list *current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				current->gamepad.threshold = threshold;
				return 0;
			}
			current = current->next;
		}
		return 0;
	}
#endif
	else
#else
	(void) port;
	(void) threshold;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Warning, "VControl_SetJoyThreshold passed "
		//	"illegal port %d", port);
		return -1;
	}
}

static void
add_binding (keybinding **newptr, int *target, sdl_key_t keycode)
{
	keybinding *newbinding;
	keypool *searchbase;
	int i;

	/* Acquire a pointer to the keybinding * that we'll be
	 * overwriting.  Along the way, ensure we haven't already
	 * bound this symbol to this target.  If we have, return.*/
	while (*newptr != NULL)
	{
		if (((*newptr)->target == target)
			&& ((*newptr)->keycode == keycode))
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
		log_add (log_Warning,
				"add_binding failed to find a free binding slot!");
		return;
	}

	newbinding->target = target;
	newbinding->keycode = keycode;
	newbinding->next = NULL;
	*newptr = newbinding;
	searchbase->remaining--;
}

static void
remove_binding (keybinding **ptr, int *target, sdl_key_t keycode)
{
	if (!(*ptr))
	{
		/* Nothing bound to symbol; return. */
		return;
	}
	else if (((*ptr)->target == target) && ((*ptr)->keycode == keycode))
	{
		keybinding *todel = *ptr;
		*ptr = todel->next;
		todel->target = NULL;
		todel->keycode = SDLK_UNKNOWN;
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
				todel->keycode = SDLK_UNKNOWN;
				todel->next = NULL;
				todel->parent->remaining++;
			}
			prev = prev->next;
		}
	}
}

static void
activate (keybinding *i, sdl_key_t keycode)
{
	while (i != NULL)
	{
		if (i->keycode == keycode)
		{
			*(i->target) = (*(i->target) + 1) | VCONTROL_STARTBIT;
		}
		i = i->next;
	}
}

static void
deactivate (keybinding *i, sdl_key_t keycode)
{
	while (i != NULL)
	{
		int v = *(i->target) & VCONTROL_MASK;
		if ((i->keycode == keycode) && (v > 0))
		{
			*(i->target) = (v - 1) | (*(i->target) & VCONTROL_STARTBIT);
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
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION > 1
	case SDL_CONTROLLERAXISMOTION:
		g->type = VCONTROL_JOYAXIS;
		g->gesture.axis.port = e->caxis.which;
		g->gesture.axis.index = e->caxis.axis;
		g->gesture.axis.polarity = (e->caxis.value < 0) ? -1 : 1;
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		g->type = VCONTROL_JOYBUTTON;
		g->gesture.button.port = e->cbutton.which;
		g->gesture.button.index = e->cbutton.button;
		break;
#else
	case SDL_JOYAXISMOTION:
		g->type = VCONTROL_JOYAXIS;
		g->gesture.axis.port = e->jaxis.which;
		g->gesture.axis.index = e->jaxis.axis;
		g->gesture.axis.polarity = (e->jaxis.value < 0) ? -1 : 1;
		break;
	case SDL_JOYBUTTONDOWN:
		g->type = VCONTROL_JOYBUTTON;
		g->gesture.button.port = e->jbutton.which;
		g->gesture.button.index = e->jbutton.button;
		break;
	case SDL_JOYHATMOTION:
		g->type = VCONTROL_JOYHAT;
		g->gesture.hat.port = e->jhat.which;
		g->gesture.hat.index = e->jhat.hat;
		g->gesture.hat.dir = e->jhat.value;
		break;
#endif // SDL_MAJOR_VERSION
#endif /* HAVE_JOYSTICK */
	default:
		g->type = VCONTROL_NONE;
		break;
	}
}

int
VControl_AddGestureBinding (VCONTROL_GESTURE *g, int *target)
{
	int result = -1;
	int logical_port;
	switch (g->type)
	{
	case VCONTROL_KEY:
		result = VControl_AddKeyBinding (g->gesture.key, target);
		break;

	case VCONTROL_JOYAXIS:
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
		result = VControl_AddJoyAxisBinding (g->gesture.axis.port,
				g->gesture.axis.index,
				(g->gesture.axis.polarity < 0) ? -1 : 1, target);
#else
		logical_port = g->gesture.axis.port;

		store_default_binding (1, g->gesture.axis.index,
				(g->gesture.axis.polarity < 0) ? -1 : 1,
				-1, target, logical_port);

		if (logical_port >= 0 && logical_port < 2 &&
				controller_assignments[logical_port] != -1)
		{
			result = VControl_AddJoyAxisBinding (
					controller_assignments[logical_port],
					g->gesture.axis.index,
					(g->gesture.axis.polarity < 0) ? -1 : 1, target);
		}
		else
			result = 0;
#endif
		break;
#endif
#if SDL_MAJOR_VERSION == 1
	case VCONTROL_JOYHAT:
#ifdef HAVE_JOYSTICK
		result = VControl_AddJoyHatBinding (g->gesture.hat.port,
				g->gesture.hat.index, g->gesture.hat.dir, target);
		break;
#endif
#endif
	case VCONTROL_JOYBUTTON:
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
		result = VControl_AddJoyButtonBinding (g->gesture.button.port,
				g->gesture.button.index, target);
#else
		logical_port = g->gesture.button.port;

		store_default_binding (0, -1, 0,
				g->gesture.button.index,
				target, logical_port);

		if (logical_port >= 0 && logical_port < 2 &&
				controller_assignments[logical_port] != -1)
		{
			result = VControl_AddJoyButtonBinding (
					controller_assignments[logical_port],
					g->gesture.button.index, target);
		}
		else
			result = 0;
#endif
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_NONE:
		/* Do nothing */
		break;

	default:
		log_add (log_Warning, "VControl_AddGestureBinding didn't "
				"understand argument gesture");
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
#if SDL_MAJOR_VERSION == 1
		VControl_RemoveJoyAxisBinding (g->gesture.axis.port,
				g->gesture.axis.index,
				(g->gesture.axis.polarity < 0) ? -1 : 1, target);
#else
		VControl_RemoveJoyAxisBinding (g->gesture.axis.port,
				g->gesture.axis.index,
				(g->gesture.axis.polarity < 0) ? -1 : 1, target);
#endif
		break;
#endif /* HAVE_JOYSTICK */
#if SDL_MAJOR_VERSION == 1
	case VCONTROL_JOYHAT:
#ifdef HAVE_JOYSTICK
		VControl_RemoveJoyHatBinding (g->gesture.hat.port,
				g->gesture.hat.index, g->gesture.hat.dir, target);
		break;
#endif /* HAVE_JOYSTICK */
#endif
	case VCONTROL_JOYBUTTON:
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
		VControl_RemoveJoyButtonBinding (g->gesture.button.port,
				g->gesture.button.index, target);
#else
		VControl_RemoveJoyButtonBinding (g->gesture.button.port,
				g->gesture.button.index, target);
#endif
		break;
#endif /* HAVE_JOYSTICK */
	case VCONTROL_NONE:
		break;
	default:
		log_add (log_Warning, "VControl_RemoveGestureBinding didn't "
				"understand argument gesture");
		break;
	}
}

int
VControl_AddKeyBinding (sdl_key_t symbol, int *target)
{
	add_binding (&bindings[symbol % KEYBOARD_INPUT_BUCKETS], target,
			symbol);
	return 0;
}

void
VControl_RemoveKeyBinding (sdl_key_t symbol, int *target)
{
	remove_binding (&bindings[symbol % KEYBOARD_INPUT_BUCKETS], target,
			symbol);
}

int
VControl_AddJoyAxisBinding (int port, int axis, int polarity, int *target)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				add_binding (&joysticks[port].axes[axis].neg, target,
						SDLK_UNKNOWN);
			}
			else if (polarity > 0)
			{
				add_binding (&joysticks[port].axes[axis].pos, target,
						SDLK_UNKNOWN);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to bind to "
						"polarity zero");
				return -1;
			}
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to "
			//	"illegal axis %d", axis);
			return -1;
		}
	}
#else
	if (port >= 0)
	{
		controller_list *current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick *j = &current->gamepad;

				if ((axis >= 0) && (axis < j->numaxes))
				{
					if (polarity < 0)
					{
						add_binding (&j->axes[axis].neg, target,
							SDLK_UNKNOWN);
					}
					else if (polarity > 0)
					{
						add_binding (&j->axes[axis].pos, target,
							SDLK_UNKNOWN);
					}
					else
					{
						log_add (log_Debug, "VControl: Attempted to bind "
								"to polarity zero");
						return -1;
					}
				}
				else
				{
					// log_add (log_Debug, "VControl: Attempted to bind to"
					//	" illegal axis %d", axis);
					return -1;
				}
			}
			current = current->next;
		}
	}
#endif
	else
#else
	(void) port;
	(void) axis;
	(void) polarity;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		// log_add (log_Debug, "VControl: Attempted to bind to illegal "
		//	"port %d", port);
		return -1;
	}
	return 0;
}

void
VControl_RemoveJoyAxisBinding (int port, int axis,
		int polarity, int *target)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((axis >= 0) && (axis < j->numaxes))
		{
			if (polarity < 0)
			{
				remove_binding (&joysticks[port].axes[axis].neg, target,
						SDLK_UNKNOWN);
			}
			else if (polarity > 0)
			{
				remove_binding (&joysticks[port].axes[axis].pos, target,
						SDLK_UNKNOWN);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to unbind from "
						"polarity zero");
			}
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from "
					"illegal axis %d", axis);
		}
	}
#else
	if (port >= 0)
	{
		controller_list *current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick *j = &current->gamepad;
				if ((axis >= 0) && (axis < j->numaxes))
				{
					if (polarity < 0)
					{
						remove_binding (&j->axes[axis].neg, target,
								SDLK_UNKNOWN);
					}
					else if (polarity > 0)
					{
						remove_binding (&j->axes[axis].pos, target,
								SDLK_UNKNOWN);
					}
					else
					{
						log_add (log_Debug, "VControl: Attempted to "
								"unbind from polarity zero");
					}
				}
				else
				{
					log_add (log_Debug, "VControl: Attempted to unbind "
							"from illegal axis %d", axis);
				}
				return;
			}
			current = current->next;
		}
	}
#endif
	else
#else
	(void) port;
	(void) axis;
	(void) polarity;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal "
				"port %d", port);
	}
}

int
VControl_AddJoyButtonBinding (int port, int button, int *target)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			add_binding (&joysticks[port].buttons[button], target,
				SDLK_UNKNOWN);
			return 0;
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to "
			//	"illegal button %d", button);
			return -1;
		}
	}
#else
	if (port >= 0)
	{
		controller_list *current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick *j = &current->gamepad;

				if ((button >= 0) && (button < j->numbuttons))
				{
					add_binding (&j->buttons[button], target,
							SDLK_UNKNOWN);
				}
				else
				{
					// log_add (log_Debug, "VControl: Attempted to bind to"
					//	" illegal button %d", button);
					return -1;
				}
			}
			current = current->next;
		}
		return 0;
	}
#endif
	else
#else
	(void) port;
	(void) button;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal "
				"port %d", port);
		return -1;
	}
}

void
VControl_RemoveJoyButtonBinding (int port, int button, int *target)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (port >= 0 && (unsigned int) port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((button >= 0) && (button < j->numbuttons))
		{
			remove_binding (&joysticks[port].buttons[button], target,
					SDLK_UNKNOWN);
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from "
					"illegal button %d", button);
		}
	}
#else
	if (port >= 0)
	{
		controller_list *current = controller_list_head;
		while (current)
		{
			if (current->instance_id == port)
			{
				joystick *j = &current->gamepad;
				if ((button >= 0) && (button < j->numbuttons))
				{
					remove_binding (&j->buttons[button], target,
							SDLK_UNKNOWN);
				}
				else
				{
					log_add (log_Debug, "VControl: Attempted to unbind "
							"from illegal button %d", button);
				}
				return;
			}
			current = current->next;
		}
	}
#endif
	else
#else
	(void) port;
	(void) button;
	(void) target;
#endif /* HAVE_JOYSTICK */
	{
		log_add (log_Debug, "VControl: Attempted to unbind from illegal "
				"port %d", port);
	}
}

#if SDL_MAJOR_VERSION == 1

int
VControl_AddJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				add_binding (&joysticks[port].hats[which].left, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				add_binding (&joysticks[port].hats[which].right, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_UP)
			{
				add_binding (&joysticks[port].hats[which].up, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				add_binding (&joysticks[port].hats[which].down, target,
						SDLK_UNKNOWN);
			}
			else
			{
				// log_add (log_Debug, "VControl: Attempted to bind to "
				//	"illegal direction");
				return -1;
			}
			return 0;
		}
		else
		{
			// log_add (log_Debug, "VControl: Attempted to bind to "
			//	"illegal hat %d", which);
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
		// log_add (log_Debug, "VControl: Attempted to bind to illegal "
		//	"port %d", port);
		return -1;
	}
}

void
VControl_RemoveJoyHatBinding (int port, int which, Uint8 dir, int *target)
{
#ifdef HAVE_JOYSTICK
	if (port >= 0 && (unsigned int)port < joycount)
	{
		joystick *j = &joysticks[port];
		if (!(j->stick))
			create_joystick (port);
		if ((which >= 0) && (which < j->numhats))
		{
			if (dir == SDL_HAT_LEFT)
			{
				remove_binding (&joysticks[port].hats[which].left, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_RIGHT)
			{
				remove_binding (&joysticks[port].hats[which].right, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_UP)
			{
				remove_binding (&joysticks[port].hats[which].up, target,
						SDLK_UNKNOWN);
			}
			else if (dir == SDL_HAT_DOWN)
			{
				remove_binding (&joysticks[port].hats[which].down, target,
						SDLK_UNKNOWN);
			}
			else
			{
				log_add (log_Debug, "VControl: Attempted to unbind from "
						"illegal direction");
			}
		}
		else
		{
			log_add (log_Debug, "VControl: Attempted to unbind from "
					"illegal hat %d", which);
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
		log_add (log_Debug,
				"VControl: Attempted to unbind from illegal port %d", port);
	}
}

#endif // SDL_MAJOR_VERSION

void
VControl_RemoveAllBindings (void)
{
	key_uninit ();
	key_init ();
}

void
VControl_ProcessKeyDown (sdl_key_t symbol)
{
	activate (bindings[symbol % KEYBOARD_INPUT_BUCKETS], symbol);
}

void
VControl_ProcessKeyUp (sdl_key_t symbol)
{
	deactivate (bindings[symbol % KEYBOARD_INPUT_BUCKETS], symbol);
}

void
VControl_ProcessJoyButtonDown (int port, int button)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (!joysticks[port].stick)
		return;
	activate (joysticks[port].buttons[button], SDLK_UNKNOWN);
#else
	controller_list *current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			activate (current->gamepad.buttons[button], SDLK_UNKNOWN);
			return;
		}
		current = current->next;
	}
#endif
#else
	(void) port;
	(void) button;
#endif /* HAVE_JOYSTICK */
}

void
VControl_ProcessJoyButtonUp (int port, int button)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	if (!joysticks[port].stick)
		return;
	deactivate (joysticks[port].buttons[button], SDLK_UNKNOWN);
#else
	controller_list *current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			deactivate (current->gamepad.buttons[button], SDLK_UNKNOWN);
			return;
		}
		current = current->next;
	}
#endif
#else
	(void) port;
	(void) button;
#endif /* HAVE_JOYSTICK */
}

void
VControl_ProcessJoyAxis (int port, int axis, int value)
{
#ifdef HAVE_JOYSTICK
#if SDL_MAJOR_VERSION == 1
	int t;
	if (!joysticks[port].stick)
		return;
	t = joysticks[port].threshold;
	if (value > t)
	{
		if (joysticks[port].axes[axis].polarity != 1)
		{
			if (joysticks[port].axes[axis].polarity == -1)
			{
				deactivate (joysticks[port].axes[axis].neg, SDLK_UNKNOWN);
			}
			joysticks[port].axes[axis].polarity = 1;
			activate (joysticks[port].axes[axis].pos, SDLK_UNKNOWN);
		}
	}
	else if (value < -t)
	{
		if (joysticks[port].axes[axis].polarity != -1)
		{
			if (joysticks[port].axes[axis].polarity == 1)
			{
				deactivate (joysticks[port].axes[axis].pos, SDLK_UNKNOWN);
			}
			joysticks[port].axes[axis].polarity = -1;
			activate (joysticks[port].axes[axis].neg, SDLK_UNKNOWN);
		}
	}
	else
	{
		if (joysticks[port].axes[axis].polarity == -1)
		{
			deactivate (joysticks[port].axes[axis].neg, SDLK_UNKNOWN);
		}
		else if (joysticks[port].axes[axis].polarity == 1)
		{
			deactivate (joysticks[port].axes[axis].pos, SDLK_UNKNOWN);
		}
		joysticks[port].axes[axis].polarity = 0;
	}
#else
	controller_list *current = controller_list_head;
	while (current)
	{
		if (current->instance_id == port)
		{
			joystick *j = &current->gamepad;
			int t = j->threshold;

			if (axis < 0 || axis >= j->numaxes)
				return;

			if (value > t)
			{
				if (j->axes[axis].polarity != 1)
				{
					if (j->axes[axis].polarity == -1)
					{
						deactivate (j->axes[axis].neg, SDLK_UNKNOWN);
					}
					j->axes[axis].polarity = 1;
					activate (j->axes[axis].pos, SDLK_UNKNOWN);
				}
			}
			else if (value < -t)
			{
				if (j->axes[axis].polarity != -1)
				{
					if (j->axes[axis].polarity == 1)
					{
						deactivate (j->axes[axis].pos, SDLK_UNKNOWN);
					}
					j->axes[axis].polarity = -1;
					activate (j->axes[axis].neg, SDLK_UNKNOWN);
				}
			}
			else
			{
				if (j->axes[axis].polarity == -1)
				{
					deactivate (j->axes[axis].neg, SDLK_UNKNOWN);
				}
				else if (j->axes[axis].polarity == 1)
				{
					deactivate (j->axes[axis].pos, SDLK_UNKNOWN);
				}
				j->axes[axis].polarity = 0;
			}
			return;
		}
		current = current->next;
	}
#endif
#else
	(void) port;
	(void) axis;
	(void) value;
#endif /* HAVE_JOYSTICK */
}

#if SDL_MAJOR_VERSION == 1

void
VControl_ProcessJoyHat (int port, int which, Uint8 value)
{
#ifdef HAVE_JOYSTICK
	Uint8 old;
	if (!joysticks[port].stick)
		return;
	old = joysticks[port].hats[which].last;
	if (!(old & SDL_HAT_LEFT) && (value & SDL_HAT_LEFT))
		activate (joysticks[port].hats[which].left, SDLK_UNKNOWN);
	if (!(old & SDL_HAT_RIGHT) && (value & SDL_HAT_RIGHT))
		activate (joysticks[port].hats[which].right, SDLK_UNKNOWN);
	if (!(old & SDL_HAT_UP) && (value & SDL_HAT_UP))
		activate (joysticks[port].hats[which].up, SDLK_UNKNOWN);
	if (!(old & SDL_HAT_DOWN) && (value & SDL_HAT_DOWN))
		activate (joysticks[port].hats[which].down, SDLK_UNKNOWN);
	if ((old & SDL_HAT_LEFT) && !(value & SDL_HAT_LEFT))
		deactivate (joysticks[port].hats[which].left, SDLK_UNKNOWN);
	if ((old & SDL_HAT_RIGHT) && !(value & SDL_HAT_RIGHT))
		deactivate (joysticks[port].hats[which].right, SDLK_UNKNOWN);
	if ((old & SDL_HAT_UP) && !(value & SDL_HAT_UP))
		deactivate (joysticks[port].hats[which].up, SDLK_UNKNOWN);
	if ((old & SDL_HAT_DOWN) && !(value & SDL_HAT_DOWN))
		deactivate (joysticks[port].hats[which].down, SDLK_UNKNOWN);
	joysticks[port].hats[which].last = value;
#else
	(void) port;
	(void) which;
	(void) value;
#endif /* HAVE_JOYSTICK */
}

#endif // SDL_MAJOR_VERSION

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
			if (base->pool[i].target)
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
			if (base->pool[i].target)
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
#if SDL_MAJOR_VERSION > 1
			if (!e->key.repeat)
#endif
			{
				VControl_ProcessKeyDown (e->key.keysym.sym);
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_KEYUP:
			VControl_ProcessKeyUp (e->key.keysym.sym);
			break;

#ifdef HAVE_JOYSTICK
# if SDL_MAJOR_VERSION > 1
		case SDL_CONTROLLERAXISMOTION:
			VControl_ProcessJoyAxis (e->caxis.which, e->caxis.axis,
					e->caxis.value);
			if ((e->caxis.value > 15000) || (e->caxis.value < -15000))
			{
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			VControl_ProcessJoyButtonDown (e->cbutton.which,
					e->cbutton.button);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_CONTROLLERBUTTONUP:
			VControl_ProcessJoyButtonUp (e->cbutton.which,
					e->cbutton.button);
			break;
		case SDL_CONTROLLERDEVICEADDED:
			create_joystick (e->cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			destroy_joystick (e->cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMAPPED:
			log_add (log_Info, "Controller mapping updated");
			break;
# else
		case SDL_JOYAXISMOTION:
			VControl_ProcessJoyAxis (
					e->jaxis.which, e->jaxis.axis, e->jaxis.value);
			if ((e->jaxis.value > 15000) || (e->jaxis.value < -15000))
			{
				last_interesting = *e;
				event_ready = 1;
			}
			break;
		case SDL_JOYHATMOTION:
			VControl_ProcessJoyHat (
					e->jhat.which, e->jhat.hat, e->jhat.value);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONDOWN:
			VControl_ProcessJoyButtonDown (
					e->jbutton.which, e->jbutton.button);
			last_interesting = *e;
			event_ready = 1;
			break;
		case SDL_JOYBUTTONUP:
			VControl_ProcessJoyButtonUp (
					e->jbutton.which, e->jbutton.button);
			break;
# endif // SDL_MAJOR_VERSION
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
		event2gesture (&last_interesting, g);
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
	while (index < (TOKEN_SIZE-1) && state->line[base+index]
			&& !isspace (state->line[base+index]))
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
		log_add (log_Warning, "VControl: Illegal key name '%s' on config "
				"file line %d", state->token, state->linenum);
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
		log_add (log_Warning,
				"VControl: Expected integer on config line %d",
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
		return snprintf (buf, n, "key %s",
				VControl_code2name (g->gesture.key));
	case VCONTROL_JOYAXIS:
		return snprintf (buf, n, "joystick %d axis %d %s",
				g->gesture.axis.port, g->gesture.axis.index,
				(g->gesture.axis.polarity > 0) ? "positive" : "negative");
	case VCONTROL_JOYBUTTON:
		return snprintf (buf, n, "joystick %d button %d",
				g->gesture.button.port, g->gesture.button.index);

#if SDL_MAJOR_VERSION == 1
	case VCONTROL_JOYHAT:
		return snprintf (buf, n, "joystick %d hat %d %s",
				g->gesture.hat.port, g->gesture.hat.index, 
				(g->gesture.hat.dir == SDL_HAT_UP) ? "up" :
				((g->gesture.hat.dir == SDL_HAT_DOWN) ? "down" : 
				((g->gesture.hat.dir == SDL_HAT_LEFT) ? "left" : "right"))
			);
#endif // SDL_MAJOR_VERSION
		default:
			buf[0] = '\0';
			return 0;
	}
}