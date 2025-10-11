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
#include "gfx_common.h"
#include "tfb_draw.h"
#include "tfb_prim.h"
#include "uqm/units.h"

HOT_SPOT
MAKE_HOT_SPOT (COORD x, COORD y)
{
	HOT_SPOT hs;
	hs.x = x;
	hs.y = y;
	return hs;
}

// XXX: INTERNAL_PRIMITIVE and INTERNAL_PRIM_DESC are not used
typedef union
{
	POINT Point;
	STAMP Stamp;
	BRESENHAM_LINE Line;
	TEXT Text;
	RECT Rect;
} INTERNAL_PRIM_DESC;

typedef struct
{
	PRIM_LINKS Links;
	GRAPHICS_PRIM Type;
	Color Color;
	INTERNAL_PRIM_DESC Object;
} INTERNAL_PRIMITIVE;


// pValidRect or origin may be NULL
BOOLEAN
GetContextValidRect (RECT *pValidRect, POINT *origin)
{
	RECT tempRect;
	POINT tempPt;

	if (!pValidRect)
		pValidRect = &tempRect;
	if (!origin)
		origin = &tempPt;

	// Start with a rect the size of foreground frame
	pValidRect->corner.x = 0;
	pValidRect->corner.y = 0;
	pValidRect->extent = GetFrameBounds (_CurFramePtr);
	*origin = _CurFramePtr->HotSpot;

	if (_pCurContext->ClipRect.extent.width)
	{
		// If the cliprect is completely outside of the valid frame
		// bounds we have nothing to draw
		if (!BoxIntersect (&_pCurContext->ClipRect,
				pValidRect, pValidRect))
			return (FALSE);

		// Foreground frame hotspot defines a drawing position offset
		// WRT the context cliprect
		origin->x += _pCurContext->ClipRect.corner.x;
		origin->y += _pCurContext->ClipRect.corner.y;
	}

	return (TRUE);
}

static void
ClearBackGround (RECT *pClipRect)
{
	RECT clearRect;
	Color color = _get_context_bg_color ();
	clearRect.corner.x = 0;
	clearRect.corner.y = 0;
	clearRect.extent = pClipRect->extent;
	TFB_Prim_FillRect (&clearRect, color, DRAW_REPLACE_MODE,
			pClipRect->corner);
}

void
DrawBatch (PRIMITIVE *lpBasePrim, PRIM_LINKS PrimLinks, 
		BATCH_FLAGS BatchFlags)
{
	RECT ValidRect;
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (&ValidRect, &origin))
	{
		COUNT CurIndex;
		PRIMITIVE *lpPrim;
		DrawMode mode = _get_context_draw_mode ();

		BatchGraphics ();

		if (BatchFlags & BATCH_BUILD_PAGE)
		{
			ClearBackGround (&ValidRect);
		}

		CurIndex = GetPredLink (PrimLinks);

		for (; CurIndex != END_OF_LIST;
				CurIndex = GetSuccLink (GetPrimLinks (lpPrim)))
		{
			GRAPHICS_PRIM PrimType;
			PRIMITIVE *lpWorkPrim;
			RECT ClipRect;
			Color color;
			BYTE flags;

			lpPrim = &lpBasePrim[CurIndex];
			PrimType = GetPrimType (lpPrim);
			if (!ValidPrimType (PrimType))
				continue;
			flags = GetPrimFlags (lpPrim);

			lpWorkPrim = lpPrim;

			switch (PrimType)
			{
				case POINT_PRIM:
					color = GetPrimColor (lpWorkPrim);
					TFB_Prim_Point (&lpWorkPrim->Object.Point, color,
							mode, origin, FALSE);
					break;
				case STAMP_PRIM:
					if (flags & HYPER_TO_QUASI_COLOR)
						TFB_Prim_Stamp (&lpWorkPrim->Object.Stamp, 
							MAKE_DRAW_MODE (DRAW_HYPTOQUAS, TRANSFER_ALPHA),
							origin, flags & UNSCALED_STAMP);
					else
						TFB_Prim_Stamp (&lpWorkPrim->Object.Stamp, mode,
							origin, flags & UNSCALED_STAMP);
					break;
				case STAMPFILL_PRIM:
					color = GetPrimColor (lpWorkPrim);
					if (flags & HS_STARMASK)
						TFB_Prim_StampFill (&lpWorkPrim->Object.Stamp, color,
							MAKE_DRAW_MODE (DRAW_OVERLAY, TRANSFER_ALPHA), 
							origin, flags & UNSCALED_STAMP);
					else
						TFB_Prim_StampFill (&lpWorkPrim->Object.Stamp, color,
								mode, origin, flags & UNSCALED_STAMP);
					break;
				case LINE_PRIM:
					color = GetPrimColor (lpWorkPrim);
					TFB_Prim_Line (&lpWorkPrim->Object.Line, color,
							mode, origin, RES_BOOL (1, 3));
					break;
				case TEXT_PRIM:
					if (!TextRect (&lpWorkPrim->Object.Text, &ClipRect, NULL))
						continue;
					// ClipRect is relative to origin
					_text_blt (&ClipRect, &lpWorkPrim->Object.Text, origin);
					break;
				case RECT_PRIM:
					color = GetPrimColor (lpWorkPrim);
					TFB_Prim_Rect (&lpWorkPrim->Object.Rect, color,
							mode, origin, FALSE);
					break;
				case RECTFILL_PRIM:
					color = GetPrimColor (lpWorkPrim);
					TFB_Prim_FillRect (&lpWorkPrim->Object.Rect, color,
							mode, origin);
					break;
				case POINT_PRIM_HD:
					color = GetPrimColor (lpWorkPrim);
					TFB_Prim_Point (&lpWorkPrim->Object.Point, color,
							mode, origin, TRUE);
					break;
			}
		}

		UnbatchGraphics ();
	}
}

void
ClearDrawable (void)
{
	RECT ValidRect;

	if (GraphicsSystemActive () && GetContextValidRect (&ValidRect, NULL))
	{
		ClearBackGround (&ValidRect);
	}
}

void
ClearScreen (void)
{
	RECT ValidRect;

	if (!GraphicsSystemActive ())
		return;

	ValidRect.corner = MAKE_POINT (0, 0);
	ValidRect.extent.width = CanvasWidth;
	ValidRect.extent.height = CanvasHeight;

	ClearBackGround (&ValidRect);
}

void
DrawPoint (POINT *lpPoint)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		Color color = GetPrimColor (&_locPrim);
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_Point (lpPoint, color, mode, origin, FALSE);
	}
}

void
InstaPoint (int x, int y)
{
	POINT origin = { x, y };
	DrawPoint (&origin);
}

void
DrawRectangle (RECT *lpRect, BOOLEAN scaled)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		Color color = GetPrimColor (&_locPrim);
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_Rect (lpRect, color, mode, origin, scaled);
	}
}

void
InstaRect (int x, int y, int w, int h, BOOLEAN scaled)
{
	RECT r = { {x, y}, { w, h } };
	DrawRectangle (&r, scaled);
}

void
DrawFilledRectangle (RECT *lpRect)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		Color color = GetPrimColor (&_locPrim);
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_FillRect (lpRect, color, mode, origin);
	}
}

void
InstaFilledRect (int x, int y, int w, int h)
{
	RECT r = { {x, y}, { w, h } };
	DrawFilledRectangle (&r);
}

void
DrawLine (LINE *lpLine, BYTE thickness)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		Color color = GetPrimColor (&_locPrim);
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_Line (lpLine, color, mode, origin, thickness);
	}
}

void
InstaLine (int x1, int y1, int x2, int y2)
{
	LINE line = { { x1, y1 }, { x2, y2 } };
	DrawLine (&line, 1);
}

void
DrawStamp (STAMP *stmp)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_Stamp (stmp, mode, origin, FALSE);
	}
}

void
DrawFilledStamp (STAMP *stmp)
{
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		Color color = GetPrimColor (&_locPrim);
		DrawMode mode = _get_context_draw_mode ();
		TFB_Prim_StampFill (stmp, color, mode, origin, FALSE);
	}
}

// Kruzen: Permanently applies layer frame to base via masking
// until base frame is unloaded from memory.
// Layer frame should be the same size or larger that base frame.
// Works with paletted but only with REPLACE mode (doesn't alter the palette).
// If layer frame is NULL and color pointer is not NULL - blend will be
// applied to every pixel equally with alpha = 255
void
ApplyMask (FRAME layer, FRAME base, DrawMode mode, Color *fill)
{	
	POINT origin;

	if (GraphicsSystemActive () && GetContextValidRect (NULL, &origin))
	{
		TFB_Prim_MaskFrame (layer, base, mode, fill);
	}
}

