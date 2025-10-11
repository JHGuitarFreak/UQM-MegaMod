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

#include "planets.h"
#include "../colors.h"
#include "../setup.h"
#include "libs/graphics/gfx_common.h"
#include "libs/graphics/drawable.h"
#include "libs/mathlib.h"
#include "scan.h"
#include "options.h"
#include "libs/log.h"
#include <math.h>

#include SDL_INCLUDE(SDL.h)

// define USE_ADDITIVE_SCAN_BLIT to use additive blittting
// instead of transparency for the planet scans.
// It still doesn't look right though (it is too bright)
#define USE_ADDITIVE_SCAN_BLIT

static int rotFrameIndex;
static int rotDirection;
static bool throbShield;
static int rotPointIndex;

static int rotwidth;
static int rotheight;

void
DrawCurrentPlanetSphere (void)
{
	STAMP s;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	CONTEXT oldContext = SetContext (PlanetContext);

	s.origin.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
	s.origin.y = PLANET_ORG_Y;

	{	// re-render current frame
		rotFrameIndex ^= 1;

		rotPointIndex -= rotDirection;// roll back
		if (rotPointIndex < 0)
			rotPointIndex = rotwidth - 1;
		else if (rotPointIndex >= rotwidth)
			rotPointIndex = 0;

		if (useDosSpheres)
		{
			if (rotFrameIndex)
				Orbit->SphereFrame = IncFrameIndex (Orbit->SphereFrame);

			RenderDOSPlanetSphere (
					Orbit, Orbit->SphereFrame, rotPointIndex);
		}
		else if (use3DOSpheres)
		{
			Orbit->SphereFrame = SetAbsFrameIndex (Orbit->SphereFrame,
					rotFrameIndex);

			Render3DOPlanetSphere (
					Orbit, Orbit->SphereFrame, rotPointIndex, rotwidth,
						rotheight);
		}
		else
		{
			Orbit->SphereFrame = SetAbsFrameIndex (Orbit->SphereFrame,
					rotFrameIndex);
			RenderPlanetSphere (Orbit, Orbit->SphereFrame, rotPointIndex,
					pSolarSysState->pOrbitalDesc->data_index
						& PLANET_SHIELDED,
					throbShield, rotwidth, rotheight,
					(rotheight >> 1) - IF_HD (2));
		}
	}
	BatchGraphics ();
	s.frame = Orbit->SphereFrame;
	DrawStamp (&s);
	if (Orbit->ObjectFrame)
	{
		s.frame = Orbit->ObjectFrame;

		if (use3DOSpheres)
			Draw3DOShield (s);
		else
			DrawStamp (&s);
	}
	UnbatchGraphics ();
	SetContext (oldContext);

	PrepareNextRotationFrame ();
}

// Draw the planet sphere and any extra graphic (like a shield) if present
void
DrawPlanetSphere (int x, int y, bool back)
{
	STAMP s;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	BatchGraphics ();
	// Draw background on static image and not during zoom
	if (Orbit->BackFrame && back)
	{
		s.origin.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1);
		s.origin.y = PLANET_ORG_Y;
		s.frame = Orbit->BackFrame;
		DrawStamp (&s);
	}

	s.origin.x = x;
	s.origin.y = y;
	s.frame = Orbit->SphereFrame;
	DrawStamp (&s);
	if (Orbit->ObjectFrame)
	{
		s.frame = Orbit->ObjectFrame;
		if (use3DOSpheres)
			Draw3DOShield (s);
		else
			DrawStamp (&s);
	}
	UnbatchGraphics ();
}

void
DrawDefaultPlanetSphere (void)
{
	CONTEXT oldContext;

	oldContext = SetContext (PlanetContext);
	DrawPlanetSphere (
			RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1), PLANET_ORG_Y, TRUE);
	SetContext (oldContext);
}

void
RerenderPlanetSphere (void)
{
	PLANET_ORBIT* Orbit = &pSolarSysState->Orbit;

	if (optTintPlanSphere != OPT_PC)
		return;

	if (useDosSpheres)
	{	// palette switch for coloring sphere into scan colors
		COUNT sc;
		sc = Orbit->scanType;
		if (sc > 2)
			sc = 0;
		else
			sc++;

		XFormColorMap (
				GetColorMapAddress (
					SetAbsColorMapIndex (Orbit->sphereMap, sc)), 0
			);
		XFormColorMap_step ();
	}

	DrawCurrentPlanetSphere ();
}

void
InitSphereRotation (int direction, BOOLEAN shielded, COUNT width,
		COUNT height)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	rotwidth = width;
	rotheight = height;

	rotDirection = direction;
	rotPointIndex = 0;
	throbShield = shielded && optWhichShield == OPT_3DO
			&& !(useDosSpheres || use3DOSpheres);

	if (throbShield)
	{
		// ObjectFrame must contain the shield graphic
		Orbit->WorkFrame = Orbit->ObjectFrame;
		// We need a scratch frame so that we can apply throbbing
		// to the shield, so create one
		Orbit->ObjectFrame = CaptureDrawable (CreateDrawable (
				WANT_PIXMAP | WANT_ALPHA,
				GetFrameWidth (Orbit->ObjectFrame),
				GetFrameHeight (Orbit->ObjectFrame), 2));
	}

	// Render the first sphere/shield frame
	// Prepare will set the next one
	rotFrameIndex = useDosSpheres ? 0 : 1;
	PrepareNextRotationFrame ();
}

void
UninitSphereRotation (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	if (Orbit->WorkFrame)
	{
		DestroyDrawable (ReleaseDrawable (Orbit->ObjectFrame));
		Orbit->ObjectFrame = Orbit->WorkFrame;
		Orbit->WorkFrame = NULL;
	}
}

void
PrepareNextRotationFrame (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	// Generate the next rotation frame
	// We alternate between the frames because we do not call
	// FlushGraphics() The frame we just drew may not have made it to the
	// screen yet
	rotFrameIndex ^= 1;

	// Go to next point, taking care of wraparounds
	rotPointIndex += rotDirection;
	if (rotPointIndex < 0)
		rotPointIndex = rotwidth - 1;
	else if (rotPointIndex >= rotwidth)
		rotPointIndex = 0;

	// prepare the next sphere frame
	if (useDosSpheres)
	{
		if (rotFrameIndex)
			Orbit->SphereFrame = IncFrameIndex (Orbit->SphereFrame);
		else
			Orbit->SphereFrame = DecFrameIndex (Orbit->SphereFrame);

		RenderDOSPlanetSphere (Orbit, Orbit->SphereFrame, rotPointIndex);
	}
	else if (use3DOSpheres)
	{
		Orbit->SphereFrame = SetAbsFrameIndex (Orbit->SphereFrame,
				rotFrameIndex);

		Render3DOPlanetSphere (Orbit, Orbit->SphereFrame, rotPointIndex,
				rotwidth, rotheight);
	}
	else
	{
		Orbit->SphereFrame = SetAbsFrameIndex(Orbit->SphereFrame,
			rotFrameIndex);
		RenderPlanetSphere(Orbit, Orbit->SphereFrame, rotPointIndex,
			pSolarSysState->pOrbitalDesc->data_index & PLANET_SHIELDED,
			throbShield, rotwidth, rotheight,
			(rotheight >> 1) - IF_HD(2));
	}
	
	if (throbShield)
	{	// prepare the next shield throb frame
		Orbit->ObjectFrame = SetAbsFrameIndex (Orbit->ObjectFrame,
				rotFrameIndex);
		SetShieldThrobEffect (Orbit->WorkFrame, rotPointIndex,
				Orbit->ObjectFrame);
	}
}

void
PrepareNextRotationFrameForIP (PLANET_DESC *pPlanetDesc, SIZE frameCounter)
{
	PLANET_ORBIT *Orbit = &pPlanetDesc->orbit;
	int oldPointIndex = pPlanetDesc->rotPointIndex;
	// Go to next point, taking care of wraparounds

	// No need to rotate planets that are off screen
	if (pPlanetDesc->radius > 4 * pSolarSysState->SunDesc[0].radius)
		return;

#ifdef NEVER
	{
		COUNT framerate;
		// Optimization : the smallest worlds are rotated only once in a
		// while. The framerate is fine-tuned so that the planet is updated
		// when the landscape has moved 1 pixel approximately
		switch (pPlanetDesc->size)
		{
				case 3: framerate = 15;
					break;
				case 4: framerate = 10;
					break;
				case 7: framerate = 4;
					break;
				case 11: framerate = 2;
					break;
				default: framerate = 1;
					break;
		}
		if ((frameCounter % framerate) != 0)
			return;
	}
#endif

	(void) frameCounter;  /* Satisfying compiler */

	// BW: account for rotation period
	pPlanetDesc->rotPointIndex =
			(int)(fmod (pPlanetDesc->rot_speed * daysElapsed (),
					pPlanetDesc->rotwidth));

	if (pPlanetDesc->rotPointIndex < 0)
		pPlanetDesc->rotPointIndex += pPlanetDesc->rotwidth;

	// Nothing to do if there has been no visible rotation
	if (pPlanetDesc->rotPointIndex == oldPointIndex)
		return;

	// Generate the next rotation frame
	// We alternate between the frames because we do not call
	// FlushGraphics(). The frame we just drew may not have made it to the
	// screen yet
	pPlanetDesc->rotFrameIndex ^= 1;

	// prepare the next sphere frame
	Orbit->SphereFrame = SetAbsFrameIndex (Orbit->SphereFrame,
			pPlanetDesc->rotFrameIndex);
	RenderPlanetSphere (Orbit, Orbit->SphereFrame,
			pPlanetDesc->rotPointIndex,
			pPlanetDesc->data_index & PLANET_SHIELDED,
			FALSE, pPlanetDesc->rotwidth, pPlanetDesc->rotheight,
			(pPlanetDesc->rotheight >> 1) - IF_HD (2)); // RADIUS
	Orbit->SphereFrame->image->dirty = TRUE;
	// BW: slightly hacky but, in DrawTexturedBody, the call
	// to DrawStamp won't re-blit the frame unless scale has changed.
}

#if SDL_MAJOR_VERSION == 1
#define ZOOM_RATE  RES_BOOL (24, 42)
#else
#define ZOOM_RATE  RES_SCALE (35)
#endif
#define ZOOM_TIME  (ONE_SECOND * 6 / 5)


// This takes care of zooming the planet sphere into place
// when entering orbit
void
ZoomInPlanetSphere (void)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	const int base = GSCALE_IDENTITY;
	int dx, dy;
	int oldScale;
	int oldMode;
	int i;
	int frameCount;
	int zoomCorner;
	RECT frameRect;
	RECT repairRect;
	TimeCount NextTime, Now, RenderNextTime;

	frameCount = ZOOM_TIME / (ONE_SECOND / ZOOM_RATE);

	// Planet zoom in from a randomly chosen corner
	zoomCorner = TFB_Random ();
	dx = 1 - (zoomCorner & 1) * 2;
	dy = 1 - (zoomCorner & 2);

	if (Orbit->ObjectFrame)
		GetFrameRect (Orbit->ObjectFrame, &frameRect);
	else
		GetFrameRect (Orbit->SphereFrame, &frameRect);
	repairRect = frameRect;
	RenderNextTime = 0;

	for (i = 0; i <= frameCount; ++i)
	{
		double scale;
		POINT pt;

		Now = GetTimeCounter ();
		NextTime = Now + (ONE_SECOND / ZOOM_RATE);

		// Use 1 + e^-2 - e^(-2x / frameCount)) function to get a
		// decelerating zoom like the one 3DO does (supposedly)
		if (i < frameCount)
			scale = 1.134 - exp (-2.0 * i / frameCount);
		else
			scale = 1.0; // final frame

		// start from beyond the screen
		pt.x = RES_SCALE (ORIG_SIS_SCREEN_WIDTH >> 1) 
				+ (int) (dx * (1.0 - scale)
				* RES_SCALE (ORIG_SIS_SCREEN_WIDTH * 6 / 10) + 0.5);
		pt.y = PLANET_ORG_Y + (int) (dy * (1.0 - scale)
				* (RES_SCALE (SCAN_SCREEN_HEIGHT) * 6 / 10) + 0.5);

		SetContext (PlanetContext);

		BatchGraphics ();
		if (i > 0)
			RepairBackRect (&repairRect);

		oldMode = SetGraphicScaleMode (TFB_SCALE_BILINEAR);
		oldScale = SetGraphicScale ((int)(base * scale + 0.5));
		DrawPlanetSphere (pt.x, pt.y, FALSE);
		SetGraphicScale (oldScale);
		SetGraphicScaleMode (oldMode);

		UnbatchGraphics ();

		repairRect.corner.x = pt.x + frameRect.corner.x;
		repairRect.corner.y = pt.y + frameRect.corner.y;

		if (Now >= RenderNextTime)
		{
			RenderNextTime = Now + PLANET_ROTATION_RATE (optScanSphere);
			PrepareNextRotationFrame ();
		}

		SleepThreadUntil (NextTime);
	}
}

#if SDL_MAJOR_VERSION == 1
#define PLANET_ROTATION_FPS (ONE_SECOND / RES_BOOL (24, 42))
#else
#define PLANET_ROTATION_FPS (ONE_SECOND / 60)
#endif

void
RotatePlanetSphere (BOOLEAN keepRate, STAMP *onTop)
{
	static TimeCount NextTime, OutNextTime, TimeOutClock;
	TimeCount Now = GetTimeCounter ();

	if (keepRate && Now < NextTime)
		return; // not time yet

	if (DIF_HARD && Now >= TimeOutClock)
	{
		GameClockTick ();
		TimeOutClock = Now + CLOCK_FRAME_RATE;
	}

	if (Now >= NextTime)
	{
		NextTime = Now + PLANET_ROTATION_RATE (optScanSphere);

		if (Now >= OutNextTime)
		{
			OutNextTime = Now + PLANET_ROTATION_FPS;

			DrawDefaultPlanetSphere ();

			if (onTop)
				DrawStamp (onTop);
		}
		PrepareNextRotationFrame ();
	}
}

static void
renderTintFrame (Color tintColor)
{
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;
	CONTEXT oldContext;
	DrawMode mode, oldMode;
	STAMP s;
	RECT r;
	double is_red;

	if (sameColor (BRIGHT_RED_COLOR, tintColor))
		is_red = 0.75;
	else
		is_red = 0.5;

	oldContext = SetContext (OffScreenContext);
	SetContextFGFrame (Orbit->TintFrame);
	SetContextClipRect (NULL);
	// get the rect of the whole context (or our frame really)
	GetContextClipRect (&r);

	// copy the topo frame to the tint frame
	s.origin.x = 0;
	s.origin.y = 0;
	s.frame = pSolarSysState->TopoFrame;
	DrawStamp (&s);

	// apply the tint
	mode = MAKE_DRAW_MODE (DRAW_ADDITIVE, DRAW_FACTOR_1 * is_red);

	oldMode = SetContextDrawMode (mode);
	SetContextForeGroundColor (tintColor);
	DrawFilledRectangle (&r);
	SetContextDrawMode (oldMode);

	SetContext (oldContext);
}

void
DrawPCScanTint (COUNT scan)
{
	STAMP s;

	s.origin.x = 0;
	s.origin.y = 0;

	s.frame = pSolarSysState->ScanFrame[scan];
	//XFormColorMap (GetColorMapAddress (SetAbsColorMapIndex (
	//		Orbit->sphereMap, scan + 1)), 0);
	
	DrawStamp (&s);
}

// tintColor.a is ignored
void
DrawPlanet (int tintY, Color tintColor)
{
	STAMP s;
	PLANET_ORBIT *Orbit = &pSolarSysState->Orbit;

	s.origin.x = 0;
	s.origin.y = 0;
	
	BatchGraphics ();
	if (sameColor (tintColor, BLACK_COLOR))
	{	// no tint -- just draw the surface
		s.frame = pSolarSysState->TopoFrame;
		DrawStamp (&s);
	}
	else
	{	// apply different scan type tints
		FRAME tintFrame = Orbit->TintFrame;
		int height = GetFrameHeight (tintFrame);

		if (!sameColor (tintColor, Orbit->TintColor))
		{
			renderTintFrame (tintColor);
			Orbit->TintColor = tintColor;
		}
		
		if (tintY < height - 1)
		{	// untinted piece showing, draw regular topo
			s.frame = pSolarSysState->TopoFrame;
			DrawStamp (&s);
		}

		if (tintY >= 0)
		{	// tinted piece showing, draw tinted piece
			RECT oldClipRect;
			RECT clipRect;
			RECT edge;
			COUNT i;

			// adjust cliprect to confine the tint
			GetContextClipRect (&oldClipRect);
			clipRect = oldClipRect;
			clipRect.extent.height = tintY + 1;
			SetContextClipRect (&clipRect);
			s.frame = tintFrame;
			DrawStamp (&s);
			SetContextClipRect (&oldClipRect);

			edge.extent.height = 1;
			edge.extent.width = SCALED_MAP_WIDTH;
			edge.corner.x = tintFrame->HotSpot.x;
			edge.corner.y = clipRect.extent.height;

			for (i = 0x90; i > 0; i -= 0x90 / RES_SCALE (3))
			{
				tintColor.a = i;
				SetContextForeGroundColor (tintColor);
				DrawFilledRectangle (&edge);
				edge.corner.y--;
			}

			tintColor = BLACK_COLOR;
			edge.corner.y = clipRect.extent.height + 1;

			for (i = 0x40; i > 0; i -= 8)
			{
				tintColor.a = i;
				SetContextForeGroundColor (tintColor);
				DrawFilledRectangle (&edge);
				if (!IS_HD)
					break;
				edge.corner.y++;
			}
		}
	}
	UnbatchGraphics ();
}
