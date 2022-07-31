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

#include <stdio.h>
#include <string.h>

#include "gfx_common.h"
#include "uqm/nameref.h"
#include "uqm/igfxres.h"
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
#define PAGE_BUTTON_INACTIVE_COLOR \
		BUILD_COLOR_RGBA (163,44,0, 0)
#define WIDGET_INACTIVE_SELECTED_COLOR \
		WHITE_COLOR
#define WIDGET_CURSOR_COLOR \
		BLACK_COLOR
#define WIDGET_DIALOG_COLOR \
		LTGRAY_COLOR
#define WIDGET_DIALOG_TEXT_COLOR \
		WIDGET_CURSOR_COLOR
#define WIDGET_BONUS_COLOR \
		BUILD_COLOR_RGBA (223,106,19, 0)

#define WIDGET_ENABLED_COLOR \
		BUILD_COLOR (MAKE_RGB15 (0x00, 0x18, 0x00), 0x00)
#define WIDGET_DISABLED_COLOR \
		PCMENU_TOP_LEFT_BORDER_COLOR
#define WIDGET_TOOLTIP_COLOR \
		BUILD_COLOR(MAKE_RGB15(0xD2, 0xB4, 0x8C), 0x00)
#define WIDGET_LABEL_COLOR \
		BUILD_COLOR_RGBA (0,119,119, 0)

#define ONSCREEN 13
#define SCROLL_OFFSET 3 // The pos from the page edge where we need to start scrolling
#define SCREEN_CENTER RES_SCALE (RES_DESCALE (SCREEN_WIDTH) / 2);
#define LSTEP RES_SCALE (RES_DESCALE (SCREEN_WIDTH) / 2 - 7)
#define RSTEP RES_SCALE (RES_DESCALE (SCREEN_WIDTH) / 2 + 7)

static Color win_bg_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x18, 0x18, 0x1F), 0x00);
static Color win_medium_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x10, 0x10, 0x18), 0x00);
static Color win_dark_clr =
		BUILD_COLOR (MAKE_RGB15_INIT (0x08, 0x08, 0x10), 0x00);

static FONT cur_font;

static COUNT offset_t = 0; // Top widget offset
static COUNT offset_b = ONSCREEN; // Bottom widget offset
static FRAME arrow_frame = NULL; // Frames for additional graphics

void
ResetOffset (void)
{	// To reset offsets while traversing different moves
	offset_t = 0;
	offset_b = ONSCREEN;
}

void
LoadArrows (void)
{	
	if (arrow_frame == NULL || optRequiresRestart)
	{
		// Load the different arrows depending on the resolution factor.
		arrow_frame = CaptureDrawable (
				LoadGraphic (
					RES_BOOL (MENUARR_PMAP_ANIM, MENUARR_PMAP_ANIM_HD)));
	}
}

void
ReleaseArrows (void)
{	// Release graphics
	if (arrow_frame)
	{
		DestroyDrawable (ReleaseDrawable (arrow_frame));
		arrow_frame = NULL;
	}
}

void
DrawShadowedBox (RECT *r, Color bg, Color dark, Color medium)
{	// Dialog box
	RECT t;
	Color oldcolor;

	BatchGraphics ();

	t.corner.x = r->corner.x - RES_SCALE (2);
	t.corner.y = r->corner.y - RES_SCALE (2);
	t.extent.width  = r->extent.width + RES_SCALE (4);
	t.extent.height  = r->extent.height + RES_SCALE (4);
	oldcolor = SetContextForeGroundColor (dark);
	DrawFilledRectangle (&t);

	t.corner.x += RES_SCALE (2);
	t.corner.y += RES_SCALE (2);
	t.extent.width -= RES_SCALE (2);
	t.extent.height -= RES_SCALE (2);
	SetContextForeGroundColor (medium);
	DrawFilledRectangle (&t);

	t.corner.x -= RES_SCALE (1);
	t.corner.y += r->extent.height + RES_SCALE (1);
	t.extent.height = RES_SCALE (1);
	DrawFilledRectangle (&t);

	t.corner.x += r->extent.width + RES_SCALE (2);
	t.corner.y -= r->extent.height + RES_SCALE (2);
	t.extent.width = RES_SCALE (1);
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
	int i, win_w, win_h;

	if (cur_font)
		oldfont = SetContextFont (cur_font);

	/* Compute the dimensions of the label */
	win_h = label->height ((WIDGET *)label) + RES_SCALE (16);
	win_w = 0;
	for (i = 0; i < label->line_count; i++)
	{
		int len = utf8StringCount (label->lines[i]);
		if (len > win_w)
		{
			win_w = RES_SCALE (len);
		}
	}
	win_w = (win_w * 6) + RES_SCALE (16);

	BatchGraphics ();
	r.corner.x = RES_SCALE (
			(RES_DESCALE (ScreenWidth) - RES_DESCALE (win_w)) >> 1);
	r.corner.y = RES_SCALE (
			(RES_DESCALE (ScreenHeight) - RES_DESCALE (win_h)) >> 1);
	r.extent.width = win_w;
	r.extent.height = win_h;
	DrawShadowedBox (&r, win_bg_clr, win_dark_clr, win_medium_clr);

	t.baseline.x = r.corner.x
			+ RES_SCALE (RES_DESCALE (r.extent.width) >> 1);
	t.baseline.y = r.corner.y + RES_SCALE (16);
	for (i = 0; i < label->line_count; i++)
	{
		t.pStr = label->lines[i];
		t.align = ALIGN_CENTER;
		t.CharCount = (COUNT)~0;
		font_DrawText (&t);
		t.baseline.y += RES_SCALE (8); 
	}

	UnbatchGraphics ();

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldfg);

	if (windowRect != NULL)
	{	// Add the outer border added by DrawShadowedBox.
		// XXX: It may be nicer to add a border size parameter to
		// DrawShadowedBox, instead of assuming 2 here.
		windowRect->corner.x = r.corner.x - RES_SCALE (2);
		windowRect->corner.y = r.corner.y - RES_SCALE (2);
		windowRect->extent.width = r.extent.width + RES_SCALE (4);
		windowRect->extent.height = r.extent.height + RES_SCALE (4);
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

	r.corner.x = RES_SCALE (2);
	r.corner.y = RES_SCALE (8);
	r.extent.width = ScreenWidth - RES_SCALE (4);
	r.extent.height = ScreenHeight + RES_SCALE (3);

	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y
			+ (r.extent.height - RES_SCALE (8) - RES_SCALE (8) * numlines);

	for (i = 0; i < numlines; i++)
	{
		t.pStr = tips[i];
		font_DrawText(&t);
		t.baseline.y += RES_SCALE (9);
	}

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawMenuScreen (WIDGET *_self, int x, int y)
{	// Main menu function that draws backgroung and all widgets
	RECT r;
	Color title, oldtext;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int widget_index, height, widget_y, on_screen;
	

	WIDGET_MENU_SCREEN *self = (WIDGET_MENU_SCREEN *)_self;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	r.corner.x = RES_SCALE (2);
	r.corner.y = RES_SCALE (1);
	r.extent.width = ScreenWidth - RES_SCALE (4);
	r.extent.height = ScreenHeight - RES_SCALE (4);
	
	title = WIDGET_INACTIVE_SELECTED_COLOR;
	
	DrawStamp (&self->bgStamp);
	
	oldtext = SetContextForeGroundColor (title);
	t.baseline.x = r.corner.x + (r.extent.width >> 1);
	t.baseline.y = r.corner.y + RES_SCALE (6);
	t.pStr = self->title;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;
	font_DrawText (&t);
	t.baseline.y += RES_SCALE (8);
	t.pStr = self->subtitle;
	font_DrawText (&t);

	height = 0;
	for (widget_index = 0; widget_index < self->num_children; widget_index++)
	{	// Calculate overall height until we hit top limit (more widgets can't fit the page)
		WIDGET *child = self->child[widget_index];
		if (widget_index <= ONSCREEN)
		{
			height += (*child->height)(child);
			height += RES_SCALE (5);   /* spacing */
		}
	}

	height -= RES_SCALE (8);
	widget_y = (ScreenHeight - height) >> 1;

	{	// Scrolling
		if (self->highlighted > (offset_b - SCROLL_OFFSET) && offset_b < self->num_children - 1)
		{
			if (self->highlighted == self->num_children - 1 && offset_t != self->highlighted - (ONSCREEN - SCROLL_OFFSET))
			{// smooth wrapping scroll
				offset_t += 1;
				offset_b += 1;
			}
			else
			{// standart scroll
				offset_b = self->highlighted + SCROLL_OFFSET;

				if (offset_b > self->num_children - 1)
				{// reached bottom widget
					offset_b = self->num_children - 1;// cap bottom offset - stop further scrolling
					offset_t = offset_b - ONSCREEN;// cap top offset
				}
				else // still moving
					offset_t = self->highlighted - (ONSCREEN - SCROLL_OFFSET);
			}			
		}
		if (self->highlighted < (offset_t + SCROLL_OFFSET) && offset_t > 0)
		{
			if (self->highlighted < offset_t && offset_t != 0)
			{// smooth wrapping scroll
				offset_t -= 1;
				offset_b -= 1;
			}
			else
			{// standart scroll
				offset_t = self->highlighted - SCROLL_OFFSET;
				if (offset_t > 65500) // unit16 overflow
				{// reached top widget
					offset_t = 0;// cap top offset - stop further scrolling
					offset_b = offset_t + ONSCREEN;// cap bottom offset
				}
				else // still moving
					offset_b = self->highlighted + (ONSCREEN - SCROLL_OFFSET);
			}
		}
		if (self->num_children > ONSCREEN)
		{	// Arrows (blue to the right)
			STAMP arr;

			arr.origin.x = RES_SCALE (290);

			if (offset_t != 0)
			{
				arr.frame = SetAbsFrameIndex (arrow_frame, 0); // Up arrow
				arr.origin.y = RES_SCALE (25);
				DrawStamp (&arr);
			}
			if (offset_b != self->num_children - 1)
			{
				arr.frame = SetAbsFrameIndex (arrow_frame, 1); // Down arrow
				arr.origin.y = RES_SCALE (195);
				DrawStamp (&arr);
			}
		}
	}
	// Determine how much widgets we're gonna draw
	on_screen = (((offset_b + 1) < self->num_children) ? (offset_b + 1) : self->num_children);

	for (widget_index = offset_t; widget_index < on_screen; widget_index++)
	{
		WIDGET *c = self->child[widget_index];
		(*c->draw)(c, 0, widget_y);
		widget_y += (*c->height)(c) + RES_SCALE (5);
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
{	// Choice drawer
	WIDGET_CHOICE *self = (WIDGET_CHOICE *)_self;
	Color oldtext;
	Color default_color, selected, enabled, disabled, canbe;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR; // White
	enabled = WIDGET_ENABLED_COLOR; // Green
	disabled = WIDGET_DISABLED_COLOR; // Grey
	selected = WIDGET_ACTIVE_COLOR; // Yellow
	canbe = WIDGET_BONUS_COLOR; // Orange

	t.baseline.x = x + LSTEP;
	t.baseline.y = y;
	t.align = ALIGN_RIGHT;
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
	font_DrawText (&t); // Choicer name

	{	// Choicer options
		t.baseline.x = RSTEP;
		t.align = ALIGN_LEFT;
		for (i = 0; i < self->numopts; i++)
		{
			if (i == self->selected && widget_focus != _self)
			{	// Unfocused widget current option (White)
				t.pStr = self->options[i].optname;
				font_DrawText(&t);
			}

			if (widget_focus == _self && self->highlighted == i)
			{	// Focused widget
				if (i == self->selected)
				{// Currently chosen option (Yellow)
					SetContextForeGroundColor(selected);
				}
				else
				{// Available option (Orange)
					SetContextForeGroundColor(canbe);
				}
				Widget_DrawToolTips(3, self->options[i].tooltip);
				t.pStr = self->options[i].optname;
				font_DrawText(&t); // Draw only 1 option name at a time

				{	// Arrows around widget option
					STAMP arr;
					
					arr.origin.x = font_GetTextRect (&t).corner.x
							- RES_SCALE (10);
					arr.origin.y = t.baseline.y;

					// Left arrow
					arr.frame = SetAbsFrameIndex (arrow_frame, 2);
					DrawStamp (&arr);

					arr.origin.x = font_GetTextRect (&t).corner.x
							+ font_GetTextRect (&t).extent.width
							+ RES_SCALE (10);

					// Right arrow
					arr.frame = SetAbsFrameIndex (arrow_frame, 3);
					DrawStamp (&arr);
				}

				{	// Navigation Dots
					RECT d;
					COUNT c;

					d.extent.width = RES_SCALE (4);
					d.extent.height = RES_SCALE (1);

					d.corner.x = t.baseline.x - RES_SCALE (6);
					d.corner.y = t.baseline.y + RES_SCALE (4);

					for (c = 0; c < self->numopts; c++)
					{
						d.corner.x += RES_SCALE (6);

						if (c == self->highlighted)
							SetContextForeGroundColor (enabled);
						else
							SetContextForeGroundColor (disabled);

						DrawFilledRectangle (&d);
					}
				}
			}
		}
	}
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_DrawButton (WIDGET *_self, int x, int y)
{	// Several buttons for navigation
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
	t.baseline.x = SCREEN_CENTER;
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
{	// Special labels
	WIDGET_LABEL *self = (WIDGET_LABEL *)_self;
	Color oldtext = SetContextForeGroundColor (WIDGET_LABEL_COLOR);
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	t.baseline.x = SCREEN_CENTER;
	t.baseline.y = y;
	t.align = ALIGN_CENTER;
	t.CharCount = ~0;

	for (i = 0; i < self->line_count; i++)
	{
		t.pStr = self->lines[i];
		font_DrawText (&t);
		t.baseline.y += RES_SCALE (10);
	}
	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
	(void) x;
}

void
Widget_DrawSlider(WIDGET *_self, int x, int y)
{	// SFX slider
	WIDGET_SLIDER *self = (WIDGET_SLIDER *)_self;
	Color oldtext;
	Color default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	RECT r;
	int tick = (ScreenWidth - x) / 8;
	int slider_width;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;

	t.baseline.x = x + LSTEP;
	t.baseline.y = y;
	t.align = ALIGN_RIGHT;
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

	// Slider Bar
	r.corner.x = RSTEP;
	r.corner.y = t.baseline.y - RES_SCALE (4);
	r.extent.height = RES_SCALE (2);
	r.extent.width = 2 * tick;
	slider_width = r.corner.x + r.extent.width;
	DrawFilledRectangle (&r);

	// Slider Indicator
	r.extent.width = RES_SCALE (3);
	r.extent.height = RES_SCALE (8);
	r.corner.y = t.baseline.y - RES_SCALE (7);
	r.corner.x = RSTEP + (2 * tick * (self->value - self->min) /
		(self->max - self->min)) - (RES_SCALE (3) >> 1);
	DrawFilledRectangle (&r);

	//printf("%d + 3 * %d - 22 * %d\n", RSTEP, tick, tick);
	(*self->draw_value)(self, slider_width, t.baseline.y);

	SetContextFontEffect (oldFontEffect);
	if (oldfont)
		SetContextFont (oldfont);
	SetContextForeGroundColor (oldtext);
}

void
Widget_Slider_DrawValue (WIDGET_SLIDER *self, int x, int y)
{	// Number from 0 to 100 near slider
	TEXT t;
	char buffer[16];

	sprintf (buffer, "%d", self->value);

	t.baseline.x = x + RES_SCALE (14);
	t.baseline.y = y;
	t.align = ALIGN_LEFT;
	t.CharCount = ~0;
	t.pStr = buffer;

	font_DrawText (&t);
}

void
Widget_DrawTextEntry (WIDGET *_self, int x, int y)
{	// Custom seed and key layout name
	WIDGET_TEXTENTRY *self = (WIDGET_TEXTENTRY *)_self;
	Color oldtext;
	Color inactive, default_color, selected, editing;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;
	inactive = WIDGET_INACTIVE_COLOR;
	editing = WIDGET_LABEL_COLOR;

	BatchGraphics ();

	t.baseline.x = x + LSTEP;
	t.baseline.y = y;
	t.align = ALIGN_RIGHT;
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

	/* Force string termination */
	self->value[WIDGET_TEXTENTRY_WIDTH-RES_SCALE (1)] = 0;

	t.baseline.y = y;
	t.CharCount = utf8StringCount (self->value);
	t.pStr = self->value;

	if (!(self->state & WTE_EDITING))
	{	// normal or selected state
		t.baseline.x = RSTEP;
		t.align = ALIGN_LEFT;

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

		t.baseline.x = RSTEP;
		t.align = ALIGN_LEFT;

		// calc background box dimensions
		// XXX: this may need some tuning, especially if a
		//   different font is used. The font 'leading' values
		//   are not what they should be.
#define BOX_VERT_OFFSET RES_SCALE (2)
		GetContextFontLeading (&leading);
		r.corner.x = t.baseline.x - RES_SCALE (1);
		r.corner.y = t.baseline.y - leading + BOX_VERT_OFFSET;
		r.extent.width = ScreenWidth - r.corner.x - RES_SCALE (10);
		r.extent.height = leading - RES_SCALE (1);

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
			r.corner.x -= RES_SCALE (1);

		if (self->state & WTE_BLOCKCUR)
		{	// Use block cursor for keyboardless systems

			r.corner.y = r.corner.y;
			r.extent.height = r.extent.height;

			if (self->cursor_pos == t.CharCount)
			{	// cursor at end-line -- use insertion point
				r.extent.width = RES_SCALE (1);
				r.corner.x -= IF_HD (3);
			}
			else if (self->cursor_pos + 1 == t.CharCount)
			{	// extra pixel for last char margin
				r.extent.width = (SIZE)*pchar_deltas - IF_HD (3);
				r.corner.x += RES_SCALE (1);
			}
			else
			{	// normal mid-line char
				r.extent.width = (SIZE)*pchar_deltas;
				r.corner.x += RES_SCALE (1);
			}
		}
		else
		{	// Insertion point cursor
			r.corner.y = r.corner.y + RES_SCALE (1);
			r.extent.height = r.extent.height - RES_SCALE (2);
			r.extent.width = RES_SCALE (1);
		}
		// position cursor within input field rect
		r.corner.x += RES_SCALE (1);
		SetContextForeGroundColor (WIDGET_CURSOR_COLOR);
		DrawFilledRectangle (&r);

		SetContextForeGroundColor (editing);
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
{	// mappable key name in controls setup
	WIDGET_CONTROLENTRY *self = (WIDGET_CONTROLENTRY *)_self;
	Color oldtext;
	Color default_color, selected;
	FONT  oldfont = 0;
	FRAME oldFontEffect = SetContextFontEffect (NULL);
	TEXT t;
	int i, home_x, home_y;

	if (cur_font)
		oldfont = SetContextFont (cur_font);
	
	default_color = WIDGET_INACTIVE_SELECTED_COLOR;
	selected = WIDGET_ACTIVE_COLOR;

	t.baseline.x = x + RES_SCALE (16);
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
	font_DrawText (&t); // Control Name E.G. Up, Down, Weapon, Thrust

	t.baseline.x -= t.baseline.x;

	home_x = t.baseline.x + (ScreenWidth / 3);
	home_y = t.baseline.y;
	t.align = ALIGN_LEFT;
	for (i = 0; i < 2; i++)
	{
		t.baseline.x = home_x + ((i % 3) * (ScreenWidth / 3));
		t.baseline.y = home_y + RES_SCALE (8 * (i / 3));
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
	(void)_self;
	return RES_SCALE (8); //effectively 1 column
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
	return RES_SCALE (8);
}

int
Widget_HeightLabel (WIDGET *_self)
{
	WIDGET_LABEL *self = (WIDGET_LABEL *)_self;
	return self->line_count * RES_SCALE (8);
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
