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
#include "colors.h"
#include "menustat.h"
#include "util.h"

static FRAME scope_frame;
static int scope_init = 0;
static FRAME scopeWork;
static EXTENT scopeSize;
BOOLEAN oscillDisabled = FALSE;

void
InitOscilloscope (FRAME scopeBg)
{
	scope_frame = scopeBg;
	if (!scope_init)
	{
		EXTENT size = GetFrameBounds (scope_frame);
		
		scopeWork = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP | MAPPED_TO_DISPLAY,
				size.width, size.height, 1));

		// assume and subtract the borders
		scopeSize.width = RES_DESCALE (size.width);
		scopeSize.height = RES_DESCALE (size.height);

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

BYTE
ScaleHeightByVolume (uint8 scope_data, BOOLEAN toScale)
{
	if (!toScale || musicVolume == NORMAL_VOLUME)
		return scope_data;
	else
	{
		float scale = (float)musicVolume / NORMAL_VOLUME;
		float res;

		if (scope_data >= (scopeSize.height / 2))
		{
			res = scope_data - (scopeSize.height / 2);
			res *= scale;
			return (BYTE)((scopeSize.height / 2) + res);
		}
		else
		{
			res = (scopeSize.height / 2) - scope_data;
			res *= scale;
			return (BYTE)((scopeSize.height / 2) - res);
		}
	}
}

void
DrawOscilloscopeLines (STAMP *s, uint8 *scope_data, BOOLEAN nonStop, BOOLEAN toScale)
{
	int i;
	CONTEXT oldContext;
	Color scopeColor;

	oldContext = SetContext (OffScreenContext);
	SetContextFGFrame (scopeWork);
	SetContextClipRect (NULL);

	// draw the background image
	s->origin.x = 0;
	s->origin.y = 0;
	s->frame = scope_frame;

	DrawStamp (s);

	// Set oscilloscope line color
	scopeColor = optScopeStyle != OPT_PC ?
			SCOPE_COLOR_3DO : SCOPE_COLOR_PC;

	if (nonStop)
	{	// Dim the oscilloscope lines for Non-Stop option
#define DIM_PERCENTAGE 0.77
		scopeColor.r *= DIM_PERCENTAGE;
		scopeColor.g *= DIM_PERCENTAGE;
		scopeColor.b *= DIM_PERCENTAGE;
	}

	SetContextForeGroundColor (scopeColor);

	if (scope_data)
	{
		for (i = 0; i < scopeSize.width - 1; ++i)
		{
			LINE line;

			line.first.x = RES_SCALE (i);
			line.first.y = RES_SCALE (ScaleHeightByVolume (scope_data[i],
					toScale));
			line.second.x = RES_SCALE (i + 1);
			line.second.y = RES_SCALE (ScaleHeightByVolume (
					scope_data[i + 1], toScale));
			DrawLine (&line, RES_SCALE (1));
		}
	}
	else
	{
		LINE line;

		line.first.x = 0;
		line.first.y = RES_SCALE ((scopeSize.height / 2));
		line.second.x = RES_SCALE (scopeSize.width);
		line.second.y = line.first.y;
		DrawLine (&line, RES_SCALE (1));
	}

	SetContext (oldContext);

	s->frame = scopeWork;
}

// draws the oscilloscope
void
DrawOscilloscope (void)
{
	STAMP s;
	BYTE scope_data[128];

	if (oscillDisabled)
		return;

	// log_add (log_Debug, "(size_t)scopeSize.width %lu,
	//		sizeof (scope_data) %lu", (size_t)scopeSize.width,
	//		sizeof (scope_data));
	
	assert ((size_t)scopeSize.width <= sizeof (scope_data));
	assert (scopeSize.height < 128);

	if (GraphForegroundStream (
			scope_data, scopeSize.width, scopeSize.height, usingSpeech))
	{
		DrawOscilloscopeLines (&s, scope_data, FALSE, !usingSpeech);
	}
	else if (GraphForegroundStream (
			scope_data, scopeSize.width, scopeSize.height, FALSE)
			&& usingSpeech && optNonStopOscill)
	{
		DrawOscilloscopeLines (&s, scope_data, TRUE, TRUE);
	}
	else
		DrawOscilloscopeLines (&s, NULL, FALSE, FALSE);

	// draw the final scope image to screen
	s.origin.x = 0;
	s.origin.y = 0;
	DrawStamp (&s);

	DrawRadarBorder ();
}

void
FlattenOscilloscope (void)
{
	STAMP s;
	CONTEXT OldContext;

	OldContext = SetContext (RadarContext);

	DrawOscilloscopeLines (&s, NULL, FALSE, FALSE);
	s.origin = MAKE_POINT(0, 0);
	DrawStamp (&s);
	SetContext (OldContext);
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
	buttonStamp.origin.y = y - (
			(buttonSize.height - sliderSize.height) / 2);
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

	if (sliderDisabled || (!usingSpeech && optSmoothScroll == OPT_PC))
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
