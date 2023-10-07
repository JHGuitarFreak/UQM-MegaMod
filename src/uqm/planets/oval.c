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

#include "../units.h"
#include "libs/gfxlib.h"
#include "libs/graphics/context.h"
#include "libs/graphics/drawable.h"
#include "../nameref.h"
#include "../igfxres.h"
#include "../init.h"
#include <math.h>

#include "planets.h"
#include "uqm/setup.h"

#define NUM_QUADS 4
#define OFFSCREEN_PRIM NUM_PRIMS

static BOOLEAN
DPointOnScreen (DPOINT *p)
{
	return (p->x >= 0 && p->x <= SIS_SCREEN_WIDTH
			&& p->y >= 0 && p->y <= SIS_SCREEN_HEIGHT);
}

static BYTE
CheckOvalCollision (DPOINT *ch_one, DPOINT *ch_two)
{
#define FIRST_QUAD (1 << 0)
#define SECOND_QUAD (1 << 1)
#define THIRD_QUAD (1 << 2)
#define FOURTH_QUAD (1 << 3)
	DPOINT mp;
	BRESENHAM_LINE ClipLine;
	DRECT ClipRect;
	BYTE quad_visible = 0;

	ClipRect.corner.x = ClipRect.corner.y = 0;
	ClipRect.extent.width = SIS_SCREEN_WIDTH;
	ClipRect.extent.height = SIS_SCREEN_HEIGHT;

	mp.x = (ch_one->x + ch_two->x) >> 1;
	mp.y = (ch_one->y + ch_two->y) >> 1;
	ClipRect.corner.x = ClipRect.corner.y = 0;

	if (ch_one->y >= 0 && ch_one->y < ClipRect.extent.height
		&& ch_two->x >= 0 && ch_two->x < ClipRect.extent.width)
		quad_visible |= FIRST_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = ch_one->y;
		ClipLine.second.x = ch_two->x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= FIRST_QUAD;
	}

	if (ch_one->y >= 0 && ch_one->y < ClipRect.extent.height
		&& ch_one->x >= 0 && ch_one->x < ClipRect.extent.width)
		quad_visible |= SECOND_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = ch_one->y;
		ClipLine.second.x = ch_one->x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= SECOND_QUAD;
	}

	if (ch_two->y >= 0 && ch_two->y < ClipRect.extent.height
		&& ch_one->x >= 0 && ch_one->x < ClipRect.extent.width)
		quad_visible |= THIRD_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = ch_two->y;
		ClipLine.second.x = ch_one->x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= THIRD_QUAD;
	}

	if (ch_two->y >= 0 && ch_two->y < ClipRect.extent.height
		&& ch_two->x >= 0 && ch_two->x < ClipRect.extent.width)
		quad_visible |= FOURTH_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = ch_two->y;
		ClipLine.second.x = ch_two->x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= FOURTH_QUAD;
	}
	return quad_visible;
}

void
DrawOval (DRECT *pRect, BYTE num_off_pixels, BOOLEAN scaled)
{
	COUNT off;
	COORD x, y;
	SIZE A, B;
	FWORD Asquared, TwoAsquared,
			Bsquared, TwoBsquared;
	FWORD d, dx, dy;
	BYTE quad_visible;
	PRIMITIVE prim[NUM_QUADS];
	COUNT StartPrim;
	DPOINT check[2];
	BOOLEAN use_pointprim = (!scaled && num_off_pixels <= 1);
	
	check[0].x = pRect->corner.x;
	check[0].y = pRect->corner.y;
	check[1].x = check[0].x + pRect->extent.width - 1;
	check[1].y = check[0].y + pRect->extent.height - 1;
	// Kruzen: adapted from the original code. If rect is defined incorrectly - make it flat as a pancake and draw a line.
	if (check[1].x <= check[0].x
		|| check[1].y <= check[0].y)
	{
		LINE corners;
		BYTE i;

		if (check[1].x < check[0].x)
			check[1].x = check[0].x;
		if (check[1].y < check[0].y)
			check[1].y = check[0].y;

		for (i = 0; i < 2; i++)
		{
			if (check[i].x > 32767) { check[i].x = 32767; }
			if (check[i].x < -32768) { check[i].x = -32768; }
			if (check[i].y > 32767) { check[i].y = 32767; }
			if (check[i].y < -32768) { check[i].y = -32768; }
		}

		corners.first.x = (COORD)check[0].x;
		corners.first.y = (COORD)check[0].y;

		corners.second.x = (COORD)check[1].x;
		corners.second.y = (COORD)check[1].y;

		DrawLine (&corners, 1);
		return;
	}

	if (DPointOnScreen (&check[0]) && DPointOnScreen(&check[1]))
		quad_visible = 15;// Kruzen: entire circle is on screen
	else
		quad_visible = CheckOvalCollision (&check[0], &check[1]);

	if (!quad_visible)
		return;

	StartPrim = END_OF_LIST;
	for (x = 0; x < NUM_QUADS; ++x)
	{
		if (quad_visible & (1 << x))
		{
			SetPrimNextLink (&prim[x], StartPrim);
			SetPrimType (&prim[x], use_pointprim ? POINT_PRIM : STAMPFILL_PRIM); // Orbit dots
			prim[x].Object.Stamp.frame =
						DecFrameIndex (stars_in_space);
			SetPrimColor (&prim[x], _get_context_fg_color ());

			StartPrim = x;
		}
		else// Kruzen: just to be sure so DrawBatch() skip it
			SetPrimType (&prim[x], OFFSCREEN_PRIM);
	}

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	Asquared = ((FWORD)A * A) << 1;
	Bsquared = ((FWORD)B * B) << 1;
	do
	{
		Asquared >>= 1;
		Bsquared >>= 1;
		TwoAsquared = Asquared << 1;
		dy = TwoAsquared * B;
	} while (dy / B != TwoAsquared);
	TwoBsquared = Bsquared << 1;

	dx = 0;
	d = Bsquared - (dy >> 1) + (Asquared >> 2);

	off = 0;
	A += pRect->corner.x;
	B += pRect->corner.y;

	// Kruzen: a check for prim being on screen can be added, but checking quads is enough for now
	if (use_pointprim)
	{
		while (dx < dy)
		{
			if (off-- == 0)
			{
				prim[0].Object.Point.x = prim[3].Object.Point.x = A + x;
				prim[0].Object.Point.y = prim[1].Object.Point.y = B - y;
				prim[1].Object.Point.x = prim[2].Object.Point.x = A - x;
				prim[2].Object.Point.y = prim[3].Object.Point.y = B + y;

				DrawBatch (prim, StartPrim, 0);
				off = num_off_pixels;
			}

			if (d > 0)
			{
				--y;
				dy -= TwoAsquared;
				d -= dy;
			}

			++x;
			dx += TwoBsquared;
			d += Bsquared + dx;
		}
	}
	else
	{
		while (dx < dy)
		{
			if (off-- == 0)
			{// Kruzen: Use correct struct for these. I have no idea how that worked before
				prim[0].Object.Stamp.origin.x = prim[3].Object.Stamp.origin.x = A + x;
				prim[0].Object.Stamp.origin.y = prim[1].Object.Stamp.origin.y = B - y;
				prim[1].Object.Stamp.origin.x = prim[2].Object.Stamp.origin.x = A - x;
				prim[2].Object.Stamp.origin.y = prim[3].Object.Stamp.origin.y = B + y;

				DrawBatch (prim, StartPrim, 0);
				off = num_off_pixels;
			}

			if (d > 0)
			{
				--y;
				dy -= TwoAsquared;
				d -= dy;
			}

			++x;
			dx += TwoBsquared;
			d += Bsquared + dx;
		}
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	if (use_pointprim)
	{
		while (y >= 0)
		{
			if (off-- == 0)
			{
				prim[0].Object.Point.x = prim[3].Object.Point.x = A + x;
				prim[0].Object.Point.y = prim[1].Object.Point.y = B - y;
				prim[1].Object.Point.x = prim[2].Object.Point.x = A - x;
				prim[2].Object.Point.y = prim[3].Object.Point.y = B + y;

				DrawBatch (prim, StartPrim, 0);
				off = num_off_pixels;
			}

			if (d < 0)
			{
				++x;
				dx += TwoBsquared;
				d += dx;
			}

			--y;
			dy -= TwoAsquared;
			d += Asquared - dy;
		}
	}
	else
	{
		while (y >= 0)
		{
			if (off-- == 0)
			{
				prim[0].Object.Stamp.origin.x = prim[3].Object.Stamp.origin.x = A + x;
				prim[0].Object.Stamp.origin.y = prim[1].Object.Stamp.origin.y = B - y;
				prim[1].Object.Stamp.origin.x = prim[2].Object.Stamp.origin.x = A - x;
				prim[2].Object.Stamp.origin.y = prim[3].Object.Stamp.origin.y = B + y;

				DrawBatch (prim, StartPrim, 0);
				off = num_off_pixels;
			}

			if (d < 0)
			{
				++x;
				dx += TwoBsquared;
				d += dx;
			}

			--y;
			dy -= TwoAsquared;
			d += Asquared - dy;
		}
	}
}

void
DrawFilledOval (DRECT *pRect)
{// Kruzen: originally there was standard rect, but drawing fuel circle in HD on max zoom causes overflow (Width ~65k)
 // So unless we want to expand standard rect to support 4 bytes per dimension - use this
	COORD x, y;
	SIZE A, B;
	FWORD Asquared, TwoAsquared,
			Bsquared, TwoBsquared;
	FWORD d, dx, dy;
	PRIMITIVE prim[NUM_QUADS >> 1];
	COUNT StartPrim;
	POINT first, second;
	DPOINT check[2];
	BYTE i;
	COUNT lines_r = 0;

	check[0].x = pRect->corner.x;
	check[0].y = pRect->corner.y;
	check[1].x = check[0].x + pRect->extent.width - 1;
	check[1].y = check[0].y + pRect->extent.height - 1;
	// Kruzen: adapted from the original code. If rect is defined incorrectly - make it flat as a pancake and draw a line.
	if (check[1].x <= check[0].x
			|| check[1].y <= check[0].y)
	{
		LINE corners;

		if (check[1].x < check[0].x)
			check[1].x = check[0].x;
		if (check[1].y < check[0].y)
			check[1].y = check[0].y;

		for (i = 0; i < 2; i++)
		{
			if (check[i].x > 32767) { check[i].x = 32767; }
			if (check[i].x < -32768) { check[i].x = -32768; }
			if (check[i].y > 32767) { check[i].y = 32767; }
			if (check[i].y < -32768) { check[i].y = -32768; }
		}

		corners.first.x = (COORD)check[0].x;
		corners.first.y = (COORD)check[0].y;

		corners.second.x = (COORD)check[1].x;
		corners.second.y = (COORD)check[1].y;

		DrawLine (&corners, 1);
		return;
	}

	StartPrim = END_OF_LIST;
	for (x = 0; x < (NUM_QUADS >> 1); ++x)
	{
		SetPrimNextLink (&prim[x], StartPrim);
		SetPrimType (&prim[x], LINE_PRIM);
		SetPrimColor (&prim[x], _get_context_fg_color ());

		StartPrim = x;
	}

	A = pRect->extent.width >> 1;
	B = pRect->extent.height >> 1;

	x = 0;
	y = B;

	Asquared = ((FWORD)A * A) << 1;
	Bsquared = ((FWORD)B * B) << 1;
	do
	{
		Asquared >>= 1;
		Bsquared >>= 1;
		TwoAsquared = Asquared << 1;
		dy = TwoAsquared * B;
	} while (dy / B != TwoAsquared);
	TwoBsquared = Bsquared << 1;

	dx = 0;
	d = Bsquared - (dy >> 1) + (Asquared >> 2);

	A += pRect->corner.x;
	B += pRect->corner.y;
	while (dx < dy)
	{
		if (d > 0)
		{// Kruzen: originally PrimType was FILLRECT with height of 1
		 // Changed to line and added a skip if offscreen
			lines_r = 0;

			first.x = max (A - x, 0);
			second.x = min (first.x + (x << 1) + 1, SIS_SCREEN_WIDTH);
			first.y = second.y = B - y;
			if (((B - y) >= 0 && (B - y) <= SIS_SCREEN_HEIGHT))
			{
				SetPrimType (&prim[0], LINE_PRIM);
				prim[0].Object.Line.first = first;
				prim[0].Object.Line.second = second;
				lines_r++;
			}
			else
				SetPrimType (&prim[0], OFFSCREEN_PRIM);

			first.y = second.y = B + y;
			if (((B + y) >= 0 && (B + y) <= SIS_SCREEN_HEIGHT))
			{
				SetPrimType (&prim[1], LINE_PRIM);
				prim[1].Object.Line.first = first;
				prim[1].Object.Line.second = second;
				lines_r++;
			}
			else
				SetPrimType (&prim[1], OFFSCREEN_PRIM);

			if (lines_r > 0)
				DrawBatch (prim, StartPrim, 0);

			--y;
			dy -= TwoAsquared;
			d -= dy;
		}

		++x;
		dx += TwoBsquared;
		d += Bsquared + dx;
	}

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

	while (y >= 0)
	{// Kruzen: originally PrimType was FILLRECT with height of 1
	 // Changed to line and added a skip if offscreen
		lines_r = 0;
		first.x = max (A - x, 0);
		second.x = min (first.x + (x << 1) + 1, SIS_SCREEN_WIDTH);
		first.y = second.y = B - y;
		if (((B - y) >= 0 && (B - y) <= SIS_SCREEN_HEIGHT))
		{
			SetPrimType (&prim[0], LINE_PRIM);
			prim[0].Object.Line.first = first;
			prim[0].Object.Line.second = second;
			lines_r++;
		}
		else
			SetPrimType (&prim[0], OFFSCREEN_PRIM);

		first.y = second.y = B + y;
		if (((B + y) >= 0 && (B + y) <= SIS_SCREEN_HEIGHT))
		{
			SetPrimType (&prim[1], LINE_PRIM);
			prim[1].Object.Line.first = first;
			prim[1].Object.Line.second = second;
			lines_r++;
		}
		else
			SetPrimType (&prim[1], OFFSCREEN_PRIM);

		if (lines_r > 0)
			DrawBatch (prim, StartPrim, 0);

		if (d < 0)
		{
			++x;
			dx += TwoBsquared;
			d += dx;
		}

		--y;
		dy -= TwoAsquared;
		d += Asquared - dy;
	}
}

void
DrawEllipseQuadrants (int cx, int cy, int x, int y, int shear, int filled)
{
	POINT p;
	LINE l;

	if (filled && y != 0)
	{
		l.first.x = l.second.x = cx - x;
		l.first.y = cy - shear - y;
		l.second.y = cy - shear + y;
		DrawLine (&l, 1);

		if (x != 0)
		{
			l.first.x = l.second.x = cx + x;
			l.first.y = cy + shear - y;
			l.second.y = cy + shear + y;
			DrawLine (&l, 1);
		}
	}
	else
	{
		p.x = cx - x;
		p.y = cy - shear - y;
		DrawPoint (&p);
		p.y = cy - shear + y;
		DrawPoint (&p);

		if (x != 0)
		{
			p.x = cx + x;
			p.y = cy + shear - y;
			DrawPoint (&p);
			p.y = cy + shear + y;
			DrawPoint (&p);
		}
	}
}

void
DrawEllipse (int cx, int cy, int rx, int ry, int shear, int filled, int dotted)
{
	// adapted from https://zingl.github.io/Bresenham.pdf section 2.1
	int x = rx;
	int y = 0;
	int s = shear;
	int d = 0;
	const int sRound = (shear < 0) ? -(rx / 2) : (rx / 2);
	FWORD dex = 2 * (long)ry * ry;
	FWORD ex = (long)ry * ry - (long)rx * dex;
	FWORD dey = 2 * (long)rx * rx;
	FWORD ey = (long)rx * rx;
	FWORD e = ex + ey;
	FWORD e2;

	if (rx < 0)
		rx = 0;
	if (ry < 0)
		ry = 0;
	if (!rx || !ry)
	{
		LINE l;
		l.first.x = cx - rx;
		l.first.y = cy - ry - shear;
		l.second.x = cx + rx;
		l.second.y = cy + ry + shear;
		DrawLine (&l, 1);
		return;
	}

	do
	{
		if (!filled && !d--)
		{
			d = dotted;
			s = (int)(((FWORD)x * shear + sRound) / rx);
			DrawEllipseQuadrants (cx, cy, x, y, -s, 0);
		}

		e2 = e * 2;
		if (e2 >= ex)
		{
			if (filled)
			{
				s = (int)(((FWORD)x * shear + sRound) / rx);
				DrawEllipseQuadrants (cx, cy, x, y, -s, 1);
			}
			x--;
			ex += dex;
			e += ex;
		}
		if (e2 <= ey)
		{
			y++;
			ey += dey;
			e += ey;
		}
	} while (x > 0);

	// when x=0, y must be ry and s must be 0
	DrawEllipseQuadrants (cx, cy, 0, ry, 0, filled);
}

void
DrawRotatedEllipse (int cx, int cy, int rx, int ry, int angle_deg, int filled, int dotted)
{
	// based on https://zingl.github.io/Bresenham.pdf section 4.3
	double rx2 = (double)rx * rx;
	double ry2 = (double)ry * ry;
	double theta = (angle_deg % 90) * M_PI / 180.0;
	double st = sin (theta);
	double ct = cos (theta);
	double xd2 = rx2 * ct * ct + ry2 * st * st;
	double xd = sqrt (xd2);
	int shear = (int)(((rx2 - ry2) * st * ct) / xd + 0.5);

	rx = (int)(xd + 0.5);
	ry = (int)(sqrt ((rx2 * ry2) / xd2) + 0.5);
	DrawEllipse (cx, cy, rx, ry, shear, filled, dotted);
}
