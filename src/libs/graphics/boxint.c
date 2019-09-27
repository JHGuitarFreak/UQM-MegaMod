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

#undef MIN
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#undef MAX
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

INTERSECT_CODE
BoxIntersect (RECT *pr1, RECT *pr2, RECT *pinter)
{
	INTERSECT_CODE intersect_code;
	COORD x1;
	SIZE w1, w2, delta;

	intersect_code = INTERSECT_NOCLIP;

	x1 = pr1->corner.x - pr2->corner.x;

	w1 = pr1->extent.width;
	w2 = pr2->extent.width;
	if ((delta = w2 - x1) <= w1)
	{
		if (delta != w1)
		{
			w1 = delta;
			intersect_code &= ~INTERSECT_NOCLIP;
		}
		intersect_code |= INTERSECT_RIGHT;
	}
	if (x1 <= 0)
	{
		if (x1 < 0)
		{
			w1 += x1;
			x1 = 0;
			intersect_code &= ~INTERSECT_NOCLIP;
		}
		intersect_code |= INTERSECT_LEFT;
	}

	if (w1 > 0)
	{
#define h2 w2
		COORD y1;
		SIZE h1;

		y1 = pr1->corner.y - pr2->corner.y;

		h1 = pr1->extent.height;
		h2 = pr2->extent.height;
		if ((delta = h2 - y1) <= h1)
		{
			if (delta != h1)
			{
				h1 = delta;
				intersect_code &= ~INTERSECT_NOCLIP;
			}
			intersect_code |= INTERSECT_BOTTOM;
		}
		if (y1 <= 0)
		{
			if (y1 < 0)
			{
				h1 += y1;
				y1 = 0;
				intersect_code &= ~INTERSECT_NOCLIP;
			}
			intersect_code |= INTERSECT_TOP;
		}

		if (h1 > 0)
		{
			pinter->corner.x = x1 + pr2->corner.x;
			pinter->corner.y = y1 + pr2->corner.y;
			pinter->extent.width = w1;
			pinter->extent.height = h1;

			return (intersect_code);
		}
#undef h2
	}

	return ((INTERSECT_CODE)0);
}

void
BoxUnion (RECT *pr1, RECT *pr2, RECT *punion)
{
#if NEVER // Part of lower FIXME.
	COORD x2, y2, w2, h2;
#endif  // NEVER

	// Union is A AND B, put together, correct?  Returns a bigger box that
	// encompasses the two.
	punion->corner.x = MIN(pr1->corner.x, pr2->corner.x);
	punion->corner.y = MIN(pr1->corner.y, pr2->corner.y);
	
	punion->extent.width  = MAX(pr1->corner.x + pr1->extent.width,
						        pr2->corner.x + pr2->extent.width) - punion->corner.x;
	punion->extent.height = MAX(pr1->corner.y + pr1->extent.height,
						        pr2->corner.y + pr2->extent.height) - punion->corner.y;


#if NEVER // FIXME - I think this is broken, but keeping it around for reference
	      // FIXME - just in case.

#if 1 /* alter based on 0 widths */

	x2 =
		(pr1->corner.x < pr2->corner.x)? pr1->corner.x : pr2->corner.x;

	y2 =
		(pr1->corner.y < pr2->corner.y)? pr1->corner.y : pr2->corner.y;

	w2 = (
		((pr1->corner.x + pr1->extent.width) > (pr2->corner.x + pr2->extent.width))?
		(pr1->corner.x + pr1->extent.width)  : (pr2->corner.x + pr2->extent.width)
		) - punion->corner.x;

	h2 = (
		((pr1->corner.y + pr1->extent.height) > (pr2->corner.y + pr2->extent.height))?
		(pr1->corner.y + pr1->extent.height)  : (pr2->corner.y + pr2->extent.height)
		) - punion->corner.y;
#else
	SIZE delta;
	COORD x1, y1, w1, h1;

	x1 = pr1->corner.x;
	w1 = pr1->extent.width;
	x2 = pr2->corner.x;
	w2 = pr2->extent.width;
	if ((delta = x1 - x2) >= 0)
		w1 += delta;
	else
	{
		w2 -= delta;
		x2 += delta;
	}

	y1 = pr1->corner.y;
	h1 = pr1->extent.height;
	y2 = pr2->corner.y;
	h2 = pr2->extent.height;
	if ((delta = y1 - y2) >= 0)
		h1 += delta;
	else
	{
		h2 -= delta;
		y2 += delta;
	}

	if ((delta = w1 - w2) > 0)
		w2 += delta;
	if ((delta = h1 - h2) > 0)
		h2 += delta;
#endif

	punion->corner.x = x2;
	punion->corner.y = y2;
	punion->extent.width = w2;
	punion->extent.height = h2;

#endif // NEVER
}

