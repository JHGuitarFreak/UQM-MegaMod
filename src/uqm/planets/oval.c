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

#include "planets.h"

#define NUM_QUADS 4

void
DrawOval (RECT *pRect, BYTE num_off_pixels)
{
#define FIRST_QUAD (1 << 0)
#define SECOND_QUAD (1 << 1)
#define THIRD_QUAD (1 << 2)
#define FOURTH_QUAD (1 << 3)
	COUNT off;
	COORD x, y;
	SIZE A, B;
	FWORD Asquared, TwoAsquared,
			Bsquared, TwoBsquared;
	FWORD d, dx, dy;
	BYTE quad_visible;
	LINE corners;
	POINT mp;
	PRIMITIVE prim[NUM_QUADS];
	COUNT StartPrim;
	RECT ClipRect;
	BRESENHAM_LINE ClipLine;

	ClipRect.corner.x = ClipRect.corner.y = 0;

	corners.first.x = pRect->corner.x - ClipRect.corner.x;
	corners.first.y = pRect->corner.y - ClipRect.corner.y;
	corners.second.x = corners.first.x + pRect->extent.width - 1;
	corners.second.y = corners.first.y + pRect->extent.height - 1;
	if (corners.second.x <= corners.first.x
			|| corners.second.y <= corners.first.y)
	{
		if (corners.second.x < corners.first.x)
			corners.second.x = corners.first.x;
		if (corners.second.y < corners.first.y)
			corners.second.y = corners.first.y;

		DrawLine (&corners);
		return;
	}

	ClipRect.extent.width = SIS_SCREEN_WIDTH;
	ClipRect.extent.height = SIS_SCREEN_HEIGHT;

	quad_visible = 0;
	mp.x = (corners.first.x + corners.second.x) >> 1;
	mp.y = (corners.first.y + corners.second.y) >> 1;
	ClipRect.corner.x = ClipRect.corner.y = 0;

	if (corners.first.y >= 0 && corners.first.y < ClipRect.extent.height
			&& corners.second.x >= 0 && corners.second.x < ClipRect.extent.width)
		quad_visible |= FIRST_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = corners.first.y;
		ClipLine.second.x = corners.second.x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= FIRST_QUAD;
	}

	if (corners.first.y >= 0 && corners.first.y < ClipRect.extent.height
			&& corners.first.x >= 0 && corners.first.x < ClipRect.extent.width)
		quad_visible |= SECOND_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = corners.first.y;
		ClipLine.second.x = corners.first.x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= SECOND_QUAD;
	}

	if (corners.second.y >= 0 && corners.second.y < ClipRect.extent.height
			&& corners.first.x >= 0 && corners.first.x < ClipRect.extent.width)
		quad_visible |= THIRD_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = corners.second.y;
		ClipLine.second.x = corners.first.x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= THIRD_QUAD;
	}

	if (corners.second.y >= 0 && corners.second.y < ClipRect.extent.height
			&& corners.second.x >= 0 && corners.second.x < ClipRect.extent.width)
		quad_visible |= FOURTH_QUAD;
	else
	{
		ClipLine.first.x = mp.x;
		ClipLine.first.y = corners.second.y;
		ClipLine.second.x = corners.second.x;
		ClipLine.second.y = mp.y;
		if (_clip_line (&ClipRect, &ClipLine))
			quad_visible |= FOURTH_QUAD;
	}

	if (!quad_visible)
		return;

	StartPrim = END_OF_LIST;
	for (x = 0; x < NUM_QUADS; ++x)
	{
		if (quad_visible & (1 << x))
		{
			SetPrimNextLink (&prim[x], StartPrim);
			SetPrimType (&prim[x], POINT_PRIM); // Orbit dots
			SetPrimColor (&prim[x], _get_context_fg_color ());

			StartPrim = x;
		}
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

	d += ((((Asquared - Bsquared) * 3) >> 1) - (dx + dy)) >> 1;

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

void
DrawFilledOval (RECT *pRect)
{
	COORD x, y;
	SIZE A, B;
	FWORD Asquared, TwoAsquared,
			Bsquared, TwoBsquared;
	FWORD d, dx, dy;
	LINE corners;
	PRIMITIVE prim[NUM_QUADS >> 1];
	COUNT StartPrim;

	corners.first.x = pRect->corner.x;
	corners.first.y = pRect->corner.y;
	corners.second.x = corners.first.x + pRect->extent.width - 1;
	corners.second.y = corners.first.y + pRect->extent.height - 1;
	if (corners.second.x <= corners.first.x
			|| corners.second.y <= corners.first.y)
	{
		if (corners.second.x < corners.first.x)
			corners.second.x = corners.first.x;
		if (corners.second.y < corners.first.y)
			corners.second.y = corners.first.y;

		DrawLine (&corners);
		return;
	}

	StartPrim = END_OF_LIST;
	for (x = 0; x < (NUM_QUADS >> 1); ++x)
	{
		SetPrimNextLink (&prim[x], StartPrim);
		SetPrimType (&prim[x], RECTFILL_PRIM);
		SetPrimColor (&prim[x], _get_context_fg_color ());
		prim[x].Object.Rect.extent.height = 1;

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
		{
			prim[0].Object.Rect.corner.x =
					prim[1].Object.Rect.corner.x = A - x;
			prim[0].Object.Rect.extent.width =
					prim[1].Object.Rect.extent.width = (x << 1) + 1;
			prim[0].Object.Rect.corner.y = B - y;
			prim[1].Object.Rect.corner.y = B + y;

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
	{
		prim[0].Object.Rect.corner.x =
				prim[1].Object.Rect.corner.x = A - x;
		prim[0].Object.Rect.extent.width =
				prim[1].Object.Rect.extent.width = (x << 1) + 1;
		prim[0].Object.Rect.corner.y = B - y;
		prim[1].Object.Rect.corner.y = B + y;

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


