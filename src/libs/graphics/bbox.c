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

#include "port.h"
#include "libs/graphics/bbox.h"

TFB_BoundingBox TFB_BBox;
int maxWidth;
int maxHeight;

void
TFB_BBox_Init (int width, int height)
{
	maxWidth = width;
	maxHeight = height;
	TFB_BBox.clip.extent.width = width;
	TFB_BBox.clip.extent.height = height;
}

void
TFB_BBox_Reset (void)
{
	TFB_BBox.valid = 0;
}

void
TFB_BBox_SetClipRect (const RECT *r)
{
	if (!r)
	{	/* No clipping -- full rect */
		TFB_BBox.clip.corner.x = 0;
		TFB_BBox.clip.corner.y = 0;
		TFB_BBox.clip.extent.width = maxWidth;
		TFB_BBox.clip.extent.height = maxHeight;
		return;
	}

	TFB_BBox.clip = *r;

	/* Make sure the cliprect is sane */
	if (TFB_BBox.clip.corner.x < 0)
		TFB_BBox.clip.corner.x = 0;

	if (TFB_BBox.clip.corner.y < 0)
		TFB_BBox.clip.corner.y = 0;

	if (TFB_BBox.clip.corner.x + TFB_BBox.clip.extent.width > maxWidth)
		TFB_BBox.clip.extent.width = maxWidth - TFB_BBox.clip.corner.x;
	
	if (TFB_BBox.clip.corner.y + TFB_BBox.clip.extent.height > maxHeight)
		TFB_BBox.clip.extent.height = maxHeight - TFB_BBox.clip.corner.y;
}

void
TFB_BBox_RegisterPoint (int x, int y) 
{
	int x1 = TFB_BBox.clip.corner.x; 
	int y1 = TFB_BBox.clip.corner.y;
	int x2 = TFB_BBox.clip.corner.x + TFB_BBox.clip.extent.width - 1;
	int y2 = TFB_BBox.clip.corner.y + TFB_BBox.clip.extent.height - 1;

	/* Constrain coordinates */
	if (x < x1) x = x1;
	if (x >= x2) x = x2;
	if (y < y1) y = y1;
	if (y >= y2) y = y2;

	/* Is this the first point?  If so, set a pixel-region and return. */
	if (!TFB_BBox.valid)
	{
		TFB_BBox.valid = 1;
		TFB_BBox.region.corner.x = x;
		TFB_BBox.region.corner.y = y;
		TFB_BBox.region.extent.width = 1;
		TFB_BBox.region.extent.height = 1;
		return;
	}

	/* Otherwise expand the rectangle if necessary. */
	x1 = TFB_BBox.region.corner.x; 
	y1 = TFB_BBox.region.corner.y;
	x2 = TFB_BBox.region.corner.x + TFB_BBox.region.extent.width - 1;
	y2 = TFB_BBox.region.corner.y + TFB_BBox.region.extent.height - 1;

	if (x < x1) {
		TFB_BBox.region.corner.x = x;
		TFB_BBox.region.extent.width += x1 - x;
	}
	if (y < y1) {
		TFB_BBox.region.corner.y = y;
		TFB_BBox.region.extent.height += y1 - y;
	}
	if (x > x2) {
		TFB_BBox.region.extent.width += x - x2;
	}
	if (y > y2) {
		TFB_BBox.region.extent.height += y - y2;
	}
}

void
TFB_BBox_RegisterRect (const RECT *r)
{
	/* RECT will still register as a corner point of the cliprect even
	 * if it does not intersect with the cliprect at all. This is not
	 * a problem, as more is not less. */
	TFB_BBox_RegisterPoint (r->corner.x, r->corner.y);
	TFB_BBox_RegisterPoint (r->corner.x + r->extent.width - 1,
			r->corner.y + r->extent.height - 1);
}

void
TFB_BBox_RegisterCanvas (TFB_Canvas c, int x, int y)
{
	RECT r;
	r.corner.x = x;
	r.corner.y = y;
	TFB_DrawCanvas_GetExtent (c, &r.extent);
	TFB_BBox_RegisterRect (&r);
}
