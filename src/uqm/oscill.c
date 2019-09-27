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

#include "oscill.h"

#include "setup.h"
		// for OffScreenContext
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/sound/sound.h"
#include "libs/sound/trackplayer.h"
#include "libs/log.h"

static FRAME scope_frame;
static int scope_init = 0;
static FRAME scopeWork;
static Color scopeColor;
static EXTENT scopeSize;
BOOLEAN oscillDisabled = FALSE;

void
InitOscilloscope (FRAME scopeBg)
{
	scope_frame = scopeBg;
	if (!scope_init)
	{
		EXTENT size = GetFrameBounds (scope_frame);
		POINT midPt = {size.width / 2, size.height / 2};

		// mid-image pixel defines the color of scope lines
		scopeColor = GetFramePixel (scope_frame, midPt);
		
		scopeWork = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP | MAPPED_TO_DISPLAY,
				size.width, size.height, 1));

		// assume and subtract the borders
		scopeSize.width = size.width - RES_STAT_SCALE(2);
		scopeSize.height = size.height - RES_STAT_SCALE(2);

		scope_init = 1;
	}
}

void
UninitOscilloscope (void)
{
	// XXX: Is never called (BUG?)
	DestroyDrawable (ReleaseDrawable (scopeWork));
	scopeWork = NULL;
	scope_init = 0;
}

// draws the oscilloscope
void
DrawOscilloscope (void)
{
	STAMP s;
	BYTE scope_data[192]; // JMS_GFX: was 128... FIXME:This is a hack: GraphForeGroundStream would really require this to be
							// less than 256. This "fix" messes up how the oscilloscope looks, but it works for now
							// (doesn't get caught in asserts). We need to fix this later.

	// BW: fixed. With narrow status panel at 4x, scope width (and data) are never more than 192.
	if (oscillDisabled)
		return;

	//log_add(log_Debug, "(size_t)scopeSize.width %lu, sizeof(scope_data) %lu", (size_t)scopeSize.width, sizeof(scope_data));
	
	assert ((size_t)scopeSize.width <= sizeof(scope_data));
	assert (scopeSize.height < 256); // JMS_GFX: Was 256. FIXME:This is a hack: GraphForeGroundStream would really require this to be
	// less than 256. This "fix" messes up how the oscilloscope looks, but it works for now
	// (doesn't get caught in asserts). We need to fix this later.

	if (GraphForegroundStream (scope_data, scopeSize.width, scopeSize.height,
			usingSpeech))
	{
		int i;
		CONTEXT oldContext;

		oldContext = SetContext (OffScreenContext);
		SetContextFGFrame (scopeWork);
		SetContextClipRect (NULL);
		
		// draw the background image
		s.origin.x = 0;
		s.origin.y = 0;
		s.frame = scope_frame;
		DrawStamp (&s);

		// draw the scope lines
		SetContextForeGroundColor (scopeColor);
		for (i = 0; i < scopeSize.width - RES_STAT_SCALE(1); ++i)
		{
			LINE line;

			line.first.x = i + RES_STAT_SCALE(1);
			line.first.y = scope_data[i] + RES_STAT_SCALE(1);
			line.second.x = i + RES_STAT_SCALE(2);
			line.second.y = scope_data[i + RES_STAT_SCALE(1)] + RES_STAT_SCALE(1);
			DrawLine (&line);
		}

		SetContext (oldContext);

		s.frame = scopeWork;
	}
	else
	{	// no data -- draw blank scope background
		s.frame = scope_frame;
	}

	// draw the final scope image to screen
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);
}


static STAMP sliderStamp;
static STAMP buttonStamp;
static BOOLEAN sliderChanged = FALSE;
int sliderSpace;  // slider width - button width
BOOLEAN sliderDisabled = FALSE;

/*
 * Initialise the communication progress bar
 * x - x location of slider
 * y - y location of slider
 * width - width of slider
 * height - height of slider
 * bwidth - width of button indicating current progress
 * bheight - height of button indicating progress
 * f - image for the slider
 */                        

void
InitSlider (int x, int y, int width, FRAME sliderFrame, FRAME buttonFrame)
{
	EXTENT sliderSize = GetFrameBounds (sliderFrame);
	EXTENT buttonSize = GetFrameBounds (buttonFrame);

	sliderStamp.origin.x = x;
	sliderStamp.origin.y = y;
	sliderStamp.frame = sliderFrame;
	
	buttonStamp.origin.x = x;
	buttonStamp.origin.y = y - ((buttonSize.height - sliderSize.height) / 2);
	buttonStamp.frame = buttonFrame;

	sliderSpace = width - buttonSize.width;
}

void
SetSliderImage (FRAME f)
{
	sliderChanged = TRUE;
	buttonStamp.frame = f;
}

void
DrawSlider (void)
{
	int offs;
	static int last_offs = -1;

	if (sliderDisabled)
		return;
	
	offs = GetTrackPosition (sliderSpace);
	if (offs != last_offs || sliderChanged)
	{
		sliderChanged = FALSE;
		last_offs = offs;
		buttonStamp.origin.x = sliderStamp.origin.x + offs;
		BatchGraphics ();
		DrawStamp (&sliderStamp);
		DrawStamp (&buttonStamp);
		UnbatchGraphics ();
	}
}

