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

#ifndef BBOX_H_INCL__
#define BBOX_H_INCL__

#include "libs/gfxlib.h"
#include "libs/graphics/tfb_draw.h"

/* Bounding Box operations.  These operations are NOT synchronized.
 * However, they should only be accessed by TFB_FlushGraphics and
 * TFB_SwapBuffers, or the routines that they exclusively call -- all
 * of which are only callable by the thread that is permitted to touch
 * the screen.  No explicit locks should therefore be required. */

typedef struct {
	int valid;   // If zero, the next point registered becomes the region
	RECT region; // The actual modified rectangle
	RECT clip;   // Points outside of this rectangle are pushed to
		     // the closest border point
} TFB_BoundingBox;

extern TFB_BoundingBox TFB_BBox;

void TFB_BBox_RegisterPoint (int x, int y);
void TFB_BBox_RegisterRect (const RECT *r);
void TFB_BBox_RegisterCanvas (TFB_Canvas c, int x, int y);

void TFB_BBox_Init (int width, int height);
void TFB_BBox_Reset (void);
void TFB_BBox_SetClipRect (const RECT *r);

#endif /* BBOX_H_INCL__ */
