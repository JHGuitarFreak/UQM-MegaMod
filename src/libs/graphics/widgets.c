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

#include "gfx_common.h"
#include "widgets.h"
#include "libs/strlib.h"
#include "uqm/colors.h"
#include "uqm/units.h"

WIDGET *widget_focus = NULL;

/* Some basic color defines */
#define WIDGET_ACTIVE_COLOR \
		MENU_HIGHLIGHT_COLOR
#define WIDGET_INACTIVE_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x18, 0x18, 0x1F), 0x00)
#define WIDGET_INACTIVE_SELECTED_COLOR \
		WHITE_COLOR
#define WIDGET_CURSOR_COLOR \
		BLACK_COLOR
#define WIDGET_DIALOG_COLOR \
		LTGRAY_COLOR
#define WIDGET_DIALOG_TEXT_COLOR \
		WIDGET_CURSOR_COLOR

#define WIDGET_ENABLED_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x18, 0x00), 0x00)
#define WIDGET_DISABLED_COLOR \
		PCMENU_TOP_LEFT_BORDER_COLOR
#define WIDGET_TOOLTIP_COLOR \
		BUILD_COLOR(MAKE_RGB15(0xD2, 0xB4, 0x8C), 0x00)

static Color win_bg_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x18, 0x18, 0x1F), 0x00);
static Color win_medium_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x10, 0x10, 0x18), 0x00);
static Color win_dark_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x08, 0x08, 0x10), 0x00);

static FONT cur_font;

void
DrawShadowedBox (RECT *r, Color bg, Color dark, Color medium)
{
	RECT t;
	Color oldcolor;

	BatchGraphics ();

	t.corner.x = r->corner.x - 2;
	t.corner.y = r->corner.y - 2;
	t.extent.width  = r->extent.width + 4;
	t.extent.height  = r->extent.height + 4;
	oldcolor = SetContextForeGroundColor (dark);
	DrawFilledRectangle (&t);

	t.corner.x += 2;
	t.corner.y += 2;
	t.extent.width -= 2;
	t.extent.height -= 2;
	SetContextForeGroundColor (medium);
	DrawFilledRectangle (&t);

	t.corner.x -= 1;
	t.corner.y += r->extent.height + 1;
	t.extent.height = 1;
	DrawFilledRectangle (&t);

	t.corner.x += r->extent.width + 2;
	t.corner.y -= r->extent.height + 2;
	t.extent.width = 1;
	DrawFilledRectangle (&t);

	SetContextForeGroundColor (bg);
	DrawFilledRectangle (r);

	SetContextForeGroundColor (oldcolor);
	UnbatchGraphics ();
}

// windowRect, if not NULL, will be filled with the dimensions of the
// window drawn.
void
DrawLabelAsWindow (WIDGET_LABEL *label, RECT *windowRect)
{
	Color oldfg = SetContextForeGroundColor (WIDGET_DIALOG_TEXT_COLOR);
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	RECT r;
	TEXT t;
	int i, win_w, win_h, extra;

	if (cur_font)
		oldfont = SetContextFont (cur_font);

	/* Compute the dimensions of the label */
	win_h = label->height ((WIDGET *)label) + (16);
	win_w = 0;
	for (i = 0; i < label->line_count; i++)
	{
		int len = utf8StringCount (label->lines[i]);
		if (len > win_w)
		{
			win_w = RES_BOOL(len, len * 2);
		}
	}
	extra = RES_BOOL(6, (label->line_count == 3 ? 12 : 6));
	win_w = (win_w * extra) + 16;

	BatchGraphics ();
	r.corner.x = (ScreenWidth - win_w) >> 1;
	r.corner.y = (ScreenHeight - win_h) >> 1;
	r.extent.width = win_w;
	r.extent.height = win_h;
	DrawShadowedBox (&r, win_bg_clr, win_dark_clr, win_medium_clr);

	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + RES_BOOL(14, 38); // JMS_GFX
	for (i = 0; i < label->line_count; i++)
	{
		t.pStr = label->lines[i];
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(8); // JMS_GFX
	}

	UnbatchGraphics ();

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldfg);

	if (windowRect != NULL) {
		// Add the outer border added by DrawShadowedBox.
		// XXX: It may be nicer to add a border size parameter to
		// DrawShadowedBox, instead of assuming 2 here.
		windowRect->corner.x = r.corner.x - 2;
		windowRect->corner.y = r.corner.y - 2;
		windowRect->extent.width = r.extent.width + 4;
		windowRect->extent.height = r.extent.height + 4;
	}
}

void
Widget_SetWindowColors (Color bg, Color dark, Color medium)
{
	win_bg_clr = bg;
	win_dark_clr = dark;
	win_medium_clr = medium;
}

FONT
Widget_SetFont (FONT newFont)
{
	FONT oldFont = cur_font;
	cur_font = newFont;
	return oldFont;
}

static void
Widget_DrawToolTips (int numlines, const char **tips)
{
	// This functions draws the text at the bottom of the screen
	// which explains what the current option does.
	RECT r;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	Color oldtext = SetContextForeGroundColor (WIDGET_TOOLTIP_COLOR);
	TEXT t;
	int i;

	if (cur_font)
		oldfont = SetContextFont (cur_font);

	r.corner.x = RES_SCALE(2); // JMS_GFX
	r.corner.y = RES_SCALE(2); // JMS_GFX
	r.extent.width = ScreenWidth - RES_SCALE(4); // JMS_GFX
	r.extent.height = ScreenHeight + RES_SCALE(2); // JMS_GFX

	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + (r.extent.height - RES_SCALE(8) - RES_SCALE(8) * numlines); // JMS_GFX

	for (i = 0; i < numlines; i++)
	{
		t.pStr = tips[i];
		font_DrawText(&t);
		t.baseline.y += RES_SCALE(9); // JMS_GFX
	}

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawMenuScreen (WIDGET *_self, int x, int y)
{
	RECT r;
	Color title, oldtext;
	Color inactive, default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int widget_index, height, widget_y;

	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	r.corner.x = RES_SCALE(2) + IF_HD(4); // JMS_GFX
	r.corner.y = RES_SCALE(2); // JMS_GFX
	r.extent.width = ScreenWidth - RES_SCALE(4); // JMS_GFX
	r.extent.height = ScreenHeight - RES_SCALE(4); // JMS_GFX
	
	title = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;
	default_color = title;
	
	DrawStamp (&self->bgStamp);
	
	oldtext = SetContextForeGroundColor (title);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + RES_SCALE(8); // JMS_GFX
	t.pStr = self->title;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	font_DrawText (&t);
	t.baseline.y += RES_SCALE(8); // JMS_GFX
	t.pStr = self->subtitle;
	font_DrawText (&t);

	height = 0;
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{
		WIDGET *child = self->child[widget_index];
		height += (*child->height)(child);
		height += RES_SCALE(8); // JMS_GFX  /* spacing */
	}

	height -= RES_SCALE(8); // JMS_GFX

	widget_y = (ScreenHeight - height) >> 1;
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{
		WIDGET *c = self->child[widget_index];
		(*c->draw)(c, 0, widget_y);
		widget_y += (*c->height)(c) + RES_SCALE(8); // JMS_GFX
	}
	
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);

	(void) x;
	(void) y;
}

void
Widget_DrawChoice (WIDGET *_self, int x, int y)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	Color oldtext;
	Color default_color, selected, enabled, disabled;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i, home_x, home_y;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	enabled = WIDGET_ENABLED_COLOR;
	disabled = WIDGET_DISABLED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;

	t.baseline.x = x + RES_SCALE(16);
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

	t.baseline.x -= t.baseline.x;

	home_x = t.baseline.x + 3 * (ScreenWidth / ((self->maxcolumns + 1) * 2));
	home_y = t.baseline.y;
	t.align = ALIGN_CENTER;
	for (i = 0; i < self->numopts; i++)
	{
		t.baseline.x = home_x + ((i % 3) *
				(ScreenWidth / (self->maxcolumns + 1)));
		t.baseline.y = home_y + RES_SCALE(10 * (i / 3)); // JMS_GFX // Was 8*(i/3): Changed for readability
		t.pStr = self->options[i].optname;
		if ((widget_focus == _self) &&
		    (self->highlighted == i))
		{
			SetContextForeGroundColor (selected);
			Widget_DrawToolTips (3, self->options[i].tooltip);
		}
		else if (i == self->selected)
		{
			SetContextForeGroundColor (enabled);
		}
		else
		{
			SetContextForeGroundColor (disabled);
		}
		font_DrawText (&t);
	}
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawButton (WIDGET *_self, int x, int y)
{
	WIDGET_BUTTON *self = (WIDGET_BUTTON *)_self;
	Color oldtext;
	Color inactive, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;

	t.baseline.x = RES_SCALE(160); // JMS_GFX
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	t.pStr = self->name;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips (3, self->tooltip);		
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (inactive);
	}
	font_DrawText (&t);
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
	(void) x;
}

void
Widget_DrawLabel (WIDGET *_self, int x, int y)
{
	WIDGET_LABEL *self = (WIDGET_LABEL *)_self;
	Color oldtext = SetContextForeGroundColor (WIDGET_INACTIVE_SELECTED_COLOR);
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	t.baseline.x = RES_SCALE(160); // JMS_GFX
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;

	for (i = 0; i < self->line_count; i++)
	{
		t.pStr = self->lines[i];
		font_DrawText (&t);
		t.baseline.y += RES_SCALE(10); // JMS_GFX
	}
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
	(void) x;
}

void
Widget_DrawSlider(WIDGET *_self, int x, int y)
{
	WIDGET_SLIDER *self = (WIDGET_SLIDER *)_self;
	Color oldtext;
	Color inactive, default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	RECT r;
	int tick = (ScreenWidth - x) / 8;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;

	t.baseline.x = x + RES_SCALE(16);
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips (3, self->tooltip);		
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (default_color);
	}
	font_DrawText (&t);

	t.baseline.x -= t.baseline.x;

	r.corner.x = t.baseline.x + 3 * tick;
	r.corner.y = t.baseline.y - 4;
	r.extent.height = 2;
	r.extent.width = 3 * tick;
	DrawFilledRectangle (&r);

	r.extent.width = 3;
	r.extent.height = 8;
	r.corner.y = t.baseline.y - 7;
	r.corner.x = t.baseline.x + 3 * tick + (3 * tick * (self->value - self->min) /
		(self->max - self->min)) - 1;
	DrawFilledRectangle (&r);

	(*self->draw_value)(self, t.baseline.x + 7 * tick, t.baseline.y);

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_Slider_DrawValue (WIDGET_SLIDER *self, int x, int y)
{
	TEXT t;
	char buffer[16];

	sprintf (buffer, "%d", self->value);

	t.baseline.x = x;
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	t.pStr = buffer;

	font_DrawText (&t);
}

void
Widget_DrawTextEntry (WIDGET *_self, int x, int y)
{
	WIDGET_TEXTENTRY *self = (WIDGET_TEXTENTRY *)_self;
	Color oldtext;
	Color inactive, default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;

	BatchGraphics ();

	t.baseline.x = x + RES_SCALE(16);
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = self->category;
	if (widget_focus == _self)
	{
		Widget_DrawToolTips(3, self->tooltip);
		oldtext = SetContextForeGroundColor (selected);
	}
	else
	{
		oldtext = SetContextForeGroundColor (default_color);
	}
	font_DrawText (&t);

	t.baseline.x -= t.baseline.x;

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

		t.baseline.x = RES_SCALE(90); // JMS_GFX
		t.align = ALIGN_LEFT;

		// calc background box dimensions
		// XXX: this may need some tuning, especially if a
		//   different font is used. The font 'leading' values
		//   are not what they should be.
#define BOX_VERT_OFFSET 2
		GetContextFontLeading (&leading);
		r.corner.x = t.baseline.x - 1;
		r.corner.y = t.baseline.y - leading + BOX_VERT_OFFSET;
		r.extent.width = ScreenWidth - r.corner.x - 10;
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
		SetContextForeGroundColor (WIDGET_CURSOR_COLOR);
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (inactive);
		font_DrawText (&t);
	}
	
	UnbatchGraphics ();
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawControlEntry (WIDGET *_self, int x, int y)
{
	WIDGET_CONTROLENTRY *self = (WIDGET_CONTROLENTRY *)_self;
	Color oldtext;
	Color inactive, default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i, home_x, home_y;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;

	t.baseline.x = x + RES_SCALE(16);
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

	t.baseline.x -= t.baseline.x;

        // 3 * ScreenWidth / ((self->maxcolumns + 1) * 2)) as per CHOICE, but only two options.
	home_x = t.baseline.x + (ScreenWidth / 2); 
	home_y = t.baseline.y;
	t.align = ALIGN_CENTER;
	for (i = 0; i < 2; i++)
	{
		t.baseline.x = home_x + ((i % 3) * (ScreenWidth / 3));  // self->maxcolumns + 1 as per CHOICE.
		t.baseline.y = home_y + RES_SCALE(8 * (i / 3)); // JMS_GFX;
		t.pStr = self->controlname[i];
		if (!t.pStr[0])
		{
			t.pStr = "---";
		}
		if ((widget_focus == _self) &&
		    (self->highlighted == i))
		{
			SetContextForeGroundColor (selected);
		} 
		else
		{
			SetContextForeGroundColor (default_color);
		}
		font_DrawText (&t);
	}
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

int
Widget_HeightChoice (WIDGET *_self)
{
	return ((((WIDGET_CHOICE *)_self)->numopts + 2) / 3) * RES_SCALE(8); // JMS_GFX;
}

int
Widget_HeightFullScreen (WIDGET *_self)
{
	(void)_self;
	return ScreenHeight;
}

int
Widget_HeightOneLine (WIDGET *_self)
{
	(void)_self;
	return RES_SCALE(8); // JMS_GFX
}

int
Widget_HeightLabel (WIDGET *_self)
{
	WIDGET_LABEL *self = (WIDGET_LABEL *)_self;
	return self->line_count * RES_SCALE(8);
}

int
Widget_WidthFullScreen (WIDGET *_self)
{
	(void)_self;
	return ScreenWidth;
}

int
Widget_ReceiveFocusSimple (WIDGET *_self, int event)
{
	widget_focus = _self;
	(void)event;
	return TRUE;
}

int
Widget_ReceiveFocusChoice (WIDGET *_self, int event)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	widget_focus = _self;
	self->highlighted = self->selected;
	(void)event;
	return TRUE;
}

int
Widget_ReceiveFocusControlEntry (WIDGET *_self, int event)
{
	WIDGET_CONTROLENTRY *self = (WIDGET_CONTROLENTRY *)_self;
	int oldval = 0;
	if (widget_focus->tag == WIDGET_TYPE_CONTROLENTRY)
	{
		oldval = ((WIDGET_CONTROLENTRY *)widget_focus)->highlighted;
	}
	widget_focus = _self;
	self->highlighted = oldval;
	(void)event;
	return TRUE;
}

int
Widget_ReceiveFocusMenuScreen (WIDGET *_self, int event)
{
	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	int x, last_x, dx;
	for (x = 0; x < self->num_children; x++)
	{
		self->child[x]->parent = _self;
	}
	if (event == WIDGET_EVENT_UP)
	{
		x = self->num_children - 1;
		dx = -1;
		last_x = -1;
	}
	else if (event == WIDGET_EVENT_DOWN)
	{
		x = 0;
		dx = 1;
		last_x = self->num_children;
	}
	else 
	{
		/* Leave highlighted value the same */
		WIDGET *child = self->child[self->highlighted];
		child->receiveFocus (child, event);
		return TRUE;
	}
	for ( ; x != last_x; x += dx) 
	{
		WIDGET *child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return TRUE;
		}
	}
	return FALSE;
}

int
Widget_ReceiveFocusRefuseFocus (WIDGET *self, int event)
{
	(void)self;
	(void)event;
	return FALSE;
}

int
Widget_HandleEventIgnoreAll (WIDGET *self, int event)
{
	(void)event;
	(void)self;
	return FALSE;
}

int
Widget_HandleEventChoice (WIDGET *_self, int event)
{
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	switch (event)
	{
	case WIDGET_EVENT_LEFT:
		self->highlighted -= 1;
		if (self->highlighted < 0)
			self->highlighted = self->numopts - 1;
		return TRUE;
	case WIDGET_EVENT_RIGHT:
		self->highlighted += 1;
		if (self->highlighted >= self->numopts)
			self->highlighted = 0;
		return TRUE;
	case WIDGET_EVENT_SELECT:
	{
		int oldval = self->selected;
		self->selected = self->highlighted;
		if (self->onChange)
		{
			(*(self->onChange))(self, oldval);
		}
		return TRUE;
	}
	default:
		return FALSE;
	}
}

int
Widget_HandleEventSlider (WIDGET *_self, int event)
{
	WIDGET_SLIDER *self = (WIDGET_SLIDER *)_self;
	switch (event)
	{
	case WIDGET_EVENT_LEFT:
		self->value -= self->step;
		if (self->value < self->min)
			self->value = self->min;			
		return TRUE;
	case WIDGET_EVENT_RIGHT:
		self->value += self->step;
		if (self->value > self->max)
			self->value = self->max;
		return TRUE;
	default:
		return FALSE;
	}
}

int
Widget_HandleEventMenuScreen (WIDGET *_self, int event)
{
	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;
	int x, last_x, dx;
	switch (event)
	{
	case WIDGET_EVENT_UP:
		dx = -1;
		break;
	case WIDGET_EVENT_DOWN:
		dx = 1;
		break;
	case WIDGET_EVENT_CANCEL:
		/* On cancel, shift focus to last element and send a SELECT. */
		self->highlighted = self->num_children - 1;
		widget_focus = self->child[self->highlighted];
		return (widget_focus->handleEvent)(widget_focus, WIDGET_EVENT_SELECT);
	default:
		return FALSE;
	}
	last_x = self->highlighted;
	x = self->highlighted + dx;
	while (x != last_x)
	{
		WIDGET *child;
		if (x == -1)
			x = self->num_children - 1;
		if (x == self->num_children)
			x = 0;
		child = self->child[x];
		if ((*child->receiveFocus)(child, event))
		{
			self->highlighted = x;
			return TRUE;
		}
		x += dx;
	}
	return FALSE;
}

int
Widget_HandleEventTextEntry (WIDGET *_self, int event)
{
	WIDGET_TEXTENTRY *self = (WIDGET_TEXTENTRY *)_self;
	if (event == WIDGET_EVENT_SELECT) {
		if (!self->handleEventSelect)
			return FALSE;
		return (*self->handleEventSelect)(self);
	}
	return FALSE;
}

int
Widget_HandleEventControlEntry (WIDGET *_self, int event)
{
	WIDGET_CONTROLENTRY *self = (WIDGET_CONTROLENTRY *)_self;
	if (event == WIDGET_EVENT_SELECT)
	{
		if (self->onChange)
		{
			(self->onChange)(self);
			return TRUE;
		}
	}
	if (event == WIDGET_EVENT_DELETE)
	{
		if (self->onDelete)
		{
			(self->onDelete)(self);
			return TRUE;
		}
	}
	if ((event == WIDGET_EVENT_RIGHT) ||
	    (event == WIDGET_EVENT_LEFT))
	{
		self->highlighted = 1-self->highlighted;
		return TRUE;
	}
	return FALSE;
}

int
Widget_Event (int event)
{
	WIDGET *widget = widget_focus;
	while (widget != NULL)
	{
		if ((*widget->handleEvent)(widget, event))
			return TRUE;
		widget = widget->parent;
	}
	return FALSE;
}
