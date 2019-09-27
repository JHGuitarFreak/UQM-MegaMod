// Copyright Michael Martin, 2003

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

/* The original Primitive routines do various elaborate checks to
 * ensure we're within bounds for the clipping.  Since clipping is
 * handled by the underlying TFB_Canvas implementation, we need not
 * worry about this. */

#include "gfxintrn.h"
#include "gfx_common.h"
#include "tfb_draw.h"
#include "tfb_prim.h"
#include "cmap.h"
#include "libs/log.h"
#include "uqm/units.h"
#include "uqm/planets/planets.h"

void
TFB_Prim_Point (POINT *p, Color color, DrawMode mode, POINT ctxOrigin)
{
	RECT r;

	// The caller must scale the origin!
	r.corner.x = p->x + ctxOrigin.x;
	r.corner.y = p->y + ctxOrigin.y;
	r.extent.width = r.extent.height = 1;

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
		TFB_DrawScreen_Rect (&r, color, mode, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Rect (&r, color, mode, _CurFramePtr->image);
}

void
TFB_Prim_Rect (RECT *r, Color color, DrawMode mode, POINT ctxOrigin)
{
	RECT arm;
	int gscale;

	// XXX: Rect prim scaling is currently unused
	//   We scale the rect size just to be consistent with stamp prim,
	//   which does same. The caller must scale the origin!
	gscale = GetGraphicScale ();
	arm = *r;
	arm.extent.width = r->extent.width;
	arm.extent.height = 1;
	TFB_Prim_FillRect (&arm, color, mode, ctxOrigin);
	arm.extent.height = r->extent.height;
	arm.extent.width = 1;
	TFB_Prim_FillRect (&arm, color, mode, ctxOrigin);
	// rounding error correction here
	arm.corner.x += ((r->extent.width * gscale + (GSCALE_IDENTITY >> 1))
			/ GSCALE_IDENTITY) - 1;
	TFB_Prim_FillRect (&arm, color, mode, ctxOrigin);
	arm.corner.x = r->corner.x;
	arm.corner.y += ((r->extent.height * gscale + (GSCALE_IDENTITY >> 1))
			/ GSCALE_IDENTITY) - 1;
	arm.extent.width = r->extent.width;
	arm.extent.height = 1;
	TFB_Prim_FillRect (&arm, color, mode, ctxOrigin);
}

void
TFB_Prim_FillRect (RECT *r, Color color, DrawMode mode, POINT ctxOrigin)
{
	RECT rect;
	int gscale;

	rect.corner.x = r->corner.x + ctxOrigin.x;
	rect.corner.y = r->corner.y + ctxOrigin.y;
	rect.extent.width = r->extent.width;
	rect.extent.height = r->extent.height;

	// XXX: Rect prim scaling is currently unused
	//   We scale the rect size just to be consistent with stamp prim,
	//   which does same. The caller must scale the origin!
	gscale = GetGraphicScale ();
	if (gscale != GSCALE_IDENTITY)
	{	// rounding error correction here
		rect.extent.width = (rect.extent.width * gscale
				+ (GSCALE_IDENTITY >> 1)) / GSCALE_IDENTITY;
		rect.extent.height = (rect.extent.height * gscale
				+ (GSCALE_IDENTITY >> 1)) / GSCALE_IDENTITY;
	}

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
		TFB_DrawScreen_Rect (&rect, color, mode, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Rect (&rect, color, mode, _CurFramePtr->image);
}

void
TFB_Prim_Line (LINE *line, Color color, DrawMode mode, POINT ctxOrigin)
{
	int x1, y1, x2, y2;

	// The caller must scale the origins!
	x1=line->first.x + ctxOrigin.x;
	y1=line->first.y + ctxOrigin.y;
	x2=line->second.x + ctxOrigin.x;
	y2=line->second.y + ctxOrigin.y;

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
		TFB_DrawScreen_Line (x1, y1, x2, y2, color, mode, TFB_SCREEN_MAIN);
	else
		TFB_DrawImage_Line (x1, y1, x2, y2, color, mode, _CurFramePtr->image);
}

void
TFB_Prim_Stamp (STAMP *stmp, DrawMode mode, POINT ctxOrigin)
{
	int x, y;
	FRAME SrcFramePtr;
	TFB_Image *img;
	TFB_ColorMap *cmap = NULL;

	SrcFramePtr = stmp->frame;
	if (!SrcFramePtr)
	{
		log_add (log_Warning, "TFB_Prim_Stamp: Tried to draw a NULL frame"
				" (Stamp address = %p)", (void *) stmp);
		return;
	}
	img = SrcFramePtr->image;
	
	if (!img)
	{
		log_add (log_Warning, "Non-existent image to TFB_Prim_Stamp()");
		return;
	}

	LockMutex (img->mutex);

	img->NormalHs = SrcFramePtr->HotSpot;
	// We scale the image size here, but the caller must scale the origin!
	x = stmp->origin.x + ctxOrigin.x;
	y = stmp->origin.y + ctxOrigin.y;

	if (TFB_DrawCanvas_IsPaletted(img->NormalImg) && img->colormap_index != -1)
	{
		// returned cmap is addrefed, must release later
		cmap = TFB_GetColorMap (img->colormap_index);
	}

	UnlockMutex (img->mutex);

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		TFB_DrawScreen_Image (img, x, y, GetGraphicScale (),
				GetGraphicScaleMode (), cmap, mode, TFB_SCREEN_MAIN);
	}
	else
	{
		TFB_DrawImage_Image (img, x, y, GetGraphicScale (),
				GetGraphicScaleMode (), cmap, mode, _CurFramePtr->image);
	}
}

void
TFB_Prim_StampFill (STAMP *stmp, Color color, DrawMode mode, POINT ctxOrigin)
{
	int x, y;
	FRAME SrcFramePtr;
	TFB_Image *img;

	SrcFramePtr = stmp->frame;
	if (!SrcFramePtr)
	{
		log_add (log_Warning, "TFB_Prim_StampFill: Tried to draw a NULL frame"
				" (Stamp address = %p)", (void *) stmp);
		return;
	}
	img = SrcFramePtr->image;

	if (!img)
	{
		log_add (log_Warning, "Non-existent image to TFB_Prim_StampFill()");
		return;
	}

	LockMutex (img->mutex);

	img->NormalHs = SrcFramePtr->HotSpot;
	// We scale the image size here, but the caller must scale the origin!
	x = stmp->origin.x + ctxOrigin.x;
	y = stmp->origin.y + ctxOrigin.y;

	UnlockMutex (img->mutex);

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		TFB_DrawScreen_FilledImage (img, x, y, GetGraphicScale (),
				GetGraphicScaleMode (), color, mode, TFB_SCREEN_MAIN);
	}
	else
	{
		TFB_DrawImage_FilledImage (img, x, y, GetGraphicScale (),
				GetGraphicScaleMode (), color, mode, _CurFramePtr->image);
	}
}

void
TFB_Prim_FontChar (POINT charOrigin, TFB_Char *fontChar, TFB_Image *backing,
		DrawMode mode, POINT ctxOrigin)
{
	int x, y;

	// Text prim does not scale
	x = charOrigin.x + ctxOrigin.x;
	y = charOrigin.y + ctxOrigin.y;

	if (_CurFramePtr->Type == SCREEN_DRAWABLE)
	{
		TFB_DrawScreen_FontChar (fontChar, backing, x, y, mode,
				TFB_SCREEN_MAIN);
	}
	else
	{
		TFB_DrawImage_FontChar (fontChar, backing, x, y, mode,
				_CurFramePtr->image);
	}
}

// Text rendering is in font.c, under the name _text_blt
