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

#include "gfxintrn.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawcmd.h"
#include "libs/timelib.h"
#include "libs/misc.h"
		// for TFB_DEBUG_HALT

// JMS_GFX
int fs_height = 0; 
int fs_width  = 0;
// End JMS_GFX

int ScreenWidth;
int ScreenHeight;
int ScreenWidthActual;
int ScreenHeightActual;
int ScreenColorDepth;
int GraphicsDriver;
int TFB_DEBUG_HALT = 0;

volatile int TransitionAmount = 255;
RECT TransitionClipRect;

static int gscale = GSCALE_IDENTITY;
static int gscale_mode = TFB_SCALE_NEAREST;

void
DrawFromExtraScreen (RECT *r, BOOLEAN Fs)
{	// Serosis: Added BOOLEAN trigger to minimize function redundancy
	TFB_DrawScreen_Copy(r, TFB_SCREEN_EXTRA, TFB_SCREEN_MAIN, Fs);
}

void
LoadIntoExtraScreen (RECT *r, BOOLEAN Fs)
{	// Serosis: Added BOOLEAN trigger to minimize function redundancy
	TFB_DrawScreen_Copy(r, TFB_SCREEN_MAIN, TFB_SCREEN_EXTRA, Fs);
}

int
SetGraphicScale (int scale)
{
	int old_scale = gscale;
	gscale = (scale ? scale : GSCALE_IDENTITY);
	return old_scale;
}

int
GetGraphicScale (void)
{
	return gscale;
}

int
SetGraphicScaleMode (int mode)
{
	int old_mode = gscale_mode;
	assert (mode >= TFB_SCALE_NEAREST && mode <= TFB_SCALE_TRILINEAR);
	gscale_mode = mode;
	return old_mode;
}

int
GetGraphicScaleMode (void)
{
	return gscale_mode;
}

/* Batching and Unbatching functions.  A "Batch" is a collection of
   DrawCommands that will never be flipped to the screen half-rendered.
   BatchGraphics and UnbatchGraphics function vaguely like a non-blocking
   recursive lock to do this respect. */
void
BatchGraphics (void)
{
	TFB_BatchGraphics ();
}

void
UnbatchGraphics (void)
{
	TFB_UnbatchGraphics ();
}

/* Sleeps this thread until all Draw Commands queued by that thread have
   been processed. */

void
FlushGraphics (void)
{
	TFB_DrawScreen_WaitForSignal ();
}

static void
ExpandRect (RECT *rect, int expansion)
{
	if (rect->corner.x - expansion >= 0)
	{
		rect->extent.width += expansion;
		rect->corner.x -= expansion;
	}
	else
	{
		rect->extent.width += rect->corner.x;
		rect->corner.x = 0;
	}

	if (rect->corner.y - expansion >= 0)
	{
		rect->extent.height += expansion;
		rect->corner.y -= expansion;
	}
	else
	{
		rect->extent.height += rect->corner.y;
		rect->corner.y = 0;
	}

	if (rect->corner.x + rect->extent.width + expansion <= ScreenWidth)
		rect->extent.width += expansion;
	else
		rect->extent.width = ScreenWidth - rect->corner.x;

	if (rect->corner.y + rect->extent.height + expansion <= ScreenHeight)
		rect->extent.height += expansion;
	else
		rect->extent.height = ScreenHeight - rect->corner.y;
}

void
SetTransitionSource (const RECT *pRect)
{
	RECT ActualRect;

	if (pRect)
	{	/* expand the rect to accomodate scalers in OpenGL mode */
		ActualRect = *pRect;
		pRect = &ActualRect;
		ExpandRect (&ActualRect, 2);
	}
	TFB_DrawScreen_Copy (pRect, TFB_SCREEN_MAIN, TFB_SCREEN_TRANSITION, FALSE);
}

// ScreenTransition() is synchronous (does not return until transition done)
void
ScreenTransition (int TransType, const RECT *pRect)
{
	const TimePeriod DURATION = ONE_SECOND * 31 / 60;
	TimeCount startTime;
	(void) TransType;  /* dodge compiler warning */

	if (pRect)
	{
		TransitionClipRect = *pRect;
	}
	else
	{
		TransitionClipRect.corner.x = 0;
		TransitionClipRect.corner.y = 0;
		TransitionClipRect.extent.width = ScreenWidth;
		TransitionClipRect.extent.height = ScreenHeight;
	}

	TFB_UploadTransitionScreen ();
	
	TransitionAmount = 0;
	FlushGraphics ();
	startTime = GetTimeCounter ();
	while (TransitionAmount < 255)
	{
		TimePeriod deltaT;
		int newAmount;

		SleepThread (ONE_SECOND / 100);

		deltaT = GetTimeCounter () - startTime;
		newAmount = deltaT * 255 / DURATION;
		if (newAmount > 255)
			newAmount = 255;

		TransitionAmount = newAmount;
	}
}
