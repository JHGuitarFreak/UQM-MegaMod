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
#include <string.h>
#include "keynames.h"

/* This code is adapted from the code in SDL_keysym.h.  Though this
 * would almost certainly be fast if we were to use a direct char *
 * array, this technique permits us to be independent of the actual
 * character encoding to keysyms. */

/* These names are case-insensitive when compared, but we format 
 * them to look pretty when output */

/* This version of Virtual Controller does not support SDLK_WORLD_*
 * keysyms or the Num/Caps/ScrollLock keys.  SDL treats locking keys
 * specially, and we cannot treat them as normal keys.  Pain, 
 * tragedy. */

typedef struct vcontrol_keyname {
	const char *name;
	int code;
} keyname;

static keyname keynames[] = {
	{"Backspace", SDLK_BACKSPACE},
	{"Tab", SDLK_TAB},
	{"Clear", SDLK_CLEAR},
	{"Return", SDLK_RETURN},
	{"Pause", SDLK_PAUSE},
	{"Escape", SDLK_ESCAPE},
	{"Space", SDLK_SPACE},
	{"!", SDLK_EXCLAIM},
	{"\"", SDLK_QUOTEDBL},
	{"Hash", SDLK_HASH},
	{"$", SDLK_DOLLAR},
	{"&", SDLK_AMPERSAND},
	{"'", SDLK_QUOTE},
	{"(", SDLK_LEFTPAREN},
	{")", SDLK_RIGHTPAREN},
	{"*", SDLK_ASTERISK},
	{"+", SDLK_PLUS},
	{",", SDLK_COMMA},
	{"-", SDLK_MINUS},
	{".", SDLK_PERIOD},
	{"/", SDLK_SLASH},
	{"0", SDLK_0},
	{"1", SDLK_1},
	{"2", SDLK_2},
	{"3", SDLK_3},
	{"4", SDLK_4},
	{"5", SDLK_5},
	{"6", SDLK_6},
	{"7", SDLK_7},
	{"8", SDLK_8},
	{"9", SDLK_9},
	{":", SDLK_COLON},
	{";", SDLK_SEMICOLON},
	{"<", SDLK_LESS},
	{"=", SDLK_EQUALS},
	{">", SDLK_GREATER},
	{"?", SDLK_QUESTION},
	{"@", SDLK_AT},
	{"[", SDLK_LEFTBRACKET},
	{"\\", SDLK_BACKSLASH},
	{"]", SDLK_RIGHTBRACKET},
	{"^", SDLK_CARET},
	{"_", SDLK_UNDERSCORE},
	{"`", SDLK_BACKQUOTE},
	{"a", SDLK_a},
	{"b", SDLK_b},
	{"c", SDLK_c},
	{"d", SDLK_d},
	{"e", SDLK_e},
	{"f", SDLK_f},
	{"g", SDLK_g},
	{"h", SDLK_h},
	{"i", SDLK_i},
	{"j", SDLK_j},
	{"k", SDLK_k},
	{"l", SDLK_l},
	{"m", SDLK_m},
	{"n", SDLK_n},
	{"o", SDLK_o},
	{"p", SDLK_p},
	{"q", SDLK_q},
	{"r", SDLK_r},
	{"s", SDLK_s},
	{"t", SDLK_t},
	{"u", SDLK_u},
	{"v", SDLK_v},
	{"w", SDLK_w},
	{"x", SDLK_x},
	{"y", SDLK_y},
	{"z", SDLK_z},
	{"Delete", SDLK_DELETE},
#if SDL_MAJOR_VERSION == 1
	{"Keypad-0", SDLK_KP0},
	{"Keypad-1", SDLK_KP1},
	{"Keypad-2", SDLK_KP2},
	{"Keypad-3", SDLK_KP3},
	{"Keypad-4", SDLK_KP4},
	{"Keypad-5", SDLK_KP5},
	{"Keypad-6", SDLK_KP6},
	{"Keypad-7", SDLK_KP7},
	{"Keypad-8", SDLK_KP8},
	{"Keypad-9", SDLK_KP9},
#else
	{"Keypad-0", SDLK_KP_0},
	{"Keypad-1", SDLK_KP_1},
	{"Keypad-2", SDLK_KP_2},
	{"Keypad-3", SDLK_KP_3},
	{"Keypad-4", SDLK_KP_4},
	{"Keypad-5", SDLK_KP_5},
	{"Keypad-6", SDLK_KP_6},
	{"Keypad-7", SDLK_KP_7},
	{"Keypad-8", SDLK_KP_8},
	{"Keypad-9", SDLK_KP_9},
#endif
	{"Keypad-.", SDLK_KP_PERIOD},
	{"Keypad-/", SDLK_KP_DIVIDE},
	{"Keypad-*", SDLK_KP_MULTIPLY},
	{"Keypad--", SDLK_KP_MINUS},
	{"Keypad-+", SDLK_KP_PLUS},
	{"Keypad-Enter", SDLK_KP_ENTER},
	{"Keypad-=", SDLK_KP_EQUALS},
	{"Up", SDLK_UP},
	{"Down", SDLK_DOWN},
	{"Right", SDLK_RIGHT},
	{"Left", SDLK_LEFT},
	{"Insert", SDLK_INSERT},
	{"Home", SDLK_HOME},
	{"End", SDLK_END},
	{"PageUp", SDLK_PAGEUP},
	{"PageDown", SDLK_PAGEDOWN},
	{"F1", SDLK_F1},
	{"F2", SDLK_F2},
	{"F3", SDLK_F3},
	{"F4", SDLK_F4},
	{"F5", SDLK_F5},
	{"F6", SDLK_F6},
	{"F7", SDLK_F7},
	{"F8", SDLK_F8},
	{"F9", SDLK_F9},
	{"F10", SDLK_F10},
	{"F11", SDLK_F11},
	{"F12", SDLK_F12},
	{"F13", SDLK_F13},
	{"F14", SDLK_F14},
	{"F15", SDLK_F15},
	{"RightShift", SDLK_RSHIFT},
	{"LeftShift", SDLK_LSHIFT},
	{"RightControl", SDLK_RCTRL},
	{"LeftControl", SDLK_LCTRL},
	{"RightAlt", SDLK_RALT},
	{"LeftAlt", SDLK_LALT},
#if SDL_MAJOR_VERSION == 1
	{"RightMeta", SDLK_RMETA},
	{"LeftMeta", SDLK_LMETA},
	{"RightSuper", SDLK_RSUPER},
	{"LeftSuper", SDLK_LSUPER},
	{"AltGr", SDLK_MODE},
	{"Compose", SDLK_COMPOSE},
	{"Help", SDLK_HELP},
	{"Print", SDLK_PRINT},
	{"SysReq", SDLK_SYSREQ},
	{"Break", SDLK_BREAK},
	{"Menu", SDLK_MENU},
	{"Power", SDLK_POWER},
	{"Euro", SDLK_EURO},
	{"Undo", SDLK_UNDO},
#ifdef _WIN32_WCE
	{"App1", SDLK_APP1},
	{"App2", SDLK_APP2},
	{"App3", SDLK_APP3},
	{"App4", SDLK_APP4},
	{"App5", SDLK_APP5},
	{"App6", SDLK_APP6},
#endif /* _WIN32_WCE */
#endif /* SDL_MAJOR_VERSION == 1 */

	{"Unknown", 0}};
/* Last element must have code zero */

const char *
VControl_code2name (int code)
{
	int i = 0;
	while (1)
	{
		int test = keynames[i].code;
		if (test == code || !test)
		{
			return keynames[i].name;
		}
		++i;
	}
}

int
VControl_name2code (const char *name)
{
	int i = 0;
	while (1)
	{
		const char *test = keynames[i].name;
		int code = keynames[i].code;
		if (!strcasecmp(test, name) || !code)
		{
			return code;
		}
		++i;
	}
}
