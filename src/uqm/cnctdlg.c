//Copyright Michael Martin, 2006

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

#ifdef NETPLAY

#include "cnctdlg.h"
#include "controls.h"
#include "colors.h"
#include "gamestr.h"
#include "setup.h"
#include "units.h"
#include "resinst.h"
#include "nameref.h"
#include "libs/graphics/widgets.h"
#include "supermelee/netplay/netoptions.h"

#define MCD_WIDTH RES_SCALE(260) // JMS_GFX
#define MCD_HEIGHT RES_SCALE(110) // JMS_GFX

#define MENU_FRAME_RATE (ONE_SECOND / 20)

typedef struct connect_dialog_state
{
	BOOLEAN (*InputFunc) (struct connect_dialog_state *pInputState);

	DWORD NextTime;
	BOOLEAN Initialized;
	int which_side;
	
	int confirmed;
} CONNECT_DIALOG_STATE;

static void DrawConnectDialog (void);

static WIDGET_MENU_SCREEN menu;
static WIDGET_BUTTON buttons[3];
static WIDGET_SLIDER slider;
static WIDGET_TEXTENTRY texts[2];

static WIDGET *menu_widgets[] = {
	(WIDGET *)&buttons[1],
	(WIDGET *)&texts[0],
	(WIDGET *)&buttons[0],
	(WIDGET *)&slider,
	(WIDGET *)&texts[1],
	(WIDGET *)&buttons[2] };

static BOOLEAN done;

/* This kind of sucks, but the Button callbacks need access to the
 * CONNECT_DIALOG_STATE, so we need a pointer to it */

static CONNECT_DIALOG_STATE *current_state;

static FONT PlayerFont;

static int do_connect (WIDGET *self, int event);
static int do_listen (WIDGET *self, int event);
static int do_cancel (WIDGET *self, int event);

static void
MCD_DrawMenuScreen (WIDGET *_self, int x, int y)
{
	int widget_index, widget_y;

	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	
	widget_y = y + RES_SCALE(8); // JMS_GFX
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{
		WIDGET *c = self->child[widget_index];
		(*c->draw)(c, x, widget_y);
		widget_y += (*c->height)(c) + RES_SCALE(8); // JMS_GFX
	}
}

static void
MCD_DrawButton (WIDGET *_self, int x, int y)
{
	WIDGET_BUTTON *self = (WIDGET_BUTTON *)_self;
	Color oldtext;
	Color inactive, selected;
	FONT  oldfont = SetContextFont (StarConFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	
	selected = MENU_HIGHLIGHT_COLOR;
	inactive = MENU_TEXT_COLOR;

	t.baseline.x = RES_SCALE(160); // JMS_GFX
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	t.pStr = self->name;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (inactive);
	}
	font_DrawText (&t);
	SetContextFontEffect (oldFontEffect);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
	(void) x;
}

static void
MCD_DrawSlider (WIDGET *_self, int x, int y)
{
	WIDGET_SLIDER *self = (WIDGET_SLIDER *)_self;
	Color oldtext;
	Color default_color, selected;
	FONT  oldfont = SetContextFont (PlayerFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	RECT r;
	int tick = (MCD_WIDTH) / 8;
	
	default_color = MENU_TEXT_COLOR;
	selected = MENU_HIGHLIGHT_COLOR;

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (default_color);
	}
	font_DrawText (&t);

	r.corner.x = t.baseline.x + 3 * tick;
	r.corner.y = t.baseline.y - 4;
	r.extent.height = 2;
	r.extent.width = 3 * tick;
	DrawFilledRectangle (&r);

	r.extent.width = 3;
	r.extent.height = 8;
	r.corner.y = t.baseline.y - 7;
	r.corner.x = t.baseline.x + 3 * tick + (3 * tick *
			(self->value - self->min) / (self->max - self->min)) - 1;
	DrawFilledRectangle (&r);

	(*self->draw_value)(self, t.baseline.x + 7 * tick, t.baseline.y);

	SetContextFontEffect (oldFontEffect);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

static void
MCD_DrawTextEntry (WIDGET *_self, int x, int y)
{
	WIDGET_TEXTENTRY *self = (WIDGET_TEXTENTRY *)_self;
	Color oldtext;
	Color inactive, default_color, selected;
	FONT  oldfont = SetContextFont (PlayerFont);
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	
	default_color = MENU_TEXT_COLOR;
	selected = MENU_HIGHLIGHT_COLOR;
	inactive = MENU_TEXT_COLOR;

	BatchGraphics ();

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (default_color);
	}
	font_DrawText (&t);

	/* Force string termination */
	self->value[WIDGET_TEXTENTRY_WIDTH-1] = 0;

	t.baseline.y = y;
	t.CharCount = utf8StringCount (self->value);
	t.pStr = self->value;

	if (!(self->state & WTE_EDITING))
	{	// normal or selected state
		t.baseline.x = RES_SCALE(160); // JMS_GFX
		t.align = ALIGN_CENTER;

		if (widget_focus == _self)
		{
			oldtext = SetContextForeGroundColor (selected);
		}
		else
		{
			oldtext = SetContextForeGroundColor (inactive);
		}
		font_DrawText (&t);
	}
	else
	{	// editing state
		COUNT i;
		RECT text_r;
		BYTE char_deltas[WIDGET_TEXTENTRY_WIDTH];
		BYTE *pchar_deltas;
		RECT r;
		SIZE leading;

		t.baseline.x = x + (RES_SCALE(90)); // JMS_GFX
		t.align = ALIGN_LEFT;

		// calc background box dimensions
		// XXX: this may need some tuning, especially if a
		//   different font is used. The font 'leading' values
		//   are not what they should be.
#define BOX_VERT_OFFSET 2
		GetContextFontLeading (&leading);
		r.corner.x = t.baseline.x - 1;
		r.corner.y = t.baseline.y - leading + BOX_VERT_OFFSET;
		r.extent.width = MCD_WIDTH - r.corner.x - 10;
		r.extent.height = leading + 2;

		TextRect (&t, &text_r, char_deltas);
#if 0
		// XXX: this should potentially be used in ChangeCallback
		if ((text_r.extent.width + 2) >= r.extent.width)
		{	// the text does not fit the input box size and so
			// will not fit when displayed later
			UnbatchGraphics ();
			// disallow the change
			return (FALSE);
		}
#endif

		oldtext = SetContextForeGroundColor (selected);
		DrawFilledRectangle (&r);

		// calculate the cursor position and draw it
		pchar_deltas = char_deltas;
		for (i = self->cursor_pos; i > 0; --i)
			r.corner.x += (SIZE)*pchar_deltas++;
		if (self->cursor_pos < t.CharCount) /* cursor mid-line */
			--r.corner.x;
		if (self->state & WTE_BLOCKCUR)
		{	// Use block cursor for keyboardless systems
			if (self->cursor_pos == t.CharCount)
			{	// cursor at end-line -- use insertion point
				r.extent.width = 1;
			}
			else if (self->cursor_pos + 1 == t.CharCount)
			{	// extra pixel for last char margin
				r.extent.width = (SIZE)*pchar_deltas + 2;
			}
			else
			{	// normal mid-line char
				r.extent.width = (SIZE)*pchar_deltas + 1;
			}
		}
		else
		{	// Insertion point cursor
			r.extent.width = 1;
		}
		// position cursor within input field rect
		++r.corner.x;
		++r.corner.y;
		r.extent.height -= 2;
		SetContextForeGroundColor (MENU_CURSOR_COLOR);
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (inactive);
		font_DrawText (&t);
	}
	
	UnbatchGraphics ();
	SetContextFontEffect (oldFontEffect);
	SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

/* Text entry stuff, mostly C&Ped from setupmenu.c.  Could use some
 * refactoring, as redraw_menu () is the only real change. */

static BOOLEAN
OnTextEntryChange (TEXTENTRY_STATE *pTES)
{
	WIDGET_TEXTENTRY *widget = (WIDGET_TEXTENTRY *) pTES->CbParam;

	widget->cursor_pos = pTES->CursorPos;
	if (pTES->JoystickMode)
		widget->state |= WTE_BLOCKCUR;
	else
		widget->state &= ~WTE_BLOCKCUR;
	
	// XXX TODO: Here, we can examine the text entered so far
	// to make sure it fits on the screen, for example,
	// and return FALSE to disallow the last change
	
	return TRUE; // allow change
}

static BOOLEAN
OnTextEntryFrame (TEXTENTRY_STATE *pTES)
{
	DrawConnectDialog ();

	SleepThreadUntil (pTES->NextTime);
	pTES->NextTime = GetTimeCounter () + MENU_FRAME_RATE;

	(void) pTES;  // satisfying compiler
	return TRUE; // continue
}

static int
OnTextEntryEvent (WIDGET_TEXTENTRY *widget)
{	// Going to edit the text
	TEXTENTRY_STATE tes;
	UNICODE revert_buf[256];

	// position cursor at the end of text
	widget->cursor_pos = utf8StringCount (widget->value);
	widget->state = WTE_EDITING;
	DrawConnectDialog ();

	// make a backup copy for revert on cancel
	utf8StringCopy (revert_buf, sizeof (revert_buf), widget->value);

	// text entry setup
	tes.Initialized = FALSE;
	tes.NextTime = GetTimeCounter () + MENU_FRAME_RATE;
	tes.BaseStr = widget->value;
	tes.MaxSize = widget->maxlen;
	tes.CursorPos = widget->cursor_pos;
	tes.CbParam = widget;
	tes.ChangeCallback = OnTextEntryChange;
	tes.FrameCallback = OnTextEntryFrame;

	// SetMenuSounds (0, MENU_SOUND_SELECT);
	if (!DoTextEntry (&tes))
	{	// editing failed (canceled) -- revert the changes
		utf8StringCopy (widget->value, widget->maxlen, revert_buf);
	}
	else
	{
		if (widget->onChange)
		{
			(*(widget->onChange))(widget);
		}
	}

	widget->state = WTE_NORMAL;
	DrawConnectDialog ();

	return TRUE; // event handled
}

/* Button response routines */

static int
do_connect (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		/* These assignments are safe exactly because texts[] is file-scope) */
		netplayOptions.peer[current_state->which_side].host = texts[0].value;
		netplayOptions.peer[current_state->which_side].port = texts[1].value;
		netplayOptions.peer[current_state->which_side].isServer = FALSE;
		current_state->confirmed = TRUE;
		netplayOptions.inputDelay = slider.value;

		done = TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_listen (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		/* These assignments are safe exactly because texts[] is file-scope) */
		netplayOptions.peer[current_state->which_side].port = texts[1].value;
		netplayOptions.peer[current_state->which_side].isServer = TRUE;
		netplayOptions.inputDelay = slider.value;
		current_state->confirmed = TRUE;
		done = TRUE;
	}
	(void)self;
	return FALSE;
}

static int
do_cancel (WIDGET *self, int event)
{
	if (event == WIDGET_EVENT_SELECT)
	{
		current_state->confirmed = FALSE;
		done = TRUE;
	}
	(void)self;
	return FALSE;
}


static void
CreateWidgets (void)
{
	int i;

	done = false;

	for (i = 0; i < 3; i++)
	{
		buttons[i].tag = WIDGET_TYPE_BUTTON;
		buttons[i].parent = NULL;
		buttons[i].receiveFocus = Widget_ReceiveFocusSimple;
		buttons[i].draw = MCD_DrawButton;
		buttons[i].height = Widget_HeightOneLine;
		buttons[i].width = Widget_WidthFullScreen;
	}
	buttons[0].name = GAME_STRING (NETMELEE_STRING_BASE + 19);
			// "Connect to remote host"
	buttons[1].name = GAME_STRING (NETMELEE_STRING_BASE + 20);
			// "Wait for incoming connection"
	buttons[2].name = GAME_STRING (NETMELEE_STRING_BASE + 21);
			// "Cancel"

	buttons[0].handleEvent = do_connect;
	buttons[1].handleEvent = do_listen;
	buttons[2].handleEvent = do_cancel;

	menu.tag = WIDGET_TYPE_MENU_SCREEN;
	menu.parent = NULL;
	menu.receiveFocus = Widget_ReceiveFocusMenuScreen;
	menu.draw = MCD_DrawMenuScreen;
	menu.height = Widget_HeightFullScreen;
	menu.width = Widget_WidthFullScreen;
	menu.num_children = 6;
	menu.child = menu_widgets;
	menu.handleEvent = Widget_HandleEventMenuScreen;

	slider.tag = WIDGET_TYPE_SLIDER;
	slider.parent = NULL;
	slider.handleEvent = Widget_HandleEventSlider;
	slider.receiveFocus = Widget_ReceiveFocusSimple;
	slider.draw = MCD_DrawSlider;
	slider.height = Widget_HeightOneLine;
	slider.width = Widget_WidthFullScreen;
	slider.draw_value = Widget_Slider_DrawValue;
	slider.min = 0;
	slider.max = 9;
	slider.step = 1;
	slider.value = netplayOptions.inputDelay;
	slider.category = GAME_STRING (NETMELEE_STRING_BASE + 24);
			// "Net Delay"

	for (i = 0; i < 2; i++)
	{
		texts[i].tag = WIDGET_TYPE_TEXTENTRY;
		texts[i].parent = NULL;
		texts[i].handleEvent = Widget_HandleEventTextEntry;
		texts[i].receiveFocus = Widget_ReceiveFocusSimple;
		texts[i].draw = MCD_DrawTextEntry;
		texts[i].height = Widget_HeightOneLine;
		texts[i].width = Widget_WidthFullScreen;
		texts[i].handleEventSelect = OnTextEntryEvent;
		texts[i].maxlen = WIDGET_TEXTENTRY_WIDTH-1;
		texts[i].state = WTE_NORMAL;
		texts[i].cursor_pos = 0;
	}

	texts[0].category = GAME_STRING (NETMELEE_STRING_BASE + 22);
			// "Host"
	texts[1].category = GAME_STRING (NETMELEE_STRING_BASE + 23);
			// "Port"

	/* We sometimes assign to these internals; cannot strncpy over self! */
	if (texts[0].value != netplayOptions.peer[current_state->which_side].host)
	{
		strncpy (texts[0].value,
				netplayOptions.peer[current_state->which_side].host,
				texts[0].maxlen);
	}
	if (texts[1].value != netplayOptions.peer[current_state->which_side].port)
	{
		strncpy (texts[1].value,
				netplayOptions.peer[current_state->which_side].port,
				texts[1].maxlen);
	}
	texts[0].value[texts[0].maxlen]=0;
	texts[1].value[texts[1].maxlen]=0;

	menu.receiveFocus ((WIDGET *)&menu, WIDGET_EVENT_DOWN);
}

static void
DrawConnectDialog (void)
{
	RECT r;
	
	r.extent.width = MCD_WIDTH;
	r.extent.height = MCD_HEIGHT;
	r.corner.x = (SCREEN_WIDTH - r.extent.width) >> 1;
	r.corner.y = (SCREEN_HEIGHT - r.extent.height) >> 1;


	DrawShadowedBox (&r, SHADOWBOX_BACKGROUND_COLOR,
			SHADOWBOX_DARK_COLOR, SHADOWBOX_MEDIUM_COLOR);

	menu.draw ((WIDGET *)&menu, r.corner.x + 10, r.corner.y + 10);

}

static BOOLEAN
DoMeleeConnectDialog (CONNECT_DIALOG_STATE *state)
{
	BOOLEAN changed;

	/* Cancel any presses of the Pause key. */
	GamePaused = FALSE;

	if (!state->Initialized)
	{
		state->Initialized = TRUE;
		SetDefaultMenuRepeatDelay ();
		state->NextTime = GetTimeCounter ();
		/* Prepare widgets, draw stuff, etc. */
		CreateWidgets ();
		DrawConnectDialog ();
	}

	changed = TRUE;

	if (PulsedInputState.menu[KEY_MENU_UP])
	{
		Widget_Event (WIDGET_EVENT_UP);
	}
	else if (PulsedInputState.menu[KEY_MENU_DOWN])
	{
		Widget_Event (WIDGET_EVENT_DOWN);
	}
	else if (PulsedInputState.menu[KEY_MENU_LEFT])
	{
		Widget_Event (WIDGET_EVENT_LEFT);
	}
	else if (PulsedInputState.menu[KEY_MENU_RIGHT])
	{
		Widget_Event (WIDGET_EVENT_RIGHT);
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		Widget_Event (WIDGET_EVENT_SELECT);
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		Widget_Event (WIDGET_EVENT_CANCEL);
	}
	else if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		Widget_Event (WIDGET_EVENT_DELETE);
	}
	else
	{
		changed = FALSE;
	}

	if (changed)
	{
		DrawConnectDialog ();
	}

	SleepThreadUntil (state->NextTime + MENU_FRAME_RATE);
	state->NextTime = GetTimeCounter ();
	return !((GLOBAL (CurrentActivity) & CHECK_ABORT) || 
		 done);
}

BOOLEAN
MeleeConnectDialog (int side)
{
	CONNECT_DIALOG_STATE state;

	PlayerFont = LoadFont (PLAYER_FONT);

	state.Initialized = FALSE;
	state.which_side = side;
	state.InputFunc = DoMeleeConnectDialog;
	state.confirmed = TRUE;

	current_state = &state;

	DoInput (&state, TRUE);

	current_state = NULL;

	DestroyFont (PlayerFont);

	return state.confirmed;
}

#endif /* NETPLAY */

