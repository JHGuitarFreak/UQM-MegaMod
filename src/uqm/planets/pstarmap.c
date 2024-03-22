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

#include "scan.h"
#include "../colors.h"
#include "../controls.h"
#include "../menustat.h"
#include "../starmap.h"
#include "../races.h"
#include "../gameopt.h"
#include "../gamestr.h"
#include "../gendef.h"
#include "../globdata.h"
#include "../shipcont.h"
#include "../units.h"
#include "../hyper.h"
#include "../sis.h"
		// for DrawHyperCoords(), DrawStatusMessage()
#include "../settings.h"
#include "../setup.h"
#include "../sounds.h"
#include "../state.h"
#include "../init.h"
#include "../uqmdebug.h"
#include "options.h"
#include "libs/inplib.h"
#include "libs/strlib.h"
#include "libs/graphics/gfx_common.h"
#include "libs/mathlib.h"
#include "libs/memlib.h"
#include "../util.h"
		// For get_fuel_to_sol()
#include <stdlib.h>
#include <ctype.h> 
		// For isdigit()
#include <math.h> // sqrt()
#include "../build.h"
		// For StartSphereTracking()
#include "uqm/setupmenu.h"

typedef enum {
	NORMAL_STARMAP,
	WAR_ERA_STARMAP,
	CONSTELLATION_MAP,
	HOMEWORLDS_MAP,
	RAINBOW_MAP,
	NUM_STARMAPS
} CURRENT_STARMAP_SHOWN;

typedef struct namePlate {
	RECT rect;
	TEXT text;
	BYTE index;
} NAMEPLATE;

static void
SwapPlates (NAMEPLATE *n1, NAMEPLATE *n2)
{
	NAMEPLATE temp = *n1;
	*n1 = *n2;
	*n2 = temp;
}

#define IGNORE_MOVING_SOI (1 << 5)
#define PRE_DEATH_SOI (1 << 6)
#define DEATH_SOI (1 << 7)

static POINT cursorLoc;
static POINT mapOrigin;
static int zoomLevel;
static FRAME StarMapFrame;
static CURRENT_STARMAP_SHOWN which_starmap;

static inline long
signedDivWithError (long val, long divisor)
{
	int invert = 0;
	if (val < 0)
	{
		invert = 1;
		val = -val;
	}
	val = (val + ROUNDING_ERROR (divisor)) / divisor;
	return invert ? -val : val;
}

#define MAP_FIT_X ((MAX_X_UNIVERSE + 1) / ORIG_SIS_SCREEN_WIDTH + 1)
#define MAP_FIT_XX ((MAX_X_UNIVERSE + 1) / SIS_SCREEN_WIDTH + 1)

static inline COORD
universeToDispx (long ux)
{
	return signedDivWithError (((ux - mapOrigin.x) << zoomLevel)
			* ORIG_SIS_SCREEN_WIDTH, MAX_X_UNIVERSE + MAP_FIT_X)
			+ ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1);
}
#define UNIVERSE_TO_DISPX(ux)  RES_SCALE (universeToDispx(ux))
#define ORIG_UNIVERSE_TO_DISPX(ux)  universeToDispx(ux)

static inline COORD
universeToDispy (long uy)
{
	return signedDivWithError (((mapOrigin.y - uy) << zoomLevel)
			* ORIG_SIS_SCREEN_HEIGHT, MAX_Y_UNIVERSE + 2)
			+ ((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1);
}
#define UNIVERSE_TO_DISPY(uy)  RES_SCALE (universeToDispy(uy))
#define ORIG_UNIVERSE_TO_DISPY(uy)  universeToDispy(uy)

static inline COORD
dispxToUniverse (COORD dx)
{
	return (((long)(dx - ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1))
			* (MAX_X_UNIVERSE + MAP_FIT_X)) >> zoomLevel)
			/ ORIG_SIS_SCREEN_WIDTH + mapOrigin.x;
}
#define DISP_TO_UNIVERSEX(dx) dispxToUniverse(RES_DESCALE (dx))
#define ORIG_DISP_TO_UNIVERSEX(dx) dispxToUniverse(dx)

static inline COORD
dispyToUniverse (COORD dy)
{
	return (((long)(((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1) - dy)
			* (MAX_Y_UNIVERSE + 2)) >> zoomLevel)
			/ ORIG_SIS_SCREEN_HEIGHT + mapOrigin.y;
}
#define DISP_TO_UNIVERSEY(dy) dispyToUniverse(RES_DESCALE (dy))
#define ORIG_DISP_TO_UNIVERSEY(dy) dispyToUniverse(dy)

// Old school HD-mod code for Malin's Sol ellipse.
static inline COORD
universeToDispx2 (COORD ux)
{
	long v = signedDivWithError ((((long)ux - mapOrigin.x) << zoomLevel)
		* SIS_SCREEN_WIDTH, MAX_X_UNIVERSE + MAP_FIT_XX)
		+ ((SIS_SCREEN_WIDTH - 1) >> 1);
	if (v > 32767) { return 32767; }
	if (v < -32768) { return -32768; }
	return v;
}
static inline COORD
universeToDispy2 (COORD uy)
{
	long v = signedDivWithError ((((long)mapOrigin.y - uy) << zoomLevel)
		* SIS_SCREEN_HEIGHT, MAX_Y_UNIVERSE + 2)
		+ ((SIS_SCREEN_HEIGHT - 1) >> 1);
	if (v > 32767) { return 32767; }
	if (v < -32768) { return -32768; }
	return v;
}

#define UNIVERSE_TO_DISPX2(ux) \
		(IS_HD ? universeToDispx2(ux) : universeToDispx(ux))
#define UNIVERSE_TO_DISPY2(uy) \
		(IS_HD ? universeToDispy2(uy) : universeToDispy(uy))

static BOOLEAN transition_pending;

static void
flashCurrentLocation (POINT *where, BOOLEAN force)
{
	static BOOLEAN redraw = FALSE;
	static BYTE c = 0;
	static int val = -2;
	static POINT universe;
	static TimeCount NextTime = 0;

	if (where)
		universe = *where;

	if (GetTimeCounter () >= NextTime)
	{
		NextTime = GetTimeCounter () + (ONE_SECOND / 16);

		if (c == 0x00 || c == 0x1A)
			val = -val;
		c += val;

		redraw = TRUE;
	}

	if (force || redraw)
	{
		Color OldColor;
		CONTEXT OldContext;
		STAMP s;

		OldContext = SetContext (SpaceContext);

		OldColor = SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (c, c, c), c));
		s.origin.x = UNIVERSE_TO_DISPX (universe.x);
		s.origin.y = UNIVERSE_TO_DISPY (universe.y);
		s.frame = IncFrameIndex (StarMapFrame);
		DrawFilledStamp (&s);
		SetContextForeGroundColor (OldColor);

		SetContext (OldContext);

		redraw = FALSE;
	}
}

static void
DrawCursor (COORD curs_x, COORD curs_y)
{
	STAMP s;

	s.origin.x = curs_x;
	s.origin.y = curs_y;
	s.frame = StarMapFrame;

	DrawStamp (&s);
}

static void
DrawMarker (POINT dest, BYTE type)
{
	STAMP s;

	if (type > 2)
		return;

	s.origin = MAKE_POINT (
			UNIVERSE_TO_DISPX (dest.x),
			UNIVERSE_TO_DISPY (dest.y));
	s.frame = SetAbsFrameIndex (MiscDataFrame, 106 + type);

	DrawStamp (&s);
}

static void
DrawAutoPilot (POINT *pDstPt)
{
	SIZE dx, dy,
				xincr, yincr,
				xerror, yerror,
				cycle, delta;
	POINT pt;
	STAMP s;

	if (!inHQSpace ())
		pt = CurStarDescPtr->star_pt;
	else
	{
		pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	if (IS_HD)
		s.frame = DecFrameIndex (stars_in_space);

	pt.x = UNIVERSE_TO_DISPX (pt.x);
	pt.y = UNIVERSE_TO_DISPY (pt.y);

	dx = UNIVERSE_TO_DISPX (pDstPt->x) - pt.x;
	if (dx >= 0)
		xincr = 1;
	else
	{
		xincr = -1;
		dx = -dx;
	}
	dx <<= 1;

	dy = UNIVERSE_TO_DISPY (pDstPt->y) - pt.y;
	if (dy >= 0)
		yincr = 1;
	else
	{
		yincr = -1;
		dy = -dy;
	}
	dy <<= 1;

	if (dx >= dy)
		cycle = dx;
	else
		cycle = dy;
	delta = xerror = yerror = cycle >> 1;

	SetContextForeGroundColor (
			BUILD_COLOR (MAKE_RGB15 (0x04, 0x04, 0x1F), 0x01));

	delta &= ~1;
	while (delta--)
	{
		if (IS_HD)
		{
			if (delta % 8 == 0 && delta != 0)
			{	// every eighth dot and not the last dot
				s.origin.x = pt.x;
				s.origin.y = pt.y;
				DrawFilledStamp (&s);
			}
		}
		else if (!(delta & 1))
			DrawPoint (&pt);

		if ((xerror -= dx) <= 0)
		{
			pt.x += xincr;
			xerror += cycle;
		}
		if ((yerror -= dy) <= 0)
		{
			pt.y += yincr;
			yerror += cycle;
		}
	}
}

static void
GetSphereRect (FLEET_INFO *FleetPtr, RECT *pRect, RECT *pRepairRect)
{
	long diameter;

	diameter = (long)(FleetPtr->known_strength * SPHERE_RADIUS_INCREMENT);
	pRect->extent.width = UNIVERSE_TO_DISPX (diameter)
			- UNIVERSE_TO_DISPX (0);
	if (pRect->extent.width < 0)
		pRect->extent.width = -pRect->extent.width;
	else if (pRect->extent.width == 0)
		pRect->extent.width = RES_SCALE (1);
	pRect->extent.height = UNIVERSE_TO_DISPY (diameter)
			- UNIVERSE_TO_DISPY (0);
	if (pRect->extent.height < 0)
		pRect->extent.height = -pRect->extent.height;
	else if (pRect->extent.height == 0)
		pRect->extent.height = RES_SCALE (1);

	pRect->corner.x = UNIVERSE_TO_DISPX (FleetPtr->known_loc.x);
	pRect->corner.y = UNIVERSE_TO_DISPY (FleetPtr->known_loc.y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	{
		TEXT t;
		STRING locString;

		t.baseline.x = pRect->corner.x + (pRect->extent.width >> 1);
		t.baseline.y = pRect->corner.y + (pRect->extent.height >> 1)
				- RES_SCALE (1);
		t.align = ALIGN_CENTER;
		locString = SetAbsStringTableIndex (FleetPtr->race_strings, 1);
		t.CharCount = GetStringLength (locString);
		t.pStr = (UNICODE *)GetStringAddress (locString);
		TextRect (&t, pRepairRect, NULL);
		
		if (pRepairRect->corner.x <= 0)
			pRepairRect->corner.x = RES_SCALE (1);
		else if (pRepairRect->corner.x + pRepairRect->extent.width >=
				SIS_SCREEN_WIDTH)
			pRepairRect->corner.x =
					SIS_SCREEN_WIDTH - pRepairRect->extent.width
					- RES_SCALE (1);
		if (pRepairRect->corner.y <= 0)
			pRepairRect->corner.y = RES_SCALE (1);
		else if (pRepairRect->corner.y + pRepairRect->extent.height >=
				SIS_SCREEN_HEIGHT)
			pRepairRect->corner.y =
					SIS_SCREEN_HEIGHT - pRepairRect->extent.height
					- RES_SCALE (1);

		BoxUnion (pRepairRect, pRect, pRepairRect);
		pRepairRect->extent.width += RES_SCALE (1);
		pRepairRect->extent.height += RES_SCALE (1);
	}
}

// For showing the War-Era situation in starmap
static void
GetWarEraSphereRect (COUNT index, const COUNT war_era_strengths[],
		const POINT war_era_locations[], RECT *pRect, RECT *pRepairRect)
{
	long diameter = (long)(war_era_strengths[index] * 2);

	pRect->extent.width = UNIVERSE_TO_DISPX (diameter)
			- UNIVERSE_TO_DISPX (0);
	if (pRect->extent.width < 0)
		pRect->extent.width = -pRect->extent.width;
	else if (pRect->extent.width == 0)
		pRect->extent.width = RES_SCALE (1);
	pRect->extent.height = UNIVERSE_TO_DISPY (diameter)
			- UNIVERSE_TO_DISPY (0);
	if (pRect->extent.height < 0)
		pRect->extent.height = -pRect->extent.height;
	else if (pRect->extent.height == 0)
		pRect->extent.height = RES_SCALE (1);

	pRect->corner.x = UNIVERSE_TO_DISPX (war_era_locations[index].x);
	pRect->corner.y = UNIVERSE_TO_DISPY (war_era_locations[index].y);
	pRect->corner.x -= pRect->extent.width >> 1;
	pRect->corner.y -= pRect->extent.height >> 1;

	if (pRepairRect->corner.x <= 0)
		pRepairRect->corner.x = RES_SCALE (1);
	else if (pRepairRect->corner.x + pRepairRect->extent.width >=
			SIS_SCREEN_WIDTH)
	{
		pRepairRect->corner.x = SIS_SCREEN_WIDTH -
				pRepairRect->extent.width - RES_SCALE (1);
	}

	if (pRepairRect->corner.y <= 0)
		pRepairRect->corner.y = RES_SCALE (1);
	else if (pRepairRect->corner.y + pRepairRect->extent.height >=
			SIS_SCREEN_HEIGHT)
	{
		pRepairRect->corner.y = SIS_SCREEN_HEIGHT -
				pRepairRect->extent.height - RES_SCALE (1);
	}

	BoxUnion (pRepairRect, pRect, pRepairRect);
	pRepairRect->extent.width += RES_SCALE (1);
	pRepairRect->extent.height += RES_SCALE (1);
}

static unsigned int
FuelRequiredTo (POINT dest)
{
	COUNT fuel_required;
	DWORD f;
	POINT pt;

	if (!inHQSpace ())
		pt = CurStarDescPtr->star_pt;
	else
	{
		pt.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		pt.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	pt.x -= dest.x;
	pt.y -= dest.y;

	f = (DWORD)((long)pt.x * pt.x + (long)pt.y * pt.y);
	if (f == 0 || GET_GAME_STATE (ARILOU_SPACE_SIDE) > 1)
		fuel_required = 0;
	else
		fuel_required = square_root (f) + (FUEL_TANK_SCALE / 20);

	return fuel_required;
}

// Begin Malin's fuel to Sol ellipse code. Edited by Kruzen
#define MATH_ROUND(X) ((X) + ((int)((X) + 0.5) > (X) ? 1 : 0))

POINT
GetPointOfEllipse (double a, double b, double radian)
{
	double t[2] = { a * cos (radian), b * sin (radian) };
	return (POINT) { (COORD)MATH_ROUND (t[0]), (COORD)MATH_ROUND (t[1]) };
}

POINT
ShiftPoint (POINT p, POINT S)
{
	return (POINT) { p.x + S.x, p.y + S.y };
}

POINT
RotatePoint (POINT p, POINT Pivot, double radian)
{
	double d[2] = { p.x - Pivot.x, p.y - Pivot.y };
	double cosine[2] = { cos (radian), sin (radian) };
	double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	return (POINT) { MATH_ROUND (x), MATH_ROUND (y) };
}

// Kruzen: Merged together with overflow check. Functions above are redundant
POINT
CalcEllipsePoint (double a, double b, double rad_one, double rad_two, POINT Pivot)
{
	double q[2] = { a * cos (rad_one), b * sin (rad_one) };
	double w[2] = { MATH_ROUND (q[0]) + Pivot.x, MATH_ROUND (q[1]) + Pivot.y };

	double d[2] = { w[0] - Pivot.x, w[1] - Pivot.y};
	double cosine[2] = { cos (rad_two), sin (rad_two) };
	double x = Pivot.x + (d[0] * cosine[0] - d[1] * cosine[1]);
	double y = Pivot.y + (d[0] * cosine[1] + d[1] * cosine[0]);

	x = MATH_ROUND(x);
	if (x > 32767.0) { x = 32767.0; }
	if (x < -32768.0) { x = -32768.0; }

	y = MATH_ROUND (y);
	if (y > 32767.0) { y = 32767.0; }
	if (y < -32768.0) { y = -32768.0; }

	return (POINT) { UNIVERSE_TO_DISPX2 (x), UNIVERSE_TO_DISPY2 (y) };
}

BOOLEAN
onScreen (LINE *l, BOOLEAN ignoreX, BOOLEAN ignoreY)
{
	return !((l->first.x < 0 && l->second.x < 0 && !ignoreX)
			|| (l->first.x >= SIS_SCREEN_WIDTH
				&& l->second.x >= SIS_SCREEN_WIDTH && !ignoreX)
			|| (l->first.y < 0 && l->second.y < 0 && !ignoreY)
			|| (l->first.y >= SIS_SCREEN_HEIGHT
				&& l->second.y >= SIS_SCREEN_HEIGHT && !ignoreY));
}

static void
DrawNoReturnZone (void)
{
	double dist;
	POINT sol, sis;
	double halfFuel = GLOBAL_SIS (FuelOnBoard) / 2;

	sol = (POINT){ SOL_X, SOL_Y };
	sis = (POINT){ LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
			LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)) };

	dist = (double)FuelRequiredTo (sol) / 2;
	
	if (dist <= halfFuel)
	{	// do not draw ellipse when fuel is not enough to reach Sol
		POINT curr, center, rmax_y, rmin_y;
		double i, Step, ry, rotation;

		ry = sqrt (pow (halfFuel, 2) - pow (dist, 2));

		// on max zoom ellipse edge becomes wobbly, so we cut num of
		// iterations in half on zoomLevel 3 and 4
		Step = (M_PI / (180.0f / (zoomLevel > 2 ? 2 : 1)));
		center = MAKE_POINT ((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);
		rotation = atan2 (sol.y - sis.y, sol.x - sis.x);

		rmax_y = (POINT){ -1 , -1 };
		rmin_y = (POINT){ SIS_SCREEN_WIDTH, SIS_SCREEN_HEIGHT };

		for (i = 0; i < M_PI * 2; i += Step)
		{
			curr = CalcEllipsePoint (halfFuel, ry, i, rotation, center);
			if (curr.y > rmax_y.y)
				rmax_y = curr;

			if (curr.y < rmin_y.y)
				rmin_y = curr;
		}

		if (rmax_y.y >= 0 || rmin_y.y < SIS_SCREEN_HEIGHT)
		{// If the ellipse is completely off screen - drop it
			LINE L;
			LINE tempLine;
			POINT prev = CalcEllipsePoint (halfFuel, ry, i - Step, rotation, center);
			COORD dy;
			double err = ((double)rmax_y.x - (double)rmin_y.x)
					/ ((double)rmax_y.y - (double)rmin_y.y);

			for (i = 0; i < M_PI * 2; i += Step)
			{
				L.first = CalcEllipsePoint (halfFuel, ry, i, rotation, center);
				L.second.x = rmax_y.x
						- (COORD)(err * (rmax_y.y - L.first.y));
				L.second.y = L.first.y;

				if (onScreen (&L, FALSE, FALSE))
					DrawLine (&L, 1);

				dy = L.first.y - prev.y;

				MAKE_LINE (&tempLine, L.first, prev);

				if ((abs (dy) > 1)
						&& onScreen (&tempLine, TRUE,
							FALSE))
				{
					LINE L2;
					COORD iter;
					double y_err = ((double)L.first.x - (double)prev.x)
							/ ((double)L.first.y - (double)prev.y);

					if (dy < 0)
						iter = -1;
					else
						iter = 1;

					while (abs (dy) > 1)
					{
						L2.first.y = L2.second.y = prev.y + dy - iter;
						L2.first.x = L.first.x
								- (COORD)(y_err
									* (L.first.y - L2.first.y));
						L2.second.x = rmax_y.x
								- (COORD)(err * (rmax_y.y - L2.first.y));

						if (onScreen (&L2, FALSE, FALSE))
							DrawLine (&L2, 1);

						dy -= iter;
					}
				}
				prev = L.first;
			}
		}
	}
}

static void
GetFuelRect (DRECT *r, SDWORD diameter, POINT corner)
{// Operating with DRECT because of overflows in HD on max zoom
	SDWORD x, y, width, height;

	if (diameter < 0)
		diameter = 0;

	// Cap the diameter to a sane range
	if (diameter > MAX_X_UNIVERSE * 4)
		diameter = MAX_X_UNIVERSE * 4;

	// Calculate in case of overflow
	width = RES_SCALE (signedDivWithError (((diameter - mapOrigin.x) << zoomLevel)
			* ORIG_SIS_SCREEN_WIDTH, MAX_X_UNIVERSE + MAP_FIT_X)
			+ ((ORIG_SIS_SCREEN_WIDTH - 1) >> 1))
			- UNIVERSE_TO_DISPX (0);// Cannot overflow in current HD resolution

	if (width < 0)
		width = -width;

	height = RES_SCALE (signedDivWithError (((mapOrigin.y - diameter) << zoomLevel)
			* ORIG_SIS_SCREEN_HEIGHT, MAX_Y_UNIVERSE + 2)
			+ ((ORIG_SIS_SCREEN_HEIGHT - 1) >> 1))
			- UNIVERSE_TO_DISPY (0);// Cannot overflow in current HD resolution

	if (height < 0)
		height = -height;

	// SIS cannot leave universe boundaries, so cannot overflow either
	x = UNIVERSE_TO_DISPX (corner.x) - (width >> 1);
	y = UNIVERSE_TO_DISPY (corner.y) - (height >> 1);

	r->corner.x = x;
	r->corner.y = y;
	r->extent.width = width;
	r->extent.height = height;	
}

static void
DrawFuelCircle (BOOLEAN secondary)
{
	DRECT r;
	POINT corner;
	Color OldColor;
	DWORD OnBoardFuel = GLOBAL_SIS (FuelOnBoard);

	if (secondary)
	{
		OnBoardFuel -= FuelRequiredTo (GLOBAL (autopilot));
		corner = GLOBAL (autopilot);
	}
	else if (!inHQSpace())
		corner = CurStarDescPtr->star_pt;
	else
	{
		corner.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		corner.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	GetFuelRect (&r, OnBoardFuel << 1, corner);

	if (secondary)
	{
		OldColor = SetContextForeGroundColor (DKGRAY_COLOR);
		DrawOval (&r, RES_BOOL (1,6), FALSE);
		SetContextForeGroundColor (OldColor);
	}
	else
	{
		/* Draw circle*/
		OldColor = SetContextForeGroundColor (STARMAP_FUEL_RANGE_COLOR);
		DrawFilledOval (&r);
		SetContextForeGroundColor (OldColor);

		if (optFuelRange > 1)
		{
			OldColor =
				SetContextForeGroundColor (STARMAP_SECONDARY_RANGE_COLOR);
			if (pointsEqual (corner, (POINT) { SOL_X, SOL_Y }))
			{// We are at Sol, foci are equal - draw a standard oval
				GetFuelRect (&r, OnBoardFuel, corner);
				DrawFilledOval (&r);
			}
			else
				DrawNoReturnZone ();

			SetContextForeGroundColor (OldColor);
		}
	}
}

// Taleden code of drawing ellipse. Unused because not precise enough
/*static void
DrawFuelEllipse ()
{
	Color OldColor;
	POINT center, sol, sis;
	double ry, dist, angle;
	double halfFuel = GLOBAL_SIS (FuelOnBoard) / 2;

	sol = (POINT){ SOL_X, SOL_Y };
	sis = (POINT){ LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
			LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y)) };

	dist = FuelRequiredTo (sol) / 2;

	if (dist >= halfFuel)
		return;
	ry = sqrt (pow (halfFuel, 2) - pow (dist, 2));
	angle = atan2 (sis.y - sol.y, sis.x - sol.x) * 180.0 / M_PI;
	center = MAKE_POINT ((sis.x + sol.x) / 2, (sis.y + sol.y) / 2);

	// convert starmap coords to screen coords
	center.x = UNIVERSE_TO_DISPX (center.x);
	center.y = UNIVERSE_TO_DISPY (center.y);

	halfFuel = UNIVERSE_TO_DISPX (halfFuel) - UNIVERSE_TO_DISPX (0);
	if (halfFuel < 0)
		halfFuel = -halfFuel;
	ry = UNIVERSE_TO_DISPY (ry) - UNIVERSE_TO_DISPY (0);
	if (ry < 0)
		ry = -ry;

	// draw
	OldColor = SetContextForeGroundColor (STARMAP_SECONDARY_RANGE_COLOR);
	DrawRotatedEllipse (center.x, center.y, halfFuel, ry, angle, 1, 0);
	SetContextForeGroundColor (OldColor);
}*/

BOOLEAN
isHomeworld (BYTE Index)
{
	BOOLEAN raceBool = FALSE;

	switch (Index)
	{
		case CHMMR_DEFINED:
			if (IsHomeworldKnown (CHMMR_HOME)
				&& CheckAlliance (CHMMR_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case ORZ_DEFINED:
			if (IsHomeworldKnown (ORZ_HOME)
				&& CheckAlliance (ORZ_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case PKUNK_DEFINED:
			if (IsHomeworldKnown (PKUNK_HOME)
				&& CheckAlliance (PKUNK_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SHOFIXTI_DEFINED:
			if (IsHomeworldKnown (SHOFIXTI_HOME)
				&& CheckAlliance (SHOFIXTI_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SPATHI_DEFINED:
			if (IsHomeworldKnown (SPATHI_HOME)
				&& CheckAlliance (SPATHI_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SUPOX_DEFINED:
			if (IsHomeworldKnown (SUPOX_HOME)
				&& CheckAlliance (SUPOX_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case THRADD_DEFINED:
			if (IsHomeworldKnown (THRADDASH_HOME)
				&& CheckAlliance (THRADDASH_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case UTWIG_DEFINED:
			if (IsHomeworldKnown (UTWIG_HOME)
				&& CheckAlliance (UTWIG_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case VUX_DEFINED:
			if (IsHomeworldKnown (VUX_HOME)
				&& CheckAlliance (VUX_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case YEHAT_DEFINED:
			if (IsHomeworldKnown (YEHAT_HOME)
				&& CheckAlliance (YEHAT_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case DRUUGE_DEFINED:
			if (IsHomeworldKnown (DRUUGE_HOME)
				&& CheckAlliance (DRUUGE_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case ILWRATH_DEFINED:
			if (IsHomeworldKnown (ILWRATH_HOME)
				&& CheckAlliance (ILWRATH_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case MYCON_DEFINED:
			if (IsHomeworldKnown (MYCON_HOME)
				&& CheckAlliance (MYCON_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SLYLANDRO_DEFINED:
			if (IsHomeworldKnown (SLYLANDRO_HOME)
				&& Index == SLYLANDRO_DEFINED)
				raceBool = TRUE;
			break;
		case UMGAH_DEFINED:
			if (IsHomeworldKnown (UMGAH_HOME)
				&& CheckAlliance (UMGAH_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case ZOQFOT_DEFINED:
			if (IsHomeworldKnown (ZOQFOTPIK_HOME)
				&& CheckAlliance (ZOQFOTPIK_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SYREEN_DEFINED:
			if (IsHomeworldKnown (SYREEN_HOME)
				&& CheckAlliance (SYREEN_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case ANDROSYNTH_DEFINED:
			if (IsHomeworldKnown (ANDROSYNTH_HOME)
				&& Index == ANDROSYNTH_DEFINED
				&& CheckAlliance (ANDROSYNTH_SHIP) != DEAD_GUY)
				raceBool = TRUE;
			break;
		case SOL_DEFINED:
		case START_COLONY_DEFINED:
			raceBool = TRUE;
			break;
	}

	return raceBool;
}

const char *
markerBuf (const int star_index, const char* marker_state)
{
	static char buf[255];

	// marker_state is the middle part of the Game States
	// "SYS_VISITED_##" or "SYS_PLYR_MARKER_##" which is used to
	// differentiate between which kind of marker we're working with.

	snprintf (buf, sizeof (buf), "SYS_%s_%02u", marker_state,
			star_index / 32);

	return buf;
}

BOOLEAN
isStarMarked (const int star_index, const char *marker_state)
{
	int starIndex = star_index;
	DWORD starData;

	if (star_index == INTERNAL_STAR_INDEX)
		starIndex = (COUNT)(CurStarDescPtr - star_array);

	starData = D_GET_GAME_STATE (markerBuf (starIndex, marker_state));

	return (starData >> (starIndex % 32)) & 1;
}

void
setStarMarked (const int star_index, const char *marker_state)
{
	int starIndex = star_index;
	DWORD starData;

	if (starIndex == INTERNAL_STAR_INDEX)
		starIndex = (COUNT)(CurStarDescPtr - star_array);

	starData = D_GET_GAME_STATE (markerBuf (starIndex, marker_state));
	starData ^= (1 << (starIndex % 32));
	D_SET_GAME_STATE (markerBuf (starIndex, marker_state), starData);
}

static COORD
CheckTextsIntersect (RECT *curr, RECT *prev)
{
	if (((curr->corner.x + curr->extent.width) <= prev->corner.x) ||
			((prev->corner.x + prev->extent.width) <= curr->corner.x))
		return 0;

	if (((curr->corner.y + curr->extent.height) <= prev->corner.y) ||
			((prev->corner.y + prev->extent.height) <= curr->corner.y))
		return 0;

	return ((prev->extent.height + RES_SCALE (1)) - abs (curr->corner.y -
			prev->corner.y));
}

static void
AdjustTextRect (RECT *r, TEXT *t)
{
	COORD offs;
	if (r->corner.x <= 0)
	{
		offs = r->corner.x - RES_SCALE (1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	else if (r->corner.x + r->extent.width
		>= SIS_SCREEN_WIDTH)
	{
		offs = (r->corner.x + r->extent.width)
			- SIS_SCREEN_WIDTH + RES_SCALE (1);
		t->baseline.x -= offs;
		r->corner.x -= offs;
	}
	if (r->corner.y <= 0)
	{
		offs = r->corner.y - RES_SCALE (1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
	else if (r->corner.y + r->extent.height
		>= SIS_SCREEN_HEIGHT)
	{
		offs = (r->corner.y + r->extent.height)
			- SIS_SCREEN_HEIGHT + RES_SCALE (1);
		t->baseline.y -= offs;
		r->corner.y -= offs;
	}
}

static void
DrawRaceName (TEXT *t, Color *c)
{
	// The text color is slightly lighter than the color of
	// the SoI.
	c->r = (c->r >= 0xff - CC5TO8 (0x03)) ?
			0xff : c->r + CC5TO8 (0x03);
	c->g = (c->g >= 0xff - CC5TO8 (0x03)) ?
			0xff : c->g + CC5TO8 (0x03);
	c->b = (c->b >= 0xff - CC5TO8 (0x03)) ?
			0xff : c->b + CC5TO8 (0x03);

	SetContextForeGroundColor (*c);
	font_DrawText (t);
}

static void
DrawStarMap (COUNT race_update, RECT *pClipRect)
{
#define GRID_DELTA 500
	SIZE i;
	COUNT which_space;
	// long diameter;
	RECT r, old_r;
	POINT oldOrigin = {0, 0};
	STAMP s;
	FRAME star_frame;
	STAR_DESC *SDPtr;
	BOOLEAN draw_cursor;

	if (pClipRect == (RECT*)-1)
	{
		pClipRect = 0;
		draw_cursor = FALSE;
	}
	else
	{
		draw_cursor = TRUE;
	}

	SetContext (SpaceContext);
	if (pClipRect)
	{
		GetContextClipRect (&old_r);
		pClipRect->corner.x += old_r.corner.x;
		pClipRect->corner.y += old_r.corner.y;
		SetContextClipRect (pClipRect);
		pClipRect->corner.x -= old_r.corner.x;
		pClipRect->corner.y -= old_r.corner.y;
		// Offset the origin so that we draw the correct gfx in the
		// cliprect
		oldOrigin = SetContextOrigin (MAKE_POINT (-pClipRect->corner.x,
				-pClipRect->corner.y));
	}

	if (transition_pending)
	{
		SetTransitionSource (NULL);
	}
	BatchGraphics ();
	
	which_space = GET_GAME_STATE (ARILOU_SPACE_SIDE);

	if (which_space <= 1)
	{
		SDPtr = &star_array[0];
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x00, 0x07), 0x57));
		SetContextBackGroundColor (BLACK_COLOR);
	}
	else
	{
		SDPtr = &star_array[NUM_SOLAR_SYSTEMS + 1];
		SetContextForeGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x0B, 0x00), 0x6D));
		SetContextBackGroundColor (
				BUILD_COLOR (MAKE_RGB15 (0x00, 0x08, 0x00), 0x6E));
	}
	ClearDrawable ();

	if (which_starmap != CONSTELLATION_MAP
			&& (race_update == 0 && which_space < 2)
			&& !(optInfiniteFuel || GLOBAL_SIS (FuelOnBoard) == 0))
	{	// Draw the fuel range circle
		DrawFuelCircle (FALSE);
	}

	{	// Horizontal lines
		r.corner.x = UNIVERSE_TO_DISPX (0);
		r.extent.width = (SIS_SCREEN_WIDTH << zoomLevel)
				- (RES_SCALE (1) << zoomLevel);
		r.extent.height = RES_SCALE (1);

		for (i = MAX_Y_UNIVERSE; i >= 0; i -= GRID_DELTA)
		{
			r.corner.y = UNIVERSE_TO_DISPY (i);
			DrawFilledRectangle (&r);
		}

		r.corner.y = UNIVERSE_TO_DISPY (0);
		DrawFilledRectangle (&r);

		// Vertical lines
		r.corner.y = UNIVERSE_TO_DISPY (MAX_Y_UNIVERSE) + RES_SCALE (1);
		r.extent.width = RES_SCALE (1);
		r.extent.height = (SIS_SCREEN_HEIGHT << zoomLevel) - RES_SCALE (1);

		for (i = 0; i < MAX_Y_UNIVERSE; i += GRID_DELTA)
		{
			r.corner.x = UNIVERSE_TO_DISPX (i);
			DrawFilledRectangle (&r);
		}

		// Edge rounding error compensation
		// so the bar wouldn't leak over the edge
		r.corner.x = UNIVERSE_TO_DISPX (MAX_X_UNIVERSE);
		r.corner.y = UNIVERSE_TO_DISPY (MAX_Y_UNIVERSE);
		r.extent.height = (SIS_SCREEN_HEIGHT << zoomLevel) + RES_SCALE (1);
		if (r.extent.height - RES_SCALE (1)
				> (-(UNIVERSE_TO_DISPY (MAX_Y_UNIVERSE)
					- UNIVERSE_TO_DISPY (0))))
		{
			r.extent.height -= RES_SCALE (1);
		}
		DrawFilledRectangle (&r);
	}

	if (which_starmap != CONSTELLATION_MAP
		&& (race_update == 0 && which_space < 2)
		&& !(optInfiniteFuel || GLOBAL_SIS (FuelOnBoard) == 0)
		&& (optFuelRange == 1 || optFuelRange == 3)
		&& (GLOBAL (autopilot.x) != ~0 && GLOBAL (autopilot.y) != ~0))
	{	// Draw the autopilot fuel range circle (on top of the grid)
		DrawFuelCircle (TRUE);
	}

	star_frame = SetRelFrameIndex (StarMapFrame, 2);
	if (which_space <= 1 && which_starmap != CONSTELLATION_MAP)
	{
		COUNT index;
		COUNT race_index = (race_update & 0x1F) - 1;
		HFLEETINFO hStarShip, hNextShip;
		NAMEPLATE nameplate[26];
		BYTE currMax = 0;
		static const Color race_colors[] =
		{
			RACE_COLORS
		};

		// JMS: For drawing SC1-era starmap.
		static const COUNT war_era_strengths[] =
		{
			WAR_ERA_STRENGTHS
		};
		static const POINT war_era_locations[] =
		{
			WAR_ERA_LOCATIONS
		};

		for (index = 0,
				hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
				hStarShip != 0; ++index, hStarShip = hNextShip)
		{
			FLEET_INFO *FleetPtr;

			FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
			hNextShip = _GetSuccLink (FleetPtr);

			if ((FleetPtr->known_strength && which_starmap != WAR_ERA_STARMAP) ||
				(which_starmap == WAR_ERA_STARMAP && war_era_strengths[index]))
			{
				RECT repair_r;

				if (which_starmap == WAR_ERA_STARMAP)
					GetWarEraSphereRect (index, war_era_strengths,
							war_era_locations, &r, &repair_r);
				else
					GetSphereRect (FleetPtr, &r, &repair_r);


				if (r.corner.x < SIS_SCREEN_WIDTH
						&& r.corner.y < SIS_SCREEN_HEIGHT
						&& r.corner.x + r.extent.width > 0
						&& r.corner.y + r.extent.height > 0
						&& (pClipRect == 0
							|| (repair_r.corner.x < pClipRect->corner.x
							+ pClipRect->extent.width
							&& repair_r.corner.y < pClipRect->corner.y
							+ pClipRect->extent.height
							&& repair_r.corner.x + repair_r.extent.width
							> pClipRect->corner.x
							&& repair_r.corner.y + repair_r.extent.height
							> pClipRect->corner.y)))
				{
					Color c;
					TEXT t;
					STRING locString;

					c = race_colors[index];
					if (index == race_index)
						SetContextForeGroundColor (WHITE_COLOR);
					else
						SetContextForeGroundColor (c);

					if (!(which_starmap == WAR_ERA_STARMAP
							&& war_era_strengths[index] == 0))
					{
						DRECT dr = RECT_TO_DRECT (r);
						DrawOval (&dr, 0, IS_HD);
					}

					if (isPC (optWhichFonts))
						SetContextFont (TinyFont);
					else
						SetContextFont (TinyFontBold);

					t.baseline.x = r.corner.x + (r.extent.width >> 1);
					t.baseline.y = r.corner.y + (r.extent.height >> 1)
							- RES_SCALE (1);
					t.align = ALIGN_CENTER;

					// For drawing War-Era starmap.
					if (which_starmap == WAR_ERA_STARMAP)
					{
						switch (index)
						{
							case PKUNK_SHIP:
							case THRADDASH_SHIP:
							case DRUUGE_SHIP:
								t.pStr = GAME_STRING (STAR_STRING_BASE + 132);
								t.CharCount = (COUNT)strlen (t.pStr);
								break;
							case ANDROSYNTH_SHIP:
								locString = SetAbsStringTableIndex(
												FleetPtr->race_strings, 0);
								t.pStr = (UNICODE *)GetStringAddress (locString);
								t.CharCount = GetStringLength (locString);
								break;
							default:
								locString = SetAbsStringTableIndex (
												FleetPtr->race_strings, 1);
								t.CharCount = GetStringLength (locString);
								t.pStr = (UNICODE *)GetStringAddress (locString);
								break;
						}						
					}
					else
					{
						locString = SetAbsStringTableIndex (
										FleetPtr->race_strings, 1);
						t.CharCount = GetStringLength (locString);
						t.pStr = (UNICODE *)GetStringAddress (locString);
					}

					TextRect (&t, &r, NULL);

					if (index == race_index && 
							race_update & IGNORE_MOVING_SOI)
					{
						AdjustTextRect (&r, &t);
						DrawRaceName (&t, &c);
					}
					else
					{
						nameplate[currMax].rect = r;
						nameplate[currMax].text = t;
						nameplate[currMax].index = index;
						currMax++;
					}
				}
			}
			else if (index == race_index &&
						race_update & (PRE_DEATH_SOI | DEATH_SOI))
			{// Kruzen: SoI is dead, but we need to fade nameplate 
				TEXT t;
				STRING locString;
				RECT repair_r;

				locString = SetAbsStringTableIndex (FleetPtr->race_strings, 1);
				t.CharCount = GetStringLength (locString);
				t.pStr = (UNICODE *)GetStringAddress (locString);

				GetSphereRect (FleetPtr, &r, &repair_r);

				t.baseline.x = r.corner.x + (r.extent.width >> 1);
				t.baseline.y = r.corner.y + (r.extent.height >> 1)
						- RES_SCALE (1);
				t.align = ALIGN_CENTER;

				TextRect (&t, &r, NULL);

				nameplate[currMax].rect = r;
				nameplate[currMax].text = t;
				nameplate[currMax].index = race_update - 1;
				currMax++;
			}
			UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		}

		if (currMax > 0)
		{
			BYTE j, k;
			BOOLEAN swapped;
			COORD offs;
			TEXT t;
			Color c;
			BYTE mid = currMax;

			for (j = 0; j < currMax - 1; j++)
			{// Sort nameplates by Y-axis from top to bottom
				swapped = FALSE;
				for (k = 0; k < currMax - j - 1; k++)
				{
					if (nameplate[k].rect.corner.y > nameplate[k + 1].rect.corner.y)
					{
						SwapPlates (&nameplate[k], &nameplate[k + 1]);
						swapped = TRUE;
					}
				}
				// If no two elements were swapped by inner loop,
				// then break
				if (swapped == FALSE)
					break;
			}

			for (j = 0; j < currMax; j++, mid--)
			{
				if (nameplate[j].index & PRE_DEATH_SOI)
					c = WHITE_COLOR;
				else if (nameplate[j].index & DEATH_SOI)
					c = BUILD_SHADE_RGBA (0x80);
				else
					c = race_colors[(nameplate[j].index & 0x1F)];
				r = nameplate[j].rect;
				t = nameplate[j].text;
				
				AdjustTextRect (&r, &t);

				r.corner.y += RES_SCALE (1);
				r.extent.height -= RES_SCALE (2);

				if (r.corner.y > (SIS_SCREEN_HEIGHT >> 1))
					break;

				for (k = 0; k < j; k++)
				{
					if ((offs = CheckTextsIntersect (&r, &nameplate[k].rect)) != 0)
					{
						r.corner.y += offs;
						t.baseline.y += offs;
					}
				}
				nameplate[j].rect = r;

				DrawRaceName (&t, &c);
			}

			for (j = 1; j <= mid; j++)
			{
				if (nameplate[currMax - j].index & PRE_DEATH_SOI)
					c = WHITE_COLOR;
				else if (nameplate[currMax - j].index & DEATH_SOI)
					c = BUILD_SHADE_RGBA (0x80);
				else
					c = race_colors[(nameplate[currMax - j].index & 0x1F)];
				r = nameplate[currMax - j].rect;
				t = nameplate[currMax - j].text;

				AdjustTextRect (&r, &t);

				r.corner.y += RES_SCALE(1);
				r.extent.height -= RES_SCALE(2);

				for (k = 0; k < j; k++)
				{
					if ((offs = CheckTextsIntersect (&r, &nameplate[currMax - k].rect)) != 0)
					{
						r.corner.y -= offs;
						t.baseline.y -= offs;
					}
				}
				nameplate[currMax - j].rect = r;

				DrawRaceName (&t, &c);
			}
		}
	}

	// Kruzen: This draws the constellation lines on the constellation
	// starmap.
	if (which_space <= 1 && which_starmap == CONSTELLATION_MAP)
	{
		Color oldColor;
		POINT *CNPtr;
		LINE l;
		BYTE c = 0x3F + IF_HD (0x11);
		CNPtr = &constel_array[0];

		oldColor = SetContextForeGroundColor (
				BUILD_COLOR_RGBA (c, c, c, 0xFF));

		while (CNPtr->x < MAX_X_UNIVERSE && CNPtr->y < MAX_Y_UNIVERSE)
		{// Have to add 2 because of HD nature (can't get exact middle of 4x4)
			l.first.x = UNIVERSE_TO_DISPX (CNPtr->x) + IF_HD (2);
			l.first.y = UNIVERSE_TO_DISPY (CNPtr->y) + IF_HD (2);
			CNPtr++;
			l.second.x = UNIVERSE_TO_DISPX (CNPtr->x) + IF_HD (2);
			l.second.y = UNIVERSE_TO_DISPY (CNPtr->y) + IF_HD (2);
			CNPtr++;
			DrawLine (&l, 1);
		}
	 	SetContextForeGroundColor (oldColor);
	}

	// This draws markers over known alien Homeworlds
	if (which_space <= 1 && which_starmap == HOMEWORLDS_MAP)
	{
		COUNT i;

		for (i = 0; i < (NUM_SOLAR_SYSTEMS + 1); ++i)
		{
			BYTE Index = star_array[i].Index;

			if (isHomeworld (Index))
				DrawMarker (star_array[i].star_pt, TRUE);
		}
	}

	// This draws markers over the Rainbow worlds
	if (which_space <= 1 && which_starmap == RAINBOW_MAP)
	{
		UWORD rainbow_mask;

		rainbow_mask = MAKE_WORD (
			GET_GAME_STATE (RAINBOW_WORLD0),
			GET_GAME_STATE (RAINBOW_WORLD1));

		if (rainbow_mask == 0)
			which_starmap = NORMAL_STARMAP;
		else
		{
			COUNT i, j = 0;

			for (i = 0; i < (NUM_SOLAR_SYSTEMS + 1); ++i)
			{
				if (star_array[i].Index == RAINBOW_DEFINED)
				{
					j++;
					if (rainbow_mask & (1 << (j - 1)))
						DrawMarker (star_array[i].star_pt, TRUE);
				}
			}
		}
	}

	do
	{	// Draws all the stars
		BYTE star_type;
		static COUNT i = 0;
		
		i = i >= NUM_SOLAR_SYSTEMS ? 0 : i;

		star_type = SDPtr->Type;

		s.origin.x = UNIVERSE_TO_DISPX (SDPtr->star_pt.x);
		s.origin.y = UNIVERSE_TO_DISPY (SDPtr->star_pt.y);
		if (which_space <= 1)
		{
			if (which_starmap == NORMAL_STARMAP
					&& isStarMarked (i, "PLYR_MARKER"))
			{	// This draws markers over tagged star systems
				DrawMarker (SDPtr->star_pt, 2);
			}

			if (optShowVisitedStars && isStarMarked (i, "VISITED")
					&& which_starmap == NORMAL_STARMAP
					&& SDPtr->Index != SOL_DEFINED)
			{
				s.frame = SetRelFrameIndex (visitedStarsFrame,
						STAR_TYPE (star_type)
						* NUM_STAR_COLORS
						+ STAR_COLOR (star_type));
			}
			else
			{
				s.frame = SetRelFrameIndex (star_frame,
						STAR_TYPE (star_type)
						* NUM_STAR_COLORS
						+ STAR_COLOR (star_type));
			}

			++i;
		}
		else if (SDPtr->star_pt.x == ARILOU_HOME_X
				&& SDPtr->star_pt.y == ARILOU_HOME_Y)
			s.frame = SetRelFrameIndex (star_frame,
					SUPER_GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		else
			s.frame = SetRelFrameIndex (star_frame,
					GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		DrawStamp (&s);

		++SDPtr;
	} while (SDPtr->star_pt.x <= MAX_X_UNIVERSE
			&& SDPtr->star_pt.y <= MAX_Y_UNIVERSE);

	if (GET_GAME_STATE (ARILOU_SPACE))
	{
		if (which_space <= 1)
		{
			s.origin.x = UNIVERSE_TO_DISPX (ARILOU_SPACE_X);
			s.origin.y = UNIVERSE_TO_DISPY (ARILOU_SPACE_Y);
		}
		else
		{
			s.origin.x = UNIVERSE_TO_DISPX (QUASI_SPACE_X);
			s.origin.y = UNIVERSE_TO_DISPY (QUASI_SPACE_Y);
		}
		s.frame = SetRelFrameIndex (star_frame,
				GIANT_STAR * NUM_STAR_COLORS + GREEN_BODY);
		DrawStamp (&s);
	}

	if (race_update == 0
			&& GLOBAL (autopilot.x) != ~0
			&& GLOBAL (autopilot.y) != ~0)
	{
		DrawAutoPilot (&GLOBAL (autopilot));
		if (IS_HD)
			DrawMarker (GLOBAL (autopilot), FALSE);
	}

	if (transition_pending)
	{
		GetContextClipRect (&r);
		ScreenTransition (optScrTrans, &r);
		transition_pending = FALSE;
	}

	if (pClipRect)
	{
		SetContextClipRect (&old_r);
		SetContextOrigin (oldOrigin);
	}

	if (race_update == 0 && draw_cursor)
	{
		GetContextClipRect (&r);
		LoadIntoExtraScreen (&r);
		DrawCursor (UNIVERSE_TO_DISPX (cursorLoc.x),
				UNIVERSE_TO_DISPY (cursorLoc.y));
		flashCurrentLocation (NULL, TRUE);
	}

	UnbatchGraphics ();
}

static void
EraseCursor (COORD curs_x, COORD curs_y)
{
	RECT r;

	GetFrameRect (StarMapFrame, &r);

	if ((r.corner.x += curs_x) < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	if ((r.corner.y += curs_y) < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;

#ifndef OLD
	RepairBackRect (&r);
#else /* NEW */
	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	DrawStarMap (0, &r);
#endif /* OLD */
}

static void
ZoomStarMap (SIZE dir)
{
#define MAX_ZOOM_SHIFT 4
	if (dir > 0)
	{
		if (zoomLevel < MAX_ZOOM_SHIFT)
		{
			++zoomLevel;
			mapOrigin = cursorLoc;

			DrawStarMap (0, NULL);
			SleepThread (ONE_SECOND / 8);
		}
	}
	else if (dir < 0)
	{
		if (zoomLevel > 0)
		{
			if (zoomLevel > 1)
				mapOrigin = cursorLoc;
			else
			{
				mapOrigin.x = MAX_X_UNIVERSE >> 1;
				mapOrigin.y = MAX_Y_UNIVERSE >> 1;
			}
			--zoomLevel;

			DrawStarMap (0, NULL);
			SleepThread (ONE_SECOND / 8);
		}
	}
}

static void
UpdateCursorLocation (int sx, int sy, const POINT *newpt)
{// Kruzen: ORIG_SIS_SCREEN_WIDTH/HEIGHT for viewport follows cursor mode
 // We're scaling the result of s.origin afterwards, but calculating
 // everything in SD values. So we can use max zoom in HD
	STAMP s;
	POINT pt;

	pt.x = ORIG_UNIVERSE_TO_DISPX(cursorLoc.x);
	pt.y = ORIG_UNIVERSE_TO_DISPY(cursorLoc.y);

	if (newpt)
	{	// absolute move
		sx = sy = 0;
		s.origin.x = ORIG_UNIVERSE_TO_DISPX (newpt->x);
		s.origin.y = ORIG_UNIVERSE_TO_DISPY (newpt->y);
		cursorLoc = *newpt;
	}
	else
	{	// incremental move
		s.origin.x = pt.x + sx;
		s.origin.y = pt.y + sy;
	}

	if (sx)
	{
		cursorLoc.x = ORIG_DISP_TO_UNIVERSEX (s.origin.x) - sx;
		while (ORIG_UNIVERSE_TO_DISPX (cursorLoc.x) == pt.x)
			cursorLoc.x += sx;
		
		if (cursorLoc.x < 0)
			cursorLoc.x = 0;
		else if (cursorLoc.x > MAX_X_UNIVERSE)
			cursorLoc.x = MAX_X_UNIVERSE;

		s.origin.x = ORIG_UNIVERSE_TO_DISPX (cursorLoc.x);
	}

	if (sy)
	{
		cursorLoc.y = ORIG_DISP_TO_UNIVERSEY (s.origin.y) + sy;
		while (ORIG_UNIVERSE_TO_DISPY(cursorLoc.y) == pt.y)
			cursorLoc.y -= sy;

		if (cursorLoc.y < 0)
			cursorLoc.y = 0;
		else if (cursorLoc.y > MAX_Y_UNIVERSE)
			cursorLoc.y = MAX_Y_UNIVERSE;

		s.origin.y = ORIG_UNIVERSE_TO_DISPY (cursorLoc.y);
	}

	
	if (s.origin.x < 0 || s.origin.y < 0
			|| s.origin.x >= ORIG_SIS_SCREEN_WIDTH 
			|| s.origin.y >= ORIG_SIS_SCREEN_HEIGHT)
	{
		mapOrigin = cursorLoc;
		DrawStarMap (0, NULL);
		
		s.origin.x = ORIG_UNIVERSE_TO_DISPX (cursorLoc.x);
		s.origin.y = ORIG_UNIVERSE_TO_DISPY (cursorLoc.y);
	}
	else
	{
		BatchGraphics ();
		EraseCursor (RES_SCALE (pt.x), RES_SCALE (pt.y));
		DrawCursor (RES_SCALE (s.origin.x), RES_SCALE (s.origin.y));
		flashCurrentLocation (NULL, TRUE);
		UnbatchGraphics ();
	}
}

#define CURSOR_INFO_BUFSIZE 256

int starIndex (POINT starPt)
{
	COUNT i;

	for (i = 0; i <= NUM_SOLAR_SYSTEMS; i++)
	{
		if (star_array[i].star_pt.x == starPt.x
			&& star_array[i].star_pt.y == starPt.y)
			break;
	}
	return i;
}

static void
UpdateCursorInfo (UNICODE *prevbuf)
{
	UNICODE buf[CURSOR_INFO_BUFSIZE] = "";
	POINT pt;
	STAR_DESC *SDPtr;
	STAR_DESC *BestSDPtr;

	if (which_starmap == NORMAL_STARMAP)
	{	// "(Star Search: F6 | Toggle Maps: F7)"
		utf8StringCopy (buf, sizeof (buf), GAME_STRING (
				FEEDBACK_STRING_BASE + 2
				+ (is3DO (optWhichFonts) || IS_PAD)));
	}
	else
		utf8StringCopy (buf, sizeof (buf),
				GAME_STRING (FEEDBACK_STRING_BASE + 3 + which_starmap));

	pt.x = UNIVERSE_TO_DISPX (cursorLoc.x);
	pt.y = UNIVERSE_TO_DISPY (cursorLoc.y);

	SDPtr = BestSDPtr = 0;
	while ((SDPtr = FindStar (SDPtr, &cursorLoc, 75, 75)))
	{
		if (UNIVERSE_TO_DISPX (SDPtr->star_pt.x) == pt.x
				&& UNIVERSE_TO_DISPY (SDPtr->star_pt.y) == pt.y
				&& (BestSDPtr == 0
				|| STAR_TYPE (SDPtr->Type) >= STAR_TYPE (BestSDPtr->Type)))
			BestSDPtr = SDPtr;
	}

	if (BestSDPtr)
	{
		// JMS: For masking the names of QS portals not yet entered.
		BYTE whichPortal = BestSDPtr->Postfix - 133;
		
		// A star is near the cursor:
		// Snap cursor onto star
		cursorLoc = BestSDPtr->star_pt;
		
		if (GET_GAME_STATE(ARILOU_SPACE_SIDE) >= 2
				&& !(GET_GAME_STATE (KNOW_QS_PORTAL) & (1 << whichPortal)))
		{
			utf8StringCopy (buf, sizeof (buf),
					GAME_STRING (STAR_STRING_BASE + 132));
		}
		else
			GetClusterName (BestSDPtr, buf);
	}
	else
	{	// No star found. Reset the coordinates to the cursor's location
		// Kruzen: bucket to avoid cursor misplacement due to
		// asymmetric DISP_TO_UNIVERSE functions
		COORD bucket;

		bucket = DISP_TO_UNIVERSEX (pt.x);
		if (bucket < 0)
			cursorLoc.x = 0;
		else if (bucket > MAX_X_UNIVERSE)
			cursorLoc.x = MAX_X_UNIVERSE;
		bucket = DISP_TO_UNIVERSEY (pt.y);
		if (bucket < 0)
			cursorLoc.y = 0;
		else if (bucket > MAX_Y_UNIVERSE)
			cursorLoc.y = MAX_Y_UNIVERSE;
	}

	if (GET_GAME_STATE (ARILOU_SPACE))
	{
		POINT ari_pt;

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			ari_pt.x = ARILOU_SPACE_X;
			ari_pt.y = ARILOU_SPACE_Y;
		}
		else
		{
			ari_pt.x = QUASI_SPACE_X;
			ari_pt.y = QUASI_SPACE_Y;
		}

		if (UNIVERSE_TO_DISPX (ari_pt.x) == pt.x
				&& UNIVERSE_TO_DISPY (ari_pt.y) == pt.y)
		{
			cursorLoc = ari_pt;
			utf8StringCopy (buf, sizeof (buf),
					GAME_STRING (STAR_STRING_BASE + 132));
		}
	}

	DrawHyperCoords (cursorLoc);
	if (strcmp (buf, prevbuf) != 0)
	{
		strcpy (prevbuf, buf);
		
		// Cursor is on top of a star. Display its name.
		if (BestSDPtr)
		{
			if (optShowVisitedStars
					&& isStarMarked (starIndex (BestSDPtr->star_pt),
						"VISITED"))
			{
				UNICODE visBuf[CURSOR_INFO_BUFSIZE] = "";

				utf8StringCopy (visBuf, sizeof (visBuf), buf);
				snprintf (buf, sizeof buf, "%c %s %c", '(', visBuf, ')');
			}

			DrawSISMessage (buf);
		}
		// Cursor is elsewhere.
		else
		{
			// In HS, display default star search button name.
			if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
			{
				CONTEXT OldContext;
				OldContext = SetContext (OffScreenContext);
				
				if (which_starmap == WAR_ERA_STARMAP)
					SetContextForeGroundColor (
							BUILD_COLOR (
								MAKE_RGB15 (0x18, 0x00, 0x00), 0x00));
				else
					SetContextForeGroundColor (
							BUILD_COLOR (
								MAKE_RGB15 (0x0E, 0xA7, 0xD9), 0x00));
						
				DrawSISMessageEx (buf, -1, -1, DSME_MYCOLOR);
				SetContext (OldContext);
			}
			else
			{	// In QS, don't display star search button - the search is
				// unusable.
				strcpy (buf, GAME_STRING (NAVIGATION_STRING_BASE + 1));
				DrawSISMessage (buf);
			}
		}
	}
}

static unsigned int
FuelRequired (void)
{
	return FuelRequiredTo (cursorLoc);
}

static void
UpdateFuelRequirement (void)
{
	UNICODE buf[80];
	COUNT fuel_required = FuelRequired();

	sprintf (buf, "%s %u.%u",
			GAME_STRING (NAVIGATION_STRING_BASE + 4),
			fuel_required / FUEL_TANK_SCALE,
			(fuel_required % FUEL_TANK_SCALE) / 10);

	DrawStatusMessage (buf);
}

#define STAR_SEARCH_BUFSIZE 256

typedef struct starsearch_state
{
	// TODO: pMS field is probably not needed anymore
	MENU_STATE *pMS;
	UNICODE Text[STAR_SEARCH_BUFSIZE];
	UNICODE LastText[STAR_SEARCH_BUFSIZE];
	DWORD LastChangeTime;
	int FirstIndex;
	int CurIndex;
	int LastIndex;
	BOOLEAN SingleClust;
	BOOLEAN SingleMatch;
	UNICODE Buffer[STAR_SEARCH_BUFSIZE];
	const UNICODE *Prefix;
	const UNICODE *Cluster;
	int PrefixLen;
	int ClusterLen;
	int ClusterPos;
	int SortedStars[NUM_SOLAR_SYSTEMS];
} STAR_SEARCH_STATE;

static int
compStarName (const void *ptr1, const void *ptr2)
{
	int index1;
	int index2;

	index1 = *(const int *) ptr1;
	index2 = *(const int *) ptr2;
	if (star_array[index1].Postfix != star_array[index2].Postfix)
	{
		return utf8StringCompare (GAME_STRING (star_array[index1].Postfix),
				GAME_STRING (star_array[index2].Postfix));
	}

	if (star_array[index1].Prefix < star_array[index2].Prefix)
		return -1;
	
	if (star_array[index1].Prefix > star_array[index2].Prefix)
		return 1;

	return 0;
}

static void
SortStarsOnName (STAR_SEARCH_STATE *pSS)
{
	int i;
	int *sorted = pSS->SortedStars;

	for (i = 0; i < NUM_SOLAR_SYSTEMS; i++)
		sorted[i] = i;

	qsort (sorted, NUM_SOLAR_SYSTEMS, sizeof (int), compStarName);
}

static void
SplitStarName (STAR_SEARCH_STATE *pSS)
{
	UNICODE *buf = pSS->Buffer;
	UNICODE *next;
	UNICODE *sep = NULL;

	pSS->Prefix = 0;
	pSS->PrefixLen = 0;
	pSS->Cluster = 0;
	pSS->ClusterLen = 0;
	pSS->ClusterPos = 0;

	// skip leading space
	for (next = buf; *next != '\0' &&
			getCharFromString ((const UNICODE **)&next)
				== (isPC (optWhichFonts) ? UNICHAR_SPACE : UNICHAR_TAB);
			buf = next)
		;
	if (*buf == '\0')
	{	// no text
		return;
	}

	pSS->Prefix = buf;

	// See if player gave a prefix
	for (buf = next; *next != '\0' &&
			getCharFromString ((const UNICODE **)&next)
				!= (isPC (optWhichFonts) ? UNICHAR_SPACE : UNICHAR_TAB);
			buf = next)
		;
	if (*buf != '\0')
	{	// found possibly separating ' '
		sep = buf;
		// skip separating space
		for (buf = next; *next != '\0' &&
				getCharFromString ((const UNICODE **)&next)
					== (isPC (optWhichFonts) ? UNICHAR_SPACE
						: UNICHAR_TAB);
				buf = next)
			;
	}

	if (*buf == '\0')
	{	// reached the end -- cluster only
		pSS->Cluster = pSS->Prefix;
		pSS->ClusterLen = utf8StringCount (pSS->Cluster);
		pSS->ClusterPos = utf8StringCountN (pSS->Buffer, pSS->Cluster);
		pSS->Prefix = 0;
		return;
	}

	// consider the rest cluster name (whatever there is)
	pSS->Cluster = buf;
	pSS->ClusterLen = utf8StringCount (pSS->Cluster);
	pSS->ClusterPos = utf8StringCountN (pSS->Buffer, pSS->Cluster);
	*sep = '\0'; // split
	pSS->PrefixLen = utf8StringCount (pSS->Prefix);
}

static inline int
SkipStarCluster (int *sortedStars, int istar)
{
	int Postfix = star_array[sortedStars[istar]].Postfix;

	for (++istar; istar < NUM_SOLAR_SYSTEMS &&
			star_array[sortedStars[istar]].Postfix == Postfix;
			++istar)
		;
	return istar;
}

static int
FindNextStarIndex (STAR_SEARCH_STATE *pSS, int from, BOOLEAN WithinClust)
{
	int i;

	if (!pSS->Cluster)
		return -1; // nothing to search for

	for (i = from; i < NUM_SOLAR_SYSTEMS; ++i)
	{
		STAR_DESC *SDPtr = &star_array[pSS->SortedStars[i]];
		UNICODE FullName[STAR_SEARCH_BUFSIZE];
		UNICODE *ClusterName = GAME_STRING (SDPtr->Postfix);
		const UNICODE *sptr;
		const UNICODE *dptr;
		int dlen;
		int c;
		
		dlen = utf8StringCount (ClusterName);
		if (pSS->ClusterLen > dlen)
		{	// no match, skip the rest of cluster
			i = SkipStarCluster (pSS->SortedStars, i) - 1;
			continue;
		}

		for (c = 0, sptr = pSS->Cluster, dptr = ClusterName;
				c < pSS->ClusterLen; ++c)
		{
			UniChar sc = getCharFromString (&sptr);
			UniChar dc = getCharFromString (&dptr);

			if (UniChar_toUpper (sc) != UniChar_toUpper (dc))
				break;
		}

		if (c < pSS->ClusterLen)
		{	// no match here, skip the rest of cluster
			i = SkipStarCluster (pSS->SortedStars, i) - 1;
			continue;
		}

		if (pSS->Prefix && !SDPtr->Prefix)
			// we were given a prefix but found a singular star;
			// that is a no match
			continue;

		if (WithinClust)
			// searching within clusters; any prefix is a match
			break;

		if (!pSS->Prefix)
		{	// searching for cluster name only
			// return only the first stars in a cluster
			if (i == 0 || SDPtr->Postfix !=
					star_array[pSS->SortedStars[i - 1]].Postfix)
			{	// found one
				break;
			}
			else
			{	// another star in the same cluster, skip cluster
				i = SkipStarCluster (pSS->SortedStars, i) - 1;
				continue;
			}
		}

		// check prefix
		GetClusterName (SDPtr, FullName);
		dlen = utf8StringCount (FullName);
		if (pSS->PrefixLen > dlen)
			continue;

		for (c = 0, sptr = pSS->Prefix, dptr = FullName;
				c < pSS->PrefixLen; ++c)
		{
			UniChar sc = getCharFromString (&sptr);
			UniChar dc = getCharFromString (&dptr);

			if (UniChar_toUpper (sc) != UniChar_toUpper (dc))
				break;
		}

		if (c >= pSS->PrefixLen)
			break; // found one
	}

	return (i < NUM_SOLAR_SYSTEMS) ? i : -1;
}

static void
DrawMatchedStarName (TEXTENTRY_STATE *pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;
	UNICODE buf[STAR_SEARCH_BUFSIZE] = "";
	SIZE ExPos = 0;
	SIZE CurPos = -1;
	STAR_DESC *SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
	COUNT flags;

	if (pSS->SingleClust || pSS->SingleMatch)
	{	// draw full star name
		GetClusterName (SDPtr, buf);
		ExPos = -1;
		flags = DSME_SETFR;
	}
	else
	{	// draw substring match
		UNICODE *pstr = buf;

		strcpy (pstr, pSS->Text);
		ExPos = pSS->ClusterPos;
		pstr = skipUTF8Chars (pstr, pSS->ClusterPos);

		strcpy (pstr, GAME_STRING (SDPtr->Postfix));
		ExPos += pSS->ClusterLen;
		CurPos = pTES->CursorPos;

		flags = DSME_CLEARFR;
		if (pTES->JoystickMode)
			flags |= DSME_BLOCKCUR;
	}
	
	DrawSISMessageEx (buf, CurPos, ExPos, flags);
	DrawHyperCoords (cursorLoc);
}

static void
MatchNextStar (STAR_SEARCH_STATE *pSS, BOOLEAN Reset)
{
	if (Reset)
		pSS->FirstIndex = -1; // reset cache
	
	if (pSS->FirstIndex < 0)
	{	// first time after changes
		pSS->CurIndex = -1;
		pSS->LastIndex = -1;
		pSS->SingleClust = FALSE;
		pSS->SingleMatch = FALSE;
		strcpy (pSS->Buffer, pSS->Text);
		SplitStarName (pSS);
	}

	pSS->CurIndex = FindNextStarIndex (pSS, pSS->CurIndex + 1,
			pSS->SingleClust);
	if (pSS->FirstIndex < 0) // first search
		pSS->FirstIndex = pSS->CurIndex;
	
	if (pSS->CurIndex >= 0)
	{	// remember as last (searching forward-only)
		pSS->LastIndex = pSS->CurIndex;
	}
	else
	{	// wrap around
		pSS->CurIndex = pSS->FirstIndex;

		if (pSS->FirstIndex == pSS->LastIndex && pSS->FirstIndex != -1)
		{
			if (!pSS->Prefix)
			{	// only one cluster matching
				pSS->SingleClust = TRUE;
			}
			else
			{	// exact match
				pSS->SingleMatch = TRUE;
			}
		}
	}
}

static BOOLEAN
OnStarNameChange (TEXTENTRY_STATE *pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;
	COUNT flags;
	BOOLEAN ret = TRUE;

	if (strcmp (pSS->Text, pSS->LastText) != 0)
	{	// string changed
		pSS->LastChangeTime = GetTimeCounter ();
		strcpy (pSS->LastText, pSS->Text);
		
		// reset the search
		MatchNextStar (pSS, TRUE);
	}

	if (pSS->CurIndex < 0)
	{	// nothing found
		if (pSS->Text[0] == '\0')
			flags = DSME_SETFR;
		else
			flags = DSME_CLEARFR;
		if (pTES->JoystickMode)
			flags |= DSME_BLOCKCUR;

		ret = DrawSISMessageEx (pSS->Text, pTES->CursorPos, -1, flags);
	}
	else
	{
		STAR_DESC *SDPtr;

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation (0, 0, &SDPtr->star_pt);

		DrawMatchedStarName (pTES);
		UpdateFuelRequirement ();
	}

	return ret;
}

static BOOLEAN
OnStarNameFrame (TEXTENTRY_STATE *pTES)
{
	STAR_SEARCH_STATE *pSS = (STAR_SEARCH_STATE *) pTES->CbParam;

	if (PulsedInputState.menu[KEY_MENU_NEXT])
	{	// search for next match
		STAR_DESC *SDPtr;

		MatchNextStar (pSS, FALSE);

		if (pSS->CurIndex < 0)
		{	// nothing found
			if (PulsedInputState.menu[KEY_MENU_NEXT])
				PlayMenuSound (MENU_SOUND_FAILURE);
			return TRUE;
		}

		// move the cursor to the found star
		SDPtr = &star_array[pSS->SortedStars[pSS->CurIndex]];
		UpdateCursorLocation (0, 0, &SDPtr->star_pt);

		DrawMatchedStarName (pTES);
		UpdateFuelRequirement ();
	}

	flashCurrentLocation (NULL, FALSE);

	SleepThread (ONE_SECOND / 30);
	
	return TRUE;
}

BOOLEAN
coords_only (UNICODE *s)
{
	BYTE i, count = 0;
	BYTE countD = 0, countC = 0;
	BYTE j = (BYTE)strlen (s);
	//const char *pattern = "^\d*(\.\d+)?:\d*(\.\d+)?$";

	for (i = 0; i < j; i++)
	{
		if (s[i] == '.')
		{
			count++;
			countD++;
		}
		else if (s[i] == ':')
		{
			count++;
			countC++;
		}
		else if (isdigit (s[i]) == 0)
			return FALSE;
		else
			count++;
	}
	return i == j && countD <= 2 && countC == 1;
}

static BOOLEAN
DoStarSearch (MENU_STATE *pMS)
{
	TEXTENTRY_STATE tes;
	STAR_SEARCH_STATE *pss;
	BOOLEAN success;

	pss = HMalloc (sizeof (*pss));
	if (!pss)
		return FALSE;

	DrawSISMessageEx ("", 0, 0, DSME_SETFR);

	TextEntry3DO = (BOOLEAN)is3DO (optWhichFonts);

	pss->pMS = pMS;
	pss->LastChangeTime = 0;
	pss->Text[0] = '\0';
	pss->LastText[0] = '\0';
	pss->FirstIndex = -1;
	SortStarsOnName (pss);

	// text entry setup
	tes.Initialized = FALSE;
	tes.BaseStr = pss->Text;
	tes.MaxSize = sizeof (pss->Text);
	tes.CursorPos = 0;
	tes.CbParam = pss;
	tes.ChangeCallback = OnStarNameChange;
	tes.FrameCallback = OnStarNameFrame;

	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();
	success = DoTextEntry (&tes);

	if (coords_only (tes.BaseStr))
	{
		POINT coord;

		coord.x = (COORD)(atof (strtok (tes.BaseStr, ":")) * 10);
		coord.y = (COORD)(atof (strtok (NULL, ":")) * 10);

		if (coord.x > MAX_X_UNIVERSE || coord.y > MAX_Y_UNIVERSE
			|| coord.x < 0 || coord.y < 0)
			success = FALSE;
		else
			UpdateCursorLocation (0, 0, &coord);

		success = TRUE;
	}
	
	DrawSISMessageEx (pss->Text, -1, -1, DSME_CLEARFR);

	HFree (pss);

	TextEntry3DO = FALSE;

	return success;
}

void
DoBubbleWarp (BOOLEAN UseFuel)
{
	PlayMenuSound (MENU_SOUND_BUBBLEWARP);

	if (UseFuel)
		DeltaSISGauges (0, -(int)FuelRequired (), 0);

	if (LOBYTE (GLOBAL (CurrentActivity)) == IN_INTERPLANETARY)
	{
		// We're in a solar system; exit it.
		GLOBAL (CurrentActivity) |= END_INTERPLANETARY;
		// Set a hook to move to the new location:
		debugHook = doInstantMove;
	}
	else
	{	// Move to the new location immediately.
		doInstantMove ();
	}
}

#define NUM_PORTALS 15
#define PORTAL_FUEL_COST DIF_CASE(10, 5, 20)

static void
AdvancedAutoPilot (void)
{
	POINT current_position;
	POINT destination = GLOBAL (autopilot);
	POINT portal_pt[NUM_PORTALS] = QUASISPACE_PORTALS_HYPERSPACE_ENDPOINTS;
	POINT portal_coordinates;
	double distance, fuel_no_portal, fuel_with_portal;
	double minimum = 0.0;
	BYTE i;
	BYTE index = 0;
	UWORD KnownQSPortals = GET_GAME_STATE (KNOW_QS_PORTAL);

	current_position.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
	current_position.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));

	if (pointsEqual (current_position, destination))
		return;

	for (i = 0; i < NUM_PORTALS; i++)
	{
		distance = ptDistance (destination, portal_pt[i]);

		if (!DIF_EASY && !(KnownQSPortals & (1 << i)))
			distance = MAX_X_UNIVERSE * MAX_Y_UNIVERSE;

		if (i == 0 || distance < minimum)
		{
			minimum = distance;
			index = i + 1;
		}
	}

	portal_coordinates = star_array[NUM_SOLAR_SYSTEMS + index].star_pt;

	fuel_no_portal = ptDistance (current_position, destination) / 100;
	fuel_with_portal = minimum / 100 + PORTAL_FUEL_COST;

	if (fuel_no_portal < fuel_with_portal)
		return;

	SaveAdvancedAutoPilot (destination);
	SaveAdvancedQuasiPilot (portal_coordinates);

	if (playerInSolarSystem ())
		GLOBAL (autopilot) = current_position;

	if (inHyperSpace ())
		InvokeSpawner ();
}

static BOOLEAN
DoMoveCursor (MENU_STATE *pMS)
{
#define MIN_ACCEL_DELAY (ONE_SECOND / 60)
#define MAX_ACCEL_DELAY (ONE_SECOND / 8)
#define STEP_ACCEL_DELAY (ONE_SECOND / 120)
	static UNICODE last_buf[CURSOR_INFO_BUFSIZE];
	DWORD TimeIn = GetTimeCounter ();
	static COUNT moveRepeats;
	BOOLEAN isMove = FALSE;

	if (!pMS->Initialized)
	{
		POINT universe;

		pMS->Initialized = TRUE;
		pMS->InputFunc = DoMoveCursor;

		if (!inHQSpace ())
			universe = CurStarDescPtr->star_pt;
		else
		{
			universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
			universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
		}
		flashCurrentLocation (&universe, FALSE);

		last_buf[0] = '\0';
		UpdateCursorInfo (last_buf);
		UpdateFuelRequirement ();

		return TRUE;
	}
	else if (PulsedInputState.menu[KEY_MENU_CANCEL])
	{
		FlushInput ();

		if ((optBubbleWarp && !optInfiniteFuel && !inQuasiSpace ())
				&& GLOBAL (autopilot.x) != ~0 && GLOBAL (autopilot.y) != ~0
				&& GLOBAL_SIS (FuelOnBoard) >= FuelRequired ())
		{
			DoBubbleWarp (TRUE);
		}

		if (!inQuasiSpace ()
				&& ValidPoint (GLOBAL (autopilot)))
		{
			if (optSmartAutoPilot)
			{
				SaveLastLoc (
						MAKE_POINT (
							LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x)),
							LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y))));
			}

			if (optAdvancedAutoPilot
					&& GET_GAME_STATE (PORTAL_SPAWNER_ON_SHIP))
			{
				AdvancedAutoPilot ();
			}
		}

		return FALSE;
	}
	else if (PulsedInputState.menu[KEY_MENU_SELECT])
	{
		/*printf ("Fuel Available: %d | Fuel Requirement: %d\n",
				GLOBAL_SIS (FuelOnBoard), FuelRequired());*/

		FlushInput ();

		if (optBubbleWarp && (optInfiniteFuel || inQuasiSpace ()))
		{
			GLOBAL (autopilot) = cursorLoc;
			DoBubbleWarp (FALSE);
			return FALSE;
		}
		else
		{
			if (GLOBAL (autopilot.x) == cursorLoc.x
					&& GLOBAL (autopilot.y) == cursorLoc.y)
				GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;
			else
			{
				GLOBAL (autopilot) = cursorLoc;
			}
			DrawStarMap (0, NULL);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SEARCH]
			|| (PulsedInputState.menu[KEY_MENU_ZOOM_IN]
				&& PulsedInputState.menu[KEY_MENU_ZOOM_OUT]))
	{
		FlushInput ();

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{	// HyperSpace search
			POINT oldpt = cursorLoc;

			if (!DoStarSearch (pMS))
			{	// search failed or canceled - return cursor
				UpdateCursorLocation (0, 0, &oldpt);
			}
			FlushCursorRect ();
			// make sure cmp fails
			strcpy (last_buf, "  <random garbage>  ");
			UpdateCursorInfo (last_buf);
			UpdateFuelRequirement ();

			SetMenuRepeatDelay (MIN_ACCEL_DELAY, MAX_ACCEL_DELAY,
					STEP_ACCEL_DELAY, TRUE);
			SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
		}
		else
		{	// no search in QuasiSpace
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_TOGGLEMAP])
	{
		FlushInput ();

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			BYTE NewState;
			NewState = which_starmap;

			if (NewState == RAINBOW_MAP)
				NewState = NORMAL_STARMAP;
			else
				++NewState;

			if (NewState != which_starmap)
				which_starmap = NewState;

			PlayMenuSound (MENU_SOUND_MOVE);

			DrawStarMap (0, NULL);
			last_buf[0] = '\0';
			UpdateCursorInfo (last_buf);
		}
		else
		{	// no alternate maps in QuasiSpace
			PlayMenuSound (MENU_SOUND_FAILURE);
		}
	}
	else if (PulsedInputState.menu[KEY_MENU_SPECIAL])
	{
		FlushInput ();

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			setStarMarked (starIndex (cursorLoc), "PLYR_MARKER");

			DrawStarMap (0, NULL);
		}
		else
			PlayMenuSound (MENU_SOUND_FAILURE);
	}
	else if (PulsedInputState.menu[KEY_MENU_DELETE])
	{
		FlushInput ();

		if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		{
			COUNT i;

			for (i = 0; i <= NUM_SOLAR_SYSTEMS; i++)
			{
				if (isStarMarked (i, "PLYR_MARKER"))
				{
					setStarMarked (i, "PLYR_MARKER");
					DrawStarMap (0, NULL);
					SleepThread (ONE_SECOND / 8);
				}
			}
		}
		else
			PlayMenuSound (MENU_SOUND_FAILURE);
	}
	else
	{
		SBYTE sx, sy;
		SIZE ZoomIn, ZoomOut;

		ZoomIn = ZoomOut = 0;
		if (PulsedInputState.menu[KEY_MENU_ZOOM_IN])
			ZoomIn = 1;
		else if (PulsedInputState.menu[KEY_MENU_ZOOM_OUT])
			ZoomOut = 1;

		ZoomStarMap (ZoomIn - ZoomOut);

		sx = sy = 0;
		if (PulsedInputState.menu[KEY_MENU_LEFT])    sx = -1;
		if (PulsedInputState.menu[KEY_MENU_RIGHT])   sx =  1;
		if (PulsedInputState.menu[KEY_MENU_UP])      sy = -1;
		if (PulsedInputState.menu[KEY_MENU_DOWN])    sy =  1;

		// Double the cursor speed when the "Next" key is held down
		if (DirKeysPress () && CurrentInputState.menu[KEY_MENU_NEXT])
		{
			sx *= 2;
			sy *= 2;
		}

		if (sx != 0 || sy != 0)
		{
			UpdateCursorLocation (sx, sy, NULL);
			UpdateCursorInfo (last_buf);
			UpdateFuelRequirement ();
			isMove = TRUE;
		}

		SleepThreadUntil (TimeIn + MIN_ACCEL_DELAY);
	}

	if (isMove)
		++moveRepeats;
	else
		moveRepeats = 0;

	flashCurrentLocation (NULL, FALSE);

	return !(GLOBAL (CurrentActivity) & CHECK_ABORT);
}

static void
RepairMap (COUNT update_race, RECT *pLastRect, RECT *pNextRect)
{
	RECT r;

	/* make a rect big enough for text */
	r.extent.width = 50;
	r.corner.x = (pNextRect->corner.x + (pNextRect->extent.width >> 1))
			- (r.extent.width >> 1);
	if (r.corner.x < 0)
		r.corner.x = 0;
	else if (r.corner.x + r.extent.width >= SIS_SCREEN_WIDTH)
		r.corner.x = SIS_SCREEN_WIDTH - r.extent.width;
	r.extent.height = 9;
	r.corner.y = (pNextRect->corner.y + (pNextRect->extent.height >> 1))
			- r.extent.height;
	if (r.corner.y < 0)
		r.corner.y = 0;
	else if (r.corner.y + r.extent.height >= SIS_SCREEN_HEIGHT)
		r.corner.y = SIS_SCREEN_HEIGHT - r.extent.height;
	BoxUnion (pLastRect, &r, &r);
	BoxUnion (pNextRect, &r, &r);
	*pLastRect = *pNextRect;

	if (r.corner.x < 0)
	{
		r.extent.width += r.corner.x;
		r.corner.x = 0;
	}
	if (r.corner.x + r.extent.width > SIS_SCREEN_WIDTH)
		r.extent.width = SIS_SCREEN_WIDTH - r.corner.x;
	if (r.corner.y < 0)
	{
		r.extent.height += r.corner.y;
		r.corner.y = 0;
	}
	if (r.corner.y + r.extent.height > SIS_SCREEN_HEIGHT)
		r.extent.height = SIS_SCREEN_HEIGHT - r.corner.y;

	r.extent.height += r.corner.y & 1;
	r.corner.y &= ~1;
	
	DrawStarMap (update_race, &r);
}

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

static void
UpdateMap (void)
{
	BYTE ButtonState, VisibleChange;
	BOOLEAN MapDrawn, Interrupted;
	COUNT index;
	HFLEETINFO hStarShip, hNextShip;
	COUNT GrowthFactor;

	FlushInput ();
	ButtonState = 1; /* assume a button down */

	MapDrawn = Interrupted = FALSE;
	for (index = 1,
			hStarShip = GetHeadLink (&GLOBAL (avail_race_q));
			hStarShip; ++index, hStarShip = hNextShip)
	{
		FLEET_INFO *FleetPtr;

		FleetPtr = LockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
		hNextShip = _GetSuccLink (FleetPtr);

		if (ButtonState)
		{
			if (!AnyButtonPress (TRUE))
				ButtonState = 0;
		}
		else if ((Interrupted = (BOOLEAN)(
				Interrupted || AnyButtonPress (TRUE)
				)))
			MapDrawn = TRUE;

		if (FleetPtr->known_strength)
		{
			SIZE dx, dy, delta;
			RECT r, last_r, temp_r0, temp_r1;
			COUNT str;

			dx = FleetPtr->loc.x - FleetPtr->known_loc.x;
			dy = FleetPtr->loc.y - FleetPtr->known_loc.y;
			if (dx || dy)
			{
				SIZE xincr, yincr,
						xerror, yerror,
						cycle;

				if (dx >= 0)
					xincr = 1;
				else
				{
					xincr = -1;
					dx = -dx;
				}
				dx <<= 1;

				if (dy >= 0)
					yincr = 1;
				else
				{
					yincr = -1;
					dy = -dy;
				}
				dy <<= 1;

				if (dx >= dy)
					cycle = dx;
				else
					cycle = dy;
				delta = xerror = yerror = cycle >> 1;

				if (!MapDrawn)
				{
					DrawStarMap ((COUNT)~0, NULL);
					MapDrawn = TRUE;
				}

				GetSphereRect (FleetPtr, &temp_r0, &last_r);
				last_r.extent.width += RES_SCALE (1) + IF_HD (1);// dot in HD is 5px wide for AA to work
				last_r.extent.height += RES_SCALE (1) + IF_HD (1);// so we have to add extra 1
				VisibleChange = FALSE;
				do
				{
					do
					{
						if ((xerror -= dx) <= 0)
						{
							FleetPtr->known_loc.x += xincr;
							xerror += cycle;
						}
						if ((yerror -= dy) <= 0)
						{
							FleetPtr->known_loc.y += yincr;
							yerror += cycle;
						}
						GetSphereRect (FleetPtr, &temp_r1, &r);
					} while (delta--
							&& ((delta & 0x1F)
							|| (temp_r0.corner.x == temp_r1.corner.x
							&& temp_r0.corner.y == temp_r1.corner.y)));

					if (ButtonState)
					{
						if (!AnyButtonPress (TRUE))
							ButtonState = 0;
					}
					else if ((Interrupted = (BOOLEAN)(
								Interrupted || AnyButtonPress (TRUE)
								)))
					{
						MapDrawn = TRUE;
						goto DoneSphereMove;
					}

					r.extent.width += RES_SCALE (1) + IF_HD (1);
					r.extent.height += RES_SCALE (1) + IF_HD (1);
					if (temp_r0.corner.x != temp_r1.corner.x
							|| temp_r0.corner.y != temp_r1.corner.y)
					{// Ignore name stacking during movement
						VisibleChange = TRUE;
						RepairMap (index | IGNORE_MOVING_SOI, &last_r, &r);
						SleepThread (ONE_SECOND / 24);
					}
				} while (delta >= 0);
				if (VisibleChange)
					RepairMap ((COUNT)~0, &last_r, &r);

DoneSphereMove:
				FleetPtr->known_loc = FleetPtr->loc;
			}

			delta = FleetPtr->actual_strength - FleetPtr->known_strength;
			if (delta)
			{
				if (!MapDrawn)
				{
					DrawStarMap ((COUNT)~0, NULL);
					MapDrawn = TRUE;
				}

				if (delta > 0)
					dx = 1;
				else
				{
					delta = -delta;
					dx = -1;
				}
				--delta;

				GetSphereRect (FleetPtr, &temp_r0, &last_r);
				last_r.extent.width += RES_SCALE (1);
				last_r.extent.height += RES_SCALE (1);
				// Kruzen: Font size to clean up double space because of text stacking now
				last_r.extent.height = MAX (last_r.extent.height, RES_SCALE (14));
				VisibleChange = FALSE;

				/*printf("%s: %d\n", raceName (index),
						FleetPtr->actual_strength);*/

				str = FleetPtr->known_strength;

				GrowthFactor = delta > 0 ? FleetPtr->actual_strength
						: FleetPtr->known_strength;

				do
				{
					do
					{
						FleetPtr->known_strength += dx;
						GetSphereRect (FleetPtr, &temp_r1, &r);
					} while (delta--
							&& ((delta & 0xF)
							|| temp_r0.extent.height
								== temp_r1.extent.height));

					if (ButtonState)
					{
						if (!AnyButtonPress (TRUE))
							ButtonState = 0;
					}
					else if ((Interrupted = (BOOLEAN)(
								Interrupted || AnyButtonPress (TRUE)
								)))
					{
						MapDrawn = TRUE;
						goto DoneSphereGrowth;
					}
					r.extent.width += RES_SCALE (1);
					r.extent.height += RES_SCALE (1);
					if ((temp_r0.extent.height != temp_r1.extent.height) &&
							!(str > 0 && FleetPtr->known_strength == 0))
					{// Update race SOI size IF the race didn't die out
						VisibleChange = TRUE;
						RepairMap (index, &last_r, &r);
						SleepThread (
								ONE_SECOND / (12 + GrowthFactor / 44));
					}
					else if (str > 0 && FleetPtr->known_strength == 0)
					{// Flash dying race name
						VisibleChange = TRUE;
						RepairMap (index | PRE_DEATH_SOI, &last_r, &r);
						SleepThread (ONE_SECOND / 12);
						RepairMap (index | DEATH_SOI, &last_r, &r);
						SleepThread (ONE_SECOND / 12);
					}
				} while (delta >= 0);
				if (VisibleChange
						|| temp_r0.extent.width != temp_r1.extent.width)
					RepairMap ((COUNT)~0, &last_r, &r);

DoneSphereGrowth:
				FleetPtr->known_strength = FleetPtr->actual_strength;
			}
		}

		UnlockFleetInfo (&GLOBAL (avail_race_q), hStarShip);
	}
}

static void
DrawStarmapHelper (void)
{

	CONTEXT OldContext;
	STAMP s;
	TEXT t;
	RECT r;
	SIZE leading;
	int frame_index;
#define GAMEPAD(a) (RES_SCALE (optControllerType ? (a) : 0))

	OldContext = SetContext (StatusContext);

	BatchGraphics ();

	DrawFlagStatDisplay (GAME_STRING (STATUS_STRING_BASE + 7));

	SetContextFont (TinyFont);
	GetContextFontLeading (&leading);

	r.corner.x = RES_SCALE (4);
	r.corner.y = RES_SCALE (34);

	frame_index = !optControllerType ? 0 : 5 * optControllerType;

	// :Maps
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index);
	s.origin = r.corner;
	DrawStamp (&s);

	SetContextForeGroundColor (MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (12) - GAMEPAD (2);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 12);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Add->O
	s.origin.y += RES_SCALE (10);
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 1);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (16) - GAMEPAD (6);
	t.baseline.y = s.origin.y + leading;
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 13);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	s.origin.x = t.baseline.x + RES_SCALE (28);
	s.origin.y += RES_SCALE (4);
	s.frame = SetAbsFrameIndex (MiscDataFrame, 108);
	DrawStamp (&s);

	// Cursor Speed
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE (6);
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 2);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (18) - GAMEPAD (8);
	t.baseline.y = s.origin.y + leading;
	t.pStr = ":";
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	s.origin.x = t.baseline.x + RES_SCALE (7);
	s.origin.y += RES_SCALE (4);
	s.frame = SetAbsFrameIndex (MiscDataFrame, 48);
	DrawStamp (&s);

	t.align = ALIGN_LEFT;
	t.baseline.x += RES_SCALE (13);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 14);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Search
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 3);
	s.origin.x = r.corner.x;
	s.origin.y += RES_SCALE (7) - GAMEPAD (1);
	DrawStamp (&s);

	t.baseline.x = s.origin.x + RES_SCALE (12) + GAMEPAD (11);
	t.baseline.y = s.origin.y + leading + RES_SCALE (5) - GAMEPAD (5);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 15);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// :Zoom
	s.frame = SetAbsFrameIndex (SubmenuFrame, frame_index + 4);
	s.origin.y += RES_SCALE (21) - GAMEPAD (11);
	DrawStamp (&s);

	t.baseline.x = s.origin.x + RES_SCALE (30) - GAMEPAD (18);
	t.baseline.y = s.origin.y + leading + RES_SCALE (5);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 16);
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// Separator
	r.corner.y = RES_SCALE (118);
	r.extent.width = FIELD_WIDTH - RES_SCALE (3);
	r.extent.height = RES_SCALE (1);
	SetContextForeGroundColor (CARGO_SELECTED_BACK_COLOR);
	DrawFilledRectangle (&r);

	// FUEL:
	SetContextForeGroundColor (MODULE_NAME_COLOR);
	t.align = ALIGN_LEFT;
	t.baseline.x = r.corner.x + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = GAME_STRING (STATUS_STRING_BASE + 8); // FUEL:
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	// Current amount of fuel
	SetContextForeGroundColor (MODULE_PRICE_COLOR);
	t.align = ALIGN_RIGHT;
	t.baseline.x = r.extent.width + RES_SCALE (2);
	t.baseline.y = r.corner.y + leading + RES_SCALE (1);
	t.pStr = WholeFuelValue ();
	t.CharCount = (COUNT)~0;
	font_DrawText (&t);

	UnbatchGraphics ();

	SetContext (OldContext);
}

BOOLEAN
StarMap (void)
{
	MENU_STATE MenuState;
	POINT universe;
	//FRAME OldFrame;
	RECT clip_r;
	CONTEXT OldContext;

	memset (&MenuState, 0, sizeof (MenuState));

	which_starmap = NORMAL_STARMAP;

	zoomLevel = 0;
	mapOrigin.x = MAX_X_UNIVERSE >> 1;
	mapOrigin.y = MAX_Y_UNIVERSE >> 1;
	StarMapFrame = SetAbsFrameIndex (MiscDataFrame, 48);

	if (!inHQSpace ())
		universe = CurStarDescPtr->star_pt;
	else
	{
		universe.x = LOGX_TO_UNIVERSE (GLOBAL_SIS (log_x));
		universe.y = LOGY_TO_UNIVERSE (GLOBAL_SIS (log_y));
	}

	cursorLoc = GLOBAL (autopilot);
	if (cursorLoc.x == ~0 && cursorLoc.y == ~0)
		cursorLoc = universe;

	if (optWhichMenu == OPT_PC)
	{
		if (playerInPlanetOrbit ())
			DrawMenuStateStrings (PM_ALT_SCAN, 1);
		else
			DrawMenuStateStrings (PM_ALT_STARMAP, 0);
	}

	MenuState.InputFunc = DoMoveCursor;
	MenuState.Initialized = FALSE;

	transition_pending = TRUE;
	if (GET_GAME_STATE (ARILOU_SPACE_SIDE) <= 1)
		UpdateMap ();
	else
	{	// This zooms the Quasi map in by 2 if within the local Quasi star
		// cluster.
		if ((universe.x <= ARILOU_HOME_X && universe.y <= ARILOU_HOME_Y)
			&& (universe.x >= 4480 && universe.y >= 4580))
			zoomLevel = 2;
	}

	if (optSubmenu)
		DrawStarmapHelper ();

	DrawStarMap (0, (RECT*)-1);
	transition_pending = FALSE;
	
	BatchGraphics ();
	OldContext = SetContext (SpaceContext);
	GetContextClipRect (&clip_r);
	SetContext (OldContext);
	LoadIntoExtraScreen (&clip_r);
	DrawCursor (UNIVERSE_TO_DISPX (cursorLoc.x),
			UNIVERSE_TO_DISPY (cursorLoc.y));
	UnbatchGraphics ();

	SetMenuSounds (MENU_SOUND_NONE, MENU_SOUND_NONE);
	SetMenuRepeatDelay (MIN_ACCEL_DELAY, MAX_ACCEL_DELAY, STEP_ACCEL_DELAY,
			TRUE);
#if defined(ANDROID) || defined(__ANDROID__)
	TFB_SetOnScreenKeyboard_Starmap();
	DoInput(&MenuState, FALSE);
	TFB_SetOnScreenKeyboard_Menu();
#else // ANDROID
	DoInput(&MenuState, FALSE);
#endif
	SetMenuSounds (MENU_SOUND_ARROWS, MENU_SOUND_SELECT);
	SetDefaultMenuRepeatDelay ();

	DrawHyperCoords (universe);
	if (GLOBAL(autopilot.x) != ~0 && GLOBAL(autopilot.y) != ~0)
		DrawAutoPilotMessage (FALSE);
	else
		DrawSISMessage (NULL);
	DrawStatusMessage (NULL);
	
	if (optSubmenu)
		DeltaSISGauges (UNDEFINED_DELTA, UNDEFINED_DELTA, UNDEFINED_DELTA);

	/*if (GLOBAL (autopilot.x) == universe.x
			&& GLOBAL (autopilot.y) == universe.y)
		GLOBAL (autopilot.x) = GLOBAL (autopilot.y) = ~0;*/

	return (GLOBAL (autopilot.x) != ~0
			&& GLOBAL (autopilot.y) != ~0);
}