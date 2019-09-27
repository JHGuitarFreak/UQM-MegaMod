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

INTERSECT_CODE
_clip_line (const RECT *pClipRect, BRESENHAM_LINE *pLine)
{
	COORD p;
	COORD x0, y0, xmin, ymin, xmax, ymax;
	SIZE abs_delta_x, abs_delta_y;
	INTERSECT_CODE intersect_code;

	xmin = pClipRect->corner.x;
	ymin = pClipRect->corner.y;
	xmax = pClipRect->corner.x + pClipRect->extent.width - 1;
	ymax = pClipRect->corner.y + pClipRect->extent.height - 1;
	if (pLine->first.x <= pLine->second.x)
		pLine->end_points_exchanged = FALSE;
	else
	{
		p = pLine->first.x;
		pLine->first.x = pLine->second.x;
		pLine->second.x = p;

		p = pLine->first.y;
		pLine->first.y = pLine->second.y;
		pLine->second.y = p;

		pLine->end_points_exchanged = TRUE;
	}

	if (pLine->first.x > xmax || pLine->second.x < xmin ||
			(pLine->first.y > ymax && pLine->second.y > ymax) ||
			(pLine->first.y < ymin && pLine->second.y < ymin))
		return ((INTERSECT_CODE)0);

	intersect_code = INTERSECT_NOCLIP;
	x0 = y0 = 0;
	abs_delta_x = (pLine->second.x - pLine->first.x) << 1;
	abs_delta_y = (pLine->second.y - pLine->first.y) << 1;
	pLine->abs_delta_x = abs_delta_x;
	pLine->abs_delta_y = abs_delta_y;
	if (abs_delta_y == 0)
	{
		if (pLine->first.x < xmin)
		{
			pLine->first.x = xmin;
			intersect_code |= INTERSECT_LEFT;
		}
		if (pLine->second.x > xmax)
		{
			pLine->second.x = xmax;
			intersect_code |= INTERSECT_RIGHT;
		}
	}
	else if (abs_delta_x == 0)
	{
		if (abs_delta_y < 0)
		{
			p = pLine->first.y;
			pLine->first.y = pLine->second.y;
			pLine->second.y = p;

			pLine->abs_delta_y =
					abs_delta_y = -abs_delta_y;
		}

		if (pLine->first.y < ymin)
		{
			pLine->first.y = ymin;
			intersect_code |= INTERSECT_TOP;
		}
		if (pLine->second.y > ymax)
		{
			pLine->second.y = ymax;
			intersect_code |= INTERSECT_BOTTOM;
		}
	}
	else
	{
		COORD x1, y1;

		p = pLine->first.x;
		x1 = pLine->second.x - p;
		xmin = xmin - p;
		xmax = xmax - p;

		p = pLine->first.y;
		if (abs_delta_y > 0)
		{
			y1 = pLine->second.y - p;
			ymin = ymin - p;
			ymax = ymax - p;
		}
		else
		{
			y1 = p - pLine->second.y;
			ymin = p - ymin;
			ymax = p - ymax;

			p = ymin;
			ymin = ymax;
			ymax = p;
			abs_delta_y = -abs_delta_y;
		}

		if (abs_delta_x > abs_delta_y)
		{
			SIZE half_dx;

			half_dx = abs_delta_x >> 1;
			if (x0 < xmin)
			{
				if ((y0 = (COORD)(((long)abs_delta_y *
						(x0 = xmin) + half_dx) / abs_delta_x)) > ymax)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_LEFT;
			}
			if (x1 > xmax)
			{
				if ((y1 = (COORD)(((long)abs_delta_y *
						(x1 = xmax) + half_dx) / abs_delta_x)) < ymin)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_RIGHT;
			}
			if (y0 < ymin)
			{
				if ((x0 = (COORD)(((long)abs_delta_x *
						(y0 = ymin) - half_dx + (abs_delta_y - 1)) /
						abs_delta_y)) > xmax)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_TOP;
				intersect_code &= ~INTERSECT_LEFT;
			}
			if (y1 > ymax)
			{
				if ((x1 = (COORD)(((long)abs_delta_x *
						((y1 = ymax) + 1) - half_dx + (abs_delta_y - 1)) /
						abs_delta_y) - 1) < xmin)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_BOTTOM;
				intersect_code &= ~INTERSECT_RIGHT;
			}
		}
		else
		{
			SIZE half_dy;

			half_dy = abs_delta_y >> 1;
			if (y0 < ymin)
			{
				if ((x0 = (COORD)(((long)abs_delta_x *
						(y0 = ymin) + half_dy) / abs_delta_y)) > xmax)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_TOP;
			}
			if (y1 > ymax)
			{
				if ((x1 = (COORD)(((long)abs_delta_x *
						(y1 = ymax) + half_dy) / abs_delta_y)) < xmin)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_BOTTOM;
			}
			if (x0 < xmin)
			{
				if ((y0 = (COORD)(((long)abs_delta_y *
						(x0 = xmin) - half_dy + (abs_delta_x - 1)) /
						abs_delta_x)) > ymax)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_LEFT;
				intersect_code &= ~INTERSECT_TOP;
			}
			if (x1 > xmax)
			{
				if ((y1 = (COORD)(((long)abs_delta_y *
						((x1 = xmax) + 1) - half_dy + (abs_delta_x - 1)) /
						abs_delta_x) - 1) < ymin)
					return ((INTERSECT_CODE)0);
				intersect_code |= INTERSECT_RIGHT;
				intersect_code &= ~INTERSECT_BOTTOM;
			}
		}

		pLine->second.x = pLine->first.x + x1;
		pLine->first.x += x0;
		if (pLine->abs_delta_y > 0)
		{
			pLine->second.y = pLine->first.y + y1;
			pLine->first.y += y0;
		}
		else
		{
			INTERSECT_CODE y_code;

			pLine->second.y = pLine->first.y - y1;
			pLine->first.y -= y0;

			y_code = (INTERSECT_CODE)(intersect_code
					& (INTERSECT_TOP | INTERSECT_BOTTOM));
			if (y_code && y_code != (INTERSECT_TOP | INTERSECT_BOTTOM))
				intersect_code ^= (INTERSECT_TOP | INTERSECT_BOTTOM);
		}
	}

	if (!(intersect_code & INTERSECT_ALL_SIDES))
	{
		if (abs_delta_x > abs_delta_y)
			pLine->error_term = -(SIZE)(abs_delta_x >> 1);
		else
			pLine->error_term = -(SIZE)(abs_delta_y >> 1);
	}
	else
	{
		intersect_code &= ~INTERSECT_NOCLIP;
		if (abs_delta_x > abs_delta_y)
			pLine->error_term = (SIZE)((x0 * (long)abs_delta_y) -
					(y0 * (long)abs_delta_x)) - (abs_delta_x >> 1);
		else
			pLine->error_term = (SIZE)((y0 * (long)abs_delta_x) -
					(x0 * (long)abs_delta_y)) - (abs_delta_y >> 1);
	}

	return (pLine->intersect_code = intersect_code);
}

